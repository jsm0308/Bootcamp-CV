$ErrorActionPreference = "Stop"

$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $ProjectRoot

$MetricsPath = Join-Path $ProjectRoot "outputs\metrics.json"
if (-not (Test-Path -LiteralPath $MetricsPath)) {
  Write-Error "STOP: metrics.json not found. Run 02_run_demo_js.ps1 first."
}

$Metrics = Get-Content -LiteralPath $MetricsPath -Raw | ConvertFrom-Json

$RequiredFiles = @(
  "outputs\point_cloud_sample.ply",
  "outputs\point_cloud_sample.csv",
  "outputs\depth_preview.bmp",
  "outputs\projection_preview.bmp",
  "outputs\metrics.json"
)

foreach ($File in $RequiredFiles) {
  if (-not (Test-Path -LiteralPath (Join-Path $ProjectRoot $File))) {
    Write-Error "STOP: Required output missing: $File"
  }
}

if ($Metrics.rgbFrames -le 0) {
  Write-Error "STOP: rgbFrames must be positive."
}
if ($Metrics.depthFrames -le 0) {
  Write-Error "STOP: depthFrames must be positive."
}
if ($Metrics.associatedPairs -le 0) {
  Write-Error "STOP: associatedPairs must be positive."
}
if ($Metrics.validDepthRatio -le 0) {
  Write-Error "STOP: validDepthRatio must be positive."
}
if ($Metrics.sampledPoints -le 0) {
  Write-Error "STOP: sampledPoints must be positive."
}

Write-Host "VALIDATION=PASS"
Write-Host "RGB_FRAMES=$($Metrics.rgbFrames)"
Write-Host "DEPTH_FRAMES=$($Metrics.depthFrames)"
Write-Host "ASSOCIATED_PAIRS=$($Metrics.associatedPairs)"
Write-Host "VALID_DEPTH_RATIO=$($Metrics.validDepthRatio)"
Write-Host "SAMPLED_POINTS=$($Metrics.sampledPoints)"
