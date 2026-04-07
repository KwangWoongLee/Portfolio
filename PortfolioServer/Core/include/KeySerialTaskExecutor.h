#pragma once

#include "CorePch.h"
#include "Task.h"
#include "LockQueue.h"

class KeySerialTaskExecutor final
{
public:
    explicit KeySerialTaskExecutor(uint8_t const threadCount);
    ~KeySerialTaskExecutor();

    void Start() const;
    void Reserve(std::shared_ptr<ITask> const& task) const;
    void Shutdown();

private:
    void ThreadLoop(size_t const index) const;

private:
    uint8_t _threadCount{};

    struct TaskWorkerContext
    {
        LockQueue<std::shared_ptr<ITask>> _queue;
        HANDLE _wakeEvent = nullptr;
        std::thread _worker;
    };

    std::vector<std::unique_ptr<TaskWorkerContext>> _workers;
    std::atomic<bool> _stopped{false};
};
