#include "CorePch.h"
#include "DbDispatcher.h"
#include "TaskDispatcher.h"

namespace
{
    class DbCommandTask final : public ITask
    {
    public:
        DbCommandTask(int64_t const ownerKey, std::unique_ptr<IDbCommand> command)
            : ITask(ETaskType::DB, ownerKey)
            , _command(std::move(command))
        {
        }

        void Execute() override
        {
            if (not _command)
            {
                return;
            }

            auto const result = _command->Execute();
            _command->OnCompleted(result);
        }

    private:
        std::unique_ptr<IDbCommand> _command;
    };
}

LambdaDbCommand::LambdaDbCommand(ExecuteFunc execute, CompletedFunc completed)
    : _execute(std::move(execute))
    , _completed(std::move(completed))
{
}

EDbCommandResult LambdaDbCommand::Execute()
{
    if (not _execute)
    {
        return EDbCommandResult::Failed;
    }

    return _execute();
}

void LambdaDbCommand::OnCompleted(EDbCommandResult const result)
{
    if (_completed)
    {
        _completed(result);
    }
}

bool DbDispatcher::Enqueue(int64_t const ownerKey, std::unique_ptr<IDbCommand> command) const
{
    if (ownerKey < 0 || not command)
    {
        return false;
    }

    return TaskDispatcher::Singleton::GetInstance().Dispatch(
        std::make_shared<DbCommandTask>(ownerKey, std::move(command)));
}
