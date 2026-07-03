#include "CorePch.h"
#include "WorldActor.h"
#include "CmsManager.h"
#include "PlayerPost.h"
#include "Siege/SiegeWarTaskRunner.h"
#include "TimerManager.h"
#include "WorldPost.h"
#include <ctime>

namespace
{
    std::chrono::milliseconds GetWakeUpDelay(SiegeWar::Clock::time_point const wakeUpTime)
    {
        auto const now = SiegeWar::Clock::now();
        if (wakeUpTime <= now)
        {
            return std::chrono::milliseconds(1);
        }

        auto const remaining = wakeUpTime - now;
        auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(remaining);
        if (delay < remaining)
        {
            ++delay;
        }
        if (delay <= std::chrono::milliseconds::zero())
        {
            return std::chrono::milliseconds(1);
        }
        return delay;
    }
    auto constexpr SIEGE_DECLARATION_PAYMENT_TIMEOUT = std::chrono::seconds(5);

    std::optional<std::chrono::milliseconds> GetNextScheduleDelay(
        SiegeScheduleData const& schedule)
    {
        auto const now = std::chrono::system_clock::now();
        auto const nowTime = std::chrono::system_clock::to_time_t(now);

        std::tm localNow{};
        if (0 != ::localtime_s(&localNow, &nowTime))
        {
            return std::nullopt;
        }

        std::tm next = localNow;
        next.tm_mday += (schedule._dayOfWeek - localNow.tm_wday + 7) % 7;
        next.tm_hour = schedule._startHour;
        next.tm_min = schedule._startMinute;
        next.tm_sec = 0;
        next.tm_isdst = -1;

        std::time_t nextTime{ std::mktime(&next) };
        if (nextTime == static_cast<std::time_t>(-1))
        {
            return std::nullopt;
        }

        auto nextTimePoint = std::chrono::system_clock::from_time_t(nextTime);
        if (nextTimePoint <= now)
        {
            next.tm_mday += 7;
            next.tm_isdst = -1;
            nextTime = std::mktime(&next);
            if (nextTime == static_cast<std::time_t>(-1))
            {
                return std::nullopt;
            }
            nextTimePoint = std::chrono::system_clock::from_time_t(nextTime);
        }

        auto const delay = std::chrono::duration_cast<std::chrono::milliseconds>(
            nextTimePoint - now);
        if (delay <= std::chrono::milliseconds::zero())
        {
            return std::chrono::milliseconds(1);
        }
        return delay;
    }
}

WorldActor::WorldActor(WorldId const worldId)
    : _worldId(worldId)
    , _guildManager(worldId)
{
    for (auto const* schedule : CmsManager::Singleton::GetConstInstance().GetAllSiegeSchedules())
    {
        if (schedule)
        {
            (void)ScheduleNextSiege(*schedule);
        }
    }
}

WorldActor::~WorldActor()
{
    for (auto const& [scheduleType, timerId] : _siegeScheduleTimerIds)
    {
        (void)scheduleType;
        TimerManager::Singleton::GetInstance().CancelTimer(timerId);
    }

    for (auto const& [siegeWarId, timerId] : _siegeWarWakeUpTimerIds)
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
    auto const result = _guildManager.CompleteSiegeDeclaration(
        msg._declarationId,
        msg._requesterActorId,
        msg._paid);

    if (result != ESiegeDeclarationCompletion::RequesterMismatch &&
        result != ESiegeDeclarationCompletion::NotFound)
    {
        CancelSiegeDeclarationPaymentTimer(msg._declarationId);
    }

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
        msg._scheduledAt);
    if (siegeWarId == INVALID_SIEGE_WAR_ID)
    {
        return;
    }

    (void)ScheduleSiegeWarWakeUp(siegeWarId);
}

void WorldActor::OnMessage(WorldMsg::SiegeScheduleTriggered const& msg)
{
    auto const timerIter = _siegeScheduleTimerIds.find(msg._scheduleType);
    if (timerIter == _siegeScheduleTimerIds.end())
    {
        return;
    }
    _siegeScheduleTimerIds.erase(timerIter);

    auto const* schedule = CmsManager::Singleton::GetConstInstance().FindSiegeSchedule(
        msg._scheduleType);
    if (not schedule)
    {
        return;
    }

    auto const scheduledAt = SiegeWar::Clock::now();
    for (SiegeWarType const siegeWarType : schedule->_siegeWarTypes)
    {
        auto const* siegeWarData = CmsManager::Singleton::GetConstInstance().FindSiegeWar(
            siegeWarType);
        if (not siegeWarData)
        {
            continue;
        }

        OnMessage(WorldMsg::RegisterSiegeWar{
            *siegeWarData,
            scheduledAt,
        });
    }

    (void)ScheduleNextSiege(*schedule);
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
        (void)ScheduleSiegeWarWakeUp(msg._snapshot._siegeWarId);
        return;
    }

    CancelSiegeWarWakeUpTimer(msg._snapshot._siegeWarId);

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

void WorldActor::CancelSiegeWarWakeUpTimer(SiegeWarId const siegeWarId)
{
    auto const iter = _siegeWarWakeUpTimerIds.find(siegeWarId);
    if (iter == _siegeWarWakeUpTimerIds.end())
    {
        return;
    }

    TimerManager::Singleton::GetInstance().CancelTimer(iter->second);
    _siegeWarWakeUpTimerIds.erase(iter);
}

bool WorldActor::ScheduleSiegeWarWakeUp(SiegeWarId const siegeWarId)
{
    auto const siegeWar = _guildManager.FindSiegeWarInternal(siegeWarId);
    if (not siegeWar)
    {
        CancelSiegeWarWakeUpTimer(siegeWarId);
        return false;
    }

    auto const wakeUpTime = siegeWar->GetNextWakeUpTime();
    if (not wakeUpTime)
    {
        CancelSiegeWarWakeUpTimer(siegeWarId);
        return false;
    }

    CancelSiegeWarWakeUpTimer(siegeWarId);

    auto const timerId = TimerManager::Singleton::GetInstance().AddTimer(
        GetWakeUpDelay(*wakeUpTime),
        static_cast<int64_t>(siegeWarId),
        [worldId = _worldId, siegeWarId]()
        {
            SiegeWarTaskRunner::PostTick(worldId, siegeWarId, SiegeWar::Clock::now());
        });
    _siegeWarWakeUpTimerIds.emplace(siegeWarId, timerId);
    return true;
}

bool WorldActor::ScheduleNextSiege(SiegeScheduleData const& schedule)
{
    auto const delay = GetNextScheduleDelay(schedule);
    if (not delay)
    {
        return false;
    }

    auto const timerId = TimerManager::Singleton::GetInstance().AddTimer(
        *delay,
        static_cast<int64_t>(_worldId),
        [worldId = _worldId, scheduleType = schedule._type]()
        {
            SendToWorld(worldId, WorldMsg::SiegeScheduleTriggered{
                scheduleType,
            });
        });
    _siegeScheduleTimerIds.insert_or_assign(schedule._type, timerId);
    return true;
}
