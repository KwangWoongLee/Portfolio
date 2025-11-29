#pragma once
#include "CorePch.h"
#include "Stream.h"

class Packet
{
public:
    virtual ~Packet() = default;

    virtual uint16_t GetId() const = 0;
    virtual bool WriteToStream(Stream& stream) const = 0;
};

#define MAKABLE_STREAM(ID, ...)                                       \
public:                                                               \
    static uint16_t constexpr PACKET_ID = ID;                         \
    uint16_t GetId() const override                                   \
    {                                                                 \
        return PACKET_ID;                                             \
    }                                                                 \
    bool WriteToStream(Stream& stream) const override                 \
    {                                                                 \
        return Stream::WritePacket(stream, PACKET_ID, __VA_ARGS__);   \
    }
