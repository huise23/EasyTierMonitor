#pragma once

#include "Common.h"
#include <vector>
#include <string>

namespace EasyTierMonitor {

// CLI output parser
class CCliParser
{
public:
    // Parse easytier-cli peer output
    static std::vector<PeerInfo> ParsePeerList(const std::wstring& cli_output);

    // Parse single peer row
    static bool ParsePeerLine(const std::wstring& line, PeerInfo& peer_info);

private:
    // Split string
    static std::vector<std::wstring> SplitLine(const std::wstring& line, wchar_t delimiter);

    // Remove leading and trailing spaces
    static std::wstring Trim(const std::wstring& str);

    // Parse byte size (e.g., "17.33 kB")
    static uint64_t ParseByteSize(const std::wstring& size_str);

    // Parse floating point number
    static double ParseDouble(const std::wstring& num_str);

    // Check if header or separator line
    static bool IsHeaderOrSeparator(const std::wstring& line);

    // Parse table row (handle misaligned columns)
    static std::vector<std::wstring> ParseTableRow(const std::wstring& line, const std::vector<int>& column_widths);
};

} // namespace EasyTierMonitor
