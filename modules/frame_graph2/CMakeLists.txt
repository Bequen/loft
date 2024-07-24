cmake_minimum_required(VERSION 3.5)

project(loft_framegraph2)

file(GLOB CXXFILES src/*.cpp)
add_library(${PROJECT_NAME} STATIC ${CXXFILES})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan QUIET)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_link_libraries(${PROJECT_NAME} PRIVATE
        loft_base
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
        ${VMA_SOURCE_DIR}/include
        ../../
)

add_subdirectory(tests)
add_test(NAME MainTest COMMAND MainTest)