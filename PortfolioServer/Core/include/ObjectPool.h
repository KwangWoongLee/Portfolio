#pragma once
#include "CorePch.h"

template <typename T>
class ObjectPool
{
public:
    using Singleton = Singleton<ObjectPool<T>>;

public:
    explicit ObjectPool(size_t const initialCapacity = 0)
    {
        for (size_t i = 0; i < initialCapacity; ++i)
        {
            _pool.emplace_back(new T());
        }
    }
    ~ObjectPool()
    {
        std::scoped_lock lock(_mutex);
	    for (auto* obj : _pool)
	    {
            delete obj;
	    }

        _pool.clear();
    }

    template <typename... ARGS>
    T* Acquire(ARGS&&... args)
    {
        std::scoped_lock lock(_mutex);

        if (!_pool.empty())
        {
            auto* obj = _pool.back();
            _pool.pop_back();
            return obj;
        }

        return new T(std::forward<ARGS>(args)...);
    }

    void Release(T* obj)
    {
        std::scoped_lock lock(_mutex);
        _pool.emplace_back(obj);
    }

    template <typename... ARGS>
    std::shared_ptr<T> AcquireShared(ARGS&&... args)
    {
        T* obj = Acquire(std::forward<ARGS>(args)...);

        return std::shared_ptr<T>(
            obj,
            [](T* ptr) {
                Singleton::GetInstance().Release(ptr);
            }
        );
    }

private:
    std::deque<T*> _pool;
    std::mutex _mutex;
};
