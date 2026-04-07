#pragma once
#include "Engine.h"

// TODO: Config
constexpr uint16_t MAX_SESSION_COUNT = 1500;

enum class ELinkType : uint8_t
{
	UniverseServer,
	WorldServer,
	Client,
	Max
};

class IOCPSession;
class Listener;
class Connector;

class ServerEngine final
	: public Engine
{
public:
	explicit ServerEngine(std::shared_ptr<IOCP> const& iocp);
	~ServerEngine() override = default;

	bool AddListener(uint16_t const port, ELinkType const linkType, std::function<std::shared_ptr<IOCPSession>()>&& funcCreateSession);

	void Run(uint32_t timeout = INFINITE) override;
	void Stop() override;
	
private:
	std::unordered_map<ELinkType, std::shared_ptr<Listener>> _listeners;
	std::unordered_map<ELinkType, std::shared_ptr<Connector>> _connectors;
};

