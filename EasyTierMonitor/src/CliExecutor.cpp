#include "stdafx.h"
#include "CliExecutor.h"
#include <Windows.h>
#include <thread>
#include <sstream>
#include <vector>

namespace EasyTierMonitor {

CCliExecutor::CCliExecutor()
{
}

CCliExecutor::~CCliExecutor()
{
}

bool CCliExecutor::IsCliAvailable(const std::wstring& cli_path)
{
    // Try executing --version or --help command
    CliResult result = Execute(cli_path, L"--version", 2000);
    return result.success || (result.exit_code == 0);
}

CliResult CCliExecutor::Execute(const std::wstring& cli_path, const std::wstring& args, int timeout_ms)
{
    CliResult result;

    // Build command line
    std::wstring command_line = L"\"" + cli_path + L"\" " + args;

    // Execute process
    if (!ExecuteProcess(command_line, timeout_ms, result))
    {
        result.error_msg = L"Execution failed: " + last_error_;
        return result;
    }

    result.success = (result.exit_code == 0);
    return result;
}

bool CCliExecutor::ExecuteProcess(const std::wstring& command_line, int timeout_ms, CliResult& result)
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // Create pipe
    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
    {
        last_error_ = L"CreatePipe failed";
        return false;
    }

    // Set write handle not inheritable
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    // Startup info
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdOutput = hWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    ZeroMemory(&pi, sizeof(pi));

    // Create process
    std::vector<wchar_t> cmd_line(command_line.begin(), command_line.end());
    cmd_line.push_back(0); // null terminator

    if (!CreateProcess(
        NULL,
        cmd_line.data(),
        NULL,
        NULL,
        TRUE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi))
    {
        DWORD error = GetLastError();
        wchar_t error_buf[256];
        swprintf_s(error_buf, L"CreateProcess failed, error code: %lu", error);
        last_error_ = error_buf;

        CloseHandle(hRead);
        CloseHandle(hWrite);
        return false;
    }

    // Close write handle
    CloseHandle(hWrite);

    // Wait for process completion or timeout
    DWORD wait_result = WaitForSingleObject(pi.hProcess, timeout_ms);

    if (wait_result == WAIT_TIMEOUT)
    {
        // Timeout, terminate process
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, 1000); // Wait 1 second for process cleanup

        last_error_ = L"Command execution timeout";
        CloseHandle(hRead);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return false;
    }

    // Get exit code
    DWORD exit_code;
    if (!GetExitCodeProcess(pi.hProcess, &exit_code))
    {
        result.exit_code = 0;
    }
    else
    {
        result.exit_code = exit_code;
    }

    // Read output
    ReadProcessOutput(hRead, result.output);

    // Cleanup
    CloseHandle(hRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}

bool CCliExecutor::ReadProcessOutput(HANDLE hRead, std::wstring& output)
{
    output.clear();

    const int BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    DWORD bytes_read;
    std::string ansi_output;

    while (ReadFile(hRead, buffer, BUFFER_SIZE - 1, &bytes_read, NULL) && bytes_read > 0)
    {
        buffer[bytes_read] = '\0';
        ansi_output.append(buffer, bytes_read);
    }

    // Convert to wide character
    if (!ansi_output.empty())
    {
        // Calculate required buffer size
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, ansi_output.c_str(),
            static_cast<int>(ansi_output.size()), NULL, 0);

        if (size_needed > 0)
        {
            output.resize(size_needed);
            MultiByteToWideChar(CP_UTF8, 0, ansi_output.c_str(),
                static_cast<int>(ansi_output.size()), &output[0], size_needed);
        }
    }

    return true;
}

} // namespace EasyTierMonitor
