$ErrorActionPreference = "Stop"

$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $ProjectRoot

$Dataset = Join-Path $ProjectRoot "data\external\tum\rgbd_dataset_freiburg1_xyz"
if (-not (Test-Path -LiteralPath $Dataset)) {
  Write-Error "STOP: Dataset not found: $Dataset"
}

node .\src\rgbd_pointcloud.js `
  --dataset ".\data\external\tum\rgbd_dataset_freiburg1_xyz" `
  --output ".\outputs" `
  --frame-index 0 `
  --max-points 60000
