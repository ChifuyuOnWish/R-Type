#pragma once

#include "NetworkConfig.hpp"

#include <array>
#include <asio/io_context.hpp>
#include <asio/ip/udp.hpp>
#include <cstdint>
#include <functional>
#include <span>
#include <system_error>

namespace rtype::network {

class UdpSocket
{
public:
    using Endpoint = asio::ip::udp::endpoint;
    using ReceiveHandler = std::function<void(const Endpoint&, std::span<const std::uint8_t>)>;
    using ErrorHandler = std::function<void(const std::error_code&)>;

    explicit UdpSocket(asio::io_context& io);
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket(UdpSocket&&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;
    UdpSocket& operator=(UdpSocket&&) = delete;
    ~UdpSocket();

    [[nodiscard]] std::error_code open(std::uint16_t port);
    [[nodiscard]] std::uint16_t localPort() const;
    void startReceive(ReceiveHandler onReceive, ErrorHandler onError);
    std::error_code sendTo(const Endpoint& to, std::span<const std::uint8_t> bytes);
    void close();
    [[nodiscard]] bool isOpen() const;

private:
    void doReceive();

    asio::ip::udp::socket _socket;
    std::array<std::uint8_t, MaxDatagramSize + 1> _buffer{};
    Endpoint _sender;
    ReceiveHandler _onReceive;
    ErrorHandler _onError;
};

} // namespace rtype::network
