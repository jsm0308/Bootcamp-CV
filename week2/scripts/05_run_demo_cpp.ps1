$ErrorActionPreference = "Stop"

$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $ProjectRoot

$OpenCvBin = Join-Path $ProjectRoot "tools\opencv\opencv\build\x64\vc16\bin"
$Exe = Join-Path $ProjectRoot "build\Release\rgbd_pointcloud_cli.exe"
$Dataset = Join-Path $ProjectRoot "data\external\tum\rgbd_dataset_freiburg1_xyz"
$Output = Join-Path $ProjectRoot "outputs\cpp"

if (-not (Test-Path -LiteralPath $OpenCvBin)) {
  Write-Error "STOP: OpenCV bin directory not found: $OpenCvBin"
}
if (-not (Test-Path -LiteralPath $Exe)) {
  Write-Error "STOP: C++ executable not found. Run 04_build_cpp.ps1 first."
}
if (-not (Test-Path -LiteralPath $Dataset)) {
  Write-Error "STOP: Dataset not found: $Dataset"
}

New-Item -ItemType Directory -Force -Path $Output | Out-Null
$env:PATH = "$OpenCvBin;$env:PATH"

& $Exe `
  --dataset $Dataset `
  --output $Output `
  --frame-index 0 `
  --max-points 60000
