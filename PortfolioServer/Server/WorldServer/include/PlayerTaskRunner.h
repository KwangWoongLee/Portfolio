#pragma once
#include "ActorTask.h"
#include "ManagedActorTaskAccess.h"
#include "PlayerManager.h"
#include "ZoneManager.h"
#include "ZonePost.h"
#include "ZoneTaskRunner.h"

class PlayerTaskRunner final
    : private ManagedActorTaskAccess<ActorId, ActorTask>
{
public:
    template <typename T_MSG>
    static void PostMessage(ActorId const actorId, T_MSG msg)
    {
        PostManagedActorTask(actorId,
            [actorId, msg = std::move(msg)]() mutable
            {
                Run(actorId,
                    [&msg](Player& player)
                    {
                        player.OnMessage(msg);
                    });
            });
    }

    static void PostMoveToZone(ActorId const actorId, ZoneId const toZoneId)
    {
        PostManagedActorTask(actorId,
            [actorId, toZoneId]()
            {
                Run(actorId,
                    [actorId, toZoneId](Player& player)
                    {
                        auto& zoneMgr = ZoneManager::Singleton::GetInstance();
                        if (INVALID_ZONE_ID != player.GetPendingZoneId())
                        {
                            return;
                        }

                        auto permit = zoneMgr.TryReserveZoneEnter(toZoneId);
                        if (not permit)
                        {
                            return;
                        }

                        player.SetPendingZoneId(toZoneId);

                        ZoneTaskRunner::PostPlayerEnter(*permit, ZoneMsg::PlayerEntered{
                            actorId,
                            player.GetSession(),
                            player.GetPosition(),
                            player.GetHp()
                        },
                        [actorId, toZoneId](bool const entered)
                        {
                            PlayerTaskRunner::PostZoneEnterResult(actorId, toZoneId, entered);
                        });
                    });
            });
    }

    static void PostDisconnect(ActorId const actorId)
    {
        PostManagedActorTask(actorId,
            [actorId]()
            {
                Run(actorId,
                    [actorId](Player& player)
                    {
                        auto const zoneId = player.GetCurrentZoneId();
                        if (INVALID_ZONE_ID != zoneId)
                        {
                            SendToZone(zoneId, ZoneMsg::PlayerLeft{ actorId });
                        }

                        auto const pendingZoneId = player.GetPendingZoneId();
                        if (INVALID_ZONE_ID != pendingZoneId && pendingZoneId != zoneId)
                        {
                            SendToZone(pendingZoneId, ZoneMsg::PlayerLeft{ actorId });
                        }

                        PlayerManager::Singleton::GetInstance().RemoveInternal(actorId);
                    });
            });
    }

private:
    static void PostZoneEnterResult(ActorId const actorId, ZoneId const zoneId, bool const entered)
    {
        PostManagedActorTask(actorId,
            [actorId, zoneId, entered]()
            {
                Run(actorId,
                    [actorId, zoneId, entered](Player& player)
                    {
                        if (player.GetPendingZoneId() != zoneId)
                        {
                            return;
                        }

                        player.SetPendingZoneId(INVALID_ZONE_ID);

                        if (not entered)
                        {
                            return;
                        }

                        auto const fromZoneId = player.GetCurrentZoneId();
                        if (INVALID_ZONE_ID != fromZoneId && fromZoneId != zoneId)
                        {
                            SendToZone(fromZoneId, ZoneMsg::PlayerLeft{ actorId });
                        }

                        player.SetCurrentZoneId(zoneId);
                    });
            });
    }

    template <typename T_FUNC>
    static void Run(ActorId const actorId, T_FUNC&& func)
    {
        auto const player = PlayerManager::Singleton::GetConstInstance().FindInternal(actorId);
        if (not player)
        {
            return;
        }

        std::invoke(std::forward<T_FUNC>(func), *player);
    }
};
