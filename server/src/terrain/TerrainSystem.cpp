#include "terrain/TerrainSystem.hpp"

#include "Spawner.hpp"
#include "components/Components.hpp"
#include "ecs/Registry.hpp"
#include "terrain/Tile.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace rtype::server {

using namespace engine;

namespace {

struct TileBox
{
    Entity entity;
    Position center;
    Tile tile;
};

struct Overlap
{
    float x = 0.F;
    float y = 0.F;
    bool any() const { return x > 0.F && y > 0.F; }
};

Overlap overlap(const Position& a, float aw, float ah, const Position& b, float bw, float bh)
{
    return Overlap{(aw + bw) / 2.F - std::abs(a.x - b.x), (ah + bh) / 2.F - std::abs(a.y - b.y)};
}

void addHit(Registry& registry, Entity victim, int amount, Entity source)
{
    if (registry.hasComponent<Hit>(victim)) {
        Hit& hit = registry.getComponent<Hit>(victim);
        hit.amount += amount;
        hit.source = source;
    } else {
        registry.addComponent(victim, Hit{amount, source});
    }
}

} // namespace

void TerrainSystem::update(Registry& registry, float dt)
{
    registry.each<TerrainContact>([dt](Entity, TerrainContact& contact) {
        contact.cooldown = std::max(0.F, contact.cooldown - dt);
    });

    std::vector<TileBox> tiles;
    registry.each<Tile, Position>(
        [&tiles](Entity e, Tile& t, Position& p) { tiles.push_back(TileBox{e, p, t}); });
    if (tiles.empty())
        return;

    struct PendingHit
    {
        Entity victim;
        int amount;
        Entity source;
    };
    std::vector<PendingHit> hits;
    std::vector<Entity> touched;

    registry.each<Controllable, Position, Hitbox>([&](Entity ship, Controllable&, Position& p,
                                                      Hitbox& box) {
        const bool canHurt = !registry.hasComponent<TerrainContact>(ship) ||
                             registry.getComponent<TerrainContact>(ship).cooldown <= 0.F;
        int damage = 0;
        Entity source{};
        for (const TileBox& t : tiles) {
            const Overlap o = overlap(p, box.width, box.height, t.center, t.tile.size, t.tile.size);
            if (!o.any())
                continue;
            if (t.tile.contactDamage > damage) {
                damage = t.tile.contactDamage;
                source = t.entity;
            }
            if (!t.tile.solid)
                continue;

            if (o.x < o.y)
                p.x += p.x < t.center.x ? -o.x : o.x;
            else
                p.y += p.y < t.center.y ? -o.y : o.y;
        }
        if (damage > 0 && canHurt) {
            hits.push_back(PendingHit{ship, damage, source});
            touched.push_back(ship);
        }
    });

    registry.each<Projectile, Position, Hitbox>(
        [&](Entity missile, Projectile& projectile, Position& p, Hitbox& box) {
            for (const TileBox& t : tiles) {
                if (!t.tile.solid ||
                    !overlap(p, box.width, box.height, t.center, t.tile.size, t.tile.size).any())
                    continue;
                registry.destroyLater(missile);
                const bool fromPlayer = registry.hasComponent<Team>(missile) &&
                                        registry.getComponent<Team>(missile).id == PlayerTeam;
                if (fromPlayer && registry.hasComponent<Health>(t.entity)) {
                    const int amount = registry.hasComponent<Damage>(missile)
                                           ? registry.getComponent<Damage>(missile).value
                                           : 1;
                    hits.push_back(PendingHit{t.entity, amount, projectile.owner});
                }
                return;
            }
        });

    for (const PendingHit& h : hits)
        addHit(registry, h.victim, h.amount, h.source);
    for (const Entity ship : touched) {
        if (registry.hasComponent<TerrainContact>(ship))
            registry.getComponent<TerrainContact>(ship).cooldown = TerrainHitCooldown;
        else
            registry.addComponent(ship, TerrainContact{TerrainHitCooldown});
    }
}

} // namespace rtype::server
