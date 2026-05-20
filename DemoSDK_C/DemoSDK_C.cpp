// DemoSDK_C.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include "tchar.h"
#include "conio.h"
#include <string>

using namespace::std;

typedef int (*PFN_PRODUCTIONCB)(int nType, DWORD dwDeviceId, WPARAM wParam, LPARAM lParam);

int		PRODUCTION_NOTIFY(int nType, DWORD dwDeviceId, WPARAM wParam, LPARAM lParam);
void	WriteLog(LPCTSTR lpszFormat, ...);
const char* Wide2Ansi(LPCWSTR lpsz, char* szVal);

/// Firmware file information
struct DS_FIRMWARE_INFO{
	TCHAR		BootDisk[16];
	TCHAR		DeviceName[128];
	TCHAR		DeviceID[16];
	TCHAR		Productor[64];
	TCHAR		Version[64];
	WORD		VID;
	WORD		PID;
};
typedef DS_FIRMWARE_INFO * LPDS_FIRMWARE_INFO;

struct DS_DEVICE_INFO{
	TCHAR	Drives[8];
	TCHAR	DevPath[512];
	DWORD	InstanceId;
	DWORD	EndPoint;
};
typedef DS_DEVICE_INFO * LPDS_DEVICE_INFO;

BOOL			g_bIsUpgrading	= FALSE;
UINT			g_nDeviceCount	= 0;
UINT			g_nExitCode		= 0;
static LONG g_DeviceSucceedCount = 0;
static LONG g_DeviceFailCount = 0;
static int g_SdkFinalCode = 0;
static TCHAR g_SdkFinalReason[512] = { 0 };

#define MAX_USR_PARM	0x4000
unsigned char	g_pParmSet[MAX_USR_PARM];
unsigned char	g_pParmGet[MAX_USR_PARM];

// Enumeration type
#ifndef ENUM_UDISK
#define ENUM_UDISK			0x000001
#define ENUM_ADFU			0x000100
#define ENUM_MTP			0x010000
#define ENUM_HID			0x040000
#define ENUM_DEFAULT		(ENUM_ADFU | ENUM_UDISK)
#endif


typedef struct
{
	int		type;			/* 0:VRAM, 1:Parameter, 2:Firmware */
	int		subtype;		/* KeyId for VRAM or Parameters, Offset for Firmware */
	int		length;			/* set zero for read */
	char	data[1];		/* variable-length array, aligned with 4B */
}usr_item_t;
typedef struct
{
	int			total;		/* total bytes */			
	usr_item_t	item[1];	/* variable-length array */
}usr_parm_t;

HWND				g_hMsgWnd = NULL;
HWND				CreateMsgWnd();
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);


#define SDKOP_PARM_BAK	0x01	/* get parameters before upgrade for backing-up */
#define SDKOP_UPGRADE	0x02	/* upgrade firmware */
#define SDKOP_PARM_SET	0x04	/* set parameters */
#define SDKOP_PARM_GET	0x08	/* get parameters */
#define SDKOP_FULL		(SDKOP_PARM_BAK|SDKOP_UPGRADE|SDKOP_PARM_SET|SDKOP_PARM_GET) /* backup -> upgrade -> set -> get */
#define SDKOP_PARM		(SDKOP_PARM_SET|SDKOP_PARM_GET) /* set -> get */

#define CFG_PRODUCTION			_T("PRODUCTION")
#define CFG_FLASH_ERASE			_T("FLASH_ERASE")
#define CFG_AUTOMATION			_T("AUTOMATION")
#define CFG_RESTART				_T("RESTART")

#define CFG_PARTITION			_T("PARTITION")
#define CFG_FLASH_DUMP			_T("FLASH_DUMP")
#define CFG_ICCONFIG			_T("IC_CONFIG")
#define CFG_MAKE_CARD			_T("CARD_MAKE")

#define CFG_DRM_PARAM			_T("MTP_DRM")
#define CFG_AUTORUN				_T("AUTORUN")
#define CFG_UDISK_COPY			_T("UDISK_COPY")
#define CFG_HIDE_ADISK			_T("ADISK_COPY") 
#define CFG_HIDE_BDISK			_T("BDISK_COPY") 
#define CFG_HIDE_DISK			_T("HIDE_DISK") 
#define CFG_HDCP_KEY			_T("HDCP_KEY")
#define CFG_CHIPID1				_T("HW_CHIPID_INPUT")
#define CFG_BT_MAC				_T("CFG_BT_MAC")
#define CFG_SDK_OPS				_T("CFG_SDK_OPS")	/* see SDKOP_*	*/
#define CFG_USR_PARM_R			_T("USR_PARM_GET")	/* see usr_parm_t, give type & stubtype only */
#define CFG_USR_PARM_W			_T("USR_PARM_SET")	/* see usr_parm_t */

enum { 
	DT_DEVICE = 1,	/* @dwDeviceId: 0, @wParam: (int), @lParam: LPDS_DEVICE_INFO */
	DT_START,		/* @dwDeviceId non-zero, @wParam: NULL, @lParam: NULL */
	DT_STATUS,		/* @dwDeviceId non-zero, @wParam: LPCTSTR, @lParam: NULL */
	DT_MSGBOX,		/* @dwDeviceId non-zero, @wParam: LPCTSTR, @lParam: NULL */
	DT_LOG,			/* @dwDeviceId non-zero, @wParam: LPCTSTR, @lParam: NULL */
	DT_ERROR,		/* @dwDeviceId non-zero, @wParam: LPCTSTR, @lParam: NULL */
	DT_PROGRESS,	/* @dwDeviceId non-zero, @wParam: int, @lParam: NULL */
	DT_CAPACITY,	/* @dwDeviceId non-zero, @wParam: int, @lParam: NULL */
	DT_COMPLETE,	/* @dwDeviceId: zero for all device completed, non-zero for one device only, 
					@wParam: int(0 succeed, other failed), @lParam: LPCTSTR */
	DT_CFG_SET,		/* @dwDeviceId non-zero, @wParam: LPCTSTR(config-name), @lParam: LPTSTR(config-value) */
	DT_CFG_GET,		/* @dwDeviceId non-zero, @wParam: LPCTSTR(config-name), @lParam: LPTSTR(config-value) */
};

typedef HANDLE	( *CTRL_Initialize)( PFN_PRODUCTIONCB pCallBack);
typedef void	( *CTRL_RefreshDevice)(HANDLE hHandle , UINT nEnumMask );
typedef int	( *CTRL_SetFirmwareFile)(HANDLE hHandle , wchar_t* lpszFirmware, LPDS_FIRMWARE_INFO pFirmwareInfo);
typedef void	( *CTRL_SetConfiguration)(HANDLE hHandle ,  wchar_t* lpszKey, LPCTSTR lpszConfig, int nLength );
typedef wchar_t*	( *CTRL_GetFirmwareConfig)(HANDLE hHandle ,  wchar_t* lpszKey );
typedef int	( *CTRL_Download)(HANDLE hHandle );
typedef int	( *CTRL_Release)(HANDLE hHandle);

CTRL_RefreshDevice g_RefreshDevice;
HANDLE	g_Handle;
static HANDLE g_hEnumEvent = NULL;

#define MAX_DEVICES 64
static DS_DEVICE_INFO g_Devices[MAX_DEVICES];
static UINT g_DeviceInfoCount = 0;

int _tmain(int argc, _TCHAR* argv[])
{
	printf("DemoSDK_C ver.20221028.1\n");
	WriteLog(_T("DemoSDK_C ver.20221028.1"));

	TCHAR szIni[512] = {0};
	TCHAR  szModule[512] = { 0 };
	TCHAR szValue[1024] = { 0 };

	//DWORD dwLength = GetModuleFileName(AfxGetInstanceHandle(), szModule, sizeof(szModule));	
	DWORD dwLength = GetModuleFileName(NULL, szModule, sizeof(szModule));	
	for (DWORD dw = dwLength - 1; dw > 0; dw--)
	{
		if (szModule[dw] == '\\')
		{
			szModule[dw+1] = '\0';
			break;
		}
	}

	_tcscpy(szIni , szModule);
	_tcscat(szIni, _T("att.ini"));
	_tcscat(szModule, _T("Controller_C.dll"));

	// Load ActDbgUsb.dll	
	HINSTANCE m_hAct = LoadLibrary(szModule);
	if (!m_hAct)
	{
		printf("Failed to load %S\n", szModule);
		printf("RESULT=FAIL code=LOAD_DLL_FAIL sdk_code=LOAD_DLL_FAIL reason=LoadLibrary_failed\n");
		return -1000;
	}
	CTRL_Initialize act_init	= (CTRL_Initialize)GetProcAddress(m_hAct, "Initialize");
	CTRL_RefreshDevice act_RefreshDevice	= (CTRL_RefreshDevice)GetProcAddress(m_hAct, "RefreshDevice");
	g_RefreshDevice = act_RefreshDevice;
	CTRL_SetFirmwareFile  act_SetFirmwareFile	= (CTRL_SetFirmwareFile)GetProcAddress(m_hAct, "SetFirmwareFile");	

	CTRL_SetConfiguration act_SetConfiguration	= (CTRL_SetConfiguration)GetProcAddress(m_hAct, "SetConfiguration");
	CTRL_GetFirmwareConfig act_GetFirmwareConfig	= (CTRL_GetFirmwareConfig)GetProcAddress(m_hAct, "GetFirmwareConfig");
	CTRL_Download  act_Download	= (CTRL_Download)GetProcAddress(m_hAct, "Download");	
	CTRL_Release  act_Release	= (CTRL_Release)GetProcAddress(m_hAct, "Release");	

	if (!act_init || !act_RefreshDevice || !act_SetFirmwareFile || !act_SetConfiguration || !act_Download || !act_Release)
	{
		printf("Invalid Controller_C.dll exports\n");
		printf("RESULT=FAIL code=LOAD_DLL_EXPORT_FAIL sdk_code=LOAD_DLL_EXPORT_FAIL reason=GetProcAddress_failed\n");
		return -1000;
	}

	g_hEnumEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!g_hEnumEvent)
		return -1000;



	/* firmware file */
	wchar_t* lpszFW = _T("e:\\111\\123.fw");
	if (argc > 1) lpszFW = argv[1];

	int ops = SDKOP_FULL;
	if (argc > 2) 
	{
		ops = _ttoi(argv[2]);
		if (ops <= 0 || ops > SDKOP_FULL)
		{
			ops = SDKOP_FULL;
			printf("Invalid SDKOP, force to default %d\n", ops);
		}
	}	

	HANDLE hProduction = act_init((PFN_PRODUCTIONCB)PRODUCTION_NOTIFY);
	if ((HANDLE)-1 == hProduction)
	{
		printf("Failed to initialize product instance!\n");
		WriteLog(_T("Failed to initialize product instance!"));
		printf("RESULT=FAIL code=INIT_FAIL sdk_code=INIT_FAIL reason=Initialize_failed\n");
		g_nExitCode = 1;
	}
	else
	{
		g_Handle = hProduction;
		printf ("Reading firmware ...\n");
		DS_FIRMWARE_INFO dsInfo;
		int nFW = act_SetFirmwareFile(hProduction ,lpszFW, &dsInfo);
		if (nFW != 0)
		{
			printf("RESULT=FAIL code=FW_INVALID sdk_code=%d reason=SetFirmwareFile_failed\n", nFW);
			g_nExitCode = 1;
		}
		else
		{

		/* configuration */
		act_SetConfiguration(hProduction,CFG_PRODUCTION,  _T("1") ,0);
		act_SetConfiguration(hProduction,CFG_FLASH_ERASE, _T("1") , 0);

		/* sdk operations, see SDKOP_* in Controller.h */
		act_SetConfiguration(hProduction ,CFG_SDK_OPS, (LPCTSTR)&ops, sizeof(int));
		/* process user parameters in PRODUCTION_NOTIFY function*/
		memset(g_pParmGet, 0, sizeof(g_pParmGet));
		memset(g_pParmSet, 0, sizeof(g_pParmSet));

		/* find device, MSC & ADFU mode */
		ResetEvent(g_hEnumEvent);
		act_RefreshDevice(hProduction , ENUM_ADFU | ENUM_UDISK);

		if (WAIT_OBJECT_0 != WaitForSingleObject(g_hEnumEvent, 10000))
		{
			ResetEvent(g_hEnumEvent);
			act_RefreshDevice(hProduction , ENUM_ADFU | ENUM_UDISK);
			WaitForSingleObject(g_hEnumEvent, 10000);
		}
		if (g_nDeviceCount > 0)
		{
			printf ("Downloading firmware ...\n");	

			g_bIsUpgrading = TRUE;
			act_Download(hProduction);		

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
	if ((HANDLE)-1 != hProduction)
		act_Release(hProduction);
	if (g_hEnumEvent)
		CloseHandle(g_hEnumEvent);

	return g_nExitCode;
	return 0;
}


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
		g_RefreshDevice(g_Handle,ENUM_DEFAULT);
		
		break;

	default:		
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

const char* Wide2Ansi(LPCWSTR lpsz, char* szVal)
{
	int nLen = wcslen(lpsz);
	for (int i = 0; i < nLen; i++)
		szVal[i] = (char)lpsz[i];
	szVal[nLen] = 0;
	return szVal;
}


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
				char szDrive[64] = { 0 };
				char szPath[1024] = { 0 };
				Wide2Ansi(g_Devices[i].Drives, szDrive);
				Wide2Ansi(g_Devices[i].DevPath, szPath);
				printf("DEVICE index=%u ep=%u instance=%u drive=%s path=%s\n",
					i + 1,
					g_Devices[i].EndPoint,
					g_Devices[i].InstanceId,
					szDrive,
					szPath);
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
		printf ("[%d] %s\n",dwDeviceId ,  Wide2Ansi((LPCTSTR)wParam, szInfo));
		break;
	case DT_LOG:
		WriteLog(_T("%d: %s"), dwDeviceId, (LPCTSTR)wParam);
		break;
	case DT_CAPACITY:
		WriteLog(_T("%d: Capacity = %d"), dwDeviceId, (int)wParam);
		break;
	case DT_PROGRESS:
		printf("[%d] %02d%%\r", dwDeviceId , (int)wParam);
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
			printf ("[%d] OnComplete\n" , dwDeviceId);
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
