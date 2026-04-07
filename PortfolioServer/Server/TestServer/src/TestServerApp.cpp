#include "CorePch.h"
#include "TestServerApp.h"
#include "TestSession.h"
#include "IOCPSessionManager.h"
#include "TimerManager.h"

uint16_t constexpr TEST_PORT = 9000;

bool TestServerApp::Init()
{
    _iocp = std::make_shared<IOCP>();
    _engine = std::make_unique<ServerEngine>(_iocp);

    if (not _engine->AddListener(TEST_PORT, ELinkType::Client,
        []() -> std::shared_ptr<IOCPSession>
        {
            return IOCPSessionManager::Singleton::GetInstance().CreateSession<TestSession>();
        }))
    {
        std::cout << "[TestServer] Failed to add listener on port " << TEST_PORT << std::endl;
        return false;
    }

    std::cout << "[TestServer] Listening on port " << TEST_PORT << std::endl;

    // 타이머 테스트: 1초마다 반복
    TimerManager::Singleton::GetInstance().AddRepeatTimer(
        std::chrono::seconds(1),
        0,
        []()
        {
            static int32_t tickCount = 0;
            std::cout << "[Timer] Tick #" << ++tickCount << std::endl;
        }
    );

    // 타이머 테스트: 5초 후 일회성
    TimerManager::Singleton::GetInstance().AddTimer(
        std::chrono::seconds(5),
        0,
        []()
        {
            std::cout << "[Timer] 5 second one-shot fired!" << std::endl;
        }
    );

    // 타이머 테스트: 등록 후 취소
    auto const cancelId = TimerManager::Singleton::GetInstance().AddTimer(
        std::chrono::seconds(3),
        0,
        []()
        {
            std::cout << "[Timer] This should NOT print (cancelled)" << std::endl;
        }
    );
    TimerManager::Singleton::GetInstance().CancelTimer(cancelId);
    std::cout << "[TestServer] Timer tests registered (repeat=1s, oneshot=5s, cancel=3s)" << std::endl;

    return true;
}

void TestServerApp::Run()
{
    _engine->Run();
}

void TestServerApp::Stop()
{
    _engine->Stop();
}
