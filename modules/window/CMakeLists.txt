cmake_minimum_required(VERSION 3.5)

project(loft_window)

file(GLOB FILES src/*.c src/*.cpp)

add_library(${PROJECT_NAME} ${FILES})

find_package(SDL2 QUIET)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

set_target_properties(${PROJECT_NAME} PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include/"
)

target_link_libraries(${PROJECT_NAME} loft_base)

target_include_directories(${PROJECT_NAME}
		PUBLIC
	${PROJECT_SOURCE_DIR}/include/
		PRIVATE
	${loft_base_SOURCE_DIR}/include 
	${Vulkan_INCLUDE_DIR} 
	${VMA_SOURCE_DIR}/include 
	${SDL2_INCLUDE_DIR} 
	../../)
