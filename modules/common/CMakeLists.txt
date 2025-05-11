cmake_minimum_required(VERSION 3.5)

project(loft_common)

file(GLOB CXXFILES src/*.cpp)
add_library(${PROJECT_NAME} STATIC ${CXXFILES})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_include_directories(${PROJECT_NAME}
	PUBLIC
	src/
)
