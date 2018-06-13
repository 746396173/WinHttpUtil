#include <locale.h>
#include "WinHttpUtil.h"
#include <stdio.h>
#include <wchar.h>

int main()
{
	setlocale(LC_CTYPE, "");
	//wchar_t *pszServer = L"http://192.168.1.33/jsbin/server.php";
	//wchar_t *server2 = L"http://192.168.1.33/test.php";
	wchar_t *httpsurl = L"https://www.lance.moe/";
	wchar_t *httpsurl2 = L"https://www.lance.moe/";
	char recv[102400] = { 0 };
	char *pszPostData = "a1=mytest2&a2=qq9222";


	WinHttpInit();

	//recvlen = SendHttpRequest(L"POST", pszServer, (UCHAR*)pszPostData, strlen(pszPostData) + 1, recv, FALSE);

	//printf("%s\n", recv);

	//recvlen = SendHttpRequest(L"POST", server2, (UCHAR*)pszPostData, strlen(server2) + 1, recv, FALSE);

	//printf("%s\n", recv);
	//proxy test
	//SetProxy(L"192.168.1.1:3128", L"", L"");

	//recvlen = SendHttpRequest(L"POST", server2, (UCHAR*)pszPostData, strlen(server2) + 1, recv, TRUE);

	printf("%s\n", SendHttpRequest(L"GET", httpsurl, "", 0));
	//printf("%s\n", SendHttpRequest(L"POST", httpsurl, pszPostData, 0));
	getchar();
	return 0;
}