# 🔍 Debug Version - 使用指南

## ✅ 已完成

带调试日志版本的DLL已成功编译！

**文件位置**: `bin\x64\Release\EasyTierMonitor.dll`  
**大小**: 约 122 KB  
**状态**: ✅ 编译成功

---

## 📝 问题诊断步骤

### 第一步：使用新DLL

1. **关闭TrafficMonitor** (完全退出)
2. **复制新DLL到插件目录**
   ```
   源文件: C:\work\ping1\EasyTierMonitor\bin\x64\Release\EasyTierMonitor.dll
   目标目录: TrafficMonitor\plugins\
   ```
3. **启动TrafficMonitor**

### 第二步：查找调试日志

调试日志会自动生成在以下位置之一：

**优先顺序：**
1. `C:\Users\<用户名>\AppData\Roaming\TrafficMonitor\easytier_monitor_debug.log`
2. TrafficMonitor.exe所在目录
3. 插件DLL所在目录

### 第三步：分析日志内容

日志格式示例：
```
[2026-03-13 17:30:45] TMPluginGetInstance:237 - [INFO] === TMPluginGetInstance() called ===
[2026-03-13 17:30:45] TMPluginGetInstance:239 - [INFO] Plugin instance address: 000001234567890
[2026-03-13 17:30:45] Constructor:17 - [INFO] Constructor: main_hwnd_=0000000000000000, initialized_=0
[2026-03-13 17:30:45] Initialize:29 - [INFO] Initialize() - initialized_=0
...
```

---

## 🎯 关键诊断点

### ✅ 如果看到这些日志 = 正常加载

- `[INFO] === TMPluginGetInstance() called ===`
- `[INFO] Plugin instance address: 0x...`
- `[INFO] Constructor: main_hwnd_=...`
- `[INFO] Initialize() - initialized_=0`
- `[INFO] DataManager created`
- `[INFO] WorkerThread created`

### ❌ 如果看到这些日志 = 有问题

1. **DLL未加载**
   - 没有生成 `easytier_monitor_debug.log`
   - 原因：DLL路径错误或位数不匹配

2. **配置加载失败**
   - `[ERROR] Exception during initialization: ...`
   - 检查配置文件权限

3. **Worker线程启动失败**
   - `[ERROR] in StartWorker: ...`
   - 检查easytier-cli路径

---

## 🔧 常见问题解决

### 问题：96条重复错误 "可能出现了异常，当前选择的连接和期望的连接不一致"

**原因分析**：
1. **旧版本DLL仍在运行** - 这个错误消息是中文，但新版本已全部改英文
2. **配置文件中的peer_id在peer列表中找不到**

**解决方法**：
1. 确认使用的是新编译的DLL（检查文件修改时间）
2. 删除或重命名配置文件：`config.ini`
3. 重新启动TrafficMonitor，会创建新配置

### 问题：找不到调试日志

**检查**：
```powershell
# 搜索所有可能的日志位置
Get-ChildItem -Path C:\ -Filter "easytier_monitor_debug.log" -Recurse -ErrorAction SilentlyContinue
```

### 问题：日志显示"找不到easytier-cli"

**解决**：
1. 打开配置对话框
2. 设置正确的CLI路径
3. 或确保easytier-cli在系统PATH中

---

## 📊 报告问题时请提供

1. **完整的调试日志内容** (`easytier_monitor_debug.log`)
2. **错误日志** (`error.log`)
3. **DLL文件信息**（大小、修改时间）
4. **TrafficMonitor版本**
5. **Windows版本**

---

## 💡 提示

- 调试日志包含时间戳，可以看出初始化顺序
- `[ERROR]`标签表示错误
- `[WARNING]`标签表示警告
- `[INFO]`标签表示正常信息流
- 日志也会输出到Windows调试器，可用DebugView查看

需要帮助？请提供调试日志内容！
