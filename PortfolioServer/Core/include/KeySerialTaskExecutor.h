#pragma once

#include "stdafx.h"
#include "Task.h"
#include "LockQueue.h"

class KeySerialTaskExecutor final
{
public:
    explicit KeySerialTaskExecutor(uint8_t const threadCount)
        : _threadCount(threadCount)
    {
        _workers.resize(_threadCount);
    }

    ~KeySerialTaskExecutor()
    {
        Shutdown();
    }

    void Start()
    {
        for (auto i = 0; i < _threadCount; ++i)
        {
            _workers[i].wakeEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr); // auto-reset
            _workers[i].worker = std::thread([this, i]() { ThreadLoop(i); });
        }
    }

    void Reserve(std::shared_ptr<ITask> const& task)
    {
        auto const key = task->GetKey();
        if (0 > key)
        {
            return;
        }

        auto const index = key % _threadCount;
        _workers[index].queue.Enqueue(task);
        ::SetEvent(_workers[index].wakeEvent);
    }

    void Shutdown()
    {
        if (_stopped.exchange(true))
        {
            return;
        }

        for (auto& w : _workers)
        {
            if (w.wakeEvent)
            {
                ::SetEvent(w.wakeEvent);
            }
        }

        for (auto& w : _workers)
        {
            if (w.worker.joinable())
            {
                w.worker.join();
            }

            if (w.wakeEvent)
            {
                ::CloseHandle(w.wakeEvent);
                w.wakeEvent = nullptr;
            }
        }
    }

private:
    void ThreadLoop(size_t const index)
    {
        auto& ctx = _workers[index];

        while (not _stopped)
        {
            ::WaitForSingleObject(ctx.wakeEvent, INFINITE);

            std::queue<std::shared_ptr<ITask>> localQueue;
            ctx.queue.DequeueAll(localQueue);

            while (not localQueue.empty())
            {
                localQueue.front()->Execute();
                localQueue.pop();
            }
        }
    }

private:
    uint8_t _threadCount{};

    struct TaskWorkerContext
    {
        LockQueue<std::shared_ptr<ITask>> queue;
        HANDLE wakeEvent = nullptr;
        std::thread worker;
    };

    std::vector<TaskWorkerContext> _workers;
    std::atomic<bool> _stopped{false};
};
