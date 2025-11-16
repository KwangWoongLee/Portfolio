#pragma once
#include "stdafx.h"

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

    void SetHandle(HANDLE const& handle)
    {
		_handle = handle;
    }

    virtual void Dispatch(Overlapped const* iocpEvent, uint32_t const numOfBytes = 0) = 0;

private:
    HANDLE _handle;
};

class IOCP final
{
public:
	explicit IOCP()
	{
		_completionPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

		if (_completionPort == nullptr)
		{
			std::abort();
		}
	}

	bool RegistForCompletionPort(std::shared_ptr<IIOCPObject> const& iocpObject) const;

	void Run(uint32_t timeout = INFINITE) const;
	void Stop() const;

	void IOWorkerFunc(uint32_t const timeout = INFINITE) const;

public:
	static ULONG_PTR constexpr SHUTDOWN_KEY{ static_cast<ULONG_PTR>(-1) };

private:
	HANDLE _completionPort{ INVALID_HANDLE_VALUE };
};