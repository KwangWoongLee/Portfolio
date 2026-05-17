#pragma once
#include "CorePch.h"
#include "IOCP.h"
#include "Engine.h"
#include "Connector.h"
#include "ObserverPackets.h"

class ObserverClient final
{
public:
    using Singleton = Singleton<ObserverClient>;

    bool Start(std::string const& ip, uint16_t const port);
    void Stop();

    bool TryGetLatestSnapshot(W2OSnapshot& outSnapshot) const;
    bool IsConnected() const { return _connected.load(std::memory_order_relaxed); }

    void OnSnapshot(W2OSnapshot snapshot);
    void SetConnected(bool const v) { _connected.store(v, std::memory_order_relaxed); }

private:
    std::shared_ptr<IOCP> _iocp;
    std::unique_ptr<Engine> _engine;
    std::unique_ptr<Connector> _connector;
    std::thread _engineThread;

    mutable std::mutex _snapshotMutex;
    W2OSnapshot _latest;
    bool _hasLatest{ false };

    std::atomic<bool> _connected{ false };
};
