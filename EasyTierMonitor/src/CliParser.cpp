#include "stdafx.h"
#include "CliParser.h"
#include <sstream>
#include <algorithm>
#include <cwctype>

namespace EasyTierMonitor {

std::vector<PeerInfo> CCliParser::ParsePeerList(const std::wstring& cli_output)
{
    std::vector<PeerInfo> peer_list;

    if (cli_output.empty())
        return peer_list;

    std::wstringstream ss(cli_output);
    std::wstring line;
    bool in_table = false;

    // First pass: find header and determine column widths
    std::vector<int> column_widths;

    while (std::getline(ss, line))
    {
        // Skip empty lines
        if (line.empty())
            continue;

        // Detect header
        if (line.find(L"ipv4") != std::wstring::npos &&
            line.find(L"hostname") != std::wstring::npos &&
            line.find(L"cost") != std::wstring::npos)
        {
            // Parse column widths
            size_t pos = 0;
            while (pos < line.length())
            {
                size_t next_delim = line.find(L'|', pos);
                if (next_delim == std::wstring::npos)
                    break;

                int width = static_cast<int>(next_delim - pos);
                column_widths.push_back(width);
                pos = next_delim + 1;
            }

            in_table = true;
            continue;
        }

        // Skip separator line
        if (line.find(L"---") != std::wstring::npos || line.find(L"===") != std::wstring::npos)
        {
            continue;
        }

        // Parse data row
        if (in_table && line.find(L'|') != std::wstring::npos)
        {
            PeerInfo peer;
            if (ParsePeerLine(line, peer))
            {
                peer_list.push_back(peer);
            }
        }
    }

    return peer_list;
}

bool CCliParser::ParsePeerLine(const std::wstring& line, PeerInfo& peer_info)
{
    peer_info = PeerInfo(); // Reset to default value
    // Split by |
    std::vector<std::wstring> columns;
    size_t start = 0;
    size_t end = 0;

    while ((end = line.find(L'|', start)) != std::wstring::npos)
    {
        std::wstring column = line.substr(start, end - start);
        columns.push_back(Trim(column));
        start = end + 1;
    }

    // Need at least basic columns
    if (columns.size() < 3)
        return false;

    try
    {
        // Parse data based on column index (refer to easytier-cli peer output format)
        // | ipv4 | hostname | cost | lat_ms | loss_rate | rx_bytes | tx_bytes | tunnel_proto | nat_type | id |

        int col_index = 0;

        // IPv4
        if (col_index < columns.size())
        {
            peer_info.ipv4 = Trim(columns[col_index]);
            if (peer_info.ipv4.empty() || peer_info.ipv4 == L"*")
                peer_info.ipv4.clear();
        }
        col_index++;

        // Hostname
        if (col_index < columns.size())
        {
            peer_info.hostname = Trim(columns[col_index]);
        }
        col_index++;

        // Cost
        if (col_index < columns.size())
        {
            peer_info.cost = Trim(columns[col_index]);
        }
        col_index++;

        // Latency (ms)
        if (col_index < columns.size())
        {
            std::wstring lat_str = Trim(columns[col_index]);
            if (!lat_str.empty() && lat_str != L"*")
            {
                peer_info.lat_ms = ParseDouble(lat_str);
            }
        }
        col_index++;

        // Loss Rate
        if (col_index < columns.size())
        {
            std::wstring loss_str = Trim(columns[col_index]);
            if (!loss_str.empty() && loss_str != L"*")
            {
                peer_info.loss_rate = ParseDouble(loss_str);
            }
        }
        col_index++;

        // RX Bytes
        if (col_index < columns.size())
        {
            std::wstring rx_str = Trim(columns[col_index]);
            if (!rx_str.empty() && rx_str != L"*")
            {
                peer_info.rx_bytes = ParseByteSize(rx_str);
            }
        }
        col_index++;

        // TX Bytes
        if (col_index < columns.size())
        {
            std::wstring tx_str = Trim(columns[col_index]);
            if (!tx_str.empty() && tx_str != L"*")
            {
                peer_info.tx_bytes = ParseByteSize(tx_str);
            }
        }
        col_index++;

        // Tunnel Protocol
        if (col_index < columns.size())
        {
            peer_info.tunnel_proto = Trim(columns[col_index]);
        }
        col_index++;

        // NAT Type
        if (col_index < columns.size())
        {
            peer_info.nat_type = Trim(columns[col_index]);
        }
        col_index++;

        // Peer ID
        if (col_index < columns.size())
        {
            peer_info.id = Trim(columns[col_index]);
        }

        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::vector<std::wstring> CCliParser::SplitLine(const std::wstring& line, wchar_t delimiter)
{
    std::vector<std::wstring> tokens;
    std::wstringstream ss(line);
    std::wstring token;

    while (std::getline(ss, token, delimiter))
    {
        tokens.push_back(Trim(token));
    }

    return tokens;
}

std::wstring CCliParser::Trim(const std::wstring& str)
{
    if (str.empty())
        return str;

    size_t start = 0;
    while (start < str.length() && std::iswspace(str[start]))
        start++;

    if (start == str.length())
        return L"";

    size_t end = str.length() - 1;
    while (end > start && std::iswspace(str[end]))
        end--;

    return str.substr(start, end - start + 1);
}

uint64_t CCliParser::ParseByteSize(const std::wstring& size_str)
{
    if (size_str.empty())
        return 0;

    std::wstring str = Trim(size_str);

    // Extract number and unit
    double value = 0.0;
    wchar_t unit[16] = {0};

    if (swscanf_s(str.c_str(), L"%lf%15s", &value, unit, (unsigned)_countof(unit)) == 2)
    {
        // Convert based on unit
        std::wstring unit_str(unit);
        std::transform(unit_str.begin(), unit_str.end(), unit_str.begin(), ::towlower);

        if (unit_str == L"kb" || unit_str == L"k")
        {
            return static_cast<uint64_t>(value * 1024);
        }
        else if (unit_str == L"mb" || unit_str == L"m")
        {
            return static_cast<uint64_t>(value * 1024 * 1024);
        }
        else if (unit_str == L"gb" || unit_str == L"g")
        {
            return static_cast<uint64_t>(value * 1024 * 1024 * 1024);
        }
        else if (unit_str == L"tb" || unit_str == L"t")
        {
            return static_cast<uint64_t>(value * 1024ULL * 1024ULL * 1024ULL * 1024ULL);
        }
        else if (unit_str == L"b" || unit_str.empty())
        {
            return static_cast<uint64_t>(value);
        }
    }

    // Try parsing as integer directly
    try
    {
        return std::stoull(str);
    }
    catch (...)
    {
        return 0;
    }
}

double CCliParser::ParseDouble(const std::wstring& num_str)
{
    if (num_str.empty())
        return 0.0;

    std::wstring str = Trim(num_str);
    std::replace(str.begin(), str.end(), L',', L'.');

    try
    {
        return std::stod(str);
    }
    catch (...)
    {
        return 0.0;
    }
}

bool CCliParser::IsHeaderOrSeparator(const std::wstring& line)
{
    if (line.empty())
        return false;

    // Header contains key fields
    if (line.find(L"ipv4") != std::wstring::npos ||
        line.find(L"hostname") != std::wstring::npos ||
        line.find(L"cost") != std::wstring::npos)
    {
        return true;
    }

    // Separator line contains --- or ===
    if (line.find(L"---") != std::wstring::npos ||
        line.find(L"===") != std::wstring::npos)
    {
        return true;
    }

    return false;
}

std::vector<std::wstring> CCliParser::ParseTableRow(const std::wstring& line, const std::vector<int>& column_widths)
{
    std::vector<std::wstring> columns;

    if (column_widths.empty())
    {
        return SplitLine(line, L'|');
    }

    size_t pos = 0;
    for (int width : column_widths)
    {
        if (pos + width > line.length())
            break;

        std::wstring column = line.substr(pos, width);
        columns.push_back(Trim(column));
        pos += width + 1; // +1 for '|'
    }

    return columns;
}

} // namespace EasyTierMonitor
