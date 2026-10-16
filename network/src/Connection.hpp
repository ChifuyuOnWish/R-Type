#pragma once

#include <array>
#include <chrono>
#include <cstdint>
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

private:
    Receive updateReceived(std::uint16_t sequence);
    void markAcked(std::uint16_t sequence, TimePoint now, std::vector<std::uint16_t>& newlyAcked);

    struct SentPacket
    {
        std::uint16_t sequence = 0;
        TimePoint sentAt{};
        bool acked = false;
        bool valid = false;
    };
    static constexpr std::size_t SentWindow = 1024;

    std::uint16_t _localSequence = 1;
    std::uint16_t _remoteSequence = 0;
    std::uint32_t _receivedBits = 0;
    bool _hasReceived = false;
    std::array<SentPacket, SentWindow> _sent{};
    TimePoint _lastReceived{};
    Rtt _rtt{0};
    bool _hasRtt = false;
};

} // namespace rtype::network
