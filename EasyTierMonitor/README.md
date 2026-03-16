# EasyTier Monitor Plugin

TrafficMonitor 插件，用于监控 EasyTier peer 的连接状态。

## 功能特性

- ✅ 实时监控 EasyTier peer 连接状态
- ✅ 显示直连/中转/断开状态
- ✅ 显示 Ping 延迟和 CLI 延迟
- ✅ 支持自定义显示（图标 + 文本）
- ✅ 详细信息悬浮提示
- ✅ 可配置更新间隔（1-60 秒）
- ✅ 智能状态判断（CLI + Ping 双重验证）
- ✅ 历史状态记录（内部支持）
- 🚧 配置对话框（待完善）
- 🚧 系统通知（待实现）

## 快速开始

### 安装

1. **下载最新版本**
   - 从 [Releases](../../releases) 下载预编译的 DLL
   - 或自行编译（见下文）

2. **安装插件**
   ```
   将 EasyTierMonitor.dll 复制到 TrafficMonitor 的 plugins 目录：

   - 安装版：C:\Program Files\TrafficMonitor\plugins\
   - Portable 版：TrafficMonitor\plugins\
   ```

3. **重启 TrafficMonitor**

4. **启用显示**
   - 右键主窗口 → "显示设置"
   - 勾选 "EasyTier 状态"

### 编译

详细编译说明请参考 [BUILD.md](docs/BUILD.md)

快速编译：
```cmd
# 使用构建脚本（推荐）
build.bat

# 或使用 MSBuild
msbuild EasyTierMonitor.sln /p:Configuration=Release /p:Platform=x64
```

## 使用说明

### 基本使用

1. 确保 EasyTier 正在运行
2. 插件会自动检测并显示 peer 状态
3. 鼠标悬停可查看详细信息

### 状态说明

- 🟢 **直连** (p2p)：与 peer 直接连接，延迟最低
- 🟡 **中转** (relay)：通过服务器中转，延迟较高
- 🔴 **断开**：无法连接到 peer
- ⚪ **未知/未找到**：EasyTier 未运行或选择的 peer 不在列表中

### 配置

配置文件位置：
```
TrafficMonitor 配置目录\easytier_monitor_config.ini
```

配置项：
```ini
peer_id=选择的 Peer ID
update_interval=5              # 更新间隔（秒）
enable_notification=1          # 启用通知
notification_cooldown=5        # 通知冷却时间（分钟）
ping_timeout=2000              # Ping 超时（毫秒）
consecutive_failures=3         # 连续失败阈值
cli_path=easytier-cli          # CLI 路径
max_history=100                # 最大历史记录数
```

## 测试

详细测试指南请参考 [TESTING.md](docs/TESTING.md)

基本测试步骤：
1. 确认插件加载成功
2. 查看 TrafficMonitor 中是否显示状态
3. 检查鼠标悬停提示
4. 测试 EasyTier 断开/重连场景

## 开发

### 系统要求

- Windows 10/11
- Visual Studio 2019 或更高版本
- Windows SDK 10.0 或更高版本
- MFC 支持

### 项目结构

```
EasyTierMonitor/
├── include/              # 公共头文件
├── src/                  # 源代码
├── res/                  # 资源文件
├── docs/                 # 文档
├── EasyTierMonitor.sln   # Visual Studio 解决方案
└── README.md             # 本文件
```

### 技术栈

- **语言**: C++17
- **框架**: MFC (Microsoft Foundation Classes)
- **API**:
  - Windows ICMP API (Ping)
  - Windows Process API (CLI 调用)
  - Windows Sockets (WinSock)

## 进度

当前版本：v1.0.0-alpha

详细进度请参考 [PROGRESS.md](docs/PROGRESS.md)

已完成：
- ✅ 核心功能实现（CLI 调用、Ping 测试、状态判断）
- ✅ 线程安全和数据管理
- ✅ 基础 UI 显示
- 🚧 配置对话框（部分实现）

待实现：
- 🚧 完整配置 UI
- 🚧 Toast 通知
- 🚧 历史记录可视化

## 故障排除

### 插件无法加载

1. 确认使用正确架构（x64 或 x86）
2. 检查是否安装了 VC++ 运行时
3. 查看 TrafficMonitor 日志

### 显示 "未知" 状态

1. 确认 EasyTier 正在运行
2. 检查 easytier-cli 是否在 PATH 中
3. 尝试在命令行运行 `easytier-cli peer` 验证

### 高 CPU 占用

1. 增加更新间隔（配置文件中 `update_interval`）
2. 检查是否有多个 peer 在监控

## 贡献

欢迎贡献代码、报告问题或提出建议！

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

## 许可证

MIT License - 详见 [LICENSE](LICENSE) 文件

## 致谢

- [TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor) - 优秀的网速监控软件
- [EasyTier](https://github.com/EasyTier/EasyTier) - 简单易用的 VPN 解决方案

## 联系方式

- 问题反馈：[GitHub Issues](../../issues)
- 讨论：[GitHub Discussions](../../discussions)

---

**注意**: 本插件目前处于 Alpha 阶段，可能存在 bug。使用时请谨慎，欢迎反馈问题！
