#pragma once
#include "IOCP.h"

class Engine
{
	class IIOCPObject;

public:
	explicit Engine(std::shared_ptr<IOCP> const& iocp);
	virtual ~Engine() = default;

	virtual void Run(uint32_t const timeout = INFINITE);
	virtual void Stop();

protected:
	////USE_LOCK;
	std::shared_ptr<IOCP> const _iocp;
};