#include "ClientNetwork.hpp"

#include <utility>
#include <variant>

namespace rtype::client {

namespace {

constexpr std::uint8_t toRaw(protocol::MessageType type)
{
    return static_cast<std::uint8_t>(type);
}

} // namespace

ClientNetwork::ClientNetwork(network::ClientConfig config) : _client(config) {}

std::error_code ClientNetwork::connect(const std::string& host, std::uint16_t port,
                                       std::string name)
{
    return _client.connect(host, port, std::move(name));
}

void ClientNetwork::disconnect()
{
    _client.disconnect();
}

void ClientNetwork::registerHandler(protocol::MessageType type, MessageHandler fn)
{
    _handlers[toRaw(type)] = std::move(fn);
}

void ClientNetwork::sendUnreliable(protocol::MessageType type, std::vector<std::uint8_t> payload)
{
    _client.send(toRaw(type), std::move(payload), false);
}

void ClientNetwork::sendReliable(protocol::MessageType type, std::vector<std::uint8_t> payload)
{
    _client.send(toRaw(type), std::move(payload), true);
}

std::size_t ClientNetwork::update()
{
    const auto events = _client.poll();
    for (const auto& ev : events) {
        std::visit([this](const auto& e) { dispatch(e); }, ev);
    }
    return events.size();
}

void ClientNetwork::dispatch(const network::Connected& ev)
{
    if (_onConnected)
        _onConnected(ev.peerId, ev.name);
}

void ClientNetwork::dispatch(const network::ConnectionFailed& ev)
{
    if (_onConnectionFailed)
        _onConnectionFailed(ev.reason);
}

void ClientNetwork::dispatch(const network::Disconnected& ev)
{
    if (_onDisconnected)
        _onDisconnected(ev.peerId, ev.reason);
}

void ClientNetwork::dispatch(const network::MessageReceived& ev)
{
    const auto it = _handlers.find(ev.type);
    if (it == _handlers.end() || !it->second)
        return;
    it->second(std::span<const std::uint8_t>(ev.payload.data(), ev.payload.size()));
}

} // namespace rtype::client