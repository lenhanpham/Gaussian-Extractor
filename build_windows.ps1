# PowerShell build script for Gaussian Extractor with icon support

Write-Host "Building Gaussian Extractor for Windows..." -ForegroundColor Green
Write-Host "Using CMake with custom icon support" -ForegroundColor Yellow
Write-Host ""

# Create build directory
if (!(Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Navigate to build directory
Push-Location build

try {
    # Configure with CMake
    Write-Host "Configuring with CMake..." -ForegroundColor Cyan
    $cmakeResult = & cmake .. -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error configuring with CMake" -ForegroundColor Red
        exit 1
    }

    # Build the project
    Write-Host ""
    Write-Host "Building project..." -ForegroundColor Cyan
    $buildResult = & cmake --build . --config Release
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error building project" -ForegroundColor Red
        exit 1
    }

    Write-Host ""
    Write-Host "Build completed successfully!" -ForegroundColor Green
    Write-Host "Executable location: build\bin\Release\gaussian_extractor.x.exe" -ForegroundColor White
    Write-Host ""
    Write-Host "The executable includes a custom icon from resources/app_icon.ico" -ForegroundColor Yellow
    Write-Host "It will display with your custom icon in Windows Explorer." -ForegroundColor Yellow

    # Test the executable
    Write-Host ""
    Write-Host "Testing executable..." -ForegroundColor Cyan
    $testResult = & ".\bin\Release\gaussian_extractor.x.exe" --version
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Warning: Executable test failed, but binary was created" -ForegroundColor Yellow
    } else {
        Write-Host "Executable test passed!" -ForegroundColor Green
    }

} finally {
    # Return to original directory
    Pop-Location
}

Write-Host ""
Write-Host "Build completed with icon support!" -ForegroundColor Green
Write-Host "You can now distribute the executable to Windows users." -ForegroundColor White
Write-Host ""

# Pause equivalent in PowerShell
Read-Host "Press Enter to continue"