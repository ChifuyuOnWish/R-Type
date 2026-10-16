#pragma once

#include <compare>
#include <cstdint>

namespace rtype::engine {

class Entity
{
public:
    using Index = std::uint32_t;
    using Generation = std::uint32_t;

    Entity() noexcept = default;
    Entity(Index index, Generation generation) noexcept;

    [[nodiscard]] Index index() const noexcept;
    [[nodiscard]] Generation generation() const noexcept;

    bool operator==(const Entity&) const noexcept = default;

private:
    Index _index = 0;
    Generation _generation = 0;
};

} // namespace rtype::engine
