# DLL诊断脚本
Write-Host "=== EasyTierMonitor DLL 诊断工具 ===" -ForegroundColor Green

$dllPath = "C:\work\ping1\EasyTierMonitor\bin\x64\Release\EasyTierMonitor.dll"

if (!(Test-Path $dllPath)) {
    Write-Host "❌ DLL文件不存在: $dllPath" -ForegroundColor Red
    exit 1
}

Write-Host "`n✅ DLL文件: $dllPath" -ForegroundColor Green
$file = Get-Item $dllPath
Write-Host "   大小: $($file.Length) 字节"
Write-Host "   修改时间: $($file.LastWriteTime)"

# 检查DLL架构
Write-Host "`n=== DLL架构检查 ===" -ForegroundColor Yellow
try {
    $output = & dumpbin /HEADERS $dllPath 2>&1 | Out-String
    if ($output -match "machine \((x64|x86)\)") {
        $arch = $Matches[1]
        Write-Host "架构: $arch" -ForegroundColor Green
        if ($arch -eq "x86") {
            Write-Host "   ⚠️  这是32位DLL，需要64位" -ForegroundColor Yellow
        }
    } else {
        Write-Host "架构: 无法确定" -ForegroundColor Red
    }
} catch {
    Write-Host "❌ dumpbin工具不可用" -ForegroundColor Red
}

# 检查导出函数
Write-Host "`n=== 导出函数检查 ===" -ForegroundColor Yellow
try {
    $exports = & dumpbin /EXPORTS $dllPath 2>&1 | Out-String
    if ($exports -match "TMPluginGetInstance") {
        Write-Host "✅ TMPluginGetInstance 导出正常" -ForegroundColor Green
    } else {
        Write-Host "❌ 找不到 TMPluginGetInstance 导出" -ForegroundColor Red
    }
} catch {
    Write-Host "❌ 无法检查导出函数" -ForegroundColor Red
}

# 检查依赖项
Write-Host "`n=== DLL依赖项 ===" -ForegroundColor Yellow
try {
    $deps = & dumpbin /DEPENDENTS $dllPath 2>&1 | Out-String
    $deps.Split("`n") | Where-Object { $_ -match "dll" } | ForEach-Object {
        Write-Host "  $_" -ForegroundColor Cyan
    }
} catch {
    Write-Host "❌ 无法检查依赖项" -ForegroundColor Red
}

Write-Host "`n=== 完成 ===" -ForegroundColor Green
