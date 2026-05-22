#include "CorePch.h"
#include "IZone.h"
#include "Player.h"
#include "WorldPackets.h"

namespace
{
    W2CActorEnter MakeEnterPacket(Player const& player)
    {
        W2CActorEnter pkt;
        pkt._actorId = player.GetActorId();
        pkt._x = player.GetPosition()._x;
        pkt._z = player.GetPosition()._z;
        pkt._hp = player.GetHp();
        return pkt;
    }
}

bool IZone::Enter(std::shared_ptr<Player> const& player)
{
    auto const actorId = player->GetActorId();

    if (_players.contains(actorId))
    {
        return false;
    }

    _players.emplace(actorId, player);
    player->SetCurrentZoneId(_zoneId);

    auto const& pos = player->GetPosition();
    _grid.Add(actorId, pos);

    auto const selfPkt = MakeEnterPacket(*player);
    BroadcastInSight(pos, selfPkt, actorId);

    std::cout << "[Zone:" << _zoneId << "] Player actor=" << actorId
        << " entered (count=" << _players.size() << ")" << std::endl;

    return true;
}

void IZone::UpdateSnapshotCache()
{
    auto newSnapshot = std::make_shared<std::vector<ActorSnapshot>>();
    newSnapshot->reserve(_players.size());

    for (auto const& [id, player] : _players)
    {
        ActorSnapshot snap;
        snap._actorId = id;
        snap._x = player->GetPosition()._x;
        snap._z = player->GetPosition()._z;
        snap._hp = player->GetHp();
        newSnapshot->push_back(snap);
    }

    _cachedSnapshot.store(std::move(newSnapshot), std::memory_order_release);
}

void IZone::CollectAllSnapshots(std::vector<ActorSnapshot>& outSnapshots) const
{
    outSnapshots.reserve(outSnapshots.size() + _players.size());

    for (auto const& [id, player] : _players)
    {
        ActorSnapshot snap;
        snap._actorId = id;
        snap._x = player->GetPosition()._x;
        snap._z = player->GetPosition()._z;
        snap._hp = player->GetHp();
        outSnapshots.emplace_back(snap);
    }
}

void IZone::GetSightSnapshot(ActorId const selfActorId, std::vector<ActorSnapshot>& outSnapshots) const
{
    auto const iter = _players.find(selfActorId);
    if (_players.end() == iter)
    {
        return;
    }

    std::vector<ActorId> nearbyIds;
    _grid.GetNearbyActorIds(iter->second->GetPosition(), nearbyIds);

    outSnapshots.reserve(outSnapshots.size() + nearbyIds.size());

    for (auto const otherId : nearbyIds)
    {
        if (otherId == selfActorId)
        {
            continue;
        }
        auto const playerIter = _players.find(otherId);
        if (_players.end() == playerIter)
        {
            continue;
        }
        auto const& other = *playerIter->second;

        ActorSnapshot snap;
        snap._actorId = otherId;
        snap._x = other.GetPosition()._x;
        snap._z = other.GetPosition()._z;
        snap._hp = other.GetHp();
        outSnapshots.emplace_back(snap);
    }
}

void IZone::Leave(ActorId const actorId)
{
    auto const iter = _players.find(actorId);
    if (_players.end() == iter)
    {
        return;
    }

    auto const& pos = iter->second->GetPosition();

    W2CActorLeave selfPkt;
    selfPkt._actorId = actorId;
    BroadcastInSight(pos, selfPkt, actorId);

    _grid.Remove(actorId, pos);
    _players.erase(iter);

    std::cout << "[Zone:" << _zoneId << "] Actor=" << actorId
        << " left (count=" << _players.size() << ")" << std::endl;
}

void IZone::OnMessage(ZoneMsg::ActorMoved const& msg)
{
    OnActorMove(msg._actorId, msg._oldPosition, msg._newPosition);

    W2CMoveBroadcast packet;
    packet._actorId = msg._actorId;
    packet._x = msg._newPosition._x;
    packet._z = msg._newPosition._z;
    BroadcastInSight(msg._newPosition, packet, msg._actorId);
}

void IZone::OnMessage(ZoneMsg::PlayerEntered const& msg)
{
    auto const& player = msg._player;
    Enter(player);

    W2CWelcome welcomePacket;
    welcomePacket._actorId = player->GetActorId();
    welcomePacket._x = player->GetPosition()._x;
    welcomePacket._z = player->GetPosition()._z;

    GetSightSnapshot(player->GetActorId(), welcomePacket._nearby);

    if (auto const session = player->GetSession())
    {
        session->SendPacket(welcomePacket);
        session->FlushPacketStream();
    }
}

void IZone::OnMessage(ZoneMsg::PlayerLeft const& msg)
{
    Leave(msg._actorId);
}

void IZone::OnMessage(ZoneMsg::BroadcastInSightRequest const& msg)
{
    BroadcastInSight(msg._center, *msg._packet, msg._excludeActorId);
}

void IZone::OnActorMove(ActorId const actorId, Position const& oldPos, Position const& newPos)
{
    _grid.Move(actorId, oldPos, newPos);

    auto const self = FindPlayer(actorId);
    if (not self)
    {
        return;
    }

    std::vector<ActorId> enteredIds;
    std::vector<ActorId> leftIds;
    _grid.GetSightDiff(newPos, oldPos, enteredIds, leftIds);

    auto const selfSession = self->GetSession();
    auto const selfEnterPkt = MakeEnterPacket(*self);

    W2CActorLeave selfLeavePkt;
    selfLeavePkt._actorId = actorId;

    for (auto const otherId : enteredIds)
    {
        if (otherId == actorId)
        {
            continue;
        }
        auto const other = FindPlayer(otherId);
        if (not other)
        {
            continue;
        }

        if (selfSession)
        {
            auto const otherPkt = MakeEnterPacket(*other);
            selfSession->SendPacket(otherPkt);
        }

        if (auto const otherSession = other->GetSession())
        {
            otherSession->SendPacket(selfEnterPkt);
        }
    }

    for (auto const otherId : leftIds)
    {
        if (otherId == actorId)
        {
            continue;
        }
        auto const other = FindPlayer(otherId);
        if (not other)
        {
            continue;
        }

        if (selfSession)
        {
            W2CActorLeave otherPkt;
            otherPkt._actorId = otherId;
            selfSession->SendPacket(otherPkt);
        }

        if (auto const otherSession = other->GetSession())
        {
            otherSession->SendPacket(selfLeavePkt);
        }
    }
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
