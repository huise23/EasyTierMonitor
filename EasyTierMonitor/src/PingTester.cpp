#include "stdafx.h"
#include "PingTester.h"
#include <Winsock2.h>
#include <Windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace EasyTierMonitor {

CPingTester::CPingTester()
    : icmp_handle_(NULL)
{
}

CPingTester::~CPingTester()
{
    Cleanup();
}

bool CPingTester::Initialize()
{
    if (icmp_handle_ != NULL)
        return true;

    // Initialize Winsock
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        return false;
    }

    // Create ICMP handle
    icmp_handle_ = IcmpCreateFile();
    if (icmp_handle_ == INVALID_HANDLE_VALUE)
    {
        WSACleanup();
        icmp_handle_ = NULL;
        return false;
    }

    return true;
}

void CPingTester::Cleanup()
{
    if (icmp_handle_ != NULL && icmp_handle_ != INVALID_HANDLE_VALUE)
    {
        IcmpCloseHandle(icmp_handle_);
        icmp_handle_ = NULL;
    }

    WSACleanup();
}

int CPingTester::Ping(const std::wstring& ipv4, int timeout_ms)
{
    if (!IsInitialized())
    {
        if (!Initialize())
            return -1;
    }

    // Convert IP address
    char ip_str[INET6_ADDRSTRLEN] = {0};
    WideCharToMultiByte(CP_UTF8, 0, ipv4.c_str(), -1,
        ip_str, sizeof(ip_str), NULL, NULL);

    // Convert string to IP address
    IPAddr ip_addr = inet_addr(ip_str);
    if (ip_addr == INADDR_NONE)
    {
        return -1;
    }

    // Prepare sending data
    const int DATA_SIZE = 32;
    unsigned char send_data[DATA_SIZE];
    for (int i = 0; i < DATA_SIZE; i++)
    {
        send_data[i] = static_cast<unsigned char>('A' + i);
    }

    // Allocate reply buffer
    DWORD reply_size = sizeof(ICMP_ECHO_REPLY) + DATA_SIZE + 8;
    unsigned char* reply_buffer = new unsigned char[reply_size];
    ZeroMemory(reply_buffer, reply_size);

    // Send ICMP Echo Request
    DWORD result = IcmpSendEcho(
        icmp_handle_,
        ip_addr,
        send_data,
        DATA_SIZE,
        NULL,
        reply_buffer,
        reply_size,
        timeout_ms
    );

    int rtt = -1;

    if (result != 0)
    {
        PICMP_ECHO_REPLY echo_reply = reinterpret_cast<PICMP_ECHO_REPLY>(reply_buffer);

        if (echo_reply->Status == IP_SUCCESS)
        {
            rtt = static_cast<int>(echo_reply->RoundTripTime);
        }
        else
        {
            // Other statuses indicate failure
            rtt = -1;
        }
    }

    delete[] reply_buffer;
    return rtt;
}

} // namespace EasyTierMonitor
