#pragma once
#include "CorePch.h"
#include "Task.h"

class TimerCallbackTask final
    : public ITask
{
public:
    explicit TimerCallbackTask(int64_t const key, std::function<void()>&& callback)
        : ITask(ETaskType::Timer, key)
        , _callback(std::move(callback))
    {
    }

    void Execute() override
    {
        if (_callback)
        {
            _callback();
        }
    }

private:
    std::function<void()> _callback;
};
