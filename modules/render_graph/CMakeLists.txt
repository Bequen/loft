file(GLOB CXXFILES src/*.cpp)
add_library(loft_render_graph OBJECT ${CXXFILES})
add_library(loft::render_graph ALIAS loft_render_graph)

target_link_libraries(loft_render_graph 
        PUBLIC 
        loft::common
        loft::base
        loft_window
        volk::volk
)

target_include_directories(loft_render_graph
        PUBLIC
        include/
)

add_subdirectory(tests)
add_test(NAME RenderGraphBuilderTests COMMAND RenderGraphBuilderTests)
