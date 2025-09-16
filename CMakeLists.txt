cmake_minimum_required(VERSION 3.16)
project(game-person VERSION 0.1
                    DESCRIPTION "A Game Boy emulator for any person(s)"
                    LANGUAGES C)

# include(CMakePrintHelpers)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
# This assumes the SDL source is available in vendored/SDL
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
# cmake_print_variables(${CMAKE_SOURCE_DIR})

IF(WIN32)
    add_executable(gp WIN32 ${CMAKE_SOURCE_DIR}/source/main.c)
ELSE()
    add_executable(gp ${CMAKE_SOURCE_DIR}/source/main.c)
ENDIF()

target_include_directories(gp PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_sources(gp PRIVATE
    ${CMAKE_SOURCE_DIR}/source/cpu.c
    ${CMAKE_SOURCE_DIR}/source/global.c
    ${CMAKE_SOURCE_DIR}/source/input.c
    ${CMAKE_SOURCE_DIR}/source/interrupts.c
    ${CMAKE_SOURCE_DIR}/source/ppu.c
    ${CMAKE_SOURCE_DIR}/source/serial.c
    ${CMAKE_SOURCE_DIR}/source/timer.c
    ${CMAKE_SOURCE_DIR}/source/util.c
)
target_link_libraries(gp PRIVATE SDL3::SDL3)
