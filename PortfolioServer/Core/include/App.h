#pragma once
#include "stdafx.h"

class IApp
{
public:
    virtual ~IApp() = default;

    virtual bool Init() = 0;
    virtual void Run() = 0;
    virtual void Stop() = 0;
};