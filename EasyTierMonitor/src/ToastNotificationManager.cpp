#include "stdafx.h"
#include "ToastNotificationManager.h"
#include <fstream>
#include <shlobj.h>
#include <shellapi.h>
#include <ctime>

namespace EasyTierMonitor {

CToastNotificationManager::CToastNotificationManager()
    : last_notification_time_(0)
    , cooldown_minutes_(5)
    , enabled_(true)
{
}

CToastNotificationManager::~CToastNotificationManager()
{
}

bool CToastNotificationManager::IsWindows10OrLater()
{
    OSVERSIONINFOEX osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    // Use VersionHelpers API
    typedef BOOL(WINAPI* LPISWINDOWS10ORGREATER)();
    HMODULE hModule = LoadLibraryW(L"kernel32.dll");
    if (hModule)
    {
        LPISWINDOWS10ORGREATER pIsWindows10OrGreater =
            (LPISWINDOWS10ORGREATER)GetProcAddress(hModule, "IsWindows10OrGreater");
        if (pIsWindows10OrGreater)
        {
            BOOL result = pIsWindows10OrGreater();
            FreeLibrary(hModule);
            return result != FALSE;
        }
        FreeLibrary(hModule);
    }

    return false;
}

std::wstring CToastNotificationManager::GetShortcutPath()
{
    wchar_t appdata_path[MAX_PATH];
    if (SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appdata_path) == S_OK)
    {
        std::wstring path = appdata_path;
        path += L"\\Microsoft\\Windows\\Start Menu\\Programs\\";
        path += L"EasyTierMonitor.lnk";
        return path;
    }
    return L"";
}

bool CToastNotificationManager::CreateShortcut()
{
    // Simplified implementation: only attempt to create shortcut on first call
    // Actual application should use COM interface to create complete shortcut
    static bool shortcut_created = false;
    if (shortcut_created)
        return true;

    // TODO: Implement complete shortcut creation logic
    // This requires using IShellLink interface

    shortcut_created = true;
    return true;
}

void CToastNotificationManager::SetCooldownMinutes(int minutes)
{
    cooldown_minutes_ = minutes;
}

bool CToastNotificationManager::CanSendNotification()
{
    if (!enabled_)
        return false;

    if (cooldown_minutes_ <= 0)
        return true;

    time_t current_time;
    time(&current_time);

    double diff = difftime(current_time, last_notification_time_);
    if (diff >= cooldown_minutes_ * 60)
        return true;

    return false;
}

bool CToastNotificationManager::ShowNotification(const std::wstring& title, const std::wstring& message, const std::wstring& peer_name)
{
    if (!enabled_)
        return false;

    if (!CanSendNotification())
        return false;

    // Create shortcut (if not yet created)
    if (!CreateShortcut())
        return false;

    // Use system tray balloon notification (works on all Windows versions)
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = GetDesktopWindow(); // Use desktop window as fallback
    nid.uID = 1001; // Unique ID for this notification
    nid.uFlags = NIF_INFO | NIF_MESSAGE;
    nid.dwInfoFlags = NIIF_INFO;
    nid.uTimeout = 5000;

    wcscpy_s(nid.szInfo, message.c_str());
    wcscpy_s(nid.szInfoTitle, title.c_str());

    // Try to add the notification icon
    Shell_NotifyIconW(NIM_ADD, &nid);

    // Show the balloon notification
    Shell_NotifyIconW(NIM_MODIFY, &nid);

    // Record notification time
    time(&last_notification_time_);

    // Clean up after a delay (the notification will remain visible)
    // We don't delete immediately to allow the notification to display

    return true;
}

} // namespace EasyTierMonitor
