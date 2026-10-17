#pragma once

#include <cstdint>

namespace rtype::network {

enum class TransportMessage : std::uint8_t
{
    ConnectRequest = 0x01,
    ConnectAccept = 0x02,
    ConnectReject = 0x03,
    Disconnect = 0x04,
    Ping = 0x05,
    Pong = 0x06,
};

inline constexpr std::uint8_t FirstGameMessage = 0x10;

enum class RejectReason : std::uint8_t
{
    Full = 1,
    BadVersion = 2,
    Banned = 3
};

} // namespace rtype::network
