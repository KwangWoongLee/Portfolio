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
}
