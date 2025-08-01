cmake_minimum_required(VERSION 3.26)

option(ENABLE_EXAMPLES "Enable building examples" ON)

if (WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif(UNIX)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XCB_KHR)
endif()


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

add_definitions(
        -DNOMINMAX=1
        -DVK_LAYERS_ENABLE=1
        -DLOFT_DEBUG=1
        -DLFT_DEBUG_LEVEL_3=1
        # Extensions
        -DVK_EXT_debug_utils
        -DVK_KHR_create_renderpass2
        -DVK_KHR_synchronization2
        -DVK_KHR_shader_non_semantic_info
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(volk)
add_subdirectory(modules)

if(ENABLE_EXAMPLES)
	add_subdirectory(examples)
endif()


