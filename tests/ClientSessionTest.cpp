#include "ClientSession.hpp"

#include "NetworkServer.hpp"

#include <chrono>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <variant>

using namespace std::chrono_literals;
namespace ui = rtype::gui;
namespace net = rtype::network;
namespace proto = rtype::protocol;
namespace {
void check(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}
template <typename Message>
void send(net::NetworkServer& server, net::PeerId peer, const Message& message)
{
    proto::ByteWriter writer;
    check(message.serialize(writer), "Message encoded");
    server.send(peer, static_cast<std::uint8_t>(Message::Type), writer.data(), true);
}
} // namespace
int main()
{
    try {
        net::NetworkServer server;
        check(!server.start(0), "Loopback server starts");
        rtype::client::ClientSession session;
        check(session.makeScene().frontend.page == ui::FrontendPage::Connection,
              "Starts on connection screen");
        net::PeerId peer = net::NoPeer;
        std::vector<net::MessageReceived> messages;
        ui::PlayerActions actions;
        const auto pump = [&] {
            session.update(1.0F / 60.0F, actions);
            for (const auto& event : server.poll()) {
                if (const auto* joined = std::get_if<net::Connected>(&event))
                    peer = joined->peerId;
                if (const auto* message = std::get_if<net::MessageReceived>(&event))
                    messages.push_back(*message);
            }
        };
        const auto wait = [&](const std::function<bool()>& predicate) {
            const auto until = std::chrono::steady_clock::now() + 3s;
            while (std::chrono::steady_clock::now() < until) {
                pump();
                if (predicate())
                    return true;
                std::this_thread::sleep_for(5ms);
            }
            return false;
        };
        ui::FrontendAction connect;
        connect.command = ui::FrontendCommand::Connect;
        connect.connection = {"127.0.0.1", server.localPort(), "Pilot"};
        session.handle(connect);
        check(wait([&] {
                  return peer != net::NoPeer &&
                         session.makeScene().frontend.page == ui::FrontendPage::Waiting;
              }),
              "Handshake waits for server lobby");
        check(messages.empty(), "No gameplay inputs sent before lobby state");
        proto::LobbyStateMessage state;
        state.leaderId = static_cast<std::uint8_t>(peer);
        state.levelNames = {"Stage 1", "Stage 2"};
        state.unlocked = 1;
        send(server, peer, state);
        check(wait([&] { return session.makeScene().frontend.page == ui::FrontendPage::Lobby; }),
              "Authoritative lobby displayed");
        ui::FrontendAction select;
        select.command = ui::FrontendCommand::Select;
        select.mode = ui::FrontendMode::Infinite;
        session.handle(select);
        check(session.makeScene().frontend.selectionPending, "Waits for selection confirmation");
        check(wait([&] {
                  for (const auto& m : messages)
                      if (m.type == static_cast<std::uint8_t>(proto::MessageType::LobbySelect))
                          return true;
                  return false;
              }),
              "Reliable mode selection reaches server");
        for (const auto& m : messages)
            if (m.type == static_cast<std::uint8_t>(proto::MessageType::LobbySelect)) {
                proto::ByteReader reader(m.payload.data(), m.payload.size());
                const auto decoded = proto::LobbySelectMessage::deserialize(reader);
                check(decoded && decoded->kind == proto::GameKind::Infinite,
                      "Protocol selection decodes correctly");
            }
        ui::FrontendAction start;
        start.command = ui::FrontendCommand::Start;
        session.handle(start);
        state.kind = proto::GameKind::Infinite;
        send(server, peer, state);
        check(wait([&] { return !session.makeScene().frontend.selectionPending; }),
              "Server acknowledgement unlocks start");
        messages.clear();
        session.handle(start);
        check(wait([&] {
                  for (const auto& m : messages)
                      if (m.type == static_cast<std::uint8_t>(proto::MessageType::LobbyStart))
                          return true;
                  return false;
              }),
              "Start request reaches server");
        check(session.makeScene().frontend.page == ui::FrontendPage::Lobby,
              "Client cannot start gameplay locally");
        state.phase = proto::LobbyPhase::Playing;
        send(server, peer, state);
        actions.heldActions[static_cast<std::size_t>(ui::PlayerAction::Fire)] = true;
        check(wait([&] { return session.makeScene().frontend.page == ui::FrontendPage::Gameplay; }),
              "Playing state opens game");
        messages.clear();
        check(wait([&] {
                  for (const auto& m : messages)
                      if (m.type == static_cast<std::uint8_t>(proto::MessageType::Input))
                          return true;
                  return false;
              }),
              "Gameplay inputs transmitted");
        for (const auto& m : messages)
            if (m.type == static_cast<std::uint8_t>(proto::MessageType::Input)) {
                proto::ByteReader reader(m.payload.data(), m.payload.size());
                const auto decoded = proto::InputMessage::deserialize(reader);
                check(decoded && decoded->clientTick > 0 && (decoded->buttons & proto::ButtonShoot),
                      "Input payload is serialized");
            }
        state.phase = proto::LobbyPhase::GameOver;
        state.leaderId = static_cast<std::uint8_t>(peer + 1);
        send(server, peer, state);
        check(wait([&] {
                  return session.makeScene().frontend.page == ui::FrontendPage::Lobby &&
                         !session.makeScene().frontend.leader;
              }),
              "Game over returns to lobby and updates leader");
        messages.clear();
        session.handle(start);
        session.handle(select);
        for (int i = 0; i < 15; ++i) {
            pump();
            std::this_thread::sleep_for(5ms);
        }
        for (const auto& message : messages)
            check(message.type != static_cast<std::uint8_t>(proto::MessageType::LobbyStart) &&
                      message.type != static_cast<std::uint8_t>(proto::MessageType::LobbySelect),
                  "Follower cannot send start or selection");
        server.kick(peer);
        check(
            wait([&] { return session.makeScene().frontend.page == ui::FrontendPage::Connection; }),
            "Disconnection returns to form");
        session.handle(connect);
        check(wait([&] { return session.makeScene().frontend.page == ui::FrontendPage::Waiting; }),
              "Reconnect works");
        ui::FrontendAction cancel;
        cancel.command = ui::FrontendCommand::Cancel;
        session.handle(cancel);
        check(session.makeScene().frontend.page == ui::FrontendPage::Connection,
              "Cancellation resets session");
        std::cout << "Client session integration passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
