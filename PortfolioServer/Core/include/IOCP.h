#pragma once
#include "CorePch.h"

class IOCP final
{
public:
	explicit IOCP();
	~IOCP();

	bool RegisterForCompletionPort(HANDLE const handle) const;

	void Run(uint32_t timeout = INFINITE) const;
	void RunWorkerPool(uint32_t workerCount, uint32_t timeout = INFINITE) const;
	void Stop() const;

	void IOWorkerFunc(uint32_t const timeout = INFINITE) const;

public:
	static ULONG_PTR constexpr SHUTDOWN_KEY{ static_cast<ULONG_PTR>(-1) };

private:
	HANDLE _completionPort{ INVALID_HANDLE_VALUE };
	mutable std::atomic<uint32_t> _runningWorkerCount{};
};
