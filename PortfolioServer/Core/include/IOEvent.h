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
    void Init(std::function<void(Overlapped*)>&& funcRelease = nullptr)
    {
        ZeroMemory(this, sizeof(OVERLAPPED));

        _funcRelease = std::move(funcRelease);
    	_ioType = EIOType::None;
        _iocpObj = nullptr;
    }

	[[nodiscard]]
    std::function<void(Overlapped*)> const& GetReleaseFunction() const
    {
        return _funcRelease;
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
    std::function<void(Overlapped*)> _funcRelease;
    EIOType _ioType{};
    std::shared_ptr<IIOCPObject> _iocpObj;
};