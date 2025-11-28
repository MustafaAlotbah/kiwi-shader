# configure.ps1 - Configure CMake for the project

$ErrorActionPreference = "Stop"

$projectRoot = $PSScriptRoot
$sourceDir = Join-Path $projectRoot "source"
$buildDir = Join-Path $projectRoot "build"

Write-Host "=== Kiwi Graphics Framework - Configure ===" -ForegroundColor Cyan
Write-Host "Source: $sourceDir"
Write-Host "Build:  $buildDir"
Write-Host ""

# Create build directory if it doesn't exist
if (-not (Test-Path $buildDir)) {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# Run CMake configure
Write-Host "Running CMake configure..." -ForegroundColor Yellow
Push-Location $buildDir
try {
    cmake $sourceDir -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed with exit code $LASTEXITCODE"
    }
    Write-Host ""
    Write-Host "Configuration complete!" -ForegroundColor Green
}
finally {
    Pop-Location
}

