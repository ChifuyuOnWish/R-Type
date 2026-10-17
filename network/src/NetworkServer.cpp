#include "NetworkServer.hpp"

#include "ByteReader.hpp"

namespace rtype::network {

using protocol::ByteWriter;
using protocol::PacketHeader;

NetworkServer::~NetworkServer()
{
    stop();
}

void NetworkServer::send(PeerId peer, std::uint8_t type, std::vector<std::uint8_t> payload,
                         bool reliable)
{
    enqueueOutgoing({peer, type, std::move(payload), reliable, NoPeer});
}

void NetworkServer::broadcast(std::uint8_t type, std::vector<std::uint8_t> payload, bool reliable,
                              PeerId except)
{
    enqueueOutgoing({AllPeers, type, std::move(payload), reliable, except});
}

void NetworkServer::kick(PeerId peer)
{
    runOnIoThread([this, peer] { kickNow(peer); });
}

void NetworkServer::onDatagram(const Endpoint& from, const PacketHeader& header,
                               std::span<const std::uint8_t> body)
{
    auto messages = parseMessages(body);
    if (!messages) {
        countDropped();
        return;
    }
    const auto now = std::chrono::steady_clock::now();

    auto found = _byEndpoint.find(from);
    if (found == _byEndpoint.end()) {
        if (header.session != 0 || messages->size() != 1 ||
            (*messages)[0].type != static_cast<std::uint8_t>(TransportMessage::ConnectRequest)) {
            countDropped();
            return;
        }
        handleConnectRequest(from, header, (*messages)[0], now);
        return;
    }

    ClientSession& session = _sessions.at(found->second);
    if (header.session != session.token) {
        if (header.session == 0)
            return;
        countDropped();
        return;
    }

    std::vector<std::uint16_t> acked;
    if (session.conn.onPacketReceived(header.sequence, header.ack, header.ackBits, now, acked) !=
        Connection::Receive::New)
        return;

    for (auto& msg : *messages) {
        if (msg.reliable && !session.conn.acceptReliable(msg.reliableId))
            continue;
        if (msg.type == static_cast<std::uint8_t>(TransportMessage::Disconnect)) {
            removeSession(session.id, DisconnectReason::Quit);
            return;
        }
        if (msg.type >= FirstGameMessage)
            pushEvent(MessageReceived{session.id, msg.type, std::move(msg.payload)});
    }
}

void NetworkServer::handleConnectRequest(const Endpoint& from, const PacketHeader& header,
                                         const ParsedMessage& msg, TimePoint now)
{
    protocol::ByteReader reader(msg.payload.data(), msg.payload.size());
    auto name = reader.readString();
    if (!name || name->size() > MaxNameLength || reader.remaining() != 0) {
        countDropped();
        return;
    }
    if (_sessions.size() >= MaxPlayers) {
        sendReject(from, RejectReason::Full);
        return;
    }

    PeerId id = freeId();
    std::uniform_int_distribution<std::uint32_t> dist(1, 0xFFFFFFFF);
    auto [it, inserted] = _sessions.try_emplace(id, id, from, dist(_rng), std::move(*name), now);
    ClientSession& session = it->second;
    _byEndpoint[from] = id;

    std::vector<std::uint16_t> acked;
    (void)session.conn.onPacketReceived(header.sequence, header.ack, header.ackBits, now, acked);

    ByteWriter writer;
    writer.writeU8(static_cast<std::uint8_t>(id));
    writer.writeU32(session.token);
    writer.writeU8(ServerTickRate);
    queueFor(session, static_cast<std::uint8_t>(TransportMessage::ConnectAccept), writer.data(),
             true, now);
    flush(session, now);
    pushEvent(Connected{id, session.name});
}

void NetworkServer::onBadVersion(const Endpoint& from, std::span<const std::uint8_t> bytes)
{
    if (bytes.size() > HeaderSize &&
        bytes[HeaderSize] == static_cast<std::uint8_t>(TransportMessage::ConnectRequest)) {
        sendReject(from, RejectReason::BadVersion);
    }
}

void NetworkServer::sendReject(const Endpoint& to, RejectReason reason)
{
    ByteWriter writer;
    PacketHeader header;
    header.encode(writer);
    std::uint8_t payload[1] = {static_cast<std::uint8_t>(reason)};
    writeMessage(writer, static_cast<std::uint8_t>(TransportMessage::ConnectReject), false, 0,
                 payload);
    (void)sendRaw(to, writer.data());
}

void NetworkServer::sendMessage(const OutgoingMessage& msg)
{
    const auto now = std::chrono::steady_clock::now();
    if (msg.peerId == AllPeers) {
        for (auto& [id, session] : _sessions) {
            if (id != msg.except)
                queueFor(session, msg.type, msg.payload, msg.reliable, now);
        }
        return;
    }
    auto it = _sessions.find(msg.peerId);
    if (it == _sessions.end())
        return;
    queueFor(it->second, msg.type, msg.payload, msg.reliable, now);
}

void NetworkServer::queueFor(ClientSession& s, std::uint8_t type, std::vector<std::uint8_t> payload,
                             bool reliable, TimePoint now)
{
    if (reliable) {
        if (!s.conn.queueReliable(type, std::move(payload), now))
            s.overflow = true;
        return;
    }
    if (messageSize(payload.size(), false) > MaxDatagramSize - HeaderSize)
        return;
    s.unreliable.push_back({type, std::move(payload)});
}

void NetworkServer::flush(ClientSession& s, TimePoint now)
{
    while (!s.unreliable.empty() || s.conn.hasDueReliable(now) || s.conn.needsHeartbeat(now)) {
        ByteWriter writer;
        auto f = s.conn.prepareSend(now);
        PacketHeader header;
        header.session = s.token;
        header.sequence = f.sequence;
        header.ack = f.ack;
        header.ackBits = f.ackBits;
        header.encode(writer);
        bool wroteSomething = false;

        for (const auto& msg :
             s.conn.takeDueReliable(f.sequence, now, MaxDatagramSize - writer.size())) {
            writeMessage(writer, msg->type, true, msg->id, msg->payload);
            wroteSomething = true;
        }
        std::vector<PendingUnreliable> later;
        for (auto& umsg : s.unreliable) {
            if (messageSize(umsg.payload.size(), false) <= MaxDatagramSize - writer.size()) {
                writeMessage(writer, umsg.type, false, 0, umsg.payload);
                wroteSomething = true;
            } else {
                later.push_back(std::move(umsg));
            }
        }
        s.unreliable = std::move(later);

        if (!wroteSomething) {
            const auto ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
                    .count();
            ByteWriter ping;
            ping.writeU32(static_cast<std::uint32_t>(ms));
            writeMessage(writer, static_cast<std::uint8_t>(TransportMessage::Ping), false, 0,
                         ping.data());
        }
        (void)sendRaw(s.endpoint, writer.data());
    }
}

void NetworkServer::onTick(TimePoint now)
{
    std::vector<PeerId> dead;
    for (auto& [id, s] : _sessions) {
        if (s.overflow || s.conn.isDead(now)) {
            dead.push_back(id);
            continue;
        }
        flush(s, now);
    }
    for (const auto id : dead) {
        removeSession(id, DisconnectReason::Timeout);
    }
}

void NetworkServer::removeSession(PeerId id, DisconnectReason reason)
{
    auto it = _sessions.find(id);
    if (it == _sessions.end())
        return;
    _byEndpoint.erase(it->second.endpoint);
    _sessions.erase(it);
    pushEvent(Disconnected{id, reason});
}

void NetworkServer::kickNow(PeerId id)
{
    auto it = _sessions.find(id);
    if (it == _sessions.end())
        return;
    queueFor(it->second, static_cast<std::uint8_t>(TransportMessage::Disconnect),
             {static_cast<std::uint8_t>(DisconnectReason::Kicked)}, true,
             std::chrono::steady_clock::now());
    flush(it->second, std::chrono::steady_clock::now());
    removeSession(id, DisconnectReason::Kicked);
}

void NetworkServer::onStop()
{
    for (auto& [id, s] : _sessions) {
        queueFor(s, static_cast<std::uint8_t>(TransportMessage::Disconnect),
                 {static_cast<std::uint8_t>(DisconnectReason::Quit)}, true,
                 std::chrono::steady_clock::now());
        flush(s, std::chrono::steady_clock::now());
    }
    _sessions.clear();
    _byEndpoint.clear();
}

PeerId NetworkServer::freeId() const
{
    for (PeerId id = 1; id <= MaxPlayers; ++id) {
        if (_sessions.find(id) == _sessions.end())
            return id;
    }
    return NoPeer;
}

bool NetworkServer::allowDatagram(const Endpoint& from)
{
    const auto now = std::chrono::steady_clock::now();
    switch (_limiter.onPacket(from, now)) {
    case RateVerdict::Allow:
        return true;
    case RateVerdict::Drop:
        return false;
    case RateVerdict::Kick:
        break;
    }
    auto found = _byEndpoint.find(from);
    if (found != _byEndpoint.end())
        kickNow(found->second);
    return false;
}

} // namespace rtype::network
