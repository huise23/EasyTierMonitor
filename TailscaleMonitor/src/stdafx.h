// stdafx.h : standard system include files
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Disable Winsock deprecated warnings
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Winsock2.h>
#include <Windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <ws2tcpip.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

#include <afxwin.h>
#include <afxext.h>
#include <afxdisp.h>

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#endif
