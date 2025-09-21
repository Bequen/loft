#pragma once

#include <volk.h>
#include <SDL2/SDL_events.h>
#include <vector>
#include <string>

#include "Surface.hpp"

/**
 * Universal interface for windows.
 * WARNING: It is not responsible for interacting with some graphics API!
 * Instead, it should contain a function that Gpu interface will call by itself
 * when ready.
 */

class Window : public Surface {
public:
    virtual ~Window() {
    }

    virtual void resize() = 0;

    [[nodiscard("Getter should not be discarded")]]
    virtual VkExtent2D get_size() = 0;

    [[nodiscard]] virtual bool is_open() const = 0;

    virtual int32_t poll_event(SDL_Event *pOutEvent) const = 0;

    virtual std::vector<std::string> get_required_extensions() = 0;
};
