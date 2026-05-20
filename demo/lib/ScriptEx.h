#ifndef __SCRIPT_LIB_H__
#define __SCRIPT_LIB_H__
#pragma once
#include <Windows.h>
#include <tchar.h>
//////////////////////////////////////////////////////////////////////////

#ifdef SCRIPT_IMPL
#define SCRIPT_CLASS  _declspec(dllexport)
#define SCRIPT_API    _declspec(dllexport)
#else
#define SCRIPT_CLASS  _declspec( dllimport )
#define SCRIPT_API    _declspec( dllimport )
#endif

//////////////////////////////////////////////////////////////////////////

enum FirmwareType
{
	FW_NONE			= 0,	// 不是Firmware文件
	FW_RAW			= 1,
	FW_DB			= 2,	// 类C脚本的Firmware文件
	FW_PYTHON		= 3,	// Python脚本的Firmware文件
	FW_PYTHON_EXT	= 4,
	FW_DB_AUDIO		= 5,
	FW_DB_VIDEO		= 6
};

#define CN_CHIPID_SIZE		512
#define CN_PDPRIVATE_SIZE	512
#define CN_PDCERT_SIZE		1024
#define CN_LFI_HEAD_SIZE	8192

//////////////////////////////////////////////////////////////////////////
class PIMPL_FWE;
class SCRIPT_CLASS CFirmwareEngine
{
public:
	CFirmwareEngine();
	~CFirmwareEngine();

	void	SetTempDir(LPCTSTR lpszTempDir);	

	int		OpenFirmware(LPCTSTR lpszFirmwareFile);	
	void	CloseFirmware();

	int		GetFirmwareBin(unsigned char* fwBin, char* lpszImageFile);	

	LPCTSTR	QueryValue(LPCTSTR lpszKey, int nQueryIndex = -1, bool bQueryAll = false);

	// Pick binary data from temp.img and save as file
	int		SaveBinFile(LPCTSTR szSrcFile, LPCTSTR szDestFile);

private:
	PIMPL_FWE*	impl;
};

//////////////////////////////////////////////////////////////////////////
//加载库
#ifndef SCRIPT_IMPL
#pragma message("----------------------------------------------------------------------------------------------")

#ifdef _DEBUG
#   pragma comment(lib, "ScriptExD.lib")
#	pragma message("动态连接库 <ScriptExD.lib> 连接的 <ScriptEx.dll> 被连接")	

#else
#   pragma comment(lib, "ScriptEx.lib")
#	pragma message("动态连接库 <ScriptEx.lib> 连接的 <ScriptEx.dll> 被连接")	

#endif

#pragma message("----------------------------------------------------------------------------------------------")
#endif

//////////////////////////////////////////////////////////////////////////
#endif/// !(__SCRIPT_LIB_H__)