#include "GameSession.hpp"
#include "NetworkServer.hpp"
#include "level/LevelLoader.hpp"
#include "modes/InfiniteMode.hpp"

#include <atomic>
#include <charconv>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>

namespace {

std::atomic<bool> g_running{true};

void onSignal(int)
{
    g_running = false;
}

bool parsePort(const char* text, std::uint16_t& port)
{
    unsigned value = 0;
    const char* end = text + std::strlen(text);
    const auto [ptr, ec] = std::from_chars(text, end, value);
    if (ec != std::errc() || ptr != end || value == 0 || value > 65535)
        return false;
    port = static_cast<std::uint16_t>(value);
    return true;
}

int checkLevels(std::filesystem::path dir, std::filesystem::path types)
{
    dir = std::filesystem::absolute(dir).lexically_normal();
    if (!dir.has_filename())
        dir = dir.parent_path();
    if (types.empty())
        types = dir.parent_path() / "entity_types.json";
    try {
        const auto data = rtype::server::level::LevelLoader::load(dir, types);
        std::size_t waves = 0;
        for (const auto& level : data.levels)
            waves += level.waves.size();
        std::cout << "OK: " << data.levels.size() << " levels, " << waves << " waves, "
                  << data.enemies.size() << " enemies, " << data.bosses.size() << " bosses, "
                  << data.formations.size() << " formations, " << data.entityTypes.size()
                  << " entity types\n";
        return 0;
    } catch (const rtype::server::level::LevelError& e) {
        std::cerr << e.what() << '\n';
        return 84;
    }
}

} // namespace

int main(int argc, char** argv)
{
    if ((argc == 3 || argc == 4) && std::strcmp(argv[1], "--check-levels") == 0)
        return checkLevels(argv[2], argc == 4 ? argv[3] : "");

    std::uint16_t port = 4242;
    if (argc > 2 || (argc == 2 && !parsePort(argv[1], port))) {
        std::cerr << "usage: " << argv[0] << " [port]   (default 4242)\n"
                  << "       " << argv[0] << " --check-levels <dir> [entity-types]\n";
        return 84;
    }
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    rtype::network::NetworkServer net;
    if (const auto ec = net.start(port)) {
        std::cerr << "r-type_server: cannot listen on UDP port " << port << ": " << ec.message()
                  << '\n';
        return 84;
    }
    std::cout << "r-type_server listening on UDP port " << net.localPort() << '\n';

    // net is declared before session, so it outlives it.
    rtype::server::GameSession session(net, std::make_unique<rtype::server::InfiniteMode>());
    session.run(g_running);

    std::cout << "r-type_server: shutting down\n";
    net.stop();
    return 0;
}
