#pragma once

#include "NetworkConfig.hpp"
#include "NetworkEvent.hpp"
#include "PacketHeader.hpp"
#include "ThreadSafeQueue.hpp"
#include "UdpSocket.hpp"

#include <asio/steady_timer.hpp>
#include <atomic>
#include <chrono>
#include <functional>
#include <span>
#include <system_error>
#include <thread>
#include <vector>

namespace rtype::network {

class NetworkHost
{
public:
    using Endpoint = UdpSocket::Endpoint;
    using TimePoint = std::chrono::steady_clock::time_point;

    NetworkHost() = default;
    NetworkHost(const NetworkHost&) = delete;
    NetworkHost(NetworkHost&&) = delete;
    NetworkHost& operator=(const NetworkHost&) = delete;
    NetworkHost& operator=(NetworkHost&&) = delete;
    virtual ~NetworkHost();

    [[nodiscard]] std::error_code start(std::uint16_t port);
    void stop();
    [[nodiscard]] std::vector<NetworkEvent> poll();
    [[nodiscard]] std::uint64_t droppedPackets() const;
    [[nodiscard]] std::uint16_t localPort() const;

protected:
    virtual void onDatagram(const Endpoint& from, const protocol::PacketHeader& header,
                            std::span<const std::uint8_t> body) = 0;
    virtual void sendMessage(const OutgoingMessage& msg) = 0;
    virtual void onTick(TimePoint) {}
    virtual void onStop() {}

    void pushEvent(NetworkEvent ev);
    std::error_code sendRaw(const Endpoint& to, std::span<const std::uint8_t> bytes);
    void countDropped();
    void enqueueOutgoing(OutgoingMessage msg);

    virtual void onBadVersion(const Endpoint&, std::span<const std::uint8_t>) {}
    void runOnIoThread(std::function<void()> fn);

private:
    void handleDatagram(const Endpoint& from, std::span<const std::uint8_t> bytes);
    void scheduleTick();

    asio::io_context _io;
    UdpSocket _socket{_io};
    asio::steady_timer _timer{_io};
    std::thread _thread;
    ThreadSafeQueue<NetworkEvent> _incoming;
    ThreadSafeQueue<OutgoingMessage> _outgoing;
    std::atomic<std::uint64_t> _dropped{0};
    std::atomic<bool> _running{false};
};

} // namespace rtype::network