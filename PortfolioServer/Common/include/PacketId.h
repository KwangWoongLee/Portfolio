#pragma once
#include "CorePch.h"

enum class EPacketId : uint16_t
{
    None = 0,

    // Client -> World (0x1xxx)
    C2WMove          = 0x1001,
    C2WAttack        = 0x1002,

    // World -> Client (0x2xxx)
    W2CWelcome       = 0x2000,
    W2CMoveBroadcast = 0x2001,
    W2CHpUpdate      = 0x2002,
    W2CDeath         = 0x2003,
    W2CActorEnter    = 0x2004,
    W2CActorLeave    = 0x2005,

    // World -> Observer (0x3xxx)
    W2OSnapshot      = 0x3001,
};
