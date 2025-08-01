file(GLOB CXXFILES src/*.cpp)
add_library(loft_common OBJECT ${CXXFILES})
add_library(loft::common ALIAS loft_common)

target_include_directories(loft_common
	PUBLIC
	src/
)
