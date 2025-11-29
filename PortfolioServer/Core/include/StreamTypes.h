#pragma once
#include "CorePch.h"

struct StreamHeader final
{
    uint32_t _size{};
};

struct PacketHeader final
{
    uint16_t _id{};
    uint16_t _size{};
};
