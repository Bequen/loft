set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required(VERSION 3.5)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

project(loft)

add_subdirectory(modules)
add_subdirectory(examples)

include(FetchContent)

FetchContent_Declare(
        VMA
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        GIT_TAG        master
        SOURCE_DIR     ./external/VulkanMemoryAllocator/
        GIT_SHALLOW 1
)

FetchContent_MakeAvailable(VMA)

