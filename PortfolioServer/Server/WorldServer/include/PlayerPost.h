#pragma once
#include "ActorPost.h"
#include "PlayerManager.h"

// 람다 기반: ad-hoc 작업, 콜백 체인용
template <typename T_FUNC>
void PostToPlayer(ActorId const actorId, T_FUNC&& func)
{
    Post(actorId,
        [actorId, func = std::forward<T_FUNC>(func)]
        {
            if (auto const player = PlayerManager::Singleton::GetInstance().Find(actorId))
            {
                func(*player);
            }
        });
}

// 타입 메시지 기반: 안정적 프로토콜용 (Player::OnMessage 오버로드 자동 선택)
template <typename T_MSG>
void SendToPlayer(ActorId const actorId, T_MSG msg)
{
    Post(actorId,
        [actorId, msg = std::move(msg)]
        {
            if (auto const player = PlayerManager::Singleton::GetInstance().Find(actorId))
            {
                player->OnMessage(msg);
            }
        });
}
