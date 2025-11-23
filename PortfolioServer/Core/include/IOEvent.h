#pragma once
#include "CorePch.h"

enum class EIOType : uint8_t
{
    None,
    Send,
    Recv,
    Accept,
    Connect,
    Disconnect
};

class IIOCPObject;

class Overlapped
    : public OVERLAPPED
{
public:
    void Init()
    {
        ZeroMemory(this, sizeof(OVERLAPPED));
        _ioType = EIOType::None;
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

    void SetIOCPObject(std::shared_ptr<IIOCPObject> const& iocpObj)
    {
        _iocpObj = iocpObj;
    }

    [[nodiscard]]
    static Overlapped* GetObjectPoolIOEvent(EIOType const ioType, std::shared_ptr<IIOCPObject> const& iocpObject)
    {
        auto const ioEvent = ObjectPool<Overlapped>::Singleton::GetInstance().Acquire();

        ioEvent->Init();
        ioEvent->SetIOType(ioType);
        ioEvent->SetIOCPObject(iocpObject);

        return ioEvent;
    }

private:
    EIOType _ioType{};
    std::shared_ptr<IIOCPObject> _iocpObj;
};