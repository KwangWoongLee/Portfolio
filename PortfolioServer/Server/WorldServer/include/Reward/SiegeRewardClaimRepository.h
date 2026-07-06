#pragma once
#include "Reward/RewardTypes.h"

enum class ESiegeRewardClaimRepositoryError : uint8_t
{
    None,
    InvalidArgument,
    ConnectionUnavailable,
    QueryFailed,
};

struct SiegeRewardClaimRepositoryResult final
{
    ESiegeRewardClaimRepositoryError _error{ ESiegeRewardClaimRepositoryError::None };
    std::string _message;
    size_t _requestedCount{};
    size_t _persistedCount{};

    bool Succeeded() const
    {
        return ESiegeRewardClaimRepositoryError::None == _error;
    }
};

class ISiegeRewardClaimRepository
{
public:
    virtual ~ISiegeRewardClaimRepository() = default;

    virtual SiegeRewardClaimRepositoryResult PrepareClaims(SiegeRewardJob const& job) = 0;
};
