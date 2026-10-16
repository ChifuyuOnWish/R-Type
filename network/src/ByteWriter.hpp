#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace rtype::protocol {

class ByteWriter {
public:
    void writeU8(std::uint8_t v);
    void writeU16(std::uint16_t v);
    void writeU32(std::uint32_t v);
    void writeI16(std::int16_t v);
    void writeI32(std::int32_t v);

    bool writeString(std::string_view s);

    const std::vector<std::uint8_t>& data() const;
    std::size_t size() const;

private:
    std::vector<std::uint8_t> m_buf;
};

} // namespace rtype::protocol