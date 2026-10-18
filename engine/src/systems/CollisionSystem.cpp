#include "systems/CollisionSystem.hpp"

#include "Components.hpp"
#include "Registry.hpp"

#include <cmath>
#include <vector>

namespace rtype::engine {

namespace {

bool overlaps(const Position& pa, const Hitbox& ha, const Position& pb, const Hitbox& hb)
{
    return std::abs(pa.x - pb.x) * 2.F < ha.width + hb.width &&
           std::abs(pa.y - pb.y) * 2.F < ha.height + hb.height;
}

void giveHit(Registry& registry, Entity attacker, Entity victim)
{
    if (!registry.hasComponent<Damage>(attacker) || !registry.hasComponent<Health>(victim)) {
        return;
    }
    const int amount = registry.getComponent<Damage>(attacker).value;
    if (registry.hasComponent<Hit>(victim)) {
        registry.getComponent<Hit>(victim).amount += amount;
    } else {
        registry.addComponent(victim, Hit{amount});
    }
}

} // namespace

void CollisionSystem::update(Registry& registry, float)
{
    const auto owners = registry.getStorage<Hitbox>().entities();
    const std::vector<Entity> entities(owners.begin(), owners.end());

    for (std::size_t i = 0; i < entities.size(); i++) {
        for (std::size_t j = i + 1; j < entities.size(); j++) {
            const Entity a = entities[i];
            const Entity b = entities[j];
            if (!registry.hasComponent<Position>(a) || !registry.hasComponent<Position>(b)) {
                continue;
            }
            if (overlaps(registry.getComponent<Position>(a), registry.getComponent<Hitbox>(a),
                         registry.getComponent<Position>(b), registry.getComponent<Hitbox>(b))) {
                giveHit(registry, a, b);
                giveHit(registry, b, a);
            }
        }
    }
}

} // namespace rtype::engine
