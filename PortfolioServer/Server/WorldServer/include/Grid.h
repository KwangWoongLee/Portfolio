#pragma once
#include "CorePch.h"
#include "WorldTypes.h"

class Grid final
{
public:
    struct CellIndex
    {
        int32_t _x{};
        int32_t _z{};

        bool operator==(CellIndex const&) const = default;
    };

    struct CellIndexHash
    {
        size_t operator()(CellIndex const& idx) const
        {
            auto const h1 = std::hash<int32_t>{}(idx._x);
            auto const h2 = std::hash<int32_t>{}(idx._z);
            return h1 ^ (h2 << 16);
        }
    };

    using Cell = std::unordered_set<ActorId>;

    explicit Grid(float const cellSize = 100.0f)
        : _cellSize(cellSize)
    {
    }

    CellIndex GetCellIndex(Position const& pos) const;

    void Add(ActorId const entityId, Position const& pos);
    void Remove(ActorId const entityId, Position const& pos);
    void Move(ActorId const entityId, Position const& oldPos, Position const& newPos);

    void GetNearbyActorIds(Position const& center, std::vector<ActorId>& outActorIds) const;

private:
    Cell& GetOrCreateCell(CellIndex const& idx);
    Cell const* GetCell(CellIndex const& idx) const;

    float _cellSize;
    std::unordered_map<CellIndex, Cell, CellIndexHash> _cells;
};
