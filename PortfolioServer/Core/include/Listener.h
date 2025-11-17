#pragma once
#include "IOCP.h"

class Listener
	: public IIOCPObject
{
public:
	explicit Listener(std::shared_ptr<IOCP> const& iocp)
		:_iocp(iocp)
	{
	}

	virtual ~Listener() = default;

	void Dispatch(Overlapped const* iocpEvent, uint32_t const numOfBytes = 0) override;

public:
	bool Init();

private:
	void prepareAccepts();
	void asyncAccept();

private:
	std::shared_ptr<IOCP> const _iocp;
};