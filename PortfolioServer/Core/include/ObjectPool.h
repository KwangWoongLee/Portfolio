#pragma once
#include "stdafx.h"

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

    T* Acquire()
    {
        std::scoped_lock lock(_mutex);

        if (!_pool.empty())
        {
            auto* obj = _pool.back();
            _pool.pop_back();
            return obj;
        }

        return new T();
    }

    void Release(T* obj)
    {
        std::scoped_lock lock(_mutex);
        _pool.emplace_back(obj);
    }

    std::shared_ptr<T> AcquireShared()
    {
        T* obj = Acquire();

        return std::shared_ptr<T>(
            obj,
            [](T* ptr) {
                Singleton::Instance().Release(ptr);
            }
        );
    }

private:
    std::deque<T*> _pool;
    std::mutex _mutex;
};
