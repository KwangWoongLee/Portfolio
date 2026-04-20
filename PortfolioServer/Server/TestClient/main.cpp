#include "CorePch.h"
#include "IOCP.h"
#include "Engine.h"
#include "Connector.h"
#include "IOCPSessionManager.h"
#include "TaskDispatcher.h"
#include "TimerManager.h"
#include "TestClientSession.h"

int main()
{
    auto const iocp = std::make_shared<IOCP>();
    Engine engine(iocp);

    IOCPSessionManager::Singleton::GetInstance().Init(iocp);

    auto& dispatcher = TaskDispatcher::Singleton::GetInstance();
    dispatcher.AddExecutor(ETaskType::NetworkIO, 1);
    dispatcher.AddExecutor(ETaskType::Timer, 1);

    TimerManager::Singleton::GetInstance().Start();

    Connector connector("127.0.0.1", 9000,
        []() -> std::shared_ptr<IOCPSession>
        {
            return IOCPSessionManager::Singleton::GetInstance().CreateSession<TestClientSession>();
        }
    );

    std::cout << "[Main] Connecting to 127.0.0.1:9000..." << std::endl;
    connector.AsyncConnect();

    engine.Run();

    return 0;
}
