$ErrorActionPreference = "Stop"

$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $ProjectRoot

$Output = Join-Path $ProjectRoot "outputs\cpp"
$RequiredFiles = @(
  "point_cloud_sample_cpp.ply",
  "point_cloud_sample_cpp.csv",
  "depth_preview_cpp.png",
  "projection_preview_cpp.png",
  "point_cloud_preview_cpp.png",
  "metrics_cpp.json"
)

foreach ($File in $RequiredFiles) {
  $Path = Join-Path $Output $File
  if (-not (Test-Path -LiteralPath $Path)) {
    Write-Error "STOP: Required C++ output missing: $Path"
  }
  $Item = Get-Item -LiteralPath $Path
  if ($Item.Length -le 0) {
    Write-Error "STOP: C++ output is empty: $Path"
  }
}

$PlyPath = Join-Path $Output "point_cloud_sample_cpp.ply"
$Header = Get-Content -LiteralPath $PlyPath -TotalCount 12
$VertexLine = $Header | Where-Object { $_ -like "element vertex *" } | Select-Object -First 1
if (-not $VertexLine) {
  Write-Error "STOP: PLY vertex count header not found."
}

$MetricsPath = Join-Path $Output "metrics_cpp.json"
$Metrics = Get-Content -Raw -LiteralPath $MetricsPath | ConvertFrom-Json
if ($Metrics.valid_depth_ratio -le 0 -or $Metrics.valid_depth_ratio -gt 1) {
  Write-Error "STOP: valid_depth_ratio is outside (0, 1]."
}
if ($Metrics.sampled_points -le 0) {
  Write-Error "STOP: sampled_points must be positive."
}
if ($Metrics.max_reprojection_error_px -gt 0.001) {
  Write-Error "STOP: reprojection error is too large: $($Metrics.max_reprojection_error_px) px"
}

Write-Host "CPP_OUTPUT_VALIDATION=PASS"
Write-Host $VertexLine
Write-Host "valid_depth_ratio=$($Metrics.valid_depth_ratio)"
Write-Host "max_reprojection_error_px=$($Metrics.max_reprojection_error_px)"
