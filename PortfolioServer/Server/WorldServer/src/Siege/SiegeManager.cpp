#include "CorePch.h"
#include "Siege/SiegeManager.h"
#include "UniqueIdGenerator.h"

SiegeManager::SiegeManager(WorldId const worldId)
    : _worldId(worldId)
{
}

SiegeWarId SiegeManager::RegisterSiegeWar(
    WorldId const worldId,
    SiegeWarData data,
    SiegeWar::Clock::time_point const scheduledAt,
    GuildId const initialDefenderGuildId)
{
    if (worldId != _worldId || _currentSiegeWarIds.contains(data._type))
    {
        return INVALID_SIEGE_WAR_ID;
    }

    auto const siegeWarIdValue = UniqueIdGenerator::Generate(
        EUniqueIdKind::SiegeWar,
        static_cast<int64_t>(_worldId));
    if (not siegeWarIdValue)
    {
        return INVALID_SIEGE_WAR_ID;
    }

    auto const siegeWarId = SiegeWarId{ *siegeWarIdValue };
    auto const declarationCostGold = data._declarationCostGold;
    auto siegeWar = std::make_shared<SiegeWar>(
        worldId,
        siegeWarId,
        std::move(data),
        scheduledAt,
        initialDefenderGuildId);

    auto const snapshot = siegeWar->CreateSnapshot();
    _currentSiegeWarIds.emplace(snapshot._siegeWarType, siegeWarId);
    _siegeWars.emplace(siegeWarId, SiegeWarEntry{
        std::move(siegeWar),
        snapshot,
        {},
        {},
        declarationCostGold,
    });

    return siegeWarId;
}

std::optional<SiegeDeclarationPayment> SiegeManager::TryReserveDeclaration(
    SiegeWarType const siegeWarType,
    GuildId const guildId,
    ActorId const requesterActorId)
{
    auto const currentIter = _currentSiegeWarIds.find(siegeWarType);
    if (currentIter == _currentSiegeWarIds.end())
    {
        return std::nullopt;
    }

    auto siegeIter = _siegeWars.find(currentIter->second);
    if (siegeIter == _siegeWars.end())
    {
        return std::nullopt;
    }

    auto& entry = siegeIter->second;
    if (entry._participantsFrozen ||
        (entry._snapshot._state != ESiegeWarState::Scheduled &&
         entry._snapshot._state != ESiegeWarState::Prepare))
    {
        return std::nullopt;
    }

    if (entry._declarationIdsByGuild.contains(guildId))
    {
        return std::nullopt;
    }

    auto const declarationIdValue = UniqueIdGenerator::Generate(
        EUniqueIdKind::SiegeDeclaration,
        static_cast<int64_t>(_worldId));
    if (not declarationIdValue)
    {
        return std::nullopt;
    }

    auto const declarationId = SiegeDeclarationId{ *declarationIdValue };
    SiegeDeclarationPayment const payment{
        declarationId,
        requesterActorId,
        guildId,
        entry._declarationCostGold,
    };

    entry._declarationIdsByGuild.emplace(guildId, declarationId);
    _declarations.emplace(declarationId, SiegeDeclarationEntry{
        currentIter->second,
        payment,
    });
    return payment;
}

ESiegeDeclarationCompletion SiegeManager::CompleteDeclaration(
    SiegeDeclarationId const declarationId,
    ActorId const requesterActorId,
    bool const paid)
{
    auto declarationIter = _declarations.find(declarationId);
    if (declarationIter == _declarations.end())
    {
        return ESiegeDeclarationCompletion::NotFound;
    }

    auto& declaration = declarationIter->second;
    if (declaration._payment._requesterActorId != requesterActorId)
    {
        return ESiegeDeclarationCompletion::RequesterMismatch;
    }

    if (declaration._state == ESiegeDeclarationState::Declared)
    {
        return ESiegeDeclarationCompletion::AlreadyCompleted;
    }

    auto siegeIter = _siegeWars.find(declaration._siegeWarId);
    if (siegeIter == _siegeWars.end())
    {
        return ESiegeDeclarationCompletion::NotFound;
    }

    auto& siegeEntry = siegeIter->second;
    if (not paid)
    {
        siegeEntry._declarationIdsByGuild.erase(declaration._payment._guildId);
        declaration._state = ESiegeDeclarationState::Canceled;
        return ESiegeDeclarationCompletion::Canceled;
    }

    if (declaration._state == ESiegeDeclarationState::Canceled)
    {
        return ESiegeDeclarationCompletion::NotFound;
    }

    declaration._state = ESiegeDeclarationState::Declared;
    siegeEntry._declaredGuildIds.insert(declaration._payment._guildId);
    return ESiegeDeclarationCompletion::Completed;
}

void SiegeManager::CancelGuildDeclarations(GuildId const guildId)
{
    for (auto& [siegeWarId, siegeEntry] : _siegeWars)
    {
        (void)siegeWarId;
        auto const declarationIdIter = siegeEntry._declarationIdsByGuild.find(guildId);
        if (declarationIdIter == siegeEntry._declarationIdsByGuild.end())
        {
            continue;
        }

        auto declarationIter = _declarations.find(declarationIdIter->second);
        if (declarationIter != _declarations.end())
        {
            declarationIter->second._state = ESiegeDeclarationState::Canceled;
        }

        siegeEntry._declaredGuildIds.erase(guildId);
        siegeEntry._declarationIdsByGuild.erase(declarationIdIter);
    }
}

bool SiegeManager::IsGuildParticipationLocked(GuildId const guildId) const
{
    return _guildParticipationLockCounts.contains(guildId);
}

bool SiegeManager::ApplySnapshot(SiegeWarSnapshot snapshot)
{
    auto iter = _siegeWars.find(snapshot._siegeWarId);
    if (iter == _siegeWars.end())
    {
        return false;
    }

    auto& entry = iter->second;
    if (snapshot._revision <= entry._snapshot._revision)
    {
        return false;
    }

    auto const previousState = entry._snapshot._state;
    entry._snapshot = std::move(snapshot);

    if (previousState != ESiegeWarState::InProgress &&
        entry._snapshot._state == ESiegeWarState::InProgress)
    {
        FreezeParticipants(entry);
    }

    if (entry._snapshot._state == ESiegeWarState::Finished ||
        entry._snapshot._state == ESiegeWarState::Canceled)
    {
        ReleaseParticipants(entry);

        auto const currentIter = _currentSiegeWarIds.find(entry._snapshot._siegeWarType);
        if (currentIter != _currentSiegeWarIds.end() &&
            currentIter->second == entry._snapshot._siegeWarId)
        {
            _currentSiegeWarIds.erase(currentIter);
        }
    }

    return true;
}

std::shared_ptr<SiegeWar> SiegeManager::FindSiegeWar(SiegeWarId const siegeWarId) const
{
    auto const iter = _siegeWars.find(siegeWarId);
    if (iter == _siegeWars.end())
    {
        return nullptr;
    }

    return iter->second._siegeWar;
}

std::optional<SiegeWarSnapshot> SiegeManager::GetSnapshot(SiegeWarId const siegeWarId) const
{
    auto const iter = _siegeWars.find(siegeWarId);
    if (iter == _siegeWars.end())
    {
        return std::nullopt;
    }

    return iter->second._snapshot;
}

void SiegeManager::FreezeParticipants(SiegeWarEntry& entry)
{
    if (entry._participantsFrozen)
    {
        return;
    }

    for (GuildId const guildId : entry._declaredGuildIds)
    {
        ++_guildParticipationLockCounts[guildId];
    }
    entry._participantsFrozen = true;
}

void SiegeManager::ReleaseParticipants(SiegeWarEntry& entry)
{
    if (not entry._participantsFrozen)
    {
        return;
    }

    for (GuildId const guildId : entry._declaredGuildIds)
    {
        auto iter = _guildParticipationLockCounts.find(guildId);
        if (iter == _guildParticipationLockCounts.end())
        {
            continue;
        }

        if (iter->second <= 1)
        {
            _guildParticipationLockCounts.erase(iter);
            continue;
        }

        --iter->second;
    }
    entry._participantsFrozen = false;
}
