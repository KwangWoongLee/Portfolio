#pragma once
#include "CorePch.h"
#include "TaskDispatcher.h"
#include "Actor.h"
#include "ActorTask.h"

template <typename T_FUNC>
void Post(ActorId const actorId, T_FUNC&& func)
{
    auto task = std::make_shared<ActorTask<std::decay_t<T_FUNC>>>(actorId, std::forward<T_FUNC>(func));
    TaskDispatcher::Singleton::GetInstance().Dispatch(task);
}
