#pragma once
#include "CorePch.h"
#include "Task.h"
#include "WorldTypes.h"

template <typename T_FUNC>
class ZoneTask final
    : public ITask
{
public:
    ZoneTask(ZoneId const zoneId, T_FUNC func)
        : ITask(ETaskType::GameLogic, static_cast<int64_t>(zoneId))
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
