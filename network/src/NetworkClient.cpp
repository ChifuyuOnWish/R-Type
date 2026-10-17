#include "NetworkClient.hpp"

#include "ByteReader.hpp"

#include <asio/io_context.hpp>
#include <asio/ip/udp.hpp>

namespace rtype::network {

using protocol::ByteReader;
using protocol::ByteWriter;
using protocol::PacketHeader;

namespace {

constexpr std::uint8_t transport(TransportMessage m)
{
    return static_cast<std::uint8_t>(m);
}

DisconnectReason disconnectReason(const std::vector<std::uint8_t>& payload)
{
    if (payload.empty())
        return DisconnectReason::Quit;
    switch (payload[0]) {
    case 1:
        return DisconnectReason::Timeout;
    case 2:
        return DisconnectReason::Kicked;
    default:
        return DisconnectReason::Quit;
    }
}

} // namespace

NetworkClient::NetworkClient(ClientConfig config) : _config(config) {}

NetworkClient::~NetworkClient()
{
    stop();
}

std::error_code NetworkClient::connect(const std::string& host, std::uint16_t port,
                                       std::string name)
{
    if (_state.load() != ClientState::Disconnected)
        return std::make_error_code(std::errc::already_connected);
    if (name.empty() || name.size() > MaxNameLength)
        return std::make_error_code(std::errc::invalid_argument);

    std::error_code ec;
    asio::io_context io;
    asio::ip::udp::resolver resolver(io);
    const auto results = resolver.resolve(asio::ip::udp::v4(), host, std::to_string(port), ec);
    if (ec)
        return ec;
    if (results.empty())
        return std::make_error_code(std::errc::host_unreachable);
    const Endpoint server = results.begin()->endpoint();

    if (!_started.exchange(true)) {
        ec = start(0);
        if (ec) {
            _started = false;
            return ec;
        }
    }
    _state = ClientState::Connecting;
    runOnIoThread([this, server, name = std::move(name)]() mutable {
        beginConnect(server, std::move(name));
    });
    return {};
}

void NetworkClient::disconnect()
{
    runOnIoThread([this] { beginDisconnect(); });
}

void NetworkClient::send(std::uint8_t type, std::vector<std::uint8_t> payload, bool reliable)
{
    enqueueOutgoing({NoPeer, type, std::move(payload), reliable, NoPeer});
}

ClientState NetworkClient::state() const
{
    return _state.load();
}

PeerId NetworkClient::clientId() const
{
    return _clientId.load();
}

std::uint8_t NetworkClient::serverTickRate() const
{
    return _tickRate.load();
}

void NetworkClient::beginConnect(const Endpoint& server, std::string name)
{
    const auto now = std::chrono::steady_clock::now();
    _server = server;
    _name = std::move(name);
    _token = 0;
    _attempts = 0;
    _disconnecting = false;
    _overflow = false;
    _ackDue = false;
    _unreliable.clear();
    _conn.emplace(now);
    _nextRequest = now;
    tickConnecting(now);
}

void NetworkClient::beginDisconnect()
{
    if (!_conn || _disconnecting)
        return;
    if (state() == ClientState::Connecting) {
        end(DisconnectReason::Quit);
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if (!_conn->queueReliable(transport(TransportMessage::Disconnect), {0}, now)) {
        end(DisconnectReason::Quit);
        return;
    }
    _disconnecting = true;
    _disconnectDeadline = now + MaxDisconnectAttempts * ReliableResendInterval;
    _unreliable.clear();
    flush(now, true);
}

void NetworkClient::tickConnecting(TimePoint now)
{
    if (now < _nextRequest)
        return;
    if (_attempts >= _config.maxConnectAttempts) {
        fail(ConnectFailure::Timeout);
        return;
    }
    sendConnectRequest(now);
    ++_attempts;
    _nextRequest = now + _config.connectRetryInterval;
}

void NetworkClient::sendConnectRequest(TimePoint now)
{
    const auto f = _conn->prepareSend(now);
    PacketHeader header;
    header.session = 0;
    header.sequence = f.sequence;
    header.ack = f.ack;
    header.ackBits = f.ackBits;

    ByteWriter writer;
    header.encode(writer);
    ByteWriter payload;
    (void)payload.writeString(_name);
    writeMessage(writer, transport(TransportMessage::ConnectRequest), false, 0, payload.data());
    (void)sendRaw(_server, writer.data());
}

bool NetworkClient::tryAccept(const PacketHeader& header, const std::vector<ParsedMessage>& msgs,
                              TimePoint)
{
    if (header.session == 0)
        return false;
    for (const auto& msg : msgs) {
        if (msg.type != transport(TransportMessage::ConnectAccept) || !msg.reliable)
            continue;
        ByteReader r(msg.payload.data(), msg.payload.size());
        const auto id = r.readU8();
        const auto token = r.readU32();
        const auto tickRate = r.readU8();
        if (!id || !token || !tickRate || r.remaining() != 0 || *token != header.session)
            return false;
        _token = *token;
        _clientId = *id;
        _tickRate = *tickRate;
        _state = ClientState::Connected;
        pushEvent(Connected{*id, _name});
        return true;
    }
    return false;
}

bool NetworkClient::tryReject(const PacketHeader& header, const std::vector<ParsedMessage>& msgs)
{
    if (header.session != 0)
        return false;
    for (const auto& msg : msgs) {
        if (msg.type != transport(TransportMessage::ConnectReject) || msg.reliable ||
            msg.payload.size() != 1)
            continue;
        switch (msg.payload[0]) {
        case static_cast<std::uint8_t>(RejectReason::Full):
            fail(ConnectFailure::Full);
            return true;
        case static_cast<std::uint8_t>(RejectReason::BadVersion):
            fail(ConnectFailure::BadVersion);
            return true;
        case static_cast<std::uint8_t>(RejectReason::Banned):
            fail(ConnectFailure::Banned);
            return true;
        default:
            return false;
        }
    }
    return false;
}

void NetworkClient::onDatagram(const Endpoint& from, const PacketHeader& header,
                               std::span<const std::uint8_t> body)
{
    if (!_conn || from != _server) {
        countDropped();
        return;
    }
    auto messages = parseMessages(body);
    if (!messages) {
        countDropped();
        return;
    }
    const auto now = std::chrono::steady_clock::now();

    if (state() == ClientState::Connecting) {
        if (tryReject(header, *messages))
            return;
        if (!tryAccept(header, *messages, now)) {
            countDropped();
            return;
        }
    }

    if (header.session != _token) {
        countDropped();
        return;
    }
    std::vector<std::uint16_t> acked;
    if (_conn->onPacketReceived(header.sequence, header.ack, header.ackBits, now, acked) !=
        Connection::Receive::New)
        return;

    for (auto& msg : *messages) {
        if (msg.reliable)
            _ackDue = true;
        if (!processMessage(msg))
            return;
    }
}

bool NetworkClient::processMessage(ParsedMessage& msg)
{
    if (msg.reliable && !_conn->acceptReliable(msg.reliableId))
        return true;

    if (msg.type == transport(TransportMessage::Disconnect)) {
        end(disconnectReason(msg.payload));
        return false;
    }
    if (msg.type == transport(TransportMessage::Ping)) {
        if (msg.payload.size() == 4 && !_disconnecting)
            _unreliable.push_back({transport(TransportMessage::Pong), std::move(msg.payload)});
        return true;
    }
    if (msg.type >= FirstGameMessage && !_disconnecting)
        pushEvent(MessageReceived{NoPeer, msg.type, std::move(msg.payload)});
    return true;
}

void NetworkClient::sendMessage(const OutgoingMessage& msg)
{
    if (!_conn || state() != ClientState::Connected || _disconnecting ||
        msg.type < FirstGameMessage)
        return;
    const auto now = std::chrono::steady_clock::now();
    if (msg.reliable) {
        if (!_conn->queueReliable(msg.type, msg.payload, now))
            _overflow = true;
        return;
    }
    if (messageSize(msg.payload.size(), false) > MaxDatagramSize - HeaderSize)
        return;
    _unreliable.push_back({msg.type, msg.payload});
}

void NetworkClient::onTick(TimePoint now)
{
    if (!_conn)
        return;
    if (state() == ClientState::Connecting) {
        tickConnecting(now);
        return;
    }

    const bool dead = _overflow || _conn->isDead(now);
    if (_disconnecting) {
        if (dead || _conn->pendingReliable() == 0 || now >= _disconnectDeadline) {
            end(DisconnectReason::Quit);
            return;
        }
    } else if (dead) {
        end(DisconnectReason::Timeout);
        return;
    }
    flush(now, _ackDue);
    _ackDue = false;
}

void NetworkClient::flush(TimePoint now, bool force)
{
    if (!_conn)
        return;
    bool first = true;
    while ((force && first) || !_unreliable.empty() || _conn->hasDueReliable(now) ||
           _conn->needsHeartbeat(now)) {
        first = false;
        ByteWriter writer;
        const auto f = _conn->prepareSend(now);
        PacketHeader header;
        header.session = _token;
        header.sequence = f.sequence;
        header.ack = f.ack;
        header.ackBits = f.ackBits;
        header.encode(writer);
        bool wroteSomething = false;

        for (const auto* msg :
             _conn->takeDueReliable(f.sequence, now, MaxDatagramSize - writer.size())) {
            writeMessage(writer, msg->type, true, msg->id, msg->payload);
            wroteSomething = true;
        }
        std::vector<PendingUnreliable> later;
        for (auto& umsg : _unreliable) {
            if (messageSize(umsg.payload.size(), false) <= MaxDatagramSize - writer.size()) {
                writeMessage(writer, umsg.type, false, 0, umsg.payload);
                wroteSomething = true;
            } else {
                later.push_back(std::move(umsg));
            }
        }
        _unreliable = std::move(later);

        if (!wroteSomething) {
            const auto ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
                    .count();
            ByteWriter ping;
            ping.writeU32(static_cast<std::uint32_t>(ms));
            writeMessage(writer, transport(TransportMessage::Ping), false, 0, ping.data());
        }
        (void)sendRaw(_server, writer.data());
    }
}

void NetworkClient::fail(ConnectFailure reason)
{
    _conn.reset();
    _state = ClientState::Disconnected;
    pushEvent(ConnectionFailed{reason});
}

void NetworkClient::end(DisconnectReason reason)
{
    const PeerId id = _clientId.exchange(NoPeer);
    _conn.reset();
    _unreliable.clear();
    _disconnecting = false;
    _overflow = false;
    _state = ClientState::Disconnected;
    pushEvent(Disconnected{id, reason});
}

void NetworkClient::onStop()
{
    if (_conn && state() == ClientState::Connected && !_disconnecting) {
        const auto now = std::chrono::steady_clock::now();
        if (_conn->queueReliable(transport(TransportMessage::Disconnect), {0}, now))
            flush(now, true);
    }
    _conn.reset();
    _unreliable.clear();
    _disconnecting = false;
    _state = ClientState::Disconnected;
    _clientId = NoPeer;
    _started = false;
}

} // namespace rtype::network