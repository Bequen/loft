cmake_minimum_required(VERSION 3.5)

find_package(loft_framegraph2)

add_executable(MainTest MainTest.cpp)
target_link_libraries(MainTest loft_framegraph2)
target_include_directories(MainTest PUBLIC ${loft_framegraph2_SOURCE_DIR}/include/)