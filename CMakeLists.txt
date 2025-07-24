cmake_minimum_required(VERSION 3.16)
project(GaussianExtractor VERSION 0.3.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4 /O2)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 minimum
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Create the executable
add_executable(gaussian_extractor
    main.cpp
    gaussian_extractor.cpp
    job_scheduler.cpp
    gaussian_extractor.h
    job_scheduler.h
)

# Link libraries
target_link_libraries(gaussian_extractor
    PRIVATE
    Threads::Threads
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(gaussian_extractor PRIVATE psapi)
endif()

# Compiler feature requirements
target_compile_features(gaussian_extractor
    PRIVATE
    cxx_std_20
)

# Set output name
set_target_properties(gaussian_extractor
    PROPERTIES
    OUTPUT_NAME "gaussian_extractor.x"
)

# Installation
install(TARGETS gaussian_extractor
    RUNTIME DESTINATION bin
)

# Create a configuration summary
message(STATUS "")
message(STATUS "Gaussian Extractor Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "")

# Optional: Create a debug build with additional safety checks
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(gaussian_extractor PRIVATE DEBUG_BUILD)
    if(NOT MSVC)
        target_compile_options(gaussian_extractor PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(gaussian_extractor PRIVATE -fsanitize=address)
    endif()
endif()

# Optional: Enable additional warnings for development
option(ENABLE_EXTRA_WARNINGS "Enable extra compiler warnings" OFF)
if(ENABLE_EXTRA_WARNINGS AND NOT MSVC)
    target_compile_options(gaussian_extractor PRIVATE
        -Wpedantic
        -Wcast-align
        -Wcast-qual
        -Wctor-dtor-privacy
        -Wdisabled-optimization
        -Wformat=2
        -Winit-self
        -Wlogical-op
        -Wmissing-declarations
        -Wmissing-include-dirs
        -Wnoexcept
        -Wold-style-cast
        -Woverloaded-virtual
        -Wredundant-decls
        -Wshadow
        -Wsign-conversion
        -Wsign-promo
        -Wstrict-null-sentinel
        -Wstrict-overflow=5
        -Wswitch-default
        -Wundef
    )
endif()
