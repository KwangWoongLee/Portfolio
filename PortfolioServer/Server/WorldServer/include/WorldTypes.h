#pragma once
#include "CorePch.h"
#include "Actor.h"

using ZoneId = uint32_t;
using InstanceId = uint64_t;

inline ZoneId constexpr INVALID_ZONE_ID = 0;

struct Position final
{
    float _x{};
    float _z{};

    float DistanceSqTo(Position const& other) const
    {
        auto const dx = _x - other._x;
        auto const dz = _z - other._z;
        return dx * dx + dz * dz;
    }
};
