#pragma once

#include "../include/Common.h"
#include <mutex>
#include <fstream>
#include <vector>

namespace EasyTierMonitor {

// Data Manager
class CDataManager
{
public:
    CDataManager();
    ~CDataManager() = default;

    // Configuration management
    bool LoadConfig(const std::wstring& config_dir);
    bool SaveConfig(const std::wstring& config_dir);

    const PluginConfig& GetConfig() const;
    void SetConfig(const PluginConfig& config);
    void UpdateConfig(const std::wstring& peer_id);

    // Status management
    void UpdateStatus(const StatusSnapshot& snapshot);
    StatusSnapshot GetCurrentStatus() const;

    // Peer management
    void UpdatePeerList(const std::vector<PeerInfo>& peers);
    std::vector<PeerInfo> GetPeerList() const;
    PeerInfo* FindPeer(const std::wstring& peer_id);
    const PeerInfo& GetCurrentPeerInfo() const;

    // History records
    std::vector<StatusSnapshot> GetHistory(int count) const;

private:
    // JSON config read/write (simplified version, should use JSON library in production)
    bool WriteSimpleConfig(const std::wstring& file_path);
    bool ReadSimpleConfig(const std::wstring& file_path);

    std::wstring FormatConfigLine(const std::wstring& key, const std::wstring& value);
    std::wstring ParseConfigValue(const std::wstring& content, const std::wstring& key);

private:
    mutable std::mutex mutex_;

    PluginConfig config_;
    StatusSnapshot current_status_;
    std::deque<StatusSnapshot> history_;
    std::vector<PeerInfo> peer_list_;
    PeerInfo empty_peer_info_;
};

} // namespace EasyTierMonitor
