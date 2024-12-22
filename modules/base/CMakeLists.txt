cmake_minimum_required(VERSION 3.5)

project(loft_base)

file(GLOB FILES
        ./src/*.cpp
        ./src/shaders/*.cpp
        ./src/io/*.cpp
        ./src/resources/*.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} STATIC ${FILES})
find_package(Vulkan REQUIRED)


target_link_libraries(${PROJECT_NAME} PRIVATE volk)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include/
        PRIVATE
        ${Vulkan_INCLUDE_DIR}
        ${VMA_SOURCE_DIR}/include
        ./
        ../common/src/
        ../../
)
