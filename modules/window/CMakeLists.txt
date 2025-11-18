project(loft_window)

file(GLOB FILES src/*.c src/*.cpp)

add_library(${PROJECT_NAME} ${FILES})

find_package(SDL2 REQUIRED)

set_target_properties(${PROJECT_NAME} PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/include/"
)

target_link_libraries(${PROJECT_NAME} 
    PUBLIC 
    loft_base
    ${SDL2_LIBRARIES}
)

target_include_directories(${PROJECT_NAME}
	PUBLIC
	${PROJECT_SOURCE_DIR}/include/
)
