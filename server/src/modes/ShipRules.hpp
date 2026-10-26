#pragma once

#include "GameContext.hpp"
#include "NetworkEvent.hpp"
#include "ecs/Entity.hpp"

namespace rtype::server {

engine::Entity spawnShipInLane(GameContext& context, network::PeerId peer);
void respawnDeadShips(engine::Registry& registry);

} // namespace rtype::server
