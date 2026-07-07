#pragma once
#include "CorePch.h"

namespace Metrics
{
    inline std::atomic<uint64_t> g_recvPacketCount{};
    inline std::atomic<uint64_t> g_sendPacketCount{};
    inline std::atomic<uint64_t> g_recvByteCount{};
    inline std::atomic<uint64_t> g_sendByteCount{};

    inline void OnRecvPacket()
    {
        g_recvPacketCount.fetch_add(1, std::memory_order_relaxed);
    }

    inline void OnSendPacket()
    {
        g_sendPacketCount.fetch_add(1, std::memory_order_relaxed);
    }

    inline void OnRecvBytes(uint32_t const byteCount)
    {
        g_recvByteCount.fetch_add(byteCount, std::memory_order_relaxed);
    }

    inline void OnSendBytes(uint32_t const byteCount)
    {
        g_sendByteCount.fetch_add(byteCount, std::memory_order_relaxed);
    }

    // 모든 세션의 _sendBuffer 누적 byte (현재 시점, gauge).
    // SendLocked에서 +, OnSendCompleted에서 -.
    // 64KB(=Stream::MAX_SIZE) * 세션 수에 가까워지면 backpressure.
    inline std::atomic<uint64_t> g_pendingSendBytesTotal{};

    inline void OnPendingSendAdd(uint32_t const byteCount)
    {
        g_pendingSendBytesTotal.fetch_add(byteCount, std::memory_order_relaxed);
    }

    inline void OnPendingSendSub(uint32_t const byteCount)
    {
        g_pendingSendBytesTotal.fetch_sub(byteCount, std::memory_order_relaxed);
    }

    // SendOverflow disconnect 누적 횟수 (counter).
    inline std::atomic<uint64_t> g_sendOverflowCount{};

    inline void OnSendOverflow()
    {
        g_sendOverflowCount.fetch_add(1, std::memory_order_relaxed);
    }

    inline std::atomic<uint64_t> g_disconnectCount{};

    inline void OnDisconnect()
    {
        g_disconnectCount.fetch_add(1, std::memory_order_relaxed);
    }
    inline std::atomic<uint64_t> g_playerMoveRequestCount{};
    inline std::atomic<uint64_t> g_playerAttackRequestCount{};
    inline std::atomic<uint64_t> g_playerAttackedCount{};

    inline std::atomic<uint64_t> g_zoneActorMovedCount{};
    inline std::atomic<uint64_t> g_zonePlayerEnteredCount{};
    inline std::atomic<uint64_t> g_zoneHpChangedCount{};
    inline std::atomic<uint64_t> g_zoneActorDiedCount{};

    inline std::atomic<uint64_t> g_zoneMoveSightDiffCount{};
    inline std::atomic<uint64_t> g_zoneMoveSightEnteredCount{};
    inline std::atomic<uint64_t> g_zoneMoveSightLeftCount{};

    inline std::atomic<uint64_t> g_zoneBroadcastInSightCount{};
    inline std::atomic<uint64_t> g_zoneBroadcastRecipientCount{};

    inline std::atomic<uint64_t> g_gridNearbyQueryCount{};
    inline std::atomic<uint64_t> g_gridNearbyResultCount{};

    inline void OnPlayerMoveRequest()
    {
        g_playerMoveRequestCount.fetch_add(1, std::memory_order_relaxed);
    }

    inline void OnPlayerAttackRequest()
    {
        g_playerAttackRequestCount.fetch_add(1, std::memory_order_relaxed);
    }

    inline void OnPlayerAttacked()
    {
        g_playerAttackedCount.fetch_add(1, std::memory_order_relaxed);
    }

    inline void OnZoneActorMoved()
    {
        g_zoneActorMovedCount.fetch_add(1, std::memory_order_relaxed);
    }

    inline void OnZonePlayerEntered()
    {
        g_zonePlayerEnteredCount.fetch_add(1, std::memory_order_relaxed);
    }

    inline void OnZoneHpChanged()
    {
        g_zoneHpChangedCount.fetch_add(1, std::memory_order_relaxed);
    }

    inline void OnZoneActorDied()
    {
        g_zoneActorDiedCount.fetch_add(1, std::memory_order_relaxed);
    }

    inline void OnZoneMoveSightDiff(size_t const enteredCount, size_t const leftCount)
    {
        g_zoneMoveSightDiffCount.fetch_add(1, std::memory_order_relaxed);
        g_zoneMoveSightEnteredCount.fetch_add(enteredCount, std::memory_order_relaxed);
        g_zoneMoveSightLeftCount.fetch_add(leftCount, std::memory_order_relaxed);
    }

    inline void OnZoneBroadcastInSight(size_t const recipientCount)
    {
        g_zoneBroadcastInSightCount.fetch_add(1, std::memory_order_relaxed);
        g_zoneBroadcastRecipientCount.fetch_add(recipientCount, std::memory_order_relaxed);
    }

    inline void OnGridNearbyQuery(size_t const resultCount)
    {
        g_gridNearbyQueryCount.fetch_add(1, std::memory_order_relaxed);
        g_gridNearbyResultCount.fetch_add(resultCount, std::memory_order_relaxed);
    }
}
