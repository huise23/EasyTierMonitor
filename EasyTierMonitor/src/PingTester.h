#pragma once

#include <string>

namespace EasyTierMonitor {

// Ping tester
class CPingTester
{
public:
    CPingTester();
    ~CPingTester();

    // Initialize (create ICMP handle)
    bool Initialize();

    // Cleanup (close ICMP handle)
    void Cleanup();

    // Ping specified IP
    // Returns RTT (ms), -1 on failure or timeout
    int Ping(const std::wstring& ipv4, int timeout_ms = 500);

    // Check if initialized
    bool IsInitialized() const { return icmp_handle_ != NULL; }

private:
    HANDLE icmp_handle_;

    // Disable copying
    CPingTester(const CPingTester&) = delete;
    CPingTester& operator=(const CPingTester&) = delete;
};

} // namespace EasyTierMonitor
