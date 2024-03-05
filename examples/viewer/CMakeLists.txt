# Default settings
cmake_minimum_required(VERSION 3.25)
project(loft_viewer)

# Settings
# ========

# Please set default paths for vulkan and SDL2
set(VULKAN_PATH ENV{VULKAN_SDK} CACHE STRING "Path to vulkan directory")

option(TRACY_ENABLED 0)


# Compile parameters


find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)


# Shader compiling
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
# Find all the shaders
file(GLOB SHADERS   ${SHADER_DIR}/*.vert
                    ${SHADER_DIR}/*.frag)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)
foreach(SHADER IN LISTS SHADERS)
    get_filename_component(FILENAME ${SHADER} NAME)
    add_custom_command(OUTPUT ${SHADER_DIR}/build/${FILENAME}.spirv
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -gVS ${SHADER} -V -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/${FILENAME}.spirv
            DEPENDS ${SHADER}
            COMMENT "Compiling ${FILENAME}")
    list(APPEND SPV_SHADERS ${SHADER_DIR}/build/${FILENAME}.spirv)
endForeach()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})

file(GLOB FILES src/*.cpp src/mesh/*.cpp src/io/*.cpp src/runtime/*.cpp src/scene/*.cpp)





# External projects
include(FetchContent)


FetchContent_Declare(cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf
        GIT_TAG        master
        SOURCE_DIR     ./external/cgltf/
		GIT_SHALLOW 1
)

FetchContent_Declare(cglm
        GIT_REPOSITORY https://github.com/recp/cglm
        GIT_TAG        master
        SOURCE_DIR     ./external/cglm/
		GIT_SHALLOW 1
)

FetchContent_MakeAvailable(cgltf cglm)


##ExternalProject_Add(imgui
#        GIT_REPOSITORY https://github.com/ocornut/imgui
#        GIT_TAG        master
#        SOURCE_DIR     ./external/imgui/
#        BUILD_COMMAND ""
#        CONFIGURE_COMMAND ""
#        INSTALL_COMMAND ""
#)

file(DOWNLOAD https://raw.githubusercontent.com/nothings/stb/master/stb_image.h ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image.h)

add_subdirectory(external/imgui/)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Executable
add_definitions(-DTRACY_ENABLE=1)
add_executable(${PROJECT_NAME} ${FILES})

add_dependencies(${PROJECT_NAME} shaders imgui)

target_include_directories(
		${PROJECT_NAME}
		PRIVATE
		${cglm_SOURCE_DIR}/include/
		${cgltf_SOURCE_DIR}
		${PROJECT_SOURCE_DIR}/external
		${PROJECT_SOURCE_DIR}/external/imgui
		${VMA_SOURCE_DIR}/include/
		${Vulkan_INCLUDE_DIR}
		${SDL2_INCLUDE_DIR}
)

target_link_libraries(
		${PROJECT_NAME} PRIVATE

		# modules
		loft_base
		loft_window
		loft_framegraph

		# external libraries
		${SDL2_LIBRARIES}
		${Vulkan_LIBRARIES}
		imgui
)
