#pragma once

#include "Messages.hpp"
#include "Players.hpp"
#include "Registry.hpp"
#include "Spawner.hpp"

#include <random>
#include <vector>

namespace rtype::server {

struct GameContext
{
    engine::Registry& registry;
    Spawner& spawner;
    const Players& players;
    std::vector<protocol::GameEventMessage>& events;
    std::mt19937& rng;
};

} // namespace rtype::server
