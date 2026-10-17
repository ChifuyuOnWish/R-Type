#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace rtype::network {

inline constexpr std::size_t MaxDatagramSize = 1200;
inline constexpr std::size_t HeaderSize = 16;
inline constexpr std::chrono::milliseconds NetworkTickInterval{10};

inline constexpr std::size_t ReliableMessageOverhead = 6;
inline constexpr std::size_t MaxPendingReliable = 256;
inline constexpr std::chrono::milliseconds ReliableResendInterval{100};
inline constexpr std::chrono::seconds ReliableTimeout{5};

inline constexpr std::chrono::seconds HeartbeatInterval{1};
inline constexpr std::chrono::seconds ConnectionTimeout{5};

inline constexpr std::size_t MaxPlayers = 4;
inline constexpr std::size_t MaxNameLength = 32;
inline constexpr std::uint8_t ServerTickRate = 60;

inline constexpr std::uint32_t MaxPacketsPerSecond = 200;
inline constexpr std::uint32_t RateLimitStrikes = 3;
inline constexpr std::size_t MaxTrackedEndpoints = 64;
inline constexpr std::chrono::seconds RateWindow{1};

} // namespace rtype::network
