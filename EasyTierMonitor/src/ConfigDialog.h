#pragma once

#include <afxdialogex.h>
#include "Plugin.h"
#include "../res/resource.h"

// Simple configuration dialog
class CConfigDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CConfigDialog)

public:
    CConfigDialog(CEasyTierPlugin* plugin, CWnd* pParent = nullptr);
    virtual ~CConfigDialog();

    enum { IDD = IDD_CONFIG_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    DECLARE_MESSAGE_MAP()

private:
    void UpdatePeerCombo();

    // Control variables
    CComboBox peer_combo_;
    CButton auto_rotate_check_;

    // Plugin reference
    CEasyTierPlugin* plugin_;

public:
    afx_msg void OnBnClickedRefreshPeerList();
};
