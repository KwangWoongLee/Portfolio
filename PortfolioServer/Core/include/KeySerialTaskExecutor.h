#pragma once

#include "CorePch.h"
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
            _workers[i]._wakeEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr); // auto-reset
            _workers[i]._worker = std::thread([this, i]() { ThreadLoop(i); });
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
        _workers[index]._queue.Enqueue(task);
        ::SetEvent(_workers[index]._wakeEvent);
    }

    void Shutdown()
    {
        if (_stopped.exchange(true))
        {
            return;
        }

        for (auto& w : _workers)
        {
            if (w._wakeEvent)
            {
                ::SetEvent(w._wakeEvent);
            }
        }

        for (auto& w : _workers)
        {
            if (w._worker.joinable())
            {
                w._worker.join();
            }

            if (w._wakeEvent)
            {
                ::CloseHandle(w._wakeEvent);
                w._wakeEvent = nullptr;
            }
        }
    }

private:
    void ThreadLoop(size_t const index)
    {
        auto& ctx = _workers[index];

        while (not _stopped)
        {
            ::WaitForSingleObject(ctx._wakeEvent, INFINITE);

            std::queue<std::shared_ptr<ITask>> localQueue;
            ctx._queue.DequeueAll(localQueue);

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
        LockQueue<std::shared_ptr<ITask>> _queue;
        HANDLE _wakeEvent = nullptr;
        std::thread _worker;
    };

    std::vector<TaskWorkerContext> _workers;
    std::atomic<bool> _stopped{false};
};
