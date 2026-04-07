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

void TaskDispatcher::Dispatch(std::shared_ptr<ITask> const& task) const
{
    auto const taskTypeIndex = static_cast<uint8_t>(task->GetTaskType());

    if (not _executors.at(taskTypeIndex))
    {
        //TODO: log
        assert(false);
    }

    _executors.at(taskTypeIndex)->Reserve(task);
}
