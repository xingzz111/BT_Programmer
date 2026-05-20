// CommandApp.cpp : Defines the entry point for the console application.
//
#include "StdAfx.h"
#include "resource.h"
#include <shlwapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "shlwapi.lib")
#pragma warning(disable:4996)
#endif

//////////////////////////////////////////////////////////////////////////
/* import sdk library */
#include ".\lib\Controller.h"

#define SCRIPT_IMPL
#include ".\lib\ScriptEx.h"

CProduction*	g_pProduction	= NULL;
BOOL			g_bIsUpgrading	= FALSE;
UINT			g_nDeviceCount	= 0;
UINT			g_nExitCode		= 0;
static HANDLE	g_hEnumEvent	= NULL;

#define MAX_DEVICES 64
static DS_DEVICE_INFO g_Devices[MAX_DEVICES];
static UINT g_DeviceInfoCount = 0;
static LONG g_DeviceSucceedCount = 0;
static LONG g_DeviceFailCount = 0;
static int g_SdkFinalCode = 0;
static TCHAR g_SdkFinalReason[512] = { 0 };

#define MAX_USR_PARM	0x4000
unsigned char	g_pParmSet[MAX_USR_PARM];
unsigned char	g_pParmGet[MAX_USR_PARM];

int		PRODUCTION_NOTIFY(int nType, DWORD dwDeviceId, WPARAM wParam, LPARAM lParam);
void	WriteLog(LPCTSTR lpszFormat, ...);
//////////////////////////////////////////////////////////////////////////

HWND				g_hMsgWnd = NULL;
HWND				CreateMsgWnd();
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#define BIT_CFG_PRODUCTION (1<<0)
#define BIT_CFG_FLASH_ERASE (1<<1)
#define BIT_CFG_EXTFLASH_ERASE (1<<2)
#define BIT_CFG_AUTOMATION (1<<3)

#define BIT_CFG_RESTART	 (1<<4)
#define BIT_CFG_RESERVER5 (1<<5)
#define BIT_CFG_RESERVER6 (1<<6)
#define BIT_CFG_RESERVER7 (1<<7)

int _tmain(int argc, _TCHAR* argv[])
{
	printf("DemoSDK ver.20250821.1\n");
	WriteLog(_T("DemoSDK ver.20250821.1"));
	
	/* firmware file */
	LPCTSTR lpszFW = _T("e:\\111\\2837\\123.fw");
	if (argc > 1) lpszFW = argv[1];

// 	int ops = SDKOP_FULL;
// 	if (argc > 2) 
// 	{
// 		ops = _ttoi(argv[2]);
// 		if (ops <= 0 || ops > SDKOP_FULL)
// 		{
// 			ops = SDKOP_FULL;
// 			printf("Invalid SDKOP, force to default %d\n", ops);
// 		}
// 	}	

	g_pProduction	= new CProduction;
	if (0 != g_pProduction->Initialize(APP_PRODUCTION,(PFN_PRODUCTIONCB)PRODUCTION_NOTIFY))
	{
		printf("Failed to initialize product instance!\n");
		WriteLog(_T("Failed to initialize product instance!"));
		printf("RESULT=FAIL code=INIT_FAIL sdk_code=INIT_FAIL reason=Initialize_failed\n");
		g_nExitCode = 1;
	}
	else
	{
		printf ("Reading firmware ...\n");
		DS_FIRMWARE_INFO dsInfo;
		int nFW = g_pProduction->SetFirmwareFile(lpszFW, &dsInfo);
		if (nFW != 0)
		{
			printf("RESULT=FAIL code=FW_INVALID sdk_code=%d reason=SetFirmwareFile_failed\n", nFW);
			g_nExitCode = 1;
		}
		else
		{

		/* configuration */
		g_pProduction->SetConfiguration(CFG_PRODUCTION,  _T("1"));

		int iConfigMask = 0xf;
		if (argc > 2) 
			iConfigMask = _tcstoul(argv[2] , NULL , 0)	;
		
		if (iConfigMask & BIT_CFG_FLASH_ERASE)
			g_pProduction->SetConfiguration(CFG_FLASH_ERASE, _T("1"));
		if (iConfigMask & BIT_CFG_EXTFLASH_ERASE)
			g_pProduction->SetConfiguration(CFG_EXTFLASH_ERASE, _T("1"));
		if (iConfigMask & BIT_CFG_AUTOMATION)
			g_pProduction->SetConfiguration(CFG_AUTOMATION, _T("1"));
		if (iConfigMask & BIT_CFG_RESTART)
			g_pProduction->SetConfiguration(CFG_RESTART, _T("1"));
		
		

		/* sdk operations, see SDKOP_* in Controller.h */
		//g_pProduction->SetConfiguration(CFG_SDK_OPS, (LPCTSTR)&ops, sizeof(int));
		/* process user parameters in PRODUCTION_NOTIFY function*/
		memset(g_pParmGet, 0, sizeof(g_pParmGet));
		memset(g_pParmSet, 0, sizeof(g_pParmSet));

		/* find device, MSC & ADFU mode */
		g_hEnumEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!g_hEnumEvent)
			return -1000;
		ResetEvent(g_hEnumEvent);
		g_pProduction->RefreshDevice(ENUM_ADFU | ENUM_UDISK);
		if (WAIT_OBJECT_0 != WaitForSingleObject(g_hEnumEvent, 10000))
		{
			ResetEvent(g_hEnumEvent);
			g_pProduction->RefreshDevice(ENUM_ADFU | ENUM_UDISK);
			WaitForSingleObject(g_hEnumEvent, 10000);
		}
		if (g_nDeviceCount > 0)
		{
			printf ("Downloading firmware ...\n");	

			g_bIsUpgrading = TRUE;
			g_pProduction->Download();		

			Sleep(2000);

			if (g_bIsUpgrading)
				CreateMsgWnd();

			if (g_DeviceFailCount > 0 || g_SdkFinalCode != 0)
			{
				char szReason[1024] = { 0 };
				printf("RESULT=FAIL code=DOWNLOAD_FAIL sdk_code=%d reason=%s\n",
					g_SdkFinalCode,
					g_SdkFinalReason[0] ? Wide2Ansi(g_SdkFinalReason, szReason) : "unknown");
				g_nExitCode = 1;
			}
			else
			{
				printf("RESULT=PASS code=0 sdk_code=0 reason=ok\n");
				g_nExitCode = 0;
			}
			printf("SUMMARY devices=%u succeed=%ld fail=%ld\n", g_nDeviceCount, g_DeviceSucceedCount, g_DeviceFailCount);
		}
		else
		{
			printf("RESULT=FAIL code=NO_DEVICE sdk_code=NO_DEVICE reason=RefreshDevice_timeout_or_zero\n");
			g_nExitCode = 1;
		}
		}
	}

	Sleep(500);
	if (g_hEnumEvent)
		CloseHandle(g_hEnumEvent);
	delete g_pProduction;
	g_pProduction = NULL;

	return g_nExitCode;
}

//////////////////////////////////////////////////////////////////////////
const char* Wide2Ansi(LPCWSTR lpsz, char* szVal)
{
	int nLen = wcslen(lpsz);
	for (int i = 0; i < nLen; i++)
		szVal[i] = (char)lpsz[i];
	szVal[nLen] = 0;
	return szVal;
}

int	PRODUCTION_NOTIFY(int nType, DWORD dwDeviceId, WPARAM wParam, LPARAM lParam)
{
	char szInfo[256] = { 0 };
	switch(nType)
	{
	case DT_DEVICE:
		WriteLog(_T("%d device found"), (int)wParam);
		printf("%d device found\n", (int)wParam);
		printf("EVENT=DEVICE count=%d\n", (int)wParam);
		g_nDeviceCount = (int)wParam;
		g_DeviceInfoCount = 0;
		if (lParam && wParam > 0)
		{
			g_DeviceInfoCount = (UINT)wParam;
			if (g_DeviceInfoCount > MAX_DEVICES)
				g_DeviceInfoCount = MAX_DEVICES;
			memcpy(g_Devices, (void*)lParam, sizeof(DS_DEVICE_INFO) * g_DeviceInfoCount);
			for (UINT i = 0; i < g_DeviceInfoCount; i++)
			{
				printf("DEVICE index=%u ep=%u instance=%u drive=%S path=%S\n",
					i + 1,
					g_Devices[i].EndPoint,
					g_Devices[i].InstanceId,
					g_Devices[i].Drives,
					g_Devices[i].DevPath);
			}
		}
		if (g_hEnumEvent)
			SetEvent(g_hEnumEvent);
		break;
	case DT_START:
		WriteLog(_T("Start %d"), dwDeviceId);
		printf("EVENT=START deviceId=%d\n", dwDeviceId);
		if (dwDeviceId > 0 && dwDeviceId <= g_DeviceInfoCount)
		{
			WriteLog(_T("Start %d: EP=%u IID=%u"), dwDeviceId, g_Devices[dwDeviceId - 1].EndPoint, g_Devices[dwDeviceId - 1].InstanceId);
			printf("MAP deviceId=%d ep=%u instance=%u path=%S\n", dwDeviceId, g_Devices[dwDeviceId - 1].EndPoint, g_Devices[dwDeviceId - 1].InstanceId, g_Devices[dwDeviceId - 1].DevPath);
		}
		break;
	case DT_STATUS:
		WriteLog(_T("%d: %s"), dwDeviceId, (LPCTSTR)wParam);
		printf ("[%d] %s\n", dwDeviceId , Wide2Ansi((LPCTSTR)wParam, szInfo));
		break;
	case DT_LOG:
		WriteLog(_T("%d: %s"), dwDeviceId, (LPCTSTR)wParam);
		break;
	case DT_CAPACITY:
		WriteLog(_T("%d: Capacity = %d"), dwDeviceId, (int)wParam);
		break;
	case DT_PROGRESS:
		printf("[%d] %02d%%\r",dwDeviceId ,  (int)wParam);
		break;
	case DT_ERROR:
		WriteLog(_T("%d: ERROR[%s]"), dwDeviceId, (LPCTSTR)wParam);
		printf ("[%d] ERROR: %s\n", dwDeviceId , Wide2Ansi((LPCTSTR)wParam, szInfo));
		printf("EVENT=ERROR deviceId=%d msg=%s\n", dwDeviceId, Wide2Ansi((LPCTSTR)wParam, szInfo));
		break;
	case DT_COMPLETE:		
		if (dwDeviceId > 0)
		{
			g_SdkFinalCode = (int)wParam;
			if (g_SdkFinalCode == 0) 
			{
				//printf("Successful\n");
				WriteLog(_T("%d: Successful"), dwDeviceId);
				InterlockedIncrement(&g_DeviceSucceedCount);
			}
			else
			{
				printf("[%d] Production failed, %d\n", dwDeviceId , g_SdkFinalCode);
				InterlockedIncrement(&g_DeviceFailCount);
				if (lParam)
				{
					_tcsncpy(g_SdkFinalReason, (LPCTSTR)lParam, _countof(g_SdkFinalReason) - 1);
					WriteLog(_T("%d: FAIL[%d: %s]"), dwDeviceId, g_SdkFinalCode, (LPCTSTR)lParam);
				}
				else
					WriteLog(_T("%d: FAIL[%d]"), dwDeviceId, g_SdkFinalCode);
			}
			printf("EVENT=COMPLETE deviceId=%d sdk_code=%d\n", dwDeviceId, g_SdkFinalCode);
		}
		else
		{
			printf ("OnComplete\n");
			printf("EVENT=ALL_COMPLETE\n");
			::PostMessage(g_hMsgWnd, WM_DEVICECHANGE, 0, 0);
			g_bIsUpgrading = FALSE;
		}
		break;

	case DT_CFG_GET:	/* write user parameters */
		printf ("\n%s\n", Wide2Ansi((LPCTSTR)wParam, szInfo));
		if (_tcscmp((LPCTSTR)wParam, CFG_USR_PARM_R) == 0
			|| _tcscmp((LPCTSTR)wParam, CFG_USR_PARM_W) == 0)
		{	/* which parameters to read or write */

			bool bIsRead = _tcscmp((LPCTSTR)wParam, CFG_USR_PARM_R) == 0;

			usr_item_t vp[] = {
				{ 0, 0x130000, bIsRead?0:0x40, {0} },
				{ 0, 0x140000, bIsRead?0:0x40, {0} },
				{ 1, 0x01,     bIsRead?0:0x30, {0} },
				{ 1, 0x02,     bIsRead?0:0x18, {0} },
			};	

			int offset = 4;
			unsigned char* parm = bIsRead? g_pParmGet : g_pParmSet;
			for (int i = 0; i < 4; i++)
			{
				memcpy(&parm[offset], &vp[i], 12);
				offset += 12;
				printf("TO_%s: %02x-%02x, %d\n", bIsRead? "READ" : "WRITE", vp[i].type, vp[i].subtype, vp[i].length);

				if (vp[i].length)
				{
					memset(&parm[offset], 0x35 + i, vp[i].length);
					offset += vp[i].length;
				}
			}
			memcpy(parm, &offset, 4);
			memcpy((unsigned char*)lParam, parm, offset);
		}
		break;

	case DT_CFG_SET:	/* read user parameters */
		printf ("\n%s\n", Wide2Ansi((LPCTSTR)wParam, szInfo));
		if (_tcscmp((LPCTSTR)wParam, CFG_USR_PARM_R) == 0
			|| _tcscmp((LPCTSTR)wParam, CFG_USR_PARM_W) == 0)
		{
			bool bIsRead = _tcscmp((LPCTSTR)wParam, CFG_USR_PARM_R) == 0;
			unsigned char* parm = (unsigned char*)lParam;
			int nLen = *((int*)parm), offset = 4;
					
			for (int i = offset; i < nLen; )
			{
				usr_item_t* p = (usr_item_t*)&parm[i];
				printf("%s: %02x-%02x, %d / %d\n", bIsRead? "READED" : "WRITEN", p->type, p->subtype, p->length, nLen);
				i += p->length + 12;
			}
			printf("\n");
		}
		break;

	default:
		break;
	}

	return 0;
}
//////////////////////////////////////////////////////////////////////////

HWND				CreateMsgWnd()
{
	MSG msg;
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Registers the window class.
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;//LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32APP));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= _T("WIN32");
	wcex.hIconSm		= NULL;//LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	::RegisterClassEx(&wcex);

	// Saves instance handle and creates main window
	g_hMsgWnd = CreateWindow(_T("WIN32"), _T("WIN32"), WS_OVERLAPPEDWINDOW,	CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	if (g_hMsgWnd)
	{
		ShowWindow(g_hMsgWnd, SW_HIDE);
		UpdateWindow(g_hMsgWnd);

		// Main message loop:
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);		

			if (!g_bIsUpgrading)
				break;
		}
	}

	return g_hMsgWnd;
}

LRESULT CALLBACK	WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{	
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_DEVICECHANGE:
		g_pProduction->RefreshDevice();
		break;

	default:		
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////

void	WriteLog(LPCTSTR lpszFormat, ...)
{
	TCHAR szLog[520] = { 0 };
	GetModuleFileName(NULL, szLog, _countof(szLog)-1);
	_tcsrchr(szLog, _T('\\'))[1] = 0;
	_tcscat(szLog, _T("Upgrade.log"));

	HANDLE hFile = ::CreateFile(szLog, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	SYSTEMTIME st;
	GetLocalTime(&st);

	va_list argList;
	va_start( argList, lpszFormat );
	TCHAR output[4096] = { 0 };
	vswprintf(output, lpszFormat, argList);
	va_end( argList );

	TCHAR szInfo[1024] = { 0 };
	wsprintf(szInfo, _T("%04d-%02d-%02d %02d:%02d:%02d %s\r\n"), 
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, output);

	DWORD dwWritten = 0;
	::SetFilePointer(hFile, 0, NULL, FILE_END);
	::WriteFile(hFile, szInfo, _tcslen(szInfo)*sizeof(TCHAR), &dwWritten, NULL);		
	::CloseHandle(hFile);
}

