//
// Created by martin on 11/12/23.
//

#ifndef LOFT_SDLWINDOW_H
#define LOFT_SDLWINDOW_H

#include "Window.hpp"
#include <SDL2/SDL.h>
#include <utility>
#include <volk.h>
#include <string>
#include <vulkan/vulkan_core.h>

namespace lft::win {

class SDLWindow : public Window {
private:
    SDL_Window *m_pWindow;
    SDL_Surface *m_pSurface;

    VkExtent2D m_extent;

public:
    // rule of five

    SDLWindow(const SDLWindow&) = delete;
    SDLWindow(SDLWindow&& other) :
        m_pWindow(std::exchange(other.m_pWindow, nullptr)),
        m_pSurface(std::exchange(other.m_pSurface, nullptr)){

    }

    SDLWindow& operator=(const SDLWindow&) = delete;
    SDLWindow& operator=(SDLWindow&& other) {
        *this = std::move(other);
        return *this;
    }

    SDL_Window* get_handle() const { return m_pWindow; }

    SDLWindow(const std::string name, VkRect2D rect);

    void resize() override;

    VkExtent2D get_size() const override;

    Surface create_surface(const Instance* instance) const override;

    bool is_open() const override;

    int32_t poll_event(SDL_Event *pOutEvent) const override;

    std::vector<std::string> get_required_extensions() const override;
};

}

#endif //LOFT_SDLWINDOW_H
