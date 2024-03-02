cmake_minimum_required(VERSION 3.25)

project(loft_base)

file(GLOB FILES ./src/*.cpp ./src/shaders/*.cpp ./src/io/*.cpp ./src/resources/*.cpp)

add_library(${PROJECT_NAME} ${FILES})

target_include_directories(${PROJECT_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}/include/
        ${VMA_SOURCE_DIR}/include/)