#pragma once

#include <chrono>
#include <cstddef>

namespace rtype::network {

inline constexpr std::size_t MaxDatagramSize = 1200;
inline constexpr std::size_t HeaderSize = 16;
inline constexpr std::chrono::milliseconds NetworkTickInterval{10};

inline constexpr std::size_t ReliableMessageOverhead = 6;
inline constexpr std::size_t MaxPendingReliable = 256;
inline constexpr std::chrono::milliseconds ReliableResendInterval{100};
inline constexpr std::chrono::seconds ReliableTimeout{5};

} // namespace rtype::network
