#pragma once

#include "ByteWriter.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace rtype::network {

struct ParsedMessage
{
    std::uint8_t type = 0;
    bool reliable = false;
    std::uint16_t reliableId = 0;
    std::vector<std::uint8_t> payload;
};

std::size_t messageSize(std::size_t payloadSize, bool reliable);
void writeMessage(protocol::ByteWriter& w, std::uint8_t type, bool reliable,
                  std::uint16_t reliableId, std::span<const std::uint8_t> payload);
std::optional<std::vector<ParsedMessage>> parseMessages(std::span<const std::uint8_t> body);

} // namespace rtype::network
