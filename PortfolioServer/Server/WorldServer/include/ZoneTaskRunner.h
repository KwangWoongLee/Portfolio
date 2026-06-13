#pragma once
#include "CorePch.h"
#include "InstanceZone.h"
#include "ManagedActorTaskAccess.h"
#include "ZoneManager.h"
#include "ZoneTask.h"

class ZoneTaskRunner final
    : private ManagedActorTaskAccess<ZoneId, ZoneTask>
{
public:
    template <typename T_FUNC>
    static void PostCallback(ZoneId const zoneId, T_FUNC&& func)
    {
        PostManagedActorTask(zoneId, std::forward<T_FUNC>(func));
    }

    template <typename T_MSG>
    static void PostMessage(ZoneId const zoneId, T_MSG msg)
    {
        PostCallback(zoneId,
            [zoneId, msg = std::move(msg)]() mutable
            {
                Run(zoneId,
                    [&msg](IZone& zone)
                    {
                        zone.OnMessage(msg);
                    });
            });
    }

    template <typename T_FUNC>
    static void PostPlayerEnter(ZoneManager::ZoneEnterPermit permit, ZoneMsg::PlayerEntered msg, T_FUNC&& onComplete)
    {
        PostCallback(permit._zoneId,
            [permit, msg = std::move(msg), onComplete = std::forward<T_FUNC>(onComplete)]() mutable
            {
                bool entered{ false };
                RunWithPermit(permit,
                    [&msg, &entered](IZone& zone)
                    {
                        entered = zone.OnMessage(msg);
                    });

                ZoneManager::Singleton::GetInstance().ReleaseZoneEnter(permit);
                onComplete(entered);
            });
    }

    static void PostUpdate(ZoneId const zoneId)
    {
        PostCallback(zoneId,
            [zoneId]()
            {
                Run(zoneId,
                    [](IZone& zone)
                    {
                        zone.Update();
                    });
            });
    }

    static void PostDestroyIfEmpty(InstanceId const instanceId, ZoneId const zoneId)
    {
        PostCallback(zoneId,
            [instanceId, zoneId]()
            {
                Run(zoneId,
                    [instanceId, zoneId](IZone& zone)
                    {
                        auto const instance = dynamic_cast<InstanceZone*>(&zone);
                        if (not instance || instance->GetInstanceId() != instanceId || instance->GetZoneId() != zoneId)
                        {
                            return;
                        }

                        if (not instance->IsEmpty())
                        {
                            return;
                        }

                        auto& zoneMgr = ZoneManager::Singleton::GetInstance();
                        if (not zoneMgr.TryMarkClosingIfDestroyable(instanceId, zoneId))
                        {
                            return;
                        }

                        zoneMgr.DestroyInstanceDungeon(instanceId);
                    });
            });
    }

private:
    template <typename T_FUNC>
    static void Run(ZoneId const zoneId, T_FUNC&& func)
    {
        auto const zone = ZoneManager::Singleton::GetConstInstance().FindZoneInternal(zoneId);
        if (not zone)
        {
            return;
        }

        std::invoke(std::forward<T_FUNC>(func), *zone);
    }

    template <typename T_FUNC>
    static void RunWithPermit(ZoneManager::ZoneEnterPermit const& permit, T_FUNC&& func)
    {
        auto const zone = ZoneManager::Singleton::GetConstInstance().FindZoneByPermit(permit);
        if (not zone)
        {
            return;
        }

        std::invoke(std::forward<T_FUNC>(func), *zone);
    }
};
