#pragma once
#include "CorePch.h"
#include "KeySerialTaskExecutor.h"
#include "Task.h"

class TaskDispatcher final
{
public:
    using Singleton = Singleton<TaskDispatcher>;

    bool AddExecutor(ETaskType const taskType, uint8_t const threadCount);
    void Dispatch(std::shared_ptr<ITask> const& task) const;

private:
    std::array<std::unique_ptr<KeySerialTaskExecutor>, TASK_TYPE_MAX> _executors;
};
