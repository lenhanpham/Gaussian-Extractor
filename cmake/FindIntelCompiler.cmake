# FindIntelCompiler.cmake
# CMake module for detecting and configuring Intel compilers
# Supports: icpx (oneAPI), icpc (classic), icc (classic), with g++ fallback

include(CheckCXXCompilerFlag)

# Function to detect Intel compiler
function(detect_intel_compiler)
    # Check if user explicitly set Intel compiler
    if(DEFINED INTEL_COMPILER)
        set(CMAKE_CXX_COMPILER ${INTEL_COMPILER} PARENT_SCOPE)
        set(INTEL_COMPILER_DETECTED ${INTEL_COMPILER} PARENT_SCOPE)
        message(STATUS "Using user-specified compiler: ${INTEL_COMPILER}")
        return()
    endif()
    
    # Auto-detection priority order
    message(STATUS "=== Compiler Auto-Detection Enable ===")
    
    # Check for common Intel compilers (Windows and Unix)
    find_program(ICPX_COMPILER icpx)
    find_program(ICPC_COMPILER icpc)  
    find_program(ICC_COMPILER icc)
    find_program(ICL_COMPILER icl)  # Windows Intel compiler
    find_program(ICX_COMPILER icx)  # Windows oneAPI compiler
    find_program(GXX_COMPILER g++)
    
    # Prioritize Intel compilers but fallback to g++ if none found
    if(ICPX_COMPILER)
        set(CMAKE_CXX_COMPILER ${ICPX_COMPILER} PARENT_SCOPE)
        set(INTEL_COMPILER_DETECTED "icpx" PARENT_SCOPE)
        set(INTEL_COMPILER_VERSION "oneAPI" PARENT_SCOPE)
        set(INTEL_COMPILER_NAME "Intel oneAPI DPC++/C++ Compiler" PARENT_SCOPE)
        message(STATUS "→ Found Intel oneAPI DPC++/C++ Compiler: ${ICPX_COMPILER}")
    elseif(ICX_COMPILER)
        set(CMAKE_CXX_COMPILER ${ICX_COMPILER} PARENT_SCOPE)
        set(INTEL_COMPILER_DETECTED "icx" PARENT_SCOPE)
        set(INTEL_COMPILER_VERSION "oneAPI" PARENT_SCOPE)
        set(INTEL_COMPILER_NAME "Intel oneAPI C++ Compiler" PARENT_SCOPE)
        message(STATUS "→ Found Intel oneAPI C++ Compiler: ${ICX_COMPILER}")
    elseif(ICPC_COMPILER)
        set(CMAKE_CXX_COMPILER ${ICPC_COMPILER} PARENT_SCOPE)
        set(INTEL_COMPILER_DETECTED "icpc" PARENT_SCOPE)
        set(INTEL_COMPILER_VERSION "classic" PARENT_SCOPE)
        set(INTEL_COMPILER_NAME "Intel C++ Compiler Classic" PARENT_SCOPE)
        message(STATUS "→ Found Intel C++ Compiler Classic: ${ICPC_COMPILER}")
    elseif(ICL_COMPILER)
        set(CMAKE_CXX_COMPILER ${ICL_COMPILER} PARENT_SCOPE)
        set(INTEL_COMPILER_DETECTED "icl" PARENT_SCOPE)
        set(INTEL_COMPILER_VERSION "classic" PARENT_SCOPE)
        set(INTEL_COMPILER_NAME "Intel C++ Compiler (Windows)" PARENT_SCOPE)
        message(STATUS "→ Found Intel C++ Compiler (Windows): ${ICL_COMPILER}")
    elseif(ICC_COMPILER)
        set(CMAKE_CXX_COMPILER ${ICC_COMPILER} PARENT_SCOPE)
        set(INTEL_COMPILER_DETECTED "icc" PARENT_SCOPE)
        set(INTEL_COMPILER_VERSION "classic" PARENT_SCOPE)
        set(INTEL_COMPILER_NAME "Intel C Compiler Classic" PARENT_SCOPE)
        message(STATUS "→ Found Intel C Compiler Classic: ${ICC_COMPILER}")
    elseif(GXX_COMPILER)
        # g++ is available but we only set it as detected, not necessarily use it
        set(INTEL_COMPILER_DETECTED "g++" PARENT_SCOPE)
        set(INTEL_COMPILER_VERSION "gnu" PARENT_SCOPE)
        set(INTEL_COMPILER_NAME "GNU GCC" PARENT_SCOPE)
        message(STATUS "→ Found GNU GCC: ${GXX_COMPILER}")
        # Check if CMAKE_CXX_COMPILER is not already set to an Intel compiler
        if(NOT CMAKE_CXX_COMPILER)
            set(CMAKE_CXX_COMPILER ${GXX_COMPILER} PARENT_SCOPE)
            message(STATUS "→ Using GNU GCC as fallback: ${GXX_COMPILER}")
        endif()
    else()
        # Fallback to system default if no specific compilers found
        set(INTEL_COMPILER_DETECTED "g++" PARENT_SCOPE) 
        set(INTEL_COMPILER_VERSION "gnu" PARENT_SCOPE)
        set(INTEL_COMPILER_NAME "GNU GCC" PARENT_SCOPE)
        message(STATUS "→ No specific compilers found, using CMAKE default: ${CMAKE_CXX_COMPILER}")
    endif()
endfunction()

# Function to configure Intel compiler flags
function(configure_intel_compiler target)
    # Get compiler ID and version
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
        set(IS_INTEL_COMPILER TRUE)
        message(STATUS "Configuring for Intel compiler")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
        set(IS_INTEL_COMPILER TRUE)
        set(IS_INTEL_ONEAPI TRUE)
        message(STATUS "Configuring for Intel oneAPI compiler")
    else()
        set(IS_INTEL_COMPILER FALSE)
    endif()
    
    if(IS_INTEL_COMPILER)
        # Intel compiler specific settings
        if(IS_INTEL_ONEAPI)
            # Check Intel oneAPI version to handle 2025.x compatibility issues
            execute_process(
                COMMAND ${CMAKE_CXX_COMPILER} --version
                OUTPUT_VARIABLE INTEL_VERSION_OUTPUT
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            string(REGEX MATCH "[0-9]{4}\.[0-9]+\.[0-9]+" INTEL_VERSION "${INTEL_VERSION_OUTPUT}")
            string(REGEX MATCH "^[0-9]{4}" INTEL_MAJOR_VERSION "${INTEL_VERSION}")
            
            if(INTEL_MAJOR_VERSION GREATER_EQUAL 2025)
                message(STATUS "Intel oneAPI ${INTEL_VERSION} detected - disabling IPO for compatibility")
                # Intel oneAPI 2025.x and newer - avoid IPO which causes object file format issues
                target_compile_options(${target} PRIVATE
                    $<$<CONFIG:Release>:-O3 -xHost -no-prec-div -fp-model fast=2>
                    $<$<CONFIG:Debug>:-O0 -g -debug all -check=stack,uninit>
                    $<$<CONFIG:RelWithDebInfo>:-O2 -g -xHost>
                    $<$<CONFIG:MinSizeRel>:-Os -xHost>
                )
                # Add stdc++fs library for std::filesystem support in Intel oneAPI 2025.x
                target_link_libraries(${target} PRIVATE stdc++fs)
            else()
                # Intel oneAPI 2024.x and older - use IPO
                target_compile_options(${target} PRIVATE
                    $<$<CONFIG:Release>:-O3 -xHost -ipo -no-prec-div -fp-model fast=2>
                    $<$<CONFIG:Debug>:-O0 -g -debug all -check=stack,uninit>
                    $<$<CONFIG:RelWithDebInfo>:-O2 -g -xHost>
                    $<$<CONFIG:MinSizeRel>:-Os -xHost>
                )
            endif()
            
            # OpenMP support for oneAPI
            target_compile_options(${target} PRIVATE -qopenmp)
            target_link_options(${target} PRIVATE -qopenmp)
            target_link_options(${target} PRIVATE -qopenmp)
        else()
            # Intel Classic compiler flags
            target_compile_options(${target} PRIVATE
                $<$<CONFIG:Release>:-O3 -xHost -ipo -no-prec-div -fp-model fast=2>
                $<$<CONFIG:Debug>:-O0 -g -debug all -check=stack,uninit>
                $<$<CONFIG:RelWithDebInfo>:-O2 -g -xHost>
                $<$<CONFIG:MinSizeRel>:-Os -xHost>
            )
            
            # OpenMP support for classic compiler
            target_compile_options(${target} PRIVATE -qopenmp)
            target_link_options(${target} PRIVATE -qopenmp)
        endif()
        
        # Intel specific warning flags
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wcheck
        )
        
        # Architecture-specific optimizations
        if(BUILD_FOR_CLUSTER)
            # Conservative optimizations for cluster deployment
            target_compile_options(${target} PRIVATE
                $<$<CONFIG:Release>:-xSSE4.2 -fp-model precise>
            )
        endif()
        
        # Intel compiler specific definitions
        target_compile_definitions(${target} PRIVATE
            INTEL_COMPILER
            $<$<BOOL:${IS_INTEL_ONEAPI}>:INTEL_ONEAPI>
        )
        
    else()
        # Non-Intel compiler (GNU, Clang, etc.)
        target_compile_options(${target} PRIVATE
            -Wall -Wextra
            $<$<CONFIG:Release>:-O3 -march=native -mtune=native>
            $<$<CONFIG:Debug>:-O0 -g>
            $<$<CONFIG:RelWithDebInfo>:-O2 -g -march=native>
            $<$<CONFIG:MinSizeRel>:-Os>
        )
        
        if(BUILD_FOR_CLUSTER)
            target_compile_options(${target} PRIVATE
                $<$<CONFIG:Release>:-march=x86-64 -mtune=generic>
                $<$<CONFIG:RelWithDebInfo>:-march=x86-64 -mtune=generic>
            )
        endif()
        
        # Try to enable OpenMP if available
        find_package(OpenMP QUIET)
        if(OpenMP_CXX_FOUND)
            target_link_libraries(${target} PRIVATE OpenMP::OpenMP_CXX)
        endif()
    endif()
endfunction()

# Function to print compiler configuration summary
function(print_compiler_summary)
    message(STATUS "")
    message(STATUS "=== Compiler Configuration Summary ===")
    message(STATUS "Detected Compiler: ${INTEL_COMPILER_NAME}")
    message(STATUS "Compiler Command:  ${CMAKE_CXX_COMPILER}")
    message(STATUS "Compiler ID:       ${CMAKE_CXX_COMPILER_ID}")
    message(STATUS "Compiler Version:  ${CMAKE_CXX_COMPILER_VERSION}")
    
    # Display Intel oneAPI version and IPO status if using Intel oneAPI
    if(CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} --version
            OUTPUT_VARIABLE INTEL_VERSION_OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(REGEX MATCH "[0-9]{4}\.[0-9]+\.[0-9]+" INTEL_VERSION "${INTEL_VERSION_OUTPUT}")
        string(REGEX MATCH "^[0-9]{4}" INTEL_MAJOR_VERSION "${INTEL_VERSION}")
        
        message(STATUS "Intel oneAPI Ver:  ${INTEL_VERSION}")
        if(INTEL_MAJOR_VERSION GREATER_EQUAL 2025)
            message(STATUS "IPO Status:        Disabled (for 2025.x compatibility)")
            message(STATUS "std::filesystem:    Added -lstdc++fs for 2025.x compatibility")
        else()
            message(STATUS "IPO Status:        Enabled")
        endif()
    endif()
    
    message(STATUS "Build Type:        ${CMAKE_BUILD_TYPE}")
    if(BUILD_FOR_CLUSTER)
        message(STATUS "Cluster Build:     Enabled")
    endif()
    message(STATUS "===================================")
    message(STATUS "")
endfunction()

# Export variables for use in main CMakeLists.txt
set(INTEL_COMPILER_FOUND FALSE)
if(INTEL_COMPILER_DETECTED AND NOT INTEL_COMPILER_DETECTED STREQUAL "g++")
    set(INTEL_COMPILER_FOUND TRUE)
endif()

# Make variables available to parent scope
if(INTEL_COMPILER_DETECTED)
    set(INTEL_COMPILER_DETECTED ${INTEL_COMPILER_DETECTED} PARENT_SCOPE)
    set(INTEL_COMPILER_VERSION ${INTEL_COMPILER_VERSION} PARENT_SCOPE)
    set(INTEL_COMPILER_NAME ${INTEL_COMPILER_NAME} PARENT_SCOPE)
    set(INTEL_COMPILER_FOUND ${INTEL_COMPILER_FOUND} PARENT_SCOPE)
endif()
