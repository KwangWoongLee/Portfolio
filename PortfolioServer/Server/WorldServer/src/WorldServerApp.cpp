#include "CorePch.h"
#include "WorldServerApp.h"
#include "ClientSession.h"
#include "ObserverSession.h"
#include "ObserverManager.h"
#include "MetricsLogger.h"
#include "IOCPSessionManager.h"
#include "TimerManager.h"
#include "ZoneManager.h"
#include <iomanip>

auto constexpr WORLD_PORT = 9000;
auto constexpr OBSERVER_PORT = 9001;
auto constexpr OBSERVER_PUSH_INTERVAL = std::chrono::milliseconds(100);

namespace
{
    auto const METRICS_CSV_PATH = "metrics/server_metrics.csv";
}

bool WorldServerApp::Init()
{
    _iocp = std::make_shared<IOCP>();
    _engine = std::make_unique<ServerEngine>(_iocp);

    if (not _engine->AddListener(WORLD_PORT, ELinkType::Client,
        []() -> std::shared_ptr<IOCPSession>
        {
            return IOCPSessionManager::Singleton::GetInstance().CreateSession<ClientSession>();
        }))
    {
        std::cout << "[WorldServer] Failed to add listener on port " << WORLD_PORT << std::endl;
        return false;
    }

    std::cout << "[WorldServer] Listening on port " << WORLD_PORT << std::endl;

    if (not _engine->AddListener(OBSERVER_PORT, ELinkType::Observer,
        []() -> std::shared_ptr<IOCPSession>
        {
            return IOCPSessionManager::Singleton::GetInstance().CreateSession<ObserverSession>();
        }))
    {
        std::cout << "[WorldServer] Failed to add observer listener on port " << OBSERVER_PORT << std::endl;
        return false;
    }

    std::cout << "[WorldServer] Observer listening on port " << OBSERVER_PORT << std::endl;

    if (not MetricsLogger::Singleton::GetInstance().Start(METRICS_CSV_PATH))
    {
        std::cout << "[WorldServer] Failed to start MetricsLogger" << std::endl;
        return false;
    }

    InitZones();

    TimerManager::Singleton::GetInstance().AddRepeatTimer(
        OBSERVER_PUSH_INTERVAL,
        0,
        []()
        {
            ObserverManager::Singleton::GetInstance().PushSnapshot();
        });

    return true;
}

void WorldServerApp::Run()
{
    _engine->Run();
}

void WorldServerApp::Stop()
{
    MetricsLogger::Singleton::GetInstance().Stop();
    _engine->Stop();
}

void WorldServerApp::InitZones()
{
    auto& zoneMgr = ZoneManager::Singleton::GetInstance();

    zoneMgr.CreateField(1);  // 마을
    zoneMgr.CreateField(2);  // 사냥터

    std::cout << "[WorldServer] Zones initialized" << std::endl;
}
