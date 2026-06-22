#pragma once
#include "Actor.h"
#include "Siege/SiegeTypes.h"

// Player가 처리하는 메시지 타입 모음.
// 액터 간 통신이나 패킷 → 액터 디스패치 시 이 타입들을 사용한다.
// 새 메시지 추가 시 Player::OnMessage 오버로드도 함께 추가할 것.
namespace PlayerMsg
{
    struct MoveRequest
    {
        float _x{};
        float _z{};
    };

    struct AttackRequest
    {
        ActorId _targetActorId;
    };

    struct Attacked
    {
        ActorId _attackerId;
        int32_t _damage{};
    };

    struct Healed
    {
        ActorId _healerId;
        int32_t _amount{};
    };

    struct Respawn
    {
    };

    struct SiegeDeclarationPaymentRequested final
    {
        WorldId _worldId{ INVALID_WORLD_ID };
        SiegeDeclarationId _declarationId{ INVALID_SIEGE_DECLARATION_ID };
        int64_t _costGold{};
    };

    struct SiegeDeclarationRefundRequested final
    {
        SiegeDeclarationId _declarationId{ INVALID_SIEGE_DECLARATION_ID };
        int64_t _costGold{};
    };
}
