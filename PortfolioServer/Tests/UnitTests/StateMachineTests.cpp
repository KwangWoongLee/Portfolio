#include <gtest/gtest.h>
#include "StateMachine.h"

namespace
{
    enum class TestState : uint8_t
    {
        Scheduled,
        Prepare,
        InProgress,
        Finished,
    };

    struct TestContext final
    {
        bool _canStart{ false };
        std::vector<std::string> _events;
    };
}

TEST(StateMachineTests, SequentialTransitionCallsExitEnterAndHook)
{
    StateMachine<TestState, TestContext> machine(TestState::Scheduled);
    TestContext context;

    machine.RegisterState(TestState::Scheduled, {
        {},
        {},
        [](TestContext& ctx) { ctx._events.emplace_back("exit scheduled"); },
    });
    machine.RegisterState(TestState::Prepare, {
        [](TestContext& ctx) { ctx._events.emplace_back("enter prepare"); },
    });
    machine.AllowSequentialTransitions({
        TestState::Scheduled,
        TestState::Prepare,
        TestState::InProgress,
        TestState::Finished,
    });
    machine.SetTransitionHook(
        [](StateTransitionResult<TestState> const&, TestContext& ctx)
        {
            ctx._events.emplace_back("transition hook");
        });

    auto const result = machine.TryTransition(TestState::Prepare, "schedule opened", context);

    EXPECT_TRUE(result.Succeeded());
    EXPECT_EQ(TestState::Prepare, machine.GetState());
    EXPECT_EQ((std::vector<std::string>{
        "exit scheduled",
        "enter prepare",
        "transition hook",
    }), context._events);
}

TEST(StateMachineTests, GuardRejectsTransitionWithoutChangingState)
{
    StateMachine<TestState, TestContext> machine(TestState::Prepare);
    TestContext context;

    machine.AllowTransition(
        TestState::Prepare,
        TestState::InProgress,
        [](TestContext const& ctx) { return ctx._canStart; });

    auto const rejected = machine.TryTransition(TestState::InProgress, "not ready", context);
    EXPECT_TRUE(rejected.Failed());
    EXPECT_EQ(EStateTransitionError::GuardRejected, rejected._error);
    EXPECT_EQ(TestState::Prepare, machine.GetState());

    context._canStart = true;
    auto const accepted = machine.TryTransition(TestState::InProgress, "ready", context);
    EXPECT_TRUE(accepted.Succeeded());
    EXPECT_EQ(TestState::InProgress, machine.GetState());
}

TEST(StateMachineTests, DisallowedTransitionReportsError)
{
    StateMachine<TestState, TestContext> machine(TestState::Scheduled);
    TestContext context;
    machine.RegisterState(TestState::Finished);

    auto const result = machine.TryTransition(TestState::Finished, "skip", context);

    EXPECT_TRUE(result.Failed());
    EXPECT_EQ(EStateTransitionError::TransitionNotAllowed, result._error);
    EXPECT_EQ(TestState::Scheduled, machine.GetState());
}
