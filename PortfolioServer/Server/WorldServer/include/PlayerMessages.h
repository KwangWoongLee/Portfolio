#pragma once
#include "Actor.h"

// Player가 처리하는 메시지 타입 모음.
// 액터 간 통신이나 패킷 → 액터 디스패치 시 이 타입들을 사용한다.
// 새 메시지 추가 시 Player::OnMessage 오버로드도 함께 추가할 것.
namespace PlayerMsg
{
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
}
