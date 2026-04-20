#pragma once
#include "CorePch.h"
#include "Task.h"
#include "Actor.h"

template <typename T_FUNC>
class ActorTask final
    : public ITask
{
public:
    ActorTask(ActorId const actorId, T_FUNC func)
        : ITask(ETaskType::GameLogic, actorId)
        , _func(std::move(func))
    {
    }

    void Execute() override
    {
        _func();
    }

private:
    T_FUNC _func;
};
