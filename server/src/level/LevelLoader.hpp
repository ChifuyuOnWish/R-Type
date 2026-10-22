#pragma once

#include "level/LevelData.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

namespace rtype::server::level {

class LevelError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class LevelLoader
{
public:
    static GameData load(const std::filesystem::path& dir,
                         const std::filesystem::path& entityTypes);
};

} // namespace rtype::server::level
