#include <volk.h>
#include <stdexcept>
#include "SDLWindow.h"
#include <SDL2/SDL_vulkan.h>
#include <vector>
#include <iostream>

//
// Created by martin on 11/12/23.
//
void SDLWindow::resize() {

}

VkExtent2D SDLWindow::get_size() {
    return {};
}

SDLWindow::SDLWindow(std::string name, VkRect2D rect) {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVERYTHING) < 0) {
        throw std::runtime_error("Failed to initialize sdl");
    }

    m_pWindow = SDL_CreateWindow("Test",
                                 SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED,
                                 rect.extent.width,
                                 rect.extent.height,
                                 SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if(m_pWindow == nullptr) {
		throw std::runtime_error("Failed to create window: " + std::string(SDL_GetError()));
    }

    uint32_t count = 0;
    SDL_Vulkan_GetInstanceExtensions(m_pWindow, &count, nullptr);

    std::vector<char*> extensions(count);
    SDL_Vulkan_GetInstanceExtensions(m_pWindow, &count, (const char**)extensions.data());


    // m_pSurface = SDL_GetWindowSurface(m_pWindow);
}

VkSurfaceKHR SDLWindow::create_surface(VkInstance instance) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    SDL_Vulkan_CreateSurface(m_pWindow, instance, &surface);
    return surface;
}

bool SDLWindow::is_open() const {
    return true;
}

int32_t SDLWindow::poll_event(SDL_Event *pOutEvent) const {
    return SDL_PollEvent(pOutEvent);
}

void SDLWindow::get_required_extensions(uint32_t *pOutSize, const char** pOut)
{
    SDL_Vulkan_GetInstanceExtensions(m_pWindow, pOutSize, (const char**)pOut);
}
