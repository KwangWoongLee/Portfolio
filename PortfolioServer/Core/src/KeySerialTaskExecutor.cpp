#include "CorePch.h"
#include "KeySerialTaskExecutor.h"

KeySerialTaskExecutor::KeySerialTaskExecutor(uint8_t const threadCount)
    : _threadCount(threadCount)
{
    assert(0 != threadCount);
    for (uint8_t i{}; i < _threadCount; ++i)
    {
        _workers.emplace_back(std::make_unique<TaskWorkerContext>());
    }
}

KeySerialTaskExecutor::~KeySerialTaskExecutor()
{
    Shutdown();
}

void KeySerialTaskExecutor::Start() const
{
    for (auto i = 0; i < _threadCount; ++i)
    {
        _workers[i]->_wakeEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
        _workers[i]->_worker = std::thread([this, i]() { ThreadLoop(i); });
    }
}

void KeySerialTaskExecutor::Reserve(std::shared_ptr<ITask> const& task) const
{
    auto const key = task->GetKey();
    if (0 > key)
    {
        return;
    }

    auto const index = key % _threadCount;
    if (not _workers[index]->_wakeEvent)
    {
        return;
    }

    _workers[index]->_queue.Enqueue(task);
    ::SetEvent(_workers[index]->_wakeEvent);
}

void KeySerialTaskExecutor::Shutdown()
{
    if (_stopped.exchange(true))
    {
        return;
    }

    for (auto const& w : _workers)
    {
        if (w->_wakeEvent)
        {
            ::SetEvent(w->_wakeEvent);
        }
    }

    for (auto const& w : _workers)
    {
        if (w->_worker.joinable())
        {
            w->_worker.join();
        }

        if (w->_wakeEvent)
        {
            ::CloseHandle(w->_wakeEvent);
            w->_wakeEvent = nullptr;
        }
    }
}

void KeySerialTaskExecutor::ThreadLoop(size_t const index) const
{
    auto const& ctx = _workers[index];

    while (not _stopped)
    {
        ::WaitForSingleObject(ctx->_wakeEvent, INFINITE);

        std::queue<std::shared_ptr<ITask>> localQueue;
        ctx->_queue.DequeueAll(localQueue);

        while (not localQueue.empty())
        {
            localQueue.front()->Execute();
            localQueue.pop();
        }
    }
}
