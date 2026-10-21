#pragma once

#include "Messages.hpp"
#include "NetworkEvent.hpp"
#include "components/Components.hpp"
#include "ecs/Entity.hpp"
#include "ecs/Registry.hpp"

#include <map>

namespace rtype::server {

struct Player
{
    std::string name;
    engine::Entity ship;
    std::uint32_t lastInputTick = 0;
    bool hasInput = false;
};

class Players
{
public:
    Player& add(network::PeerId id, const std::string& name, engine::Entity ship);
    std::optional<Player> remove(network::PeerId id);
    Player* find(network::PeerId id);
    const Player* find(network::PeerId id) const;
    bool isShip(engine::Entity entity) const;
    const std::map<network::PeerId, Player>& all() const;
    void applyInput(network::PeerId id, const protocol::InputMessage& input,
                    engine::Registry& registry);

private:
    std::map<network::PeerId, Player> _players;
};

} // namespace rtype::server
