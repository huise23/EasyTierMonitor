#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <ctime>

// Simple debug logger for troubleshooting
class CDebugLog
{
public:
    static void Log(const wchar_t* function, int line, const wchar_t* format, ...)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Get current time
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);

        wchar_t time_buffer[64];
        wcsftime(time_buffer, 64, L"%Y-%m-%d %H:%M:%S", &timeinfo);

        // Format log message
        va_list args;
        va_start(args, format);

        wchar_t buffer[2048];
        vswprintf_s(buffer, 2048, format, args);

        va_end(args);

        // Write to file
        if (!log_file_.is_open())
        {
            log_file_.open(L"easytier_monitor_debug.log", std::ios::app);
        }

        if (log_file_.is_open())
        {
            log_file_ << L"[" << time_buffer << L"] " << function << L":" << line << L" - " << buffer << std::endl;
            log_file_.flush();
        }

        // Also output to debugger
        wchar_t debug_buffer[2048];
        swprintf_s(debug_buffer, 2048, L"[EasyTierMonitor] %s:%d - %s\n", function, line, buffer);
        OutputDebugStringW(debug_buffer);
    }

    static void Close()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (log_file_.is_open())
        {
            log_file_.close();
        }
    }

private:
    static std::mutex mutex_;
    static std::wofstream log_file_;
};

// Convenience macros
#define DEBUG_LOG(fmt, ...) CDebugLog::Log(__FUNCTIONW__, __LINE__, fmt, ##__VA_ARGS__)
#define DEBUG_LOG_ERROR(fmt, ...) CDebugLog::Log(__FUNCTIONW__, __LINE__, L"[ERROR] " fmt, ##__VA_ARGS__)
#define DEBUG_LOG_INFO(fmt, ...) CDebugLog::Log(__FUNCTIONW__, __LINE__, L"[INFO] " fmt, ##__VA_ARGS__)
#define DEBUG_LOG_WARNING(fmt, ...) CDebugLog::Log(__FUNCTIONW__, __LINE__, L"[WARNING] " fmt, ##__VA_ARGS__)
