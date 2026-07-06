#pragma once
#include "Reward/SiegeRewardClaimRepository.h"

class MySqlConnectionPool;

class MySqlSiegeRewardClaimRepository final
    : public ISiegeRewardClaimRepository
{
public:
    explicit MySqlSiegeRewardClaimRepository(
        std::shared_ptr<MySqlConnectionPool> connectionPool);

    SiegeRewardClaimRepositoryResult PrepareClaims(SiegeRewardJob const& job) override;

private:
    std::shared_ptr<MySqlConnectionPool> _connectionPool;
};
