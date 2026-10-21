#include "Players.hpp"

namespace rtype::server {

Player& Players::add(network::PeerId id, const std::string& name, engine::Entity ship)
{
    auto [it, inserted] = _players.insert_or_assign(id, Player{name, ship});
    (void)inserted;
    return it->second;
}

std::optional<Player> Players::remove(network::PeerId id)
{
    auto it = _players.find(id);
    if (it == _players.end())
        return std::nullopt;

    Player player = std::move(it->second);
    _players.erase(it);
    return player;
}

Player* Players::find(network::PeerId id)
{
    auto it = _players.find(id);
    if (it == _players.end())
        return nullptr;

    return &it->second;
}

const Player* Players::find(network::PeerId id) const
{
    auto it = _players.find(id);
    if (it == _players.end())
        return nullptr;

    return &it->second;
}

bool Players::isShip(engine::Entity entity) const
{
    for (const auto& [_, player] : _players) {
        if (player.ship == entity)
            return true;
    }
    return false;
}

const std::map<network::PeerId, Player>& Players::all() const
{
    return _players;
}

void Players::applyInput(network::PeerId id, const protocol::InputMessage& input,
                         engine::Registry& registry)
{
    Player* player = find(id);
    if (!player)
        return;
    if (player->hasInput && input.clientTick <= player->lastInputTick)
        return;
    player->lastInputTick = input.clientTick;
    player->hasInput = true;

    if (!registry.isAlive(player->ship) ||
        !registry.hasComponent<engine::Controllable>(player->ship))
        return;
    auto& c = registry.getComponent<engine::Controllable>(player->ship);
    c.up = (input.buttons & protocol::ButtonUp) != 0;
    c.down = (input.buttons & protocol::ButtonDown) != 0;
    c.left = (input.buttons & protocol::ButtonLeft) != 0;
    c.right = (input.buttons & protocol::ButtonRight) != 0;
    c.shoot = (input.buttons & protocol::ButtonShoot) != 0;
}

} // namespace rtype::server
