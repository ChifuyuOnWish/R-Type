#pragma once

#include "ByteWriter.hpp"

#include <cstddef>
#include <cstdint>

namespace rtype::protocol {

inline constexpr std::uint16_t PacketMagic = 0x5254;
inline constexpr std::uint8_t ProtocolVersion = 1;

enum class HeaderStatus
{
    Ok,
    TooShort,
    TooLong,
    BadMagic,
    BadVersion,
    BadFlags,
};

struct PacketHeader
{
    std::uint8_t version = ProtocolVersion;
    std::uint8_t flags = 0;
    std::uint32_t session = 0;
    std::uint16_t sequence = 0;
    std::uint16_t ack = 0;
    std::uint32_t ackBits = 0;

    void encode(ByteWriter& w) const;

    static HeaderStatus decode(const std::uint8_t* data, std::size_t size, PacketHeader& out);
};

} // namespace rtype::protocol