#pragma once

#include <volk.h>
#include <SDL2/SDL_events.h>
#include <vector>
#include <string>

#include "Surface.hpp"
#include "Instance.hpp"

namespace lft::win {

/**
 * Universal interface for windows.
 * WARNING: It is not responsible for interacting with some graphics API!
 * Instead, it should contain a function that Gpu interface will call by itself
 * when ready.
 */

class Window {
public:
    virtual void resize() = 0;

    [[nodiscard("Getter should not be discarded")]]
    virtual VkExtent2D get_size() const = 0;

    [[nodiscard]] virtual bool is_open() const = 0;

    virtual int32_t poll_event(SDL_Event *pOutEvent) const = 0;

    virtual std::vector<std::string> get_required_extensions() const = 0;

    virtual Surface create_surface(const Instance* instance) const = 0;
};

}
