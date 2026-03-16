#include "stdafx.h"
#include "HistoryDialog.h"
#include "../res/resource.h"
#include <algorithm>
#include <fstream>
#include <iomanip>

namespace EasyTierMonitor {

IMPLEMENT_DYNAMIC(CHistoryDialog, CDialogEx)

CHistoryDialog::CHistoryDialog(const std::deque<StatusSnapshot>& history, const std::wstring& peer_name, CWnd* pParent)
    : CDialogEx(IDD, pParent)
    , history_(history.begin(), history.end())
    , peer_name_(peer_name)
    , total_count_(0)
    , direct_count_(0)
    , relay_count_(0)
    , disconnected_count_(0)
    , unknown_count_(0)
    , avg_ping_ms_(0.0)
{
}

CHistoryDialog::~CHistoryDialog()
{
}

void CHistoryDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_HISTORY_LIST, history_list_);
    DDX_Control(pDX, IDC_DETAILS_EDIT, details_edit_);
    DDX_Control(pDX, IDC_EXPORT_BUTTON, export_button_);
    DDX_Control(pDX, IDC_REFRESH_BUTTON, refresh_button_);
}

BEGIN_MESSAGE_MAP(CHistoryDialog, CDialogEx)
    ON_LBN_SELCHANGE(IDC_HISTORY_LIST, &CHistoryDialog::OnLbnSelChangeHistoryList)
    ON_BN_CLICKED(IDC_EXPORT_BUTTON, &CHistoryDialog::OnBnClickedExport)
    ON_BN_CLICKED(IDC_REFRESH_BUTTON, &CHistoryDialog::OnBnClickedRefresh)
END_MESSAGE_MAP()

BOOL CHistoryDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Set dialog title
    CString title;
    title.Format(L"EasyTier Monitor - History Records (%s)", peer_name_.c_str());
    SetWindowTextW(title);

    // Initialize controls
    InitControls();

    // Load history data
    LoadHistoryData();

    return TRUE;
}

void CHistoryDialog::InitControls()
{
    // Set list box style
    history_list_.ResetContent();

    // Set details edit box as read-only
    details_edit_.SetReadOnly(TRUE);
}

void CHistoryDialog::LoadHistoryData()
{
    history_list_.ResetContent();

    // Reset statistics
    total_count_ = 0;
    direct_count_ = 0;
    relay_count_ = 0;
    disconnected_count_ = 0;
    unknown_count_ = 0;
    avg_ping_ms_ = 0.0;
    double ping_sum = 0.0;
    int ping_count = 0;

    // Display in reverse order (newest on top)
    for (auto it = history_.rbegin(); it != history_.rend(); ++it)
    {
        std::wstring entry;
        FormatHistoryEntry(*it, entry);

        int index = history_list_.AddString(entry.c_str());
        if (index != LB_ERR)
        {
            // Set item data to index history records
            history_list_.SetItemData(index, static_cast<DWORD_PTR>(std::distance(history_.begin(), it.base()) - 1));
        }

        // Statistics
        total_count_++;
        switch (it->status)
        {
        case ConnStatus::Direct:
            direct_count_++;
            break;
        case ConnStatus::Relay:
            relay_count_++;
            break;
        case ConnStatus::Disconnected:
            disconnected_count_++;
            break;
        default:
            unknown_count_++;
            break;
        }

        if (it->ping_ms >= 0)
        {
            ping_sum += it->ping_ms;
            ping_count++;
        }
    }

    // Calculate average latency
    if (ping_count > 0)
    {
        avg_ping_ms_ = ping_sum / ping_count;
    }

    // Display statistics information
    CString stats;
    stats.Format(L"Total: %d entries | Direct: %d | Relay: %d | Disconnected: %d | Unknown: %d | Avg Latency: %.1f ms",
        total_count_, direct_count_, relay_count_, disconnected_count_, unknown_count_, avg_ping_ms_);
    SetDlgItemTextW(IDC_STATS_STATIC, stats);
}

void CHistoryDialog::FormatHistoryEntry(const StatusSnapshot& snapshot, std::wstring& text)
{
    // Format: [Time] Status Latency
    std::wstring time_str = FormatTime(snapshot.timestamp);
    const wchar_t* status_str = StatusToString(snapshot.status);
    const wchar_t* icon = StatusToIcon(snapshot.status);

    text = icon;
    text += L" ";
    text += time_str;
    text += L" - ";
    text += status_str;

    if (snapshot.ping_ms >= 0)
    {
        wchar_t buf[32];
        swprintf_s(buf, L" (%d ms)", snapshot.ping_ms);
        text += buf;
    }

    if (!snapshot.message.empty())
    {
        text += L": ";
        text += snapshot.message;
    }
}

void CHistoryDialog::ExportHistory()
{
    // Open file save dialog
    CFileDialog fileDlg(FALSE, L"csv", L"history",
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        L"CSV files (*.csv)|*.csv|Text files (*.txt)|*.txt|All files (*.*)|*.*||",
        this);

    if (fileDlg.DoModal() != IDOK)
        return;

    std::wstring file_path = fileDlg.GetPathName().GetString();

    // Open file
    std::ofstream file;
    file.open(file_path, std::ios::out | std::ios::trunc);
    if (!file.is_open())
    {
        AfxMessageBox(L"Unable to create file");
        return;
    }

    // Write UTF-8 BOM
    const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
    file.write(reinterpret_cast<const char*>(bom), 3);

    // Write CSV header
    file << "Time,Status,Latency(ms),Message\n";

    // Write data (in order, from old to new)
    for (const auto& snapshot : history_)
    {
        std::string time_str;
        std::string status_str;
        std::string message_str;

        // Convert to UTF-8
        int size = WideCharToMultiByte(CP_UTF8, 0, FormatTime(snapshot.timestamp).c_str(), -1, nullptr, 0, nullptr, nullptr);
        time_str.resize(size - 1);
        WideCharToMultiByte(CP_UTF8, 0, FormatTime(snapshot.timestamp).c_str(), -1, &time_str[0], size, nullptr, nullptr);

        size = WideCharToMultiByte(CP_UTF8, 0, StatusToString(snapshot.status), -1, nullptr, 0, nullptr, nullptr);
        status_str.resize(size - 1);
        WideCharToMultiByte(CP_UTF8, 0, StatusToString(snapshot.status), -1, &status_str[0], size, nullptr, nullptr);

        size = WideCharToMultiByte(CP_UTF8, 0, snapshot.message.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (size > 1)
        {
            message_str.resize(size - 1);
            WideCharToMultiByte(CP_UTF8, 0, snapshot.message.c_str(), -1, &message_str[0], size, nullptr, nullptr);
        }

        // Write row
        file << "\"" << time_str << "\","
             << "\"" << status_str << "\","
             << snapshot.ping_ms << ","
             << "\"" << message_str << "\"\n";
    }

    file.close();

    CString msg;
    msg.Format(L"Exported %d history records to:\n%s", total_count_, file_path.c_str());
    AfxMessageBox(msg);
}

void CHistoryDialog::OnLbnSelChangeHistoryList()
{
    int sel = history_list_.GetCurSel();
    if (sel == LB_ERR)
        return;

    DWORD_PTR index = history_list_.GetItemData(sel);
    if (index >= static_cast<DWORD_PTR>(history_.size()))
        return;

    const StatusSnapshot& snapshot = history_[index];

    // Format ping text
    CString ping_text;
    if (snapshot.ping_ms >= 0)
    {
        ping_text.Format(L"%d ms", snapshot.ping_ms);
    }
    else
    {
        ping_text = L"Timeout";
    }

    // Display detailed information
    CString details;
    details.Format(
        L"Time: %s\n"
        L"Status: %s %s\n"
        L"Ping Latency: %s\n"
        L"CLI Latency: %.2f ms\n"
        L"Message: %s",
        FormatTime(snapshot.timestamp).c_str(),
        StatusToIcon(snapshot.status),
        StatusToString(snapshot.status),
        ping_text.GetString(),
        snapshot.cli_lat_ms,
        snapshot.message.c_str()
    );

    details_edit_.SetWindowTextW(details);
}

void CHistoryDialog::OnBnClickedExport()
{
    ExportHistory();
}

void CHistoryDialog::OnBnClickedRefresh()
{
    // TODO: Reload history records from DataManager
    LoadHistoryData();
}

} // namespace EasyTierMonitor
