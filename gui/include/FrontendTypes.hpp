#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace rtype::gui {

enum class FrontendPage
{
    Gameplay,
    Connection,
    Connecting,
    Waiting,
    Lobby,
    Error
};
enum class FrontendMode
{
    Campaign,
    Duel,
    Infinite
};
enum class FrontendCommand
{
    None,
    Connect,
    Cancel,
    Disconnect,
    Select,
    Start,
    Quit
};

struct ConnectionDetails
{
    std::string host = "127.0.0.1";
    std::uint16_t port = 4242;
    std::string name = "Player";
};

struct FrontendAction
{
    FrontendCommand command = FrontendCommand::None;
    ConnectionDetails connection;
    FrontendMode mode = FrontendMode::Campaign;
    std::size_t level = 0;
};

struct FrontendState
{
    FrontendPage page = FrontendPage::Gameplay;
    ConnectionDetails connection;
    std::string message;
    bool leader = false;
    bool selectionPending = false;
    FrontendMode mode = FrontendMode::Campaign;
    std::size_t level = 0;
    std::size_t unlocked = 0;
    std::vector<std::string> levels;
};

} // namespace rtype::gui
