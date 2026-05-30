#pragma once
#include "CorePch.h"
#include "Logger.h"
#include "Stream.h"

class DisconnectLogger final
    : public Logger
{
public:
    using Singleton = Singleton<DisconnectLogger>;

    bool Start(std::string const& filePath);

    void Log(SessionId const sessionId, EDisconnectReason const reason);
};
