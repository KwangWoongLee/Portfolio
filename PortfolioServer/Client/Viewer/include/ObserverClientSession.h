#pragma once
#include "CorePch.h"
#include "IOCPSession.h"

class ObserverClientSession final
    : public IOCPSession
{
public:
    void HandlePacket(uint16_t const packetId, void const* const payload, uint32_t const size) override;

private:
    void OnConnected() override;
    void OnDisconnected() override;
};
