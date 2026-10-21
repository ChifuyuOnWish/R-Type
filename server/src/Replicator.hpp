#pragma once

#include "ByteWriter.hpp"
#include "Messages.hpp"
#include "NetworkServer.hpp"
#include "Registry.hpp"
#include "ServerComponents.hpp"

#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <vector>

namespace rtype::server {

namespace detail {

template <typename M> std::optional<std::vector<std::uint8_t>> encode(const M& msg)
{
    protocol::ByteWriter w;
    if (!msg.serialize(w)) {
        std::cerr << "server: refusing to send an invalid message\n";
        return std::nullopt;
    }
    return w.data();
}

} // namespace detail

class Replicator
{
public:
    Replicator(network::NetworkServer& net, engine::Registry& registry);

    template <typename M>
    void broadcast(const M& msg, bool reliable, network::PeerId except = network::NoPeer)
    {
        if (auto bytes = detail::encode(msg))
            _net.broadcast(static_cast<std::uint8_t>(M::Type), std::move(*bytes), reliable, except);
    }

    template <typename M> void sendTo(network::PeerId peer, const M& msg, bool reliable)
    {
        if (auto bytes = detail::encode(msg))
            _net.send(peer, static_cast<std::uint8_t>(M::Type), std::move(*bytes), reliable);
    }

    void sync();
    void sendEvents(std::vector<protocol::GameEventMessage>& events);
    void afterSteps(unsigned steps, std::uint32_t serverTick);
    void sendWorldTo(network::PeerId peer);
    void forget(engine::Entity entity);
    std::uint32_t wireId(engine::Entity entity) const;

private:
    struct Synced
    {
        engine::Entity entity;
        std::uint16_t type;
        float x;
        float y;
    };

    void sendSnapshot(std::uint32_t serverTick);

    network::NetworkServer& _net;
    engine::Registry& _registry;
    std::map<std::uint32_t, Synced> _synced;
    std::uint32_t _nextNetId = 1;
    unsigned _stepsSinceSnapshot = 0;
};

} // namespace rtype::server
