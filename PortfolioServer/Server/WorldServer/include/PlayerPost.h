#pragma once
#include "PlayerTaskRunner.h"

template <typename T_FUNC>
void PostToPlayer(ActorId const actorId, T_FUNC&& func) = delete;

// 타입 메시지 기반: 안정적 프로토콜용 (Player::OnMessage 오버로드 자동 선택)
template <typename T_MSG>
void SendToPlayer(ActorId const actorId, T_MSG msg)
{
    PlayerTaskRunner::PostMessage(actorId, std::move(msg));
}
