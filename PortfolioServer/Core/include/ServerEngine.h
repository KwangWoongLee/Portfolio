#pragma once
#include "Engine.h"

// TODO: Config
constexpr uint16_t MAX_SESSION_COUNT = 1500;

class Listener;
class Connector;

class ServerEngine final
	: public Engine
{
public:
	ServerEngine(uint16_t const port, std::shared_ptr<IOCP> const& iocp);
	~ServerEngine() override = default;

	void Run(uint32_t timeout = INFINITE) override;
	void Stop() override;
	
private:
	std::shared_ptr<Listener> _listener;
	std::shared_ptr<Connector> _connector;
};

