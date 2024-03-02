cmake_minimum_required(VERSION 3.25)

project(loft_window)

file(GLOB FILES src/*.c src/*.cpp)

add_library(${PROJECT_NAME} ${FILES}
        src/SDLWindow.cpp
        include/SDLWindow.h)

target_include_directories(${PROJECT_NAME} PUBLIC ./include/)

target_link_libraries(${PROJECT_NAME} loft_base)

target_include_directories(${PROJECT_NAME} PUBLIC C:/VulkanSDK/1.3.268.0/include/)