#include <gtest/gtest.h>
#include "MemoryTransaction.h"

namespace
{
    class TrackingUndoLog final : public IUndoLog
    {
    public:
        TrackingUndoLog(
            int& value,
            int const delta,
            bool const persistResult,
            std::vector<int>& rollbackOrder,
            int const rollbackMarker,
            int64_t& persistedOwnerId)
            : _value(value)
            , _delta(delta)
            , _persistResult(persistResult)
            , _rollbackOrder(rollbackOrder)
            , _rollbackMarker(rollbackMarker)
            , _persistedOwnerId(persistedOwnerId)
            , _previousValue(value)
        {
        }

        void Apply() override
        {
            _value += _delta;
        }

        void Rollback() override
        {
            _value = _previousValue;
            _rollbackOrder.emplace_back(_rollbackMarker);
        }

        bool Persist() override
        {
            _persistedOwnerId = GetOwnerId();
            return _persistResult;
        }

    private:
        int& _value;
        int const _delta;
        bool const _persistResult;
        std::vector<int>& _rollbackOrder;
        int const _rollbackMarker;
        int64_t& _persistedOwnerId;
        int const _previousValue;
    };
}

TEST(MemoryTransactionTests, CommitKeepsAppliedValueAndPassesOwnerId)
{
    int value{ 100 };
    std::vector<int> rollbackOrder;
    int64_t persistedOwnerId{ 0 };

    MemoryTransaction transaction(77);
    transaction.Add<TrackingUndoLog>(
        value,
        -25,
        true,
        rollbackOrder,
        1,
        persistedOwnerId);

    EXPECT_TRUE(transaction.Commit());
    EXPECT_EQ(75, value);
    EXPECT_EQ(77, persistedOwnerId);
    EXPECT_TRUE(rollbackOrder.empty());
}

TEST(MemoryTransactionTests, PersistFailureRollsBackInReverseOrder)
{
    int value{ 100 };
    std::vector<int> rollbackOrder;
    int64_t firstOwnerId{ 0 };
    int64_t secondOwnerId{ 0 };

    MemoryTransaction transaction(88);
    transaction.Add<TrackingUndoLog>(value, -10, true, rollbackOrder, 1, firstOwnerId);
    transaction.Add<TrackingUndoLog>(value, -20, false, rollbackOrder, 2, secondOwnerId);

    EXPECT_FALSE(transaction.Commit());
    EXPECT_EQ(100, value);
    EXPECT_EQ((std::vector<int>{ 2, 1 }), rollbackOrder);
    EXPECT_EQ(88, firstOwnerId);
    EXPECT_EQ(88, secondOwnerId);
}

TEST(MemoryTransactionTests, MarkFailedRollsBackWithoutPersisting)
{
    int value{ 100 };
    std::vector<int> rollbackOrder;
    int64_t persistedOwnerId{ 0 };

    MemoryTransaction transaction(99);
    transaction.Add<TrackingUndoLog>(
        value,
        -30,
        true,
        rollbackOrder,
        1,
        persistedOwnerId);
    transaction.MarkFailed();

    EXPECT_FALSE(transaction.Commit());
    EXPECT_EQ(100, value);
    EXPECT_EQ(0, persistedOwnerId);
    EXPECT_EQ((std::vector<int>{ 1 }), rollbackOrder);
}
