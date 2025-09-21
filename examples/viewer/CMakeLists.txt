# Default settings
cmake_minimum_required(VERSION 3.26)
project(loft_viewer)

# Settings
# ========

# Compile parameters
find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

# Shader compiling

# Collect all shaders
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
file(GLOB SHADERS ${SHADER_DIR}/*.vert
        ${SHADER_DIR}/*.frag
        ${SHADER_DIR}/*.comp)

foreach(SHADER IN LISTS SHADERS)
    get_filename_component(FILENAME ${SHADER} NAME)
    set(OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/shaders/${FILENAME}.spirv) 
    add_custom_command(OUTPUT ${OUTPUT_FILE} 
        COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -gVS ${SHADER} -V -o ${OUTPUT_FILE}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${SHADER}"
    )

    list(APPEND SPV_SHADERS ${OUTPUT_FILE})
endForeach()

add_custom_target(viewer_shaders ALL DEPENDS ${SPV_SHADERS})

file(GLOB FILES
		src/*.cpp
		src/mesh/*.cpp
		src/mesh/runtime/*.cpp
		src/mesh/data/*.cpp
		src/io/*.cpp
		src/runtime/*.cpp
		src/scene/*.cpp)





# External projects
include(FetchContent)

FetchContent_Declare(cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf
        GIT_TAG        master
        SOURCE_DIR     ${CMAKE_CURRENT_BINARY_DIR}/external/cgltf/
		GIT_SHALLOW 1
)

FetchContent_Declare(cglm
        GIT_REPOSITORY https://github.com/recp/cglm
        GIT_TAG        master
        SOURCE_DIR     ${CMAKE_CURRENT_BINARY_DIR}/external/cglm/
		GIT_SHALLOW 1
)

FetchContent_MakeAvailable(cgltf cglm)

file(DOWNLOAD 
    https://raw.githubusercontent.com/nothings/stb/master/stb_image.h 
    ${CMAKE_CURRENT_BINARY_DIR}/external/stb_image.h
)

add_subdirectory(external/imgui/)

# Executable
add_definitions(-DTRACY_ENABLE=1)
add_executable(${PROJECT_NAME} ${FILES})

if ( MSVC )
	set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
	set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR} )
	set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR} )
	# etc for the other available configuration types (MinSizeRel, RelWithDebInfo)
endif ( MSVC )

add_dependencies(${PROJECT_NAME} viewer_shaders imgui)

target_include_directories(
		${PROJECT_NAME}
		PUBLIC
        src/
        ${cglm_INCLUDE_DIRECTORIES}
        ${imgui_INCLUDE_DIRECTORIES}
        # stb-image
        ${CMAKE_CURRENT_BINARY_DIR}/external/
        ${CMAKE_CURRENT_BINARY_DIR}/external/cgltf/
)

target_link_libraries(
		${PROJECT_NAME} 

        PUBLIC
		# modules
        loft::common
		loft::base
		loft_window
		loft::render_graph

		# external libraries
		imgui
        cglm
)

target_link_libraries(
		${PROJECT_NAME} PRIVATE
		${SDL2_LIBRARIES}
)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

if(WIN32)
	add_custom_command(
			TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:SDL2::SDL2>
			$<TARGET_FILE_DIR:${PROJECT_NAME}>
			VERBATIM
	)
endif()
