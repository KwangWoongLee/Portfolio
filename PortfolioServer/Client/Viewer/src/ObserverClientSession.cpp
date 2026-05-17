#include "CorePch.h"
#include "ObserverClientSession.h"
#include "ObserverClient.h"
#include "ObserverPackets.h"
#include "PacketId.h"

void ObserverClientSession::HandlePacket(uint16_t const packetId, void const* const payload, uint32_t const size)
{
    if (EPacketId::W2OSnapshot == static_cast<EPacketId>(packetId))
    {
        W2OSnapshot pkt;
        if (pkt.ReadFromBytes(payload, size))
        {
            ObserverClient::Singleton::GetInstance().OnSnapshot(std::move(pkt));
        }
    }
}

void ObserverClientSession::OnConnected()
{
    ObserverClient::Singleton::GetInstance().SetConnected(true);
    std::cout << "[ObserverClient] Connected" << std::endl;
}

void ObserverClientSession::OnDisconnected()
{
    ObserverClient::Singleton::GetInstance().SetConnected(false);
    std::cout << "[ObserverClient] Disconnected" << std::endl;
}
