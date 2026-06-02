#pragma once
#include "CorePch.h"

template <typename T>
class ObjectPool
{
public:
    using Singleton = Singleton<ObjectPool>;

public:
    explicit ObjectPool(size_t const initialCapacity = 0)
    {
        for (size_t i = 0; i < initialCapacity; ++i)
        {
            _pool.emplace_back(AllocateStorage());
        }
    }
    ~ObjectPool()
    {
        std::scoped_lock lock(_mutex);
	    for (auto* obj : _pool)
	    {
            DeallocateStorage(obj);
	    }

        _pool.clear();
    }

    template <typename... ARGS>
    T* Acquire(ARGS&&... args)
    {
        std::scoped_lock lock(_mutex);

        if (not _pool.empty())
        {
            auto* obj = _pool.back();
            _pool.pop_back();
            try
            {
                return new (obj) T(std::forward<ARGS>(args)...);
            }
            catch (...)
            {
                _pool.emplace_back(obj);
                throw;
            }
        }

        auto* obj = AllocateStorage();
        try
        {
            return new (obj) T(std::forward<ARGS>(args)...);
        }
        catch (...)
        {
            DeallocateStorage(obj);
            throw;
        }
    }

    void Release(T* obj)
    {
        if (not obj)
        {
            return;
        }

        obj->~T();

        std::scoped_lock lock(_mutex);
        _pool.emplace_back(obj);
    }

    template <typename... ARGS>
    std::shared_ptr<T> AcquireShared(ARGS&&... args)
    {
        T* obj{ Acquire(std::forward<ARGS>(args)...) };

        return std::shared_ptr<T>(
            obj,
            [](T* ptr) {
                Singleton::GetInstance().Release(ptr);
            }
        );
    }

private:
    static T* AllocateStorage()
    {
        return static_cast<T*>(::operator new(sizeof(T), std::align_val_t{ alignof(T) }));
    }

    static void DeallocateStorage(T* obj)
    {
        ::operator delete(obj, std::align_val_t{ alignof(T) });
    }

    std::deque<T*> _pool;
    std::mutex _mutex;
};
