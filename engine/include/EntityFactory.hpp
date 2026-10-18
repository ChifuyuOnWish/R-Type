#pragma once

#include "Components.hpp"
#include "Registry.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

namespace rtype::engine {

struct SpawnParams
{
    float x = 0.F;
    float y = 0.F;
    unsigned playerId = 0;
    Entity owner{};
    std::uint32_t netId = 0;
};

class EntityFactory
{
public:
    using Builder = std::function<Entity(Registry&, const SpawnParams&)>;

    explicit EntityFactory(Registry& registry);
    void registerArchetype(std::string name, Builder builder);
    bool has(const std::string& name) const;
    Entity create(const std::string& name, const SpawnParams& params = {}) const;

private:
    Registry& _registry;
    std::unordered_map<std::string, Builder> _builders;
};

} // namespace rtype::engine
