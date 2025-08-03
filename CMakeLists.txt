# CMake configuration for Gaussian Extractor
cmake_minimum_required(VERSION 3.16)
project(GaussianExtractor
    VERSION 0.4.1
    DESCRIPTION "High-performance Gaussian log file processor"
    LANGUAGES CXX
)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

# Project options
option(ENABLE_EXTRA_WARNINGS "Enable extra compiler warnings" OFF)
option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(BUILD_FOR_CLUSTER "Build with cluster-specific optimizations" OFF)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Define source files
set(SOURCES
    src/main.cpp
    src/core/gaussian_extractor.cpp
    src/core/job_scheduler.cpp
    src/core/command_system.cpp
    src/core/job_checker.cpp
    src/core/command_executor.cpp
    src/core/config_manager.cpp
    src/core/high_level_energy.cpp
)

set(HEADERS
    src/core/gaussian_extractor.h
    src/core/job_scheduler.h
    src/core/command_system.h
    src/core/job_checker.h
    src/core/config_manager.h
    src/core/high_level_energy.h
    src/core/version.h
)

# Create the executable
add_executable(gaussian_extractor ${SOURCES} ${HEADERS})

# Set include directories
target_include_directories(gaussian_extractor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

# Find required packages
find_package(Threads REQUIRED)

# Compiler and platform specific settings
if(MSVC)
    target_compile_options(gaussian_extractor PRIVATE
        /W4
        /EHsc
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Od /Zi>
    )
    target_compile_definitions(gaussian_extractor PRIVATE
        _WIN32_WINNT=0x0601  # Windows 7 minimum
        _CRT_SECURE_NO_WARNINGS
    )
else()
    target_compile_options(gaussian_extractor PRIVATE
        -Wall
        -Wextra
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-O0 -g>
    )

    if(BUILD_FOR_CLUSTER)
        target_compile_options(gaussian_extractor PRIVATE
            -march=x86-64-v3  # Modern CPU architecture for clusters
            -mtune=generic
        )
    endif()

    if(ENABLE_EXTRA_WARNINGS)
        target_compile_options(gaussian_extractor PRIVATE
            -Wpedantic
            -Wcast-align
            -Wcast-qual
            -Wconversion
            -Wdouble-promotion
            -Wformat=2
            -Winit-self
            -Wlogical-op
            -Wmissing-declarations
            -Wmissing-include-dirs
            -Wold-style-cast
            -Woverloaded-virtual
            -Wredundant-decls
            -Wshadow
            -Wsign-conversion
            -Wswitch-default
            -Wundef
        )
    endif()
endif()

# Link dependencies
target_link_libraries(gaussian_extractor
    PRIVATE
        Threads::Threads
)

# Link stdc++fs for non-MSVC compilers if needed for std::filesystem
if(NOT MSVC)
    target_link_libraries(gaussian_extractor PRIVATE stdc++fs)
endif()

if(WIN32)
    target_link_libraries(gaussian_extractor PRIVATE psapi)
endif()

# Enable Address Sanitizer if requested
if(ENABLE_ASAN AND NOT MSVC)
    target_compile_options(gaussian_extractor PRIVATE
        -fsanitize=address
        -fno-omit-frame-pointer
    )
    target_link_options(gaussian_extractor PRIVATE
        -fsanitize=address
    )
endif()

# Set output name
set_target_properties(gaussian_extractor
    PROPERTIES
    OUTPUT_NAME "gaussian_extractor.x"
)

# Installation rules
include(GNUInstallDirs)
install(TARGETS gaussian_extractor
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install documentation
install(FILES
    README.MD
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

# Testing configuration (if tests are added in the future)
enable_testing()
# add_subdirectory(tests)  # Uncomment when tests are added

# Print configuration summary
message(STATUS "")
message(STATUS "Gaussian Extractor Configuration Summary")
message(STATUS "=======================================")
message(STATUS "Version:          ${PROJECT_VERSION}")
message(STATUS "Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard:     ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler:         ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Platform:         ${CMAKE_SYSTEM_NAME}")
message(STATUS "Install prefix:   ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  Extra warnings: ${ENABLE_EXTRA_WARNINGS}")
message(STATUS "  ASAN enabled:   ${ENABLE_ASAN}")
message(STATUS "  Cluster build:  ${BUILD_FOR_CLUSTER}")
message(STATUS "")
