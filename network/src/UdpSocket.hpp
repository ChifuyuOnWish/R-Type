#pragma one

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

private:
};

} // namespace rtype::network