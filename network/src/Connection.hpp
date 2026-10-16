#pragma once

#include "NetworkConfig.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <vector>

namespace rtype::network {

class Connection
{
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Rtt = std::chrono::duration<float, std::milli>;

    struct AckFields
    {
        std::uint16_t sequence;
        std::uint16_t ack;
        std::uint32_t ackBits;
    };

    enum class Receive
    {
        New,
        Duplicate,
        TooOld
    };

    [[nodiscard]] AckFields prepareSend(TimePoint now);
    [[nodiscard]] Receive onPacketReceived(std::uint16_t sequence, std::uint16_t ack,
                                           std::uint32_t ackBits, TimePoint now,
                                           std::vector<std::uint16_t>& newlyAcked);
    [[nodiscard]] Rtt rtt() const;
    [[nodiscard]] TimePoint lastReceived() const;

    struct ReliableMessage
    {
        std::uint16_t id = 0;
        std::uint8_t type = 0;
        std::vector<std::uint8_t> payload;
        TimePoint queuedAt{};
        TimePoint lastSentAt{};
        bool sent = false;
    };

    [[nodiscard]] bool queueReliable(std::uint8_t type, std::vector<std::uint8_t> payload,
                                     TimePoint now);
    [[nodiscard]] std::vector<const ReliableMessage*>
    takeDueReliable(std::uint16_t sequence, TimePoint now, std::size_t budget);
    [[nodiscard]] bool acceptReliable(std::uint16_t id);
    [[nodiscard]] bool reliableTimedOut(TimePoint now) const;
    [[nodiscard]] std::size_t pendingReliable() const;

private:
    Receive updateReceived(std::uint16_t sequence);
    void markAcked(std::uint16_t sequence, TimePoint now, std::vector<std::uint16_t>& newlyAcked);

    struct SentPacket
    {
        std::uint16_t sequence = 0;
        TimePoint sentAt{};
        bool acked = false;
        bool valid = false;
        std::vector<std::uint16_t> reliableIds;
    };

    struct ReceivedReliable
    {
        std::uint16_t id = 0;
        bool valid = false;
    };
    static constexpr std::size_t ReceivedWindow = 1024;
    static constexpr std::size_t SentWindow = 1024;

    std::uint16_t _localSequence = 1;
    std::uint16_t _remoteSequence = 0;
    std::uint32_t _receivedBits = 0;
    bool _hasReceived = false;
    std::array<SentPacket, SentWindow> _sent{};
    TimePoint _lastReceived{};
    Rtt _rtt{0};
    bool _hasRtt = false;

    std::uint16_t _nextReliableId = 0;
    std::deque<ReliableMessage> _pending;
    std::array<ReceivedReliable, ReceivedWindow> _received{};
};

} // namespace rtype::network
