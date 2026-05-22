#pragma once
#include "CorePch.h"
#include "TaskDispatcher.h"
#include "WorldTypes.h"
#include "ZoneTask.h"
#include "ZoneManager.h"

template <typename T_FUNC>
void PostToZone(ZoneId const zoneId, T_FUNC&& func)
{
    auto task = std::make_shared<ZoneTask<std::decay_t<T_FUNC>>>(zoneId, std::forward<T_FUNC>(func));
    TaskDispatcher::Singleton::GetInstance().Dispatch(task);
}

template <typename T_MSG>
void SendToZone(ZoneId const zoneId, T_MSG msg)
{
    PostToZone(zoneId,
        [zoneId, msg = std::move(msg)]
        {
            if (auto const zone = ZoneManager::Singleton::GetInstance().FindZone(zoneId))
            {
                zone->OnMessage(msg);
            }
        });
}
