cmake_minimum_required(VERSION 3.25)

project(imgui)

file(GLOB FILES ./*.cpp ./backends/imgui_impl_vulkan.cpp ./backends/imgui_impl_sdl2.cpp)

include_directories(backends ${CMAKE_CURRENT_SOURCE_DIR})

find_package(SDL2 QUIET)
find_package(Vulkan REQUIRED)

# add_definitions(-DVK_NO_PROTOTYPES=1)
add_library(${PROJECT_NAME} ${FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES} ${SDL2_LIBRARIES})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS})