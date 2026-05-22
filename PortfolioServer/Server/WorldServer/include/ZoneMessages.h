#pragma once
#include "Actor.h"
#include "Packet.h"
#include "WorldTypes.h"

class Player;

// IZone가 처리하는 메시지 타입 모음.
// Player → Zone 비동기 메시지로 변환할 때 사용 (zone 액터화).
// 새 메시지 추가 시 IZone::OnMessage 오버로드도 함께 추가할 것.
namespace ZoneMsg
{
    struct ActorMoved
    {
        ActorId _actorId;
        Position _oldPosition;
        Position _newPosition;
    };

    struct PlayerEntered
    {
        std::shared_ptr<Player> _player;
    };

    struct PlayerLeft
    {
        ActorId _actorId;
    };

    // Player worker나 다른 thread에서 broadcast 요청 시 zone worker에 위임.
    // _packet은 polymorphic이라 shared_ptr로 전달.
    struct BroadcastInSightRequest
    {
        std::shared_ptr<Packet const> _packet;
        Position _center;
        ActorId _excludeActorId;
    };
}
