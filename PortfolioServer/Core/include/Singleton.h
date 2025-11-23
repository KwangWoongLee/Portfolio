#pragma once
#include "CorePch.h"

template <typename T>
class Singleton final
{
public:
	Singleton() = default;
	~Singleton() = default;

	static T& GetInstance()
	{
		static T instance;

		return instance;
	}

	static T const& GetConstInstance()
	{
		return GetInstance();
	}
};
