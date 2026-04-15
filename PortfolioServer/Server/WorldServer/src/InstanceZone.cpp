#include "CorePch.h"
#include "InstanceZone.h"

void InstanceZone::Update()
{
    // TODO: 던전 로직, 몬스터 AI, 클리어 조건 체크 등
}

void InstanceZone::Leave(EntityId const entityId)
{
    IZone::Leave(entityId);

    if (IsEmpty())
    {
        std::cout << "[Instance:" << _instanceId << "] Empty - pending destroy" << std::endl;
    }
}
