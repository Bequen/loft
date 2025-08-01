project(loft_base)

file(GLOB FILES
        ./src/*.cpp
        ./src/shaders/*.cpp
        ./src/io/*.cpp
        ./src/resources/*.cpp)


file(GLOB HEADERS
        ./include/*.h
        ./include/*.hpp
        ./include/shaders/*.h
        ./include/shaders/*.hpp
        ./include/resources/*.h
        ./include/resources/*.hpp
        ./include/io/*.h
        ./include/io/*.hpp
        ./include/debug/*.h
        ./include/debug/*.hpp)

add_library(loft_base OBJECT ${FILES})
add_library(loft::base ALIAS loft_base)
target_sources(loft_base
    PUBLIC FILE_SET HEADERS
    BASE_DIRS include
    FILES ${HEADERS})

set_target_properties(loft_base PROPERTIES POSITION_INDEPENDENT_CODE 1)

find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME} 
    PUBLIC 
    GPUOpen::VulkanMemoryAllocator 
    volk::volk)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include/
)


