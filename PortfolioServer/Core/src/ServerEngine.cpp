#include "CorePch.h"
#include "ServerEngine.h"
#include "Listener.h"
#include "IOCP.h"

ServerEngine::ServerEngine(uint16_t const port, std::shared_ptr<IOCP> const& iocp)
	: Engine(iocp)
{
	_listener = std::make_shared<Listener>(port, 
		[this](HANDLE const handle)
		{
			return _iocp->RegisterForCompletionPort(handle);
		});
}

void ServerEngine::Run(uint32_t const timeout)
{
	Engine::Run(timeout);
}

void ServerEngine::Stop()
{
	_listener = nullptr;

	Engine::Stop();
}
