#pragma once
#include "CorePch.h"
#include "IOCPSession.h"
#include "Actor.h"

class ClientSession final
    : public IOCPSession
{
public:
    void HandlePacket(uint16_t const packetId, void const* const payload, uint32_t const size) override;

    ActorId GetActorId() const { return ActorId{ _actorIdValue.load(std::memory_order_relaxed) }; }
    void SetActorId(ActorId const actorId)
    {
        _actorIdValue.store(static_cast<int64_t>(actorId), std::memory_order_relaxed);
    }

private:
    void OnConnected() override;
    void OnDisconnected() override;

    std::atomic<int64_t> _actorIdValue{ static_cast<int64_t>(INVALID_ACTOR_ID) };
    std::atomic_bool _disconnected{ false };
};
