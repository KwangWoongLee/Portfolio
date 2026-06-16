#pragma once
#include "CorePch.h"
#include <initializer_list>

struct EmptyStateMachineContext final
{
};

enum class EStateTransitionError : uint8_t
{
    None,
    SameState,
    UnknownState,
    TransitionNotAllowed,
    GuardRejected,
    AlreadyTransitioning,
};

char const* ToString(EStateTransitionError const error);

template <typename T_STATE>
struct StateTransitionResult final
{
    bool _changed{ false };
    T_STATE _from{};
    T_STATE _to{};
    EStateTransitionError _error{ EStateTransitionError::None };
    std::string _reason;

    bool Succeeded() const;
    bool Failed() const;
};

// This is a pure state container. Own and tick it from one actor/task owner.
template <
    typename T_STATE,
    typename T_CONTEXT = EmptyStateMachineContext,
    typename T_STATE_HASH = std::hash<T_STATE>>
class StateMachine final
{
public:
    struct TransitionRequest final
    {
        T_STATE _to{};
        std::string _reason;
    };

    struct StateHandlers final
    {
        std::function<void(T_CONTEXT&)> _onEnter;
        std::function<std::optional<TransitionRequest>(T_CONTEXT&)> _onTick;
        std::function<void(T_CONTEXT&)> _onExit;
    };

    using TransitionGuard = std::function<bool(T_CONTEXT const&)>;
    using TransitionHook = std::function<void(StateTransitionResult<T_STATE> const&, T_CONTEXT&)>;

    explicit StateMachine(T_STATE const initialState);

    StateMachine(StateMachine const&) = delete;
    StateMachine& operator=(StateMachine const&) = delete;
    StateMachine(StateMachine&&) = delete;
    StateMachine& operator=(StateMachine&&) = delete;

    T_STATE GetState() const;
    bool IsInState(T_STATE const state) const;

    void RegisterState(T_STATE const state, StateHandlers handlers = {});
    void AllowTransition(
        T_STATE const from,
        T_STATE const to,
        TransitionGuard guard = {});
    void AllowSequentialTransitions(std::initializer_list<T_STATE> states);
    void SetTransitionHook(TransitionHook hook);

    StateTransitionResult<T_STATE> Tick(T_CONTEXT& context);
    StateTransitionResult<T_STATE> TryTransition(
        T_STATE const to,
        std::string reason,
        T_CONTEXT& context);

private:
    struct TransitionRule final
    {
        T_STATE _from{};
        T_STATE _to{};
        TransitionGuard _guard;
    };

    void EnsureState(T_STATE const state);
    std::optional<std::reference_wrapper<StateHandlers>> FindStateHandlers(T_STATE const state);
    std::optional<std::reference_wrapper<TransitionRule>> FindTransitionRule(T_STATE const from, T_STATE const to);
    std::optional<std::reference_wrapper<TransitionRule const>> FindTransitionRule(
        T_STATE const from,
        T_STATE const to) const;

    static StateTransitionResult<T_STATE> MakeNoChangeResult(T_STATE const state);
    static StateTransitionResult<T_STATE> MakeFailedResult(
        T_STATE const from,
        T_STATE const to,
        EStateTransitionError const error,
        std::string reason);

    T_STATE _currentState;
    std::unordered_map<T_STATE, StateHandlers, T_STATE_HASH> _states;
    std::vector<TransitionRule> _transitionRules;
    TransitionHook _onTransition;
    bool _isTransitioning{ false };
};

#include "StateMachine.inl"
