#pragma once
#include <extdll.h>
#include "util.h"
#include "cbase.h"
#include "CBaseMonster.h"
#include "CBasePlayer.h"
#include "CBasePlayerWeapon.h"
#include "user_messages.h"
#include "PluginHooks.h"

#define CLASS_XRACE_PITDRONE CLASS_ALIEN_RACE_X_PITDRONE
#define CLASS_XRACE_SHOCK CLASS_ALIEN_RACE_X

#if defined(__GNUC__) || defined (_MSC_VER) && _MSC_VER >= 1400
#define snprintf	_snprintf
#define vsnprintf	_vsnprintf
#define unlink		_unlink
#define strlwr		_strlwr
#define strdup		_strdup
#define strcasecmp	_stricmp
#define strncasecmp	_strnicmp
#define getcwd		_getcwd
#define open		_open
#define read		_read
#define write		_write
#define close		_close
#endif

#define MDLL_FUNC	gpGamedllFuncs->dllapi_table

#define MDLL_GameDLLInit				MDLL_FUNC->pfnGameInit
#define MDLL_Spawn						MDLL_FUNC->pfnSpawn
#define MDLL_Think						MDLL_FUNC->pfnThink
#define MDLL_Use						MDLL_FUNC->pfnUse
#define MDLL_Touch						MDLL_FUNC->pfnTouch
#define MDLL_Blocked					MDLL_FUNC->pfnBlocked
#define MDLL_KeyValue					MDLL_FUNC->pfnKeyValue
#define MDLL_Save						MDLL_FUNC->pfnSave
#define MDLL_Restore					MDLL_FUNC->pfnRestore
#define MDLL_ObjectCollsionBox			MDLL_FUNC->pfnAbsBox
#define MDLL_SaveWriteFields			MDLL_FUNC->pfnSaveWriteFields
#define MDLL_SaveReadFields				MDLL_FUNC->pfnSaveReadFields
#define MDLL_SaveGlobalState			MDLL_FUNC->pfnSaveGlobalState
#define MDLL_RestoreGlobalState			MDLL_FUNC->pfnRestoreGlobalState
#define MDLL_ResetGlobalState			MDLL_FUNC->pfnResetGlobalState
#define MDLL_ClientConnect				MDLL_FUNC->pfnClientConnect
#define MDLL_ClientDisconnect			MDLL_FUNC->pfnClientDisconnect
#define MDLL_ClientKill					MDLL_FUNC->pfnClientKill
#define MDLL_ClientPutInServer			MDLL_FUNC->pfnClientPutInServer
#define MDLL_ClientCommand				MDLL_FUNC->pfnClientCommand
#define MDLL_ClientUserInfoChanged		MDLL_FUNC->pfnClientUserInfoChanged
#define MDLL_ServerActivate				MDLL_FUNC->pfnServerActivate
#define MDLL_ServerDeactivate			MDLL_FUNC->pfnServerDeactivate
#define MDLL_PlayerPreThink				MDLL_FUNC->pfnPlayerPreThink
#define MDLL_PlayerPostThink			MDLL_FUNC->pfnPlayerPostThink
#define MDLL_StartFrame					MDLL_FUNC->pfnStartFrame
#define MDLL_ParmsNewLevel				MDLL_FUNC->pfnParmsNewLevel
#define MDLL_ParmsChangeLevel			MDLL_FUNC->pfnParmsChangeLevel
#define MDLL_GetGameDescription			MDLL_FUNC->pfnGetGameDescription
#define MDLL_PlayerCustomization		MDLL_FUNC->pfnPlayerCustomization
#define MDLL_SpectatorConnect			MDLL_FUNC->pfnSpectatorConnect
#define MDLL_SpectatorDisconnect		MDLL_FUNC->pfnSpectatorDisconnect
#define MDLL_SpectatorThink				MDLL_FUNC->pfnSpectatorThink
#define MDLL_Sys_Error					MDLL_FUNC->pfnSys_Error
#define MDLL_PM_Move					MDLL_FUNC->pfnPM_Move
#define MDLL_PM_Init					MDLL_FUNC->pfnPM_Init
#define MDLL_PM_FindTextureType			MDLL_FUNC->pfnPM_FindTextureType
#define MDLL_SetupVisibility			MDLL_FUNC->pfnSetupVisibility
#define MDLL_UpdateClientData			MDLL_FUNC->pfnUpdateClientData
#define MDLL_AddToFullPack				MDLL_FUNC->pfnAddToFullPack
#define MDLL_CreateBaseline				MDLL_FUNC->pfnCreateBaseline
#define MDLL_RegisterEncoders			MDLL_FUNC->pfnRegisterEncoders
#define MDLL_GetWeaponData				MDLL_FUNC->pfnGetWeaponData
#define MDLL_CmdStart					MDLL_FUNC->pfnCmdStart
#define MDLL_CmdEnd						MDLL_FUNC->pfnCmdEnd
#define MDLL_ConnectionlessPacket		MDLL_FUNC->pfnConnectionlessPacket
#define MDLL_GetHullBounds				MDLL_FUNC->pfnGetHullBounds
#define MDLL_CreateInstancedBaselines	MDLL_FUNC->pfnCreateInstancedBaselines
#define MDLL_InconsistentFile			MDLL_FUNC->pfnInconsistentFile
#define MDLL_AllowLagCompensation		MDLL_FUNC->pfnAllowLagCompensation

#define UTIL_LogPrintf(fmt, ...) ALERT(at_logged, fmt, __VA_ARGS__)
#define ClientPrint UTIL_ClientPrint
#define ClientPrintALL UTIL_ClientPrintAll

#define GET_INFOKEYBUFFER	(*g_engfuncs.pfnGetInfoKeyBuffer)
#define GET_USER_MSG_ID(pluginid, name, size)  GetUserMsgInfo(name, size)

#ifndef _WIN32
#define _stricmp stricmp
#define _strdup strdup
#endif