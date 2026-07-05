#pragma once
#include "CorePch.h"
#include <random>
#include <unordered_set>
#include "Actor.h"
#include "IOCPSession.h"
#include "Packet.h"

class TestClientSession final
    : public IOCPSession
{
public:
    void HandlePacket(uint16_t const packetId, void const* const payload, uint32_t const size) override;

private:
    void OnConnected() override;
    void OnDisconnected() override;

    void TickMove();
    void TickAttack();

    void TrackVisibleActor(ActorId actorId);
    void UntrackVisibleActor(ActorId actorId);

    mutable std::mutex _stateMutex;
    std::mt19937 _rng;
    ActorId _selfActorId{ INVALID_ACTOR_ID };
    std::unordered_set<ActorId> _visibleActorIds;
    float _x{};
    float _z{};
    TimerId _moveTimerId{};
    TimerId _attackTimerId{};
};
