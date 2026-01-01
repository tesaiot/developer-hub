cmake_minimum_required(VERSION 3.10)
project(mqtt_quic_client C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

# Find NanoSDK (NNG) library
find_package(nng CONFIG QUIET)

if(NOT nng_FOUND)
    # Try pkg-config as fallback
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(NNG nng)
    endif()
endif()

# Check if NNG was found
if(nng_FOUND OR NNG_FOUND)
    message(STATUS "Found NanoSDK/NNG library")
    if(NNG_INCLUDE_DIRS)
        include_directories(${NNG_INCLUDE_DIRS})
    endif()
else()
    message(WARNING "NanoSDK/NNG not found via CMake or pkg-config")
    message(STATUS "Attempting manual include path search...")

    # Common installation paths
    set(NNG_SEARCH_PATHS
        /usr/local/include
        /usr/include
        /opt/homebrew/include
        $ENV{HOME}/.local/include
    )

    find_path(NNG_INCLUDE_DIR
        NAMES nng/nng.h
        PATHS ${NNG_SEARCH_PATHS}
    )

    if(NNG_INCLUDE_DIR)
        message(STATUS "Found NNG headers at: ${NNG_INCLUDE_DIR}")
        include_directories(${NNG_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "NanoSDK/NNG headers not found. Please install NanoSDK:")
        message(FATAL_ERROR "  git clone https://github.com/nanomq/NanoSDK.git")
        message(FATAL_ERROR "  cd NanoSDK && mkdir build && cd build")
        message(FATAL_ERROR "  cmake -DBUILD_SHARED_LIBS=ON .. && make && sudo make install")
    endif()
endif()

# Add executable
add_executable(mqtt_quic_client mqtt_quic_client.c)

# Link libraries
if(nng_FOUND)
    target_link_libraries(mqtt_quic_client nng::nng)
elseif(NNG_FOUND)
    target_link_libraries(mqtt_quic_client ${NNG_LIBRARIES})
else()
    # Manual library search
    find_library(NNG_LIBRARY
        NAMES nng
        PATHS /usr/local/lib /usr/lib /opt/homebrew/lib $ENV{HOME}/.local/lib
    )

    if(NNG_LIBRARY)
        message(STATUS "Found NNG library at: ${NNG_LIBRARY}")
        target_link_libraries(mqtt_quic_client ${NNG_LIBRARY})
    else()
        message(FATAL_ERROR "NNG library not found")
    endif()
endif()

# Link pthread (required by NNG)
find_package(Threads REQUIRED)
target_link_libraries(mqtt_quic_client Threads::Threads)

# Installation
install(TARGETS mqtt_quic_client DESTINATION bin)

# Print build information
message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
