#pragma once
#include "WorldTaskRunner.h"

template <typename T_MSG>
void SendToWorld(WorldId const worldId, T_MSG msg)
{
    WorldTaskRunner::PostMessage(worldId, std::move(msg));
}
