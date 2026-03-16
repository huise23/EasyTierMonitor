#pragma once

#include <Winsock2.h>
#include <Windows.h>
#include "Common.h"
#include "CliExecutor.h"
#include "CliParser.h"
#include "PingTester.h"
#include "DataManager.h"
#include "ToastNotificationManager.h"
#include <thread>
#include <atomic>
#include <memory>

namespace EasyTierMonitor {

// Worker thread
class CWorkerThread
{
public:
    CWorkerThread();
    ~CWorkerThread();

    // Start worker thread
    void Start(CDataManager* data_manager, HWND notify_hwnd);

    // Stop worker thread
    void Stop();

    // Check if running
    bool IsRunning() const { return thread_.joinable(); }

    // Force immediate update
    void ForceUpdate();

private:
    // Thread main function
    void ThreadProc();

    // Get peer list
    std::vector<PeerInfo> FetchPeerList();

    // Ping test
    int PingPeer(const std::wstring& ipv4);

    // Status determination logic
    ConnStatus DetermineStatus(const PeerInfo* peer, int ping_ms);

    // Send system notification
    void SendNotification(const StatusSnapshot& snapshot);

private:
    std::thread thread_;
    std::atomic<bool> stop_requested_;
    std::atomic<bool> update_requested_;

    CDataManager* data_manager_;
    HWND notify_hwnd_;

    // Runtime status
    int consecutive_failures_;
    time_t last_notification_time_;

    // CLI executor and Ping tester
    std::unique_ptr<CCliExecutor> cli_executor_;
    std::unique_ptr<CPingTester> ping_tester_;
    std::unique_ptr<CToastNotificationManager> toast_manager_;

    // Custom message (for notifying main thread)
    static const UINT WM_UPDATE_STATUS = WM_USER + 100;
};

} // namespace EasyTierMonitor
