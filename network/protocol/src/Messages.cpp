#include "Messages.hpp"

namespace rtype::protocol {

namespace {

bool readFixed(ByteReader& r, Fixed& out)
{
    const auto v = r.readI32();
    if (!v)
        return false;
    out.raw = *v;
    return true;
}

bool readFixed16(ByteReader& r, Fixed16& out)
{
    const auto v = r.readI16();
    if (!v)
        return false;
    out.raw = *v;
    return true;
}

void writeFixed(ByteWriter& w, Fixed f)
{
    w.writeI32(f.raw);
}

void writeFixed16(ByteWriter& w, Fixed16 f)
{
    w.writeI16(f.raw);
}

} // namespace

bool InputMessage::serialize(ByteWriter& w) const
{
    if ((buttons & ~ButtonMask) != 0)
        return false;
    w.writeU32(clientTick);
    w.writeU8(buttons);
    return true;
}

std::optional<InputMessage> InputMessage::deserialize(ByteReader& r)
{
    const auto tick = r.readU32();
    const auto buttons = r.readU8();
    if (!tick || !buttons || (*buttons & ~ButtonMask) != 0 || r.remaining() != 0)
        return std::nullopt;
    return InputMessage{*tick, *buttons};
}

bool PlayerJoinedMessage::serialize(ByteWriter& w) const
{
    if (name.size() < MinPlayerNameLength || name.size() > MaxPlayerNameLength)
        return false;
    w.writeU8(clientId);
    return w.writeString(name);
}

std::optional<PlayerJoinedMessage> PlayerJoinedMessage::deserialize(ByteReader& r)
{
    const auto id = r.readU8();
    auto name = r.readString();
    if (!id || !name || name->size() < MinPlayerNameLength || name->size() > MaxPlayerNameLength ||
        r.remaining() != 0)
        return std::nullopt;
    return PlayerJoinedMessage{*id, std::move(*name)};
}

bool PlayerLeftMessage::serialize(ByteWriter& w) const
{
    if (static_cast<std::uint8_t>(reason) > static_cast<std::uint8_t>(PlayerLeftReason::Kicked))
        return false;
    w.writeU8(clientId);
    w.writeU8(static_cast<std::uint8_t>(reason));
    return true;
}

std::optional<PlayerLeftMessage> PlayerLeftMessage::deserialize(ByteReader& r)
{
    const auto id = r.readU8();
    const auto reason = r.readU8();
    if (!id || !reason || *reason > static_cast<std::uint8_t>(PlayerLeftReason::Kicked) ||
        r.remaining() != 0)
        return std::nullopt;
    return PlayerLeftMessage{*id, static_cast<PlayerLeftReason>(*reason)};
}

bool EntitySpawnMessage::serialize(ByteWriter& w) const
{
    w.writeU32(entityId);
    w.writeU16(entityType);
    writeFixed(w, x);
    writeFixed(w, y);
    return true;
}

std::optional<EntitySpawnMessage> EntitySpawnMessage::deserialize(ByteReader& r)
{
    EntitySpawnMessage m;
    const auto id = r.readU32();
    const auto type = r.readU16();
    if (!id || !type)
        return std::nullopt;
    m.entityId = *id;
    m.entityType = *type;
    if (!readFixed(r, m.x) || !readFixed(r, m.y) || r.remaining() != 0)
        return std::nullopt;
    return m;
}

bool EntityDestroyMessage::serialize(ByteWriter& w) const
{
    if (static_cast<std::uint8_t>(cause) > static_cast<std::uint8_t>(DestroyCause::Other))
        return false;
    w.writeU32(entityId);
    w.writeU8(static_cast<std::uint8_t>(cause));
    return true;
}

std::optional<EntityDestroyMessage> EntityDestroyMessage::deserialize(ByteReader& r)
{
    const auto id = r.readU32();
    const auto cause = r.readU8();
    if (!id || !cause || *cause > static_cast<std::uint8_t>(DestroyCause::Other) ||
        r.remaining() != 0)
        return std::nullopt;
    return EntityDestroyMessage{*id, static_cast<DestroyCause>(*cause)};
}

bool SnapshotMessage::serialize(ByteWriter& w) const
{
    if (entries.size() > MaxSnapshotEntries)
        return false;
    w.writeU32(serverTick);
    w.writeU16(static_cast<std::uint16_t>(entries.size()));
    for (const SnapshotEntry& e : entries) {
        w.writeU32(e.entityId);
        writeFixed(w, e.x);
        writeFixed(w, e.y);
        writeFixed16(w, e.vx);
        writeFixed16(w, e.vy);
    }
    return true;
}

std::optional<SnapshotMessage> SnapshotMessage::deserialize(ByteReader& r)
{
    const auto tick = r.readU32();
    const auto count = r.readU16();
    if (!tick || !count || *count > MaxSnapshotEntries ||
        r.remaining() != static_cast<std::size_t>(*count) * SnapshotEntrySize)
        return std::nullopt;

    SnapshotMessage m;
    m.serverTick = *tick;
    m.entries.reserve(*count);
    for (std::uint16_t i = 0; i < *count; ++i) {
        SnapshotEntry e;
        const auto id = r.readU32();
        if (!id)
            return std::nullopt;
        e.entityId = *id;
        if (!readFixed(r, e.x) || !readFixed(r, e.y) || !readFixed16(r, e.vx) ||
            !readFixed16(r, e.vy))
            return std::nullopt;
        m.entries.push_back(e);
    }
    return m;
}

bool GameEventMessage::serialize(ByteWriter& w) const
{
    if (static_cast<std::uint8_t>(event) > static_cast<std::uint8_t>(GameEventType::PlayerDeath))
        return false;
    w.writeU8(static_cast<std::uint8_t>(event));
    w.writeU32(sourceId);
    w.writeU32(targetId);
    return true;
}

std::optional<GameEventMessage> GameEventMessage::deserialize(ByteReader& r)
{
    const auto event = r.readU8();
    const auto source = r.readU32();
    const auto target = r.readU32();
    if (!event || !source || !target ||
        *event > static_cast<std::uint8_t>(GameEventType::PlayerDeath) || r.remaining() != 0)
        return std::nullopt;
    return GameEventMessage{static_cast<GameEventType>(*event), *source, *target};
}

} // namespace rtype::protocol