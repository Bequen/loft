cmake_minimum_required(VERSION 3.5)

project(loft_render_graph_tests)

add_executable(loft_render_graph_tests RenderGraphBuilderTests.cpp)

find_package(Vulkan QUIET)

target_link_libraries(loft_render_graph_tests PRIVATE
		loft_render_graph
		loft_base
		loft_common
		volk
)

target_include_directories(loft_render_graph_tests
		PUBLIC
		include/
		../include/
        ${Vulkan_INCLUDE_DIR}
		${PROJECT_SOURCE_DIR}/include/
		${loft_base_SOURCE_DIR}/include/
		${loft_render_graph_SOURCE_DIR}/include
		${loft_common_SOURCE_DIR}/src/
		../../../
)

