#include "CorePch.h"
#include "TestClientSession.h"

#include "IOCPSessionManager.h"
#include "TimerManager.h"

void TestClientSession::OnConnected()
{
    std::cout << "[Client:" << GetSessionId() << "] Connected to server!" << std::endl;

    auto const sessionId = GetSessionId();

    TimerManager::Singleton::GetInstance().AddRepeatTimer(
        std::chrono::seconds(2),
        sessionId,
        [sessionId]()
        {
            auto const session = IOCPSessionManager::Singleton::GetInstance().Find(sessionId);
            if (not session)
            {
                return;
            }

            static int32_t seq = 0;
            TestPacket pkt;
            pkt._value = ++seq;

            std::cout << "[Client] Sending TestPacket value=" << pkt._value << std::endl;

            session->SendPacket(pkt);
            session->FlushPacketStream();
        }
    );
}

void TestClientSession::OnDisconnected()
{
    std::cout << "[Client:" << GetSessionId() << "] Disconnected" << std::endl;
}
