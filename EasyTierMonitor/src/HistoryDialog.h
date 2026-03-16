#pragma once

#include <afxdialogex.h>
#include "Common.h"
#include "../res/resource.h"

namespace EasyTierMonitor {

// History record dialog class
class CHistoryDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CHistoryDialog)

public:
    CHistoryDialog(const std::deque<StatusSnapshot>& history, const std::wstring& peer_name, CWnd* pParent = nullptr);
    virtual ~CHistoryDialog();

    enum { IDD = IDD_HISTORY_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

private:
    // Initialize controls
    void InitControls();
    void LoadHistoryData();
    void FormatHistoryEntry(const StatusSnapshot& snapshot, std::wstring& text);
    void ExportHistory();

    // Control variables
    CListBox history_list_;
    CEdit details_edit_;
    CButton export_button_;
    CButton refresh_button_;

    // Data
    std::vector<StatusSnapshot> history_;
    std::wstring peer_name_;

    // Statistics information
    int total_count_;
    int direct_count_;
    int relay_count_;
    int disconnected_count_;
    int unknown_count_;
    double avg_ping_ms_;

public:
    afx_msg void OnLbnSelChangeHistoryList();
    afx_msg void OnBnClickedExport();
    afx_msg void OnBnClickedRefresh();
};

} // namespace EasyTierMonitor
