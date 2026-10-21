#include "Replicator.hpp"

#include "World.hpp"
#include "components/Components.hpp"
#include "core/Update.hpp"

namespace rtype::server {

using namespace engine;
using namespace protocol;
using network::PeerId;

namespace {
constexpr unsigned StepsPerSnapshot = 3;

bool outsideWorld(float x, float y)
{
    return x < 0.F || x > WorldWidth || y < 0.F || y > WorldHeight;
}
} // namespace

Replicator::Replicator(network::NetworkServer& net, Registry& registry)
    : _net(net), _registry(registry)
{
}

void Replicator::sync()
{
    _registry.each<NetType, Position>([&](Entity e, NetType& t, Position& p) {
        if (_registry.hasComponent<NetId>(e))
            return;
        const std::uint32_t netId = _nextNetId++;
        const Synced synced{e, t.type, p.x, p.y};
        _registry.addComponent(e, NetId{netId});
        _synced[netId] = synced;
        broadcast(EntitySpawnMessage{netId, synced.type, Fixed::fromFloat(synced.x),
                                     Fixed::fromFloat(synced.y)},
                  true);
    });
    for (auto it = _synced.begin(); it != _synced.end();) {
        Synced& s = it->second;
        if (_registry.isAlive(s.entity)) {
            if (_registry.hasComponent<Position>(s.entity)) {
                const auto& p = _registry.getComponent<Position>(s.entity);
                s.x = p.x;
                s.y = p.y;
            }
            ++it;
            continue;
        }
        const auto cause = outsideWorld(s.x, s.y) ? DestroyCause::LeftScreen : DestroyCause::Killed;
        broadcast(EntityDestroyMessage{it->first, cause}, true);
        it = _synced.erase(it);
    }
}

void Replicator::sendEvents(std::vector<GameEventMessage>& events)
{
    for (const auto& ev : events)
        broadcast(ev, true);
    events.clear();
}

void Replicator::afterSteps(unsigned steps, std::uint32_t serverTick)
{
    _stepsSinceSnapshot += steps;
    if (_stepsSinceSnapshot >= StepsPerSnapshot) {
        sendSnapshot(serverTick);
        _stepsSinceSnapshot = 0;
    }
}

void Replicator::sendWorldTo(PeerId peer)
{
    for (const auto& [netId, s] : _synced) {
        if (!_registry.isAlive(s.entity) || !_registry.hasComponent<Position>(s.entity))
            continue;
        const auto& p = _registry.getComponent<Position>(s.entity);
        sendTo(peer,
               EntitySpawnMessage{netId, s.type, Fixed::fromFloat(p.x), Fixed::fromFloat(p.y)},
               true);
    }
}

void Replicator::forget(Entity entity)
{
    if (const std::uint32_t netId = netIdOf(_registry, entity)) {
        broadcast(EntityDestroyMessage{netId, DestroyCause::Other}, true);
        _synced.erase(netId);
    }
}

std::uint32_t Replicator::wireId(Entity entity) const
{
    if (const std::uint32_t id = netIdOf(_registry, entity))
        return id;
    for (const auto& [netId, s] : _synced)
        if (s.entity == entity)
            return netId;
    return 0;
}

void Replicator::sendSnapshot(std::uint32_t serverTick)
{
    constexpr float PerTick = static_cast<float>(Update::Step);
    SnapshotMessage snap{serverTick, {}};
    for (const auto& [netId, s] : _synced) {
        SnapshotEntry e{netId, Fixed::fromFloat(s.x), Fixed::fromFloat(s.y), {}, {}};
        if (_registry.hasComponent<Velocity>(s.entity)) {
            const auto& v = _registry.getComponent<Velocity>(s.entity);
            e.vx = Fixed16::fromFloat(v.dx * PerTick);
            e.vy = Fixed16::fromFloat(v.dy * PerTick);
        }
        snap.entries.push_back(e);
        if (snap.entries.size() == MaxSnapshotEntries) {
            broadcast(snap, false);
            snap.entries.clear();
        }
    }
    if (!snap.entries.empty() || _synced.empty())
        broadcast(snap, false);
}

} // namespace rtype::server
