#define NOGDI
#define NOUSER
#define NOMMSYSTEM
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "raylib.h"

#include "CorePch.h"
#include "ObserverClient.h"

namespace
{
    auto constexpr WINDOW_SIZE = 800;
    auto constexpr WORLD_HALF_EXTENT = 1000.0f;
    auto constexpr CELL_SIZE = 100.0f;

    int MapToScreen(float const world)
    {
        return static_cast<int>((world + WORLD_HALF_EXTENT) / (2.0f * WORLD_HALF_EXTENT) * WINDOW_SIZE);
    }
}

int main()
{
    auto& client = ObserverClient::Singleton::GetInstance();
    if (not client.Start("127.0.0.1", 9001))
    {
        return 1;
    }

    InitWindow(WINDOW_SIZE, WINDOW_SIZE, "Portfolio Viewer");
    SetTargetFPS(60);

    std::unordered_map<ActorId, int32_t> prevHpMap;
    std::unordered_map<ActorId, int> damageFramesMap;
    auto constexpr DAMAGE_EFFECT_FRAMES = 30;

    while (not WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

        auto const gridColor = Color{ 30, 30, 30, 255 };
        for (int g = -10; g <= 10; ++g)
        {
            auto const sx = MapToScreen(static_cast<float>(g) * CELL_SIZE);
            auto const sz = MapToScreen(static_cast<float>(g) * CELL_SIZE);
            DrawLine(sx, 0, sx, WINDOW_SIZE, gridColor);
            DrawLine(0, sz, WINDOW_SIZE, sz, gridColor);
        }

        W2OSnapshot snap;
        auto const hasSnap = client.TryGetLatestSnapshot(snap);

        if (hasSnap)
        {
            std::unordered_map<ActorId, int> currDamageFrames;

            for (auto const& a : snap._actors)
            {
                int damageFrames{ 0 };

                auto const dfIter = damageFramesMap.find(a._actorId);
                if (damageFramesMap.end() != dfIter && 0 < dfIter->second)
                {
                    damageFrames = dfIter->second - 1;
                }

                auto const hpIter = prevHpMap.find(a._actorId);
                if (prevHpMap.end() != hpIter && a._hp < hpIter->second)
                {
                    damageFrames = DAMAGE_EFFECT_FRAMES;
                }
                prevHpMap[a._actorId] = a._hp;

                if (0 < damageFrames)
                {
                    currDamageFrames[a._actorId] = damageFrames;
                }

                Color color;
                if (a._hp <= 0)
                {
                    color = RED;
                }
                else if (0 < damageFrames)
                {
                    color = YELLOW;
                }
                else
                {
                    color = WHITE;
                }

                auto const sx = MapToScreen(a._x);
                auto const sz = MapToScreen(a._z);
                DrawCircle(sx, sz, 2.0f, color);
            }

            damageFramesMap = std::move(currDamageFrames);

            DrawText(TextFormat("CCU: %u", snap._ccu), 10, 10, 20, WHITE);
            DrawText(TextFormat("Send: %u pps", snap._sendPps), 10, 35, 20, WHITE);
            DrawText(TextFormat("Recv: %u pps", snap._recvPps), 10, 60, 20, WHITE);
            DrawText(TextFormat("Queue: %u", snap._queueLen), 10, 85, 20, WHITE);
            DrawText(TextFormat("Actors: %d", static_cast<int>(snap._actors.size())), 10, 110, 20, WHITE);
        }
        else if (client.IsConnected())
        {
            DrawText("Waiting for first snapshot...", 10, 10, 20, GRAY);
        }
        else
        {
            DrawText("Connecting to 127.0.0.1:9001...", 10, 10, 20, GRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    client.Stop();
    return 0;
}
