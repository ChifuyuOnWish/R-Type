#include "RateLimiter.hpp"

namespace rtype::network {

RateVerdict RateLimiter::onPacket(const UdpSocket::Endpoint& from, TimePoint now)
{
    auto it = _entries.find(from);
    if (it == _entries.end()) {
        if (_entries.size() >= MaxTrackedEndpoints)
            purge(now);
        if (_entries.size() >= MaxTrackedEndpoints)
            return RateVerdict::Drop;
        it = _entries.try_emplace(from, Entry{now}).first;
    }

    Entry& entry = it->second;
    if (now - entry.windowStart >= RateWindow) {
        if (entry.count <= MaxPacketsPerSecond || now - entry.windowStart >= RateWindow * 2) {
            entry.strikes = 0;
        }
        entry.windowStart = now;
        entry.count = 0;
    }
    ++entry.count;
    if (entry.count <= MaxPacketsPerSecond) {
        return RateVerdict::Allow;
    }
    if (entry.count == MaxPacketsPerSecond + 1) {
        ++entry.strikes;
    }
    return entry.strikes >= RateLimitStrikes ? RateVerdict::Kick : RateVerdict::Drop;
}

void RateLimiter::purge(TimePoint now)
{
    for (auto it = _entries.begin(); it != _entries.end();) {
        if (now - it->second.windowStart >= 2 * RateWindow) {
            it = _entries.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace rtype::network
