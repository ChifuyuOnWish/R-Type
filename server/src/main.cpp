#include "GameSession.hpp"
#include "InfiniteMode.hpp"
#include "NetworkServer.hpp"

#include <atomic>
#include <charconv>
#include <csignal>
#include <cstdint>
#include <cstring>
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

} // namespace

int main(int argc, char** argv)
{
    std::uint16_t port = 4242;
    if (argc > 2 || (argc == 2 && !parsePort(argv[1], port))) {
        std::cerr << "usage: " << argv[0] << " [port]   (default 4242)\n";
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

    rtype::server::GameSession session(net, std::make_unique<rtype::server::InfiniteMode>());
    session.run(g_running);

    std::cout << "r-type_server: shutting down\n";
    net.stop();
    return 0;
}
