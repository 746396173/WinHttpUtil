#ifndef _LC_WIN_HTTP_H
#define _LC_WIN_HTTP_H

#include <windows.h>

static const unsigned int INT_RETRYTIMES = 3;
static wchar_t *SZ_AGENT = L"Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Safari/537.36";

void WinHttpUtilInit();
void WinHttpUtilSetProxy(LPCWSTR szProxyHost, LPCWSTR szUsername, LPCWSTR szPassword);
BOOL WinHttpUtilSetUserAgent(LPCWSTR szUserAgent);

DWORD WinHttpUtilGetLastError();
LPSTR WinHttpUtilSendRequest(LPCWSTR pstrMethod, LPCWSTR pstrURL, LPCSTR pszPostMsg, BOOL bProxy);


#endif