#include "CorePch.h"
#include "Player.h"
#include "PlayerPost.h"
#include "MemoryTransaction.h"
#include "DbDispatcher.h"
#include "TimerManager.h"
#include "WorldMessages.h"
#include "WorldPost.h"
#include "ZoneManager.h"
#include "ZonePost.h"
#include "WorldPackets.h"

namespace
{
    auto constexpr DEFAULT_ATTACK_DAMAGE = 25;
    auto constexpr RESPAWN_DELAY = std::chrono::seconds(5);
}

class Player::GoldUndoLog final : public IUndoLog
{
public:
    GoldUndoLog(Player& player, int64_t const delta)
        : _player(player)
        , _delta(delta)
        , _previousGold(player._gold)
    {
    }

    void Apply() override
    {
        _player._gold += _delta;
    }

    void Rollback() override
    {
        _player._gold = _previousGold;
    }

    bool Persist() override
    {
        auto const characterId = _player._characterId;
        auto const characterRepository = _player._characterRepository;
        auto const gold = _player._gold;
        if (not characterRepository)
        {
            return false;
        }

        return DbDispatcher::Singleton::GetInstance().Enqueue(
            static_cast<int64_t>(characterId),
            std::make_unique<LambdaDbCommand>(
                [characterRepository, characterId, gold]()
                {
                    auto const result = characterRepository->UpdateGold(characterId, gold);
                    if (result.Succeeded())
                    {
                        return EDbCommandResult::Succeeded;
                    }

                    std::cerr << "[CharacterRepository] UpdateGold failed (character="
                        << characterId << "): " << result._message << std::endl;
                    return EDbCommandResult::Failed;
                },
                [characterId](EDbCommandResult const result)
                {
                    if (result == EDbCommandResult::Succeeded)
                    {
                        return;
                    }

                    (void)characterId;
                    // TODO: record DB error tracking and disconnect/reload the owner session.
                }));
    }

private:
    Player& _player;
    int64_t const _delta;
    int64_t const _previousGold;
};

void Player::OnPacket(uint16_t const packetId, std::vector<uint8_t> const& payload)
{
    std::cout << "[Player:" << GetActorId() << "] OnPacket id=" << packetId
        << " size=" << payload.size() << std::endl;
}

void Player::OnMessage(PlayerMsg::MoveRequest const& msg)
{
    if (IsDead())
    {
        return;
    }

    auto const oldPosition = _position;
    _position = { msg._x, msg._z };

    if (INVALID_ZONE_ID == _currentZoneId)
    {
        return;
    }

    SendToZone(_currentZoneId, ZoneMsg::ActorMoved{ GetActorId(), oldPosition, _position });
}

void Player::OnMessage(PlayerMsg::AttackRequest const& msg)
{
    if (IsDead())
    {
        return;
    }

    SendToPlayer(msg._targetActorId, PlayerMsg::Attacked{ GetActorId(), DEFAULT_ATTACK_DAMAGE });
}

void Player::OnMessage(PlayerMsg::Attacked const& msg)
{
    if (IsDead())
    {
        return;
    }

    _hp -= msg._damage;

    if (INVALID_ZONE_ID != _currentZoneId)
    {
        SendToZone(_currentZoneId, ZoneMsg::HpChanged{ GetActorId(), _position, _hp, INVALID_ACTOR_ID });
    }

    if (IsDead())
    {
        if (INVALID_ZONE_ID != _currentZoneId)
        {
            SendToZone(_currentZoneId, ZoneMsg::ActorDied{ GetActorId(), _position, INVALID_ACTOR_ID });
        }

        auto const selfActorId = GetActorId();
        TimerManager::Singleton::GetInstance().AddTimer(
            std::chrono::duration_cast<std::chrono::milliseconds>(RESPAWN_DELAY),
            static_cast<int64_t>(selfActorId),
            [selfActorId]()
            {
                SendToPlayer(selfActorId, PlayerMsg::Respawn{});
            });
    }
}

void Player::OnMessage(PlayerMsg::Healed const& msg)
{
    if (IsDead())
    {
        return;
    }

    _hp += msg._amount;
    if (MAX_HP < _hp)
    {
        _hp = MAX_HP;
    }

    if (INVALID_ZONE_ID != _currentZoneId)
    {
        SendToZone(_currentZoneId, ZoneMsg::HpChanged{ GetActorId(), _position, _hp, INVALID_ACTOR_ID });
    }
}

void Player::OnMessage(PlayerMsg::Respawn const& msg)
{
    (void)msg;

    _hp = MAX_HP;

    if (INVALID_ZONE_ID != _currentZoneId)
    {
        SendToZone(_currentZoneId, ZoneMsg::HpChanged{ GetActorId(), _position, _hp, INVALID_ACTOR_ID });
    }
}

void Player::OnMessage(PlayerMsg::SiegeDeclarationPaymentRequested const& msg)
{
    if (_paidSiegeDeclarationIds.contains(msg._declarationId))
    {
        SendToWorld(msg._worldId, WorldMsg::SiegeDeclarationPaymentCompleted{
            GetActorId(),
            msg._declarationId,
            msg._costGold,
            true,
        });
        return;
    }

    if (msg._costGold < 0 || _gold < msg._costGold)
    {
        SendToWorld(msg._worldId, WorldMsg::SiegeDeclarationPaymentCompleted{
            GetActorId(),
            msg._declarationId,
            msg._costGold,
            false,
        });
        return;
    }

    MemoryTransaction transaction(static_cast<int64_t>(_characterId));
    transaction.Add<GoldUndoLog>(*this, -msg._costGold);
    if (not transaction.Commit())
    {
        SendToWorld(msg._worldId, WorldMsg::SiegeDeclarationPaymentCompleted{
            GetActorId(),
            msg._declarationId,
            msg._costGold,
            false,
        });
        return;
    }

    _paidSiegeDeclarationIds.insert(msg._declarationId);

    SendToWorld(msg._worldId, WorldMsg::SiegeDeclarationPaymentCompleted{
        GetActorId(),
        msg._declarationId,
        msg._costGold,
        true,
    });
}

void Player::OnMessage(PlayerMsg::SiegeDeclarationRefundRequested const& msg)
{
    if (not _paidSiegeDeclarationIds.contains(msg._declarationId))
    {
        return;
    }

    MemoryTransaction transaction(static_cast<int64_t>(_characterId));
    transaction.Add<GoldUndoLog>(*this, msg._costGold);
    if (not transaction.Commit())
    {
        return;
    }

    _paidSiegeDeclarationIds.erase(msg._declarationId);
}
