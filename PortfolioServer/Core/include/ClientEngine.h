#pragma once
#include "Listener.h"
#include "Engine.h"

class ClientEngine : public Engine
{
public:
	ClientEngine(std::string_view serverIP, uint16_t serverPort, IOCPRef iocp, uint16_t maxSessionCount, SessionFactory sessionFactory);
	virtual ~ClientEngine() = default;

public:
	virtual bool Init() override;
	virtual void Run(uint32_t timeout = INFINITE) override;
	virtual void Stop() override;

	SocketAddress& GetServerSockAddr() { return mServerSockAddr; };

private:
	//std::string_view mServerIP;
	uint16_t mServerPort;
	SocketAddress mServerSockAddr;
};

