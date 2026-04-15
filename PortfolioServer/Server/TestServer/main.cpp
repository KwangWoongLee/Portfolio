#include "CorePch.h"
#include "TestServerApp.h"
#include "MemoryTransaction.h"

namespace
{
    struct Player
    {
        int64_t _gold{};
        int32_t _potion{};
    };

    class GoldUndoLog final : public IUndoLog
    {
    public:
        GoldUndoLog(Player& player, int64_t const delta)
            : _player(player), _delta(delta), _prev(player._gold) {}

        void Apply() override
        {
            _player._gold += _delta;
            std::cout << "  [Gold] Apply: " << _prev << " -> " << _player._gold << std::endl;
        }
        void Rollback() override
        {
            _player._gold = _prev;
            std::cout << "  [Gold] Rollback -> " << _player._gold << std::endl;
        }
        void Persist() override
        {
            // FireAndForget: ownerId를 value 캡처, 실패 콜백에서 세션 lookup 후 disconnect
            auto const ownerId = GetOwnerId();
            auto const gold = _player._gold;
            std::cout << "  [Gold] Persist(DB) owner=" << ownerId << " gold=" << gold << std::endl;
            // 실제 구현 예:
            //   Db::Fire(ownerId, "UPDATE player SET gold=? WHERE id=?", gold, ownerId,
            //       [ownerId](bool ok) {
            //           if (not ok) {
            //               LOG_ERROR("DB fail: disconnect " + std::to_string(ownerId));
            //               if (auto s = SessionMgr::Find(ownerId)) s->Disconnect();
            //           }
            //       });
        }

    private:
        Player& _player;
        int64_t const _delta;
        int64_t const _prev;
    };

    class PotionUndoLog final : public IUndoLog
    {
    public:
        PotionUndoLog(Player& player, int32_t const delta)
            : _player(player), _delta(delta), _prev(player._potion) {}

        void Apply() override
        {
            _player._potion += _delta;
            std::cout << "  [Potion] Apply: " << _prev << " -> " << _player._potion << std::endl;
        }
        void Rollback() override
        {
            _player._potion = _prev;
            std::cout << "  [Potion] Rollback -> " << _player._potion << std::endl;
        }
        void Persist() override
        {
            auto const ownerId = GetOwnerId();
            auto const potion = _player._potion;
            std::cout << "  [Potion] Persist(DB) owner=" << ownerId << " potion=" << potion << std::endl;
        }

    private:
        Player& _player;
        int32_t const _delta;
        int32_t const _prev;
    };

    // 포션 구매 시나리오: 골드 차감 + 포션 증가를 원자적으로 처리
    bool TryBuyPotion(int64_t const ownerId, Player& player, int64_t const price, int32_t const amount)
    {
        MemoryTransaction tx(ownerId);

        if (player._gold < price)
        {
            std::cout << "  pre-check fail: gold shortage" << std::endl;
            tx.MarkFailed();
            return false;
        }
        tx.Add<GoldUndoLog>(player, -price);

        int32_t constexpr MAX_POTION = 10;
        if (MAX_POTION < player._potion + amount)
        {
            std::cout << "  post-check fail: potion over max, rollback begins" << std::endl;
            tx.MarkFailed();
            return false;
        }
        tx.Add<PotionUndoLog>(player, amount);

        return true;
    }

    void RunMemoryTransactionDemo()
    {
        std::cout << "===== MemoryTransaction Demo =====" << std::endl;

        int64_t constexpr PLAYER_ID = 12345;
        Player player{ ._gold = 1000, ._potion = 8 };
        std::cout << "[Initial] ownerId=" << PLAYER_ID << " gold=" << player._gold << ", potion=" << player._potion << "\n" << std::endl;

        std::cout << "-- Case 1: 성공 (100골드로 포션 1개) --" << std::endl;
        auto const ok = TryBuyPotion(PLAYER_ID, player, 100, 1);
        std::cout << "Result=" << ok << " / gold=" << player._gold << ", potion=" << player._potion << "\n" << std::endl;

        std::cout << "-- Case 2: 중간 실패 (gold Apply 후 potion MAX 초과) --" << std::endl;
        auto const fail = TryBuyPotion(PLAYER_ID, player, 100, 5);
        std::cout << "Result=" << fail << " / gold=" << player._gold << ", potion=" << player._potion << "\n" << std::endl;

        std::cout << "-- Case 3: 사전 실패 (골드 부족) --" << std::endl;
        auto const fail2 = TryBuyPotion(PLAYER_ID, player, 99999, 1);
        std::cout << "Result=" << fail2 << " / gold=" << player._gold << ", potion=" << player._potion << "\n" << std::endl;

        std::cout << "===== Demo End =====\n" << std::endl;
    }
}

int main()
{
    RunMemoryTransactionDemo();

    TestServerApp app;

    if (not app.Init())
    {
        std::cout << "[Main] Init failed" << std::endl;
        return 1;
    }

    std::cout << "[Main] Server starting..." << std::endl;

    app.Run();

    return 0;
}
