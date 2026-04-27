#include "CorePch.h"
#include "IZone.h"
#include "Player.h"

bool IZone::Enter(std::shared_ptr<Player> const& player)
{
    auto const actorId = player->GetActorId();

    if (_players.contains(actorId))
    {
        return false;
    }

    _players.emplace(actorId, player);
    player->SetCurrentZoneId(_zoneId);

    _grid.Add(actorId, player->GetPosition());

    std::cout << "[Zone:" << _zoneId << "] Player actor=" << actorId
        << " entered (count=" << _players.size() << ")" << std::endl;

    return true;
}

void IZone::Leave(ActorId const actorId)
{
    auto const iter = _players.find(actorId);
    if (_players.end() == iter)
    {
        return;
    }

    _grid.Remove(actorId, iter->second->GetPosition());
    _players.erase(iter);

    std::cout << "[Zone:" << _zoneId << "] Actor=" << actorId
        << " left (count=" << _players.size() << ")" << std::endl;
}

void IZone::OnActorMove(ActorId const actorId, Position const& oldPos, Position const& newPos)
{
    _grid.Move(actorId, oldPos, newPos);
}

void IZone::Broadcast(Packet const& packet)
{
    for (auto const& [id, player] : _players)
    {
        if (auto const session = player->GetSession())
        {
            session->SendPacket(packet);
        }
    }
}

void IZone::BroadcastInSight(Position const& center, Packet const& packet, ActorId const excludeActorId)
{
    std::vector<ActorId> nearbyIds;
    _grid.GetNearbyActorIds(center, nearbyIds);

    for (auto const actorId : nearbyIds)
    {
        if (excludeActorId == actorId)
        {
            continue;
        }

        auto const iter = _players.find(actorId);
        if (_players.end() == iter)
        {
            continue;
        }

        if (auto const session = iter->second->GetSession())
        {
            session->SendPacket(packet);
        }
    }
}

std::shared_ptr<Player> IZone::FindPlayer(ActorId const actorId) const
{
    auto const iter = _players.find(actorId);
    if (_players.end() == iter)
    {
        return nullptr;
    }
    return iter->second;
}
