#include "stdafx.h"
#include "DebugLog.h"
#include <Windows.h>

std::mutex CDebugLog::mutex_;
std::wofstream CDebugLog::log_file_;
bool CDebugLog::enabled_ = true;
