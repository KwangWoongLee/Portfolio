#include "CorePch.h"
#include "ClientEngine.h"

#include "IOCPSession.h"

ClientEngine::ClientEngine(std::string const& serverIp, uint16_t const serverPort, std::shared_ptr<IOCP> const& iocp, uint16_t const maxSessionCount, SessionFactory const& sessionFactory)
    : Engine(iocp, maxSessionCount, sessionFactory),
	_serverPort(serverPort),
	//mServerIP(serverIP),
	_serverSockAddr(SocketAddress(serverIp, serverPort))
{
}

bool ClientEngine::Init()
{
	if (not Engine::Init())
	{
		return false;
	}

	int32_t const sessionCount = GetMaxSessionCount();
	for (int32_t i{}; i < sessionCount; i++)
	{
		if (auto const session = CreateSession(); 
			not session->Connect())
			return false;
	}

	return true;
}

void ClientEngine::Run(uint32_t const timeout)
{
	Engine::Run(timeout);
}

void ClientEngine::Stop()
{
}
