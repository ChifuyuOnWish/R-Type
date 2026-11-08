#pragma once

#include "Messages.hpp"
#include "NetworkEvent.hpp"
#include "ecs/Entity.hpp"
#include "ecs/Registry.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace rtype::server {

struct Player
{
    std::string name;
    std::optional<engine::Entity> ship;
    std::uint32_t lastInputTick = 0;
    bool hasInput = false;
    std::uint64_t joinOrder = 0;
    std::uint32_t score = 0;
    std::uint16_t kills = 0;
    bool dead = false;
};

class Players
{
public:
    Player& add(network::PeerId id, const std::string& name, std::optional<engine::Entity> ship);
    std::optional<Player> remove(network::PeerId id);
    Player* find(network::PeerId id);
    const Player* find(network::PeerId id) const;
    bool isShip(engine::Entity entity) const;
    const std::map<network::PeerId, Player>& all() const;
    void applyInput(network::PeerId id, const protocol::InputMessage& input,
                    engine::Registry& registry);

    std::optional<network::PeerId> leader() const;
    void setShip(network::PeerId id, std::optional<engine::Entity> ship);

    bool refreshDeaths(engine::Registry& registry);
    bool allDead() const;

    bool addScore(engine::Entity ship, std::uint32_t points);
    std::uint32_t teamScore() const;
    void resetScores();

private:
    std::map<network::PeerId, Player> _players;
    std::uint64_t _nextJoin = 0;
};

} // namespace rtype::server
