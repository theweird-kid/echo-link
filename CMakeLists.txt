# CMake configuration for echo-link: UDP-based voice chat app
cmake_minimum_required(VERSION 3.14)
project(echo-link LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optionally allow static or shared linking
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# Find Asio (Fedora: asio-devel, header-only)
find_path(ASIO_INCLUDE_DIR asio.hpp
    PATH_SUFFIXES asio
    DOC "Path to Asio headers"
)
if(NOT ASIO_INCLUDE_DIR)
    message(FATAL_ERROR "Asio headers not found. Please install asio-devel.")
endif()

# Find PortAudio (Fedora: portaudio-devel)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
if(NOT PORTAUDIO_FOUND)
    message(FATAL_ERROR "PortAudio not found. Please install portaudio-devel.")
endif()

# Find Opus (Fedora: opus-devel)
pkg_check_modules(OPUS REQUIRED opus)
if(NOT OPUS_FOUND)
    message(FATAL_ERROR "Opus library not found. Please install opus-devel.")
endif()

# Source files
file(GLOB_RECURSE SOURCES
    src/*.cc
    src/*.cpp
)

# Header files (optional, for IDEs)
file(GLOB_RECURSE HEADERS
    include/*.h
    include/*.hpp
)

add_executable(echo-link ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(echo-link
    PRIVATE
        ${ASIO_INCLUDE_DIR}
        ${PORTAUDIO_INCLUDE_DIRS}
        ${OPUS_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(echo-link
    PRIVATE
        ${PORTAUDIO_LIBRARIES}
        ${OPUS_LIBRARIES}
)

# Platform-specific settings
if(WIN32)
    target_link_libraries(echo-link PRIVATE ws2_32)
endif()

# Installation
install(TARGETS echo-link RUNTIME DESTINATION bin)

# Print summary
message(STATUS "echo-link build configuration:")
message(STATUS "  ASIO_INCLUDE_DIR: ${ASIO_INCLUDE_DIR}")
message(STATUS "  PORTAUDIO_INCLUDE_DIRS: ${PORTAUDIO_INCLUDE_DIRS}")
message(STATUS "  OPUS_INCLUDE_DIRS: ${OPUS_INCLUDE_DIRS}")
