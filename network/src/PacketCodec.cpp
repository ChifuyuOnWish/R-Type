#include "PacketCodec.hpp"

#include "ByteReader.hpp"

namespace rtype::network {

std::size_t messageSize(std::size_t payloadSize, bool reliable)
{
    return (reliable ? 6 : 4) + payloadSize;
}

void writeMessage(protocol::ByteWriter& w, std::uint8_t type, bool reliable,
                  std::uint16_t reliableId, std::span<const std::uint8_t> payload)
{
    w.writeU8(type);
    w.writeU8(reliable ? 1 : 0);
    if (reliable) {
        w.writeU16(reliableId);
    }
    w.writeU16(static_cast<std::uint16_t>(payload.size()));
    for (const auto& b : payload) {
        w.writeU8(b);
    }
}

std::optional<std::vector<ParsedMessage>> parseMessages(std::span<const std::uint8_t> body)
{
    std::vector<ParsedMessage> out;
    protocol::ByteReader r(body.data(), body.size());
    while (r.remaining() > 0) {
        ParsedMessage m;
        auto type = r.readU8();
        auto channel = r.readU8();
        if (!type || !channel)
            return std::nullopt;
        if (*channel > 1)
            return std::nullopt;
        m.type = *type;
        m.reliable = (*channel == 1);
        if (m.reliable) {
            auto id = r.readU16();
            if (!id)
                return std::nullopt;
            m.reliableId = *id;
        }
        auto length = r.readU16();
        if (!length || *length > r.remaining())
            return std::nullopt;
        m.payload.reserve(*length);
        for (std::size_t i = 0; i < *length; ++i) {
            m.payload.push_back(*r.readU8());
        }
        out.push_back(std::move(m));
    }
    return out;
}

} // namespace rtype::network
