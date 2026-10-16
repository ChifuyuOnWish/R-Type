#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace rtype::protocol {

class ByteReader {
public:
    ByteReader(const std::uint8_t* data, std::size_t size);

    std::optional<std::uint8_t> readU8();
    std::optional<std::uint16_t> readU16();
    std::optional<std::uint32_t> readU32();
    std::optional<std::int16_t> readI16();
    std::optional<std::int32_t> readI32();

    std::optional<std::string> readString();

    std::size_t remaining() const;

private:
    const std::uint8_t* m_data;
    std::size_t m_size;
    std::size_t m_pos = 0;
};

}