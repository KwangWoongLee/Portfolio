#pragma once
#include "CorePch.h"
#include "IOCPSession.h"
#include "Actor.h"

class ClientSession final
    : public IOCPSession
{
public:
    void HandlePacket(uint16_t const packetId, void const* const payload, uint32_t const size) override;

    ActorId GetActorId() const { return _actorId; }
    void SetActorId(ActorId const actorId) { _actorId = actorId; }

private:
    void OnConnected() override;
    void OnDisconnected() override;

    ActorId _actorId{ INVALID_ACTOR_ID };
};
