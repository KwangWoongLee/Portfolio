#pragma once
#include "CorePch.h"

#include "IOCPSessionManager.h"
#include "TaskDispatcher.h"

template <typename T_PACKET>
class PacketSendTask final
    : public ITask
{
public:
    PacketSendTask(SessionId const sessionId, T_PACKET&& packet)
        : ITask(ETaskType::NetworkIO, sessionId)
        , _sessionId(sessionId)
        , _packet(std::forward<T_PACKET>(packet))
    {
    }

    void Execute() override
    {
        auto const session = IOCPSessionManager::Singleton::GetInstance().Find(_sessionId);
        if (not session)
        {
            return;
        }

        session->SendPacket(_packet);
    }

private:
    SessionId _sessionId{};
    T_PACKET _packet;
};


class PacketRecvTask final 
	: public ITask
{
public:
    PacketRecvTask(SessionId const sessionId, uint16_t const packetId, std::vector<uint8_t> payload)
        : ITask(ETaskType::NetworkIO, sessionId)
        , _sessionId(sessionId)
        , _packetId(packetId)
        , _payload(std::move(payload))
    {
    }

    void Execute() override
    {
        auto const iocpSession = IOCPSessionManager::Singleton::GetInstance().Find(_sessionId);
        if (not iocpSession)
        {
            return;
        }

        iocpSession->HandlePacket(_packetId, _payload.data(), static_cast<uint32_t>(_payload.size()));
    }

private:
    SessionId _sessionId{};
    uint16_t _packetId{};
    std::vector<uint8_t> _payload;
};

template <typename T_PACKET>
void PostSendPacket(SessionId const sessionId, T_PACKET&& packet)
{
    using PacketType = std::decay_t<T_PACKET>;
    using TaskType = PacketSendTask<PacketType>;

    auto task = ObjectPool<TaskType>::Singleton::GetInstance().AcquireShared(sessionId, std::forward<T_PACKET>(packet));
    TaskDispatcher::Singleton::GetInstance().Dispatch(task);
}