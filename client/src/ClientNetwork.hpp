#pragma once

#include "MessageType.hpp"
#include "NetworkClient.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace rtype::client {

/// Glue between the client game loop and network::NetworkClient (issue #16).
///
/// NetworkClient runs on its own io thread and queues NetworkEvents.
/// ClientNetwork::update() must be called once per frame, on the game
/// thread: it drains that queue and dispatches each event to the callbacks
/// registered here, so game code never touches the network thread.
class ClientNetwork
{
public:
    using ConnectedHandler = std::function<void(network::PeerId id, const std::string& name)>;
    using ConnectionFailedHandler = std::function<void(network::ConnectFailure reason)>;
    using DisconnectedHandler =
        std::function<void(network::PeerId id, network::DisconnectReason reason)>;
    using MessageHandler = std::function<void(std::span<const std::uint8_t> payload)>;

    explicit ClientNetwork(network::ClientConfig config = {});
    ~ClientNetwork() = default; // ~NetworkClient() stops the io thread and sends Disconnect

    ClientNetwork(const ClientNetwork&) = delete;
    ClientNetwork& operator=(const ClientNetwork&) = delete;

    /// Start connecting (non-blocking). The outcome arrives through
    /// onConnected / onConnectionFailed during a later update().
    [[nodiscard]] std::error_code connect(const std::string& host, std::uint16_t port,
                                          std::string name);

    /// Ask for a graceful disconnect. onDisconnected fires once it is done.
    void disconnect();

    /// Drain pending network events and dispatch them. Call once per frame
    /// on the game thread. Returns the number of events processed.
    std::size_t update();

    // --- callbacks (set them before connect(); do not change them from inside a handler) ---
    void onConnected(ConnectedHandler fn) { _onConnected = std::move(fn); }
    void onConnectionFailed(ConnectionFailedHandler fn) { _onConnectionFailed = std::move(fn); }
    void onDisconnected(DisconnectedHandler fn) { _onDisconnected = std::move(fn); }

    /// Handler for one server->client game message. Registering the same type
    /// twice replaces the first handler.
    void registerHandler(protocol::MessageType type, MessageHandler fn);

    // --- sending (dropped silently while not connected) ---
    /// Unreliable: per-frame data such as Input. May be lost.
    void sendUnreliable(protocol::MessageType type, std::vector<std::uint8_t> payload);
    /// Reliable: events that must arrive.
    void sendReliable(protocol::MessageType type, std::vector<std::uint8_t> payload);

    // --- state ---
    [[nodiscard]] network::ClientState state() const { return _client.state(); }
    [[nodiscard]] bool isConnected() const { return state() == network::ClientState::Connected; }
    [[nodiscard]] network::PeerId clientId() const { return _client.clientId(); }
    [[nodiscard]] std::uint8_t serverTickRate() const { return _client.serverTickRate(); }
    [[nodiscard]] std::uint64_t droppedPackets() const { return _client.droppedPackets(); }

    /// Raw client, for tests and debug tools.
    [[nodiscard]] network::NetworkClient& raw() { return _client; }

private:
    void dispatch(const network::Connected& ev);
    void dispatch(const network::ConnectionFailed& ev);
    void dispatch(const network::Disconnected& ev);
    void dispatch(const network::MessageReceived& ev);

    network::NetworkClient _client;
    ConnectedHandler _onConnected;
    ConnectionFailedHandler _onConnectionFailed;
    DisconnectedHandler _onDisconnected;
    std::unordered_map<std::uint8_t, MessageHandler> _handlers;
};

} // namespace rtype::client