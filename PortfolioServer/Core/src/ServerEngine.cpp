#include "CorePch.h"
#include "ServerEngine.h"
#include "Listener.h"
#include "IOCP.h"
#include "IOCPSessionManager.h"
#include "TaskDispatcher.h"

ServerEngine::ServerEngine(std::shared_ptr<IOCP> const& iocp)
	: Engine(iocp)
{
}

bool ServerEngine::AddListener(uint16_t const port, ELinkType const linkType, std::function<std::shared_ptr<IOCPSession>()>&& funcCreateSession)
{
	if (_listeners.contains(linkType))
	{
		return false;
	}

	_listeners.insert_or_assign(linkType
		, std::make_shared<Listener>(port , 
			[this](HANDLE const handle)
			{
				return _iocp->RegisterForCompletionPort(handle);
			}
			, std::move(funcCreateSession)));

	return true;
}

void ServerEngine::Run(uint32_t const timeout)
{
	auto& dispatcher = TaskDispatcher::Singleton::GetInstance();
	dispatcher.AddExecutor(ETaskType::Basic, 4); // TODO:: config
	dispatcher.AddExecutor(ETaskType::DB, 2); // TODO:: config

	Engine::Run(timeout);
}

void ServerEngine::Stop()
{
    _listeners.clear();

	Engine::Stop();
}
