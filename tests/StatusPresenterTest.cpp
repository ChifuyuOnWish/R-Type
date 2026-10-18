#include "StatusPresenter.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

using namespace rtype::gui;

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string combinedText(const RenderScene& scene)
{
    std::string result;
    for (const auto& text : scene.overlayTexts)
        result += text.text + ' ';
    return result;
}

std::size_t characters(const std::string& text)
{
    std::size_t count = 0;
    for (const unsigned char byte : text) {
        if ((byte & 0xC0) != 0x80)
            ++count;
    }
    return count;
}

void checkBounds(const RenderScene& scene, GuiVector2 viewport)
{
    for (const auto& text : scene.overlayTexts) {
        require(std::isfinite(text.position.x) && std::isfinite(text.position.y) &&
                    text.position.x >= 0.0F && text.position.y >= 0.0F,
                "Status text must have valid positions");
        require(text.fontSize > 0.0F && text.position.y + text.fontSize <= viewport.y,
                "Status text must fit vertically");
        const auto conservativeWidth =
            static_cast<float>(characters(text.text)) * (text.fontSize + text.spacing);
        require(text.position.x + conservativeWidth <= viewport.x + 0.01F,
                "Status text must fit horizontally");
        require(text.fontId.empty(), "Status pages must not depend on a custom font asset");
    }
}

void checkStatuses()
{
    const std::array statuses{GuiStatus::Connecting, GuiStatus::Rejected, GuiStatus::Disconnected,
                              GuiStatus::Error};
    const std::array titles{"Connecting", "Connection rejected", "Disconnected", "Error"};
    const std::array viewports{GuiVector2{1280, 720}, GuiVector2{640, 360}, GuiVector2{320, 240},
                               GuiVector2{1920, 1080}};
    for (std::size_t index = 0; index < statuses.size(); ++index) {
        for (const auto viewport : viewports) {
            const auto scene = StatusPresenter::makeScene({statuses[index], {}}, viewport);
            const auto text = combinedText(scene);
            require(text.find(titles[index]) != std::string::npos,
                    "Status must be identified by an explicit title");
            require(text.find("Escape: close window") != std::string::npos,
                    "Status must provide the window closure hint");
            require(scene.sprites.empty() && scene.sceneTexts.empty() && !scene.background.enabled,
                    "Status pages must not contain gameplay entities, labels or animation");
            require(scene.clearColor.alpha == 255 && scene.feedback.status == GuiStatus::None,
                    "Status presentation must be opaque and ready for direct rendering");
            checkBounds(scene, viewport);
        }
    }
}

void checkMessages()
{
    const auto custom =
        StatusPresenter::makeScene({GuiStatus::Rejected, "The lobby is full."}, {1280, 720});
    require(combinedText(custom).find("The lobby is full.") != std::string::npos,
            "Client-supplied messages must be displayed");
    const auto wrapped =
        StatusPresenter::makeScene({GuiStatus::Error, std::string(5000, 'x')}, {640, 360});
    require(combinedText(wrapped).find("...") != std::string::npos,
            "Truncated messages must end with a visible ellipsis");
    checkBounds(wrapped, {640, 360});

    const auto newlines =
        StatusPresenter::makeScene({GuiStatus::Error, "First line\nSecond line"}, {1280, 720});
    require(combinedText(newlines).find("First line Second line") != std::string::npos,
            "Explicit message lines must be preserved");

    std::string multibyte;
    for (int index = 0; index < 80; ++index)
        multibyte += "\xC3\xA9";
    const auto unicode = StatusPresenter::makeScene({GuiStatus::Error, multibyte}, {640, 360});
    for (const auto& text : unicode.overlayTexts) {
        for (std::size_t index = 0; index < text.text.size(); ++index) {
            const auto byte = static_cast<unsigned char>(text.text[index]);
            if (byte < 128)
                continue;
            require(byte == 0xC3 && index + 1 < text.text.size() &&
                        static_cast<unsigned char>(text.text[index + 1]) == 0xA9,
                    "Wrapping must not split UTF-8 characters");
            ++index;
        }
    }
    checkBounds(unicode, {640, 360});

    const auto unknown = StatusPresenter::makeScene({static_cast<GuiStatus>(-1), {}}, {1280, 720});
    require(combinedText(unknown).find("Error") != std::string::npos,
            "Unknown statuses must use generic error feedback");
}

} // namespace

int main()
{
    checkStatuses();
    checkMessages();
    require(StatusPresenter::makeScene({}, {1280, 720}).overlayTexts.empty(),
            "None must not generate a status page");
    require(StatusPresenter::makeScene({GuiStatus::Error, {}}, {0, 0}).overlayTexts.empty(),
            "An empty viewport must not generate text");
    require(StatusPresenter::makeScene({GuiStatus::Error, {}}, {1, 1}).overlayTexts.empty(),
            "Tiny viewports must omit text that cannot fit");
    bool rejected = false;
    try {
        (void)StatusPresenter::makeScene({GuiStatus::Error, {}},
                                         {std::numeric_limits<float>::quiet_NaN(), 720});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "Invalid viewport geometry must be rejected");
}
