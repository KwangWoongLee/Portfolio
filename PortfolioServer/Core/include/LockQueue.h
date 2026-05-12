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
        _approxSize.fetch_add(1, std::memory_order_relaxed);
    }

    void Enqueue(std::vector<T> const& values)
    {
        std::scoped_lock lock(_mutex);

        for (auto const& value : values)
        {
            _queue.emplace(value);
        }
        _approxSize.fetch_add(values.size(), std::memory_order_relaxed);
    }

    void DequeueAll(std::queue<T>& out)
    {
        std::scoped_lock lock(_mutex);
        auto const popped = _queue.size();
        _queue.swap(out);
        _approxSize.fetch_sub(popped, std::memory_order_relaxed);
    }

    size_t ApproxSize() const
    {
        return _approxSize.load(std::memory_order_relaxed);
    }

private:
    std::mutex _mutex;
    std::queue<T> _queue;
    std::atomic<size_t> _approxSize{};
};
