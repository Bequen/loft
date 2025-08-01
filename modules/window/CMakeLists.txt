project(loft_window)

file(GLOB FILES src/*.c src/*.cpp)

add_library(${PROJECT_NAME} ${FILES})
find_package(SDL2 QUIET)

set_target_properties(${PROJECT_NAME} PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/include/"
)

target_link_libraries(${PROJECT_NAME} loft_base)

target_include_directories(${PROJECT_NAME}
	PUBLIC
	${PROJECT_SOURCE_DIR}/include/
)
