#include "stdafx.h"
#include "ConfigDialog.h"
#include "../res/resource.h"

IMPLEMENT_DYNAMIC(CConfigDialog, CDialogEx)

CConfigDialog::CConfigDialog(CEasyTierPlugin* plugin, CWnd* pParent)
    : CDialogEx(IDD, pParent)
    , plugin_(plugin)
{
}

CConfigDialog::~CConfigDialog()
{
}

void CConfigDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PEER_COMBO, peer_combo_);
    DDX_Control(pDX, IDC_ENABLE_NOTIFICATION_CHECK, auto_rotate_check_);
}

BEGIN_MESSAGE_MAP(CConfigDialog, CDialogEx)
    ON_BN_CLICKED(IDC_REFRESH_PEER_LIST, &CConfigDialog::OnBnClickedRefreshPeerList)
END_MESSAGE_MAP()

BOOL CConfigDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Set dialog title
    SetWindowTextW(L"EasyTier Monitor Configuration");

    // Update peer list combo
    UpdatePeerCombo();

    // Set auto-rotate checkbox (reuse notification check control)
    auto_rotate_check_.SetCheck(plugin_->IsAutoRotate() ? BST_CHECKED : BST_UNCHECKED);

    // Set current selection based on hostname
    if (!plugin_->IsAutoRotate())
    {
        int selected = plugin_->GetSelectedPeerIndex();
        if (selected >= 0 && selected < peer_combo_.GetCount() - 1)
        {
            peer_combo_.SetCurSel(selected + 1); // +1 because index 0 is "Auto Rotate"
        }
    }
    else
    {
        peer_combo_.SetCurSel(0); // "Auto Rotate"
    }

    return TRUE;
}

void CConfigDialog::UpdatePeerCombo()
{
    peer_combo_.ResetContent();

    // Add "Auto Rotate" option
    peer_combo_.AddString(L"Auto Rotate (All Peers)");

    // Add all peers
    const auto& peer_list = plugin_->GetPeerList();
    for (const auto& peer : peer_list)
    {
        // Simplify hostname: replace "PublicServer_" with "_" to keep the prefix
        std::wstring display_name = peer.hostname;
        const std::wstring prefix = L"PublicServer_";
        if (display_name.find(prefix) == 0)
        {
            display_name = L"_" + display_name.substr(prefix.length());
        }

        // Format: hostname (IP)
        std::wstring item = display_name + L" (" + peer.virtual_ip + L")";
        peer_combo_.AddString(item.c_str());
    }

    peer_combo_.SetCurSel(0);
}

void CConfigDialog::OnOK()
{
    int sel = peer_combo_.GetCurSel();

    if (sel == 0)
    {
        // Auto rotate mode
        plugin_->SetAutoRotate(true);
        plugin_->SetSelectedPeerIndex(-1);
    }
    else if (sel > 0)
    {
        // Fixed peer mode
        int peer_index = sel - 1; // -1 because index 0 is "Auto Rotate"
        const auto& peer_list = plugin_->GetPeerList();

        if (peer_index >= 0 && peer_index < (int)peer_list.size())
        {
            plugin_->SetAutoRotate(false);
            plugin_->SetSelectedPeerIndex(peer_index);
            plugin_->SetSelectedPeerHostname(peer_list[peer_index].hostname);
        }
    }

    // Save configuration
    plugin_->SaveConfig();

    CDialogEx::OnOK();
}

void CConfigDialog::OnBnClickedRefreshPeerList()
{
    // Refresh peer list from plugin
    UpdatePeerCombo();
}
