#undef NDEBUG
#include "Messages.hpp"

#include <cassert>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

using namespace rtype::protocol;
using Bytes = std::vector<std::uint8_t>;

template <typename T> static Bytes encode(const T& m)
{
    ByteWriter w;
    assert(m.serialize(w));
    return w.data();
}

template <typename T> static std::optional<T> decode(const Bytes& b)
{
    ByteReader r(b.data(), b.size());
    return T::deserialize(r);
}

template <typename T> static void checkRoundTrip(const T& m)
{
    const Bytes b = encode(m);
    const auto back = decode<T>(b);
    assert(back.has_value() && *back == m);

    for (std::size_t n = 0; n < b.size(); ++n) {
        ByteReader r(b.data(), n);
        assert(!T::deserialize(r).has_value());
    }
    Bytes extra = b;
    extra.push_back(0);
    assert(!decode<T>(extra).has_value());
}

template <typename T> static void checkSerializeRejected(const T& m)
{
    ByteWriter w;
    assert(!m.serialize(w));
    assert(w.size() == 0);
}

static void test_message_types()
{
    assert(static_cast<int>(MessageType::Input) == 0x10);
    assert(static_cast<int>(MessageType::PlayerJoined) == 0x11);
    assert(static_cast<int>(MessageType::PlayerLeft) == 0x12);
    assert(static_cast<int>(MessageType::EntitySpawn) == 0x13);
    assert(static_cast<int>(MessageType::EntityDestroy) == 0x14);
    assert(static_cast<int>(MessageType::Snapshot) == 0x15);
    assert(static_cast<int>(MessageType::GameEvent) == 0x16);
}

static void test_fixed()
{
    assert(Fixed::fromFloat(1.0f).raw == 256);
    assert(Fixed::fromFloat(-1.5f).raw == -384);
    assert(Fixed::fromFloat(3.25f).toFloat() == 3.25f);
    assert(Fixed16::fromFloat(2.0f).raw == 512);
    assert(Fixed16::fromFloat(1e9f).raw == std::numeric_limits<std::int16_t>::max());
    assert(Fixed16::fromFloat(-1e9f).raw == std::numeric_limits<std::int16_t>::min());
    assert(Fixed::fromFloat(1e12f).raw > 0);
    assert(Fixed::fromFloat(-1e12f).raw < 0);
}

static void test_input()
{
    checkRoundTrip(InputMessage{0, 0});
    checkRoundTrip(InputMessage{0xFFFFFFFF, ButtonMask});
    checkRoundTrip(InputMessage{42, ButtonUp | ButtonShoot});

    const Bytes exact = encode(InputMessage{0x01020304, ButtonLeft});
    assert((exact == Bytes{0x01, 0x02, 0x03, 0x04, 0x04}));

    // Bits 5-7 must be 0.
    checkSerializeRejected(InputMessage{1, 0x20});
    checkSerializeRejected(InputMessage{1, 0x80});
    assert(!decode<InputMessage>(Bytes{0, 0, 0, 1, 0x20}).has_value());
    assert(!decode<InputMessage>(Bytes{0, 0, 0, 1, 0xE0}).has_value());
}

static void test_player_joined()
{
    checkRoundTrip(PlayerJoinedMessage{0, "A"});
    checkRoundTrip(PlayerJoinedMessage{255, std::string(32, 'x')});
    checkRoundTrip(PlayerJoinedMessage{3, "Joueur é"});

    const Bytes exact = encode(PlayerJoinedMessage{7, "Bob"});
    assert((exact == Bytes{0x07, 0x03, 0x42, 0x6F, 0x62}));

    // Name must be 1 to 32 bytes.
    checkSerializeRejected(PlayerJoinedMessage{1, ""});
    checkSerializeRejected(PlayerJoinedMessage{1, std::string(33, 'x')});
    assert(!decode<PlayerJoinedMessage>(Bytes{1, 0}).has_value());
    Bytes tooLong{1, 33};
    tooLong.insert(tooLong.end(), 33, 'x');
    assert(!decode<PlayerJoinedMessage>(tooLong).has_value());
}

static void test_player_left()
{
    checkRoundTrip(PlayerLeftMessage{0, PlayerLeftReason::Quit});
    checkRoundTrip(PlayerLeftMessage{9, PlayerLeftReason::TimedOut});
    checkRoundTrip(PlayerLeftMessage{255, PlayerLeftReason::Kicked});

    checkSerializeRejected(PlayerLeftMessage{1, static_cast<PlayerLeftReason>(3)});
    assert(!decode<PlayerLeftMessage>(Bytes{1, 3}).has_value());
    assert(!decode<PlayerLeftMessage>(Bytes{1, 255}).has_value());
}

static void test_entity_spawn()
{
    checkRoundTrip(EntitySpawnMessage{0, 0, Fixed{0}, Fixed{0}});
    checkRoundTrip(EntitySpawnMessage{0xFFFFFFFF, 0xFFFF,
                                      Fixed{std::numeric_limits<std::int32_t>::max()},
                                      Fixed{std::numeric_limits<std::int32_t>::min()}});
    checkRoundTrip(EntitySpawnMessage{12, 3, Fixed::fromFloat(100.5f), Fixed::fromFloat(-20.25f)});

    const Bytes exact = encode(EntitySpawnMessage{1, 2, Fixed{0x100}, Fixed{-1}});
    assert((exact == Bytes{0, 0, 0, 1, 0, 2, 0, 0, 1, 0, 0xFF, 0xFF, 0xFF, 0xFF}));
}

static void test_entity_destroy()
{
    checkRoundTrip(EntityDestroyMessage{0, DestroyCause::LeftScreen});
    checkRoundTrip(EntityDestroyMessage{77, DestroyCause::Killed});
    checkRoundTrip(EntityDestroyMessage{0xFFFFFFFF, DestroyCause::Other});

    checkSerializeRejected(EntityDestroyMessage{1, static_cast<DestroyCause>(3)});
    assert(!decode<EntityDestroyMessage>(Bytes{0, 0, 0, 1, 3}).has_value());
}

static SnapshotEntry entry(std::uint32_t id, int k)
{
    return SnapshotEntry{id, Fixed{k * 1000}, Fixed{-k * 500},
                         Fixed16{static_cast<std::int16_t>(k)},
                         Fixed16{static_cast<std::int16_t>(-k)}};
}

static void test_snapshot()
{
    checkRoundTrip(SnapshotMessage{0, {}});
    checkRoundTrip(SnapshotMessage{1234, {entry(1, 1)}});
    checkRoundTrip(SnapshotMessage{0xFFFFFFFF, {entry(1, 1), entry(2, 2), entry(0xFFFFFFFF, 3)}});

    SnapshotEntry extremes{0xFFFFFFFF, Fixed{std::numeric_limits<std::int32_t>::min()},
                           Fixed{std::numeric_limits<std::int32_t>::max()},
                           Fixed16{std::numeric_limits<std::int16_t>::min()},
                           Fixed16{std::numeric_limits<std::int16_t>::max()}};
    checkRoundTrip(SnapshotMessage{5, {extremes}});

    // Fills a datagram exactly.
    SnapshotMessage full;
    full.serverTick = 9;
    for (std::size_t i = 0; i < MaxSnapshotEntries; ++i)
        full.entries.push_back(entry(static_cast<std::uint32_t>(i + 1), 1));
    checkRoundTrip(full);
    assert(encode(full).size() == SnapshotFixedPartSize + SnapshotEntrySize * MaxSnapshotEntries);
    assert(SnapshotFixedPartSize + SnapshotEntrySize * MaxSnapshotEntries + 4 + 16 <= 1200);

    // One more does not fit.
    full.entries.push_back(entry(9999, 1));
    checkSerializeRejected(full);
    Bytes b = encode(SnapshotMessage{1, {entry(1, 1), entry(2, 2)}});
    b[5] = 3; // count says 3, payload holds 2
    assert(!decode<SnapshotMessage>(b).has_value());
    b[5] = 1; // count says 1, payload holds 2
    assert(!decode<SnapshotMessage>(b).has_value());
    b[4] = 0xFF; // huge count
    b[5] = 0xFF;
    assert(!decode<SnapshotMessage>(b).has_value());
}

static void test_game_event()
{
    checkRoundTrip(GameEventMessage{GameEventType::Shoot, 1, 0});
    checkRoundTrip(GameEventMessage{GameEventType::Hit, 1, 2});
    checkRoundTrip(GameEventMessage{GameEventType::Kill, 0xFFFFFFFF, 0xFFFFFFFF});
    checkRoundTrip(GameEventMessage{GameEventType::PlayerDeath, 5, 0});

    checkSerializeRejected(GameEventMessage{static_cast<GameEventType>(4), 1, 2});
    assert(!decode<GameEventMessage>(Bytes{4, 0, 0, 0, 1, 0, 0, 0, 2}).has_value());
    assert(!decode<GameEventMessage>(Bytes{255, 0, 0, 0, 1, 0, 0, 0, 2}).has_value());
}

int main()
{
    test_message_types();
    test_fixed();
    test_input();
    test_player_joined();
    test_player_left();
    test_entity_spawn();
    test_entity_destroy();
    test_snapshot();
    test_game_event();
    return 0;
}