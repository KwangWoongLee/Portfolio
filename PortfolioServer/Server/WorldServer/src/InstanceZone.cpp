#include "CorePch.h"
#include "InstanceZone.h"

void InstanceZone::Update()
{
    // TODO: 던전 로직, 몬스터 AI, 클리어 조건 체크 등
}

void InstanceZone::Leave(ActorId const actorId)
{
    IZone::Leave(actorId);

    if (IsEmpty())
    {
        std::cout << "[Instance:" << _instanceId << "] Empty - pending destroy" << std::endl;
    }
}
