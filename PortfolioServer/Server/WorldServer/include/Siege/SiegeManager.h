#pragma once
#include "Siege/SiegeWar.h"

class SiegeManager final
{
public:
    SiegeManager() = default;

    SiegeManager(SiegeManager const&) = delete;
    SiegeManager& operator=(SiegeManager const&) = delete;
    SiegeManager(SiegeManager&&) = delete;
    SiegeManager& operator=(SiegeManager&&) = delete;

private:
    friend class GuildManager;

    struct SiegeWarEntry final
    {
        std::shared_ptr<SiegeWar> _siegeWar;
        SiegeWarSnapshot _snapshot;
        std::unordered_set<GuildId> _declaredGuildIds;
        std::unordered_map<GuildId, SiegeDeclarationId> _declarationIdsByGuild;
        int64_t _declarationCostGold{};
        bool _participantsFrozen{};
    };

    struct SiegeDeclarationEntry final
    {
        SiegeWarId _siegeWarId{ INVALID_SIEGE_WAR_ID };
        SiegeDeclarationPayment _payment;
        ESiegeDeclarationState _state{ ESiegeDeclarationState::PendingPayment };
    };

    SiegeWarId RegisterSiegeWar(
        WorldId worldId,
        SiegeWarData data,
        SiegeWar::Clock::time_point scheduledAt,
        GuildId initialDefenderGuildId);

    std::optional<SiegeDeclarationPayment> TryReserveDeclaration(
        SiegeWarType siegeWarType,
        GuildId guildId,
        ActorId requesterActorId);
    ESiegeDeclarationCompletion CompleteDeclaration(
        SiegeDeclarationId declarationId,
        ActorId requesterActorId,
        bool paid);
    void CancelGuildDeclarations(GuildId guildId);
    bool IsGuildParticipationLocked(GuildId guildId) const;
    bool ApplySnapshot(SiegeWarSnapshot snapshot);

    std::shared_ptr<SiegeWar> FindSiegeWar(SiegeWarId siegeWarId) const;
    std::optional<SiegeWarSnapshot> GetSnapshot(SiegeWarId siegeWarId) const;

    void FreezeParticipants(SiegeWarEntry& entry);
    void ReleaseParticipants(SiegeWarEntry& entry);

    std::unordered_map<SiegeWarId, SiegeWarEntry> _siegeWars;
    std::unordered_map<SiegeWarType, SiegeWarId, SiegeWarTypeHash> _currentSiegeWarIds;
    std::unordered_map<SiegeDeclarationId, SiegeDeclarationEntry> _declarations;
    std::unordered_map<GuildId, uint32_t> _guildParticipationLockCounts;
    int64_t _nextSiegeWarId{ 1 };
    int64_t _nextDeclarationId{ 1 };
};
