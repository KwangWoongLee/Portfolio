#include "CorePch.h"
#include "ObserverClient.h"
#include "ObserverClientSession.h"
#include "IOCPSessionManager.h"
#include "TaskDispatcher.h"

namespace
{
    auto constexpr NETWORK_IO_THREADS = 1;
}

bool ObserverClient::Start(std::string const& ip, uint16_t const port)
{
    _iocp = std::make_shared<IOCP>();
    _engine = std::make_unique<Engine>(_iocp);

    auto& dispatcher = TaskDispatcher::Singleton::GetInstance();
    dispatcher.AddExecutor(ETaskType::NetworkIO, NETWORK_IO_THREADS);

    _connector = std::make_unique<Connector>(ip, port,
        []() -> std::shared_ptr<IOCPSession>
        {
            return IOCPSessionManager::Singleton::GetInstance().CreateSession<ObserverClientSession>();
        });

    _connector->AsyncConnect();

    _engineThread = std::thread([this]() { _engine->Run(); });

    return true;
}

void ObserverClient::Stop()
{
    if (_engine)
    {
        _engine->Stop();
    }
    if (_engineThread.joinable())
    {
        _engineThread.join();
    }
}

bool ObserverClient::TryGetLatestSnapshot(W2OSnapshot& outSnapshot) const
{
    std::scoped_lock lock(_snapshotMutex);
    if (not _hasLatest)
    {
        return false;
    }
    outSnapshot = _latest;
    return true;
}

void ObserverClient::OnSnapshot(W2OSnapshot snapshot)
{
    std::scoped_lock lock(_snapshotMutex);
    _latest = std::move(snapshot);
    _hasLatest = true;
}
