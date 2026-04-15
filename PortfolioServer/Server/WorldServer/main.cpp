#include "CorePch.h"
#include "WorldServerApp.h"

int main()
{
    WorldServerApp app;

    if (not app.Init())
    {
        std::cout << "[Main] Init failed" << std::endl;
        return 1;
    }

    std::cout << "[Main] WorldServer starting..." << std::endl;

    app.Run();

    return 0;
}
