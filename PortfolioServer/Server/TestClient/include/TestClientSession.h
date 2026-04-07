#pragma once
#include "CorePch.h"
#include "IOCPSession.h"
#include "Packet.h"

struct TestPacket final : public Packet
{
    MAKABLE_STREAM(1, _value)

    int32_t _value{};
};

class TestClientSession final
    : public IOCPSession
{
public:
    void HandlePacket(uint16_t packetId, void const* payload, uint32_t size) override
    {
        std::cout << "[Client:" << GetSessionId() << "] "
            << "Recv PacketId=" << packetId << " Size=" << size << std::endl;
    }

private:
    void OnConnected() override;
    void OnDisconnected() override;
};
