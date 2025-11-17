#pragma once
#include "stdafx.h"

template <typename T>
class LockQueue final
{
public:
    void Enqueue(T const& value)
    {
        std::scoped_lock lock(_mutex);
        _queue.emplace(value);
    }

    void DequeueAll(std::queue<T>& out)
    {
        std::scoped_lock lock(_mutex);
        _queue.swap(out);
    }

private:
    std::mutex _mutex;
    std::queue<T> _queue;
};
