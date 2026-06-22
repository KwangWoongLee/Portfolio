#include "CorePch.h"
#include "WorldActor.h"
#include "PlayerPost.h"
#include "Siege/SiegeWarTaskRunner.h"
#include "TimerManager.h"

namespace
{
    auto constexpr SIEGE_WAR_TICK_INTERVAL = std::chrono::seconds(1);
    auto constexpr SIEGE_DECLARATION_PAYMENT_TIMEOUT = std::chrono::seconds(5);
}

WorldActor::~WorldActor()
{
    for (auto const& [siegeWarId, timerId] : _siegeWarTimerIds)
    {
        (void)siegeWarId;
        TimerManager::Singleton::GetInstance().CancelTimer(timerId);
    }

    for (auto const& [declarationId, timerId] : _siegeDeclarationPaymentTimerIds)
    {
        (void)declarationId;
        TimerManager::Singleton::GetInstance().CancelTimer(timerId);
    }
}

std::optional<GuildSnapshot> WorldActor::GetGuildSnapshot(GuildId const guildId) const
{
    return _guildManager.GetGuildSnapshot(guildId);
}

std::optional<GuildSnapshot> WorldActor::GetGuildSnapshotByMember(ActorId const actorId) const
{
    return _guildManager.GetGuildSnapshotByMember(actorId);
}

std::optional<SiegeWarSnapshot> WorldActor::GetSiegeWarSnapshot(SiegeWarId const siegeWarId) const
{
    return _guildManager.GetSiegeWarSnapshot(siegeWarId);
}

void WorldActor::OnMessage(WorldMsg::CreateGuild const& msg)
{
    (void)_guildManager.CreateGuild(msg._leaderActorId, msg._name);
}

void WorldActor::OnMessage(WorldMsg::JoinGuild const& msg)
{
    (void)_guildManager.JoinGuild(msg._actorId, msg._guildId);
}

void WorldActor::OnMessage(WorldMsg::LeaveGuild const& msg)
{
    (void)_guildManager.LeaveGuild(msg._actorId, msg._successorActorId);
}

void WorldActor::OnMessage(WorldMsg::TransferGuildLeader const& msg)
{
    (void)_guildManager.TransferLeader(msg._leaderActorId, msg._successorActorId);
}

void WorldActor::OnMessage(WorldMsg::DeclareSiege const& msg)
{
    auto const payment = _guildManager.TryReserveSiegeDeclaration(
        msg._requesterActorId,
        msg._siegeWarType);
    if (not payment)
    {
        return;
    }

    auto const paymentTimerId = TimerManager::Singleton::GetInstance().AddTimer(
        std::chrono::duration_cast<std::chrono::milliseconds>(SIEGE_DECLARATION_PAYMENT_TIMEOUT),
        static_cast<int64_t>(payment->_declarationId),
        [worldId = _worldId,
         requesterActorId = msg._requesterActorId,
         declarationId = payment->_declarationId]()
        {
            SendToWorld(worldId, WorldMsg::SiegeDeclarationPaymentTimedOut{
                requesterActorId,
                declarationId,
            });
        });
    _siegeDeclarationPaymentTimerIds.emplace(payment->_declarationId, paymentTimerId);

    SendToPlayer(msg._requesterActorId, PlayerMsg::SiegeDeclarationPaymentRequested{
        _worldId,
        payment->_declarationId,
        payment->_costGold,
    });
}

void WorldActor::OnMessage(WorldMsg::SiegeDeclarationPaymentCompleted const& msg)
{
    CancelSiegeDeclarationPaymentTimer(msg._declarationId);

    auto const result = _guildManager.CompleteSiegeDeclaration(
        msg._declarationId,
        msg._requesterActorId,
        msg._paid);

    if (msg._paid &&
        result != ESiegeDeclarationCompletion::Completed &&
        result != ESiegeDeclarationCompletion::AlreadyCompleted)
    {
        SendToPlayer(msg._requesterActorId, PlayerMsg::SiegeDeclarationRefundRequested{
            msg._declarationId,
            msg._costGold,
        });
    }
}

void WorldActor::OnMessage(WorldMsg::SiegeDeclarationPaymentTimedOut const& msg)
{
    CancelSiegeDeclarationPaymentTimer(msg._declarationId);
    (void)_guildManager.CompleteSiegeDeclaration(
        msg._declarationId,
        msg._requesterActorId,
        false);
}

void WorldActor::OnMessage(WorldMsg::RegisterSiegeWar const& msg)
{
    auto const siegeWarId = _guildManager.RegisterSiegeWar(
        _worldId,
        msg._data,
        msg._scheduledAt,
        msg._initialDefenderGuildId);
    if (siegeWarId == INVALID_SIEGE_WAR_ID)
    {
        return;
    }

    auto const timerId = TimerManager::Singleton::GetInstance().AddRepeatTimer(
        std::chrono::duration_cast<std::chrono::milliseconds>(SIEGE_WAR_TICK_INTERVAL),
        static_cast<int64_t>(siegeWarId),
        [worldId = _worldId, siegeWarId]()
        {
            SiegeWarTaskRunner::PostTick(worldId, siegeWarId, SiegeWar::Clock::now());
        });
    _siegeWarTimerIds.emplace(siegeWarId, timerId);
}

void WorldActor::OnMessage(WorldMsg::SiegeWarSnapshotUpdated const& msg)
{
    if (not _guildManager.ApplySiegeWarSnapshot(msg._snapshot))
    {
        return;
    }

    if (msg._snapshot._state != ESiegeWarState::Finished &&
        msg._snapshot._state != ESiegeWarState::Canceled)
    {
        return;
    }

    auto const timerIter = _siegeWarTimerIds.find(msg._snapshot._siegeWarId);
    if (timerIter == _siegeWarTimerIds.end())
    {
        return;
    }

    TimerManager::Singleton::GetInstance().CancelTimer(timerIter->second);
    _siegeWarTimerIds.erase(timerIter);

    // TODO: enqueue final siege result persistence and reward/tax settlement jobs.
}

std::shared_ptr<SiegeWar> WorldActor::FindSiegeWarInternal(SiegeWarId const siegeWarId) const
{
    return _guildManager.FindSiegeWarInternal(siegeWarId);
}

void WorldActor::CancelSiegeDeclarationPaymentTimer(
    SiegeDeclarationId const declarationId)
{
    auto const iter = _siegeDeclarationPaymentTimerIds.find(declarationId);
    if (iter == _siegeDeclarationPaymentTimerIds.end())
    {
        return;
    }

    TimerManager::Singleton::GetInstance().CancelTimer(iter->second);
    _siegeDeclarationPaymentTimerIds.erase(iter);
}
