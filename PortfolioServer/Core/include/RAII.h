#pragma once
#include "CorePch.h"

template <typename T>
class RAII final
{
public:
	explicit RAII(T&& func) noexcept
		: _func(std::forward<T>(func))
	{
	};
	~RAII()
	{
		if (_func.has_value())
		{
			(*_func)();
		}
	};

private:
	std::optional<T> const _func{};
};
