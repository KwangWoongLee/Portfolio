#pragma once
#include "CorePch.h"
#include "Packet.h"
#include "Actor.h"
#include "PacketId.h"

struct C2WMove final
    : public Packet
{
    PACKET_STREAM(EPacketId::C2WMove, _x, _z)

    float _x{};
    float _z{};
};

struct C2WAttack final
    : public Packet
{
    PACKET_STREAM(EPacketId::C2WAttack, _targetActorId)

    ActorId _targetActorId{};
};

struct W2CMoveBroadcast final
    : public Packet
{
    PACKET_STREAM(EPacketId::W2CMoveBroadcast, _actorId, _x, _z)

    ActorId _actorId{};
    float _x{};
    float _z{};
};

struct W2CHpUpdate final
    : public Packet
{
    PACKET_STREAM(EPacketId::W2CHpUpdate, _actorId, _hp)

    ActorId _actorId{};
    int32_t _hp{};
};

struct W2CDeath final
    : public Packet
{
    PACKET_STREAM(EPacketId::W2CDeath, _actorId)

    ActorId _actorId{};
};
