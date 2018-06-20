#include <locale.h>
#include "WinHttpUtil.h"
#include <stdio.h>
#include <wchar.h>

int main()
{
	char* locname = setlocale(LC_ALL, "Chinese-simplified");
	printf("Current locale is: %s\n", locname);

	//std::locale::global(std::locale("Chinese-simplified"));

	//wchar_t *pszServer = L"http://192.168.1.33/jsbin/server.php";
	//wchar_t *server2 = L"http://192.168.1.33/test.php";
	wchar_t httpsurl[] = L"https://www.lance.moe/";
	wchar_t httpsurl2[] = L"https://www.lance.moe/";
	wchar_t httpurl_IP138[] = L"http://www.ip138.com/ips1388.asp?ip=180.104.222.195&action=2";

	char recv[102400] = { 0 };
	char pszPostData[] = "a1=mytest2&a2=qq9222";


	WinHttpUtilInit();

	//recvlen = WinHttpUtilSendRequest(L"POST", pszServer, (UCHAR*)pszPostData, strlen(pszPostData) + 1, recv, FALSE);

	//printf("%s\n", recv);

	//recvlen = WinHttpUtilSendRequest(L"POST", server2, (UCHAR*)pszPostData, strlen(server2) + 1, recv, FALSE);

	//printf("%s\n", recv);
	//proxy test
	//SetProxy(L"192.168.1.1:3128", L"", L"");

	//recvlen = WinHttpUtilSendRequest(L"POST", server2, (UCHAR*)pszPostData, strlen(server2) + 1, recv, TRUE);


	char* s1 = WinHttpUtilSendRequest(L"GET", httpurl_IP138, NULL, 0, NULL, NULL, NULL);
	printf("%s\n", s1);


	//printf("%s\n", WinHttpUtilSendRequest(L"GET", httpsurl, NULL, 0, NULL, NULL, NULL));
	//printf("%s\n", WinHttpUtilSendRequest(L"POST", httpsurl, pszPostData, 0));
	getchar();
	return 0;
}