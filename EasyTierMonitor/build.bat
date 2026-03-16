@echo off
REM EasyTierMonitor 构建脚本

echo ========================================
echo EasyTierMonitor Plugin Build Script
echo ========================================
echo.

REM 检查 MSBuild 是否在 PATH 中
where msbuild >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo 错误: 未找到 MSBuild
    echo 请使用 Developer Command Prompt 或添加 MSBuild 到 PATH
    pause
    exit /b 1
)

REM 询问配置类型
echo 请选择配置类型:
echo 1. Debug x64
echo 2. Release x64
echo 3. Debug Win32
echo 4. Release Win32
echo.
set /p choice="请输入选择 (1-4): "

REM 设置配置参数
if "%choice%"=="1" (
    set CONFIG=Debug
    set PLATFORM=x64
) else if "%choice%"=="2" (
    set CONFIG=Release
    set PLATFORM=x64
) else if "%choice%"=="3" (
    set CONFIG=Debug
    set PLATFORM=Win32
) else if "%choice%"=="4" (
    set CONFIG=Release
    set PLATFORM=Win32
) else (
    echo 无效的选择！
    pause
    exit /b 1
)

echo.
echo 开始编译 %CONFIG% %PLATFORM%...
echo.

REM 执行编译
msbuild EasyTierMonitor.sln ^
    /p:Configuration=%CONFIG% ^
    /p:Platform=%PLATFORM% ^
    /t:Rebuild ^
    /v:minimal ^
    /nologo

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo 编译成功！
    echo ========================================
    echo.
    echo 输出位置: bin\%PLATFORM%\%CONFIG%\EasyTierMonitor.dll
    echo.
    echo 下一步:
    echo 1. 将 DLL 复制到 TrafficMonitor 的 plugins 目录
    echo 2. 重启 TrafficMonitor
    echo 3. 在显示设置中启用 "EasyTier 状态"
    echo.
) else (
    echo.
    echo ========================================
    echo 编译失败！
    echo ========================================
    echo.
    echo 请检查错误信息并重试
    echo.
)

pause
