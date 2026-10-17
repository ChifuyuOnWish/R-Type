// Loopback tests for NetworkClient against a real NetworkServer, and against a
// hand-written fake server for the failure paths (plain asserts).
#undef NDEBUG
#include "ByteReader.hpp"
#include "NetworkClient.hpp"
#include "NetworkServer.hpp"

#include <algorithm>
#include <asio/io_context.hpp>
#include <asio/ip/udp.hpp>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <variant>
#include <vector>

using namespace rtype::network;
namespace protocol = rtype::protocol;
using namespace std::chrono_literals;
using protocol::ByteWriter;
using protocol::PacketHeader;
using Bytes = std::vector<std::uint8_t>;
using Log = std::vector<NetworkEvent>;

namespace {

template <typename T>
const T* waitFor(NetworkHost& host, Log& log, std::chrono::milliseconds timeout = 3000ms)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;) {
        for (auto& ev : host.poll())
            log.push_back(std::move(ev));
        for (const auto& ev : log) {
            if (const auto* found = std::get_if<T>(&ev))
                return found;
        }
        if (std::chrono::steady_clock::now() >= deadline)
            return nullptr;
        std::this_thread::sleep_for(2ms);
    }
}

template <typename T> bool has(const Log& log)
{
    return std::any_of(log.begin(), log.end(),
                       [](const NetworkEvent& ev) { return std::holds_alternative<T>(ev); });
}

struct FakeServer
{
    asio::io_context io;
    asio::ip::udp::socket socket{io, asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 0)};

    FakeServer() { socket.non_blocking(true); }
    std::uint16_t port() const { return socket.local_endpoint().port(); }

    // Returns false when nothing is waiting.
    bool receive(Bytes& out, asio::ip::udp::endpoint& from)
    {
        std::uint8_t buffer[MaxDatagramSize + 1];
        std::error_code ec;
        const std::size_t n = socket.receive_from(asio::buffer(buffer), from, 0, ec);
        if (ec)
            return false;
        out.assign(buffer, buffer + n);
        return true;
    }
};

bool isConnectRequest(const Bytes& datagram, const std::string& name)
{
    PacketHeader header;
    if (PacketHeader::decode(datagram.data(), datagram.size(), header) !=
        protocol::HeaderStatus::Ok)
        return false;
    if (header.session != 0)
        return false;
    auto msgs = parseMessages(std::span<const std::uint8_t>(datagram).subspan(HeaderSize));
    if (!msgs || msgs->size() != 1)
        return false;
    const auto& m = (*msgs)[0];
    if (m.type != static_cast<std::uint8_t>(TransportMessage::ConnectRequest) || m.reliable)
        return false;
    protocol::ByteReader r(m.payload.data(), m.payload.size());
    const auto got = r.readString();
    return got && *got == name && r.remaining() == 0;
}

void test_connect_and_exchange()
{
    NetworkServer server;
    const auto sec = server.start(0);
    assert(!sec);

    NetworkClient client;
    assert(client.state() == ClientState::Disconnected);
    assert(client.connect("127.0.0.1", server.localPort(), "Alice") == std::error_code{});
    assert(client.state() == ClientState::Connecting);

    Log clientLog;
    Log serverLog;
    const auto* connected = waitFor<Connected>(client, clientLog);
    assert(connected);
    assert(connected->name == "Alice");
    assert(client.state() == ClientState::Connected);
    assert(client.clientId() == connected->peerId && client.clientId() != NoPeer);
    assert(client.serverTickRate() == ServerTickRate);

    const auto* joined = waitFor<Connected>(server, serverLog);
    assert(joined && joined->name == "Alice" && joined->peerId == client.clientId());
    const PeerId peer = joined->peerId;

    client.send(0x10, {1, 2, 3}, true);
    const auto* up = waitFor<MessageReceived>(server, serverLog);
    assert(up && up->peerId == peer && up->type == 0x10 && up->payload == (Bytes{1, 2, 3}));

    server.send(peer, 0x15, {9, 8}, true);
    const auto* down = waitFor<MessageReceived>(client, clientLog);
    assert(down && down->peerId == NoPeer && down->type == 0x15 && down->payload == (Bytes{9, 8}));

    std::this_thread::sleep_for(1500ms);
    assert(client.state() == ClientState::Connected);
    assert(!has<Disconnected>(clientLog) && !has<Disconnected>(serverLog));
}

void test_disconnect_then_reconnect()
{
    NetworkServer server;
    assert(!server.start(0));
    NetworkClient client;
    Log clientLog;
    Log serverLog;

    assert(!client.connect("127.0.0.1", server.localPort(), "Bob"));
    assert(waitFor<Connected>(client, clientLog));
    const auto* joined = waitFor<Connected>(server, serverLog);
    assert(joined);
    const PeerId peer = joined->peerId;
    const PeerId id = client.clientId();

    assert(client.connect("127.0.0.1", server.localPort(), "Bob") ==
           std::make_error_code(std::errc::already_connected));

    client.disconnect();
    const auto* left = waitFor<Disconnected>(server, serverLog);
    assert(left && left->peerId == peer && left->reason == DisconnectReason::Quit);
    const auto* gone = waitFor<Disconnected>(client, clientLog);
    assert(gone && gone->peerId == id && gone->reason == DisconnectReason::Quit);
    assert(client.state() == ClientState::Disconnected);
    assert(client.clientId() == NoPeer);

    clientLog.clear();
    serverLog.clear();
    assert(!client.connect("127.0.0.1", server.localPort(), "Bob"));
    assert(waitFor<Connected>(client, clientLog));
    assert(waitFor<Connected>(server, serverLog));
    assert(client.state() == ClientState::Connected);
}

void test_rejected_when_full()
{
    NetworkServer server;
    assert(!server.start(0));

    std::vector<std::unique_ptr<NetworkClient>> clients;
    std::vector<Log> logs(MaxPlayers + 1);
    for (std::size_t i = 0; i < MaxPlayers; ++i) {
        clients.push_back(std::make_unique<NetworkClient>());
        assert(!clients[i]->connect("127.0.0.1", server.localPort(), "P" + std::to_string(i)));
        assert(waitFor<Connected>(*clients[i], logs[i]));
    }

    NetworkClient extra;
    assert(!extra.connect("127.0.0.1", server.localPort(), "Late"));
    const auto* failed = waitFor<ConnectionFailed>(extra, logs[MaxPlayers]);
    assert(failed && failed->reason == ConnectFailure::Full);
    assert(extra.state() == ClientState::Disconnected);
    assert(!has<Connected>(logs[MaxPlayers]));
}

void test_kicked_by_server()
{
    NetworkServer server;
    assert(!server.start(0));
    NetworkClient client;
    Log clientLog;
    Log serverLog;
    assert(!client.connect("127.0.0.1", server.localPort(), "Eve"));
    assert(waitFor<Connected>(client, clientLog));
    const auto* joined = waitFor<Connected>(server, serverLog);
    assert(joined);

    server.kick(joined->peerId);
    const auto* gone = waitFor<Disconnected>(client, clientLog);
    assert(gone && gone->reason == DisconnectReason::Kicked);
    assert(client.state() == ClientState::Disconnected);
}

void test_gives_up_after_retries()
{
    FakeServer fake;
    ClientConfig config;
    config.connectRetryInterval = 50ms;
    config.maxConnectAttempts = 3;
    NetworkClient client(config);
    Log log;
    assert(!client.connect("127.0.0.1", fake.port(), "Bob"));

    // Count what reaches the fake server until well after the client gave up.
    int requests = 0;
    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < 800ms) {
        Bytes datagram;
        asio::ip::udp::endpoint from;
        if (fake.receive(datagram, from)) {
            assert(isConnectRequest(datagram, "Bob"));
            ++requests;
        }
        std::this_thread::sleep_for(2ms);
    }
    assert(requests == 3);

    const auto* failed = waitFor<ConnectionFailed>(client, log, 500ms);
    assert(failed && failed->reason == ConnectFailure::Timeout);
    assert(client.state() == ClientState::Disconnected);
}

void test_rejected_by_fake_server()
{
    FakeServer fake;
    NetworkClient client;
    Log log;
    assert(!client.connect("127.0.0.1", fake.port(), "Bob"));

    Bytes datagram;
    asio::ip::udp::endpoint from;
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (!fake.receive(datagram, from)) {
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(2ms);
    }
    assert(isConnectRequest(datagram, "Bob"));

    auto reject = [&](RejectReason reason) {
        ByteWriter w;
        PacketHeader{}.encode(w);
        const std::uint8_t payload[1] = {static_cast<std::uint8_t>(reason)};
        writeMessage(w, static_cast<std::uint8_t>(TransportMessage::ConnectReject), false, 0,
                     payload);
        std::error_code ec;
        fake.socket.send_to(asio::buffer(w.data()), from, 0, ec);
        assert(!ec);
    };
    reject(RejectReason::BadVersion);

    const auto* failed = waitFor<ConnectionFailed>(client, log);
    assert(failed && failed->reason == ConnectFailure::BadVersion);
    assert(client.state() == ClientState::Disconnected);
}

void test_start_stop_roundtrip()
{
    for (int i = 0; i < 25; ++i) {
        NetworkServer server;
        assert(!server.start(0));
        std::this_thread::sleep_for(5ms);
        server.stop();
    }
}

void test_invalid_arguments()
{
    NetworkClient client;
    assert(client.connect("127.0.0.1", 4242, "") ==
           std::make_error_code(std::errc::invalid_argument));
    assert(client.connect("127.0.0.1", 4242, std::string(MaxNameLength + 1, 'x')) ==
           std::make_error_code(std::errc::invalid_argument));
    assert(client.state() == ClientState::Disconnected);
    // Nothing to disconnect from: must not crash or produce events.
    client.disconnect();
    std::this_thread::sleep_for(30ms);
    assert(client.poll().empty());
}

} // namespace

int main()
{
    test_start_stop_roundtrip();
    test_invalid_arguments();
    test_connect_and_exchange();
    test_disconnect_then_reconnect();
    test_rejected_when_full();
    test_kicked_by_server();
    test_gives_up_after_retries();
    test_rejected_by_fake_server();
    return 0;
}