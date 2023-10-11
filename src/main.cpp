#include "mmlib.h"
#include "bot_const.h"
#include "bot.h"

using namespace std;

#pragma comment(linker, "/EXPORT:GiveFnptrsToDll=_GiveFnptrsToDll@8")
#pragma comment(linker, "/SECTION:.data,RW")

extern cvar_t bot_ver_cvar;

int debug_engine;
char* g_argv;

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


void PluginInit() {  
    CVAR_REGISTER(&bot_ver_cvar);

    REG_SVR_COMMAND(BOT_COMMAND_ACCESS, RCBot_ServerCommand);

    // Do these at start
    gBotGlobals.Init();
    gBotGlobals.GetModInfo();

    SetupMenus();

    //gBotGlobals.GameInit();

    g_argv = (char*)malloc(1024);
}



void PluginExit() {
    gBotGlobals.FreeGlobalMemory();
}