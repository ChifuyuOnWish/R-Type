#pragma once

#include "ClientNetwork.hpp"
#include "ClientWorld.hpp"
#include "GuiTypes.hpp"
#include "LobbyMessages.hpp"

#include <chrono>
#include <memory>
#include <unordered_map>

namespace rtype::client {

class ClientSession
{
public:
    explicit ClientSession(gui::ConnectionDetails connection = {});
    void handle(const gui::FrontendAction& action);
    void update(float dt, const gui::PlayerActions& actions);
    [[nodiscard]] gui::RenderScene makeScene();
    [[nodiscard]] const std::vector<gui::SoundEvent>& sounds() const noexcept;

private:
    void connect(const gui::ConnectionDetails& connection);
    void installHandlers();
    void resetWorld();
    void lobby(const protocol::LobbyStateMessage& state);
    void select(const gui::FrontendAction& action);
    std::unique_ptr<ClientNetwork> _network;
    std::unique_ptr<ClientWorld> _world;
    gui::FrontendState _frontend;
    std::unordered_map<std::uint32_t, std::string> _names;
    std::vector<gui::SoundEvent> _sounds;
    std::uint32_t _clientTick = 1;
    std::chrono::steady_clock::time_point _waitingSince;
    gui::FrontendAction _pending;
};

} // namespace rtype::client
