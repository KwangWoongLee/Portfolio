#include "CorePch.h"
#include "WorldActor.h"
#include "CmsManager.h"
#include "DbCompletionTarget.h"
#include "DbDispatcher.h"
#include "PlayerPost.h"
#include "Reward/SiegeRewardClaimRepository.h"
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

    class PersistSiegeRewardClaimsCommand final
        : public IDbCommand
    {
    public:
        PersistSiegeRewardClaimsCommand(
            DbCompletionTarget completionTarget,
            std::shared_ptr<ISiegeRewardClaimRepository> repository,
            SiegeRewardJob job)
            : _completionTarget(completionTarget)
            , _repository(std::move(repository))
            , _job(std::move(job))
        {
        }

        EDbCommandResult Execute() override
        {
            if (not _repository)
            {
                _result = SiegeRewardClaimRepositoryResult{
                    ESiegeRewardClaimRepositoryError::InvalidArgument,
                    "siege reward claim repository unavailable",
                    _job._claims.size(),
                };
                return EDbCommandResult::Failed;
            }

            _result = _repository->PrepareClaims(_job);
            return _result.Succeeded() ? EDbCommandResult::Succeeded : EDbCommandResult::Failed;
        }

        void OnCompleted(EDbCommandResult const result) override
        {
            (void)result;
            if (_completionTarget._type != EDbCompletionTargetType::Actor ||
                not _completionTarget.IsValid())
            {
                return;
            }

            SendToWorld(WorldId{ _completionTarget._id }, WorldMsg::SiegeRewardClaimsPersisted{
                _job._siegeWarId,
                _result._requestedCount,
                _result._persistedCount,
                _result.Succeeded(),
                std::move(_result._message),
            });
        }

    private:
        DbCompletionTarget _completionTarget;
        std::shared_ptr<ISiegeRewardClaimRepository> _repository;
        SiegeRewardJob _job;
        SiegeRewardClaimRepositoryResult _result;
    };

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

WorldActor::WorldActor(
    WorldId const worldId,
    std::shared_ptr<ISiegeRewardClaimRepository> siegeRewardClaimRepository)
    : _worldId(worldId)
    , _guildManager(worldId)
    , _siegeRewardPlanner(worldId)
    , _siegeRewardClaimRepository(std::move(siegeRewardClaimRepository))
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

    auto const& snapshot = msg._snapshot;
    std::cout << "[SiegeWarSnapshot] id=" << snapshot._siegeWarId
        << " type=" << snapshot._siegeWarType
        << " rev=" << snapshot._revision
        << " state=" << ToString(snapshot._state)
        << " step=" << ToString(snapshot._progressStep)
        << " defender=" << snapshot._defenderGuildId
        << " winner=" << snapshot._winnerGuildId
        << " reason=" << ToString(snapshot._endReason)
        << std::endl;

    if (msg._snapshot._state != ESiegeWarState::Finished &&
        msg._snapshot._state != ESiegeWarState::Canceled)
    {
        (void)ScheduleSiegeWarWakeUp(msg._snapshot._siegeWarId);
        return;
    }

    CancelSiegeWarWakeUpTimer(msg._snapshot._siegeWarId);
    CreateSiegeRewardJob(msg._snapshot);
}

void WorldActor::OnMessage(WorldMsg::SiegeRewardClaimsPersisted const& msg)
{
    std::cout << "[SiegeRewardClaimRepository] siegeWarId=" << msg._siegeWarId
        << " result=" << (msg._succeeded ? "Succeeded" : "Failed")
        << " requested=" << msg._requestedCount
        << " persisted=" << msg._persistedCount;
    if (not msg._message.empty())
    {
        std::cout << " message=" << msg._message;
    }
    std::cout << std::endl;
}

void WorldActor::OnMessage(WorldMsg::StartSiegeDemo const& msg)
{
    auto redGuildResult = _guildManager.CreateGuild(
        msg._redLeaderActorId,
        msg._redGuildName);
    if (not redGuildResult.Succeeded())
    {
        if (auto snapshot = _guildManager.GetGuildSnapshotByMember(msg._redLeaderActorId))
        {
            redGuildResult._guildId = snapshot->_guildId;
        }
    }

    auto blueGuildResult = _guildManager.CreateGuild(
        msg._blueLeaderActorId,
        msg._blueGuildName);
    if (not blueGuildResult.Succeeded())
    {
        if (auto snapshot = _guildManager.GetGuildSnapshotByMember(msg._blueLeaderActorId))
        {
            blueGuildResult._guildId = snapshot->_guildId;
        }
    }

    auto const redGuildId = redGuildResult._guildId;
    auto const blueGuildId = blueGuildResult._guildId;

    if (redGuildId == INVALID_GUILD_ID || blueGuildId == INVALID_GUILD_ID)
    {
        std::cout << "[SiegeDemo] Failed to prepare demo guilds" << std::endl;
        return;
    }

    auto const siegeWarId = _guildManager.RegisterSiegeWar(
        _worldId,
        msg._data,
        SiegeWar::Clock::now());
    if (siegeWarId == INVALID_SIEGE_WAR_ID)
    {
        std::cout << "[SiegeDemo] Failed to register demo siege type="
            << msg._data._type << std::endl;
        return;
    }

    std::cout << "[SiegeDemo] Started demo siege id=" << siegeWarId
        << " type=" << msg._data._type
        << " redGuild=" << redGuildId
        << " blueGuild=" << blueGuildId
        << std::endl;

    (void)ScheduleSiegeWarWakeUp(siegeWarId);

    TimerManager::Singleton::GetInstance().AddTimer(
        msg._redOccupationDelay,
        static_cast<int64_t>(siegeWarId),
        [worldId = _worldId, siegeWarId, redGuildId]()
        {
            std::cout << "[SiegeDemo] Red guild occupies siege point" << std::endl;
            SiegeWarTaskRunner::PostOccupied(
                worldId,
                siegeWarId,
                redGuildId,
                SiegeWar::Clock::now());
        });

    TimerManager::Singleton::GetInstance().AddTimer(
        msg._blueOccupationDelay,
        static_cast<int64_t>(siegeWarId),
        [worldId = _worldId, siegeWarId, blueGuildId]()
        {
            std::cout << "[SiegeDemo] Blue guild occupies siege point" << std::endl;
            SiegeWarTaskRunner::PostOccupied(
                worldId,
                siegeWarId,
                blueGuildId,
                SiegeWar::Clock::now());
        });
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

void WorldActor::CreateSiegeRewardJob(SiegeWarSnapshot const& snapshot)
{
    std::optional<GuildSnapshot> winnerGuildSnapshot;
    if (snapshot._winnerGuildId != INVALID_GUILD_ID)
    {
        winnerGuildSnapshot = _guildManager.GetGuildSnapshot(snapshot._winnerGuildId);
    }

    auto job = _siegeRewardPlanner.CreateFinishedSiegeJob(
        snapshot,
        winnerGuildSnapshot);
    if (not job)
    {
        return;
    }

    std::cout << "[SiegeRewardJob] siegeWarId=" << job->_siegeWarId
        << " world=" << job->_worldId
        << " winnerGuild=" << job->_winnerGuildId
        << " endReason=" << ToString(job->_endReason)
        << " state=" << ToString(job->_state)
        << " claims=" << job->_claims.size()
        << std::endl;

    for (RewardClaim const& claim : job->_claims)
    {
        std::cout << "[RewardClaim] id=" << claim._id
            << " eventId=" << claim._eventId
            << " character=" << claim._characterId
            << " guild=" << claim._guildId
            << " type=" << ToString(claim._rewardType)
            << " state=" << ToString(claim._state)
            << " gold=" << claim._goldAmount
            << std::endl;
    }

    if (job->_claims.empty())
    {
        return;
    }

    if (not _siegeRewardClaimRepository)
    {
        std::cout << "[SiegeRewardClaimRepository] unavailable siegeWarId="
            << job->_siegeWarId << std::endl;
        return;
    }

    auto command = std::make_unique<PersistSiegeRewardClaimsCommand>(
        DbCompletionTarget::Actor(static_cast<int64_t>(_worldId)),
        _siegeRewardClaimRepository,
        std::move(*job));
    if (not DbDispatcher::Singleton::GetInstance().Enqueue(
        static_cast<int64_t>(snapshot._siegeWarId),
        std::move(command)))
    {
        std::cout << "[SiegeRewardClaimRepository] enqueue failed siegeWarId="
            << snapshot._siegeWarId << std::endl;
    }
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
