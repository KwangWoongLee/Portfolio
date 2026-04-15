#include "CorePch.h"
#include "WorldServerApp.h"
#include "ClientSession.h"
#include "IOCPSessionManager.h"
#include "ZoneManager.h"

uint16_t constexpr WORLD_PORT = 9000;

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

    InitZones();

    return true;
}

void WorldServerApp::Run()
{
    _engine->Run();
}

void WorldServerApp::Stop()
{
    _engine->Stop();
}

void WorldServerApp::InitZones()
{
    auto& zoneMgr = ZoneManager::Singleton::GetInstance();

    zoneMgr.CreateField(1);  // 마을
    zoneMgr.CreateField(2);  // 사냥터

    std::cout << "[WorldServer] Zones initialized" << std::endl;
}
