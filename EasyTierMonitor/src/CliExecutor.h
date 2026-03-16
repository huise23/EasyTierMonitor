#pragma once

#include <string>

namespace EasyTierMonitor {

// CLI execution result
struct CliResult
{
    bool success;           // Whether successful
    std::wstring output;    // Standard output
    DWORD exit_code;        // Exit code
    std::wstring error_msg; // Error message

    CliResult()
        : success(false)
        , exit_code(0)
    {}
};

// CLI executor
class CCliExecutor
{
public:
    CCliExecutor();
    ~CCliExecutor();

    // Execute CLI command
    // cli_path: easytier-cli executable file path
    // args: Command arguments (e.g., L"peer")
    // timeout_ms: Timeout (milliseconds)
    CliResult Execute(const std::wstring& cli_path, const std::wstring& args, int timeout_ms = 5000);

    // Check if CLI is available
    bool IsCliAvailable(const std::wstring& cli_path);

private:
    // Internal implementation of command execution
    bool ExecuteProcess(const std::wstring& command_line, int timeout_ms, CliResult& result);

    // Read process output
    bool ReadProcessOutput(HANDLE hRead, std::wstring& output);

private:
    std::wstring last_error_;
};

} // namespace EasyTierMonitor
