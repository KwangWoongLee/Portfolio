#pragma once
#include <cstdint>
#include <cmath>
#include <atomic>

using ZoneId = uint32_t;
using InstanceId = uint64_t;
using EntityId = int64_t;

inline ZoneId constexpr INVALID_ZONE_ID = 0;
inline EntityId constexpr INVALID_ENTITY_ID = 0;

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

class EntityIdGenerator final
{
public:
    static EntityId Generate()
    {
        static std::atomic<EntityId> sNextId{ 1 };
        return sNextId.fetch_add(1, std::memory_order_relaxed);
    }
};
