#undef NDEBUG
#include "ClientNetwork.hpp"
#include "NetworkServer.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
namespace protocol = rtype::protocol;
namespace network = rtype::network;

namespace {

bool waitForConnection(rtype::client::ClientNetwork& client, std::chrono::milliseconds timeout = 2s)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        client.update();
        if (client.isConnected())
            return true;
        std::this_thread::sleep_for(5ms);
    }
    return false;
}

bool waitForConnected(network::NetworkServer& server, network::PeerId& peerId, std::string& name,
                      std::chrono::milliseconds timeout = 2s)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        for (const auto& event : server.poll()) {
            if (const auto* connected = std::get_if<network::Connected>(&event)) {
                peerId = connected->peerId;
                name = connected->name;
                return true;
            }
        }
        std::this_thread::sleep_for(5ms);
    }
    return false;
}

bool waitForMessage(rtype::client::ClientNetwork& client, std::function<bool()> predicate,
                    std::chrono::milliseconds timeout = 2s)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        client.update();
        if (predicate())
            return true;
        std::this_thread::sleep_for(5ms);
    }
    return false;
}

void test_connect_and_dispatch_game_message()
{
    network::NetworkServer server;
    assert(!server.start(0));

    rtype::client::ClientNetwork client;
    bool connected = false;
    bool disconnected = false;
    network::PeerId peerId = network::NoPeer;
    std::string peerName;
    client.onConnected([&](network::PeerId id, const std::string& name) {
        connected = true;
        peerId = id;
        peerName = name;
    });
    client.onDisconnected([&](network::PeerId, network::DisconnectReason) { disconnected = true; });

    assert(!client.connect("127.0.0.1", server.localPort(), "Alice"));
    assert(waitForConnection(client));
    assert(connected);
    assert(client.isConnected());
    assert(peerId == client.clientId());
    assert(peerName == "Alice");

    std::vector<std::uint8_t> received;
    bool messageHandled = false;
    client.registerHandler(protocol::MessageType::GameEvent,
                           [&](std::span<const std::uint8_t> payload) {
                               messageHandled = true;
                               received.assign(payload.begin(), payload.end());
                           });

    network::PeerId serverPeer = network::NoPeer;
    std::string serverName;
    assert(waitForConnected(server, serverPeer, serverName));
    server.send(serverPeer, static_cast<std::uint8_t>(protocol::MessageType::GameEvent), {9, 8, 7},
                true);

    assert(waitForMessage(client, [&] { return messageHandled; }));
    assert(messageHandled);
    const std::vector<std::uint8_t> expected{9u, 8u, 7u};
    assert(received == expected);
    assert(!disconnected);
}

void test_client_network_filters_unknown_messages_and_routes_sends()
{
    network::NetworkServer server;
    assert(!server.start(0));

    rtype::client::ClientNetwork client;
    assert(!client.connect("127.0.0.1", server.localPort(), "Bob"));
    assert(waitForConnection(client));

    bool sawSnapshot = false;
    client.registerHandler(protocol::MessageType::Snapshot,
                           [&](std::span<const std::uint8_t>) { sawSnapshot = true; });

    network::PeerId serverPeer = network::NoPeer;
    std::string serverName;
    assert(waitForConnected(server, serverPeer, serverName));

    server.send(serverPeer, static_cast<std::uint8_t>(protocol::MessageType::GameEvent), {1, 2, 3},
                false);
    client.update();
    assert(!sawSnapshot);

    client.sendReliable(protocol::MessageType::Input, {0x5A});
    client.sendUnreliable(protocol::MessageType::Input, {0x3C});

    const auto deadline = std::chrono::steady_clock::now() + 2s;
    bool inputSeen = false;
    while (std::chrono::steady_clock::now() < deadline) {
        for (const auto& event : server.poll()) {
            if (const auto* msg = std::get_if<network::MessageReceived>(&event)) {
                if (msg->type == static_cast<std::uint8_t>(protocol::MessageType::Input)) {
                    const std::vector<std::uint8_t> expectedInput{0x5A};
                    const std::vector<std::uint8_t> expectedInput2{0x3C};
                    assert(msg->payload == expectedInput || msg->payload == expectedInput2);
                    inputSeen = true;
                    break;
                }
            }
        }
        if (inputSeen)
            break;
        std::this_thread::sleep_for(5ms);
    }
    assert(inputSeen);
}

} // namespace

int main()
{
    test_connect_and_dispatch_game_message();
    test_client_network_filters_unknown_messages_and_routes_sends();
    return 0;
}
