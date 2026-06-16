#include "CorePch.h"
#include "StateMachine.h"

char const* ToString(EStateTransitionError const error)
{
    switch (error)
    {
    case EStateTransitionError::None:
        return "None";
    case EStateTransitionError::SameState:
        return "SameState";
    case EStateTransitionError::UnknownState:
        return "UnknownState";
    case EStateTransitionError::TransitionNotAllowed:
        return "TransitionNotAllowed";
    case EStateTransitionError::GuardRejected:
        return "GuardRejected";
    case EStateTransitionError::AlreadyTransitioning:
        return "AlreadyTransitioning";
    default:
        return "Unknown";
    }
}
