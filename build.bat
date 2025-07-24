@echo off
REM Build script for Gaussian Extractor - Windows
REM Enhanced Safety Edition v0.3

setlocal EnableDelayedExpansion

echo Gaussian Extractor Build Script for Windows
echo ==========================================

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
set FLAGS=/std:c++20 /O2 /W4 /EHsc /D_WIN32_WINNT=0x0601
set LIBS=psapi.lib
set OUTPUT=/Fe:gaussian_extractor.x.exe

REM Clean previous build
if exist *.obj del *.obj
if exist gaussian_extractor.x.exe del gaussian_extractor.x.exe

echo Compiling...
%COMPILER% %FLAGS% main.cpp gaussian_extractor.cpp job_scheduler.cpp %LIBS% %OUTPUT%

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
set FLAGS=-std=c++20 -O2 -Wall -Wextra -pthread
set LIBS=-lpsapi
set OUTPUT=-o gaussian_extractor.x.exe

REM Clean previous build
if exist *.o del *.o
if exist gaussian_extractor.x.exe del gaussian_extractor.x.exe

echo Compiling...
%COMPILER% %FLAGS% main.cpp gaussian_extractor.cpp job_scheduler.cpp %LIBS% %OUTPUT%

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
set FLAGS=-std=c++20 -O2 -Wall -Wextra -pthread
set LIBS=-lpsapi
set OUTPUT=-o gaussian_extractor.x.exe

REM Clean previous build
if exist *.o del *.o
if exist gaussian_extractor.x.exe del gaussian_extractor.x.exe

echo Compiling...
%COMPILER% %FLAGS% main.cpp gaussian_extractor.cpp job_scheduler.cpp %LIBS% %OUTPUT%

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
        echo Usage examples:
        echo   gaussian_extractor.x.exe                    # Process all .log files
        echo   gaussian_extractor.x.exe -q                 # Quiet mode
        echo   gaussian_extractor.x.exe -nt half           # Use half CPU cores
        echo   gaussian_extractor.x.exe -f csv             # CSV output format
        echo   gaussian_extractor.x.exe --resource-info    # Show resource information
        echo   gaussian_extractor.x.exe --help             # Show full help
        echo.

        REM Check if test files exist
        if exist test-1.log (
            if exist test-2.log (
                echo Test files found. Running sample processing...
                gaussian_extractor.x.exe -q -f csv
                echo Sample processing completed.
            )
        ) else (
            echo Note: No test files found. Place .log files in this directory to process them.
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
pause
