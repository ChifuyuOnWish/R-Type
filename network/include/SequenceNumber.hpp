#pragma once

#include <cstdint>

namespace rtype::network {

inline bool sequenceGreater(std::uint16_t a, std::uint16_t b)
{
    std::uint16_t diff = std::uint16_t(a - b);
    return diff < 0x8000 && diff != 0;
}

} // namespace rtype::network