#pragma once

#include <string>
#include <Windows.h>

namespace EasyTierMonitor {

// Toast notification manager class
class CToastNotificationManager
{
public:
    CToastNotificationManager();
    ~CToastNotificationManager();

    // Show notification
    bool ShowNotification(const std::wstring& title, const std::wstring& message, const std::wstring& peer_name = L"");

    // Set cooldown time (minutes)
    void SetCooldownMinutes(int minutes);

    // Check if notification can be sent (considering cooldown time)
    bool CanSendNotification();

    // Enable/disable notifications
    void Enable(bool enable) { enabled_ = enable; }
    bool IsEnabled() const { return enabled_; }

private:
    bool CreateShortcut();
    std::wstring GetShortcutPath();
    bool IsWindows10OrLater();
    time_t last_notification_time_;
    int cooldown_minutes_;
    bool enabled_;
};

} // namespace EasyTierMonitor
