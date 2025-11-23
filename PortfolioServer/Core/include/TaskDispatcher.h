#pragma once
#include "CorePch.h"
#include "KeySerialTaskExecutor.h"

#include "Task.h"

class TaskDispatcher final
{
public:
    using Singleton = Singleton<TaskDispatcher>;

public:
    bool AddExecutor(ETaskType const taskType, uint8_t const threadCount)
    {
        // TODO: taskType valid

        if (0 >= threadCount)
        {
            //TODO: log
            return false;
        }

    	if (_executors.contains(taskType))
    	{
            //TODO: log
            return false;
    	}

        auto executor = std::make_unique<KeySerialTaskExecutor>(threadCount);
        executor->Start();

        _executors.emplace(taskType, std::move(executor));
        return true;
    }

    void Dispatch(std::shared_ptr<ITask> const& task)
    {
        auto const iter = _executors.find(task->GetTaskType());
        if (_executors.end() == iter)
        {
            //TODO: log
            assert(false);
        }

        iter->second->Reserve(task);
    }

private:
    std::unordered_map<ETaskType, std::unique_ptr<KeySerialTaskExecutor>> _executors;
};