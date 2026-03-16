#pragma once

#include <string>
#include <vector>
#include <deque>
#include <ctime>
#include <cstdint>

namespace EasyTierMonitor {

// Peer Information
struct PeerInfo
{
    std::wstring ipv4;              // Virtual IP address
    std::wstring hostname;          // Hostname
    std::wstring cost;              // Connection type: Local/p2p/relay
    double lat_ms;                 // CLI reported latency (milliseconds)
    double loss_rate;              // Packet loss rate (0-1)
    uint64_t rx_bytes;              // Received bytes
    uint64_t tx_bytes;              // Transmitted bytes
    std::wstring tunnel_proto;      // Tunnel protocol: udp/tcp/wss
    std::wstring nat_type;          // NAT type
    std::wstring id;                // Peer unique ID

    PeerInfo()
        : lat_ms(-1.0)
        , loss_rate(-1.0)
        , rx_bytes(0)
        , tx_bytes(0)
    {}
};

// Connection Status
enum class ConnStatus
{
    Unknown,        // Cannot determine status
    NotFound,       // Peer not in list
    Disconnected,   // Disconnected
    Direct,         // Direct connection (p2p)
    Relay           // Relay connection
};

// Status Snapshot (for history recording)
struct StatusSnapshot
{
    time_t timestamp;            // Timestamp
    ConnStatus status;           // Connection status
    int ping_ms;                 // Ping latency (milliseconds), -1 if timeout
    double cli_lat_ms;            // CLI reported latency
    std::wstring message;       // Status description

    StatusSnapshot()
        : timestamp(0)
        , status(ConnStatus::Unknown)
        , ping_ms(-1)
        , cli_lat_ms(-1.0)
        , message(L"")
    {}
};

// Plugin Configuration
struct PluginConfig
{
    std::wstring selected_peer_id;      // Selected Peer ID
    int update_interval_sec;             // Update interval (seconds)
    bool enable_notification;            // Enable system notification
    int notification_cooldown_min;       // Notification cooldown time (minutes)
    int ping_timeout_ms;                 // Ping timeout (milliseconds)
    int consecutive_failures_threshold;  // Consecutive failures threshold
    std::wstring cli_path;               // easytier-cli path
    int max_history_count;               // Maximum history count

    PluginConfig()
        : update_interval_sec(5)
        , enable_notification(true)
        , notification_cooldown_min(0)
        , ping_timeout_ms(500)
        , consecutive_failures_threshold(3)
        , cli_path(L"easytier-cli")
        , max_history_count(100)
    {}
};

// Runtime State
struct RuntimeState
{
    ConnStatus current_status;
    int current_ping_ms;
    PeerInfo current_peer_info;
    std::deque<StatusSnapshot> history;
    int consecutive_failures;
    time_t last_notification_time;
    bool is_first_update;

    RuntimeState()
        : current_status(ConnStatus::Unknown)
        , current_ping_ms(-1)
        , consecutive_failures(0)
        , last_notification_time(0)
        , is_first_update(true)
    {}
};

// Convert connection status to string
inline const wchar_t* StatusToString(ConnStatus status)
{
    switch (status)
    {
    case ConnStatus::Direct:
        return L"Direct";
    case ConnStatus::Relay:
        return L"Relay";
    case ConnStatus::Disconnected:
        return L"Disconnected";
    case ConnStatus::NotFound:
        return L"NotFound";
    case ConnStatus::Unknown:
    default:
        return L"Unknown";
    }
}

// Convert connection status to icon
inline const wchar_t* StatusToIcon(ConnStatus status)
{
    switch (status)
    {
    case ConnStatus::Direct:
        return L"[O]";
    case ConnStatus::Relay:
        return L"[~]";
    case ConnStatus::Disconnected:
        return L"[X]";
    case ConnStatus::NotFound:
    case ConnStatus::Unknown:
    default:
        return L"[?]";
    }
}

// Format bytes to readable format
inline std::wstring FormatBytes(uint64_t bytes)
{
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    int unit_index = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unit_index < 4)
    {
        size /= 1024.0;
        unit_index++;
    }

    wchar_t buffer[64];
    swprintf_s(buffer, L"%.2f %s", size, units[unit_index]);
    return buffer;
}

// Format time to string
inline std::wstring FormatTime(time_t timestamp)
{
    wchar_t buffer[64];
    struct tm timeinfo;
    localtime_s(&timeinfo, &timestamp);
    wcsftime(buffer, sizeof(buffer) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &timeinfo);
    return buffer;
}

} // namespace EasyTierMonitor
