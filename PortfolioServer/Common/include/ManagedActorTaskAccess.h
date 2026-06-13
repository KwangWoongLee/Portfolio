#pragma once
#include "CorePch.h"
#include "TaskDispatcher.h"

template <typename T_KEY, template <typename> typename T_TASK>
class ManagedActorTaskAccess
{
protected:
    template <typename T_FUNC>
    static void PostManagedActorTask(T_KEY const key, T_FUNC&& func)
    {
        auto task = std::make_shared<T_TASK<std::decay_t<T_FUNC>>>(key, std::forward<T_FUNC>(func));
        TaskDispatcher::Singleton::GetInstance().Dispatch(task);
    }
};
