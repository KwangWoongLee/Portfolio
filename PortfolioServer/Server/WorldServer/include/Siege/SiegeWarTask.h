#pragma once
#include "CorePch.h"
#include "Siege/SiegeTypes.h"
#include "Task.h"

template <typename T_FUNC>
class SiegeWarTask final
    : public ITask
{
public:
    SiegeWarTask(SiegeWarId const siegeWarId, T_FUNC func)
        : ITask(ETaskType::GameLogic, static_cast<int64_t>(siegeWarId))
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
