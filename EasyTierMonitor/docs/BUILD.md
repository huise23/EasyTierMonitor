# 编译指南

本文档介绍如何编译 EasyTierMonitor 插件。

## 系统要求

- Windows 10/11
- Visual Studio 2019 或更高版本
- Windows SDK 10.0 或更高版本

## 依赖项

### 必需依赖
- **MFC (Microsoft Foundation Classes)**: 已包含在 Visual Studio 中
- **Windows SDK**: 提供系统 API 支持

### 链接库
项目会自动链接以下库：
- `iphlpapi.lib`: ICMP (Ping) API
- `ws2_32.lib`: Windows Sockets API

## 编译步骤

### 方法 1：使用 Visual Studio GUI

1. **打开解决方案**
   ```
   双击 EasyTierMonitor.sln 打开项目
   ```

2. **选择配置**
   - 顶部工具栏选择配置：Debug 或 Release
   - 选择平台：x64（推荐）或 Win32

3. **生成解决方案**
   - 菜单：生成 → 生成解决方案 (Ctrl+Shift+B)
   - 或右键点击解决方案 → 生成

4. **查找输出**
   - Debug x64: `bin\x64\Debug\EasyTierMonitor.dll`
   - Release x64: `bin\x64\Release\EasyTierMonitor.dll`

### 方法 2：使用命令行 (MSBuild)

1. **打开 Developer Command Prompt**
   - 开始菜单 → Visual Studio 文件夹 → Developer Command Prompt

2. **导航到项目目录**
   ```cmd
   cd C:\work\ping1\EasyTierMonitor
   ```

3. **编译项目**
   ```cmd
   # Debug x64
   msbuild EasyTierMonitor.sln /p:Configuration=Debug /p:Platform=x64

   # Release x64
   msbuild EasyTierMonitor.sln /p:Configuration=Release /p:Platform=x64
   ```

### 方法 3：使用 PowerShell 脚本

创建 `build.ps1`:
```powershell
# 编译 Release x64
msbuild EasyTierMonitor.sln `
    /p:Configuration=Release `
    /p:Platform=x64 `
    /t:Rebuild `
    /v:minimal

if ($LASTEXITCODE -eq 0) {
    Write-Host "编译成功！" -ForegroundColor Green
    Write-Host "输出位置: bin\x64\Release\EasyTierMonitor.dll"
} else {
    Write-Host "编译失败！" -ForegroundColor Red
    exit 1
}
```

运行：
```powershell
.\build.ps1
```

## 常见编译问题

### 问题 1：找不到 MFC 头文件
**错误**: `fatal error C1083: 无法打开包括文件: "afxwin.h"`

**解决方案**:
1. 确保安装了 "使用 C++ 的桌面开发" 工作负载
2. 在项目属性中确认 "使用 MFC" 设置为 "在共享 DLL 中使用 MFC"

### 问题 2：链接错误
**错误**: `unresolved external symbol IcmpCreateFile`

**解决方案**:
1. 确认项目设置中包含 `iphlpapi.lib`
2. 项目属性 → 链接器 → 输入 → 附加依赖项

### 问题 3：字符集错误
**错误**: `cannot convert from 'const char *' to 'const wchar_t *'`

**解决方案**:
1. 项目属性 → 配置属性 → 高级 → 字符集
2. 设置为 "使用 Unicode 字符集"

## 编译选项

### Debug 配置
- 包含调试信息
- 未优化代码
- 更大的二进制文件
- 用于开发和调试

### Release 配置
- 无调试信息
- 启用优化 (/O2)
- 更小的二进制文件
- 用于生产环境

## 发布版本

发布时应该使用 **Release x64** 配置：

1. 生成更小的 DLL
2. 性能更好
3. 不包含调试符号

```cmd
msbuild EasyTierMonitor.sln /p:Configuration=Release /p:Platform=x64 /t:Rebuild
```

## 验证编译结果

编译完成后，检查以下内容：

1. **DLL 文件存在**
   ```cmd
   dir bin\x64\Release\EasyTierMonitor.dll
   ```

2. **DLL 依赖项检查**
   使用 Dependency Walker 或类似工具检查依赖项是否正常

3. **基本加载测试**
   将 DLL 复制到 TrafficMonitor 的 plugins 目录，查看是否能正常加载

## 下一步

编译成功后，参考 [测试指南](TESTING.md) 进行功能测试。
