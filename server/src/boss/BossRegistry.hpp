#pragma once

#include "LevelData.hpp"
#include "boss/IBoss.hpp"

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace rtype::server {

// Boss id (the key under "bosses" in enemies.json) -> the class that plays it.
class BossRegistry
{
public:
    using Factory = std::function<std::unique_ptr<IBoss>()>;

    struct Entry
    {
        Factory make;
        std::map<std::string, float> tuning; // every key the class reads, with its default
        std::vector<std::string> parts;      // "parts" it needs in enemies.json
        std::vector<std::string> minions;    // "minions" roles it needs
        std::size_t maxPhases = 1;
    };

    // A boss class provides Defaults(), Parts, Minions and MaxPhases (see
    // GiantTank.hpp).
    template <typename Boss> void add(const std::string& id)
    {
        _entries[id] = Entry{[] { return std::make_unique<Boss>(); },
                             Boss::Defaults(),
                             {Boss::Parts.begin(), Boss::Parts.end()},
                             {Boss::Minions.begin(), Boss::Minions.end()},
                             Boss::MaxPhases};
    }

    bool has(const std::string& id) const;
    std::unique_ptr<IBoss> create(const std::string& id) const;
    const Entry& entry(const std::string& id) const;

    // Every boss in enemies.json must have a class, and give it the parts and
    // minions it needs, no tuning key it doesn't know (a typo would otherwise be
    // silently ignored) and no more phases than it has. Throws level::LevelError,
    // like the loader.
    void check(const level::GameData& data) const;

private:
    std::map<std::string, Entry> _entries;
};

// All the game's bosses. Registering one is one line in BossRegistry.cpp.
const BossRegistry& bosses();

} // namespace rtype::server
