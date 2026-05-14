#pragma once
#include "CorePch.h"

namespace Metrics
{
    inline std::atomic<uint64_t> g_recvPacketCount{};
    inline std::atomic<uint64_t> g_sendPacketCount{};

    inline void OnRecvPacket()
    {
        g_recvPacketCount.fetch_add(1, std::memory_order_relaxed);
    }

    inline void OnSendPacket()
    {
        g_sendPacketCount.fetch_add(1, std::memory_order_relaxed);
    }
}
