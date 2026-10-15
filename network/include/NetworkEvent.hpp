#pragma once

#include <cstdint>
#include <variant>
#include <vector>

namespace rtype::network {

using PeerId = std::uint32_t;

enum class DisconnectReason : std::uint8_t
{
    Quit,
    Timeout,
    Kicked,
};

struct Connected
{
    PeerId peerId;
};

struct MessageReceived
{
    PeerId peerId;
    std::uint8_t type;
    std::vector<std::uint8_t> payload;
};

struct Disconnected
{
    PeerId peerId;
    DisconnectReason reason;
};

using NetworkEvent = std::variant<Connected, MessageReceived, Disconnected>;

struct OutgoingMessage
{
    PeerId peerId;
    std::uint8_t type;
    std::vector<std::uint8_t> payload;
    bool reliable;
};

} // namespace rtype::network