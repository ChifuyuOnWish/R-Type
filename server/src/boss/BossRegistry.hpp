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

class BossRegistry
{
public:
    using Factory = std::function<std::unique_ptr<IBoss>()>;

    struct Entry
    {
        Factory make;
        std::map<std::string, float> tuning;
        std::vector<std::string> parts;
        std::vector<std::string> minions;
        std::size_t maxPhases = 1;
    };

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

    void check(const level::GameData& data) const;

private:
    std::map<std::string, Entry> _entries;
};

const BossRegistry& bosses();

} // namespace rtype::server
