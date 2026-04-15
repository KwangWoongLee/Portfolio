#include "CorePch.h"
#include "Grid.h"

Grid::CellIndex Grid::GetCellIndex(Position const& pos) const
{
    return {
        static_cast<int32_t>(std::floor(pos._x / _cellSize)),
        static_cast<int32_t>(std::floor(pos._z / _cellSize))
    };
}

void Grid::Add(EntityId const entityId, Position const& pos)
{
    auto& cell = GetOrCreateCell(GetCellIndex(pos));
    cell.insert(entityId);
}

void Grid::Remove(EntityId const entityId, Position const& pos)
{
    auto const idx = GetCellIndex(pos);
    auto const iter = _cells.find(idx);
    if (_cells.end() != iter)
    {
        iter->second.erase(entityId);

        if (iter->second.empty())
        {
            _cells.erase(iter);
        }
    }
}

void Grid::Move(EntityId const entityId, Position const& oldPos, Position const& newPos)
{
    auto const oldIdx = GetCellIndex(oldPos);
    auto const newIdx = GetCellIndex(newPos);

    if (newIdx == oldIdx)
    {
        return;
    }

    Remove(entityId, oldPos);
    Add(entityId, newPos);
}

void Grid::GetNearbyEntityIds(Position const& center, std::vector<EntityId>& outEntityIds) const
{
    auto const centerIdx = GetCellIndex(center);

    for (int32_t dx = -1; dx <= 1; ++dx)
    {
        for (int32_t dz = -1; dz <= 1; ++dz)
        {
            CellIndex const neighborIdx{ centerIdx._x + dx, centerIdx._z + dz };

            if (auto const* cell = GetCell(neighborIdx))
            {
                for (auto const entityId : *cell)
                {
                    outEntityIds.push_back(entityId);
                }
            }
        }
    }
}

Grid::Cell& Grid::GetOrCreateCell(CellIndex const& idx)
{
    return _cells[idx];
}

Grid::Cell const* Grid::GetCell(CellIndex const& idx) const
{
    auto const iter = _cells.find(idx);
    if (_cells.end() == iter)
    {
        return nullptr;
    }
    return &iter->second;
}
