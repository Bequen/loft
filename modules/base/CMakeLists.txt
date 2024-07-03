cmake_minimum_required(VERSION 3.25)

project(loft_base)




file(GLOB FILES ./src/*.cpp ./src/shaders/*.cpp ./src/io/*.cpp ./src/resources/*.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} ${FILES}
        ../common/src/error.h
        ../common/src/Display.h)
find_package(Vulkan REQUIRED)


target_link_libraries(${PROJECT_NAME} PRIVATE volk)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include/
        ${Vulkan_INCLUDE_DIR}
        ${VMA_SOURCE_DIR}/include
        ./
        ../common/src/
../../)