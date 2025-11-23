#pragma once

enum class ETaskType : uint8_t
{
    None = 0,
    Basic,
    DB,
    Max
};

size_t constexpr TASK_TYPE_MAX = static_cast<size_t>(ETaskType::Max);

class ITask
{
public:
    explicit ITask(ETaskType const taskType, int64_t const key)
        : _taskType(taskType)
        , _key(key)
    {}

    virtual ~ITask() = default;

public:
    auto GetKey() const { return _key; }
    auto GetTaskType() const { return _taskType; }

    virtual void Execute() = 0;

private:
    ETaskType _taskType{};
    int64_t _key{};
};
