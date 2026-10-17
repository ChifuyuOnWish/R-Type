#pragma once

#include "ByteReader.hpp"
#include "ByteWriter.hpp"
#include "Fixed.hpp"
#include "MessageType.hpp"
#include "NetworkConfig.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace rtype::protocol {

inline constexpr std::size_t MinPlayerNameLength = 1;
inline constexpr std::size_t MaxPlayerNameLength = 32;

inline constexpr std::uint8_t ButtonUp = 1U << 0;
inline constexpr std::uint8_t ButtonDown = 1U << 1;
inline constexpr std::uint8_t ButtonLeft = 1U << 2;
inline constexpr std::uint8_t ButtonRight = 1U << 3;
inline constexpr std::uint8_t ButtonShoot = 1U << 4;
inline constexpr std::uint8_t ButtonMask = 0x1F;

enum class PlayerLeftReason : std::uint8_t
{
    Quit = 0,
    TimedOut = 1,
    Kicked = 2,
};

enum class DestroyCause : std::uint8_t
{
    LeftScreen = 0,
    Killed = 1,
    Other = 2,
};

enum class GameEventType : std::uint8_t
{
    Shoot = 0,
    Hit = 1,
    Kill = 2,
    PlayerDeath = 3,
};

struct InputMessage
{
    static constexpr MessageType Type = MessageType::Input;

    std::uint32_t clientTick = 0;
    std::uint8_t buttons = 0;

    bool serialize(ByteWriter& w) const;
    static std::optional<InputMessage> deserialize(ByteReader& r);
    bool operator==(const InputMessage&) const = default;
};

struct PlayerJoinedMessage
{
    static constexpr MessageType Type = MessageType::PlayerJoined;

    std::uint8_t clientId = 0;
    std::string name;

    bool serialize(ByteWriter& w) const;
    static std::optional<PlayerJoinedMessage> deserialize(ByteReader& r);
    bool operator==(const PlayerJoinedMessage&) const = default;
};

struct PlayerLeftMessage
{
    static constexpr MessageType Type = MessageType::PlayerLeft;

    std::uint8_t clientId = 0;
    PlayerLeftReason reason = PlayerLeftReason::Quit;

    bool serialize(ByteWriter& w) const;
    static std::optional<PlayerLeftMessage> deserialize(ByteReader& r);
    bool operator==(const PlayerLeftMessage&) const = default;
};

// Reliable.
struct EntitySpawnMessage
{
    static constexpr MessageType Type = MessageType::EntitySpawn;

    std::uint32_t entityId = 0;
    std::uint16_t entityType = 0;
    Fixed x;
    Fixed y;

    bool serialize(ByteWriter& w) const;
    static std::optional<EntitySpawnMessage> deserialize(ByteReader& r);
    bool operator==(const EntitySpawnMessage&) const = default;
};

struct EntityDestroyMessage
{
    static constexpr MessageType Type = MessageType::EntityDestroy;

    std::uint32_t entityId = 0;
    DestroyCause cause = DestroyCause::LeftScreen;

    bool serialize(ByteWriter& w) const;
    static std::optional<EntityDestroyMessage> deserialize(ByteReader& r);
    bool operator==(const EntityDestroyMessage&) const = default;
};

struct SnapshotEntry
{
    std::uint32_t entityId = 0;
    Fixed x;
    Fixed y;
    Fixed16 vx;
    Fixed16 vy;

    bool operator==(const SnapshotEntry&) const = default;
};

inline constexpr std::size_t SnapshotEntrySize = 16;
inline constexpr std::size_t SnapshotFixedPartSize = 6;

inline constexpr std::size_t MaxSnapshotEntries =
    (network::MaxDatagramSize - network::HeaderSize - 4 - SnapshotFixedPartSize) /
    SnapshotEntrySize;

struct SnapshotMessage
{
    static constexpr MessageType Type = MessageType::Snapshot;

    std::uint32_t serverTick = 0;
    std::vector<SnapshotEntry> entries;

    bool serialize(ByteWriter& w) const;
    static std::optional<SnapshotMessage> deserialize(ByteReader& r);
    bool operator==(const SnapshotMessage&) const = default;
};

struct GameEventMessage
{
    static constexpr MessageType Type = MessageType::GameEvent;

    GameEventType event = GameEventType::Shoot;
    std::uint32_t sourceId = 0;
    std::uint32_t targetId = 0;

    bool serialize(ByteWriter& w) const;
    static std::optional<GameEventMessage> deserialize(ByteReader& r);
    bool operator==(const GameEventMessage&) const = default;
};

} // namespace rtype::protocol