# Windows 冒烟测试入口：CMake 配置 → 编译 → ctest → 用编译好的 toyc 编译单个 .tc 源文件，
# 验证编译器端到端管线是否正常工作。

param(
    [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"

cmake -S . -B $BuildDir
cmake --build $BuildDir
ctest --test-dir $BuildDir --output-on-failure

$compilerCandidates = @(
    Join-Path $BuildDir "toyc.exe",
    Join-Path $BuildDir "toyc",
    Join-Path $BuildDir "Debug\toyc.exe",
    Join-Path $BuildDir "Release\toyc.exe"
)

$compiler = $compilerCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $compiler) {
    throw "Compiler executable not found in $BuildDir"
}

$outDir = Join-Path $BuildDir "smoke"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$inputFile = "tests\basic\return_7.tc"
$outputFile = Join-Path $outDir "return_7.s"
Get-Content -LiteralPath $inputFile -Raw | & $compiler > $outputFile

if (-not (Test-Path -LiteralPath $outputFile)) {
    throw "Smoke test did not create output file"
}

if ((Get-Item -LiteralPath $outputFile).Length -eq 0) {
    throw "Smoke test produced empty output"
}

Write-Host "Smoke tests passed with compiler: $compiler"
