#pragma once
#include "IOCP.h"
#include "IOCPObject.h"
#include "SocketAddress.h"

class Listener
	: public IIOCPObject
{
public:
	explicit Listener(uint16_t const port, std::function<bool(HANDLE const)> const&& funcRegisterForCompletionPort);
	~Listener() override = default;

	void Dispatch(Overlapped const* iocpEvent, uint32_t const numOfBytes = 0) override;

private:
	void PrepareAccepts() const;
	void AsyncAccept() const;

private:
	SocketAddress _socketAddress;
};
