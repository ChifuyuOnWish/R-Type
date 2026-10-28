#include "LobbyMessages.hpp"

#include <algorithm>

namespace rtype::protocol {

namespace {

void writeText(ByteWriter& w, const std::string& text)
{
    const std::size_t size = std::min(text.size(), MaxLevelNameLength);
    w.writeU8(static_cast<std::uint8_t>(size));
    for (std::size_t i = 0; i < size; ++i)
        w.writeU8(static_cast<std::uint8_t>(text[i]));
}

std::optional<std::string> readText(ByteReader& r)
{
    const auto size = r.readU8();
    if (!size || *size > MaxLevelNameLength || *size > r.remaining())
        return std::nullopt;
    std::string text;
    for (std::uint8_t i = 0; i < *size; ++i)
        text.push_back(static_cast<char>(*r.readU8()));
    return text;
}

std::optional<GameKind> readKind(ByteReader& r)
{
    const auto v = r.readU8();
    if (!v || *v > static_cast<std::uint8_t>(GameKind::Pvp))
        return std::nullopt;
    return static_cast<GameKind>(*v);
}

} // namespace

bool LobbyStateMessage::serialize(ByteWriter& w) const
{
    const std::size_t count = std::min(levelNames.size(), MaxLobbyLevels);
    w.writeU8(static_cast<std::uint8_t>(phase));
    w.writeU8(leaderId);
    w.writeU8(static_cast<std::uint8_t>(kind));
    w.writeU8(level);
    w.writeU8(unlocked);
    w.writeU8(static_cast<std::uint8_t>(count));
    for (std::size_t i = 0; i < count; ++i)
        writeText(w, levelNames[i]);
    return true;
}

std::optional<LobbyStateMessage> LobbyStateMessage::deserialize(ByteReader& r)
{
    LobbyStateMessage m;
    const auto phase = r.readU8();
    const auto leader = r.readU8();
    const auto kind = readKind(r);
    const auto level = r.readU8();
    const auto unlocked = r.readU8();
    const auto count = r.readU8();
    if (!phase || *phase > static_cast<std::uint8_t>(LobbyPhase::Playing) || !leader || !kind ||
        !level || !unlocked || !count || *count > MaxLobbyLevels)
        return std::nullopt;
    m.phase = static_cast<LobbyPhase>(*phase);
    m.leaderId = *leader;
    m.kind = *kind;
    m.level = *level;
    m.unlocked = *unlocked;
    for (std::uint8_t i = 0; i < *count; ++i) {
        auto name = readText(r);
        if (!name)
            return std::nullopt;
        m.levelNames.push_back(std::move(*name));
    }
    if (r.remaining() != 0 || m.unlocked > *count || (*count > 0 && m.level >= *count))
        return std::nullopt;
    return m;
}

bool LobbySelectMessage::serialize(ByteWriter& w) const
{
    w.writeU8(static_cast<std::uint8_t>(kind));
    w.writeU8(level);
    return true;
}

std::optional<LobbySelectMessage> LobbySelectMessage::deserialize(ByteReader& r)
{
    const auto kind = readKind(r);
    const auto level = r.readU8();
    if (!kind || !level || r.remaining() != 0)
        return std::nullopt;
    return LobbySelectMessage{*kind, *level};
}

bool LobbyStartMessage::serialize(ByteWriter&) const
{
    return true;
}

std::optional<LobbyStartMessage> LobbyStartMessage::deserialize(ByteReader& r)
{
    if (r.remaining() != 0)
        return std::nullopt;
    return LobbyStartMessage{};
}

} // namespace rtype::protocol
