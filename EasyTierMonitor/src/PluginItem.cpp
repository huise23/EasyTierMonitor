#include "stdafx.h"
#include "PluginItem.h"
#include "Plugin.h"
#include <sstream>

CPluginItem::CPluginItem(CDataManager* data_manager)
    : data_manager_(data_manager)
    , plugin_(nullptr)
{
}

CPluginItem::~CPluginItem()
{
}

const wchar_t* CPluginItem::GetItemName() const
{
    return L"EasyTier Status";
}

const wchar_t* CPluginItem::GetItemId() const
{
    return L"easytier_status";
}

const wchar_t* CPluginItem::GetItemLableText() const
{
    return L"";
}

const wchar_t* CPluginItem::GetItemValueText() const
{
    if (!plugin_)
    {
        cached_text_ = L"No Plugin";
        return cached_text_.c_str();
    }

    // Get current peer info from plugin
    const auto& peer_list = plugin_->GetPeerList();

    if (peer_list.empty())
    {
        cached_text_ = L"No Peers";
        return cached_text_.c_str();
    }

    // Get current peer index for rotation
    int index = plugin_->GetCurrentPeerIndex();
    if (index < 0 || index >= static_cast<int>(peer_list.size()))
        index = 0;

    const auto& peer = peer_list[index];

    // Simplify hostname: replace "PublicServer_" with "_" to keep the prefix
    std::wstring display_hostname = peer.hostname;
    const std::wstring prefix = L"PublicServer_";
    if (display_hostname.find(prefix) == 0)
    {
        display_hostname = L"_" + display_hostname.substr(prefix.length());
    }

    std::wstringstream ss;
    ss << display_hostname << L" | " << peer.cost << L" | ";

    // Tunnel latency
    if (peer.tunnel_latency_ms >= 0)
        ss << peer.tunnel_latency_ms << L"ms";
    else
        ss << L"-";

    ss << L" | ";

    // Ping latency
    if (peer.ping_latency_ms >= 0)
        ss << peer.ping_latency_ms << L"ms";
    else
        ss << L"-";

    cached_text_ = ss.str();
    return cached_text_.c_str();
}

const wchar_t* CPluginItem::GetItemValueSampleText() const
{
    return L"_HostName | p2p | 99ms | 99ms";
}

bool CPluginItem::IsCustomDraw() const
{
    return false;
}

int CPluginItem::GetItemWidth() const
{
    return 220;  // Moderate width for content
}

void CPluginItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    // Not using custom draw
}

int CPluginItem::GetItemWidthEx(void* hDC) const
{
    return 0;
}

int CPluginItem::OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag)
{
    if (type == MT_RCLICKED && plugin_)
    {
        // Show context menu on right click
        HMENU hMenu = CreatePopupMenu();
        if (hMenu)
        {
            // Add "Auto Rotate" option
            AppendMenuW(hMenu, MF_STRING | (plugin_->IsAutoRotate() ? MF_CHECKED : 0), 1, L"Auto Rotate");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

            // Add peer list
            const auto& peer_list = plugin_->GetPeerList();
            int current_selected = plugin_->GetSelectedPeerIndex();

            for (size_t i = 0; i < peer_list.size(); i++)
            {
                // Simplify hostname
                std::wstring display_name = peer_list[i].hostname;
                const std::wstring prefix = L"PublicServer_";
                if (display_name.find(prefix) == 0)
                {
                    display_name = L"_" + display_name.substr(prefix.length());
                }

                UINT flags = MF_STRING;
                if (!plugin_->IsAutoRotate() && current_selected == (int)i)
                {
                    flags |= MF_CHECKED;
                }

                AppendMenuW(hMenu, flags, 100 + i, display_name.c_str());
            }

            // Show menu
            POINT pt = { x, y };
            ClientToScreen((HWND)hWnd, &pt);

            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN, pt.x, pt.y, 0, (HWND)hWnd, NULL);

            if (cmd == 1)
            {
                // Toggle auto rotate
                plugin_->SetAutoRotate(true);
                plugin_->SetSelectedPeerIndex(-1);
                plugin_->SetSelectedPeerHostname(L"");
                plugin_->SaveConfig();
            }
            else if (cmd >= 100)
            {
                // Select specific peer
                int peer_index = cmd - 100;
                if (peer_index >= 0 && peer_index < (int)peer_list.size())
                {
                    plugin_->SetAutoRotate(false);
                    plugin_->SetSelectedPeerIndex(peer_index);
                    plugin_->SetSelectedPeerHostname(peer_list[peer_index].hostname);
                    plugin_->SaveConfig();
                }
            }

            DestroyMenu(hMenu);
            return 1;  // Event handled
        }
    }

    return 0;
}

int CPluginItem::OnKeboardEvent(int key, bool ctrl, bool shift, bool alt, void* hWnd, int flag)
{
    return 0;
}
