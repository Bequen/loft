cmake_minimum_required(VERSION 3.25)

project(imgui)

file(GLOB FILES ./*.cpp ./backends/imgui_impl_vulkan.cpp ./backends/imgui_impl_sdl2.cpp)

include_directories(backends ${CMAKE_CURRENT_SOURCE_DIR})



add_library(${PROJECT_NAME} ${FILES})

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS})