#include "CorePch.h"
#include "IZone.h"
#include "ClientSession.h"

bool IZone::Enter(std::shared_ptr<ClientSession> const& session)
{
    auto const entityId = session->GetEntityId();

    if (_players.contains(entityId))
    {
        return false;
    }

    _players.emplace(entityId, session);
    session->SetCurrentZoneId(_zoneId);

    _grid.Add(entityId, session->GetPosition());

    std::cout << "[Zone:" << _zoneId << "] Player entity=" << entityId
        << " entered (count=" << _players.size() << ")" << std::endl;

    return true;
}

void IZone::Leave(EntityId const entityId)
{
    auto const iter = _players.find(entityId);
    if (_players.end() == iter)
    {
        return;
    }

    _grid.Remove(entityId, iter->second->GetPosition());
    _players.erase(iter);

    std::cout << "[Zone:" << _zoneId << "] Entity=" << entityId
        << " left (count=" << _players.size() << ")" << std::endl;
}

void IZone::OnEntityMove(EntityId const entityId, Position const& oldPos, Position const& newPos)
{
    _grid.Move(entityId, oldPos, newPos);
}

void IZone::Broadcast(Packet const& packet)
{
    for (auto const& [id, session] : _players)
    {
        session->SendPacket(packet);
    }
}

void IZone::BroadcastInSight(Position const& center, Packet const& packet, EntityId const excludeEntityId)
{
    std::vector<EntityId> nearbyIds;
    _grid.GetNearbyEntityIds(center, nearbyIds);

    for (auto const entityId : nearbyIds)
    {
        if (excludeEntityId == entityId)
        {
            continue;
        }

        if (auto const iter = _players.find(entityId); _players.end() != iter)
        {
            iter->second->SendPacket(packet);
        }
    }
}

std::shared_ptr<ClientSession> IZone::FindPlayer(EntityId const entityId) const
{
    auto const iter = _players.find(entityId);
    if (_players.end() == iter)
    {
        return nullptr;
    }
    return iter->second;
}
