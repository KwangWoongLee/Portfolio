#pragma once

template <typename T_STATE>
bool StateTransitionResult<T_STATE>::Succeeded() const
{
    return _changed and _error == EStateTransitionError::None;
}

template <typename T_STATE>
bool StateTransitionResult<T_STATE>::Failed() const
{
    return _error != EStateTransitionError::None;
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::StateMachine(T_STATE const initialState)
    : _currentState(initialState)
{
    RegisterState(initialState);
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
T_STATE StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::GetState() const
{
    return _currentState;
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
bool StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::IsInState(T_STATE const state) const
{
    return _currentState == state;
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
void StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::RegisterState(
    T_STATE const state,
    typename StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::StateHandlers handlers)
{
    _states[state] = std::move(handlers);
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
void StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::AllowTransition(
    T_STATE const from,
    T_STATE const to,
    typename StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::TransitionGuard guard)
{
    EnsureState(from);
    EnsureState(to);

    auto rule = FindTransitionRule(from, to);
    if (rule)
    {
        rule->get()._guard = std::move(guard);
        return;
    }

    TransitionRule rule;
    rule._from = from;
    rule._to = to;
    rule._guard = std::move(guard);
    _transitionRules.emplace_back(std::move(rule));
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
void StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::AllowSequentialTransitions(
    std::initializer_list<T_STATE> states)
{
    if (states.size() < 2)
    {
        for (T_STATE const state : states)
        {
            EnsureState(state);
        }

        return;
    }

    auto iter = states.begin();
    T_STATE previous = *iter;
    EnsureState(previous);
    ++iter;

    for (; iter != states.end(); ++iter)
    {
        auto const current = *iter;
        AllowTransition(previous, current);
        previous = current;
    }
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
void StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::SetTransitionHook(
    typename StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::TransitionHook hook)
{
    _onTransition = std::move(hook);
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
StateTransitionResult<T_STATE> StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::Tick(T_CONTEXT& context)
{
    auto handlers = FindStateHandlers(_currentState);
    if (not handlers or not handlers->get()._onTick)
    {
        return MakeNoChangeResult(_currentState);
    }

    auto const request = handlers->get()._onTick(context);
    if (not request)
    {
        return MakeNoChangeResult(_currentState);
    }

    return TryTransition(request->_to, request->_reason, context);
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
StateTransitionResult<T_STATE> StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::TryTransition(
    T_STATE const to,
    std::string reason,
    T_CONTEXT& context)
{
    auto const from = _currentState;

    if (_isTransitioning)
    {
        return MakeFailedResult(from, to, EStateTransitionError::AlreadyTransitioning, std::move(reason));
    }

    if (from == to)
    {
        return MakeFailedResult(from, to, EStateTransitionError::SameState, std::move(reason));
    }

    auto fromHandlers = FindStateHandlers(from);
    auto toHandlers = FindStateHandlers(to);
    if (not fromHandlers or not toHandlers)
    {
        return MakeFailedResult(from, to, EStateTransitionError::UnknownState, std::move(reason));
    }

    auto rule = FindTransitionRule(from, to);
    if (not rule)
    {
        return MakeFailedResult(from, to, EStateTransitionError::TransitionNotAllowed, std::move(reason));
    }

    if (rule->get()._guard and not rule->get()._guard(context))
    {
        return MakeFailedResult(from, to, EStateTransitionError::GuardRejected, std::move(reason));
    }

    _isTransitioning = true;
    RAII transitionGuard([this]() { _isTransitioning = false; });

    if (fromHandlers->get()._onExit)
    {
        fromHandlers->get()._onExit(context);
    }

    _currentState = to;

    if (toHandlers->get()._onEnter)
    {
        toHandlers->get()._onEnter(context);
    }

    StateTransitionResult<T_STATE> result{
        true,
        from,
        to,
        EStateTransitionError::None,
        std::move(reason),
    };

    if (_onTransition)
    {
        _onTransition(result, context);
    }

    return result;
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
void StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::EnsureState(T_STATE const state)
{
    if (not _states.contains(state))
    {
        RegisterState(state);
    }
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
std::optional<std::reference_wrapper<typename StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::StateHandlers>>
StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::FindStateHandlers(T_STATE const state)
{
    auto iter = _states.find(state);
    if (iter == _states.end())
    {
        return std::nullopt;
    }

    return std::ref(iter->second);
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
std::optional<std::reference_wrapper<typename StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::TransitionRule>>
StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::FindTransitionRule(T_STATE const from, T_STATE const to)
{
    for (TransitionRule& rule : _transitionRules)
    {
        if (rule._from == from and rule._to == to)
        {
            return std::ref(rule);
        }
    }

    return std::nullopt;
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
std::optional<std::reference_wrapper<typename StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::TransitionRule const>>
StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::FindTransitionRule(T_STATE const from, T_STATE const to) const
{
    for (TransitionRule const& rule : _transitionRules)
    {
        if (rule._from == from and rule._to == to)
        {
            return std::cref(rule);
        }
    }

    return std::nullopt;
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
StateTransitionResult<T_STATE> StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::MakeNoChangeResult(
    T_STATE const state)
{
    return StateTransitionResult<T_STATE>{
        false,
        state,
        state,
        EStateTransitionError::None,
    };
}

template <typename T_STATE, typename T_CONTEXT, typename T_STATE_HASH>
StateTransitionResult<T_STATE> StateMachine<T_STATE, T_CONTEXT, T_STATE_HASH>::MakeFailedResult(
    T_STATE const from,
    T_STATE const to,
    EStateTransitionError const error,
    std::string reason)
{
    return StateTransitionResult<T_STATE>{
        false,
        from,
        to,
        error,
        std::move(reason),
    };
}
