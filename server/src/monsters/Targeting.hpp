#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"

#include <optional>
#include <vector>

namespace rtype::server {

std::vector<engine::Position> shipPositions(engine::Registry& registry);

std::optional<engine::Position> nearest(const std::vector<engine::Position>& ships,
                                        engine::Position from);

std::optional<engine::Position> nearestShip(engine::Registry& registry, engine::Position from);

engine::Velocity towards(engine::Position from, engine::Position to, float speed);

} // namespace rtype::server
