set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

cmake_minimum_required(VERSION 3.5)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

# Please set default paths for vulkan and SDL2
set(VULKAN_PATH "$ENV{VULKAN_SDK}" CACHE STRING "Path to vulkan directory")
set(SDL2_PATH "$ENV{VULKAN_SDK}/cmake" CACHE STRING "Path to SDL2 directory")

list(APPEND CMAKE_PREFIX_PATH "$ENV{VULKAN_SDK}/cmake")

include(FetchContent)

FetchContent_Declare(
        VMA
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        GIT_TAG        master
        SOURCE_DIR     ./external/VulkanMemoryAllocator/
        GIT_SHALLOW 1
)

FetchContent_MakeAvailable(VMA)

project(loft)

add_subdirectory(modules)
add_subdirectory(examples)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

