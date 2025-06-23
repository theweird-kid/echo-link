cmake_minimum_required(VERSION 3.10)
project(echo-link LANGUAGES CXX)

if(WIN32)
    message(FATAL_ERROR "echo-link does not currently support building on Windows.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Find PortAudio
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
include_directories(${PORTAUDIO_INCLUDE_DIRS})
link_directories(${PORTAUDIO_LIBRARY_DIRS})

# Find Opus
pkg_check_modules(OPUS REQUIRED opus)
include_directories(${OPUS_INCLUDE_DIRS})
link_directories(${OPUS_LIBRARY_DIRS})

# Find ASIO (header-only, so just include directory)
# You may need to set ASIO_INCLUDE_DIR manually if not using Boost
set(ASIO_INCLUDE_DIR "/usr/include/asio" CACHE PATH "Path to ASIO include directory")
include_directories(${ASIO_INCLUDE_DIR})

# Add source files (adjust as needed)
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.cxx" "src/*.cc")
file(GLOB_RECURSE HEADERS "include/*.hpp" "include/*.h")

add_executable(echo-link ${SOURCES} ${HEADERS})

target_include_directories(echo-link PRIVATE include)
target_link_libraries(echo-link
    ${PORTAUDIO_LIBRARIES}
    ${OPUS_LIBRARIES}
    Threads::Threads
)

# Optionally, install target
install(TARGETS echo-link DESTINATION bin)
