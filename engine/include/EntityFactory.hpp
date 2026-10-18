#pragma once

#include "Components.hpp"
#include "Registry.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

namespace rtype::engine {

struct ComponentParams
{
    Position position{};
    Velocity velocity{};
    Health health{};
    Hitbox hitbox{};
    Damage damage{};
    Weapon weapon{};
    std::optional<EnemyBehavior> behavior;
    Lifetime lifetime{};
    Invincibility invincibility{};
    Score score{};

    unsigned playerId = 0;
    Entity owner{};
    unsigned pierceCount = 0;
    std::uint32_t netId = 0;
};

class EntityFactory
{
public:
    using Builder = std::function<Entity(Registry&, const ComponentParams&)>;

    explicit EntityFactory(Registry& registry);
    void registerArchetype(std::string name, Builder builder);
    bool has(const std::string& name) const;
    Entity create(const std::string& name, const ComponentParams& params = {}) const;

private:
    Registry& _registry;
    std::unordered_map<std::string, Builder> _builders;
};

} // namespace rtype::engine
