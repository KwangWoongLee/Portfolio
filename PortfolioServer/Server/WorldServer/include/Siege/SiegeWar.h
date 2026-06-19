#pragma once
#include "CorePch.h"
#include "SiegeWarData.h"
#include "StateMachine.h"

using GuildId = int64_t;
inline GuildId constexpr INVALID_GUILD_ID = 0;

enum class ESiegeWarState : uint8_t
{
    Scheduled,
    Prepare,
    InProgress,
    Finished,
    Canceled,
};

enum class ESiegeWarProgressStep : uint8_t
{
    None,
    AttackWindow,
    OccupationGrace,
};

enum class ESiegeWarEndReason : uint8_t
{
    None,
    DefenderVictory,
    DrawNoOwner,
    Canceled,
};

char const* ToString(ESiegeWarState const state);
char const* ToString(ESiegeWarProgressStep const step);
char const* ToString(ESiegeWarEndReason const reason);

class SiegeWar final
{
public:
    using Clock = std::chrono::steady_clock;

    SiegeWar(
        SiegeWarData data,
        Clock::time_point scheduledAt,
        GuildId initialDefenderGuildId = INVALID_GUILD_ID);

    SiegeWar(SiegeWar const&) = delete;
    SiegeWar& operator=(SiegeWar const&) = delete;
    SiegeWar(SiegeWar&&) = delete;
    SiegeWar& operator=(SiegeWar&&) = delete;

    ESiegeWarState GetState() const;
    ESiegeWarProgressStep GetProgressStep() const;
    ESiegeWarEndReason GetEndReason() const;
    GuildId GetDefenderGuildId() const;
    GuildId GetWinnerGuildId() const;

    StateTransitionResult<ESiegeWarState> Tick(Clock::time_point now);
    StateTransitionResult<ESiegeWarState> Cancel(std::string reason, Clock::time_point now);

    bool OnOccupied(GuildId guildId, Clock::time_point now);

private:
    struct TickContext final
    {
        SiegeWar& _war;
        Clock::time_point _now;
    };

    using SiegeStateMachine = StateMachine<ESiegeWarState, TickContext>;

    void ConfigureStateMachine();

    static std::optional<SiegeStateMachine::TransitionRequest> TickScheduled(TickContext& context);
    static std::optional<SiegeStateMachine::TransitionRequest> TickPrepare(TickContext& context);
    static std::optional<SiegeStateMachine::TransitionRequest> TickInProgress(TickContext& context);

    static void EnterPrepare(TickContext& context);
    static void EnterInProgress(TickContext& context);
    static void EnterFinished(TickContext& context);
    static void EnterCanceled(TickContext& context);

    bool HasElapsed(Clock::time_point from, Clock::time_point now, int32_t durationSec) const;
    void BeginAttackWindow(Clock::time_point now);
    void BeginOccupationGrace(Clock::time_point now);
    void ResolveByCurrentDefender();
    void ResolveAsDraw();

    SiegeWarData _data;
    Clock::time_point _scheduledAt;
    Clock::time_point _phaseStartedAt;
    Clock::time_point _stepStartedAt;
    std::optional<Clock::time_point> _finishedAt;

    GuildId _defenderGuildId{ INVALID_GUILD_ID };
    GuildId _winnerGuildId{ INVALID_GUILD_ID };
    ESiegeWarProgressStep _progressStep{ ESiegeWarProgressStep::None };
    ESiegeWarEndReason _endReason{ ESiegeWarEndReason::None };

    SiegeStateMachine _stateMachine;
};
