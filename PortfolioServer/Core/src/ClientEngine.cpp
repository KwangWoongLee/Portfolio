#include "stdafx.h"
#include "ClientEngine.h"

ClientEngine::ClientEngine(std::string_view serverIP, uint16_t serverPort, IOCPRef iocp, uint16_t maxSessionCount, SessionFactory sessionFactory)
    : Engine(iocp, maxSessionCount, sessionFactory),
	mServerSockAddr(SocketAddress(serverIP, serverPort)),
	//mServerIP(serverIP),
	mServerPort(serverPort)
{
}

bool ClientEngine::Init()
{
	if (Engine::Init() == false)
		return false;

	const int32_t sessionCount = GetMaxSessionCount();
	for (int32_t i = 0; i < sessionCount; i++)
	{
		SessionRef session = CreateSession();
		if (session->Connect() == false)
			return false;
	}

	return true;
}

void ClientEngine::Run(uint32_t timeout)
{
	Engine::Run(timeout);
}

void ClientEngine::Stop()
{
}
