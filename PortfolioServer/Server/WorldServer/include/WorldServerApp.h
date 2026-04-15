#pragma once
#include "App.h"
#include "ServerEngine.h"

class WorldServerApp final
    : public IApp
{
public:
    bool Init() override;
    void Run() override;
    void Stop() override;

private:
    void InitZones();

    std::shared_ptr<IOCP> _iocp;
    std::unique_ptr<ServerEngine> _engine;
};
