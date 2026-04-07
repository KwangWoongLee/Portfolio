#pragma once
#include "CorePch.h"

class TimerManager final
{
public:
    using Singleton = Singleton<TimerManager>;
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::milliseconds;

    struct TimerEntry
    {
        TimerId _id{};
        TimePoint _expireTime;
        Duration _interval{};
        int64_t _taskKey{};
        std::function<void()> _callback;
        bool _isRepeat{};
    };

    struct TimerCompare
    {
        bool operator()(TimerEntry const& lhs, TimerEntry const& rhs) const
        {
            return lhs._expireTime > rhs._expireTime;
        }
    };

public:
    TimerId AddTimer(Duration const delay, int64_t const taskKey, std::function<void()>&& callback)
    {
        return AddTimerInternal(delay, Duration::zero(), taskKey, std::move(callback), false);
    }

    TimerId AddRepeatTimer(Duration const interval, int64_t const taskKey, std::function<void()>&& callback)
    {
        return AddTimerInternal(interval, interval, taskKey, std::move(callback), true);
    }

    void CancelTimer(TimerId const timerId)
    {
        std::scoped_lock lock(_mutex);
        _cancelledIds.insert(timerId);
    }

    void Start();
    void Stop();

private:
    TimerId AddTimerInternal(Duration const delay, Duration const interval, int64_t const taskKey,
        std::function<void()>&& callback, bool const isRepeat)
    {
        std::scoped_lock lock(_mutex);

        auto const id = _nextTimerId++;

        TimerEntry entry;
        entry._id = id;
        entry._expireTime = Clock::now() + delay;
        entry._interval = interval;
        entry._taskKey = taskKey;
        entry._callback = std::move(callback);
        entry._isRepeat = isRepeat;

        _heap.push(std::move(entry));
        _cv.notify_one();

        return id;
    }

    void TimerThreadFunc();

private:
    std::mutex _mutex;
    std::condition_variable _cv;

    std::priority_queue<TimerEntry, std::vector<TimerEntry>, TimerCompare> _heap;
    std::unordered_set<TimerId> _cancelledIds;

    TimerId _nextTimerId{ 1 };

    std::thread _timerThread;
    std::atomic<bool> _stopped{ false };
};
