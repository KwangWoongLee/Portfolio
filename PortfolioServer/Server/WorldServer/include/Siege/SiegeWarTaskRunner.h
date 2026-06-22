#pragma once
#include "ManagedActorTaskAccess.h"
#include "Siege/SiegeWarTask.h"
#include "WorldActorRegistry.h"
#include "WorldPost.h"

class SiegeWarTaskRunner final
    : private ManagedActorTaskAccess<SiegeWarId, SiegeWarTask>
{
public:
    static void PostTick(
        WorldId const worldId,
        SiegeWarId const siegeWarId,
        SiegeWar::Clock::time_point const now)
    {
        PostManagedActorTask(siegeWarId,
            [worldId, siegeWarId, now]()
            {
                Run(worldId, siegeWarId,
                    [worldId, now](SiegeWar& siegeWar)
                    {
                        auto const result = siegeWar.Tick(now);
                        if (result.Succeeded())
                        {
                            Publish(worldId, siegeWar.CreateSnapshot());
                        }
                    });
            });
    }

    static void PostOccupied(
        WorldId const worldId,
        SiegeWarId const siegeWarId,
        GuildId const guildId,
        SiegeWar::Clock::time_point const now)
    {
        PostManagedActorTask(siegeWarId,
            [worldId, siegeWarId, guildId, now]()
            {
                Run(worldId, siegeWarId,
                    [worldId, guildId, now](SiegeWar& siegeWar)
                    {
                        if (siegeWar.OnOccupied(guildId, now))
                        {
                            Publish(worldId, siegeWar.CreateSnapshot());
                        }
                    });
            });
    }

    static void PostCancel(
        WorldId const worldId,
        SiegeWarId const siegeWarId,
        std::string reason,
        SiegeWar::Clock::time_point const now)
    {
        PostManagedActorTask(siegeWarId,
            [worldId, siegeWarId, reason = std::move(reason), now]() mutable
            {
                Run(worldId, siegeWarId,
                    [worldId, reason = std::move(reason), now](SiegeWar& siegeWar) mutable
                    {
                        auto const result = siegeWar.Cancel(std::move(reason), now);
                        if (result.Succeeded())
                        {
                            Publish(worldId, siegeWar.CreateSnapshot());
                        }
                    });
            });
    }

private:
    template <typename T_FUNC>
    static void Run(
        WorldId const worldId,
        SiegeWarId const siegeWarId,
        T_FUNC&& func)
    {
        auto const worldActor = WorldActorRegistry::Singleton::GetConstInstance().FindInternal(worldId);
        if (not worldActor)
        {
            return;
        }

        auto const siegeWar = worldActor->FindSiegeWarInternal(siegeWarId);
        if (not siegeWar)
        {
            return;
        }

        std::invoke(std::forward<T_FUNC>(func), *siegeWar);
    }

    static void Publish(WorldId const worldId, SiegeWarSnapshot snapshot)
    {
        SendToWorld(worldId, WorldMsg::SiegeWarSnapshotUpdated{ std::move(snapshot) });
    }
};
