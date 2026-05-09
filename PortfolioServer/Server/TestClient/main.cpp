#include "CorePch.h"
#include "IOCP.h"
#include "Engine.h"
#include "Connector.h"
#include "IOCPSessionManager.h"
#include "TaskDispatcher.h"
#include "TimerManager.h"
#include "TestClientSession.h"

namespace
{
    auto constexpr TARGET_CONNECTIONS = 1000;
    auto constexpr SPAWN_INTERVAL = std::chrono::milliseconds(1);

    auto constexpr NETWORK_IO_THREADS = 4;
    auto constexpr TIMER_THREADS = 2;
}

int main()
{
    auto const iocp = std::make_shared<IOCP>();
    Engine engine(iocp);

    IOCPSessionManager::Singleton::GetInstance().Init(iocp);

    auto& dispatcher = TaskDispatcher::Singleton::GetInstance();
    dispatcher.AddExecutor(ETaskType::NetworkIO, NETWORK_IO_THREADS);
    dispatcher.AddExecutor(ETaskType::Timer, TIMER_THREADS);

    TimerManager::Singleton::GetInstance().Start();

    Connector connector("127.0.0.1", 9000,
        []() -> std::shared_ptr<IOCPSession>
        {
            return IOCPSessionManager::Singleton::GetInstance().CreateSession<TestClientSession>();
        }
    );

    std::cout << "[Main] Spawning " << TARGET_CONNECTIONS << " connections..." << std::endl;

    std::thread spawner(
        [&connector]()
        {
            for (int32_t i = 0; i < TARGET_CONNECTIONS; ++i)
            {
                connector.AsyncConnect();
                std::this_thread::sleep_for(SPAWN_INTERVAL);
            }
            std::cout << "[Main] All connect requests issued." << std::endl;
        });
    spawner.detach();

    engine.Run();

    return 0;
}
