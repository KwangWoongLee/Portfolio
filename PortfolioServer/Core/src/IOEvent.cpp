#include "CorePch.h"
#include "IOEvent.h"

void Overlapped::Init(std::function<void(Overlapped*)>&& funcRelease)
{
    ZeroMemory(this, sizeof(OVERLAPPED));

    _funcRelease = std::move(funcRelease);
    _ioType = EIOType::None;
    _iocpObj = nullptr;
}

Overlapped* Overlapped::GetObjectPoolIOEvent(EIOType const ioType, std::shared_ptr<IIOCPObject> const& iocpObject)
{
    auto const ioEvent = ObjectPool<Overlapped>::Singleton::GetInstance().Acquire();

    ioEvent->Init();
    ioEvent->SetIOType(ioType);
    ioEvent->SetIOCPObject(iocpObject);

    return ioEvent;
}
