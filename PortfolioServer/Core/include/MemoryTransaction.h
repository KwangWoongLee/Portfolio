#pragma once
#include "CorePch.h"

class MemoryTransaction;

class IUndoLog
{
public:
    virtual ~IUndoLog() = default;
    virtual void Apply() = 0;
    virtual void Rollback() = 0;
    virtual void Persist() {}

    int64_t GetOwnerId() const { return _ownerId; }

private:
    friend class MemoryTransaction;
    int64_t _ownerId{};
};

class MemoryTransaction final
{
public:
    explicit MemoryTransaction(int64_t const ownerId) : _ownerId(ownerId) {}
    ~MemoryTransaction();

    MemoryTransaction(MemoryTransaction const&) = delete;
    MemoryTransaction& operator=(MemoryTransaction const&) = delete;
    MemoryTransaction(MemoryTransaction&&) = delete;
    MemoryTransaction& operator=(MemoryTransaction&&) = delete;

    void AddUndoLog(std::unique_ptr<IUndoLog> log);

    template <typename T, typename... ARGS>
    void Add(ARGS&&... args)
    {
        static_assert(std::is_base_of_v<IUndoLog, T>, "T must derive from IUndoLog");
        AddUndoLog(std::make_unique<T>(std::forward<ARGS>(args)...));
    }

    void MarkFailed();

    bool IsFailed() const { return _failed; }
    int64_t GetOwnerId() const { return _ownerId; }

private:
    void Rollback();

    int64_t const _ownerId;
    std::vector<std::unique_ptr<IUndoLog>> _undoLogs;
    bool _failed{ false };
};
