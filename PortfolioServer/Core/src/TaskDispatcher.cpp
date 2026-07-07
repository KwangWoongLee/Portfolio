#include "CorePch.h"
#include "TaskDispatcher.h"

bool TaskDispatcher::AddExecutor(ETaskType const taskType, uint8_t const threadCount)
{
    if (0 == threadCount)
    {
        //TODO: log
        return false;
    }

    auto const taskTypeIndex = static_cast<uint8_t>(taskType);
    if (_executors.at(taskTypeIndex))
    {
        //TODO: log
        return false;
    }

    auto executor = std::make_unique<KeySerialTaskExecutor>(threadCount);
    executor->Start();

    _executors.at(taskTypeIndex) = std::move(executor);
    return true;
}

bool TaskDispatcher::Dispatch(std::shared_ptr<ITask> const& task) const
{
    if (not task)
    {
        return false;
    }

    auto const taskTypeIndex = static_cast<uint8_t>(task->GetTaskType());

    if (not _executors.at(taskTypeIndex))
    {
        //TODO: log
        return false;
    }

    _executors.at(taskTypeIndex)->Reserve(task);
    return true;
}

size_t TaskDispatcher::GetTotalQueueSize() const
{
    size_t total{ 0 };
    for (auto const& exec : _executors)
    {
        if (exec)
        {
            total += exec->GetTotalQueueSize();
        }
    }
    return total;
}

TaskDispatcherQueueSnapshot TaskDispatcher::GetQueueSnapshot() const
{
    TaskDispatcherQueueSnapshot snapshot;
    for (size_t i = 0; i < _executors.size(); ++i)
    {
        auto const& exec = _executors[i];
        if (not exec)
        {
            continue;
        }

        snapshot._queueSizeByTaskType[i] = exec->GetTotalQueueSize();
        exec->CollectQueueSizes(snapshot._workerQueueSizesByTaskType[i]);
    }
    return snapshot;
}
