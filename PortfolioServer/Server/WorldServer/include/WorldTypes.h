#pragma once
#include "CorePch.h"
#include "Actor.h"

using WorldId = StrongId<struct WorldIdTag, int64_t>;
using ZoneId = uint32_t;
using InstanceId = uint64_t;

inline WorldId constexpr INVALID_WORLD_ID{ 0 };
inline WorldId constexpr DEFAULT_WORLD_ID{ 1 };
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
