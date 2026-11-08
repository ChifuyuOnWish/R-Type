#include "boss/BossRegistry.hpp"

#include "boss/GiantTank.hpp"
#include "level/LevelLoader.hpp"

#include <stdexcept>

namespace rtype::server {

const BossRegistry& bosses()
{
    static const BossRegistry registry = [] {
        BossRegistry r;
        r.add<GiantTank>("giant_tank");
        return r;
    }();
    return registry;
}

bool BossRegistry::has(const std::string& id) const
{
    return _entries.contains(id);
}

const BossRegistry::Entry& BossRegistry::entry(const std::string& id) const
{
    const auto it = _entries.find(id);
    if (it == _entries.end())
        throw std::out_of_range("no boss class registered as '" + id + "'");
    return it->second;
}

std::unique_ptr<IBoss> BossRegistry::create(const std::string& id) const
{
    return entry(id).make();
}

void BossRegistry::check(const level::GameData& data) const
{
    for (const auto& [id, def] : data.bosses) {
        const std::string at = "enemies.json: bosses/" + id;
        const auto it = _entries.find(id);
        if (it == _entries.end())
            throw level::LevelError(at + ": no boss class registered as '" + id +
                                    "' (server/src/boss/BossRegistry.cpp)");
        const Entry& e = it->second;
        for (const std::string& part : e.parts)
            if (!def.parts.contains(part))
                throw level::LevelError(at + "/parts/" + part + ": missing");
        for (const std::string& role : e.minions)
            if (!def.minions.contains(role))
                throw level::LevelError(at + "/minions/" + role + ": missing");
        for (const auto& [key, value] : def.tuning) {
            if (e.tuning.contains(key))
                continue;
            std::string known;
            for (const auto& [k, v] : e.tuning)
                known += (known.empty() ? "" : ", ") + k;
            throw level::LevelError(at + "/tuning/" + key + ": unknown key (known: " + known + ")");
        }
        if (def.phases.size() + 1 > e.maxPhases)
            throw level::LevelError(at + "/phases: " + id + " has " + std::to_string(e.maxPhases) +
                                    " phases, so at most " + std::to_string(e.maxPhases - 1) +
                                    " thresholds");
    }
}

} // namespace rtype::server
