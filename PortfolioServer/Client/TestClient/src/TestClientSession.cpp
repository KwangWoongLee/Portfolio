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

    auto constexpr ATTACK_TARGET_RANGE = 2000;
}

void TestClientSession::HandlePacket(uint16_t const packetId, void const* const payload, uint32_t const size)
{
    if (EPacketId::W2CWelcome == static_cast<EPacketId>(packetId))
    {
        W2CWelcome pkt;
        if (pkt.ReadFromBytes(payload, size))
        {
            _x = pkt._x;
            _z = pkt._z;
        }
    }
}

void TestClientSession::OnConnected()
{
    _rng.seed(static_cast<uint32_t>(static_cast<int64_t>(GetSessionId())));
    _x = 0.0f;
    _z = 0.0f;

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
}

void TestClientSession::TickMove()
{
    std::uniform_real_distribution<float> deltaDist(-MOVE_STEP, MOVE_STEP);

    _x += deltaDist(_rng);
    _z += deltaDist(_rng);

    if (MAP_HALF_EXTENT < _x) { _x = MAP_HALF_EXTENT; }
    if (_x < -MAP_HALF_EXTENT) { _x = -MAP_HALF_EXTENT; }
    if (MAP_HALF_EXTENT < _z) { _z = MAP_HALF_EXTENT; }
    if (_z < -MAP_HALF_EXTENT) { _z = -MAP_HALF_EXTENT; }

    C2WMove pkt;
    pkt._x = _x;
    pkt._z = _z;

    SendPacket(pkt);
    FlushPacketStream();
}

void TestClientSession::TickAttack()
{
    std::uniform_int_distribution<int64_t> targetDist(1, ATTACK_TARGET_RANGE);

    C2WAttack pkt;
    pkt._targetActorId = ActorId{ targetDist(_rng) };

    SendPacket(pkt);
    FlushPacketStream();
}
