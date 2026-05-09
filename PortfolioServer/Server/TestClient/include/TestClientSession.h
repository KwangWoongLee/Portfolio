#pragma once
#include "CorePch.h"
#include <random>
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

    std::mt19937 _rng;
    float _x{};
    float _z{};
    TimerId _moveTimerId{};
    TimerId _attackTimerId{};
};
