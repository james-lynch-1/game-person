cmake_minimum_required(VERSION 3.16)
project(game-person VERSION 0.1
                    DESCRIPTION "A Game Boy emulator for any person(s)"
                    LANGUAGES C)

include(CMakePrintHelpers)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in vendored/SDL
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
# cmake_print_variables(${CMAKE_SOURCE_DIR})

# Create your game executable target as usual

add_library(libgp SHARED ${CMAKE_SOURCE_DIR}/source/util.c
                         ${CMAKE_SOURCE_DIR}/source/ram.c
                         ${CMAKE_SOURCE_DIR}/source/registers.c
                         ${CMAKE_SOURCE_DIR}/source/rom.c
                         ${CMAKE_SOURCE_DIR}/source/opcodes.c
                         ${CMAKE_SOURCE_DIR}/source/global.c)
                         
target_include_directories(libgp PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_executable(gp ${CMAKE_SOURCE_DIR}/source/main.c)


# target_link_directories(libgp PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Link to the actual SDL3 library.
target_link_libraries(libgp PUBLIC SDL3::SDL3)
target_link_libraries(gp PRIVATE libgp)
