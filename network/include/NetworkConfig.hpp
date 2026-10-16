#pragma once

#include <chrono>
#include <cstddef>

namespace rtype::network {

inline constexpr std::size_t MaxDatagramSize = 1200;
inline constexpr std::size_t HeaderSize = 16;
inline constexpr std::chrono::milliseconds NetworkTickInterval{10};

} // namespace rtype::network
