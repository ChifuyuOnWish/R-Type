#pragma once

#include "Connection.hpp"
#include "NetworkHost.hpp"
#include "PacketCodec.hpp"
#include "TransportMessages.hpp"

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace rtype::network {

enum class ClientState : std::uint8_t
{
    Disconnected,
    Connecting,
    Connected,
};

struct ClientConfig
{
    std::chrono::milliseconds connectRetryInterval = ConnectRetryInterval;
    unsigned maxConnectAttempts = MaxConnectAttempts;
};

class NetworkClient : public NetworkHost
{
public:
    explicit NetworkClient(ClientConfig config = {});
    ~NetworkClient() override;

    [[nodiscard]] std::error_code connect(const std::string& host, std::uint16_t port,
                                          std::string name);
    void disconnect();

    void send(std::uint8_t type, std::vector<std::uint8_t> payload, bool reliable);

    [[nodiscard]] ClientState state() const;
    [[nodiscard]] PeerId clientId() const;
    [[nodiscard]] std::uint8_t serverTickRate() const;

protected:
    void onDatagram(const Endpoint& from, const protocol::PacketHeader& header,
                    std::span<const std::uint8_t> body) override;
    void sendMessage(const OutgoingMessage& msg) override;
    void onTick(TimePoint now) override;
    void onStop() override;

private:
    struct PendingUnreliable
    {
        std::uint8_t type;
        std::vector<std::uint8_t> payload;
    };

    void beginConnect(const Endpoint& server, std::string name);
    void beginDisconnect();
    void tickConnecting(TimePoint now);
    void sendConnectRequest(TimePoint now);
    bool tryAccept(const protocol::PacketHeader& header, const std::vector<ParsedMessage>& msgs,
                   TimePoint now);
    bool tryReject(const protocol::PacketHeader& header, const std::vector<ParsedMessage>& msgs);
    bool processMessage(ParsedMessage& msg);
    void flush(TimePoint now, bool force = false);
    void fail(ConnectFailure reason);
    void end(DisconnectReason reason);

    ClientConfig _config;

    std::atomic<ClientState> _state{ClientState::Disconnected};
    std::atomic<PeerId> _clientId{NoPeer};
    std::atomic<std::uint8_t> _tickRate{0};
    std::atomic<bool> _started{false};

    std::optional<Connection> _conn;
    Endpoint _server;
    std::string _name;
    std::uint32_t _token = 0;
    unsigned _attempts = 0;
    TimePoint _nextRequest{};
    bool _disconnecting = false;
    bool _overflow = false;
    bool _ackDue = false;
    TimePoint _disconnectDeadline{};
    std::vector<PendingUnreliable> _unreliable;
};

} // namespace rtype::network