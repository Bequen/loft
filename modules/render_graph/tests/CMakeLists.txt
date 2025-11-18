project(loft_render_graph_tests)

find_package(Vulkan QUIET)
find_package(SDL2 REQUIRED)
find_package(Catch2 REQUIRED)

set(LIBS
    loft_render_graph
    loft_base
    loft_common
    loft_window
    ${SDL2_LIBRARIES}
    volk)

set(FILES 
    # ./DependencyGraphTests.cpp 
    ./RenderGraphBuilderTests.cpp 
    ./TopologicalSortTests.cpp
)

add_library(render_graph_unit_tests_sources OBJECT Mock.cpp ${FILES})
target_link_libraries(render_graph_unit_tests_sources
    Catch2::Catch2WithMain
    ${LIBS}
)

add_executable(render_graph_unit_tests)

target_link_libraries(render_graph_unit_tests 
    PRIVATE 
    ${LIBS} 
    render_graph_unit_tests_sources
    Catch2::Catch2WithMain
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)

catch_discover_tests(render_graph_unit_tests)
