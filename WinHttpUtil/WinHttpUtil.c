#include <stdio.h>
#include "WinHttpUtil.h"

void WinHttpInit()
{
	memset(whpdata.m_proxy, '\0', sizeof(whpdata.m_proxy));
	wcscpy(whpdata.m_userAgent, SZ_AGENT);
	wcscpy(whpdata.m_proxyUsername, L"");
	wcscpy(whpdata.m_proxyPassword, L"");

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
void SetProxy(LPCWSTR proxyhost, LPCWSTR user, LPCWSTR passwd)
{
	wcscpy(whpdata.m_proxy, proxyhost);
	if (lstrlen(user) == 0)
		wcscpy(whpdata.m_proxyUsername, L"");
	else
		wcscpy(whpdata.m_proxyUsername, user);

	if (lstrlen(passwd) == 0)
		wcscpy(whpdata.m_proxyPassword, L"");
	else
		wcscpy(whpdata.m_proxyPassword, passwd);
}
/*
* @Description: 设置UserAgent
*/
void SetUserAgent(LPCWSTR szUserAgent)
{
	SZ_AGENT = wcsdup(szUserAgent);
}


/*
* @Description: 返回最后一次错误的错误代码
*
*/
int getLastError()
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
*/
char* SendHttpRequest(LPCWSTR pstrMethod, LPCWSTR pstrURL, LPCSTR pszPostMsg, BOOL bProxy)
{
	DWORD nPostMsgLen = lstrlenA(pszPostMsg);
	DWORD dwOpenRequestFlag;
	DWORD options;

	HINTERNET m_sessionHandle = NULL;
	HINTERNET hConnect = NULL;
	HINTERNET hRequest = NULL;

	wchar_t szHostName[MAX_PATH] = L"";
	wchar_t szURLPath[MAX_PATH * 4] = L"";
	URL_COMPONENTS urlComp;

	BOOL bGetReponseSucceed = FALSE;
	DWORD dwRetryTimes = 0;

	wchar_t m_additionalRequestHeaders[10240] = L"";
	WINHTTP_PROXY_INFO proxyInfo;
	wchar_t szProxy[MAX_PATH] = L"";

	BOOL bSendRequestSucceed = FALSE;
	WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig;
	WINHTTP_AUTOPROXY_OPTIONS autoProxyOptions;

	DWORD dwWritten = 0;
	DWORD dwSize = 0;
	BOOL bResult = FALSE;

	DWORD dwBufSize = 0;
	BYTE *pResponse = NULL;
	wchar_t *szStatusCode = NULL;
	DWORD dwRead = 0;
	char *pszOut = (char*)malloc(1);


	//url检查
	if (lstrlen(pstrURL) <= 0)
	{
		whpdata.m_dwLastError = ERROR_PATH_NOT_FOUND;
		return strdup("ERROR_PATH_NOT_FOUND");
	}


	m_sessionHandle = WinHttpOpen(whpdata.m_userAgent,
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS,
		0);
	if (m_sessionHandle == NULL)
	{
		whpdata.m_dwLastError = GetLastError();

		return strdup("m_sessionHandle == NULL");
	}

	WinHttpSetTimeouts(m_sessionHandle, whpdata.m_resolveTimeout, whpdata.m_connectTimeout, whpdata.m_sendTimeout, whpdata.m_receiveTimeout);

	memset(&urlComp, 0, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(urlComp);
	urlComp.lpszHostName = szHostName;
	urlComp.dwHostNameLength = MAX_PATH;
	urlComp.lpszUrlPath = szURLPath;
	urlComp.dwUrlPathLength = MAX_PATH * 5;
	urlComp.dwSchemeLength = 1; // None zero

	if (WinHttpCrackUrl(pstrURL, lstrlen(pstrURL), 0, &urlComp))
	{
		hConnect = WinHttpConnect(m_sessionHandle, szHostName, urlComp.nPort, 0);
		if (hConnect != NULL)
		{
			dwOpenRequestFlag = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
			hRequest = WinHttpOpenRequest(hConnect,
				pstrMethod,
				urlComp.lpszUrlPath,
				NULL,
				WINHTTP_NO_REFERER,
				WINHTTP_DEFAULT_ACCEPT_TYPES,
				dwOpenRequestFlag);

			if (hRequest != NULL)
			{
				// If HTTPS, then client is very susceptable to invalid certificates
				// Easiest to accept anything for now
				if (!whpdata.m_requireValidSsl && urlComp.nScheme == INTERNET_SCHEME_HTTPS)
				{
					options = SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_UNKNOWN_CA;

					WinHttpSetOption(hRequest,
						WINHTTP_OPTION_SECURITY_FLAGS,
						(LPVOID)&options,
						sizeof(DWORD));
				}

				//发送请求失败时，多尝试几次
				while (!bGetReponseSucceed && dwRetryTimes++ < INT_RETRYTIMES)
				{
					m_additionalRequestHeaders[0] = 0;
					// memcpy(hdsize,&nPostMsgLen,sizeof(int));

					if (nPostMsgLen > 0) wsprintf(m_additionalRequestHeaders, L"%sContent-Length: %d\r\n", m_additionalRequestHeaders, nPostMsgLen);

					lstrcat(m_additionalRequestHeaders, L"Content-Type: application/x-www-form-urlencoded\r\n");

					if (!WinHttpAddRequestHeaders(hRequest, m_additionalRequestHeaders, lstrlen(m_additionalRequestHeaders), WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON))
					{
						whpdata.m_dwLastError = GetLastError();
					}

					if (bProxy)
					{
						memset(&proxyInfo, 0, sizeof(proxyInfo));
						proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
						wcscpy_s(szProxy, MAX_PATH, whpdata.m_proxy);
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

					if (WinHttpSendRequest(hRequest,
						WINHTTP_NO_ADDITIONAL_HEADERS,
						0,
						WINHTTP_NO_REQUEST_DATA,
						0,
						0,
						NULL))
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

								if (WinHttpGetProxyForUrl(m_sessionHandle, pstrURL, &autoProxyOptions, &proxyInfo))
								{
									if (WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo)))
									{
										if (WinHttpSendRequest(hRequest,
											WINHTTP_NO_ADDITIONAL_HEADERS,
											0,
											WINHTTP_NO_REQUEST_DATA,
											0,
											0,
											NULL))
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
							if (!WinHttpWriteData(hRequest,
								pszPostMsg,
								nPostMsgLen,
								&dwWritten))
							{
								whpdata.m_dwLastError = GetLastError();
							}
						}
						if (WinHttpReceiveResponse(hRequest, NULL))
						{
							dwSize = 0;
							bResult = WinHttpQueryHeaders(hRequest,
								WINHTTP_QUERY_STATUS_CODE,
								WINHTTP_HEADER_NAME_BY_INDEX,
								NULL,
								&dwSize,
								WINHTTP_NO_HEADER_INDEX);

							if (bResult || (!bResult && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)))
							{
								szStatusCode = (wchar_t*)malloc(sizeof(wchar_t)*dwSize);
								memset(szStatusCode, 0, sizeof(wchar_t)*dwSize);

								memset(szStatusCode, 0, dwSize * sizeof(wchar_t));

								WinHttpQueryHeaders(hRequest,
									WINHTTP_QUERY_STATUS_CODE,
									WINHTTP_HEADER_NAME_BY_INDEX,
									szStatusCode,
									&dwSize,
									WINHTTP_NO_HEADER_INDEX);


							}

							pszOut[0] = 0;
							do
							{
								dwSize = 0;
								if (WinHttpQueryDataAvailable(hRequest, &dwSize))
								{
									pResponse = (BYTE *)malloc(sizeof(BYTE)*(dwSize + 1));
									memset(pResponse, 0, sizeof(BYTE)*(dwSize + 1));
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
										//lstrcpy(output + iCurrentBufferSize, pResponse);
										//lstrcpyn(output + iCurrentBufferSize, pResponse, dwRead);
										//memcpy(output + iCurrentBufferSize, pResponse, dwRead);

									}
									free(pResponse);
									pResponse = NULL;
								}
							} while (dwSize > 0);

							bGetReponseSucceed = TRUE;

							free(szStatusCode);
							szStatusCode = NULL;
						}

					}

				}

				WinHttpCloseHandle(hRequest);
			}
			WinHttpCloseHandle(hConnect);
		}
	}
	WinHttpCloseHandle(m_sessionHandle);

	return pszOut;
}

