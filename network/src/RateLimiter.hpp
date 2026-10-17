#pragma once

#include "NetworkConfig.hpp"
#include "UdpSocket.hpp"

#include <chrono>
#include <cstdint>
#include <map>

namespace rtype::network {

enum class RateVerdict
{
    Allow,
    Drop,
    Kick
};

class RateLimiter
{
public:
    using TimePoint = std::chrono::steady_clock::time_point;

    [[nodiscard]] RateVerdict onPacket(const UdpSocket::Endpoint& from, TimePoint now);

private:
    void purge(TimePoint now);

    struct Entry
    {
        TimePoint windowStart;
        std::uint32_t count = 0;
        std::uint32_t strikes = 0;
    };
    std::map<UdpSocket::Endpoint, Entry> _entries;
};

} // namespace rtype::network