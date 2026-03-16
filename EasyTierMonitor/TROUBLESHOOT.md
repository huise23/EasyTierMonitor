# 🔧 EasyTierMonitor - 故障排除指南

## 问题：插件无法加载，日志显示立即析构

### 当前症状
```
✅ TMPluginGetInstance() called
✅ Constructor called  
❌ Destructor called (立即)
❌ GetInfo() 未被调用
❌ GetItem() 未被调用
❌ DataRequired() 未被调用
```

---

## 🎯 根本原因分析

TrafficMonitor加载插件的顺序：
```
1. LoadLibrary() → 调用 DllMain
2. GetProcAddress(TMPluginGetInstance) → 获取导出函数
3. 调用 TMPluginGetInstance() → ✅ 成功
4. 调用 GetInfo() 查询插件信息 → ❌ 这里被拒绝！
5. 如果GetInfo返回有效信息 → 继续加载
6. 调用 GetItem(0) → 获取显示项
7. 如果成功 → 插件正常工作
```

**当前问题**：插件在步骤4之前就被销毁，说明可能：

1. **DLL位数不匹配** (32位 vs 64位)
2. **缺少依赖DLL** (MFC/VC Runtime)
3. **GetInfo()返回了无效指针** 
4. **导出函数有问题**

---

## ✅ 诊断步骤

### 第1步：确认DLL位数

```powershell
# 在DLL所在目录执行
dumpbin /HEADERS EasyTierMonitor.dll | Select-String "machine"

# 应该显示：
# x64 → 64位 DLL (正确)
# x86 → 32位 DLL (错误，如果TrafficMonitor是64位)
```

**或者**使用以下命令检查导出函数：
```powershell
dumpbin /EXPORTS EasyTierMonitor.dll | Select-String "TMPluginGetInstance"
```

应该看到：
```
1 0x00001234 ? TMPluginGetInstance@@YAPEAVITMPlugin@@XZ
```

### 第2步：检查依赖项

```powershell
# 使用Dependencies工具（GUI）或
dumpbin /DEPENDENTS EasyTierMonitor.dll

# 应该看到：
# MFC140u.dll
# VCRUNTIME140.dll
# MSVCP140.dll
# 等VC++ Runtime DLL
```

### 第3步：系统日志检查

```powershell
# 查看Windows事件查看器
eventvwr.msc

# 路径：Windows日志 → 应用程序
# 查找TrafficMonitor或EasyTierMonitor相关的错误
```

---

## 🔧 解决方案

### 方案A：确保DLL位数匹配

如果TrafficMonitor是64位，必须使用x64配置编译的DLL。

### 方案B：安装VC++ Runtime

下载安装：Microsoft Visual C++ 2015-2022 Redistributable (x64)

### 方案C：检查MFC DLL

确认MFC140.dll或MFC140u.dll在系统中。

---

## 📋 测试最新版本（17:41编译）

1. **关闭TrafficMonitor**（完全退出）
2. **复制新DLL**
   ```powershell
   Copy-Item "C:\work\ping1\EasyTierMonitor\bin\x64\Release\EasyTierMonitor.dll" "C:\Program Files\TrafficMonitor\plugins\" -Force
   ```
3. **启动TrafficMonitor**
4. **查看新的调试日志**

### 期待的新日志内容

```
[timestamp] GetPluginInstance - [INFO] === TMPluginGetInstance() called ===
[timestamp] Instance - [INFO] Instance() called
[timestamp] Constructor - [INFO] Constructor called
[timestamp] GetPluginInstance - [INFO] Plugin instance address: 0x...
[timestamp] GetInfo(0) - [INFO] GetInfo(0) called  ← 应该出现！
[timestamp] GetInfo(0) - [INFO] GetInfo(0) returning: 'EasyTier Monitor' (len=15)
[timestamp] GetInfo(1) - [INFO] GetInfo(1) called  ← 应该出现！
...更多GetInfo调用...
```

---

## ❓ 如果仍然只看到4条日志

说明GetInfo()也没有被调用，问题更深层。请提供：

1. **dumpbin输出**：
   ```powershell
   dumpbin /EXPORTS EasyTierMonitor.dll > exports.txt
   cat exports.txt
   ```

2. **DLL依赖**：
   ```powershell
   dumpbin /DEPENDENTS EasyTierMonitor.dll
   ```

3. **系统架构**：
   ```powershell
   wmic os get osarchitecture
   ```

4. **TrafficMonitor版本和架构**

---

## 🎯 下一步

请用最新编译的DLL（17:41）测试，并提供：
- `easytier_monitor_debug.log` 的完整内容
- 如果仍然只有4条日志，提供上述诊断信息
