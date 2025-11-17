#pragma once
#include "stdafx.h"

template <typename T>
class Singleton final
{
public:
	Singleton() = default;
	~Singleton() = default;

	static T& Instance()
	{
		static T instance;

		return instance;
	}

	static T const& ConstInstance()
	{
		return Instance();
	}
};
