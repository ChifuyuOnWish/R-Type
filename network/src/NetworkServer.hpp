#pragma once

#include "Connection.hpp"
#include "NetworkHost.hpp"
#include "PacketCodec.hpp"
#include "TransportMessages.hpp"

#include <map>
#include <random>
#include <string>

namespace rtype::network {

class NetworkServer : public NetworkHost
{
public:
    NetworkServer() = default;
    ~NetworkServer() override;

    void send(PeerId peer, std::uint8_t type, std::vector<std::uint8_t> payload, bool reliable);
    void broadcast(std::uint8_t type, std::vector<std::uint8_t> payload, bool reliable,
                   PeerId except = NoPeer);
    void kick(PeerId peer);

protected:
    void onDatagram(const Endpoint& from, const protocol::PacketHeader& header,
                    std::span<const std::uint8_t> body) override;
    void onBadVersion(const Endpoint& from, std::span<const std::uint8_t> bytes) override;
    void sendMessage(const OutgoingMessage& msg) override;
    void onTick(TimePoint now) override;
    void onStop() override;

private:
    struct PendingUnreliable
    {
        std::uint8_t type;
        std::vector<std::uint8_t> payload;
    };

    struct ClientSession
    {
        ClientSession(PeerId id_, Endpoint endpoint_, std::uint32_t token_, std::string name_,
                      TimePoint now)
            : id(id_), endpoint(endpoint_), token(token_), name(std::move(name_)), conn(now)
        {
        }
        PeerId id;
        Endpoint endpoint;
        std::uint32_t token;
        std::string name;
        Connection conn;
        std::vector<PendingUnreliable> unreliable;
        bool overflow = false;
    };

    void handleConnectRequest(const Endpoint& from, const protocol::PacketHeader& header,
                              const ParsedMessage& msg, TimePoint now);
    void queueFor(ClientSession& s, std::uint8_t type, std::vector<std::uint8_t> payload,
                  bool reliable, TimePoint now);
    void flush(ClientSession& s, TimePoint now);
    void sendReject(const Endpoint& to, RejectReason reason);
    void removeSession(PeerId id, DisconnectReason reason);
    void kickNow(PeerId id);
    PeerId freeId() const;

    std::map<PeerId, ClientSession> _sessions;
    std::map<Endpoint, PeerId> _byEndpoint;
    std::mt19937 _rng{std::random_device{}()};
};

} // namespace rtype::network
