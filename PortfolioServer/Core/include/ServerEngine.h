#pragma once
#include "Listener.h"
#include "Engine.h"

class ServerEngine : public Engine
{
public:
	ServerEngine(std::string_view ip, uint16_t port, std::shared_ptr<IOCP> iocp, uint16_t maxSessionCount, SessionFactory sessionFactory);
	virtual ~ServerEngine() = default;

public:
	virtual bool Init() override;
	virtual void Run(uint32_t timeout = INFINITE) override;
	virtual void Stop() override;

	SocketAddress& GetSockAddress() { return mSockAddr; };
	
private:
	std::string mIP;
	uint16_t mPort;
	ListenerRef mListener = nullptr;
	SocketAddress mSockAddr = {};
};

