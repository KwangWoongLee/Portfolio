#include "CorePch.h"
#include "TimerManager.h"
#include "TimerTask.h"
#include "TaskDispatcher.h"

void TimerManager::Start()
{
    _stopped = false;
    _timerThread = std::thread([this]() { TimerThreadFunc(); });
}

void TimerManager::Stop()
{
    _stopped = true;
    _cv.notify_one();

    if (_timerThread.joinable())
    {
        _timerThread.join();
    }
}

void TimerManager::TimerThreadFunc()
{
    while (not _stopped)
    {
        std::vector<TimerEntry> expired;
        std::vector<TimerEntry> requeue;

        {
            std::unique_lock lock(_mutex);

            if (_heap.empty())
            {
                _cv.wait(lock, [this]() { return _stopped.load() || not _heap.empty(); });

                if (_stopped)
                {
                    return;
                }
            }

            auto const nextExpire = _heap.top()._expireTime;
            if (nextExpire > Clock::now())
            {
                _cv.wait_until(lock, nextExpire, [this]()
                {
                    return _stopped.load() || (!_heap.empty() && _heap.top()._expireTime <= Clock::now());
                });

                if (_stopped)
                {
                    return;
                }
            }

            auto const now = Clock::now();
            while (not _heap.empty() && _heap.top()._expireTime <= now)
            {
                auto entry = _heap.top();
                _heap.pop();

                if (_cancelledIds.contains(entry._id))
                {
                    _cancelledIds.erase(entry._id);
                    continue;
                }

                expired.push_back(entry);

                if (entry._isRepeat)
                {
                    entry._expireTime = now + entry._interval;
                    requeue.push_back(std::move(entry));
                }
            }

            for (auto& entry : requeue)
            {
                _heap.push(std::move(entry));
            }
        }

        for (auto& entry : expired)
        {
            auto task = std::make_shared<TimerCallbackTask>(entry._taskKey, std::move(entry._callback));
            TaskDispatcher::Singleton::GetInstance().Dispatch(task);
        }
    }
}
