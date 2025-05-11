#include <stdexcept>
#include <memory>

#include <volk/volk.h>

#include "SDLWindow.h"

int runtime(int argc, char** argv) {
    VkExtent2D extent = {
            .width = 1024,
            .height = 1024
    };

    io::path::setup_exe_path(argv[0]);

    if(volkInitialize()) {
        throw std::runtime_error("Failed to initialize volk");
    }

	/**
     * Opens up a window
     */
    std::shared_ptr<Window> window = std::make_shared<SDLWindow>(SDLWindow("loft", {
            0, 0,
            extent.width, extent.height
    }));
}
