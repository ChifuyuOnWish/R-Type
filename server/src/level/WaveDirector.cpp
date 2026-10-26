#include "level/WaveDirector.hpp"

#include "World.hpp"
#include "components/Components.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace rtype::server::level {

namespace {
constexpr float SpawnX = WorldWidth + 40.F;
constexpr float SpawnMargin = 80.F;
constexpr float MinFirstShot = 0.5F;
constexpr float MaxFirstShot = 2.5F;
} // namespace

WaveDirector::WaveDirector(const Level& level, const Scaling& scaling, std::uint32_t seed)
    : _level(level), _scaling(scaling), _rng(seed)
{
    schedule(0.F);
}

bool WaveDirector::finished() const
{
    return _next >= _level.waves.size() && cleared();
}

bool WaveDirector::cleared() const
{
    return _pending.empty() && _spawned.empty();
}

void WaveDirector::schedule(float previousStart)
{
    if (_next >= _level.waves.size())
        return;
    const Wave& wave = _level.waves[_next];
    if (wave.waitForClear)
        _nextWaveAt.reset();
    else
        _nextWaveAt = previousStart + wave.delay;
}

void WaveDirector::prune(engine::Registry& registry)
{
    std::erase_if(_spawned, [&registry](engine::Entity e) { return !registry.isAlive(e); });
}

void WaveDirector::startWave(const Wave& wave, float at, std::size_t players)
{
    const float extra = static_cast<float>(players > 1 ? players - 1 : 0);
    const int sends =
        std::max(wave.repeat, static_cast<int>(std::lround(static_cast<float>(wave.repeat) *
                                                           (1.F + _scaling.enemyCount * extra))));

    std::vector<float> weights;
    for (const RosterEntry& entry : wave.roster)
        weights.push_back(entry.weight);
    std::discrete_distribution<std::size_t> pick(weights.begin(), weights.end());
    const Formation& formation = *wave.formation;
    float top = 0.F;
    float bottom = 0.F;
    for (const Vec2& slot : formation.slots) {
        top = std::min(top, slot.y);
        bottom = std::max(bottom, slot.y);
    }
    const float minAnchor = SpawnMargin - top;
    const float maxAnchor = std::max(minAnchor, WorldHeight - SpawnMargin - bottom);
    std::uniform_real_distribution<float> randomY(minAnchor, maxAnchor);

    const std::size_t before = _pending.size();
    for (int s = 0; s < sends; ++s) {
        const float base = at + static_cast<float>(s) * wave.repeatDelay;
        float anchor = WorldHeight / 2.F;
        if (wave.y.kind == YAnchor::Kind::Value)
            anchor = wave.y.value;
        else if (wave.y.kind == YAnchor::Kind::Random)
            anchor = randomY(_rng);
        anchor = std::clamp(anchor, minAnchor, maxAnchor);
        for (std::size_t j = 0; j < formation.slots.size(); ++j) {
            const Vec2& slot = formation.slots[j];
            const EnemyDef& enemy = *wave.roster[pick(_rng)].enemy;
            float when = base + static_cast<float>(j) * formation.slotDelay;
            if (enemy.speed > 0.F)
                when += slot.x / enemy.speed;
            const float y = anchor + slot.y;
            const int health = static_cast<int>(
                std::ceil(static_cast<float>(enemy.health) * (1.F + _scaling.enemyHealth * extra)));
            _pending.push_back(Pending{when, enemy, y, health});
        }
    }
    std::cout << "wave " << _next + 1 << "/" << _level.waves.size() << ": " << formation.id << ", "
              << _pending.size() - before << " enemies\n";
}

void WaveDirector::update(GameContext& context, float dt)
{
    _clock += dt;
    prune(context.registry);

    while (_next < _level.waves.size()) {
        const Wave& wave = _level.waves[_next];
        if (!_nextWaveAt) {
            if (!cleared())
                break;
            _nextWaveAt = _clock + wave.delay;
        }
        if (_clock < *_nextWaveAt)
            break;
        const float startedAt = *_nextWaveAt;
        startWave(wave, startedAt, context.players.all().size());
        ++_next;
        schedule(startedAt);
    }

    std::uniform_real_distribution<float> firstShot(MinFirstShot, MaxFirstShot);
    std::erase_if(_pending, [&](const Pending& p) {
        if (p.at > _clock)
            return false;
        _spawned.push_back(context.spawner.spawnEnemy(p.enemy.get(), engine::Position{SpawnX, p.y},
                                                      p.health, firstShot(_rng)));
        return true;
    });
}

} // namespace rtype::server::level
