#pragma once
#include "CorePch.h"
#include "Stream.h"

class Packet
{
public:
    virtual ~Packet() = default;

    virtual uint16_t GetId() const = 0;
    virtual bool WriteToStream(StreamWriter& writer) const = 0;
};

#define MAKABLE_STREAM(ID, ...)                                       \
public:                                                               \
    static uint16_t constexpr PACKET_ID = ID;                         \
    uint16_t GetId() const override                                   \
    {                                                                 \
        return PACKET_ID;                                             \
    }                                                                 \
    bool WriteToStream(StreamWriter& writer) const override           \
    {                                                                 \
        return StreamWriter::WritePacket(writer, PACKET_ID, __VA_ARGS__); \
    }

#define READABLE_STREAM(ID, ...)                                      \
public:                                                               \
    static uint16_t constexpr PACKET_ID = ID;                         \
    bool ReadFromStream(StreamReader& reader)                         \
    {                                                                 \
        return reader.ReadFields(__VA_ARGS__);                        \
    }                                                                 \
    bool ReadFromBytes(void const* const data, uint32_t const size)   \
    {                                                                 \
        StreamReader reader(data, size);                              \
        return ReadFromStream(reader);                                \
    }
