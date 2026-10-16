#include "ByteReader.hpp"

namespace rtype::protocol {

ByteReader::ByteReader(const std::uint8_t* data, std::size_t size)
    : m_data(data), m_size(size)
{
}

std::optional<std::uint8_t> ByteReader::readU8()
{
    if (remaining() < 1)
        return std::nullopt;
    return m_data[m_pos++];
}

std::optional<std::uint16_t> ByteReader::readU16()
{
    if (remaining() < 2)
        return std::nullopt;
    std::uint16_t v = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(m_data[m_pos]) << 8) | m_data[m_pos + 1]);
    m_pos += 2;
    return v;
}

std::optional<std::uint32_t> ByteReader::readU32()
{
    if (remaining() < 4)
        return std::nullopt;
    std::uint32_t v = (static_cast<std::uint32_t>(m_data[m_pos]) << 24)
        | (static_cast<std::uint32_t>(m_data[m_pos + 1]) << 16)
        | (static_cast<std::uint32_t>(m_data[m_pos + 2]) << 8)
        | static_cast<std::uint32_t>(m_data[m_pos + 3]);
    m_pos += 4;
    return v;
}

std::optional<std::int16_t> ByteReader::readI16()
{
    auto v = readU16();
    if (!v)
        return std::nullopt;
    return static_cast<std::int16_t>(*v);
}

std::optional<std::int32_t> ByteReader::readI32()
{
    auto v = readU32();
    if (!v)
        return std::nullopt;
    return static_cast<std::int32_t>(*v);
}

std::optional<std::string> ByteReader::readString()
{
    if (remaining() < 1)
        return std::nullopt;
    const std::size_t len = m_data[m_pos];
    if (remaining() - 1 < len)
        return std::nullopt;
    std::string s(reinterpret_cast<const char*>(m_data + m_pos + 1), len);
    m_pos += 1 + len;
    return s;
}

std::size_t ByteReader::remaining() const
{
    return m_size - m_pos;
}

}