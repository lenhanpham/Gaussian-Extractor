@echo off
REM Build script for Gaussian Extractor - Windows
REM Enhanced Safety Edition v0.3.1 with Job Management Commands

setlocal EnableDelayedExpansion

echo Gaussian Extractor Build Script for Windows
echo ==========================================

REM Set source directories
set SRC_DIR=src
set CORE_DIR=%SRC_DIR%\core

REM Source files
set SOURCES=%SRC_DIR%\main.cpp %CORE_DIR%\gaussian_extractor.cpp %CORE_DIR%\job_scheduler.cpp %CORE_DIR%\command_system.cpp %CORE_DIR%\job_checker.cpp %CORE_DIR%\command_executor.cpp %CORE_DIR%\config_manager.cpp %CORE_DIR%\high_level_energy.cpp

REM Include directories
set INCLUDES=-I%SRC_DIR% -I%CORE_DIR%

REM Check for Visual Studio compiler
where cl >nul 2>nul
if %ERRORLEVEL% == 0 (
    echo Found Visual Studio compiler (MSVC)
    goto :build_msvc
)

REM Check for MinGW compiler
where g++ >nul 2>nul
if %ERRORLEVEL% == 0 (
    echo Found MinGW compiler (g++)
    goto :build_mingw
)

REM Check for clang
where clang++ >nul 2>nul
if %ERRORLEVEL% == 0 (
    echo Found Clang compiler (clang++)
    goto :build_clang
)

echo ERROR: No suitable C++ compiler found!
echo Please install one of the following:
echo   - Visual Studio 2019/2022 with C++ support
echo   - MinGW-w64
echo   - Clang/LLVM
echo.
echo For MinGW, you can install via:
echo   - MSYS2: pacman -S mingw-w64-x86_64-gcc
echo   - Or download from: https://www.mingw-w64.org/
pause
exit /b 1

:build_msvc
echo Building with Visual Studio (MSVC)...
set COMPILER=cl
set FLAGS=/std:c++20 /O2 /W4 /EHsc /D_WIN32_WINNT=0x0601 %INCLUDES%
set LIBS=psapi.lib
set OUTPUT=/Fe:gaussian_extractor.x.exe

REM Clean previous build
if exist *.obj del *.obj
if exist gaussian_extractor.x.exe del gaussian_extractor.x.exe

echo Compiling source files...
echo Sources: %SOURCES%
%COMPILER% %FLAGS% %SOURCES% %LIBS% %OUTPUT%

if %ERRORLEVEL% == 0 (
    echo Build successful! Created gaussian_extractor.x.exe
    goto :test_build
) else (
    echo Build failed with MSVC
    pause
    exit /b 1
)

:build_mingw
echo Building with MinGW (g++)...
set COMPILER=g++
set FLAGS=-std=c++20 -O2 -Wall -Wextra -pthread %INCLUDES%
set LIBS=-lpsapi
set OUTPUT=-o gaussian_extractor.x.exe

REM Clean previous build
if exist *.o del *.o
if exist gaussian_extractor.x.exe del gaussian_extractor.x.exe

echo Compiling source files...
echo Sources: %SOURCES%
%COMPILER% %FLAGS% %SOURCES% %LIBS% %OUTPUT%

if %ERRORLEVEL% == 0 (
    echo Build successful! Created gaussian_extractor.x.exe
    goto :test_build
) else (
    echo Build failed with MinGW
    pause
    exit /b 1
)

:build_clang
echo Building with Clang...
set COMPILER=clang++
set FLAGS=-std=c++20 -O2 -Wall -Wextra -pthread %INCLUDES%
set LIBS=-lpsapi
set OUTPUT=-o gaussian_extractor.x.exe

REM Clean previous build
if exist *.o del *.o
if exist gaussian_extractor.x.exe del gaussian_extractor.x.exe

echo Compiling source files...
echo Sources: %SOURCES%
%COMPILER% %FLAGS% %SOURCES% %LIBS% %OUTPUT%

if %ERRORLEVEL% == 0 (
    echo Build successful! Created gaussian_extractor.x.exe
    goto :test_build
) else (
    echo Build failed with Clang
    pause
    exit /b 1
)

:test_build
echo.
echo Testing the build...
if exist gaussian_extractor.x.exe (
    echo Running basic functionality test...
    gaussian_extractor.x.exe --help

    if %ERRORLEVEL% == 0 (
        echo.
        echo SUCCESS: Build and basic test completed!
        echo.
        echo The executable 'gaussian_extractor.x.exe' is ready to use.
        echo.
        echo NEW COMMANDS AVAILABLE:
        echo   gaussian_extractor.x.exe                    # Extract energies (default)
        echo   gaussian_extractor.x.exe extract           # Extract energies (explicit)
        echo   gaussian_extractor.x.exe done              # Check completed jobs
        echo   gaussian_extractor.x.exe errors            # Check error jobs
        echo   gaussian_extractor.x.exe pcm               # Check PCM failures
        echo   gaussian_extractor.x.exe check             # Run all checks
        echo.
        echo COMMON OPTIONS:
        echo   -q                       # Quiet mode
        echo   -nt half                 # Use half CPU cores
        echo   -nt 4                    # Use 4 threads
        echo   --max-file-size 200      # Handle larger files (MB)
        echo.
        echo EXTRACT COMMAND OPTIONS:
        echo   -t 300                   # Temperature (K)
        echo   -c 2                     # Concentration (M)
        echo   -f csv                   # CSV output format
        echo   --resource-info          # Show system resources
        echo.
        echo JOB MANAGEMENT OPTIONS:
        echo   --target-dir my-errors   # Custom directory name
        echo   --dir-suffix completed   # Custom done directory suffix
        echo   --show-details           # Show error details
        echo.

        REM Test new commands if log files exist
        if exist *.log (
            echo Test .log files found. Testing new commands...
            echo.
            echo Testing 'done' command:
            gaussian_extractor.x.exe done -q
            echo.
            echo Testing 'errors' command:
            gaussian_extractor.x.exe errors -q
            echo.
            echo Testing 'pcm' command:
            gaussian_extractor.x.exe pcm -q
            echo.
            echo Command tests completed.
        ) else (
            echo Note: No .log files found. Place Gaussian log files in this directory to test job management commands.
        )
    ) else (
        echo WARNING: Build succeeded but executable test failed.
        echo The program may have runtime issues.
    )
) else (
    echo ERROR: Executable not found after build.
)

echo.
echo Build process completed.
echo.
echo For more information, see:
echo   - README.MD for general usage
echo   - COMMANDS.md for detailed command documentation
echo.
pause
