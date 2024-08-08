set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required(VERSION 3.5)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0
        -DNOMINMAX=1
        -DVK_LAYERS_ENABLE=0
        -DLOFT_DEBUG=0
        -DLFT_DEBUG_LEVEL_0
        # Extensions
        # -DVK_EXT_debug_utils
        -DVK_KHR_create_renderpass2
        -DVK_KHR_synchronization2
        -DVK_KHR_shader_non_semantic_info)

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

if (WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif(UNIX)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XCB_KHR)
endif()

add_subdirectory(volk)

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

