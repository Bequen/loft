cmake_minimum_required(VERSION 3.25)

project(loft_window)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB FILES src/*.c src/*.cpp)

add_library(${PROJECT_NAME} ${FILES}
        src/SDLWindow.cpp
        include/SDLWindow.h)

target_include_directories(${PROJECT_NAME} PUBLIC ./include/)

target_link_libraries(${PROJECT_NAME} loft_base)

target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIR} ${SDL2_INCLUDE_DIR})