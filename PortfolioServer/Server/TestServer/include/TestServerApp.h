#pragma once
#include "App.h"

class TestServerApp :
    public IApp
{
	bool Init() override;
    void Run() override;
    void Stop() override;
};

