#include "CorePch.h"
#include "MemoryTransaction.h"

MemoryTransaction::~MemoryTransaction()
{
    if (_failed)
    {
        Rollback();
        return;
    }

    for (auto const& log : _undoLogs)
    {
        log->Persist();
    }
}

void MemoryTransaction::AddUndoLog(std::unique_ptr<IUndoLog> log)
{
    if (_failed)
    {
        return;
    }

    log->_ownerId = _ownerId;
    log->Apply();
    _undoLogs.push_back(std::move(log));
}

void MemoryTransaction::MarkFailed()
{
    _failed = true;
}

void MemoryTransaction::Rollback()
{
    for (auto iter = _undoLogs.rbegin(); _undoLogs.rend() != iter; ++iter)
    {
        (*iter)->Rollback();
    }
}
