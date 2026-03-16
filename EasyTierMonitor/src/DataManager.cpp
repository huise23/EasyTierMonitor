#include "stdafx.h"
#include "DataManager.h"
#include <sstream>
#include <algorithm>

namespace EasyTierMonitor {

CDataManager::CDataManager()
{
    // Use default configuration initialization
}

const PluginConfig& CDataManager::GetConfig() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void CDataManager::SetConfig(const PluginConfig& config)
{
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

void CDataManager::UpdateConfig(const std::wstring& peer_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    config_.selected_peer_id = peer_id;
}

bool CDataManager::LoadConfig(const std::wstring& config_dir)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (config_dir.empty())
        return false;

    std::wstring config_file = config_dir;
    if (config_file.back() != L'\\' && config_file.back() != L'/')
        config_file += L'\\';
    config_file += L"easytier_monitor_config.ini";

    return ReadSimpleConfig(config_file);
}

bool CDataManager::SaveConfig(const std::wstring& config_dir)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (config_dir.empty())
        return false;

    std::wstring config_file = config_dir;
    if (config_file.back() != L'\\' && config_file.back() != L'/')
        config_file += L'\\';
    config_file += L"easytier_monitor_config.ini";

    return WriteSimpleConfig(config_file);
}

void CDataManager::UpdateStatus(const StatusSnapshot& snapshot)
{
    std::lock_guard<std::mutex> lock(mutex_);

    current_status_ = snapshot;

    // Add to history records
    history_.push_back(snapshot);

    // Limit history records count
    while (history_.size() > static_cast<size_t>(config_.max_history_count))
    {
        history_.pop_front();
    }
}

StatusSnapshot CDataManager::GetCurrentStatus() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return current_status_;
}

void CDataManager::UpdatePeerList(const std::vector<PeerInfo>& peers)
{
    std::lock_guard<std::mutex> lock(mutex_);
    peer_list_ = peers;
}

std::vector<PeerInfo> CDataManager::GetPeerList() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return peer_list_;
}

PeerInfo* CDataManager::FindPeer(const std::wstring& peer_id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find_if(peer_list_.begin(), peer_list_.end(),
        [&peer_id](const PeerInfo& peer) { return peer.id == peer_id; });

    if (it != peer_list_.end())
        return &(*it);

    return nullptr;
}

const PeerInfo& CDataManager::GetCurrentPeerInfo() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Find based on selected_peer_id
    if (!config_.selected_peer_id.empty())
    {
        auto it = std::find_if(peer_list_.begin(), peer_list_.end(),
            [this](const PeerInfo& peer) { return peer.id == config_.selected_peer_id; });

        if (it != peer_list_.end())
            return *it;
    }

    return empty_peer_info_;
}

std::vector<StatusSnapshot> CDataManager::GetHistory(int count) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<StatusSnapshot> result;

    int start = (std::max)(0, static_cast<int>(history_.size()) - count);
    for (size_t i = start; i < history_.size(); ++i)
    {
        result.push_back(history_[i]);
    }

    return result;
}

bool CDataManager::WriteSimpleConfig(const std::wstring& file_path)
{
    try
    {
        std::ofstream file(file_path);
        if (!file.is_open())
            return false;

        file << "[EasyTier Monitor Config]\n";

        if (!config_.selected_peer_id.empty())
            file << "peer_id=" << std::string(config_.selected_peer_id.begin(), config_.selected_peer_id.end()) << "\n";

        file << "update_interval=" << config_.update_interval_sec << "\n";
        file << "enable_notification=" << (config_.enable_notification ? "1" : "0") << "\n";
        file << "notification_cooldown=" << config_.notification_cooldown_min << "\n";
        file << "ping_timeout=" << config_.ping_timeout_ms << "\n";
        file << "consecutive_failures=" << config_.consecutive_failures_threshold << "\n";

        if (!config_.cli_path.empty() && config_.cli_path != L"easytier-cli")
            file << "cli_path=" << std::string(config_.cli_path.begin(), config_.cli_path.end()) << "\n";

        file << "max_history=" << config_.max_history_count << "\n";

        file.close();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool CDataManager::ReadSimpleConfig(const std::wstring& file_path)
{
    try
    {
        std::ifstream file(file_path);
        if (!file.is_open())
            return false;

        std::string line;
        while (std::getline(file, line))
        {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#' || line[0] == '[')
                continue;

            size_t pos = line.find('=');
            if (pos == std::string::npos)
                continue;

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Parse configuration items
            if (key == "peer_id")
            {
                config_.selected_peer_id = std::wstring(value.begin(), value.end());
            }
            else if (key == "update_interval")
            {
                config_.update_interval_sec = std::stoi(value);
            }
            else if (key == "enable_notification")
            {
                config_.enable_notification = (std::stoi(value) != 0);
            }
            else if (key == "notification_cooldown")
            {
                config_.notification_cooldown_min = std::stoi(value);
            }
            else if (key == "ping_timeout")
            {
                config_.ping_timeout_ms = std::stoi(value);
            }
            else if (key == "consecutive_failures")
            {
                config_.consecutive_failures_threshold = std::stoi(value);
            }
            else if (key == "cli_path")
            {
                config_.cli_path = std::wstring(value.begin(), value.end());
            }
            else if (key == "max_history")
            {
                config_.max_history_count = std::stoi(value);
            }
        }

        file.close();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::wstring CDataManager::FormatConfigLine(const std::wstring& key, const std::wstring& value)
{
    return key + L"=" + value + L"\n";
}

std::wstring CDataManager::ParseConfigValue(const std::wstring& content, const std::wstring& key)
{
    std::wstring search = key + L"=";
    size_t pos = content.find(search);
    if (pos == std::wstring::npos)
        return L"";

    size_t start = pos + search.length();
    size_t end = content.find(L'\n', start);
    if (end == std::wstring::npos)
        end = content.length();

    return content.substr(start, end - start);
}

} // namespace EasyTierMonitor
