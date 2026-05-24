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
}
