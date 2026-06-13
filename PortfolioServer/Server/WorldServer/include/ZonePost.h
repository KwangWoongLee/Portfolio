#pragma once
#include "CorePch.h"
#include "TaskDispatcher.h"
#include "WorldTypes.h"
#include "ZoneTask.h"
#include "ZoneTaskRunner.h"

template <typename T_FUNC>
void PostToZone(ZoneId const zoneId, T_FUNC&& func)
{
    ZoneTaskRunner::PostCallback(zoneId, std::forward<T_FUNC>(func));
}

template <typename T_MSG>
void SendToZone(ZoneId const zoneId, T_MSG msg)
{
    ZoneTaskRunner::PostMessage(zoneId, std::move(msg));
}
