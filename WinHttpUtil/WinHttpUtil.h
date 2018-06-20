#ifndef _LC_WIN_HTTP_H
#define _LC_WIN_HTTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

	static const unsigned int INT_RETRYTIMES = 3;
	static wchar_t SZ_AGENT[256] = L"Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/66.0.3359.181 Safari/537.36";

	void WinHttpUtilInit();
	void WinHttpUtilSetProxy(LPCWSTR szProxyHost, LPCWSTR szUsername, LPCWSTR szPassword);
	BOOL WinHttpUtilSetUserAgent(LPCWSTR szUserAgent);

	DWORD WinHttpUtilGetLastError();
	LPSTR WinHttpUtilSendRequest(LPCWSTR pstrMethod, LPCWSTR pstrURL, LPCSTR pszPostMsg, BOOL bProxy, LPWSTR szHeader, LPWSTR szCookies, LPCWSTR szAddHeader);


#ifdef __cplusplus
}
#endif

#endif