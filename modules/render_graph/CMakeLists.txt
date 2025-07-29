project(loft_render_graph)

file(GLOB CXXFILES src/*.cpp)
add_library(${PROJECT_NAME} STATIC ${CXXFILES})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan QUIET)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

target_link_libraries(${PROJECT_NAME} PRIVATE
        loft_base
        loft_common
        loft_window
        volk
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        include/
        ${Vulkan_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/include/
        ${loft_base_SOURCE_DIR}/include/
        ${loft_window_SOURCE_DIR}/include
        ${loft_common_SOURCE_DIR}/src/
        ${VMA_SOURCE_DIR}/include
        ../../
)

add_subdirectory(tests)
add_test(NAME RenderGraphBuilderTests COMMAND RenderGraphBuilderTests)
