#include "Connection.hpp"

#include "SequenceNumber.hpp"

namespace rtype::network {

Connection::AckFields Connection::prepareSend(TimePoint now)
{
    std::uint16_t seq = _localSequence++;
    _lastSent = now;
    _sent[seq % SentWindow] = {seq, now, false, true, {}};
    return {seq, _remoteSequence, _receivedBits};
}

Connection::Receive Connection::onPacketReceived(std::uint16_t sequence, std::uint16_t ack,
                                                 std::uint32_t ackBits, TimePoint now,
                                                 std::vector<std::uint16_t>& newlyAcked)
{
    Connection::Receive r = updateReceived(sequence);
    if (r != Receive::New)
        return r;
    _lastReceived = now;

    markAcked(ack, now, newlyAcked);
    for (std::uint32_t n = 0; n < 32; ++n) {
        if (ackBits & (1u << n)) {
            markAcked(static_cast<std::uint16_t>(ack - n - 1), now, newlyAcked);
        }
    }
    return r;
}

Connection::Receive Connection::updateReceived(std::uint16_t sequence)
{
    if (!_hasReceived) {
        _hasReceived = true;
        _remoteSequence = sequence;
        _receivedBits = 0;
        return Receive::New;
    }
    if (sequence == _remoteSequence) {
        return Receive::Duplicate;
    }
    if (sequenceGreater(sequence, _remoteSequence)) {
        std::uint16_t diff = static_cast<std::uint16_t>(sequence - _remoteSequence);
        if (diff < 32) {
            _receivedBits = (_receivedBits << diff) | (1u << (diff - 1));
        } else if (diff == 32) {
            _receivedBits = 1u << 31;
        } else {
            _receivedBits = 0;
        }
        _remoteSequence = sequence;
        return Receive::New;
    } else {
        std::uint16_t diff = static_cast<std::uint16_t>(_remoteSequence - sequence);
        if (diff > 32)
            return Receive::TooOld;
        std::uint32_t bit = 1u << (diff - 1);
        if (_receivedBits & bit)
            return Receive::Duplicate;
        _receivedBits |= bit;
        return Receive::New;
    }
}

void Connection::markAcked(std::uint16_t sequence, TimePoint now,
                           std::vector<std::uint16_t>& newlyAcked)
{
    SentPacket& slot = _sent[sequence % SentWindow];
    if (!slot.valid || slot.sequence != sequence || slot.acked)
        return;
    slot.acked = true;
    Rtt sample = now - slot.sentAt;
    if (!_hasRtt) {
        _rtt = sample;
        _hasRtt = true;
    } else {
        _rtt += (sample - _rtt) * 0.1f;
    }
    for (std::uint16_t id : slot.reliableIds)
        std::erase_if(_pending, [id](const ReliableMessage& m) { return m.id == id; });
    slot.reliableIds.clear();
    newlyAcked.push_back(sequence);
}

Connection::Rtt Connection::rtt() const
{
    return _rtt;
}

Connection::TimePoint Connection::lastReceived() const
{
    return _lastReceived;
}

bool Connection::queueReliable(std::uint8_t type, std::vector<std::uint8_t> payload, TimePoint now)
{
    if (_pending.size() >= MaxPendingReliable)
        return false;
    if (ReliableMessageOverhead + payload.size() > MaxDatagramSize - HeaderSize)
        return false;
    _pending.push_back({_nextReliableId++, type, std::move(payload), now, {}, false});
    return true;
}

std::vector<const Connection::ReliableMessage*>
Connection::takeDueReliable(std::uint16_t sequence, TimePoint now, std::size_t budget)
{
    std::vector<const ReliableMessage*> out;
    SentPacket& slot = _sent[sequence % SentWindow];
    if (!slot.valid || slot.sequence != sequence)
        return out;
    for (auto& m : _pending) {
        if (m.sent && now - m.lastSentAt < ReliableResendInterval)
            continue;
        std::size_t size = ReliableMessageOverhead + m.payload.size();
        if (size > budget)
            continue;
        budget -= size;
        m.sent = true;
        m.lastSentAt = now;
        slot.reliableIds.push_back(m.id);
        out.push_back(&m);
    }
    return out;
}

bool Connection::acceptReliable(std::uint16_t id)
{
    ReceivedReliable& slot = _received[id % ReceivedWindow];
    if (slot.valid && slot.id == id)
        return false;
    slot = {id, true};
    return true;
}

bool Connection::reliableTimedOut(TimePoint now) const
{
    return !_pending.empty() && now - _pending.front().queuedAt >= ReliableTimeout;
}

std::size_t Connection::pendingReliable() const
{
    return _pending.size();
}

Connection::Connection(TimePoint now) : _lastReceived(now), _lastSent(now) {}

bool Connection::needsHeartbeat(TimePoint now) const
{
    return now - _lastSent >= HeartbeatInterval;
}

bool Connection::hasDueReliable(TimePoint now) const
{
    for (const auto& m : _pending)
        if (!m.sent || now - m.lastSentAt >= ReliableResendInterval)
            return true;
    return false;
}

bool Connection::isTimedOut(TimePoint now) const
{
    return now - _lastReceived >= ConnectionTimeout;
}

bool Connection::isDead(TimePoint now) const
{
    return isTimedOut(now) || reliableTimedOut(now);
}

} // namespace rtype::network
