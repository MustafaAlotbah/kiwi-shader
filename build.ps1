# build.ps1 - Build and run the application

param(
    [switch]$NoBuild,
    [switch]$NoRun,
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

$projectRoot = $PSScriptRoot
$buildDir = Join-Path $projectRoot "build"
$exeName = "opengl_template.exe"

Write-Host "=== Kiwi Graphics Framework - Build ===" -ForegroundColor Cyan

# Check if build directory exists
if (-not (Test-Path $buildDir)) {
    Write-Host "Build directory not found. Run configure.ps1 first." -ForegroundColor Red
    exit 1
}

# Build
if (-not $NoBuild) {
    Write-Host "Building ($Config)..." -ForegroundColor Yellow
    cmake --build $buildDir --config $Config
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
    Write-Host "Build complete!" -ForegroundColor Green
    Write-Host ""
}

# Run
if (-not $NoRun) {
    # Find executable (handles both single-config and multi-config generators)
    $exePath = Join-Path $buildDir $exeName
    if (-not (Test-Path $exePath)) {
        $exePath = Join-Path $buildDir "$Config\$exeName"
    }
    
    if (Test-Path $exePath) {
        Write-Host "Running $exeName..." -ForegroundColor Yellow
        Write-Host "----------------------------------------"
        & $exePath
    }
    else {
        Write-Host "Executable not found at expected locations:" -ForegroundColor Red
        Write-Host "  - $buildDir\$exeName"
        Write-Host "  - $buildDir\$Config\$exeName"
        exit 1
    }
}

