$ErrorActionPreference = "Stop"

$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $ProjectRoot

function Find-ExistingPath($Candidates) {
  foreach ($Candidate in $Candidates) {
    if (Test-Path -LiteralPath $Candidate) {
      return $Candidate
    }
  }
  return $null
}

function Find-CommandPath($Name) {
  $Command = Get-Command $Name -ErrorAction SilentlyContinue
  if ($Command) {
    return $Command.Source
  }
  return $null
}

function Show-Value($Value) {
  if ($Value) {
    return $Value
  }
  return "MISSING"
}

$VcVars = Find-ExistingPath @(
  "C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
  "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
  "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
  "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
  "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
)

$CMake = Find-ExistingPath @(
  "C:\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
  "C:\Program Files\CMake\bin\cmake.exe",
  "C:\Program Files (x86)\CMake\bin\cmake.exe",
  "C:\Users\Gram\scoop\apps\cmake\current\bin\cmake.exe",
  "C:\ProgramData\chocolatey\bin\cmake.exe"
)
if (-not $CMake) {
  $CMake = Find-CommandPath "cmake"
}

$Vcpkg = Find-ExistingPath @(
  "C:\vcpkg\vcpkg.exe",
  "C:\Users\Gram\vcpkg\vcpkg.exe",
  "C:\src\vcpkg\vcpkg.exe",
  "C:\tools\vcpkg\vcpkg.exe"
)
if (-not $Vcpkg) {
  $Vcpkg = Find-CommandPath "vcpkg"
}

$Node = Find-CommandPath "node"
$Cl = Find-CommandPath "cl"
$Gpp = Find-CommandPath "g++"

Write-Host "PROJECT_ROOT=$ProjectRoot"
Write-Host "NODE=$(Show-Value $Node)"
Write-Host "VCVARS64=$(Show-Value $VcVars)"
Write-Host "CMAKE=$(Show-Value $CMake)"
Write-Host "VCPKG=$(Show-Value $Vcpkg)"
Write-Host "CL=$(Show-Value $Cl)"
Write-Host "GPP=$(Show-Value $Gpp)"

if (-not $Node) {
  Write-Error "STOP: Node is missing, so local JS verification cannot run."
}

if (-not $VcVars -or -not $CMake) {
  Write-Warning "C++ checkpoint is blocked: Visual Studio Build Tools and/or CMake not found."
  exit 2
}

Write-Host "ENV_CHECK=PASS"
