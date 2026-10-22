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

class ClientNetwork
{
public:
    using ConnectedHandler = std::function<void(network::PeerId id, const std::string& name)>;
    using ConnectionFailedHandler = std::function<void(network::ConnectFailure reason)>;
    using DisconnectedHandler =
        std::function<void(network::PeerId id, network::DisconnectReason reason)>;
    using MessageHandler = std::function<void(std::span<const std::uint8_t> payload)>;

    explicit ClientNetwork(network::ClientConfig config = {});
    ~ClientNetwork() = default;

    ClientNetwork(const ClientNetwork&) = delete;
    ClientNetwork& operator=(const ClientNetwork&) = delete;

    [[nodiscard]] std::error_code connect(const std::string& host, std::uint16_t port,
                                          std::string name);

    void disconnect();

    std::size_t update();

    void onConnected(ConnectedHandler fn) { _onConnected = std::move(fn); }
    void onConnectionFailed(ConnectionFailedHandler fn) { _onConnectionFailed = std::move(fn); }
    void onDisconnected(DisconnectedHandler fn) { _onDisconnected = std::move(fn); }

    void registerHandler(protocol::MessageType type, MessageHandler fn);

    void sendUnreliable(protocol::MessageType type, std::vector<std::uint8_t> payload);
    void sendReliable(protocol::MessageType type, std::vector<std::uint8_t> payload);

    [[nodiscard]] network::ClientState state() const { return _client.state(); }
    [[nodiscard]] bool isConnected() const { return state() == network::ClientState::Connected; }
    [[nodiscard]] network::PeerId clientId() const { return _client.clientId(); }
    [[nodiscard]] std::uint8_t serverTickRate() const { return _client.serverTickRate(); }
    [[nodiscard]] std::uint64_t droppedPackets() const { return _client.droppedPackets(); }
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
