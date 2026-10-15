#include "UdpSocket.hpp"

namespace rtype::network {

UdpSocket::UdpSocket(asio::io_context& io) : _socket(io) {}

UdpSocket::~UdpSocket()
{
    close();
}

std::error_code UdpSocket::open(std::uint16_t port)
{
    std::error_code ec;

    _socket.open(asio::ip::udp::v4(), ec);
    if (ec)
        return ec;
    _socket.set_option(asio::socket_base::reuse_address(true), ec);
    _socket.bind(Endpoint(asio::ip::udp::v4(), port), ec);
    if (ec) {
        close();
        return ec;
    }
    return {};
}

std::uint16_t UdpSocket::localPort() const
{
    std::error_code ec;
    auto ep = _socket.local_endpoint(ec);
    return ec ? 0 : ep.port();
}

void UdpSocket::startReceive(ReceiveHandler onReceive, ErrorHandler onError)
{
    _onReceive = std::move(onReceive);
    _onError = std::move(onError);
    doReceive();
}

std::error_code UdpSocket::sendTo(const Endpoint& to, std::span<const std::uint8_t> bytes)
{
    if (bytes.size() > MaxDatagramSize) {
        return std::make_error_code(std::errc::message_size);
    }

    std::error_code ec;
    _socket.send_to(asio::buffer(bytes.data(), bytes.size()), to, 0, ec);
    return ec;
}

void UdpSocket::close()
{
    if (!_socket.is_open()) {
        return;
    }

    std::error_code ec;
    _socket.close(ec);
}

bool UdpSocket::isOpen() const
{
    return _socket.is_open();
}

void UdpSocket::doReceive()
{
    _socket.async_receive_from(
        asio::buffer(_buffer), _sender, [this](const std::error_code& ec, std::size_t n) {
            if (ec == asio::error::operation_aborted)
                return;
            if (ec) {
                if (ec == asio::error::connection_reset || ec == asio::error::connection_refused ||
                    ec == asio::error::message_size) {
                    doReceive();
                    return;
                }
                if (_onError)
                    _onError(ec);
                doReceive();
                return;
            }
            if (n > MaxDatagramSize) {
                doReceive();
                return;
            }
            if (_onReceive)
                _onReceive(_sender, std::span<const std::uint8_t>(_buffer.data(), n));
            doReceive();
        });
}

} // namespace rtype::network
