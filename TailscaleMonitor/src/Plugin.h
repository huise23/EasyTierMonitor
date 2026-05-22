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
    bool is_active;             // active in tailscale status
    bool is_offline;            // offline in tailscale status
    int tunnel_latency_ms;      // from CLI
    int ping_latency_ms;        // from ICMP ping

    SimplePeerInfo()
        : is_active(false)
        , is_offline(false)
        , tunnel_latency_ms(-1)
        , ping_latency_ms(-1)
    {}
};

// Plugin Class (implements ITMPlugin)
class CTailscalePlugin : public ITMPlugin
{
public:
    // Get singleton instance
    static CTailscalePlugin& Instance();

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
    bool HasCliPath() const { return !cli_path_.empty(); }
    const std::wstring& GetCliPath() const { return cli_path_; }
    void AdvanceToNextPeer();

    // Configuration
    void SetSelectedPeerIndex(int index) { selected_peer_index_ = index; }
    int GetSelectedPeerIndex() const { return selected_peer_index_; }
    bool IsAutoRotate() const { return auto_rotate_; }
    void SetAutoRotate(bool enable) { auto_rotate_ = enable; }
    bool IsDebugLogEnabled() const { return debug_log_enabled_; }
    void SetDebugLogEnabled(bool enable);
    void SetSelectedPeerHostname(const std::wstring& hostname) { selected_peer_hostname_ = hostname; }
    void SaveConfig();

private:
    CTailscalePlugin();
    ~CTailscalePlugin();

    // Singleton instance (pointer for lazy initialization)
    static CTailscalePlugin* m_instance;

    // Helper methods
    std::wstring FindTailscaleCli();
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
    bool debug_log_enabled_;     // enable debug log output
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
