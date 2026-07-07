#include "CorePch.h"
#include "Grid.h"
#include "Metrics.h"

Grid::CellIndex Grid::GetCellIndex(Position const& pos) const
{
    return {
        static_cast<int32_t>(std::floor(pos._x / _cellSize)),
        static_cast<int32_t>(std::floor(pos._z / _cellSize))
    };
}

void Grid::Add(ActorId const actorId, Position const& pos)
{
    auto& cell = GetOrCreateCell(GetCellIndex(pos));
    cell.insert(actorId);
}

void Grid::Remove(ActorId const actorId, Position const& pos)
{
    auto const idx = GetCellIndex(pos);
    auto const iter = _cells.find(idx);
    if (_cells.end() != iter)
    {
        iter->second.erase(actorId);

        if (iter->second.empty())
        {
            _cells.erase(iter);
        }
    }
}

void Grid::Move(ActorId const actorId, Position const& oldPos, Position const& newPos)
{
    auto const oldIdx = GetCellIndex(oldPos);
    auto const newIdx = GetCellIndex(newPos);

    if (newIdx == oldIdx)
    {
        return;
    }

    Remove(actorId, oldPos);
    Add(actorId, newPos);
}

void Grid::GetNearbyActorIds(Position const& center, std::vector<ActorId>& outActorIds) const
{
    auto const initialResultCount = outActorIds.size();
    auto const centerIdx = GetCellIndex(center);

    for (int32_t dx = -1; dx <= 1; ++dx)
    {
        for (int32_t dz = -1; dz <= 1; ++dz)
        {
            auto const neighborIdx = CellIndex{ centerIdx._x + dx, centerIdx._z + dz };

            if (auto const* cell = GetCell(neighborIdx))
            {
                for (auto const actorId : *cell)
                {
                    outActorIds.emplace_back(actorId);
                }
            }
        }
    }

    Metrics::OnGridNearbyQuery(outActorIds.size() - initialResultCount);
}

void Grid::GetSightDiff(
    Position const& newCenter,
    Position const& oldCenter,
    std::vector<ActorId>& outEntered,
    std::vector<ActorId>& outLeft) const
{
    auto const newIdx = GetCellIndex(newCenter);
    auto const oldIdx = GetCellIndex(oldCenter);

    auto const isInOldSight = [&oldIdx](CellIndex const& idx)
        {
            return std::abs(idx._x - oldIdx._x) <= 1 && std::abs(idx._z - oldIdx._z) <= 1;
        };

    auto const isInNewSight = [&newIdx](CellIndex const& idx)
        {
            return std::abs(idx._x - newIdx._x) <= 1 && std::abs(idx._z - newIdx._z) <= 1;
        };

    for (int32_t dx = -1; dx <= 1; ++dx)
    {
        for (int32_t dz = -1; dz <= 1; ++dz)
        {
            auto const cellIdx = CellIndex{ newIdx._x + dx, newIdx._z + dz };
            if (isInOldSight(cellIdx))
            {
                continue;
            }
            if (auto const* cell = GetCell(cellIdx))
            {
                for (auto const actorId : *cell)
                {
                    outEntered.emplace_back(actorId);
                }
            }
        }
    }

    for (int32_t dx = -1; dx <= 1; ++dx)
    {
        for (int32_t dz = -1; dz <= 1; ++dz)
        {
            auto const cellIdx = CellIndex{ oldIdx._x + dx, oldIdx._z + dz };
            if (isInNewSight(cellIdx))
            {
                continue;
            }
            if (auto const* cell = GetCell(cellIdx))
            {
                for (auto const actorId : *cell)
                {
                    outLeft.emplace_back(actorId);
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
