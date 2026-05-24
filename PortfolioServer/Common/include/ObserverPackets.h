#pragma once
#include "CorePch.h"
#include "Packet.h"
#include "PacketId.h"
#include "WorldPackets.h"

struct W2OSnapshot final
    : public Packet
{
    PACKET_STREAM(EPacketId::W2OSnapshot, _actors, _ccu, _sendPps, _recvPps, _sendBytesPerSecond, _recvBytesPerSecond, _cpuPercent, _queueLen)

    std::vector<ActorSnapshot> _actors;
    uint32_t _ccu{};
    uint32_t _sendPps{};
    uint32_t _recvPps{};
    uint32_t _sendBytesPerSecond{};
    uint32_t _recvBytesPerSecond{};
    uint32_t _cpuPercent{};
    uint32_t _queueLen{};
};
