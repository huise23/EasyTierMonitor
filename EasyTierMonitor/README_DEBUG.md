# EasyTierMonitor - Debug Version

## What's New

This version includes **comprehensive debug logging** to help diagnose startup issues.

## Debug Log Location

When TrafficMonitor loads this plugin, a debug log will be created:
- **File**: `easytier_monitor_debug.log`
- **Location**: Same directory as TrafficMonitor.exe or plugin directory
- **Content**: Detailed logs of all plugin operations

## What Gets Logged

### Plugin Initialization
- DLL export function calls
- Constructor/destructor calls
- DataManager creation and config loading
- WorkerThread creation and startup

### Runtime Operations
- Peer list fetching
- Ping testing
- Configuration updates
- Any exceptions or errors

## How to Use

1. **Replace the DLL**
   - Copy `bin\x64\Release\EasyTierMonitor.dll` to TrafficMonitor plugins folder
   - Overwrite existing version

2. **Restart TrafficMonitor**
   - Close TrafficMonitor completely
   - Reopen TrafficMonitor

3. **Check the Debug Log**
   - Look for `easytier_monitor_debug.log` in:
     - TrafficMonitor.exe directory
     - Or the plugin configuration directory

4. **Report Issues**
   - Include the debug log when reporting problems
   - The log shows exact execution flow and any errors

## Debug Log Format

```
[2026-03-13 17:00:00] TMPluginGetInstance:236 - [INFO] === TMPluginGetInstance() called ===
[2026-03-13 17:00:00] TMPluginGetInstance:238 - [INFO] Plugin instance address: 000001234567890
[2026-03-13 17:00:00] Constructor:17 - [INFO] Constructor: main_hwnd_=0000000000000000, initialized_=0
...
```

## Troubleshooting

### Problem: Plugin doesn't load
**Check**: Does `easytier_monitor_debug.log` exist?
- **No**: DLL not loaded at all - check TrafficMonitor compatibility
- **Yes**: Review log for errors

### Problem: Continuous error messages
**Check**: Look for `[ERROR]` tags in debug log
- Common issues:
  - `easytier-cli` path incorrect
  - Configuration file corrupted
  - Permission issues

### Problem: Wrong peer selected
**Check**: Look for peer_id and ipv4 in log
```
[INFO] Config loaded: cli_path=easytier-cli, selected_peer_id=peer123, update_interval=5
```

## Build Information

- **Version**: Debug-enabled Release build
- **Date**: 2026-03-13
- **Platform**: x64
- **Compiler**: MSVC 14.50 (VS 2025)
- **Configuration**: Release with debug logging enabled
