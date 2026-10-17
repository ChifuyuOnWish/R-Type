#include "Gui.hpp"

#include <exception>
#include <iostream>

int main()
{
    try {
        rtype::gui::Gui gui;
        gui.open({});

        rtype::gui::RenderScene scene;
        scene.clearColor = {20, 24, 32, 255};

        while (!gui.shouldClose()) {
            const auto actions = gui.pollActions();
            (void)actions;
            gui.render(scene);
        }

        gui.close();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "r-type_client: " << error.what() << '\n';
        return 1;
    }
}
