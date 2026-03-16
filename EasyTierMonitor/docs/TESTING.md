# EasyTierMonitor Plugin Testing Guide

## Build Information
- **Build Date**: 2026-03-14 16:32
- **DLL Size**: 134KB
- **Configuration**: Release x64
- **Compiler**: MSVC 14.50.35717 (Visual Studio 2025)

## Fixed Issues
1. ✅ Corrected PluginInfoIndex enum values from `PI_*` to `TMI_*`
2. ✅ Fixed type cast in OnExtenedInfo from `static_cast` to `reinterpret_cast` for HWND
3. ✅ Added TMI_URL support

## Installation Steps

1. Copy the DLL to TrafficMonitor plugins directory:
   ```
   copy "C:\work\ping1\EasyTierMonitor\bin\x64\Release\EasyTierMonitor.dll" "C:\Program Files\TrafficMonitor\plugins\"
   ```

2. Restart TrafficMonitor

## Expected Log Output

When plugin loads successfully, you should see these log entries:

```
=== TMPluginGetInstance() called ===
Plugin instance address: [address]
Instance() called
Constructor: main_hwnd_=0, initialized_=0
GetInfo(0) called  // TMI_NAME
GetInfo(1) called  // TMI_DESCRIPTION
GetInfo(2) called  // TMI_AUTHOR
GetInfo(3) called  // TMI_COPYRIGHT
GetInfo(4) called  // TMI_VERSION
GetInfo(5) called  // TMI_URL
OnExtenedInfo(0) called, data=[config_dir]  // EI_CONFIG_DIR
OnExtenedInfo(1) called, data=[hwnd]        // EI_MAIN_HWND
GetItem(0) called - initialized_=0
Not initialized, calling Initialize()...
Initialize() - initialized_=0
Creating DataManager...
Loading config from: [config_dir]
Creating display item...
Creating worker thread...
Initialization completed successfully
Returning status_item_: [address]
DataRequired() called - initialized_=1
Calling StartWorker()...
```

## Verification Checklist

### 1. Plugin Loading
- [ ] TMPluginGetInstance() is called
- [ ] Plugin instance is created successfully
- [ ] GetInfo() returns correct values for all TMI_* enums

### 2. Initialization
- [ ] OnExtenedInfo(EI_CONFIG_DIR) receives config directory
- [ ] OnExtenedInfo(EI_MAIN_HWND) receives main window handle
- [ ] Initialize() completes without errors
- [ ] DataManager, PluginItem, WorkerThread are created

### 3. Runtime
- [ ] GetItem(0) returns valid IPluginItem
- [ ] DataRequired() triggers worker thread start
- [ ] No crashes or exceptions

## Debug Log Location

The debug log file should be created at:
```
C:\Users\[username]\AppData\Roaming\TrafficMonitor\EasyTierMonitor_debug.log
```

## Troubleshooting

### Plugin Not Loading
1. Check if DLL is 64-bit (should be x64)
2. Verify MFC runtime is installed (mfc140u.dll)
3. Check TrafficMonitor version supports API v6

### No Log Output
1. Verify DEBUG_LOG is enabled in build
2. Check file permissions for log directory
3. Ensure log file path is writable

### Crashes on Load
1. Check GetInfo() enum values match TrafficMonitor expectations
2. Verify OnExtenedInfo() type casts are correct
3. Review initialization sequence in debug log
