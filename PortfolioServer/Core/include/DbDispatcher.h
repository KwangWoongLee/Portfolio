#pragma once
#include "CorePch.h"

enum class EDbCommandResult : uint8_t
{
    Succeeded,
    Failed,
};

class IDbCommand
{
public:
    virtual ~IDbCommand() = default;

    virtual EDbCommandResult Execute() = 0;

    // Runs on the DB task worker. Callback bodies must only post messages or record
    // tracking data; they must not mutate actor-owned game state directly.
    virtual void OnCompleted(EDbCommandResult result) = 0;
};

class LambdaDbCommand final : public IDbCommand
{
public:
    using ExecuteFunc = std::function<EDbCommandResult()>;
    using CompletedFunc = std::function<void(EDbCommandResult)>;

    LambdaDbCommand(ExecuteFunc execute, CompletedFunc completed);

    EDbCommandResult Execute() override;
    void OnCompleted(EDbCommandResult result) override;

private:
    ExecuteFunc _execute;
    CompletedFunc _completed;
};

class DbDispatcher final
{
public:
    using Singleton = Singleton<DbDispatcher>;

    bool Enqueue(int64_t ownerKey, std::unique_ptr<IDbCommand> command) const;
};
