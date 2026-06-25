#include "CorePch.h"
#include "MemoryTransaction.h"

MemoryTransaction::~MemoryTransaction()
{
    if (_completed)
    {
        return;
    }

    if (_failed)
    {
        Rollback();
        _completed = true;
        return;
    }

    (void)Commit();
}

void MemoryTransaction::AddUndoLog(std::unique_ptr<IUndoLog> log)
{
    if (_failed || _completed)
    {
        return;
    }

    log->_ownerId = _ownerId;
    log->Apply();
    _undoLogs.emplace_back(std::move(log));
}

void MemoryTransaction::MarkFailed()
{
    if (_completed)
    {
        return;
    }

    _failed = true;
}

bool MemoryTransaction::Commit()
{
    if (_completed)
    {
        return not _failed;
    }

    if (_failed)
    {
        Rollback();
        _completed = true;
        return false;
    }

    for (auto const& log : _undoLogs)
    {
        if (not log->Persist())
        {
            _failed = true;
            Rollback();
            _completed = true;
            return false;
        }
    }

    _completed = true;
    return true;
}

void MemoryTransaction::Rollback()
{
    for (auto iter = _undoLogs.rbegin(); _undoLogs.rend() != iter; ++iter)
    {
        (*iter)->Rollback();
    }
}
