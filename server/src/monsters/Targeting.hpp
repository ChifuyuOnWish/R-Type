#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"

#include <optional>
#include <vector>

namespace rtype::server {

// Every player ship's position. MonsterSystem reads them once per step, before its own loop,
// so it never runs one `each` inside another.
std::vector<engine::Position> shipPositions(engine::Registry& registry);

// The closest of `ships` to `from`, or nothing when the list is empty.
std::optional<engine::Position> nearest(const std::vector<engine::Position>& ships,
                                        engine::Position from);

// The same, looking the ships up: for one-off questions such as "where does this shot go".
std::optional<engine::Position> nearestShip(engine::Registry& registry, engine::Position from);

// A velocity of length `speed` from `from` towards `to`. Straight left when they overlap.
engine::Velocity towards(engine::Position from, engine::Position to, float speed);

} // namespace rtype::server
