#ifndef _LC_WIN_HTTP_H
#define _LC_WIN_HTTP_H

#include <windows.h>
#include <Winhttp.h>

#pragma comment(lib, "Winhttp.lib")

static const unsigned int INT_RETRYTIMES = 3;
static wchar_t *SZ_AGENT = L"Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Safari/537.36";

typedef struct WIN_HTTP_CLIENT
{
	wchar_t m_proxy[MAX_PATH];
	wchar_t m_proxyUsername[50];
	wchar_t m_proxyPassword[50];

	wchar_t m_userAgent[50];
	DWORD m_dwLastError;
	BOOL m_requireValidSsl;

	unsigned int m_resolveTimeout;
	unsigned int m_connectTimeout;
	unsigned int m_sendTimeout;
	unsigned int m_receiveTimeout;

}WinHttp, *LPWinHttp;

WinHttp whpdata;

void WinHttpInit();
void SetProxy(LPCWSTR proxyhost, LPCWSTR user, LPCWSTR passwd);
void SetUserAgent(LPCWSTR szUserAgent);

int getLastError();
char* SendHttpRequest(LPCWSTR pstrMethod, LPCWSTR pstrURL, LPCSTR pszPostMsg, BOOL bProxy);


#endif