#pragma once

#pragma pack(push, 1)

struct StreamHeader
{
    uint32_t size = 0;
};

struct PacketHeader
{
    uint16_t id = 0;
    uint32_t size = 0;
};

#pragma pack(pop)