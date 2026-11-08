#pragma once

#include "Messages.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace rtype::protocol {

inline constexpr std::size_t MaxBossNameLength = 32;
inline constexpr std::size_t MaxBossPhases = 8;

inline constexpr MessageType BossStateType = static_cast<MessageType>(0x30);

struct BossStateMessage
{
    static constexpr MessageType Type = BossStateType;

    enum Flag : std::uint8_t
    {
        Exposed = 1 << 0,
        Enraged = 1 << 1,
        Dying = 1 << 2,
    };

    std::uint32_t entityId = 0;
    std::uint8_t phase = 0;
    std::uint8_t phaseCount = 1;
    std::uint16_t health = 0;
    std::uint16_t maxHealth = 1;
    std::uint8_t flags = 0;
    std::vector<std::uint8_t> marks;
    std::string name;

    bool operator==(const BossStateMessage&) const = default;
    bool has(Flag flag) const { return (flags & flag) != 0; }

    bool serialize(ByteWriter& w) const;
    static std::optional<BossStateMessage> deserialize(ByteReader& r);
};

inline bool BossStateMessage::serialize(ByteWriter& w) const
{
    if (maxHealth == 0 || health > maxHealth || marks.size() > MaxBossPhases)
        return false;
    const std::size_t size = std::min(name.size(), MaxBossNameLength);
    w.writeU32(entityId);
    w.writeU8(phase);
    w.writeU8(phaseCount);
    w.writeU16(health);
    w.writeU16(maxHealth);
    w.writeU8(flags);
    w.writeU8(static_cast<std::uint8_t>(marks.size()));
    for (const std::uint8_t mark : marks)
        w.writeU8(mark);
    w.writeU8(static_cast<std::uint8_t>(size));
    for (std::size_t i = 0; i < size; ++i)
        w.writeU8(static_cast<std::uint8_t>(name[i]));
    return true;
}

inline std::optional<BossStateMessage> BossStateMessage::deserialize(ByteReader& r)
{
    BossStateMessage m;
    const auto entity = r.readU32();
    const auto phase = r.readU8();
    const auto phaseCount = r.readU8();
    const auto health = r.readU16();
    const auto maxHealth = r.readU16();
    const auto flags = r.readU8();
    const auto markCount = r.readU8();
    if (!entity || !phase || !phaseCount || !health || !maxHealth || !flags || !markCount ||
        *maxHealth == 0 || *health > *maxHealth || *phase > *phaseCount ||
        *markCount > MaxBossPhases || *markCount > r.remaining())
        return std::nullopt;
    m.entityId = *entity;
    m.phase = *phase;
    m.phaseCount = *phaseCount;
    m.health = *health;
    m.maxHealth = *maxHealth;
    m.flags = *flags;
    for (std::uint8_t i = 0; i < *markCount; ++i) {
        const std::uint8_t mark = *r.readU8();
        if (mark > 100)
            return std::nullopt;
        m.marks.push_back(mark);
    }
    const auto size = r.readU8();
    if (!size || *size > MaxBossNameLength || *size != r.remaining())
        return std::nullopt;
    for (std::uint8_t i = 0; i < *size; ++i)
        m.name.push_back(static_cast<char>(*r.readU8()));
    return m;
}

} // namespace rtype::protocol
