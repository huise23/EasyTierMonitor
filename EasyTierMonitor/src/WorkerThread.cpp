#include "stdafx.h"
#include "WorkerThread.h"
#include "DataManager.h"
#include "CliExecutor.h"
#include "CliParser.h"
#include "PingTester.h"
#include "DebugLog.h"
#include <Windows.h>
#include <chrono>

namespace EasyTierMonitor {

const UINT CWorkerThread::WM_UPDATE_STATUS;

CWorkerThread::CWorkerThread()
    : stop_requested_(false)
    , update_requested_(false)
    , data_manager_(nullptr)
    , notify_hwnd_(nullptr)
    , consecutive_failures_(0)
    , last_notification_time_(0)
{
    cli_executor_ = std::make_unique<CCliExecutor>();
    ping_tester_ = std::make_unique<CPingTester>();
}

CWorkerThread::~CWorkerThread()
{
    Stop();
}

void CWorkerThread::Start(CDataManager* data_manager, HWND notify_hwnd)
{
    if (IsRunning())
        return;

    data_manager_ = data_manager;
    notify_hwnd_ = notify_hwnd;
    stop_requested_ = false;
    update_requested_ = false;
    consecutive_failures_ = 0;

    // Initialize Ping tester
    ping_tester_->Initialize();

    // Start thread
    thread_ = std::thread(&CWorkerThread::ThreadProc, this);
}

void CWorkerThread::Stop()
{
    if (!IsRunning())
        return;

    stop_requested_ = true;
    update_requested_ = true; // Wake up thread

    if (thread_.joinable())
    {
        thread_.join();
    }

    // Cleanup Ping tester
    ping_tester_->Cleanup();

    data_manager_ = nullptr;
    notify_hwnd_ = nullptr;
}

void CWorkerThread::ForceUpdate()
{
    update_requested_ = true;
}

void CWorkerThread::ThreadProc()
{
    while (!stop_requested_)
    {
        auto start_time = std::chrono::steady_clock::now();

        // Get configuration
        const auto& config = data_manager_->GetConfig();
        int interval_ms = config.update_interval_sec * 1000;

        // 1. Get peer list
        std::vector<PeerInfo> peers = FetchPeerList();

        if (!peers.empty())
        {
            // Update peer list to data manager
            data_manager_->UpdatePeerList(peers);

            // 2. Find target peer
            const std::wstring& target_peer_id = config.selected_peer_id;
            const PeerInfo* target_peer = nullptr;

            if (!target_peer_id.empty())
            {
                for (const auto& peer : peers)
                {
                    if (peer.id == target_peer_id)
                    {
                        target_peer = &peer;
                        break;
                    }
                }
            }

            // 3. Ping test (only when target peer exists)
            int ping_ms = -1;
            if (target_peer && !target_peer->ipv4.empty())
            {
                ping_ms = PingPeer(target_peer->ipv4);
            }

            // 4. Determine status
            ConnStatus status = DetermineStatus(target_peer, ping_ms);

            // 5. Create status snapshot
            StatusSnapshot snapshot;
            time(&snapshot.timestamp);
            snapshot.status = status;
            snapshot.ping_ms = ping_ms;

            if (target_peer)
            {
                snapshot.cli_lat_ms = target_peer->lat_ms;
                snapshot.message = StatusToString(status);
            }
            else
            {
                snapshot.cli_lat_ms = -1.0;
                snapshot.message = L"Selected peer not in list";
            }

            // 6. Update status
            data_manager_->UpdateStatus(snapshot);

            // 7. Send notification (if needed)
            if (config.enable_notification)
            {
                SendNotification(snapshot);
            }

            // 8. Notify main thread to update UI
            if (notify_hwnd_)
            {
                ::PostMessage(notify_hwnd_, WM_UPDATE_STATUS, 0, 0);
            }
        }
        else
        {
            // CLI failed, record unknown status
            StatusSnapshot snapshot;
            time(&snapshot.timestamp);
            snapshot.status = ConnStatus::Unknown;
            snapshot.ping_ms = -1;
            snapshot.cli_lat_ms = -1.0;
            snapshot.message = L"Cannot get peer list";

            data_manager_->UpdateStatus(snapshot);
        }

        // Calculate wait time
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        auto wait_ms = interval_ms - elapsed_ms;

        if (wait_ms > 0)
        {
            // Split waiting for easy response to forced updates
            const int CHECK_INTERVAL_MS = 100;
            while (wait_ms > 0 && !update_requested_ && !stop_requested_)
            {
                int sleep_ms = (wait_ms >= CHECK_INTERVAL_MS) ? CHECK_INTERVAL_MS : static_cast<int>(wait_ms);
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
                wait_ms -= sleep_ms;
            }
        }

        update_requested_ = false;
    }
}

std::vector<PeerInfo> CWorkerThread::FetchPeerList()
{
    const auto& config = data_manager_->GetConfig();

    // Call easytier-cli peer
    CliResult result = cli_executor_->Execute(config.cli_path, L"peer", 5000);

    if (!result.success)
    {
        return std::vector<PeerInfo>();
    }

    // Parse output
    return CCliParser::ParsePeerList(result.output);
}

int CWorkerThread::PingPeer(const std::wstring& ipv4)
{
    const auto& config = data_manager_->GetConfig();
    return ping_tester_->Ping(ipv4, config.ping_timeout_ms);
}

ConnStatus CWorkerThread::DetermineStatus(const PeerInfo* peer, int ping_ms)
{
    const auto& config = data_manager_->GetConfig();

    // 1. Peer is null (not in list)
    if (peer == nullptr)
    {
        return ConnStatus::NotFound;
    }

    // 2. Ping failed
    if (ping_ms < 0)
    {
        consecutive_failures_++;

        if (consecutive_failures_ >= config.consecutive_failures_threshold)
        {
            return ConnStatus::Disconnected;
        }

        // Single failure, maintain original status
        return data_manager_->GetCurrentStatus().status;
    }

    // 3. Ping success, reset failure counter
    consecutive_failures_ = 0;

    // 4. Determine connection type based on cost
    if (peer->cost == L"Local" || peer->cost == L"p2p")
    {
        return ConnStatus::Direct;
    }
    else if (peer->cost == L"relay")
    {
        return ConnStatus::Relay;
    }

    // 5. Unknown cost value
    return ConnStatus::Unknown;
}

void CWorkerThread::SendNotification(const StatusSnapshot& snapshot)
{
    const auto& config = data_manager_->GetConfig();
    const auto& current_status = data_manager_->GetCurrentStatus();

    // Only notify on status change
    if (current_status.status == snapshot.status)
        return;

    // Check cooldown time
    time_t now;
    time(&now);

    if (now - last_notification_time_ < config.notification_cooldown_min * 60)
        return;

    bool should_notify = false;
    std::wstring title;
    std::wstring message;

    // Detect disconnect: from connected state to disconnected state
    bool was_connected = (current_status.status == ConnStatus::Direct ||
                         current_status.status == ConnStatus::Relay);
    bool is_disconnected = (snapshot.status == ConnStatus::Disconnected ||
                           snapshot.status == ConnStatus::NotFound);

    if (was_connected && is_disconnected)
    {
        should_notify = true;
        title = L"EasyTier Connection Lost";
        message = L"Connection to peer lost";
    }

    // Detect reconnect: from disconnected state to connected state
    bool was_disconnected = (current_status.status == ConnStatus::Disconnected ||
                            current_status.status == ConnStatus::NotFound);
    bool is_connected = (snapshot.status == ConnStatus::Direct ||
                        snapshot.status == ConnStatus::Relay);

    if (was_disconnected && is_connected)
    {
        should_notify = true;
        title = L"EasyTier Connected";
        message = L"Connection to peer established";
        if (snapshot.status == ConnStatus::Direct)
            message += L" (Direct)";
        else if (snapshot.status == ConnStatus::Relay)
            message += L" (Relay)";
    }

    if (should_notify)
    {
        // Show notification using MessageBox
        MessageBoxW(nullptr, message.c_str(), title.c_str(),
                   MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
        last_notification_time_ = now;
    }
}

} // namespace EasyTierMonitor
