#include "NetworkHost.hpp"

#include <asio/post.hpp>
#include <iostream>

namespace rtype::network {

std::error_code NetworkHost::start(std::uint16_t port)
{
    if (_running.exchange(true))
        return std::make_error_code(std::errc::already_connected);
    _io.restart();
    if (auto ec = _socket.open(port)) {
        _running = false;
        return ec;
    }
    _socket.startReceive(
        [this](const Endpoint& from, std::span<const std::uint8_t> b) { handleDatagram(from, b); },
        [](const std::error_code& ec) {
            std::cerr << "NetworkHost: receive error: " << ec.message() << '\n';
        });
    scheduleTick();
    _thread = std::thread([this] { _io.run(); });
    return {};
}

void NetworkHost::stop()
{
    if (!_running.exchange(false))
        return;
    asio::post(_io, [this] {
        onStop();
        _timer.cancel();
        _socket.close();
    });
    if (_thread.joinable())
        _thread.join();
}

NetworkHost::~NetworkHost()
{
    stop();
}

std::vector<NetworkEvent> NetworkHost::poll()
{
    return _incoming.drain();
}

std::uint64_t NetworkHost::droppedPackets() const
{
    return _dropped.load();
}

std::uint16_t NetworkHost::localPort() const
{
    return _socket.localPort();
}

void NetworkHost::handleDatagram(const Endpoint& from, std::span<const std::uint8_t> bytes)
{
    if (bytes.size() < HeaderSize) {
        countDropped();
        return;
    }
    // TODO(#6): decode + validate PacketHeader here
    onDatagram(from, bytes);
}

void NetworkHost::scheduleTick()
{
    _timer.expires_after(NetworkTickInterval);
    _timer.async_wait([this](const std::error_code& ec) {
        if (ec == asio::error::operation_aborted)
            return;
        for (auto& msg : _outgoing.drain())
            sendMessage(msg);
        onTick(std::chrono::steady_clock::now());
        scheduleTick();
    });
}

void NetworkHost::pushEvent(NetworkEvent ev)
{
    _incoming.push(std::move(ev));
}

std::error_code NetworkHost::sendRaw(const Endpoint& to, std::span<const std::uint8_t> bytes)
{
    return _socket.sendTo(to, bytes);
}

void NetworkHost::countDropped()
{
    _dropped++;
}

void NetworkHost::enqueueOutgoing(OutgoingMessage msg)
{
    _outgoing.push(std::move(msg));
}

} // namespace rtype::network
