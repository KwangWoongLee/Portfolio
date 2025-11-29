#pragma once
#include "CorePch.h"

template <typename T>
class LockQueue final
{
public:
    using value_type = std::remove_cv_t<T>;

    void Enqueue(T const& value)
    {
        std::scoped_lock lock(_mutex);
        _queue.emplace(value);
    }

    void Enqueue(std::vector<T> const& values)
    {
        std::scoped_lock lock(_mutex);

        for (auto const& value : values)
        {
            _queue.emplace(value);
        }
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
