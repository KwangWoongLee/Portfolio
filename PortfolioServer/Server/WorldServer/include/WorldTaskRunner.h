#pragma once
#include "ManagedActorTaskAccess.h"
#include "WorldActorRegistry.h"
#include "WorldTask.h"

class WorldTaskRunner final
    : private ManagedActorTaskAccess<WorldId, WorldTask>
{
public:
    template <typename T_MSG>
    static void PostMessage(WorldId const worldId, T_MSG msg)
    {
        PostManagedActorTask(worldId,
            [worldId, msg = std::move(msg)]() mutable
            {
                Run(worldId,
                    [&msg](WorldActor& worldActor)
                    {
                        worldActor.OnMessage(msg);
                    });
            });
    }

private:
    template <typename T_FUNC>
    static void Run(WorldId const worldId, T_FUNC&& func)
    {
        auto const worldActor = WorldActorRegistry::Singleton::GetConstInstance().FindInternal(worldId);
        if (not worldActor)
        {
            return;
        }

        std::invoke(std::forward<T_FUNC>(func), *worldActor);
    }
};
