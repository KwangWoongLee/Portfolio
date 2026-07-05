#include <gtest/gtest.h>
#include "DbCompletionTarget.h"

TEST(DbCompletionTargetTests, NetworkSessionTargetCarriesSessionId)
{
    auto const target = DbCompletionTarget::NetworkSession(SessionId{ 42 });

    EXPECT_EQ(EDbCompletionTargetType::NetworkSession, target._type);
    EXPECT_EQ(42, target._id);
    EXPECT_TRUE(target.IsValid());
}

TEST(DbCompletionTargetTests, ActorTargetCarriesActorId)
{
    auto const target = DbCompletionTarget::Actor(7);

    EXPECT_EQ(EDbCompletionTargetType::Actor, target._type);
    EXPECT_EQ(7, target._id);
    EXPECT_TRUE(target.IsValid());
}