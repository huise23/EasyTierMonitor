#include "stdafx.h"
#include "Plugin.h"
#include "PluginItem.h"
#include "ConfigDialog.h"
#include "DebugLog.h"
#include <afxwin.h>
#include <Windows.h>
#include <sstream>

// Static member definition
CEasyTierPlugin* CEasyTierPlugin::m_instance = nullptr;

CEasyTierPlugin& CEasyTierPlugin::Instance()
{
    if (!m_instance)
    {
        m_instance = new CEasyTierPlugin();
    }
    return *m_instance;
}

int CEasyTierPlugin::GetAPIVersion() const
{
    return 7;
}

CEasyTierPlugin::CEasyTierPlugin()
    : current_peer_index_(0)
    , selected_peer_index_(-1)  // -1 means auto-rotate
    , auto_rotate_(true)
{
    // Clear log file on startup
    std::wofstream clear_log(L"easytier_monitor_debug.log", std::ios::trunc);
    clear_log.close();

    DEBUG_LOG_INFO(L"Plugin initializing...");

    // Load configuration
    LoadConfig();

    // Find easytier-cli.exe
    cli_path_ = FindEasyTierCli();

    if (cli_path_.empty())
    {
        DEBUG_LOG_ERROR(L"easytier-cli.exe not found!");
    }
    else
    {
        DEBUG_LOG_INFO(L"Found easytier-cli.exe at: %s", cli_path_.c_str());
    }

    // Create display item
    status_item_ = std::make_unique<CPluginItem>(nullptr);
    status_item_->SetPlugin(this);

    DEBUG_LOG_INFO(L"Plugin initialized successfully");
}

CEasyTierPlugin::~CEasyTierPlugin()
{
}

std::wstring CEasyTierPlugin::FindEasyTierCli()
{
    DEBUG_LOG_INFO(L"Searching for easytier-cli.exe...");

    // Default path
    std::wstring default_path = L"C:\\Program Files\\easytier-manager-pro\\resource\\easytier-cli.exe";
    DEBUG_LOG_INFO(L"Checking default path: %s", default_path.c_str());

    // Check if file exists
    DWORD attrs = GetFileAttributesW(default_path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY))
    {
        DEBUG_LOG_INFO(L"Found at default path");
        return default_path;
    }
    else
    {
        DEBUG_LOG_WARNING(L"Not found at default path (attrs=0x%08X)", attrs);
    }

    return L"";
}

void CEasyTierPlugin::UpdatePeerList()
{
    // Use temporary list to avoid flickering
    std::vector<SimplePeerInfo> temp_peer_list;

    if (cli_path_.empty())
    {
        DEBUG_LOG_ERROR(L"CLI path is empty, cannot update peer list");
        return;
    }

    // Execute easytier-cli peer command
    std::wstring command = L"\"" + cli_path_ + L"\" peer";
    DEBUG_LOG_INFO(L"Executing command: %s", command.c_str());

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        DEBUG_LOG_ERROR(L"CreatePipe failed: %d", GetLastError());
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

        // Log full output for debugging (split if too long)
        if (!output.empty())
        {
            // Convert to wide string for logging
            int wlen = MultiByteToWideChar(CP_UTF8, 0, output.c_str(), -1, NULL, 0);
            if (wlen > 0)
            {
                std::vector<wchar_t> wbuf(wlen);
                MultiByteToWideChar(CP_UTF8, 0, output.c_str(), -1, wbuf.data(), wlen);

                // Log in chunks if needed
                std::wstring full_output = wbuf.data();
                size_t chunk_size = 1000;
                for (size_t i = 0; i < full_output.length(); i += chunk_size)
                {
                    std::wstring chunk = full_output.substr(i, chunk_size);
                    DEBUG_LOG_INFO(L"CLI output part %d: %s", i / chunk_size + 1, chunk.c_str());
                }
            }
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // Parse table output
        if (!output.empty())
        {
            std::istringstream iss(output);
            std::string line;
            int line_num = 0;
            std::string current_ip;
            std::string current_hostname;

            while (std::getline(iss, line))
            {
                line_num++;
                // Skip header lines (first 2 lines)
                if (line_num <= 2) continue;

                // Skip empty lines
                if (line.empty() || line.find_first_not_of(" \t\r\n|") == std::string::npos)
                    continue;

                // Parse table row: | ipv4 | hostname | cost | lat(ms) | ...
                std::vector<std::string> fields;
                std::istringstream line_stream(line);
                std::string field;

                while (std::getline(line_stream, field, '|'))
                {
                    // Trim whitespace
                    size_t start = field.find_first_not_of(" \t\r\n");
                    size_t end = field.find_last_not_of(" \t\r\n");
                    if (start != std::string::npos && end != std::string::npos)
                        fields.push_back(field.substr(start, end - start + 1));
                    else
                        fields.push_back("");
                }

                // Skip first empty field (before first |)
                if (!fields.empty() && fields[0].empty())
                    fields.erase(fields.begin());

                if (fields.size() >= 4)
                {
                    DEBUG_LOG_INFO(L"Parsing line %d, fields count: %d", line_num, fields.size());

                    // Field 0: ipv4 (may be empty for continuation rows)
                    if (!fields[0].empty() && fields[0] != "-")
                    {
                        current_ip = fields[0];
                        // Extract IP without CIDR
                        size_t slash_pos = current_ip.find('/');
                        if (slash_pos != std::string::npos)
                            current_ip = current_ip.substr(0, slash_pos);

                        DEBUG_LOG_INFO(L"  IP updated: %S", current_ip.c_str());
                    }

                    // Field 1: hostname
                    std::string hostname = fields[1];
                    if (!hostname.empty() && hostname != "-")
                    {
                        current_hostname = hostname;
                        DEBUG_LOG_INFO(L"  Hostname updated: %S", current_hostname.c_str());
                    }

                    // Field 2: cost (Local, p2p, relay, etc.)
                    std::string cost = fields[2];
                    DEBUG_LOG_INFO(L"  Cost: %S", cost.c_str());

                    // Field 3: latency
                    std::string latency_str = fields[3];

                    // Skip local peer and empty hostnames
                    if (cost == "Local" || current_hostname.empty())
                    {
                        DEBUG_LOG_INFO(L"  Skipped (Local or empty hostname)");
                        continue;
                    }

                    // Only add peers with valid IP and hostname
                    if (!current_ip.empty() && !current_hostname.empty())
                    {
                        SimplePeerInfo peer;

                        // Convert hostname to wide string
                        int len = MultiByteToWideChar(CP_UTF8, 0, current_hostname.c_str(), -1, NULL, 0);
                        if (len > 0)
                        {
                            std::vector<wchar_t> buf(len);
                            MultiByteToWideChar(CP_UTF8, 0, current_hostname.c_str(), -1, buf.data(), len);
                            peer.hostname = buf.data();
                        }

                        // Convert IP to wide string
                        len = MultiByteToWideChar(CP_UTF8, 0, current_ip.c_str(), -1, NULL, 0);
                        if (len > 0)
                        {
                            std::vector<wchar_t> buf(len);
                            MultiByteToWideChar(CP_UTF8, 0, current_ip.c_str(), -1, buf.data(), len);
                            peer.virtual_ip = buf.data();
                        }

                        // Convert cost to wide string
                        len = MultiByteToWideChar(CP_UTF8, 0, cost.c_str(), -1, NULL, 0);
                        if (len > 0)
                        {
                            std::vector<wchar_t> buf(len);
                            MultiByteToWideChar(CP_UTF8, 0, cost.c_str(), -1, buf.data(), len);
                            peer.cost = buf.data();
                        }

                        // Parse tunnel latency
                        peer.tunnel_latency_ms = -1;
                        if (!latency_str.empty() && latency_str != "-")
                        {
                            try {
                                peer.tunnel_latency_ms = static_cast<int>(std::stof(latency_str));
                            } catch (...) {
                                peer.tunnel_latency_ms = -1;
                            }
                        }

                        // Initialize ping latency (will be measured separately)
                        peer.ping_latency_ms = -1;

                        temp_peer_list.push_back(peer);
                        DEBUG_LOG_INFO(L"Added peer: %s (%s) cost=%s lat=%dms",
                                      peer.hostname.c_str(), peer.virtual_ip.c_str(),
                                      peer.cost.c_str(), peer.tunnel_latency_ms);
                    }
                }
            }
        }

        // Atomically replace the peer list to avoid flickering
        peer_list_ = std::move(temp_peer_list);

        DEBUG_LOG_INFO(L"Parsed %d peers from CLI output", peer_list_.size());
    }
    else
    {
        CloseHandle(hWritePipe);
        DEBUG_LOG_ERROR(L"CreateProcess failed: %d", GetLastError());
    }

    CloseHandle(hReadPipe);
}

IPluginItem* CEasyTierPlugin::GetItem(int index)
{
    if (index == 0 && status_item_)
    {
        return status_item_.get();
    }
    return nullptr;
}

void CEasyTierPlugin::DataRequired()
{
    DEBUG_LOG_INFO(L"DataRequired called");

    // Update peer list periodically
    UpdatePeerList();

    // Update ping latency for current peer only (to avoid blocking)
    if (!peer_list_.empty() && current_peer_index_ >= 0 && current_peer_index_ < (int)peer_list_.size())
    {
        DEBUG_LOG_INFO(L"Pinging peer %d: %s", current_peer_index_, peer_list_[current_peer_index_].virtual_ip.c_str());
        int ping_result = PingHost(peer_list_[current_peer_index_].virtual_ip);
        peer_list_[current_peer_index_].ping_latency_ms = ping_result;
        DEBUG_LOG_INFO(L"Ping result: %d ms", ping_result);
    }

    // Advance to next peer for rotation
    AdvanceToNextPeer();

    DEBUG_LOG_INFO(L"DataRequired completed, peer_list size: %d", peer_list_.size());
}

void CEasyTierPlugin::UpdatePingLatency()
{
    for (auto& peer : peer_list_)
    {
        peer.ping_latency_ms = PingHost(peer.virtual_ip);
    }
}

int CEasyTierPlugin::PingHost(const std::wstring& ip_address)
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
    char SendData[32] = "EasyTierMonitor";
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

void CEasyTierPlugin::AdvanceToNextPeer()
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

const wchar_t* CEasyTierPlugin::GetInfo(PluginInfoIndex index)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    switch (index)
    {
    case TMI_NAME:
        return L"EasyTier Monitor";
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

ITMPlugin::OptionReturn CEasyTierPlugin::ShowOptionsDialog(void* hParent)
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

void CEasyTierPlugin::OnMonitorInfo(const MonitorInfo& monitor_info)
{
}

void CEasyTierPlugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data)
{
}

// Export function implementation
ITMPlugin* TMPluginGetInstance()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return &CEasyTierPlugin::Instance();
}

void CEasyTierPlugin::LoadConfig()
{
    std::wifstream config_file(L"easytier_monitor_config.ini");
    if (config_file.is_open())
    {
        std::wstring line;
        while (std::getline(config_file, line))
        {
            if (line.find(L"auto_rotate=") == 0)
            {
                auto_rotate_ = (line.substr(12) == L"1");
            }
            else if (line.find(L"selected_peer_hostname=") == 0)
            {
                selected_peer_hostname_ = line.substr(23);
            }
        }
        config_file.close();
        DEBUG_LOG_INFO(L"Config loaded: auto_rotate=%d, selected_peer_hostname=%s",
                      auto_rotate_, selected_peer_hostname_.c_str());
    }
}

void CEasyTierPlugin::SaveConfig()
{
    std::wofstream config_file(L"easytier_monitor_config.ini");
    if (config_file.is_open())
    {
        config_file << L"auto_rotate=" << (auto_rotate_ ? L"1" : L"0") << std::endl;
        config_file << L"selected_peer_hostname=" << selected_peer_hostname_ << std::endl;
        config_file.close();
        DEBUG_LOG_INFO(L"Config saved: auto_rotate=%d, selected_peer_hostname=%s",
                      auto_rotate_, selected_peer_hostname_.c_str());
    }
}
