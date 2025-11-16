#pragma once

enum class ETaskType : uint8_t
{
    NONE = 0,
    BASIC,
    DB,
    MAX
};

size_t constexpr TaskTypeMax = static_cast<size_t>(ETaskType::MAX);

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
