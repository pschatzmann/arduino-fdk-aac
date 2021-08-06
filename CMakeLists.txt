cmake_minimum_required(VERSION 3.16)

# set the project name
project(fdk_aac)

# lots of warnings and all warnings as errors
## add_compile_options(-Wall -Wextra )
set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRC_LIST_CPP CONFIGURE_DEPENDS  "${PROJECT_SOURCE_DIR}/src/*.cpp" )

# define libraries
add_library (fdk_aac ${SRC_LIST_CPP})

# prevent compile errors
#target_compile_options(fdk_aac PRIVATE -DUSE_DEFAULT_STDLIB)

# define location for header files
target_include_directories(fdk_aac PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src )

# build examples
add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/examples/decode")
add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/examples/encode")
