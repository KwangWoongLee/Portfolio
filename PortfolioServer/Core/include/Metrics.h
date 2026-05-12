#pragma once
#include "CorePch.h"

namespace Metrics
{
    inline std::atomic<uint64_t> g_recvPackets{};
    inline std::atomic<uint64_t> g_sendPackets{};
    inline std::atomic<uint32_t> g_ccu{};
    inline std::atomic<uint32_t> g_taskQueueLen{};

    inline void OnRecvPacket()
    {
        g_recvPackets.fetch_add(1);
    }

    inline void OnSendPacket()
    {
        g_sendPackets.fetch_add(1);
    }
}
