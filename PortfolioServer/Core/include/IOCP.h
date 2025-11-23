#pragma once
#include "CorePch.h"

class IOCP final
{
public:
	explicit IOCP()
	{
		_completionPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

		if (not _completionPort)
		{
			assert(false);
		}
	}

	bool RegisterForCompletionPort(HANDLE const handle) const;

	void Run(uint32_t timeout = INFINITE) const;
	void Stop() const;

	void IOWorkerFunc(uint32_t const timeout = INFINITE) const;

public:
	static ULONG_PTR constexpr SHUTDOWN_KEY{ static_cast<ULONG_PTR>(-1) };

private:
	HANDLE _completionPort{ INVALID_HANDLE_VALUE };
};