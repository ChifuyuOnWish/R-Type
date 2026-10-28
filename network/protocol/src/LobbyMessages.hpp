#pragma once

#include "Messages.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace rtype::protocol {

enum class GameKind : std::uint8_t
{
    Level = 0,
    Infinite = 1,
    Pvp = 2
};

enum class LobbyPhase : std::uint8_t
{
    Selecting = 0,
    LevelCleared = 1,
    CampaignCleared = 2,
    GameOver = 3,
    Playing = 4,
};

inline constexpr std::size_t MaxLobbyLevels = 16;
inline constexpr std::size_t MaxLevelNameLength = 32;

struct LobbyStateMessage
{
    static constexpr MessageType Type = MessageType::LobbyState;

    LobbyPhase phase = LobbyPhase::Selecting;
    std::uint8_t leaderId = 0;
    GameKind kind = GameKind::Level;
    std::uint8_t level = 0;
    std::uint8_t unlocked = 1;
    std::vector<std::string> levelNames;

    bool operator==(const LobbyStateMessage&) const = default;

    bool serialize(ByteWriter& w) const;
    static std::optional<LobbyStateMessage> deserialize(ByteReader& r);
};

struct LobbySelectMessage
{
    static constexpr MessageType Type = MessageType::LobbySelect;

    GameKind kind = GameKind::Level;
    std::uint8_t level = 0;

    bool serialize(ByteWriter& w) const;
    static std::optional<LobbySelectMessage> deserialize(ByteReader& r);
};

struct LobbyStartMessage
{
    static constexpr MessageType Type = MessageType::LobbyStart;

    bool serialize(ByteWriter& w) const;
    static std::optional<LobbyStartMessage> deserialize(ByteReader& r);
};

} // namespace rtype::protocol
