cmake_minimum_required(VERSION 3.5)

project(loft_framegraph)

file(GLOB CXXFILES src/*.cpp)
add_library(${PROJECT_NAME} STATIC ${CXXFILES})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


target_link_libraries(${PROJECT_NAME}
        loft_base
        loft_window
)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include/)
