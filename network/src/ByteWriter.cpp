#include "ByteWriter.hpp"

namespace rtype::protocol {

void ByteWriter::writeU8(std::uint8_t v)
{
    m_buf.push_back(v);
}

void ByteWriter::writeU16(std::uint16_t v)
{
    m_buf.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    m_buf.push_back(static_cast<std::uint8_t>(v & 0xFF));
}

void ByteWriter::writeU32(std::uint32_t v)
{
    m_buf.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
    m_buf.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
    m_buf.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    m_buf.push_back(static_cast<std::uint8_t>(v & 0xFF));
}

void ByteWriter::writeI16(std::int16_t v)
{
    writeU16(static_cast<std::uint16_t>(v));
}

void ByteWriter::writeI32(std::int32_t v)
{
    writeU32(static_cast<std::uint32_t>(v));
}

bool ByteWriter::writeString(std::string_view s)
{
    if (s.size() > 255)
        return false;
    writeU8(static_cast<std::uint8_t>(s.size()));
    for (char c : s)
        m_buf.push_back(static_cast<std::uint8_t>(c));
    return true;
}

const std::vector<std::uint8_t>& ByteWriter::data() const
{
    return m_buf;
}

std::size_t ByteWriter::size() const
{
    return m_buf.size();
}

} // namespace rtype::protocol