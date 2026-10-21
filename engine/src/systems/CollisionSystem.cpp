#include "CollisionSystem.hpp"

#include "Components/Components.hpp"
#include "ecs/Registry.hpp"

#include <cmath>
#include <vector>

namespace rtype::engine {

namespace {

bool overlaps(const Position& pa, const Hitbox& ha, const Position& pb, const Hitbox& hb)
{
    return std::abs(pa.x - pb.x) * 2.F < ha.width + hb.width &&
           std::abs(pa.y - pb.y) * 2.F < ha.height + hb.height;
}

bool sameTeam(Registry& registry, Entity a, Entity b)
{
    return registry.hasComponent<Team>(a) && registry.hasComponent<Team>(b) &&
           registry.getComponent<Team>(a).id == registry.getComponent<Team>(b).id;
}

bool giveHit(Registry& registry, Entity attacker, Entity victim)
{
    if (!registry.hasComponent<Damage>(attacker) || !registry.hasComponent<Health>(victim)) {
        return false;
    }
    const int amount = registry.getComponent<Damage>(attacker).value;
    const Entity source = registry.hasComponent<Projectile>(attacker)
                              ? registry.getComponent<Projectile>(attacker).owner
                              : attacker;
    if (registry.hasComponent<Hit>(victim)) {
        Hit& hit = registry.getComponent<Hit>(victim);
        hit.amount += amount;
        hit.source = source;
    } else {
        registry.addComponent(victim, Hit{amount, source});
    }
    return true;
}

} // namespace

void CollisionSystem::update(Registry& registry, float)
{
    const auto owners = registry.getStorage<Hitbox>().entities();
    const std::vector<Entity> entities(owners.begin(), owners.end());
    std::vector<Entity> spent;

    const auto isSpent = [&spent](Entity e) {
        for (const Entity s : spent) {
            if (s == e) {
                return true;
            }
        }
        return false;
    };

    for (std::size_t i = 0; i < entities.size(); i++) {
        for (std::size_t j = i + 1; j < entities.size(); j++) {
            const Entity a = entities[i];
            const Entity b = entities[j];
            if (!registry.hasComponent<Position>(a) || !registry.hasComponent<Position>(b)) {
                continue;
            }
            if (sameTeam(registry, a, b) || isSpent(a) || isSpent(b)) {
                continue;
            }
            const bool aIsMissile = registry.hasComponent<Projectile>(a);
            const bool bIsMissile = registry.hasComponent<Projectile>(b);
            if (aIsMissile && bIsMissile) {
                continue;
            }
            if (!overlaps(registry.getComponent<Position>(a), registry.getComponent<Hitbox>(a),
                          registry.getComponent<Position>(b), registry.getComponent<Hitbox>(b))) {
                continue;
            }
            if (giveHit(registry, a, b) && aIsMissile) {
                spent.push_back(a);
                registry.destroyLater(a);
            }
            if (giveHit(registry, b, a) && bIsMissile) {
                spent.push_back(b);
                registry.destroyLater(b);
            }
        }
    }
}

} // namespace rtype::engine
