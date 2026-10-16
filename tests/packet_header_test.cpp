#undef NDEBUG
#include "PacketHeader.hpp"
#include "SequenceNumber.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

using rtype::protocol::HeaderStatus;
using rtype::protocol::PacketHeader;
using rtype::protocol::PacketMagic;
using rtype::protocol::ProtocolVersion;

static std::vector<std::uint8_t> makeValidHeader(std::uint16_t sequence = 7, std::uint16_t ack = 6,
                                                 std::uint32_t ackBits = 0,
                                                 std::uint32_t session = 12)
{
    std::vector<std::uint8_t> bytes(16, 0);
    bytes[0] = static_cast<std::uint8_t>((PacketMagic >> 8) & 0xFF);
    bytes[1] = static_cast<std::uint8_t>(PacketMagic & 0xFF);
    bytes[2] = ProtocolVersion;
    bytes[3] = 0;
    bytes[4] = static_cast<std::uint8_t>((session >> 24) & 0xFF);
    bytes[5] = static_cast<std::uint8_t>((session >> 16) & 0xFF);
    bytes[6] = static_cast<std::uint8_t>((session >> 8) & 0xFF);
    bytes[7] = static_cast<std::uint8_t>(session & 0xFF);
    bytes[8] = static_cast<std::uint8_t>((sequence >> 8) & 0xFF);
    bytes[9] = static_cast<std::uint8_t>(sequence & 0xFF);
    bytes[10] = static_cast<std::uint8_t>((ack >> 8) & 0xFF);
    bytes[11] = static_cast<std::uint8_t>(ack & 0xFF);
    bytes[12] = static_cast<std::uint8_t>((ackBits >> 24) & 0xFF);
    bytes[13] = static_cast<std::uint8_t>((ackBits >> 16) & 0xFF);
    bytes[14] = static_cast<std::uint8_t>((ackBits >> 8) & 0xFF);
    bytes[15] = static_cast<std::uint8_t>(ackBits & 0xFF);
    return bytes;
}

static void test_valid_header_decode()
{
    auto bytes = makeValidHeader(42, 41, 0x01020304U, 99U);
    PacketHeader hdr{};
    assert(PacketHeader::decode(bytes.data(), bytes.size(), hdr) == HeaderStatus::Ok);
    assert(hdr.session == 99U);
    assert(hdr.sequence == 42U);
    assert(hdr.ack == 41U);
    assert(hdr.ackBits == 0x01020304U);
    assert(hdr.version == ProtocolVersion);
    assert(hdr.flags == 0);
}

static void test_invalid_size_and_field_rejections()
{
    std::vector<std::uint8_t> shortBytes(15, 0);
    PacketHeader hdr{};
    assert(PacketHeader::decode(shortBytes.data(), shortBytes.size(), hdr) ==
           HeaderStatus::TooShort);

    auto tooLong = makeValidHeader();
    tooLong.resize(1201, 0);
    assert(PacketHeader::decode(tooLong.data(), tooLong.size(), hdr) == HeaderStatus::TooLong);

    auto badMagic = makeValidHeader();
    badMagic[0] = 0x00;
    badMagic[1] = 0x00;
    assert(PacketHeader::decode(badMagic.data(), badMagic.size(), hdr) == HeaderStatus::BadMagic);

    auto badVersion = makeValidHeader();
    badVersion[2] = 2;
    assert(PacketHeader::decode(badVersion.data(), badVersion.size(), hdr) ==
           HeaderStatus::BadVersion);

    auto badFlags = makeValidHeader();
    badFlags[3] = 1;
    assert(PacketHeader::decode(badFlags.data(), badFlags.size(), hdr) == HeaderStatus::BadFlags);
}

static void test_sequence_wraparound()
{
    using rtype::network::sequenceGreater;

    assert(sequenceGreater(0u, 65535u));
    assert(!sequenceGreater(65535u, 0u));
    assert(sequenceGreater(1u, 65535u));
    assert(!sequenceGreater(10u, 10u));
    assert(sequenceGreater(50u, 49u));
    assert(!sequenceGreater(49u, 50u));
}

int main()
{
    test_valid_header_decode();
    test_invalid_size_and_field_rejections();
    test_sequence_wraparound();
    return 0;
}