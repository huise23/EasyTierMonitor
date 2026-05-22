#include "stdafx.h"
#include "Plugin.h"
#include "PluginItem.h"
#include "ConfigDialog.h"
#include "DebugLog.h"
#include <afxwin.h>
#include <Windows.h>
#include <sstream>

namespace
{
std::string TrimAnsi(const std::string& value)
{
    const char* whitespace = " \t\r\n";
    size_t start = value.find_first_not_of(whitespace);
    if (start == std::string::npos)
        return "";
    size_t end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

std::wstring Utf8ToWide(const std::string& value)
{
    if (value.empty())
        return L"";

    int len = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, NULL, 0);
    if (len <= 0)
        return L"";

    std::vector<wchar_t> buffer(len);
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, buffer.data(), len);
    return buffer.data();
}
} // namespace

// Static member definition
CTailscalePlugin* CTailscalePlugin::m_instance = nullptr;

CTailscalePlugin& CTailscalePlugin::Instance()
{
    if (!m_instance)
    {
        m_instance = new CTailscalePlugin();
    }
    return *m_instance;
}

int CTailscalePlugin::GetAPIVersion() const
{
    return 7;
}

CTailscalePlugin::CTailscalePlugin()
    : current_peer_index_(0)
    , selected_peer_index_(-1)  // -1 means auto-rotate
    , auto_rotate_(true)
    , debug_log_enabled_(true)
{
    // Load configuration
    LoadConfig();

    // Apply debug log setting loaded from config.
    CDebugLog::SetEnabled(debug_log_enabled_);
    if (debug_log_enabled_)
    {
        std::wofstream clear_log(L"tailscale_monitor_debug.log", std::ios::trunc);
        clear_log.close();
        DEBUG_LOG_INFO(L"Plugin initializing...");
    }

    // Find tailscale.exe
    cli_path_ = FindTailscaleCli();

    if (cli_path_.empty())
    {
        DEBUG_LOG_ERROR(L"tailscale.exe not found!");
    }
    else
    {
        DEBUG_LOG_INFO(L"Found tailscale.exe at: %s", cli_path_.c_str());
    }

    // Create display item
    status_item_ = std::make_unique<CPluginItem>(nullptr);
    status_item_->SetPlugin(this);

    DEBUG_LOG_INFO(L"Plugin initialized successfully");
}

CTailscalePlugin::~CTailscalePlugin()
{
    CDebugLog::Close();
}

void CTailscalePlugin::SetDebugLogEnabled(bool enable)
{
    debug_log_enabled_ = enable;
    CDebugLog::SetEnabled(enable);
}

std::wstring CTailscalePlugin::FindTailscaleCli()
{
    DEBUG_LOG_INFO(L"Searching for tailscale.exe...");

    // Common install paths on Windows
    const std::vector<std::wstring> candidates = {
        L"C:\\Program Files\\Tailscale\\tailscale.exe",
        L"C:\\Program Files (x86)\\Tailscale\\tailscale.exe",
        L"C:\\Program Files\\tailscale-manager-pro\\resource\\tailscale.exe"
    };

    for (const auto& candidate : candidates)
    {
        DEBUG_LOG_INFO(L"Checking path: %s", candidate.c_str());
        DWORD attrs = GetFileAttributesW(candidate.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY))
        {
            DEBUG_LOG_INFO(L"Found tailscale.exe at: %s", candidate.c_str());
            return candidate;
        }
    }

    // Fallback to PATH resolution
    DEBUG_LOG_WARNING(L"tailscale.exe not found in common locations, fallback to PATH");
    return L"tailscale.exe";
}

void CTailscalePlugin::UpdatePeerList()
{
    // Use temporary list to avoid flickering
    std::vector<SimplePeerInfo> temp_peer_list;

    if (cli_path_.empty())
    {
        DEBUG_LOG_ERROR(L"CLI path is empty, cannot update peer list");
        peer_list_.clear();
        return;
    }

    // Execute tailscale status command (tailscale has no 'peer' subcommand)
    std::wstring command = L"\"" + cli_path_ + L"\" status";
    DEBUG_LOG_INFO(L"Executing command: %s", command.c_str());

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        DEBUG_LOG_ERROR(L"CreatePipe failed: %d", GetLastError());
        peer_list_.clear();
        return;
    }

    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = { 0 };

    if (CreateProcessW(NULL, const_cast<LPWSTR>(command.c_str()), NULL, NULL, TRUE,
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        CloseHandle(hWritePipe);

        // Wait for process to complete (max 5 seconds)
        DWORD wait_result = WaitForSingleObject(pi.hProcess, 5000);
        DEBUG_LOG_INFO(L"Process wait result: %d", wait_result);

        // Read output
        char buffer[4096];
        DWORD bytesRead;
        std::string output;

        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            output += buffer;
        }

        DEBUG_LOG_INFO(L"CLI output length: %d bytes", output.length());

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (!output.empty())
        {
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line))
            {
                line = TrimAnsi(line);
                if (line.empty())
                    continue;

                std::istringstream ls(line);
                std::string ip;
                std::string device_name;
                std::string login_name;
                std::string os_name;

                if (!(ls >> ip >> device_name >> login_name >> os_name))
                    continue;

                std::string detail;
                std::getline(ls, detail);
                detail = TrimAnsi(detail);

                // Exclude local/self row and non-peer rows.
                if (detail == "-")
                    continue;

                // Parse state from the detail field.
                std::string state = "unknown";
                bool is_offline = (detail.find("offline") != std::string::npos);
                bool is_active = (detail.find("active") != std::string::npos);
                if (is_offline)
                    state = "offline";
                else if (detail.find("direct") != std::string::npos)
                    state = "direct";
                else if (detail.find("relay") != std::string::npos)
                    state = "relay";
                else if (is_active)
                    state = "active";

                SimplePeerInfo peer;
                peer.hostname = Utf8ToWide(device_name);
                peer.virtual_ip = Utf8ToWide(ip);
                peer.cost = Utf8ToWide(state);
                peer.is_active = is_active;
                peer.is_offline = is_offline;
                peer.tunnel_latency_ms = -1;
                peer.ping_latency_ms = -1;
                temp_peer_list.push_back(std::move(peer));
            }
        }

        // Atomically replace
        peer_list_ = std::move(temp_peer_list);
        DEBUG_LOG_INFO(L"Parsed %d peers from tailscale status output", peer_list_.size());
    }
    else
    {
        CloseHandle(hWritePipe);
        DEBUG_LOG_ERROR(L"CreateProcess failed: %d", GetLastError());
        peer_list_.clear();
    }

    CloseHandle(hReadPipe);
}

IPluginItem* CTailscalePlugin::GetItem(int index)
{
    if (index == 0 && status_item_)
    {
        return status_item_.get();
    }
    return nullptr;
}

void CTailscalePlugin::DataRequired()
{
    DEBUG_LOG_INFO(L"DataRequired called");

    // Update peer list periodically
    UpdatePeerList();

    // Update ping latency for current peer only (to avoid blocking)
    if (!peer_list_.empty() && current_peer_index_ >= 0 && current_peer_index_ < (int)peer_list_.size())
    {
        auto& current_peer = peer_list_[current_peer_index_];
        if (current_peer.cost == L"offline")
        {
            current_peer.ping_latency_ms = -1;
            DEBUG_LOG_INFO(L"Peer is offline, skip ping: %s", current_peer.hostname.c_str());
        }
        else
        {
            DEBUG_LOG_INFO(L"Pinging peer %d: %s", current_peer_index_, current_peer.virtual_ip.c_str());
            int ping_result = PingHost(current_peer.virtual_ip);
            current_peer.ping_latency_ms = ping_result;
            DEBUG_LOG_INFO(L"Ping result: %d ms", ping_result);
        }
    }

    // Advance to next peer for rotation
    AdvanceToNextPeer();

    DEBUG_LOG_INFO(L"DataRequired completed, peer_list size: %d", peer_list_.size());
}

void CTailscalePlugin::UpdatePingLatency()
{
    for (auto& peer : peer_list_)
    {
        peer.ping_latency_ms = PingHost(peer.virtual_ip);
    }
}

int CTailscalePlugin::PingHost(const std::wstring& ip_address)
{
    HANDLE hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE)
        return -1;

    // Convert wide string to narrow string
    int len = WideCharToMultiByte(CP_UTF8, 0, ip_address.c_str(), -1, NULL, 0, NULL, NULL);
    if (len <= 0)
    {
        IcmpCloseHandle(hIcmpFile);
        return -1;
    }

    std::vector<char> ip_str(len);
    WideCharToMultiByte(CP_UTF8, 0, ip_address.c_str(), -1, ip_str.data(), len, NULL, NULL);

    // Convert IP string to address
    unsigned long ipaddr = inet_addr(ip_str.data());
    if (ipaddr == INADDR_NONE)
    {
        IcmpCloseHandle(hIcmpFile);
        return -1;
    }

    // Prepare ping data
    char SendData[32] = "TailscaleMonitor";
    DWORD ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
    std::vector<char> ReplyBuffer(ReplySize);

    // Send ping with 1 second timeout
    DWORD dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, SendData, sizeof(SendData),
                                   NULL, ReplyBuffer.data(), ReplySize, 1000);

    int latency = -1;
    if (dwRetVal != 0)
    {
        PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer.data();
        if (pEchoReply->Status == IP_SUCCESS)
        {
            latency = static_cast<int>(pEchoReply->RoundTripTime);
        }
    }

    IcmpCloseHandle(hIcmpFile);
    return latency;
}

void CTailscalePlugin::AdvanceToNextPeer()
{
    if (peer_list_.empty())
    {
        current_peer_index_ = 0;
        return;
    }

    // If auto-rotate is disabled and a specific peer is selected
    if (!auto_rotate_ && !selected_peer_hostname_.empty())
    {
        // Find peer by hostname
        for (size_t i = 0; i < peer_list_.size(); i++)
        {
            if (peer_list_[i].hostname == selected_peer_hostname_)
            {
                current_peer_index_ = static_cast<int>(i);
                selected_peer_index_ = static_cast<int>(i);
                return;
            }
        }

        // If hostname not found, fall back to auto-rotate
        DEBUG_LOG_WARNING(L"Selected peer hostname not found: %s, falling back to auto-rotate",
                         selected_peer_hostname_.c_str());
    }

    // Auto-rotate mode
    current_peer_index_ = (current_peer_index_ + 1) % peer_list_.size();
}

const wchar_t* CTailscalePlugin::GetInfo(PluginInfoIndex index)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    switch (index)
    {
    case TMI_NAME:
        return L"Tailscale Monitor";
    case TMI_DESCRIPTION:
        return L"Minimal test plugin";
    case TMI_AUTHOR:
        return L"Test";
    case TMI_COPYRIGHT:
        return L"Copyright (C) 2026";
    case TMI_VERSION:
        return L"0.1.0";
    case TMI_URL:
        return L"";
    default:
        return L"";
    }
}

ITMPlugin::OptionReturn CTailscalePlugin::ShowOptionsDialog(void* hParent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    try
    {
        CWnd* pParent = CWnd::FromHandle((HWND)hParent);
        CConfigDialog dlg(this, pParent);

        if (dlg.DoModal() == IDOK)
        {
            return OR_OPTION_CHANGED;
        }
    }
    catch (...)
    {
        DEBUG_LOG_ERROR(L"Exception in ShowOptionsDialog");
        return OR_OPTION_UNCHANGED;
    }

    return OR_OPTION_UNCHANGED;
}

void CTailscalePlugin::OnMonitorInfo(const MonitorInfo& monitor_info)
{
}

void CTailscalePlugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data)
{
}

// Export function implementation
ITMPlugin* TMPluginGetInstance()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return &CTailscalePlugin::Instance();
}

void CTailscalePlugin::LoadConfig()
{
    std::wifstream config_file(L"tailscale_monitor_config.ini");
    if (config_file.is_open())
    {
        std::wstring line;
        while (std::getline(config_file, line))
        {
            if (line.find(L"auto_rotate=") == 0)
            {
                auto_rotate_ = (line.substr(12) == L"1");
            }
            else if (line.find(L"debug_log=") == 0)
            {
                debug_log_enabled_ = (line.substr(10) == L"1");
            }
            else if (line.find(L"selected_peer_hostname=") == 0)
            {
                selected_peer_hostname_ = line.substr(23);
            }
        }
        config_file.close();
    }
}

void CTailscalePlugin::SaveConfig()
{
    std::wofstream config_file(L"tailscale_monitor_config.ini");
    if (config_file.is_open())
    {
        config_file << L"auto_rotate=" << (auto_rotate_ ? L"1" : L"0") << std::endl;
        config_file << L"debug_log=" << (debug_log_enabled_ ? L"1" : L"0") << std::endl;
        config_file << L"selected_peer_hostname=" << selected_peer_hostname_ << std::endl;
        config_file.close();
        DEBUG_LOG_INFO(L"Config saved: auto_rotate=%d, selected_peer_hostname=%s",
                      auto_rotate_, selected_peer_hostname_.c_str());
    }
}
