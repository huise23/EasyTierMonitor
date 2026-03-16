#pragma once

#include "PluginInterface.h"
#include "PluginItem.h"
#include <memory>
#include <string>
#include <vector>

// Simple peer info structure
struct SimplePeerInfo
{
    std::wstring hostname;
    std::wstring virtual_ip;
    std::wstring cost;          // p2p, relay, etc.
    int tunnel_latency_ms;      // from CLI
    int ping_latency_ms;        // from ICMP ping
};

// Plugin Class (implements ITMPlugin)
class CEasyTierPlugin : public ITMPlugin
{
public:
    // Get singleton instance
    static CEasyTierPlugin& Instance();

    // ITMPlugin interface implementation
    virtual int GetAPIVersion() const override;
    virtual IPluginItem* GetItem(int index) override;
    virtual void DataRequired() override;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override;
    virtual OptionReturn ShowOptionsDialog(void* hParent) override;
    virtual void OnMonitorInfo(const MonitorInfo& monitor_info) override;
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) override;

    // Public accessor for PluginItem
    const std::vector<SimplePeerInfo>& GetPeerList() const { return peer_list_; }
    int GetCurrentPeerIndex() const { return current_peer_index_; }
    void AdvanceToNextPeer();

    // Configuration
    void SetSelectedPeerIndex(int index) { selected_peer_index_ = index; }
    int GetSelectedPeerIndex() const { return selected_peer_index_; }
    bool IsAutoRotate() const { return auto_rotate_; }
    void SetAutoRotate(bool enable) { auto_rotate_ = enable; }
    void SetSelectedPeerHostname(const std::wstring& hostname) { selected_peer_hostname_ = hostname; }
    void SaveConfig();

private:
    CEasyTierPlugin();
    ~CEasyTierPlugin();

    // Singleton instance (pointer for lazy initialization)
    static CEasyTierPlugin* m_instance;

    // Helper methods
    std::wstring FindEasyTierCli();
    void UpdatePeerList();
    void UpdatePingLatency();
    int PingHost(const std::wstring& ip_address);
    void LoadConfig();

    // Member variables
    std::wstring cli_path_;
    std::vector<SimplePeerInfo> peer_list_;
    int current_peer_index_;
    int selected_peer_index_;   // -1 for auto-rotate, >= 0 for fixed peer
    bool auto_rotate_;           // true for auto-rotate mode
    std::wstring selected_peer_hostname_;  // hostname of selected peer for persistence

    // Components
    std::unique_ptr<CPluginItem> status_item_;
};

// Export function declaration
#ifdef __cplusplus
extern "C" {
#endif
    __declspec(dllexport) ITMPlugin* TMPluginGetInstance();

#ifdef __cplusplus
}
#endif
