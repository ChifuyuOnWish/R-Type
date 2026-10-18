#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace rtype::network {

using PeerId = std::uint32_t;

inline constexpr PeerId NoPeer = 0;
inline constexpr PeerId AllPeers = 0xFFFFFFFF;

enum class DisconnectReason : std::uint8_t
{
    Quit,
    Timeout,
    Kicked,
    ServerShutdown,
};

struct Connected
{
    PeerId peerId;
    std::string name;
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

enum class ConnectFailure : std::uint8_t
{
    Full,
    BadVersion,
    Banned,
    Timeout,
};

struct ConnectionFailed
{
    ConnectFailure reason;
};

using NetworkEvent = std::variant<Connected, MessageReceived, Disconnected, ConnectionFailed>;

struct OutgoingMessage
{
    PeerId peerId;
    std::uint8_t type;
    std::vector<std::uint8_t> payload;
    bool reliable;
    PeerId except = NoPeer;
};

} // namespace rtype::network