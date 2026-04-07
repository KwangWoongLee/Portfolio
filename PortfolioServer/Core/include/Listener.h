#pragma once
#include "IOCPObject.h"
#include "SocketAddress.h"

class IOCPSession;

class Listener final
    : public IIOCPObject
{
public:
    using FuncCreateSession = std::function<std::shared_ptr<IOCPSession>()>;

public:
    Listener(uint16_t const port,
        std::function<bool(HANDLE const)> const& funcRegisterForCompletionPort,
        FuncCreateSession&& funcCreateSession);

    void Start();
    void Dispatch(Overlapped const* iocpEvent, uint32_t numOfBytes = 0) override;

private:
    void PrepareAccepts();
    void AsyncAccept();

private:
    SocketAddress _socketAddress;

    FuncCreateSession  _funcCreateSession;
};
