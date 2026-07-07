#pragma once
#include "CorePch.h"
#include "KeySerialTaskExecutor.h"
#include "Task.h"

struct TaskDispatcherQueueSnapshot final
{
    std::array<size_t, TASK_TYPE_MAX> _queueSizeByTaskType{};
    std::array<std::vector<size_t>, TASK_TYPE_MAX> _workerQueueSizesByTaskType;

    size_t GetTotalQueueSize() const
    {
        size_t total{ 0 };
        for (auto const queueSize : _queueSizeByTaskType)
        {
            total += queueSize;
        }
        return total;
    }
};

class TaskDispatcher final
{
public:
    using Singleton = Singleton<TaskDispatcher>;

    bool AddExecutor(ETaskType const taskType, uint8_t const threadCount);
    bool Dispatch(std::shared_ptr<ITask> const& task) const;

    size_t GetTotalQueueSize() const;
    TaskDispatcherQueueSnapshot GetQueueSnapshot() const;

private:
    std::array<std::unique_ptr<KeySerialTaskExecutor>, TASK_TYPE_MAX> _executors;
};
