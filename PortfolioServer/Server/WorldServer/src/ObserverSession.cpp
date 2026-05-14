#include "CorePch.h"
#include "ObserverSession.h"
#include "ObserverManager.h"
#include "IOCPSessionManager.h"

void ObserverSession::OnConnected()
{
    auto const session = std::static_pointer_cast<ObserverSession>(shared_from_this());
    ObserverManager::Singleton::GetInstance().Register(session);

    std::cout << "[ObserverSession:" << GetSessionId() << "] Connected" << std::endl;
}

void ObserverSession::OnDisconnected()
{
    ObserverManager::Singleton::GetInstance().Unregister(GetSessionId());

    std::cout << "[ObserverSession:" << GetSessionId() << "] Disconnected" << std::endl;
}
