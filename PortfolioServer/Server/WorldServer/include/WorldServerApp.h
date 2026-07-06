#pragma once
#include "App.h"
#include "ServerEngine.h"

class MySqlConnectionPool;
class ICharacterRepository;
class ISiegeRewardClaimRepository;

class WorldServerApp final
    : public IApp
{
public:
    WorldServerApp();
    ~WorldServerApp() override;

    bool Init() override;
    void Run() override;
    void Stop() override;

private:
    void InitZones();
    void StartSiegeDemoIfEnabled();

    std::shared_ptr<IOCP> _iocp;
    std::unique_ptr<ServerEngine> _engine;
    std::shared_ptr<MySqlConnectionPool> _dbConnectionPool;
    std::shared_ptr<ICharacterRepository> _characterRepository;
    std::shared_ptr<ISiegeRewardClaimRepository> _siegeRewardClaimRepository;
};
