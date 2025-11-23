#pragma once
#include "IOCPSession.h"

class IOCP;

class IOCPSessionManager
	: public BaseSessionManager
{
public:
    using Singleton = Singleton<IOCPSessionManager>;

public:
    void Init(std::shared_ptr<IOCP> const& iocp);

    std::shared_ptr<IOCPSession> CreateSession();
    void ReleaseSession(SessionId const sessionId);

private:
    std::shared_ptr<IOCP> _iocp;
};

