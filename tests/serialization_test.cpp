// Unit tests for ByteWriter / ByteReader (plain asserts, no framework needed).
#undef NDEBUG
#include "ByteReader.hpp"
#include "ByteWriter.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace rtype::protocol;
using Bytes = std::vector<std::uint8_t>;

static void test_exact_layout()
{
    ByteWriter w;
    w.writeU8(0xAB);
    w.writeU16(0x1234);
    w.writeU32(0x12345678);
    w.writeI16(-2);
    w.writeI32(-1);
    const Bytes expected = {0xAB, 0x12, 0x34, 0x12, 0x34, 0x56, 0x78,
                            0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF};
    assert(w.data() == expected);

    ByteWriter s;
    assert(s.writeString("Bob"));
    const Bytes expectedStr = {0x03, 0x42, 0x6F, 0x62};
    assert(s.data() == expectedStr);
}

static void test_round_trip()
{
    ByteWriter w;
    w.writeU8(0);
    w.writeU8(255);
    w.writeU16(0);
    w.writeU16(std::numeric_limits<std::uint16_t>::max());
    w.writeU32(0);
    w.writeU32(std::numeric_limits<std::uint32_t>::max());
    w.writeI16(std::numeric_limits<std::int16_t>::min());
    w.writeI16(std::numeric_limits<std::int16_t>::max());
    w.writeI32(std::numeric_limits<std::int32_t>::min());
    w.writeI32(std::numeric_limits<std::int32_t>::max());
    w.writeString("");
    w.writeString("h\xC3\xA9llo"); // UTF-8 "héllo"

    ByteReader r(w.data().data(), w.size());
    assert(r.readU8() == 0);
    assert(r.readU8() == 255);
    assert(r.readU16() == 0);
    assert(r.readU16() == std::numeric_limits<std::uint16_t>::max());
    assert(r.readU32() == 0);
    assert(r.readU32() == std::numeric_limits<std::uint32_t>::max());
    assert(r.readI16() == std::numeric_limits<std::int16_t>::min());
    assert(r.readI16() == std::numeric_limits<std::int16_t>::max());
    assert(r.readI32() == std::numeric_limits<std::int32_t>::min());
    assert(r.readI32() == std::numeric_limits<std::int32_t>::max());
    assert(r.readString() == "");
    assert(r.readString() == "h\xC3\xA9llo");
    assert(r.remaining() == 0);
}

static void test_truncated_input()
{
    const Bytes two = {0x01, 0x02};
    ByteReader r(two.data(), two.size());
    assert(!r.readU32().has_value());
    assert(!r.readI32().has_value());
    assert(r.remaining() == 2);
    assert(r.readU16() == 0x0102);
    assert(!r.readU8().has_value());
    assert(!r.readString().has_value());

    const Bytes badStr = {0x05, 'a', 'b'};
    ByteReader r2(badStr.data(), badStr.size());
    assert(!r2.readString().has_value());
    assert(r2.remaining() == 3);
    ByteReader r3(nullptr, 0);
    assert(!r3.readU8().has_value());
}

static void test_string_too_long()
{
    ByteWriter w;
    assert(w.writeString(std::string(255, 'x')));
    const std::size_t before = w.size();
    assert(!w.writeString(std::string(256, 'x')));
    assert(w.size() == before); // nothing written on failure
}

int main()
{
    test_exact_layout();
    test_round_trip();
    test_truncated_input();
    test_string_too_long();
    std::cout << "protocol serialization: all tests passed\n";
    return 0;
}