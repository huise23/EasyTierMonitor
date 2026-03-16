$msbuildPath = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
$solutionPath = "C:\work\ping1\EasyTierMonitor\EasyTierMonitor.sln"

Write-Host "========================================"  -ForegroundColor Cyan
Write-Host "Building EasyTierMonitor Plugin" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path $msbuildPath)) {
    Write-Host "Error: MSBuild not found at $msbuildPath" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $solutionPath)) {
    Write-Host "Error: Solution not found at $solutionPath" -ForegroundColor Red
    exit 1
}

Write-Host "Running MSBuild..." -ForegroundColor Green
Write-Host ""

& $msbuildPath $solutionPath /p:Configuration=Release /p:Platform=x64 /t:Clean,Build /m /v:minimal /nologo

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "Build Successful!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Output: bin\x64\Release\EasyTierMonitor.dll" -ForegroundColor Yellow
    Write-Host ""
} else {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "Build Failed!" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    Write-Host ""
    exit 1
}
