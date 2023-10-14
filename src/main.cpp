#include "mmlib.h"
#include "bot_const.h"
#include "bot.h"
#include "waypoint.h"

using namespace std;

#pragma comment(linker, "/EXPORT:GiveFnptrsToDll=_GiveFnptrsToDll@8")
#pragma comment(linker, "/SECTION:.data,RW")

extern cvar_t bot_ver_cvar;

int debug_engine;
char* g_argv;
static FILE* fp;

extern CBotGlobals gBotGlobals;

plugin_info_t Plugin_info = {
    META_INTERFACE_VERSION,	// ifvers
    BOT_NAME,	// name
    BOT_VER,	// version
    __DATE__,	// date
    BOT_AUTHOR,	// author
    BOT_WEBSITE,	// url
    BOT_DBG_MSG_TAG,	// logtag, all caps please
    PT_STARTUP,	// (when) loadable
    PT_NEVER,	// (when) unloadable
};

extern int debug_engine;
extern char* g_argv;

DLL_GLOBAL const Vector g_vecZero = Vector(0, 0, 0);

CBotGlobals gBotGlobals;

extern CWaypointVisibilityTable WaypointVisibility;
extern CWaypointLocations WaypointLocations;

cvar_t bot_ver_cvar = { BOT_VER_CVAR,BOT_VER,FCVAR_SERVER };

int pfnCmd_Argc(void)
{
	// this function returns the number of arguments the current client command string has. Since 
	// bots have no client DLL and we may want a bot to execute a client command, we had to 
	// implement a g_argv string in the bot DLL for holding the bots' commands, and also keep 
	// track of the argument count. Hence this hook not to let the engine ask an unexistent client 
	// DLL for a command we are holding here. Of course, real clients commands are still retrieved 
	// the normal way, by asking the engine. 
#ifdef RCBOT_META_BUILD
	if (gBotGlobals.m_bIsFakeClientCommand) {
		RETURN_META_VALUE(MRES_SUPERCEDE, gBotGlobals.m_iFakeArgCount);
	}

	RETURN_META_VALUE(MRES_IGNORED, 0);
#else
	// is this a bot issuing that client command ? 
	if (gBotGlobals.m_bIsFakeClientCommand)
		return (gBotGlobals.m_iFakeArgCount); // if so, then return the argument count we know 

	return ((*g_engfuncs.pfnCmd_Argc) ()); // ask the engine how many arguments there are 
#endif

}

const char* pfnCmd_Args(void)
{
	// this function returns a pointer to the whole current client command string. Since bots 
	// have no client DLL and we may want a bot to execute a client command, we had to implement 
	// a g_argv string in the bot DLL for holding the bots' commands, and also keep track of the 
	// argument count. Hence this hook not to let the engine ask an unexistent client DLL for a 
	// command we are holding here. Of course, real clients commands are still retrieved the 
	// normal way, by asking the engine. 

	extern char* g_argv;

	// is this a bot issuing that client command ? 
	if (gBotGlobals.m_bIsFakeClientCommand)
	{
#ifdef RCBOT_META_BUILD
		// is it a "say" or "say_team" client command ? 
		if (strncmp("say ", g_argv, 4) == 0) {
			RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[0] + 4); // skip the "say" bot client command (bug in HL engine) 
		}
		else if (strncmp("say_team ", g_argv, 9) == 0) {
			RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[0] + 9); // skip the "say_team" bot client command (bug in HL engine) 
		}
		RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[0]); // else return the whole bot client command string we know 
#else
		// is it a "say" or "say_team" client command ? 
		if (strncmp("say ", g_argv, 4) == 0)
			return (&g_argv[0] + 4); // skip the "say" bot client command (bug in HL engine) 
		else if (strncmp("say_team ", g_argv, 9) == 0)
			return (&g_argv[0] + 9); // skip the "say_team" bot client command (bug in HL engine) 

		return (&g_argv[0]); // else return the whole bot client command string we know 
#endif
	}

#ifdef RCBOT_META_BUILD
	RETURN_META_VALUE(MRES_IGNORED, NULL);
#else
	return ((*g_engfuncs.pfnCmd_Args) ()); // ask the client command string to the engine 
#endif

}

const char* pfnCmd_Argv(int argc)
{
	// this function returns a pointer to a certain argument of the current client command. Since 
	// bots have no client DLL and we may want a bot to execute a client command, we had to 
	// implement a g_argv string in the bot DLL for holding the bots' commands, and also keep 
	// track of the argument count. Hence this hook not to let the engine ask an unexistent client 
	// DLL for a command we are holding here. Of course, real clients commands are still retrieved 
	// the normal way, by asking the engine.

	extern char* g_argv;


#ifdef RCBOT_META_BUILD
	if (gBotGlobals.m_bIsFakeClientCommand)
	{
		RETURN_META_VALUE(MRES_SUPERCEDE, GetArg(g_argv, argc));
	}
	else
	{
		RETURN_META_VALUE(MRES_IGNORED, NULL);
		//return ((*g_engfuncs.pfnCmd_Argv) (argc));    
	}
#else
	// is this a bot issuing that client command ? 
	if (gBotGlobals.m_bIsFakeClientCommand)
		return (GetArg(g_argv, argc)); // if so, then return the wanted argument we know 

	return ((*g_engfuncs.pfnCmd_Argv) (argc)); // ask the argument number "argc" to the engine 
#endif
}

const char* GetArg(const char* command, int arg_number)
{
	// the purpose of this function is to provide fakeclients (bots) with the same Cmd_Argv 
	// convenience the engine provides to real clients. This way the handling of real client 
	// commands and bot client commands is exactly the same, just have a look in engine.cpp 
	// for the hooking of pfnCmd_Argc, pfnCmd_Args and pfnCmd_Argv, which redirects the call 
	// either to the actual engine functions (when the caller is a real client), either on 
	// our function here, which does the same thing, when the caller is a bot. 

	int length, i, index = 0, arg_count = 0, fieldstart, fieldstop;

	// multiple buffers are used in case the calling DLL saves args in multiple variables and
	// expects them to point to unique memory areas. (say command processing in Sven Co-op does this)
	static char arg[128][1024];
	int argidx = arg_number % 128;

	//	if ( arg_number == 0 )
	arg[argidx][0] = 0; // reset arg 

	if (!command || !*command)
		return NULL;

	length = strlen(command); // get length of command 

	// while we have not reached end of line 
	while ((index < length) && (arg_count <= arg_number))
	{
		while ((index < length) && (command[index] == ' '))
			index++; // ignore spaces 

		// is this field multi-word between quotes or single word ? 
		if (command[index] == '"')
		{
			index++; // move one step further to bypass the quote 
			fieldstart = index; // save field start position 
			while ((index < length) && (command[index] != '"'))
				index++; // reach end of field 
			fieldstop = index - 1; // save field stop position 
			index++; // move one step further to bypass the quote 
		}
		else
		{
			fieldstart = index; // save field start position 
			while ((index < length) && (command[index] != ' '))
				index++; // reach end of field 
			fieldstop = index - 1; // save field stop position 
		}

		// is this argument we just processed the wanted one ? 
		if (arg_count == arg_number)
		{
			for (i = fieldstart; i <= fieldstop; i++)
				arg[argidx][i - fieldstart] = command[i]; // store the field value in a string 
			arg[argidx][i - fieldstart] = 0; // terminate the string 
		}

		arg_count++; // we have processed one argument more 
	}

	return (&arg[argidx][0]); // returns the wanted argument 
}


int DispatchSpawn(edict_t* pent)
{
	if (gpGlobals->deathmatch)
	{
		char* pClassname = (char*)STRING(pent->v.classname);

		if (debug_engine)
		{
			FILE* fp;

			fp = fopen("bot.txt", "a");
			fprintf(fp, "DispatchSpawn: %x %s\n", pent, pClassname);
			if (pent->v.model != 0)
				fprintf(fp, " model=%s\n", STRING(pent->v.model));
			fclose(fp);
		}

		if (strcmp(pClassname, "worldspawn") == 0)
		{
			// do level initialization stuff here...
			gBotGlobals.MapInit();

			// clear waypoints
			WaypointInit();
			WaypointLoad(NULL);
		}

		if (gBotGlobals.IsNS())
		{
			if (!gBotGlobals.m_pMarineStart)
			{
				if (strcmp(STRING(pent->v.classname), "team_command") == 0)
				{
					gBotGlobals.m_pMarineStart = pent;

					if (gBotGlobals.IsCombatMap())
					{
						// to continously check if comm console is under attack
						gBotGlobals.m_CommConsole = CStructure(pent);
					}
				}
			}
			/*
			doesn't work here... obviously iuser3 isn't set till after DispatchSpawn ¬_¬

			// Use the bots vision to advantage
			// and add ant structures it sees in NS to the hivemind
			if ( EntityIsAlienStruct(pent) )
				gBotGlobals.m_HiveMind.AddStructure(pent,ENTINDEX(pent));*/
		}
	}

#ifdef RCBOT_META_BUILD
	RETURN_META_VALUE(MRES_IGNORED, 0);
#else
	return (*other_gFunctionTable.pfnSpawn)(pent);
#endif
}

void DispatchTouch(edict_t* pentTouched, edict_t* pentOther)
{
	static CBot* pBot;

	// only don't touch triggers if "no touch" is on the client.
	if (pentTouched->v.solid == SOLID_TRIGGER)
	{
		CClient* pClient = gBotGlobals.m_Clients.GetClientByEdict(pentOther);

		if (pClient)
		{
			if (pClient->m_bNoTouch)
			{
#ifdef RCBOT_META_BUILD
				RETURN_META(MRES_SUPERCEDE);
#else
				return;
#endif
			}
		}
	}

	pBot = UTIL_GetBotPointer(pentOther);

	if (pBot)
	{
		// bot code wants to evade engine seeing entities being touched.
		if (pBot->Touch(pentTouched))
		{
#ifdef RCBOT_META_BUILD
			RETURN_META(MRES_SUPERCEDE);
#else
			return;
#endif
		}
	}

#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*other_gFunctionTable.pfnTouch)(pentTouched, pentOther);
#endif
}

void DispatchBlocked(edict_t* pentBlocked, edict_t* pentOther)
{
	static CBot* pBot = NULL;

	// Save some time since we use a static variable
	BOOL bFindNewBot = TRUE;

	if (pBot != NULL)
	{
		if (pBot->m_pEdict == pentOther)
			bFindNewBot = FALSE;
	}

	if (bFindNewBot) // takes a few loops
		pBot = UTIL_GetBotPointer(pentOther);

	if (pBot)
		pBot->Blocked(pentBlocked);

#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*other_gFunctionTable.pfnBlocked)(pentBlocked, pentOther);
#endif
}

void DispatchKeyValue(edict_t* pentKeyvalue, KeyValueData* pkvd)
{
	gBotGlobals.KeyValue(pentKeyvalue, pkvd);


#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*other_gFunctionTable.pfnKeyValue)(pentKeyvalue, pkvd);
#endif
}

BOOL ClientConnect(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
	if (gpGlobals->deathmatch)
	{
		int iIndex = ENTINDEX(pEntity) - 1;

		CClient* pClient = gBotGlobals.m_Clients.GetClientByIndex(iIndex);

		gBotGlobals.m_iJoiningClients[iIndex] = 0;

		if (pClient)
		{
			pClient->Init();
		}

		if (debug_engine) { FILE* fp; fp = fopen("bot.txt", "a"); fprintf(fp, "ClientConnect: pent=%x name=%s\n", pEntity, pszName); fclose(fp); }

		if (!IS_DEDICATED_SERVER())
		{
			if (gBotGlobals.m_pListenServerEdict == NULL)
			{
				// check if this client is the listen server client
				if (strcmp(pszAddress, "loopback") == 0)
				{
					// save the edict of the listen server client...
					gBotGlobals.m_pListenServerEdict = pEntity;

					if (pClient)
						pClient->SetAccessLevel(10);
				}
			}
			else if (pEntity == gBotGlobals.m_pListenServerEdict)
			{
				if (pClient)
					pClient->SetAccessLevel(10);
			}
		}

		if (strcmp(pszAddress, "127.0.0.1") != 0) // not a bot connecting
		{
			if (gBotGlobals.m_iMinBots != -1)
			{
				int iNumPlayerCheck = UTIL_GetNumClients(TRUE) + 1 + gBotGlobals.GetNumJoiningClients();

				if ((gBotGlobals.m_iNumBots > gBotGlobals.m_iMinBots) && (iNumPlayerCheck > gBotGlobals.m_iMaxBots))
					// Can it kick a bot to free a slot?
				{
					int i;
					CBot* pBot;

					char cmd[80];

					for (i = 0; i < MAX_PLAYERS; i++)
					{
						pBot = &gBotGlobals.m_Bots[i];

						if (pBot)  // is this slot used?
						{
							if (pBot->IsUsed())
							{
								//int iUserId;

								//iUserId = (*g_engfuncs.pfnGetPlayerUserId)(pBot->m_pEdict);

								BotMessage(NULL, 0, "Kicking a bot from server to free a slot...");

								//sprintf(cmd, "kick #%d\n", iUserId);
								// kick [# userid] doesnt work on linux ??
								sprintf(cmd, "kick \"%s\"\n", pBot->m_szBotName);

								SERVER_COMMAND(cmd);  // kick the bot using kick name //(kick #id)

								gBotGlobals.m_fBotRejoinTime = gpGlobals->time + 2.0;
								gBotGlobals.m_bBotCanRejoin = FALSE;

								break;
							}
						}
					}
					// kick the bot				   
				}
				else if (gBotGlobals.IsConfigSettingOn(BOT_CONFIG_RESERVE_BOT_SLOTS) && (gBotGlobals.m_iNumBots < gBotGlobals.m_iMinBots))
				{
					int iNumPlayers = UTIL_GetNumClients(TRUE) + 1;
					int iBotsStillToJoin = gBotGlobals.m_iMinBots - gBotGlobals.m_iNumBots;
					int iNewSlotsFree = (gpGlobals->maxClients - iNumPlayers);
					// dont allow player to connect as the number of bots 
					// have not been reached yet.

					BotMessage(NULL, 0, "Player joining, min_bots not reached, Checking to disallow player...");
					BotMessage(NULL, 0, "numplayers to be = %d\nmin_bots = %d\nbots still to join = %d\nmax clients = %d\nnew slots = %d\nnum bots = %d", iNumPlayers,
						gBotGlobals.m_iMinBots,
						iBotsStillToJoin,
						gpGlobals->maxClients,
						iNewSlotsFree,
						gBotGlobals.m_iNumBots);

					if (iNewSlotsFree < iBotsStillToJoin)
					{
						BotMessage(NULL, 0, "Rejecting real player from joining as min_bots has not been reached yet...");

						strcpy(szRejectReason, "\nMaximum public slots reached (some slots are reserved)");

#ifdef RCBOT_META_BUILD
						RETURN_META_VALUE(MRES_SUPERCEDE, 0);
#else
						return FALSE;
#endif
					}
				}
			}
		}

		gBotGlobals.m_iJoiningClients[iIndex] = 1;
	}

#ifdef RCBOT_META_BUILD
	RETURN_META_VALUE(MRES_IGNORED, 0);
#else
	return (*other_gFunctionTable.pfnClientConnect)(pEntity, pszName, pszAddress, szRejectReason);
#endif
}

void ClientDisconnect(edict_t* pEntity)
{
	// Is the player that is disconnecting an RCbot?
	CBot* pBot = UTIL_GetBotPointer(pEntity);

	int iIndex = ENTINDEX(pEntity) - 1;

	if (EntityIsCommander(pEntity))
		gBotGlobals.SetCommander(NULL);

	if (iIndex < MAX_PLAYERS)
		gBotGlobals.m_iJoiningClients[iIndex] = 0;

	if (pBot)
	{
		//		FILE *fp;
		//		char szFilename[128];		
		//		int iNewProfile = 1;

		/*		if ( pBot->m_Profile.m_iProfileId == 0 ) // Bot doesn't have a Profile yet?
				{
					// Create a NEW Profile...

					// find a free id

					dataStack<int> s_iProfiles;

					UTIL_BuildFileName(szFilename,BOT_PROFILES_FILE,NULL);

					fp = fopen(szFilename,"r");

					if ( fp )
					{
						int iCurrentId = 0;

						while ( !feof(fp) )
						{
							fscanf(fp,"%d\n",&iCurrentId);

							s_iProfiles.Push(iCurrentId);
						}

						fclose(fp);
					}

					while ( iNewProfile <= MAX_BOT_ID )
					{
						if ( !s_iProfiles.IsMember(&iNewProfile) )
							break;
						else
							iNewProfile ++;
					}

					if ( iNewProfile <= MAX_BOT_ID )
					{
						fp = fopen(szFilename,"a"); // Open the "botprofiles.ini" add this profile to the list

						if ( fp )
						{
							fprintf(fp,"%d\n",iNewProfile);
							fclose(fp);
						}
						else
							BotMessage(NULL,0,"Error creating bot profile!");
					}
				}
				else
					iNewProfile = pBot->m_Profile.m_iProfileId;

				if ( iNewProfile <= MAX_BOT_ID )
				{
					char szBotProfile[8];

					sprintf(szBotProfile,"%d.ini",iNewProfile);

					UTIL_BuildFileName(szFilename,"botprofiles",szBotProfile);

					fp = fopen(szFilename,"w");

					if ( fp )
					{
						BotFunc_WriteProfile(fp,&pBot->m_Profile);

						fclose(fp);
					}
				}
				else
					BotMessage(NULL,0,"Error: Couldn't Create Bot Profile!");*/

		int iProfileId = pBot->m_Profile.m_iProfileId;
		int iTeam = pBot->m_Profile.m_iFavTeam;

		SaveHALBrainForPersonality(&pBot->m_Profile); // save this personality's HAL brain

		pBot->saveLearnedData();
		pBot->FreeLocalMemory();

		//if ( pBot->m_iRespawnState != RESPAWN_NEED_TO_REJOIN )
		pBot->m_iRespawnState = RESPAWN_NONE;

		pBot->Init();
		pBot->SetEdict(NULL);
		pBot->m_bIsUsed = FALSE;
		pBot->m_fKickTime = gpGlobals->time;
		pBot->m_bIsUsed = FALSE;

		pBot->m_Profile.m_iProfileId = iProfileId;
		pBot->m_Profile.m_iFavTeam = iTeam;

		if (!gBotGlobals.IsConfigSettingOn(BOT_CONFIG_REAL_MODE)) // NOT Realistic mode...
		{
			// Free this bots info and initiliases
			// we'll add a new random bot upon map 
			// change to cover this bots slot
		}
		else
		{
			// Keeps this bots info in memory and we'll decide on what the bot
			// wants to do after joining into the next map
			// The bot can then "see" what the next map is and dependant on
			// their favourite map + team they can join + their reputation 
			// with others joining the server and how they've done
			// on previous games will decide whether they want to leave or not...

			// Edict info will change so reset it.
		}
	}

	gBotGlobals.m_Clients.ClientDisconnected(pEntity);

#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*other_gFunctionTable.pfnClientDisconnect)(pEntity);
#endif
}

void ClientPutInServer(edict_t* pEntity)
{
	if (debug_engine) { FILE* fp; fp = fopen("bot.txt", "a"); fprintf(fp, "ClientPutInServer: %x\n", pEntity); fclose(fp); }

	gBotGlobals.m_Clients.ClientConnected(pEntity);

#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*other_gFunctionTable.pfnClientPutInServer)(pEntity);
#endif
}

void FreeArgs(const char* pcmd,
	const char* arg1,
	const char* arg2,
	const char* arg3,
	const char* arg4,
	const char* arg5)
{
	// free the dupliacted strings when a fake client command
	// is made
	if (gBotGlobals.m_bIsFakeClientCommand)
	{
		if (pcmd)
			free((void*)pcmd);
		if (arg1)
			free((void*)arg1);
		if (arg2)
			free((void*)arg2);
		if (arg3)
			free((void*)arg3);
		if (arg4)
			free((void*)arg4);
		if (arg5)
			free((void*)arg5);
	}
}

void BotFunc_TraceToss(edict_t* ent, edict_t* ignore, TraceResult* tr)
{
	edict_t tempent = *ent;
	Vector gravity = Vector(0, 0, CVAR_GET_FLOAT("sv_gravity"));

	for (int i = 0; i < 4; i++)
	{
		Vector nextvel = tempent.v.velocity + (tempent.v.velocity - gravity) * 0.5;//Vector(0,0,(CVAR_GET_FLOAT("sv_gravity")/tempent.v.velocity.Length()));
		Vector nextstep = tempent.v.origin + nextvel;

		UTIL_TraceLine(tempent.v.origin, nextstep, dont_ignore_monsters, dont_ignore_glass, ignore, tr);

		//WaypointDrawBeam(gBotGlobals.m_Clients.GetClientByIndex(0)->GetPlayer(),tempent.v.origin,tr->vecEndPos,16,0,255,255,255,200,10);

		tempent.v.origin = tr->vecEndPos;
		tempent.v.velocity = nextvel;
	}

	// final draw to end
	UTIL_TraceLine(tempent.v.origin, tempent.v.origin + (tempent.v.velocity * 256), dont_ignore_monsters, dont_ignore_glass, ignore, tr);
	//WaypointDrawBeam(gBotGlobals.m_Clients.GetClientByIndex(0)->GetPlayer(),tempent.v.origin,tr->vecEndPos,16,0,255,255,255,200,10);
}

///////////////////////////////////////////////////////////////////////////
void ClientCommand(edict_t* pEntity)
{
	const char* pcmd;
	const char* arg1;
	const char* arg2;
	const char* arg3;
	const char* arg4;
	const char* arg5;

	if (!gBotGlobals.m_bIsFakeClientCommand)
	{
		pcmd = CMD_ARGV(0);
		arg1 = CMD_ARGV(1);
		arg2 = CMD_ARGV(2);
		arg3 = CMD_ARGV(3);
		arg4 = CMD_ARGV(4);
		arg5 = CMD_ARGV(5);
	}
	else
	{
		pcmd = strdup(pfnCmd_Argv(0));
		arg1 = strdup(pfnCmd_Argv(1));
		arg2 = strdup(pfnCmd_Argv(2));
		arg3 = strdup(pfnCmd_Argv(3));
		arg4 = strdup(pfnCmd_Argv(4));
		arg5 = strdup(pfnCmd_Argv(5));
	}

	eBotCvarState iState = BOT_CVAR_CONTINUE;
	int iAccessLevel = 0;
	BOOL bSayTeamMsg = FALSE;
	BOOL bSayMsg = FALSE;

	CClient* pClient;

	pClient = gBotGlobals.m_Clients.GetClientByEdict(pEntity);

	if (pClient)
		iAccessLevel = pClient->GetAccessLevel();

	/// available for all :p :)
	if (FStrEq(pcmd, "get_rcbot_ver"))
	{
		BotMessage(pEntity, 0, "Bot version is : %s build %s-%s", BOT_VER, __DATE__, __TIME__);

		iState = BOT_CVAR_ACCESSED;
	}
	// someone said something
	else if (((bSayMsg = FStrEq(pcmd, "say")) == TRUE) || ((bSayTeamMsg = FStrEq(pcmd, "say_team")) == TRUE))
	{
		BOOL bMadeSquad = FALSE;

		if (bSayTeamMsg)
		{
			// player wants to lead squad?
			if ((strncmp(arg1, "form", 4) == 0) &&
				((strncmp(&arg1[4], " up", 3) == 0) || (strncmp(arg2, "up", 2) == 0)))
			{
				// loop through bots in team
				BotFunc_MakeSquad(pClient);
				bMadeSquad = TRUE;
			}
		}

		// not trying to make a squad (i.e. dont reply to it)...
		if (!bMadeSquad)
		{
			///////
			// see if bot can learn its HAL brain from this person speaking
			BOOL bSenderIsBot = (UTIL_GetBotPointer(pEntity) != NULL);

			if (!bSenderIsBot || gBotGlobals.IsConfigSettingOn(BOT_CONFIG_CHAT_REPLY_TO_BOTS))
			{
				// team only message?
				int iTeamOnly = (int)bSayTeamMsg;

				char* szMessage = NULL;
				//				char *szTempArgument;

				if (arg2 && *arg2)
				{
					// argh! someone said something in series of arguments. work out the message
					int i = 1;
					int iLenSoFar = 0;
					const char* szArgument;
					// for concatenating string dynamically
					char* szTemp = NULL;
					BOOL bIsQuote;
					BOOL bWasQuote = FALSE;

					const char* (*CmdArgv_func)(int);
					int iArgCount;

					if (bSenderIsBot)
					{
						CmdArgv_func = pfnCmd_Argv;
						iArgCount = pfnCmd_Argc();
					}
					else
					{
						CmdArgv_func = CMD_ARGV;
						iArgCount = CMD_ARGC();
					}

					while (i < iArgCount)
					{
						szArgument = CmdArgv_func(i);

						if (!szArgument || !*szArgument)
						{
							i++;
							continue;
						}

						iLenSoFar += strlen(szArgument) + 1;

						// read a string already
						if (szMessage)
						{
							szTemp = strdup(szMessage);
							free(szMessage);
							szMessage = NULL;
						}

						// 2 extra chars, 1 for terminator and 1 for space
						szMessage = new char[iLenSoFar + 1];
						szMessage[0] = 0;

						// copy old string
						if (szTemp)
						{
							bIsQuote = FALSE;

							// if not a bot sending message, then the ' quotes can seperate words
							// so can spaces argh :-@
							if (!bSenderIsBot)
							{
								bIsQuote = (szArgument[0] == '\'') && (szArgument[1] == 0);
							}
							if (bIsQuote || bWasQuote)
							{
								sprintf(szMessage, "%s%s", szTemp, szArgument);

								bWasQuote = !bWasQuote;
							}
							else
							{
								// take space into count (thats what is seperating these words)
								sprintf(szMessage, "%s %s", szTemp, szArgument);
							}

							free(szTemp);
							szTemp = NULL;
						}
						else
						{
							strcat(szMessage, szArgument);
						}
						szMessage[iLenSoFar] = 0;

						i++;

					}
				}
				else
					szMessage = strdup(arg1);

				if (szMessage)
				{

					int i;
					CBot* pBot;

					for (i = 0; i < MAX_PLAYERS; i++)
					{
						pBot = &gBotGlobals.m_Bots[i];

						if (pBot && pBot->IsUsed())
						{
							// say message, everyone can see, team message only team mates can see
							if (bSayMsg || (bSayTeamMsg && (UTIL_GetTeam(pEntity) == pBot->GetTeam())))
							{
								pBot->ReplyToMessage(szMessage, pEntity, iTeamOnly);
							}
						}
					}

					// finished with message
					free(szMessage);

					szMessage = NULL;

				}
			}
		}
	}
	// Let the menuselect command go through
	// make sure the command has "rcbot" in front to call bot commands
	// and make sure the listen server edict will use the ServerCommand function instead
	// of this.
	else if (FStrEq(pcmd, "menuselect"))
	{
		gBotGlobals.m_CurrentHandledCvar = gBotGlobals.m_BotCvars.GetCvar(pcmd);

		if (gBotGlobals.m_CurrentHandledCvar)
		{
			if (pClient == NULL)
			{
				BugMessage(pEntity, "Your client was not found in list of clients");

				FreeArgs(pcmd, arg1, arg2, arg3, arg4, arg5);

#ifdef RCBOT_META_BUILD
				RETURN_META(MRES_SUPERCEDE);
#else
				return;
#endif
			}

			iState = gBotGlobals.m_CurrentHandledCvar->action(pClient, arg1, arg2, arg3, arg4);
		}
	}
	//////////////////////////////////////////
	// little force grip thing, a bit messy!!!
	else if (FStrEq(pcmd, "+rcbot_force_grip"))
	{
		if (iAccessLevel < RCBOT_ACCESS_FORCE_GRIP)
			iState = BOT_CVAR_NEEDACCESS;
		else
		{
			pClient->m_pForceGripEntity = UTIL_FacingEnt(pClient->GetPlayer());

			iState = BOT_CVAR_ACCESSED;
		}
	}
	else if (FStrEq(pcmd, "-rcbot_force_grip"))
	{
		if (iAccessLevel < RCBOT_ACCESS_FORCE_GRIP)
			iState = BOT_CVAR_NEEDACCESS;
		else
		{
			if (pClient->m_pForceGripEntity)
			{
				pClient->m_pForceGripEntity->v.flags &= ~FL_FROZEN;

				DROP_TO_FLOOR(pClient->m_pForceGripEntity);
			}

			pClient->m_pForceGripEntity = NULL;

			iState = BOT_CVAR_ACCESSED;
		}
	}
	else if (FStrEq(pcmd, BOT_COMMAND_ACCESS) &&
		((!IS_DEDICATED_SERVER() && (pEntity != gBotGlobals.m_pListenServerEdict)) ||
			IS_DEDICATED_SERVER()))
	{
		gBotGlobals.m_CurrentHandledCvar = gBotGlobals.m_BotCvars.GetCvar(arg1);

		if (gBotGlobals.m_CurrentHandledCvar)
		{
			if (pClient == NULL)
			{
				BugMessage(pEntity, "Your client was not found in list of clients");

				FreeArgs(pcmd, arg1, arg2, arg3, arg4, arg5);
#ifdef RCBOT_META_BUILD
				RETURN_META(MRES_SUPERCEDE);
#else
				return;
#endif
			}

			//if ( pEntity == gBotGlobals.m_pListenServerEdict )
			//	iAccessLevel = 10;
			//else			
			// GET ACCESS LEVEL FROM CLIENTS

			if (gBotGlobals.m_CurrentHandledCvar->needAccess(iAccessLevel))
				iState = BOT_CVAR_NEEDACCESS;
			else
				iState = gBotGlobals.m_CurrentHandledCvar->action(pClient, arg2, arg3, arg4, arg5);
		}
		else
			iState = BOT_CVAR_NOTEXIST;
	}

	FreeArgs(pcmd, arg1, arg2, arg3, arg4, arg5);

	if ((iState == BOT_CVAR_ERROR) || (iState == BOT_CVAR_ACCESSED) || (iState == BOT_CVAR_NEEDACCESS) || (iState == BOT_CVAR_NOTEXIST))
	{
		if (iState == BOT_CVAR_NEEDACCESS)
		{
			BotMessage(pEntity, 0, "You do not have access to this command...");

			if (pClient)
			{
				if (!pClient->HasToolTipSent(BOT_TOOL_TIP_ADMIN_HELP))
				{
					gBotGlobals.SayToolTip(pEntity, BOT_TOOL_TIP_ADMIN_HELP);
				}
			}
		}
		else if (iState == BOT_CVAR_ERROR)
			BotMessage(pEntity, 0, "Error accessing command...");
		else if (iState == BOT_CVAR_NOTEXIST)
			BotMessage(pEntity, 0, "No command entered or bot command does not exist!");

#ifdef RCBOT_META_BUILD
		RETURN_META(MRES_SUPERCEDE);
#else
		return;
#endif

	}

	if ((pcmd[0] != 0) && (strstr(pcmd, "addbot") != NULL))
	{
		BotMessage(pEntity, 0, "Tip: If you want to add an RCBOT use the command \"rcbot addbot\"\n");
	}

#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*other_gFunctionTable.pfnClientCommand)(pEntity);
#endif
}

void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax)
{
	memset(gBotGlobals.m_iJoiningClients, 0, sizeof(int) * MAX_PLAYERS);

	//	gBotGlobals.m_iJoiningClients = 0;
#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*other_gFunctionTable.pfnServerActivate)(pEdictList, edictCount, clientMax);
#endif
}
///////////////////////////////////////////////////////////////////////////
void ServerDeactivate(void)
{
	// server has finished (map changed for example) but new map
	// hasn't loaded yet!!

	int iIndex;
	CBot* pBot;

	// free our memory, I call it local meaning not everywhere, 
	// since we are only changing maps, not quitting the game 
	// usually when this function is called...
	gBotGlobals.FreeLocalMemory();

	gBotGlobals.m_bNetMessageStarted = FALSE;
	gBotGlobals.m_fBotRejoinTime = 0;
	gBotGlobals.m_fClientUpdateTime = 0;
	gBotGlobals.m_bBotCanRejoin = FALSE;
	gBotGlobals.m_fReadConfigTime = 0;
	gBotGlobals.m_Squads.FreeMemory();
	gBotGlobals.m_fNextJoinTeam = 0;

	gBotGlobals.saveLearnedData();

	// mark the bots as needing to be rejoined next game...
	for (iIndex = 0; iIndex < MAX_PLAYERS; iIndex++)
	{
		pBot = &gBotGlobals.m_Bots[iIndex];

		// Respawn left game when bot wants to leave game
		// initialize bot so that it wont re-use the profile (hopefully)
		if (pBot->m_iRespawnState == RESPAWN_LEFT_GAME)
		{
			pBot->Init();
			continue;
		}

		// took a very short time for bots to be kicked from server
		// when server was deactivated
		// assume that bots were just kicked because the server changed map
		// and let them reconnect upon map change.
		if ((pBot->m_fKickTime + 0.5) >= gpGlobals->time)
			pBot->m_iRespawnState = RESPAWN_NEED_TO_REJOIN;
	}

	WaypointVisibility.ClearVisibilityTable();

	gBotGlobals.m_iNumBots = 0;
#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*other_gFunctionTable.pfnServerDeactivate)();
#endif
}

void StartFrame(void)
{
	gBotGlobals.StartFrame();

#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else

	(*other_gFunctionTable.pfnStartFrame)();

#endif

}

//
// engine hooks
//

void pfnChangeLevel(char* s1, char* s2)
{
	if (debug_engine) { fp = fopen("bot.txt", "a"); fprintf(fp, "pfnChangeLevel:\n"); fclose(fp); }

	CBot* pBot;

	// kick any bot off of the server after time/frag limit...
	for (int index = 0; index < MAX_PLAYERS; index++)
	{
		pBot = &gBotGlobals.m_Bots[index];

		if (pBot->m_bIsUsed)  // is this slot used?
		{
			char cmd[40];

			sprintf(cmd, "kick \"%s\"\n", pBot->m_szBotName);

			pBot->m_iRespawnState = RESPAWN_NEED_TO_RESPAWN;

			SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
		}
	}

#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else   
	(*g_engfuncs.pfnChangeLevel)(s1, s2);
#endif
}

void pfnEmitSound(edict_t* entity, int channel, const char* sample, /*int*/float volume, float attenuation, int fFlags, int pitch)
{
	if (entity != NULL)
	{
		int i;
		CBot* pBot;

		Vector vOrigin;

		vOrigin = EntityOrigin(entity);

		eSoundType iSound = SOUND_UNKNOWN;

		if (gBotGlobals.IsMod(MOD_SVENCOOP))
		{
			if (entity->v.flags & FL_CLIENT)
			{
				if ((strncmp(sample, "speech/saveme", 13) == 0))
					iSound = SOUND_NEEDHEALTH;
				else if ((strncmp(sample, "speech/grenade", 14) == 0))
					iSound = SOUND_TAKE_COVER;
			}
		}
		else if (gBotGlobals.IsMod(MOD_TFC))
		{
			if (strncmp(sample, "speech/saveme", 13) == 0)
				iSound = SOUND_NEEDHEALTH;
		}

		if (iSound == SOUND_UNKNOWN)
		{
			// common sounds, like doors etc.
			if ((sample[0] == 'd') && !strncmp(sample, "doors/", 6))
				iSound = SOUND_DOOR;
			else if ((sample[0] == 'p') && !strncmp(sample, "plats/", 6))
				iSound = SOUND_DOOR;
			else if ((sample[0] == 'w') && !strncmp(sample, "weapons/", 8))
				iSound = SOUND_WEAPON;
			else if ((sample[0] == 'p') && !strncmp(sample, "player/", 7))
			{
				if (strncmp(&sample[7], "pain", 4) == 0)
					iSound = SOUND_PLAYER_PAIN;
				else
					iSound = SOUND_PLAYER;
			}
			else if ((sample[0] == 'b') && !strncmp(sample, "buttons/", 8))
				iSound = SOUND_BUTTON;
		}

		if (iSound == SOUND_UNKNOWN)
		{
			switch (gBotGlobals.m_iCurrentMod)
			{
			case MOD_NS:
			{
				//NS sounds like gorges / taunts/ radio etc
				int sample_num = 0;

				// Starts with "vox/" ?? (Do it manually...)
				if ((sample[0] == 'v') && (sample[1] == 'o') &&
					(sample[2] == 'x') && (sample[3] == '/'))
				{
					if (!strncmp(&sample[4], "ssay", 4)) // Marine Said Something
					{
						sample_num = atoi(&sample[8]);

						if ((sample_num > 10) && (sample_num < 20))
							iSound = SOUND_FOLLOW;
						else if (sample_num < 30)
							iSound = SOUND_COVERING;
						else if (sample_num < 40)
							iSound = SOUND_TAUNT;
						else if (sample_num < 50)
							iSound = SOUND_NEEDHEALTH;
						else if (sample_num < 60)
							iSound = SOUND_NEEDAMMO;
						else if (sample_num < 70)
							iSound = SOUND_INPOSITION;
						else if (sample_num < 80)
							iSound = SOUND_INCOMING;
						else if (sample_num < 90)
							iSound = SOUND_MOVEOUT;
						else if (sample_num < 100)
							iSound = SOUND_ALLCLEAR;
					}
					else if (!strncmp(&sample[4], "asay", 4)) // Alien Sound?
					{
						sample_num = atoi(&sample[8]);

						if (sample_num < 20)
							iSound = SOUND_UNKNOWN;

						if (sample_num < 30) // healing sounds end in ..21, ..22.wav etc			
							iSound = SOUND_NEEDHEALTH;
						else if (sample_num < 40)
							iSound = SOUND_NEEDBACKUP;
						else if (sample_num < 50)
							iSound = SOUND_INCOMING;
						else if (sample_num < 60)
							iSound = SOUND_ATTACK;
						else if (sample_num < 70)
							iSound = SOUND_BUILDINGHERE;
					}
				}
			}
			break;
			case MOD_BUMPERCARS:
				// bumpercars sounds, like horns, taunts etc
				break;
			default:
				break;
			}
		}

		edict_t* pEntityOwner = entity->v.owner;

		for (i = 0; i < 32; i++)
		{
			pBot = &gBotGlobals.m_Bots[i];

			if (pBot == NULL)
				continue;
			if (pBot->m_pEdict == NULL)
				continue;
			if (pBot->m_pEdict == pEntityOwner)
				continue;
			if (!pBot->m_bIsUsed)
				continue;
			if (pBot->m_pEdict == entity)
				continue;
			if (pBot->m_pEnemy != NULL)
				continue;

			if (pBot->DistanceFrom(vOrigin) < BOT_HEAR_DISTANCE)
				pBot->HearSound(iSound, vOrigin, entity);
		}

	}

	if (debug_engine) { fp = fopen("bot.txt", "a"); fprintf(fp, "pfnEmitSound:\n"); fclose(fp); }
#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else   
	(*g_engfuncs.pfnEmitSound)(entity, channel, sample, volume, attenuation, fFlags, pitch);
#endif
}

void pfnMessageBegin(int msg_dest, int msg_type, const float* pOrigin, edict_t* ed)
{
	BOOL no_error = gBotGlobals.NetMessageStarted(msg_dest, msg_type, pOrigin, ed);

#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	//if ( no_error )
	(*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ed);
#endif
}
void pfnMessageEnd(void)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp = fopen("bot.txt", "a"); fprintf(fp, "pfnMessageEnd:\n"); fclose(fp); }

		if (gBotGlobals.m_CurrentMessage)
		{
			if (gBotGlobals.m_pDebugMessage == gBotGlobals.m_CurrentMessage)//gBotGlobals.IsDebugLevelOn(BOT_DEBUG_MESSAGE_LEVEL) )
				ALERT(at_console, "---------MESSAGE_END(\"%s\")-------\n", gBotGlobals.m_CurrentMessage->getMessageName());

			if (gBotGlobals.m_CurrentMessage->isStateMsg())
				((CBotStatedNetMessage*)gBotGlobals.m_CurrentMessage)->messageEnd();
			else
				gBotGlobals.m_CurrentMessage->execute(NULL, gBotGlobals.m_iBotMsgIndex);  // NULL indicated msg end
		}
	}

	// clear out the bot message function pointers...

	gBotGlobals.m_CurrentMessage = NULL;
	gBotGlobals.m_iCurrentMessageState = 0;
	gBotGlobals.m_iCurrentMessageState2 = 0;

	gBotGlobals.m_bNetMessageStarted = FALSE;

#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*g_engfuncs.pfnMessageEnd)();
#endif
}
void pfnWriteByte(int iValue)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp = fopen("bot.txt", "a"); fprintf(fp, "pfnWriteByte: %d\n", iValue); fclose(fp); }

		// if this message is for a bot, call the client message function...
		if (gBotGlobals.m_CurrentMessage)
		{
			if (gBotGlobals.m_pDebugMessage == gBotGlobals.m_CurrentMessage)//gBotGlobals.IsDebugLevelOn(BOT_DEBUG_MESSAGE_LEVEL) )
				ALERT(at_console, "WRITE_BYTE(%d)\n", iValue);

			if (gBotGlobals.m_CurrentMessage->isStateMsg())
				((CBotStatedNetMessage*)gBotGlobals.m_CurrentMessage)->writeByte(iValue);
			else
				gBotGlobals.m_CurrentMessage->execute((void*)&iValue, gBotGlobals.m_iBotMsgIndex);
		}
	}
#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*g_engfuncs.pfnWriteByte)(iValue);
#endif
}
void pfnWriteChar(int iValue)
{
	if (gpGlobals->deathmatch)
	{
		//if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnWriteChar: %d\n",iValue); fclose(fp); }

		// if this message is for a bot, call the client message function...
		if (gBotGlobals.m_CurrentMessage)
		{
			if (gBotGlobals.m_pDebugMessage == gBotGlobals.m_CurrentMessage)//gBotGlobals.IsDebugLevelOn(BOT_DEBUG_MESSAGE_LEVEL) )
				ALERT(at_console, "WRITE_CHAR(%c)\n", (char)iValue);

			if (gBotGlobals.m_CurrentMessage->isStateMsg())
				((CBotStatedNetMessage*)gBotGlobals.m_CurrentMessage)->writeChar((char)iValue);
			else
				gBotGlobals.m_CurrentMessage->execute((void*)&iValue, gBotGlobals.m_iBotMsgIndex);
		}
	}
#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*g_engfuncs.pfnWriteChar)(iValue);
#endif
}
void pfnWriteShort(int iValue)
{
	if (gpGlobals->deathmatch)
	{

		if (debug_engine) { fp = fopen("bot.txt", "a"); fprintf(fp, "pfnWriteShort: %d\n", iValue); fclose(fp); }

		// if this message is for a bot, call the client message function...
		if (gBotGlobals.m_CurrentMessage)
		{
			if (gBotGlobals.m_pDebugMessage == gBotGlobals.m_CurrentMessage)//gBotGlobals.IsDebugLevelOn(BOT_DEBUG_MESSAGE_LEVEL) )
				ALERT(at_console, "WRITE_SHORT(%d)\n", iValue);

			if (gBotGlobals.m_CurrentMessage->isStateMsg())
				((CBotStatedNetMessage*)gBotGlobals.m_CurrentMessage)->writeShort(iValue);
			else
				gBotGlobals.m_CurrentMessage->execute((void*)&iValue, gBotGlobals.m_iBotMsgIndex);
		}
	}
#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*g_engfuncs.pfnWriteShort)(iValue);
#endif
}
void pfnWriteLong(int iValue)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp = fopen("bot.txt", "a"); fprintf(fp, "pfnWriteLong: %d\n", iValue); fclose(fp); }

		// if this message is for a bot, call the client message function...
		if (gBotGlobals.m_CurrentMessage)
		{
			if (gBotGlobals.m_pDebugMessage == gBotGlobals.m_CurrentMessage)//gBotGlobals.IsDebugLevelOn(BOT_DEBUG_MESSAGE_LEVEL) )
				ALERT(at_console, "WRITE_LONG(%d)\n", iValue);

			if (gBotGlobals.m_CurrentMessage->isStateMsg())
				((CBotStatedNetMessage*)gBotGlobals.m_CurrentMessage)->writeLong(iValue);
			else
				gBotGlobals.m_CurrentMessage->execute((void*)&iValue, gBotGlobals.m_iBotMsgIndex);
		}
	}
#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*g_engfuncs.pfnWriteLong)(iValue);
#endif
}
void pfnWriteAngle(float flValue)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp = fopen("bot.txt", "a"); fprintf(fp, "pfnWriteAngle: %f\n", flValue); fclose(fp); }

		// if this message is for a bot, call the client message function...
		if (gBotGlobals.m_CurrentMessage)
		{
			if (gBotGlobals.m_pDebugMessage == gBotGlobals.m_CurrentMessage)//gBotGlobals.IsDebugLevelOn(BOT_DEBUG_MESSAGE_LEVEL) )
				ALERT(at_console, "WRITE_ANGLE(%0.3f)\n", flValue);

			if (gBotGlobals.m_CurrentMessage->isStateMsg())
				((CBotStatedNetMessage*)gBotGlobals.m_CurrentMessage)->writeAngle(flValue);
			else
				gBotGlobals.m_CurrentMessage->execute((void*)&flValue, gBotGlobals.m_iBotMsgIndex);
		}
	}
#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*g_engfuncs.pfnWriteAngle)(flValue);
#endif
}
void pfnWriteCoord(float flValue)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp = fopen("bot.txt", "a"); fprintf(fp, "pfnWriteCoord: %f\n", flValue); fclose(fp); }

		// if this message is for a bot, call the client message function...
		if (gBotGlobals.m_CurrentMessage)
		{
			if (gBotGlobals.m_pDebugMessage == gBotGlobals.m_CurrentMessage)//gBotGlobals.IsDebugLevelOn(BOT_DEBUG_MESSAGE_LEVEL) )
				ALERT(at_console, "WRITE_COORD(%0.3f)\n", flValue);

			if (gBotGlobals.m_CurrentMessage->isStateMsg())
				((CBotStatedNetMessage*)gBotGlobals.m_CurrentMessage)->writeCoord(flValue);
			else
				gBotGlobals.m_CurrentMessage->execute((void*)&flValue, gBotGlobals.m_iBotMsgIndex);
		}
	}
#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else   
	(*g_engfuncs.pfnWriteCoord)(flValue);
#endif
}
void pfnWriteString(const char* sz)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp = fopen("bot.txt", "a"); fprintf(fp, "pfnWriteString: %s\n", sz); fclose(fp); }

		// if this message is for a bot, call the client message function...
		if (gBotGlobals.m_CurrentMessage)
		{
			if (gBotGlobals.m_pDebugMessage == gBotGlobals.m_CurrentMessage)//gBotGlobals.IsDebugLevelOn(BOT_DEBUG_MESSAGE_LEVEL) )
				ALERT(at_console, "WRITE_STRING(%s)\n", sz);

			if (gBotGlobals.m_CurrentMessage->isStateMsg())
				((CBotStatedNetMessage*)gBotGlobals.m_CurrentMessage)->writeString(sz);
			else
				gBotGlobals.m_CurrentMessage->execute((void*)sz, gBotGlobals.m_iBotMsgIndex);
		}
	}
#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*g_engfuncs.pfnWriteString)(sz);
#endif
}
void pfnWriteEntity(int iValue)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp = fopen("bot.txt", "a"); fprintf(fp, "pfnWriteEntity: %d\n", iValue); fclose(fp); }

		// if this message is for a bot, call the client message function...
		if (gBotGlobals.m_CurrentMessage)
		{
			if (gBotGlobals.m_pDebugMessage == gBotGlobals.m_CurrentMessage)//gBotGlobals.IsDebugLevelOn(BOT_DEBUG_MESSAGE_LEVEL) )
				ALERT(at_console, "WRITE_ENTITY(%d)\n", iValue);

			if (gBotGlobals.m_CurrentMessage->isStateMsg())
				((CBotStatedNetMessage*)gBotGlobals.m_CurrentMessage)->writeEntity(INDEXENT(iValue));
			else
				gBotGlobals.m_CurrentMessage->execute((void*)&iValue, gBotGlobals.m_iBotMsgIndex);
		}
	}
#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*g_engfuncs.pfnWriteEntity)(iValue);
#endif
}

int pfnRegUserMsg(const char* pszName, int iSize)
{
	int msg = 0;

#ifdef RCBOT_META_BUILD

	extern plugin_info_t Plugin_info;

	//msg = GET_USER_MSG_ID(&Plugin_info, pszName, &iSize);
#else

	msg = (*g_engfuncs.pfnRegUserMsg)(pszName, iSize);

	if (gpGlobals->deathmatch)
	{
#ifdef _DEBUG
		fp = fopen("bot.txt", "a"); fprintf(fp, "pfnRegUserMsg: pszName=%s msg=%d\n", pszName, msg); fclose(fp);
#endif

	}
#endif

	gBotGlobals.m_NetEntityMessages.UpdateMessage(pszName, msg, iSize);
	gBotGlobals.m_NetAllMessages.UpdateMessage(pszName, msg, iSize);

#ifdef RCBOT_META_BUILD
	RETURN_META_VALUE(MRES_IGNORED, 0);
#else
	return msg;
#endif

}

void pfnSetClientMaxspeed(const edict_t* pEdict, float fNewMaxspeed)
{
	// Is this player a bot?
	CBot* pBot = UTIL_GetBotPointer(pEdict);

	if (pBot)
	{
		pBot->m_fMaxSpeed = fNewMaxspeed;
	}

	if (debug_engine) { fp = fopen("bot.txt", "a"); fprintf(fp, "pfnSetClientMaxspeed: edict=%x %f\n", pEdict, fNewMaxspeed); fclose(fp); }
#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*g_engfuncs.pfnSetClientMaxspeed)(pEdict, fNewMaxspeed);
#endif
}

void pfnSetClientKeyValue(int clientIndex, char* infobuffer, char* key, char* value)
{
	edict_t* pEdict = INDEXENT(clientIndex);

	// Copy a players reputation info to a new name
	if (strcmp(key, "name") == 0)
	{
		if (pEdict)
		{
			CBot* pBot;
			CBotReputation* pRep;
			CBotReputations* pRepList;
			int i;

			int iOldPlayerRepId = GetPlayerEdictRepId(pEdict);

			if (iOldPlayerRepId != -1) // otherwise : error...
			{
				for (i = 0; i < MAX_PLAYERS; i++)
				{
					pBot = &gBotGlobals.m_Bots[i];

					if (pBot && (pBot->m_iRespawnState == RESPAWN_IDLE))
					{
						if (pBot->m_pEdict && pBot->m_bIsUsed)
						{
							if (pBot->m_pEdict == pEdict)
								continue;

							pRepList = &pBot->m_Profile.m_Rep;

							if ((pRep = pBot->m_Profile.m_Rep.GetRep(iOldPlayerRepId)) != NULL)
							{
								// New name = value

								int iNewPlayerRepId = GetPlayerRepId(value);

								if (pBot->m_Profile.m_Rep.GetRep(iNewPlayerRepId) == NULL)
									pRepList->AddRep(iNewPlayerRepId, pRep->CurrentRep());
							}
							else
							{
								//	ERROR!

							}
						}
					}
				}
			}

			int iFlags = pEdict->v.flags;

			if ((iFlags & FL_CLIENT) && !(iFlags & FL_FAKECLIENT))
			{
				CClient* pClient = gBotGlobals.m_Clients.GetClientByEdict(pEdict);

				if (pClient)
				{
					pClient->m_bRecheckAuth = TRUE;
				}
			}

		}

	}

	if (debug_engine) { fp = fopen("bot.txt", "a"); fprintf(fp, "pfnSetClientKeyValue: %s %s\n", key, value); fclose(fp); }
#ifdef RCBOT_META_BUILD
	RETURN_META(MRES_IGNORED);
#else
	(*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, key, value);
#endif
}

const char* pfnGetPlayerAuthId(edict_t* e)
{
	// TODO: not needed for sven, but for other games? -w00t
	static const char* BOT_STEAM_ID = "BOT";
	BOOL bIsBot = (UTIL_GetBotPointer(e) != NULL);
#ifdef RCBOT_META_BUILD

	if (bIsBot)
		RETURN_META_VALUE(MRES_SUPERCEDE, BOT_STEAM_ID);

	RETURN_META_VALUE(MRES_IGNORED, NULL);
#else

	if (bIsBot)
		return BOT_STEAM_ID;

	return (*g_engfuncs.pfnGetPlayerAuthId)(e);
#endif
}

void PluginInit() {
    CVAR_REGISTER(&bot_ver_cvar);

    REG_SVR_COMMAND(BOT_COMMAND_ACCESS, RCBot_ServerCommand);

    // Do these at start
    gBotGlobals.Init();
    gBotGlobals.GetModInfo();

    SetupMenus();

    //gBotGlobals.GameInit();

    g_argv = (char*)malloc(1024);

	g_dll_hooks.pfnSpawn = DispatchSpawn;
	g_dll_hooks.pfnTouch = DispatchTouch;
	g_dll_hooks.pfnBlocked = DispatchBlocked;
	g_dll_hooks.pfnKeyValue = DispatchKeyValue;
	g_dll_hooks.pfnClientConnect = ClientConnect;
	g_dll_hooks.pfnClientDisconnect = ClientDisconnect;
	g_dll_hooks.pfnClientPutInServer = ClientPutInServer;
	g_dll_hooks.pfnClientCommand = ClientCommand;
	g_dll_hooks.pfnServerActivate = ServerActivate;
	g_dll_hooks.pfnServerDeactivate = ServerDeactivate;
	g_dll_hooks.pfnStartFrame = StartFrame;

	g_engine_hooks.pfnChangeLevel = pfnChangeLevel;
	g_engine_hooks.pfnEmitSound = pfnEmitSound;
	g_engine_hooks.pfnRegUserMsg = pfnRegUserMsg;
	g_engine_hooks.pfnSetClientMaxspeed = pfnSetClientMaxspeed;
	g_engine_hooks.pfnSetClientKeyValue = pfnSetClientKeyValue;
	g_engine_hooks.pfnGetPlayerAuthId = pfnGetPlayerAuthId;
	g_engine_hooks.pfnCmd_Argc = pfnCmd_Argc;
	g_engine_hooks.pfnCmd_Argv = pfnCmd_Argv;
	g_engine_hooks.pfnCmd_Args = pfnCmd_Args;

	g_engine_hooks.pfnMessageBegin = pfnMessageBegin;
	g_engine_hooks.pfnWriteAngle = pfnWriteAngle;
	g_engine_hooks.pfnWriteByte = pfnWriteByte;
	g_engine_hooks.pfnWriteChar = pfnWriteChar;
	g_engine_hooks.pfnWriteCoord = pfnWriteCoord;
	g_engine_hooks.pfnWriteEntity = pfnWriteEntity;
	g_engine_hooks.pfnWriteLong = pfnWriteLong;
	g_engine_hooks.pfnWriteShort = pfnWriteShort;
	g_engine_hooks.pfnWriteString = pfnWriteString;
	g_engine_hooks.pfnMessageEnd = pfnMessageEnd;
}

void PluginExit() {
    gBotGlobals.FreeGlobalMemory();
}