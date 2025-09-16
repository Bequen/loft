//
// Created by martin on 11/12/23.
//

#ifndef LOFT_SDLWINDOW_H
#define LOFT_SDLWINDOW_H

#include "Window.hpp"
#include "event.h"
#include <SDL2/SDL.h>
#include <volk.h>
#include <string>

class SDLWindow : public Window {
private:
    SDL_Window *m_pWindow;
    SDL_Surface *m_pSurface;

public:
    SDL_Window* get_handle() { return m_pWindow; }

    SDLWindow(std::string name, VkRect2D rect);

    void resize() override;

    VkExtent2D get_size() override;

    VkSurfaceKHR create_surface(VkInstance instance) override;

    bool is_open() const override;

    int32_t poll_event(SDL_Event *pOutEvent) const override;

    std::vector<std::string> get_required_extensions() override;
};

#endif //LOFT_SDLWINDOW_H
