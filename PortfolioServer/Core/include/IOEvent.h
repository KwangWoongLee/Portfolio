#pragma once
#include "stdafx.h"

enum class EIOType : uint8_t
{
    NONE,
    SEND,
    RECV,
    ACCEPT,
    CONNECT,
    DISCONNECT
};

class IIOCPObject;
class IOCPSession;

class Overlapped
    : public OVERLAPPED
{
public:
    void Init()
    {
        ZeroMemory(this, sizeof(OVERLAPPED));
        _ioType = EIOType::NONE;
        _iocpObj = nullptr;
    }

    [[nodiscard]]
    EIOType GetIOType() const
    {
        return _ioType;
    }

    void SetIOType(EIOType const ioType)
    {
        _ioType = ioType;
    }

    [[nodiscard]]
    std::shared_ptr<IIOCPObject> GetIOCPObject() const
    {
        return _iocpObj;
    }

    void SetIOCPObject(std::shared_ptr<IIOCPObject> const iocpObj)
    {
        _iocpObj = iocpObj;
    }

    [[nodiscard]]
    static Overlapped* GetObjectPoolIOEvent(EIOType const ioType, std::shared_ptr<IIOCPObject> const& iocpObject)
    {
        auto const ioEvent = ObjectPool<Overlapped>::Singleton::Instance().Acquire();

        ioEvent->Init();
        ioEvent->SetIOType(ioType);
        ioEvent->SetIOCPObject(iocpObject);

        return ioEvent;
    }

private:
    EIOType _ioType{};
    std::shared_ptr<IIOCPObject> _iocpObj;
};

class OverlappedAccept final
	: public Overlapped
{
public:
    OverlappedAccept()
    {
        ZeroMemory(_acceptBuf, sizeof(_acceptBuf));
    }

    char* GetBuffer()
    {
        return _acceptBuf;
    }

private:
    char _acceptBuf[sizeof(SOCKADDR_IN) * 2 + 32]{};
};