#include "PacketHeader.hpp"

#include "ByteReader.hpp"
#include "NetworkConfig.hpp"

namespace rtype::protocol {

void PacketHeader::encode(ByteWriter& w) const
{
    w.writeU16(PacketMagic);
    w.writeU8(version);
    w.writeU8(flags);
    w.writeU32(session);
    w.writeU16(sequence);
    w.writeU16(ack);
    w.writeU32(ackBits);
}

HeaderStatus PacketHeader::decode(const std::uint8_t* data, std::size_t size, PacketHeader& out)
{
    using rtype::network::HeaderSize;
    using rtype::network::MaxDatagramSize;

    if (data == nullptr || size < HeaderSize)
        return HeaderStatus::TooShort;
    if (size > MaxDatagramSize)
        return HeaderStatus::TooLong;

    ByteReader r(data, size);
    const auto magic = r.readU16();
    const auto version = r.readU8();
    const auto flags = r.readU8();
    const auto session = r.readU32();
    const auto sequence = r.readU16();
    const auto ack = r.readU16();
    const auto ackBits = r.readU32();

    if (!magic || !version || !flags || !session || !sequence || !ack || !ackBits)
        return HeaderStatus::TooShort;

    if (*magic != PacketMagic)
        return HeaderStatus::BadMagic;
    if (*version != ProtocolVersion)
        return HeaderStatus::BadVersion;
    if (*flags != 0)
        return HeaderStatus::BadFlags;

    out.version = *version;
    out.flags = *flags;
    out.session = *session;
    out.sequence = *sequence;
    out.ack = *ack;
    out.ackBits = *ackBits;
    return HeaderStatus::Ok;
}

} // namespace rtype::protocol