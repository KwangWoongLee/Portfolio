#include "CorePch.h"
#include "Siege/SiegeWar.h"

namespace
{
    bool IsValidGuildId(GuildId const guildId)
    {
        return guildId != INVALID_GUILD_ID;
    }
}

char const* ToString(ESiegeWarState const state)
{
    switch (state)
    {
    case ESiegeWarState::Scheduled:
        return "Scheduled";
    case ESiegeWarState::Prepare:
        return "Prepare";
    case ESiegeWarState::InProgress:
        return "InProgress";
    case ESiegeWarState::Finished:
        return "Finished";
    case ESiegeWarState::Canceled:
        return "Canceled";
    default:
        return "Unknown";
    }
}

char const* ToString(ESiegeWarProgressStep const step)
{
    switch (step)
    {
    case ESiegeWarProgressStep::None:
        return "None";
    case ESiegeWarProgressStep::AttackWindow:
        return "AttackWindow";
    case ESiegeWarProgressStep::OccupationGrace:
        return "OccupationGrace";
    default:
        return "Unknown";
    }
}

char const* ToString(ESiegeWarEndReason const reason)
{
    switch (reason)
    {
    case ESiegeWarEndReason::None:
        return "None";
    case ESiegeWarEndReason::DefenderVictory:
        return "DefenderVictory";
    case ESiegeWarEndReason::DrawNoOwner:
        return "DrawNoOwner";
    case ESiegeWarEndReason::Canceled:
        return "Canceled";
    default:
        return "Unknown";
    }
}

SiegeWar::SiegeWar(
    WorldId const worldId,
    SiegeWarId const siegeWarId,
    SiegeWarData data,
    Clock::time_point const scheduledAt,
    GuildId const initialDefenderGuildId)
    : _worldId(worldId)
    , _siegeWarId(siegeWarId)
    , _data(std::move(data))
    , _scheduledAt(scheduledAt)
    , _phaseStartedAt(scheduledAt)
    , _stepStartedAt(scheduledAt)
    , _defenderGuildId(initialDefenderGuildId)
    , _stateMachine(ESiegeWarState::Scheduled)
{
    ConfigureStateMachine();
}

ESiegeWarState SiegeWar::GetState() const
{
    return _stateMachine.GetState();
}

ESiegeWarProgressStep SiegeWar::GetProgressStep() const
{
    return _progressStep;
}

ESiegeWarEndReason SiegeWar::GetEndReason() const
{
    return _endReason;
}

GuildId SiegeWar::GetDefenderGuildId() const
{
    return _defenderGuildId;
}

GuildId SiegeWar::GetWinnerGuildId() const
{
    return _winnerGuildId;
}

SiegeWarSnapshot SiegeWar::CreateSnapshot() const
{
    return SiegeWarSnapshot{
        _worldId,
        _siegeWarId,
        _data._type,
        _revision,
        _stateMachine.GetState(),
        _progressStep,
        _endReason,
        _defenderGuildId,
        _winnerGuildId,
    };
}

StateTransitionResult<ESiegeWarState> SiegeWar::Tick(Clock::time_point const now)
{
    TickContext context{ *this, now };
    auto result = _stateMachine.Tick(context);
    if (result.Succeeded())
    {
        ++_revision;
    }
    return result;
}

StateTransitionResult<ESiegeWarState> SiegeWar::Cancel(std::string reason, Clock::time_point const now)
{
    TickContext context{ *this, now };
    auto result = _stateMachine.TryTransition(ESiegeWarState::Canceled, std::move(reason), context);
    if (result.Succeeded())
    {
        ++_revision;
    }
    return result;
}

bool SiegeWar::OnOccupied(GuildId const guildId, Clock::time_point const now)
{
    if (not _stateMachine.IsInState(ESiegeWarState::InProgress) || not IsValidGuildId(guildId))
    {
        return false;
    }

    if (_progressStep == ESiegeWarProgressStep::OccupationGrace && _defenderGuildId != guildId)
    {
        _defenderGuildId = guildId;
        BeginAttackWindow(now);
        ++_revision;
        return true;
    }

    _defenderGuildId = guildId;
    BeginOccupationGrace(now);
    ++_revision;
    return true;
}

void SiegeWar::ConfigureStateMachine()
{
    _stateMachine.RegisterState(ESiegeWarState::Scheduled, SiegeStateMachine::StateHandlers{
        {},
        &SiegeWar::TickScheduled,
    });
    _stateMachine.RegisterState(ESiegeWarState::Prepare, SiegeStateMachine::StateHandlers{
        &SiegeWar::EnterPrepare,
        &SiegeWar::TickPrepare,
    });
    _stateMachine.RegisterState(ESiegeWarState::InProgress, SiegeStateMachine::StateHandlers{
        &SiegeWar::EnterInProgress,
        &SiegeWar::TickInProgress,
    });
    _stateMachine.RegisterState(ESiegeWarState::Finished, SiegeStateMachine::StateHandlers{
        &SiegeWar::EnterFinished,
    });
    _stateMachine.RegisterState(ESiegeWarState::Canceled, SiegeStateMachine::StateHandlers{
        &SiegeWar::EnterCanceled,
    });

    _stateMachine.AllowSequentialTransitions({
        ESiegeWarState::Scheduled,
        ESiegeWarState::Prepare,
        ESiegeWarState::InProgress,
        ESiegeWarState::Finished,
    });
    _stateMachine.AllowTransition(ESiegeWarState::Scheduled, ESiegeWarState::Canceled);
    _stateMachine.AllowTransition(ESiegeWarState::Prepare, ESiegeWarState::Canceled);
    _stateMachine.AllowTransition(ESiegeWarState::InProgress, ESiegeWarState::Canceled);

    _stateMachine.SetTransitionHook(
        [](StateTransitionResult<ESiegeWarState> const& result, TickContext& context)
        {
            std::cout << "[SiegeWar:" << context._war._data._type << "] "
                << ToString(result._from) << " -> " << ToString(result._to)
                << " reason=" << result._reason << std::endl;
        });
}

std::optional<SiegeWar::SiegeStateMachine::TransitionRequest> SiegeWar::TickScheduled(TickContext& context)
{
    if (context._now < context._war._scheduledAt)
    {
        return std::nullopt;
    }

    return SiegeStateMachine::TransitionRequest{
        ESiegeWarState::Prepare,
        "schedule reached",
    };
}

std::optional<SiegeWar::SiegeStateMachine::TransitionRequest> SiegeWar::TickPrepare(TickContext& context)
{
    if (not context._war.HasElapsed(
        context._war._phaseStartedAt,
        context._now,
        context._war._data._prepDurationSec))
    {
        return std::nullopt;
    }

    return SiegeStateMachine::TransitionRequest{
        ESiegeWarState::InProgress,
        "prepare duration elapsed",
    };
}

std::optional<SiegeWar::SiegeStateMachine::TransitionRequest> SiegeWar::TickInProgress(TickContext& context)
{
    auto& war = context._war;

    if (war._progressStep == ESiegeWarProgressStep::AttackWindow)
    {
        if (not war.HasElapsed(war._stepStartedAt, context._now, war._data._attackDurationSec))
        {
            return std::nullopt;
        }

        if (IsValidGuildId(war._defenderGuildId))
        {
            war.ResolveByCurrentDefender();
        }
        else
        {
            war.ResolveAsDraw();
        }

        return SiegeStateMachine::TransitionRequest{
            ESiegeWarState::Finished,
            "attack window elapsed",
        };
    }

    if (war._progressStep == ESiegeWarProgressStep::OccupationGrace)
    {
        if (not war.HasElapsed(war._stepStartedAt, context._now, war._data._swapSideDurationSec))
        {
            return std::nullopt;
        }

        war.ResolveByCurrentDefender();
        return SiegeStateMachine::TransitionRequest{
            ESiegeWarState::Finished,
            "occupation grace elapsed",
        };
    }

    return std::nullopt;
}

void SiegeWar::EnterPrepare(TickContext& context)
{
    context._war._phaseStartedAt = context._now;
    context._war._progressStep = ESiegeWarProgressStep::None;
}

void SiegeWar::EnterInProgress(TickContext& context)
{
    context._war._phaseStartedAt = context._now;
    context._war.BeginAttackWindow(context._now);
}

void SiegeWar::EnterFinished(TickContext& context)
{
    auto& war = context._war;
    war._finishedAt = context._now;
    war._progressStep = ESiegeWarProgressStep::None;

    // TODO: create reward job, tax payout ledger, and reward_claim rows.
    // SiegeWar stays Finished; retry/recovery belongs to reward job state.
}

void SiegeWar::EnterCanceled(TickContext& context)
{
    auto& war = context._war;
    war._finishedAt = context._now;
    war._progressStep = ESiegeWarProgressStep::None;
    war._endReason = ESiegeWarEndReason::Canceled;
    war._winnerGuildId = INVALID_GUILD_ID;
}

bool SiegeWar::HasElapsed(
    Clock::time_point const from,
    Clock::time_point const now,
    int32_t const durationSec) const
{
    if (durationSec <= 0)
    {
        return true;
    }

    return now - from >= std::chrono::seconds(durationSec);
}

void SiegeWar::BeginAttackWindow(Clock::time_point const now)
{
    _progressStep = ESiegeWarProgressStep::AttackWindow;
    _stepStartedAt = now;
}

void SiegeWar::BeginOccupationGrace(Clock::time_point const now)
{
    _progressStep = ESiegeWarProgressStep::OccupationGrace;
    _stepStartedAt = now;
}

void SiegeWar::ResolveByCurrentDefender()
{
    _endReason = ESiegeWarEndReason::DefenderVictory;
    _winnerGuildId = _defenderGuildId;
}

void SiegeWar::ResolveAsDraw()
{
    _endReason = ESiegeWarEndReason::DrawNoOwner;
    _winnerGuildId = INVALID_GUILD_ID;
}
