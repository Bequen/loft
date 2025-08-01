project(loft_render_graph_tests)

add_executable(TopologicalSortTests TopologicalSortTests.cpp)
add_executable(RenderGraphBuilderTests RenderGraphBuilderTests.cpp)
add_executable(DependencyGraphTests DependencyGraphTests.cpp)

find_package(Vulkan QUIET)
find_package(SDL2 REQUIRED)

set(LIBS
    loft_render_graph
    loft_base
    loft_common
    loft_window
    ${SDL2_LIBRARIES}
    volk)

target_link_libraries(TopologicalSortTests PRIVATE ${LIBS})
target_link_libraries(RenderGraphBuilderTests PRIVATE ${LIBS})
target_link_libraries(DependencyGraphTests PRIVATE ${LIBS})

target_include_directories(TopologicalSortTests PUBLIC ${INCLUDE})
target_include_directories(RenderGraphBuilderTests PUBLIC ${INCLUDE})
target_include_directories(DependencyGraphTests PUBLIC ${INCLUDE})
