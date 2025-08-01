project(imgui)

file(GLOB FILES ./*.cpp ./backends/imgui_impl_vulkan.cpp ./backends/imgui_impl_sdl2.cpp)


find_package(SDL2 QUIET)
find_package(Vulkan REQUIRED)

# add_definitions(-DVK_NO_PROTOTYPES=1)
add_library(${PROJECT_NAME} OBJECT ${FILES})
target_include_directories(${PROJECT_NAME}
    PUBLIC
    backends
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    volk::volk
    ${SDL2_LIBRARIES})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${volk_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS})
