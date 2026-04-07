#pragma once
#include "App.h"
#include "ServerEngine.h"

class TestServerApp final
    : public IApp
{
public:
    bool Init() override;
    void Run() override;
    void Stop() override;

private:
    std::shared_ptr<IOCP> _iocp;
    std::unique_ptr<ServerEngine> _engine;
};
