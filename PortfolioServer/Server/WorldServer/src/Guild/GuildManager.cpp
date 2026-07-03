#include "CorePch.h"
#include "Guild/GuildManager.h"
#include "UniqueIdGenerator.h"

GuildManager::GuildManager(WorldId const worldId)
    : _worldId(worldId)
    , _siegeManager(worldId)
{
}

std::optional<GuildSnapshot> GuildManager::GetGuildSnapshot(GuildId const guildId) const
{
    std::shared_lock lock(_mutex);
    auto const iter = _guilds.find(guildId);
    if (iter == _guilds.end())
    {
        return std::nullopt;
    }

    return iter->second->CreateSnapshot();
}

std::optional<GuildSnapshot> GuildManager::GetGuildSnapshotByMember(ActorId const actorId) const
{
    std::shared_lock lock(_mutex);
    auto const guildIdIter = _guildIdsByMember.find(actorId);
    if (guildIdIter == _guildIdsByMember.end())
    {
        return std::nullopt;
    }

    auto const guildIter = _guilds.find(guildIdIter->second);
    if (guildIter == _guilds.end())
    {
        return std::nullopt;
    }

    return guildIter->second->CreateSnapshot();
}

std::optional<SiegeWarSnapshot> GuildManager::GetSiegeWarSnapshot(SiegeWarId const siegeWarId) const
{
    std::shared_lock lock(_mutex);
    return _siegeManager.GetSnapshot(siegeWarId);
}

GuildId GuildManager::GetSiegeDefenderGuildId(SiegeWarType const siegeWarType) const
{
    std::shared_lock lock(_mutex);
    return _siegeManager.GetDefenderGuildId(siegeWarType);
}

GuildOperationResult GuildManager::CreateGuild(ActorId const leaderActorId, std::string name)
{
    if (leaderActorId == INVALID_ACTOR_ID || name.empty())
    {
        return { EGuildOperationError::InvalidArgument };
    }

    std::unique_lock lock(_mutex);
    if (_guildIdsByMember.contains(leaderActorId))
    {
        return { EGuildOperationError::AlreadyInGuild };
    }

    if (_guildIdsByName.contains(name))
    {
        return { EGuildOperationError::GuildNameAlreadyExists };
    }

    auto const guildIdValue = UniqueIdGenerator::Generate(
        EUniqueIdKind::Guild,
        static_cast<int64_t>(_worldId));
    if (not guildIdValue)
    {
        return { EGuildOperationError::InvalidArgument };
    }

    auto const guildId = GuildId{ *guildIdValue };
    auto guild = std::make_shared<Guild>(guildId, name, leaderActorId);

    _guildIdsByMember.emplace(leaderActorId, guildId);
    _guildIdsByName.emplace(std::move(name), guildId);
    _guilds.emplace(guildId, std::move(guild));

    return { EGuildOperationError::None, guildId };
}

GuildOperationResult GuildManager::JoinGuild(ActorId const actorId, GuildId const guildId)
{
    if (actorId == INVALID_ACTOR_ID || guildId == INVALID_GUILD_ID)
    {
        return { EGuildOperationError::InvalidArgument };
    }

    std::unique_lock lock(_mutex);
    if (_guildIdsByMember.contains(actorId))
    {
        return { EGuildOperationError::AlreadyInGuild };
    }

    auto const guildIter = _guilds.find(guildId);
    if (guildIter == _guilds.end())
    {
        return { EGuildOperationError::GuildNotFound };
    }

    if (_siegeManager.IsGuildParticipationLocked(guildId))
    {
        return {
            EGuildOperationError::SiegeParticipationLocked,
            guildId,
        };
    }

    auto const error = guildIter->second->AddMember(actorId);
    if (error != EGuildOperationError::None)
    {
        return { error, guildId };
    }

    _guildIdsByMember.emplace(actorId, guildId);
    return { EGuildOperationError::None, guildId };
}

GuildOperationResult GuildManager::LeaveGuild(
    ActorId const actorId,
    std::optional<ActorId> const successorActorId)
{
    std::unique_lock lock(_mutex);
    auto const memberIter = _guildIdsByMember.find(actorId);
    if (memberIter == _guildIdsByMember.end())
    {
        return { EGuildOperationError::NotGuildMember };
    }

    auto const guildId = memberIter->second;
    auto const guildIter = _guilds.find(guildId);
    if (guildIter == _guilds.end())
    {
        return { EGuildOperationError::GuildNotFound };
    }

    auto const before = guildIter->second->CreateSnapshot();
    if (_siegeManager.IsGuildParticipationLocked(guildId))
    {
        return {
            EGuildOperationError::SiegeParticipationLocked,
            guildId,
        };
    }

    bool disbanded{ false };
    auto const error = guildIter->second->RemoveMember(
        actorId,
        successorActorId,
        disbanded);
    if (error != EGuildOperationError::None)
    {
        return { error, guildId };
    }

    _guildIdsByMember.erase(memberIter);
    if (disbanded)
    {
        _siegeManager.CancelGuildDeclarations(guildId);
        _guildIdsByName.erase(before._name);
        _guilds.erase(guildIter);
    }

    return { EGuildOperationError::None, guildId };
}

GuildOperationResult GuildManager::TransferLeader(
    ActorId const leaderActorId,
    ActorId const successorActorId)
{
    std::unique_lock lock(_mutex);
    auto const memberIter = _guildIdsByMember.find(leaderActorId);
    if (memberIter == _guildIdsByMember.end())
    {
        return { EGuildOperationError::NotGuildMember };
    }

    auto const guildId = memberIter->second;
    auto const guildIter = _guilds.find(guildId);
    if (guildIter == _guilds.end())
    {
        return { EGuildOperationError::GuildNotFound };
    }

    if (_siegeManager.IsGuildParticipationLocked(guildId))
    {
        return {
            EGuildOperationError::SiegeParticipationLocked,
            guildId,
        };
    }

    auto const error = guildIter->second->TransferLeader(
        leaderActorId,
        successorActorId);
    return { error, guildId };
}

std::optional<SiegeDeclarationPayment> GuildManager::TryReserveSiegeDeclaration(
    ActorId const requesterActorId,
    SiegeWarType const siegeWarType)
{
    std::unique_lock lock(_mutex);
    auto const memberIter = _guildIdsByMember.find(requesterActorId);
    if (memberIter == _guildIdsByMember.end())
    {
        return std::nullopt;
    }

    auto const guildId = memberIter->second;
    auto const guildIter = _guilds.find(guildId);
    if (guildIter == _guilds.end())
    {
        return std::nullopt;
    }

    if (not guildIter->second->IsLeader(requesterActorId))
    {
        return std::nullopt;
    }

    return _siegeManager.TryReserveDeclaration(
        siegeWarType,
        guildId,
        requesterActorId);
}

ESiegeDeclarationCompletion GuildManager::CompleteSiegeDeclaration(
    SiegeDeclarationId const declarationId,
    ActorId const requesterActorId,
    bool const paid)
{
    std::unique_lock lock(_mutex);
    return _siegeManager.CompleteDeclaration(declarationId, requesterActorId, paid);
}

SiegeWarId GuildManager::RegisterSiegeWar(
    WorldId const worldId,
    SiegeWarData data,
    SiegeWar::Clock::time_point const scheduledAt)
{
    std::unique_lock lock(_mutex);
    return _siegeManager.RegisterSiegeWar(
        worldId,
        std::move(data),
        scheduledAt);
}

bool GuildManager::ApplySiegeWarSnapshot(SiegeWarSnapshot snapshot)
{
    std::unique_lock lock(_mutex);
    return _siegeManager.ApplySnapshot(std::move(snapshot));
}

std::shared_ptr<SiegeWar> GuildManager::FindSiegeWarInternal(SiegeWarId const siegeWarId) const
{
    std::shared_lock lock(_mutex);
    return _siegeManager.FindSiegeWar(siegeWarId);
}
