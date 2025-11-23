#pragma once

class Overlapped;

class IIOCPObject
    : public std::enable_shared_from_this<IIOCPObject>
{
public:
    IIOCPObject() = default;
    virtual ~IIOCPObject() = default;

    auto GetHandle() const
    {
        return _handle;
    }

    void SetHandle(HANDLE const handle)
    {
        _handle = handle;
    }

    virtual void Dispatch(Overlapped const* iocpEvent, uint32_t const numOfBytes = 0) = 0;

private:
    HANDLE _handle;
};