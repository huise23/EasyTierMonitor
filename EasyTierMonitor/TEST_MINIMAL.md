# 最小插件测试说明

## 当前版本
这是一个最小化的测试插件，用于验证 TrafficMonitor 能否成功加载我们的插件。

## 功能
- 显示固定文本 "ET: OK"
- 无任何实际监控功能
- 仅用于测试插件加载机制

## 测试步骤

1. **复制 DLL 到 TrafficMonitor 插件目录**
   ```
   源文件: C:\work\ping1\EasyTierMonitor\bin\x64\Release\EasyTierMonitor.dll
   目标: [TrafficMonitor安装目录]\plugins\
   ```

2. **启动 TrafficMonitor**
   - 如果 TrafficMonitor 能正常启动，说明插件加载成功
   - 如果启动失败或崩溃，说明还有接口问题

3. **检查插件是否显示**
   - 右键点击 TrafficMonitor 窗口
   - 选择 "显示项目" 或类似菜单
   - 查看是否有 "EasyTier" 或 "ET: OK" 选项

4. **查看日志**
   ```
   日志位置: C:\work\ping1\EasyTierMonitor\bin\x64\Release\debug.log
   ```
   应该看到类似的日志：
   ```
   [INFO] Instance() called
   [INFO] GetAPIVersion() called, returning 6
   [INFO] GetItem(0) called
   [INFO] GetInfo(0) called
   ```

## 预期结果
- ✅ TrafficMonitor 正常启动
- ✅ 插件出现在显示项目列表中
- ✅ 显示 "ET: OK" 文本

## 如果测试失败
请提供以下信息：
1. TrafficMonitor 是否能启动？
2. 有什么错误提示？
3. debug.log 的内容（如果有）
4. TrafficMonitor 的版本号

## 下一步
测试成功后，我们将逐步添加功能：
1. ✅ 基础插件加载（当前阶段）
2. 添加配置对话框
3. 添加 EasyTier CLI 调用
4. 添加 Ping 测试
5. 添加状态显示
6. 添加历史记录
