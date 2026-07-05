#pragma once
#include "Guild/GuildTypes.h"
#include "Siege/SiegeTypes.h"

using RewardClaimId = StrongId<struct RewardClaimIdTag, int64_t>;

inline RewardClaimId constexpr INVALID_REWARD_CLAIM_ID{ 0 };

enum class ERewardType : uint8_t
{
    SiegeWinnerGold,
};

enum class ERewardClaimState : uint8_t
{
    ReadyToClaim,
    Claimed,
    Failed,
};

enum class ESiegeRewardJobState : uint8_t
{
    ClaimsPrepared,
    SkippedNoWinner,
};

struct RewardClaim final
{
    RewardClaimId _claimId{ INVALID_REWARD_CLAIM_ID };
    SiegeWarId _eventId{ INVALID_SIEGE_WAR_ID };
    ActorId _actorId{ INVALID_ACTOR_ID };
    GuildId _guildId{ INVALID_GUILD_ID };
    ERewardType _rewardType{ ERewardType::SiegeWinnerGold };
    ERewardClaimState _state{ ERewardClaimState::ReadyToClaim };
    int64_t _goldAmount{};
};

struct SiegeRewardJob final
{
    SiegeWarId _siegeWarId{ INVALID_SIEGE_WAR_ID };
    WorldId _worldId{ INVALID_WORLD_ID };
    GuildId _winnerGuildId{ INVALID_GUILD_ID };
    ESiegeWarEndReason _endReason{ ESiegeWarEndReason::None };
    ESiegeRewardJobState _state{ ESiegeRewardJobState::SkippedNoWinner };
    std::vector<RewardClaim> _claims;
};

char const* ToString(ERewardType rewardType);
char const* ToString(ERewardClaimState state);
char const* ToString(ESiegeRewardJobState state);
