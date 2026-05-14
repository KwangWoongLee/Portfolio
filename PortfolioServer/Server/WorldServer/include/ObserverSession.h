#pragma once
#include "CorePch.h"
#include "IOCPSession.h"

class ObserverSession final
    : public IOCPSession
{
public:
    void HandlePacket(uint16_t const packetId, void const* const payload, uint32_t const size) override
    {
        (void)packetId;
        (void)payload;
        (void)size;
    }

private:
    void OnConnected() override;
    void OnDisconnected() override;
};
