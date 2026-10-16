#pragma once

#include <cstdint>
#include <limits>

namespace rtype::engine {

class Entity
{
public:
    using Index = std::uint32_t;
    using Generation = std::uint32_t;
    static constexpr Index NullIndex = std::numeric_limits<Index>::max();

    Entity() noexcept = default;
    Entity(Index index, Generation generation) noexcept;

    Index index() const noexcept;
    Generation generation() const noexcept;
    bool isNull() const noexcept;

    bool operator==(const Entity&) const noexcept = default;

private:
    Index _index = NullIndex;
    Generation _generation = 0;
};

} // namespace rtype::engine
