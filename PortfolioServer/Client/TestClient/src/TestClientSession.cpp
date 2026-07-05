#include "CorePch.h"
#include "TestClientSession.h"

#include "IOCPSessionManager.h"
#include "TimerManager.h"
#include "PacketId.h"
#include "WorldPackets.h"

namespace
{
    auto constexpr MOVE_INTERVAL = std::chrono::milliseconds(100);
    auto constexpr ATTACK_INTERVAL = std::chrono::milliseconds(250);

    auto constexpr MAP_HALF_EXTENT = 1000.0f;
    auto constexpr MOVE_STEP = 5.0f;
}

void TestClientSession::HandlePacket(uint16_t const packetId, void const* const payload, uint32_t const size)
{
    switch (static_cast<EPacketId>(packetId))
    {
    case EPacketId::W2CWelcome:
        {
            W2CWelcome pkt;
            if (not pkt.ReadFromBytes(payload, size))
            {
                return;
            }

            std::scoped_lock lock(_stateMutex);
            _selfActorId = pkt._actorId;
            _x = pkt._x;
            _z = pkt._z;
            _visibleActorIds.clear();
            for (auto const& actor : pkt._nearby)
            {
                if (actor._actorId != _selfActorId)
                {
                    _visibleActorIds.insert(actor._actorId);
                }
            }
        } break;
    case EPacketId::W2CActorEnter:
        {
            W2CActorEnter pkt;
            if (pkt.ReadFromBytes(payload, size))
            {
                TrackVisibleActor(pkt._actorId);
            }
        } break;
    case EPacketId::W2CActorLeave:
        {
            W2CActorLeave pkt;
            if (pkt.ReadFromBytes(payload, size))
            {
                UntrackVisibleActor(pkt._actorId);
            }
        } break;
    case EPacketId::W2CMoveBroadcast:
        {
            W2CMoveBroadcast pkt;
            if (pkt.ReadFromBytes(payload, size))
            {
                TrackVisibleActor(pkt._actorId);
            }
        } break;
    case EPacketId::W2CHpUpdate:
        {
            W2CHpUpdate pkt;
            if (pkt.ReadFromBytes(payload, size))
            {
                TrackVisibleActor(pkt._actorId);
            }
        } break;
    case EPacketId::W2CDeath:
        {
            W2CDeath pkt;
            if (pkt.ReadFromBytes(payload, size))
            {
                TrackVisibleActor(pkt._actorId);
            }
        } break;
    default:
        break;
    }
}

void TestClientSession::OnConnected()
{
    _rng.seed(static_cast<uint32_t>(static_cast<int64_t>(GetSessionId())));
    {
        std::scoped_lock lock(_stateMutex);
        _selfActorId = INVALID_ACTOR_ID;
        _visibleActorIds.clear();
        _x = 0.0f;
        _z = 0.0f;
    }

    auto const sessionId = GetSessionId();

    _moveTimerId = TimerManager::Singleton::GetInstance().AddRepeatTimer(
        MOVE_INTERVAL,
        sessionId,
        [sessionId]()
        {
            auto const session = IOCPSessionManager::Singleton::GetConstInstance().Find(sessionId);
            if (not session)
            {
                return;
            }
            std::static_pointer_cast<TestClientSession>(session)->TickMove();
        });

    _attackTimerId = TimerManager::Singleton::GetInstance().AddRepeatTimer(
        ATTACK_INTERVAL,
        sessionId,
        [sessionId]()
        {
            auto const session = IOCPSessionManager::Singleton::GetConstInstance().Find(sessionId);
            if (not session)
            {
                return;
            }
            std::static_pointer_cast<TestClientSession>(session)->TickAttack();
        });
}

void TestClientSession::OnDisconnected()
{
    if (0 != _moveTimerId)
    {
        TimerManager::Singleton::GetInstance().CancelTimer(_moveTimerId);
        _moveTimerId = 0;
    }
    if (0 != _attackTimerId)
    {
        TimerManager::Singleton::GetInstance().CancelTimer(_attackTimerId);
        _attackTimerId = 0;
    }

    std::scoped_lock lock(_stateMutex);
    _selfActorId = INVALID_ACTOR_ID;
    _visibleActorIds.clear();
}

void TestClientSession::TickMove()
{
    std::uniform_real_distribution<float> deltaDist(-MOVE_STEP, MOVE_STEP);

    float x{ 0.0f };
    float z{ 0.0f };

    {
        std::scoped_lock lock(_stateMutex);

        _x += deltaDist(_rng);
        _z += deltaDist(_rng);

        if (MAP_HALF_EXTENT < _x) { _x = MAP_HALF_EXTENT; }
        if (_x < -MAP_HALF_EXTENT) { _x = -MAP_HALF_EXTENT; }
        if (MAP_HALF_EXTENT < _z) { _z = MAP_HALF_EXTENT; }
        if (_z < -MAP_HALF_EXTENT) { _z = -MAP_HALF_EXTENT; }

        x = _x;
        z = _z;
    }

    C2WMove pkt;
    pkt._x = x;
    pkt._z = z;

    SendPacket(pkt);
    FlushPacketStream();
}

void TestClientSession::TickAttack()
{
    ActorId targetActorId{ INVALID_ACTOR_ID };

    {
        std::scoped_lock lock(_stateMutex);
        if (_visibleActorIds.empty())
        {
            return;
        }

        std::uniform_int_distribution<size_t> targetDist(0, _visibleActorIds.size() - 1);
        auto iter = _visibleActorIds.begin();
        std::advance(iter, targetDist(_rng));
        targetActorId = *iter;
    }

    C2WAttack pkt;
    pkt._targetActorId = targetActorId;

    SendPacket(pkt);
    FlushPacketStream();
}

void TestClientSession::TrackVisibleActor(ActorId const actorId)
{
    std::scoped_lock lock(_stateMutex);
    if (actorId == INVALID_ACTOR_ID || actorId == _selfActorId)
    {
        return;
    }

    _visibleActorIds.insert(actorId);
}

void TestClientSession::UntrackVisibleActor(ActorId const actorId)
{
    std::scoped_lock lock(_stateMutex);
    _visibleActorIds.erase(actorId);
}
