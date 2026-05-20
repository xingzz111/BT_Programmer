/////////////////////////////////////////////////////////////////////
//
//					Media Player Production SDK
//
//			Copyright(c) 2011-2020 Actions Semiconductor,
//					All Rights Reserved.  
//
/////////////////////////////////////////////////////////////////////
#ifndef __PRODUCTION_INCLUDE__
#define __PRODUCTION_INCLUDE__
#pragma once

#ifdef CONTROLLER_IMPL
#define PRODUCTION_CLASS  _declspec(dllexport)
#define PRODUCTION_API    _declspec(dllexport)
#else
#define PRODUCTION_CLASS  _declspec( dllimport )
#define PRODUCTION_API    _declspec( dllimport )
#endif

//{{AFX_CONSTANT

// Application type
#define APP_PRODUCTION		0x01
#define APP_UPGRADE			0x02
#define APP_FLASH_DUMP		0x03

// Enumeration type
#ifndef ENUM_UDISK
#define ENUM_UDISK			0x000001
#define ENUM_ADFU			0x000100
#define ENUM_MTP			0x010000
#define ENUM_HID			0x040000
#define ENUM_DEFAULT		(ENUM_ADFU | ENUM_UDISK)
#endif


enum {
	ERR_FW_NONEXIST			= 0xFFFF0000,
	ERR_FW_OPEN,	
	ERR_FW_UNKNOWN,
	ERR_NO_DEVICE,
	ERR_SPEC_VER,
	ERR_PICK_SCRIPT,
	ERR_INVALID_PARM,
	ERR_NOT_PYTHON,	
	ERR_PY_LOAD,
	ERR_PY_FUNC,	
	ERR_BUSY,
};
//}}AFX_CONSTANT

//{{AFX_APP_STRUCTURE
/// Device information
struct DS_DEVICE_INFO{
	TCHAR	Drives[8];		// max 7 partitions
	TCHAR	DevPath[512];
	DWORD	InstanceId;
	DWORD	EndPoint;		// HUB end-point
};
typedef DS_DEVICE_INFO * LPDS_DEVICE_INFO;

/// DRM Parameters
struct DS_DRM_PARAM{
	BYTE		MajorVersion[4];
	BYTE		MinorVersion[4];	
	BYTE		Model[256];
	BYTE		Manufacturer[256];	
	TCHAR		PrivateKey[520];
	TCHAR		DevCertTmpl[520];
};
typedef DS_DRM_PARAM * LPDS_DRM_PARAM;

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

//}}AFX_APP_STRUCTURE

//{{AFX_ACTIONS_MEDIA_PRODUCTION_CALLBACK

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

typedef int (*PFN_PRODUCTIONCB)(int nType, DWORD dwDeviceId, WPARAM wParam, LPARAM lParam);

//}}AFX_ACTIONS_MEDIA_PRODUCTION_CALLBACK

//{{AFX_CONFIG_KEY
// Set Configuration by Application Tool
#define CFG_PRODUCTION			_T("PRODUCTION")
#define CFG_FLASH_ERASE			_T("FLASH_ERASE")
#define CFG_AUTOMATION			_T("AUTOMATION")
#define CFG_RESTART				_T("RESTART")
#define CFG_EXTFLASH_ERASE		_T("EXTFLASH_ERASE")

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

#define CFG_INIT_ADFUS			_T("INIT_ADFUS")	/* flag for download adfu server */
//}}AFX_CONFIG_KEY

#define SDKOP_PARM_BAK	0x01	/* get parameters before upgrade for backing-up */
#define SDKOP_UPGRADE	0x02	/* upgrade firmware */
#define SDKOP_PARM_SET	0x04	/* set parameters */
#define SDKOP_PARM_GET	0x08	/* get parameters */
#define SDKOP_FULL		(SDKOP_PARM_BAK|SDKOP_UPGRADE|SDKOP_PARM_SET|SDKOP_PARM_GET) /* backup -> upgrade -> set -> get */
#define SDKOP_PARM		(SDKOP_PARM_SET|SDKOP_PARM_GET) /* set -> get */

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

//////////////////////////////////////////////////////////////////////////
/// Production class interface
class PIMPL;
class PRODUCTION_CLASS CProduction
{
public:
	CProduction();
	virtual ~CProduction();

	/// Initialize after instance created, call it just once
	/// Parameters
	///		@nAppType	: [IN] Specifies the application type, see APP_ defined 
	///		@pCallBack	: [IN] Specifies the application callback interface	
	/// Return values
	///   Return 0 if succeeds, others is error-code 
	int		Initialize(UINT nAppType, PFN_PRODUCTIONCB pCallBack = NULL);

	/// Enumerate device notification, asynchronous
	/// Using DT_DEVICE callback to return the device list
	/// Parameters
	///		@nEnumMask	: [IN] Specifies the enumeration mask, see ENUM_ defined
	void	RefreshDevice(UINT nEnumMask = ENUM_DEFAULT);

	/// Set firmware file before call Download function 
	///		@lpszFirmware	: [IN]  Firmware file path
	///		@pFirmwareInfo	: [OUT] Firmware information
	///	Return 0 if file exist and it's valid, nonzero for others
	int		SetFirmwareFile(LPCTSTR lpszFirmware, LPDS_FIRMWARE_INFO pFirmwareInfo);

	/// Set production additional configuration
	/// Parameters
	///		@lpszKey	: [IN] Configuration KEY, such as DRM_PARAM / AUTO_COPY / UDISK_COPY etc
	///		@lpszConfig	: [IN] Configuration value
	///		@nLength	: [IN] bytes of @lpszConfig 
	/// NOTE: If there are multiple configurations, you should call it multiple times
	void	SetConfiguration(LPCTSTR lpszKey, LPCTSTR lpszConfig, int nLength = 0);

	/// Get configuration from firmware by specified KEY, without USB communication
	/// Parameters
	///		@lpszKey	: [IN] Config KEY
	/// Return values
	///		Return the value
	LPCTSTR	GetFirmwareConfig(LPCTSTR lpszKey);

	/// Download firmware
	/// Parameters
	///   non
	/// Return values
	///	  greater than zero if succeeded
	int		Download();	

private:
	PIMPL*	Impl;
};

//////////////////////////////////////////////////////////////////////////
//加载库
#ifndef CONTROLLER_IMPL
#pragma message("----------------------------------------------------------------------------------------------")

#ifdef _DEBUG
#   ifdef _UNICODE
#       pragma comment(lib, "ControllerD.lib")
#		pragma message("动态连接库 <ControllerD.lib> 连接的 <Controller.dll> 被连接")	
#   else
#       pragma comment(lib, "ControllerD.lib")
#		pragma message("动态连接库 <ControllerD.lib> 连接的 <Controller.dll> 被连接")	
#   endif
#else
#   ifdef _UNICODE
#       pragma comment(lib, "Controller.lib")
#		pragma message("动态连接库 <Controller.lib> 连接的 <Controller.dll> 被连接")	
#   else
#       pragma comment(lib, "Controller.lib")
#		pragma message("动态连接库 <Controller.lib> 连接的 <Controller.dll> 被连接")	
#   endif
#endif

#pragma message("----------------------------------------------------------------------------------------------")
#endif

#endif // !__PRODUCTION_INCLUDE__