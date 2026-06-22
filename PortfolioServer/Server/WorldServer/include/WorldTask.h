#pragma once
#include "CorePch.h"
#include "Task.h"
#include "WorldTypes.h"

template <typename T_FUNC>
class WorldTask final
    : public ITask
{
public:
    WorldTask(WorldId const worldId, T_FUNC func)
        : ITask(ETaskType::GameLogic, static_cast<int64_t>(worldId))
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
