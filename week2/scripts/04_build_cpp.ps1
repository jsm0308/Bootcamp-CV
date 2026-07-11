$ErrorActionPreference = "Stop"

$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $ProjectRoot

$VcVarsCandidates = @(
  "C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
  "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
  "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
  "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
  "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
)

$VcVars = $null
foreach ($Candidate in $VcVarsCandidates) {
  if (Test-Path -LiteralPath $Candidate) {
    $VcVars = $Candidate
    break
  }
}

$CMake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $CMake) {
  $CMakePath = "C:\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
  if (Test-Path -LiteralPath $CMakePath) {
    $CMake = [pscustomobject]@{ Source = $CMakePath }
  }
}
if (-not $CMake) {
  $CMakePath = "C:\Program Files\CMake\bin\cmake.exe"
  if (Test-Path -LiteralPath $CMakePath) {
    $CMake = [pscustomobject]@{ Source = $CMakePath }
  }
}

if (-not $VcVars) {
  Write-Error "STOP: Visual Studio vcvars64.bat not found. Install Visual Studio Build Tools or provide the path."
}

if (-not $CMake) {
  Write-Error "STOP: CMake not found. Install CMake or provide the path."
}

$OpenCvDir = Join-Path $ProjectRoot "tools\opencv\opencv\build\x64\vc16\lib"
$OpenCvBin = Join-Path $ProjectRoot "tools\opencv\opencv\build\x64\vc16\bin"
if (-not (Test-Path -LiteralPath (Join-Path $OpenCvDir "OpenCVConfig.cmake"))) {
  Write-Error "STOP: OpenCVConfig.cmake not found at $OpenCvDir"
}

$CTest = Join-Path (Split-Path -Parent $CMake.Source) "ctest.exe"
if (-not (Test-Path -LiteralPath $CTest)) {
  Write-Error "STOP: ctest.exe not found next to CMake: $CTest"
}

$Command = "call `"$VcVars`" && set `"PATH=$OpenCvBin;%PATH%`" && `"$($CMake.Source)`" -S `"$ProjectRoot`" -B `"$ProjectRoot\build`" -DOpenCV_DIR=`"$OpenCvDir`" && `"$($CMake.Source)`" --build `"$ProjectRoot\build`" --config Release && `"$CTest`" --test-dir `"$ProjectRoot\build`" --output-on-failure -C Release"
cmd.exe /c $Command
