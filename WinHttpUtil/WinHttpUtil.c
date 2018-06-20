#include "WinHttpUtil.h"

#include <Winhttp.h>
#pragma comment(lib, "Winhttp.lib")

struct
{
	wchar_t m_proxy[MAX_PATH];
	wchar_t m_proxyUsername[128];
	wchar_t m_proxyPassword[128];

	DWORD m_dwLastError;
	BOOL m_requireValidSsl;

	unsigned int m_resolveTimeout;
	unsigned int m_connectTimeout;
	unsigned int m_sendTimeout;
	unsigned int m_receiveTimeout;

}whpdata;


/*
* @Description: 加载基本设置
*/
void WinHttpUtilInit()
{
	memset(whpdata.m_proxy, '\0', sizeof(whpdata.m_proxy));
	lstrcpyn(whpdata.m_proxyUsername, L"", sizeof(whpdata.m_proxyUsername));
	lstrcpyn(whpdata.m_proxyPassword, L"", sizeof(whpdata.m_proxyPassword));

	whpdata.m_requireValidSsl = FALSE; // In https, Accept any certificate while performing HTTPS request.
	whpdata.m_dwLastError = 0;
	whpdata.m_resolveTimeout = 0;
	whpdata.m_connectTimeout = 60000;
	whpdata.m_sendTimeout = 30000;
	whpdata.m_receiveTimeout = 30000;

}

/*
* @Description: 设置http请求的代理服务器
*	例如：“192.168.1.1:1080”
*/
void WinHttpUtilSetProxy(LPCWSTR szProxyHost, LPCWSTR szUsername, LPCWSTR szPassword)
{
	lstrcpyn(whpdata.m_proxy, szProxyHost, sizeof(whpdata.m_proxy));
	if (lstrlen(szUsername) == 0)
		lstrcpyn(whpdata.m_proxyUsername, L"", sizeof(whpdata.m_proxyUsername));
	else
		lstrcpyn(whpdata.m_proxyUsername, szUsername, sizeof(whpdata.m_proxyUsername));

	if (lstrlen(szPassword) == 0)
		lstrcpyn(whpdata.m_proxyPassword, L"", sizeof(whpdata.m_proxyPassword));
	else
		lstrcpyn(whpdata.m_proxyPassword, szPassword, sizeof(whpdata.m_proxyPassword));
}

/*
* @Description: 设置UserAgent
*/
BOOL WinHttpUtilSetUserAgent(LPCWSTR szUserAgent)
{
	return (BOOL)(lstrcpyn(SZ_AGENT, szUserAgent, 256) != NULL);
}


/*
* @Description: 返回最后一次错误的错误代码
*
*/
DWORD WinHttpUtilGetLastError()
{
	return whpdata.m_dwLastError;
}

/*
* @Description:发送和接收消息接口
*	return：接收到的回复数据
*	pstrMethod：http请求形式，POST和GET
*	pstrURL：请求路径
*	pszPostMsg：要发送的数据
*	bProxy：是否使用代理，1-使用，0-不使用
*   szHeader: 返回头
*   szCookies: 返回的cookies
*   szAddHeader: 要添加的请求头
*/
LPSTR WinHttpUtilSendRequest(LPCWSTR pstrMethod, LPCWSTR pstrURL, LPCSTR pszPostMsg, BOOL bProxy, LPWSTR szHeader, LPWSTR szCookies, LPCWSTR szAddHeader)
{
	DWORD nPostMsgLen = NULL;
	if (pszPostMsg != NULL) nPostMsgLen = lstrlenA(pszPostMsg);


	wchar_t szHostName[MAX_PATH] = L"";
	wchar_t szURLPath[MAX_PATH * 4] = L"";


	BOOL bGetReponseSucceed = FALSE;
	DWORD dwRetryTimes = 0;

	wchar_t szWithHeader[10240] = L"";
	WINHTTP_PROXY_INFO proxyInfo;
	wchar_t szProxy[MAX_PATH] = L"";

	BOOL bSendRequestSucceed = FALSE;
	WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig;
	WINHTTP_AUTOPROXY_OPTIONS autoProxyOptions;

	DWORD dwWritten = 0;
	DWORD dwSize = 0;
	BOOL bResult = FALSE;

	DWORD dwBufSize = 0;
	CHAR *pResponse = NULL;
	LPWSTR szHeaderBuf = NULL;
	LPWSTR szCookiesBuf = NULL;
	DWORD dwRead = 0;
	char *pszOut = (char*)malloc(1);


	//url检查
	if (lstrlen(pstrURL) <= 0)
	{
		whpdata.m_dwLastError = ERROR_PATH_NOT_FOUND;
		return _strdup("ERROR_PATH_NOT_FOUND");
	}


	HINTERNET hSession = WinHttpOpen(SZ_AGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (hSession == NULL)
	{
		whpdata.m_dwLastError = GetLastError();

		return _strdup("hSession == NULL");
	}

	WinHttpSetTimeouts(hSession, whpdata.m_resolveTimeout, whpdata.m_connectTimeout, whpdata.m_sendTimeout, whpdata.m_receiveTimeout);

	URL_COMPONENTS urlComp;
	memset(&urlComp, 0, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(urlComp);
	urlComp.lpszHostName = szHostName;
	urlComp.dwHostNameLength = MAX_PATH;
	urlComp.lpszUrlPath = szURLPath;
	urlComp.dwUrlPathLength = MAX_PATH * 5;
	urlComp.dwSchemeLength = 1; // None zero

	if (WinHttpCrackUrl(pstrURL, lstrlen(pstrURL), 0, &urlComp))
	{
		HINTERNET hConnect = WinHttpConnect(hSession, szHostName, urlComp.nPort, 0);
		if (hConnect != NULL)
		{
			DWORD dwOpenRequestFlag = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;

			HINTERNET hRequest = WinHttpOpenRequest(hConnect, pstrMethod, urlComp.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, dwOpenRequestFlag);

			if (hRequest != NULL)
			{
				// If HTTPS, then client is very susceptable to invalid certificates
				// Easiest to accept anything for now
				if (!whpdata.m_requireValidSsl && urlComp.nScheme == INTERNET_SCHEME_HTTPS)
				{
					DWORD dwOptions = SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_UNKNOWN_CA;
					WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, (LPVOID)&dwOptions, sizeof(DWORD));

				}

				DWORD dwOptions = WINHTTP_DISABLE_REDIRECTS; //禁止重定向
				WinHttpSetOption(hRequest, WINHTTP_OPTION_DISABLE_FEATURE, (LPVOID)&dwOptions, sizeof(DWORD));
				//发送请求失败时，多尝试几次
				while (!bGetReponseSucceed && dwRetryTimes++ < INT_RETRYTIMES)
				{
					szWithHeader[0] = 0;
					// memcpy(hdsize,&nPostMsgLen,sizeof(int));

					if (nPostMsgLen > 0) wsprintf(szWithHeader, L"Content-Length: %d\r\n", nPostMsgLen);

					lstrcat(szWithHeader, L"Content-Type: application/x-www-form-urlencoded\r\n");

					if (szAddHeader != NULL) lstrcat(szWithHeader, szAddHeader);

					lstrcat(szWithHeader, L"\r\n");

					if (!WinHttpAddRequestHeaders(hRequest, szWithHeader, lstrlen(szWithHeader), WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON))
					{
						whpdata.m_dwLastError = GetLastError();
					}

					if (bProxy)
					{
						memset(&proxyInfo, 0, sizeof(proxyInfo));
						proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
						lstrcpyn(szProxy, whpdata.m_proxy, sizeof(szProxy));
						proxyInfo.lpszProxy = szProxy;
						if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo)))
						{
							whpdata.m_dwLastError = GetLastError();
						}
						if (lstrlen(whpdata.m_proxyUsername) > 0)
						{
							if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY_USERNAME, (LPVOID)whpdata.m_proxyUsername, lstrlen(whpdata.m_proxyUsername) * sizeof(wchar_t)))
							{
								whpdata.m_dwLastError = GetLastError();
							}
							if (lstrlen(whpdata.m_proxyPassword) > 0)
							{
								if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY_PASSWORD, (LPVOID)whpdata.m_proxyPassword, lstrlen(whpdata.m_proxyUsername) * sizeof(wchar_t)))
								{
									whpdata.m_dwLastError = GetLastError();
								}
							}
						}

					}

					if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, NULL))
					{
						bSendRequestSucceed = TRUE;
					}
					else
					{
						// Query the proxy information from IE setting and set the proxy if any.
						memset(&proxyConfig, 0, sizeof(proxyConfig));
						if (WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig))
						{
							if (proxyConfig.lpszAutoConfigUrl != NULL)
							{
								memset(&autoProxyOptions, 0, sizeof(autoProxyOptions));
								autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT | WINHTTP_AUTOPROXY_CONFIG_URL;
								autoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP;
								autoProxyOptions.lpszAutoConfigUrl = proxyConfig.lpszAutoConfigUrl;
								autoProxyOptions.fAutoLogonIfChallenged = TRUE;
								autoProxyOptions.dwReserved = 0;
								autoProxyOptions.lpvReserved = NULL;

								memset(&proxyInfo, 0, sizeof(proxyInfo));

								if (WinHttpGetProxyForUrl(hSession, pstrURL, &autoProxyOptions, &proxyInfo))
								{
									if (WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo)))
									{
										if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, NULL))
										{
											bSendRequestSucceed = TRUE;
										}
									}
									if (proxyInfo.lpszProxy != NULL)
									{
										GlobalFree(proxyInfo.lpszProxy);
									}
									if (proxyInfo.lpszProxyBypass != NULL)
									{
										GlobalFree(proxyInfo.lpszProxyBypass);
									}
								}
								else
								{
									whpdata.m_dwLastError = GetLastError();
								}
							}
							else if (proxyConfig.lpszProxy != NULL)
							{
								memset(&proxyInfo, 0, sizeof(proxyInfo));
								proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
								memset(szProxy, 0, sizeof(szProxy));
								lstrcpyn(szProxy, proxyConfig.lpszProxy, sizeof(szProxy));
								proxyInfo.lpszProxy = szProxy;

								if (proxyConfig.lpszProxyBypass != NULL)
								{
									wchar_t szProxyBypass[MAX_PATH] = L"";
									lstrcpyn(szProxyBypass, proxyConfig.lpszProxyBypass, sizeof(szProxyBypass));
									proxyInfo.lpszProxyBypass = szProxyBypass;
								}

								if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo)))
								{
									whpdata.m_dwLastError = GetLastError();
								}
							}
							if (proxyConfig.lpszAutoConfigUrl != NULL)
							{
								GlobalFree(proxyConfig.lpszAutoConfigUrl);
							}
							if (proxyConfig.lpszProxy != NULL)
							{
								GlobalFree(proxyConfig.lpszProxy);
							}
							if (proxyConfig.lpszProxyBypass != NULL)
							{
								GlobalFree(proxyConfig.lpszProxyBypass);
							}
						}
						else
						{
							whpdata.m_dwLastError = GetLastError();
						}
					}
					if (bSendRequestSucceed)
					{
						if (nPostMsgLen > 0)
						{
							if (!WinHttpWriteData(hRequest, pszPostMsg, nPostMsgLen, &dwWritten))
							{
								whpdata.m_dwLastError = GetLastError();
							}
						}
						if (WinHttpReceiveResponse(hRequest, NULL))
						{
							dwSize = 0;
							bResult = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwSize, WINHTTP_NO_HEADER_INDEX);

							if (bResult || (!bResult && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)))
							{
								szHeaderBuf = (wchar_t*)malloc(sizeof(wchar_t)*dwSize);
								ZeroMemory(szHeaderBuf, dwSize * sizeof(wchar_t));

								WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, szHeaderBuf, &dwSize, WINHTTP_NO_HEADER_INDEX);
								if (szHeader != NULL) lstrcpyn(szHeader, szHeaderBuf, dwSize);
							}

							if (szCookies != NULL)
							{
								lstrcpy(szCookies, L"Cookie:");
								LPCWSTR SETCOOKIES = L"Set-Cookie:";
								DWORD dwSlen = lstrlen(SETCOOKIES);
								LPWSTR lpFindHead = wcsstr(szHeaderBuf, SETCOOKIES);
								while (lpFindHead != NULL)
								{
									LPWSTR lpFindEnd = wcsstr(lpFindHead + dwSlen, L";");
									lpFindEnd[0] = 0; // 切断
									if (lpFindEnd != NULL)
									{
										lstrcat(szCookies, lpFindHead + dwSlen);
										lstrcat(szCookies, L";");
									}
									else {
										break;
									}
									lpFindHead = wcsstr(lpFindEnd + 1, SETCOOKIES);
								}
							}


							pszOut[0] = 0;
							do
							{
								dwSize = 0;
								if (WinHttpQueryDataAvailable(hRequest, &dwSize))
								{
									pResponse = (CHAR *)malloc(sizeof(CHAR)*(dwSize + 1));
									memset(pResponse, 0, sizeof(CHAR)*(dwSize + 1));
									dwRead = 0;

									if (WinHttpReadData(hRequest, pResponse, dwSize, &dwRead))
									{
										dwBufSize += dwRead;
										char *pszTmp = pszOut;
										pszOut = (char*)malloc(dwBufSize + 2);
										ZeroMemory(pszOut, dwBufSize + 2);
										lstrcpyA(pszOut, pszTmp);
										lstrcatA(pszOut, pResponse);

										free(pszTmp);
										pszTmp = NULL;

									}
									free(pResponse);
									pResponse = NULL;
								}
							} while (dwSize > 0);

							bGetReponseSucceed = TRUE;

							free(szHeaderBuf);
							szHeaderBuf = NULL;
							free(szCookiesBuf);
							szCookiesBuf = NULL;
						}

					}

				}

				WinHttpCloseHandle(hRequest);
			}
			WinHttpCloseHandle(hConnect);
		}
	}
	WinHttpCloseHandle(hSession);

	return pszOut;
}

