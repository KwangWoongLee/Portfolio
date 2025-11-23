#include "CorePch.h"
#include "Engine.h"

#include "SocketUtil.h"
#include "IOCPSessionManager.h"


Engine::Engine(std::shared_ptr<IOCP> const& iocp)
	:_iocp(iocp)
{
	if (not SocketUtil::Singleton::GetInstance().Init())
	{
		assert(false);
	}

	IOCPSessionManager::Singleton::GetInstance().Init(_iocp);
}

void Engine::Run(uint32_t const timeout)
{
	_iocp->Run(timeout);
}

void Engine::Stop()
{
	_iocp->Stop();
}