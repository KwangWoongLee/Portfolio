#pragma once
#include "CorePch.h"
#include "IOCPSession.h"

class TestSession final
    : public IOCPSession
{
public:
    void HandlePacket(uint16_t packetId, void const* payload, uint32_t size) override
    {
        std::cout << "[TestSession:" << GetSessionId() << "] "
            << "PacketId=" << packetId << " Size=" << size << std::endl;
    }

private:
    void OnConnected() override
    {
        std::cout << "[TestSession:" << GetSessionId() << "] Connected" << std::endl;
    }

    void OnDisconnected() override
    {
        std::cout << "[TestSession:" << GetSessionId() << "] Disconnected" << std::endl;
    }
};
