#pragma once

#include "LevelData.hpp"

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
    static GameData load(const std::filesystem::path& dir);
};

} // namespace rtype::server::level
