#include "CorePch.h"
#include "IZone.h"
#include "IOCPSession.h"
#include "WorldPackets.h"
#include "Metrics.h"

namespace
{
    W2CActorEnter MakeEnterPacket(ActorId const actorId, Position const& position, int32_t const hp)
    {
        W2CActorEnter pkt;
        pkt._actorId = actorId;
        pkt._x = position._x;
        pkt._z = position._z;
        pkt._hp = hp;
        return pkt;
    }
}

bool IZone::Enter(ZoneMsg::PlayerEntered const& msg)
{
    auto const actorId = msg._actorId;

    if (_actors.contains(actorId))
    {
        return false;
    }

    ZoneActorState state;
    state._actorId = actorId;
    state._session = msg._session;
    state._position = msg._position;
    state._hp = msg._hp;

    _actors.emplace(actorId, std::move(state));
    Metrics::OnZonePlayerEntered();
    _grid.Add(actorId, msg._position);
    _cachedActorCount.store(_actors.size(), std::memory_order_relaxed);

    auto const selfPkt = MakeEnterPacket(actorId, msg._position, msg._hp);
    BroadcastInSight(msg._position, selfPkt, actorId);

    std::cout << "[Zone:" << _zoneId << "] Player actor=" << actorId
        << " entered (count=" << _actors.size() << ")" << std::endl;

    return true;
}

void IZone::UpdateSnapshotCache()
{
    auto newSnapshot = std::make_shared<std::vector<ActorSnapshot>>();
    newSnapshot->reserve(_actors.size());

    for (auto const& [id, actor] : _actors)
    {
        ActorSnapshot snap;
        snap._actorId = id;
        snap._x = actor._position._x;
        snap._z = actor._position._z;
        snap._hp = actor._hp;
        newSnapshot->push_back(snap);
    }

    _cachedSnapshot.store(std::move(newSnapshot), std::memory_order_release);
}

void IZone::CollectAllSnapshots(std::vector<ActorSnapshot>& outSnapshots) const
{
    outSnapshots.reserve(outSnapshots.size() + _actors.size());

    for (auto const& [id, actor] : _actors)
    {
        ActorSnapshot snap;
        snap._actorId = id;
        snap._x = actor._position._x;
        snap._z = actor._position._z;
        snap._hp = actor._hp;
        outSnapshots.emplace_back(snap);
    }
}

void IZone::GetSightSnapshot(ActorId const selfActorId, std::vector<ActorSnapshot>& outSnapshots) const
{
    auto const iter = _actors.find(selfActorId);
    if (_actors.end() == iter)
    {
        return;
    }

    std::vector<ActorId> nearbyIds;
    _grid.GetNearbyActorIds(iter->second._position, nearbyIds);

    outSnapshots.reserve(outSnapshots.size() + nearbyIds.size());

    for (auto const otherId : nearbyIds)
    {
        if (otherId == selfActorId)
        {
            continue;
        }
        auto const actorIter = _actors.find(otherId);
        if (_actors.end() == actorIter)
        {
            continue;
        }
        auto const& other = actorIter->second;

        ActorSnapshot snap;
        snap._actorId = otherId;
        snap._x = other._position._x;
        snap._z = other._position._z;
        snap._hp = other._hp;
        outSnapshots.emplace_back(snap);
    }
}

void IZone::Leave(ActorId const actorId)
{
    auto const iter = _actors.find(actorId);
    if (_actors.end() == iter)
    {
        return;
    }

    auto const pos = iter->second._position;

    W2CActorLeave selfPkt;
    selfPkt._actorId = actorId;
    BroadcastInSight(pos, selfPkt, actorId);

    _grid.Remove(actorId, pos);
    _actors.erase(iter);
    _cachedActorCount.store(_actors.size(), std::memory_order_relaxed);

    std::cout << "[Zone:" << _zoneId << "] Actor=" << actorId
        << " left (count=" << _actors.size() << ")" << std::endl;
}

void IZone::OnMessage(ZoneMsg::ActorMoved const& msg)
{
    Metrics::OnZoneActorMoved();
    OnActorMove(msg._actorId, msg._oldPosition, msg._newPosition);

    W2CMoveBroadcast packet;
    packet._actorId = msg._actorId;
    packet._x = msg._newPosition._x;
    packet._z = msg._newPosition._z;
    BroadcastInSight(msg._newPosition, packet, msg._actorId);
}

bool IZone::OnMessage(ZoneMsg::PlayerEntered const& msg)
{
    if (not Enter(msg))
    {
        return false;
    }

    W2CWelcome welcomePacket;
    welcomePacket._actorId = msg._actorId;
    welcomePacket._x = msg._position._x;
    welcomePacket._z = msg._position._z;

    GetSightSnapshot(msg._actorId, welcomePacket._nearby);

    if (auto const session = msg._session)
    {
        session->SendPacket(welcomePacket);
        session->FlushPacketStream();
    }

    return true;
}

void IZone::OnMessage(ZoneMsg::PlayerLeft const& msg)
{
    Leave(msg._actorId);
}

void IZone::OnMessage(ZoneMsg::BroadcastInSightRequest const& msg)
{
    BroadcastInSight(msg._center, *msg._packet, msg._excludeActorId);
}

void IZone::OnMessage(ZoneMsg::HpChanged const& msg)
{
    Metrics::OnZoneHpChanged();
    auto const actor = FindActor(msg._actorId);
    auto center = msg._center;
    if (actor)
    {
        actor->_hp = msg._hp;
        center = actor->_position;
    }

    W2CHpUpdate packet;
    packet._actorId = msg._actorId;
    packet._hp = msg._hp;
    BroadcastInSight(center, packet, msg._excludeActorId);
}

void IZone::OnMessage(ZoneMsg::ActorDied const& msg)
{
    Metrics::OnZoneActorDied();
    auto const actor = FindActor(msg._actorId);
    auto center = msg._center;
    if (actor)
    {
        center = actor->_position;
    }

    W2CDeath packet;
    packet._actorId = msg._actorId;
    BroadcastInSight(center, packet, msg._excludeActorId);
}

void IZone::OnActorMove(ActorId const actorId, Position const& oldPos, Position const& newPos)
{
    (void)oldPos;

    auto const self = FindActor(actorId);
    if (not self)
    {
        return;
    }

    auto const prevPos = self->_position;
    self->_position = newPos;

    _grid.Move(actorId, prevPos, newPos);

    std::vector<ActorId> enteredIds;
    std::vector<ActorId> leftIds;
    _grid.GetSightDiff(newPos, prevPos, enteredIds, leftIds);
    Metrics::OnZoneMoveSightDiff(enteredIds.size(), leftIds.size());

    auto const selfSession = self->_session.lock();
    auto const selfEnterPkt = MakeEnterPacket(actorId, self->_position, self->_hp);

    W2CActorLeave selfLeavePkt;
    selfLeavePkt._actorId = actorId;

    for (auto const otherId : enteredIds)
    {
        if (otherId == actorId)
        {
            continue;
        }
        auto const other = FindActor(otherId);
        if (not other)
        {
            continue;
        }

        if (selfSession)
        {
            auto const otherPkt = MakeEnterPacket(otherId, other->_position, other->_hp);
            selfSession->SendPacket(otherPkt);
            selfSession->FlushPacketStream();
        }

        if (auto const otherSession = other->_session.lock())
        {
            otherSession->SendPacket(selfEnterPkt);
            otherSession->FlushPacketStream();
        }
    }

    for (auto const otherId : leftIds)
    {
        if (otherId == actorId)
        {
            continue;
        }
        auto const other = FindActor(otherId);
        if (not other)
        {
            continue;
        }

        if (selfSession)
        {
            W2CActorLeave otherPkt;
            otherPkt._actorId = otherId;
            selfSession->SendPacket(otherPkt);
            selfSession->FlushPacketStream();
        }

        if (auto const otherSession = other->_session.lock())
        {
            otherSession->SendPacket(selfLeavePkt);
            otherSession->FlushPacketStream();
        }
    }
}

void IZone::Broadcast(Packet const& packet)
{
    for (auto const& [id, actor] : _actors)
    {
        if (auto const session = actor._session.lock())
        {
            session->SendPacket(packet);
            session->FlushPacketStream();
        }
    }
}

void IZone::BroadcastInSight(Position const& center, Packet const& packet, ActorId const excludeActorId)
{
    std::vector<ActorId> nearbyIds;
    _grid.GetNearbyActorIds(center, nearbyIds);

    size_t recipientCount{ 0 };
    for (auto const actorId : nearbyIds)
    {
        if (excludeActorId == actorId)
        {
            continue;
        }

        auto const iter = _actors.find(actorId);
        if (_actors.end() == iter)
        {
            continue;
        }

        if (auto const session = iter->second._session.lock())
        {
            session->SendPacket(packet);
            session->FlushPacketStream();
            ++recipientCount;
        }
    }

    Metrics::OnZoneBroadcastInSight(recipientCount);
}

IZone::ZoneActorState* IZone::FindActor(ActorId const actorId)
{
    auto const iter = _actors.find(actorId);
    if (_actors.end() == iter)
    {
        return nullptr;
    }
    return &iter->second;
}

IZone::ZoneActorState const* IZone::FindActor(ActorId const actorId) const
{
    auto const iter = _actors.find(actorId);
    if (_actors.end() == iter)
    {
        return nullptr;
    }
    return &iter->second;
}
