/*
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from botman's template 3.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */
 /*


  MegaHAL Stuff

  code from PM's RACC Bot ( http://racc.bots-united.com ) Check it out!
  adapted to rcbot code

  MegaHAL - http://megahal.sourceforge.net

  */

// RACC - AI development project for first-person shooter games derivated from Valve's Half-Life
// (http://www.racc-ai.com/)
//
// The game to engine interfacing code is based on the work done by Jeffrey 'Botman' Broome
// (http://planethalflife.com/botman/)
//
// This project is partially based on the work done by Eric Bieschke in his BSDbot
// (http://gamershomepage.com/csbot/)
//
// This project is partially based on the work done by Brendan 'Spyro' McCarthy in his ODD Bot
// (http://oddbot.hlfusion.com/)
//
// This project is partially based on the work done by Alistair 'eLiTe' Stewart in his TEAMbot
// (http://www.planethalflife.com/teambot/)
//
// This project is partially based on the work done by Johannes '@$3.1415rin' Lampel in his JoeBot
// (http://www.joebot.net/)
//
// Rational Autonomous Cybernetic Commandos AI
//
// bot_chat.cpp
//

#ifdef HLCOOP_BUILD
#include "hlcoop.h"
#else
#include "mmlib.h"
#include <unordered_map>
#endif

#include "bot_const.h"
#include "bot.h"

#include "bot_chat.h"

extern char *g_argv;
extern CBotGlobals gBotGlobals;

char *name_in_msg = "%n";

/* anonym001 */
#ifndef min
#define min(x,y) (((x) <= (y)) ? (x) : (y))
#endif
#ifndef max
#define max(x,y) (((x) >= (y)) ? (x) : (y))
#endif

// maps a profile ID to a brain
unordered_map<int, MegaHal*> g_megahal_brains;

MegaHal* getBotBrain(bot_profile_t* pBotProfile) {
	int profileid = pBotProfile->m_iProfileId;

	if (g_megahal_brains.find(profileid) == g_megahal_brains.end()) {
		MegaHal* brain = new MegaHal();
		g_megahal_brains[profileid] = brain;
	}
	
	return g_megahal_brains[profileid];
}

void BotChatReply ( CBot *pBot, char *szMsg, edict_t *pSender, char *szReplyMsg )
{
   // the purpose of this function is to make the bot keep an eye on what's happening in the
   // chat room, and in case of new messages, think about a possible reply.
	
	//callStack.Push("BotChatReply (%x,%x,%x,%x)",pBot,szMsg,pSender,szReplyMsg);
	// no message!
	if ( !szMsg || !*szMsg )
		return;

	// is bot chat allowed AND this message is not from this bot itself ?
	if ( gBotGlobals.IsConfigSettingOn(BOT_CONFIG_CHATTING) && (pSender != pBot->m_pEdict) )
	{
		int iNameLength;
		char *szName;
		
		iNameLength = strlen(pBot->m_szBotName);
		szName = new char [ sizeof(char) * (iNameLength + 1) ];
		RemoveNameTags(pBot->m_szBotName,szName);
		szName[iNameLength] = 0;

		//int i,j;

		bool bNameInMsg = FALSE;
		char *szNamePos;		

		strlow(szMsg);
		strlow(szName);

		szNamePos = strstr(szMsg,szName);
		bNameInMsg = (szNamePos != NULL);

		int iSenderNameLength = strlen(STRING(pSender->v.netname));
		char *szSenderName = new char [ sizeof(char) * (iSenderNameLength + 1) ];
		RemoveNameTags(STRING(pSender->v.netname),szSenderName);
		szSenderName[iSenderNameLength] = 0;

		/*while ( szNamePos != NULL )
		{
			i = 0;



			strstr(szNamePos,szName);
		}*/
		

		if ( bNameInMsg )
		{
			BotFunc_FillString(szMsg,szName,name_in_msg,strlen(szMsg));
		}
		
		int iRep;
		CClient *pClient;

		pClient = gBotGlobals.m_Clients.GetClientByEdict(pSender);
		iRep = pBot->m_Profile.m_Rep.GetClientRep(pClient);

		// if sender is not a bot then learn from it's message
		bool shouldLearnFromInput = !gBotGlobals.IsConfigSettingOn(BOT_CONFIG_CHAT_DONT_LEARN)
			&& UTIL_GetBotPointer(pSender) == NULL;

		MegaHal* hal = getBotBrain(&pBot->m_Profile);

		// does the bot feel concerned ? (more chances of replying if its name appears)
		// if real mode is on, then bot chat is affected by bots rep with sender
		// and depends on chat_reply_percent command
		if (gBotGlobals.m_iBotChatReplyPercent&&( bNameInMsg || /*gBotGlobals.m_Clients*/
			 ( ( !gBotGlobals.IsConfigSettingOn(BOT_CONFIG_REAL_MODE) || (RANDOM_LONG(BOT_MIN_REP,BOT_MAX_REP) < iRep)) && (RANDOM_LONG(0,100) < gBotGlobals.m_iBotChatReplyPercent) ) || (UTIL_GetNumClients(FALSE)==2)))
		{
			pBot->m_MegaHALTalkEdict = pSender;

            //BotHALGenerateReply (pBot, szReplyMsg ); // generate the reply
			char* replyString = hal->do_reply(szMsg, shouldLearnFromInput);
			strncpy(szReplyMsg, replyString, BOT_CHAT_MESSAGE_LENGTH);
			szReplyMsg[BOT_CHAT_MESSAGE_LENGTH - 1] = '\0';

			BotFunc_FillString(szReplyMsg,name_in_msg,szSenderName,BOT_CHAT_MESSAGE_LENGTH);

			if ( strcmp(szReplyMsg,szMsg) ) // not the exact same message? :-p
				strlow (szReplyMsg); // convert the output string to lowercase			
			else
				szReplyMsg[0]=0;
		} else if (shouldLearnFromInput) {
			hal->learn_no_reply(szMsg);
		}

		delete[] szName;
		delete[] szSenderName;

		szName = NULL;
		szSenderName = NULL;
	}
}
// from old rcbot
void HumanizeString ( char *string )
{
	const int drop_percent = 1;
	const int swap_percent = 1;
	const int capitalise_percent = 1;
	const int lower_percent = 2;

	int length = strlen(string);
	int i = 0;
	int n = 0;
	int rand;

	char temp;

	while ( i < length )
	{
		if ( ((i + 1) < length) && (RANDOM_LONG(0,1000) < swap_percent) )
		{
			temp = string[i];

			string[i] = string[i + 1];

			string[i + 1] = temp;

			i+=2;
			continue;
		}

		if ( ((rand = RANDOM_LONG(0,1000)) < drop_percent) ||
			 ( ((string[n] < '0') && (string[n] > '9')) && (rand < drop_percent*2) ) )
		{
			n = i;

			while ( n < (length - 1))
			{
				string[n] = string[n + 1];
				n++;
			}

			string[--length] = 0;

			i++;
			continue;
		}
		
		if ( !isupper((byte)string[i]) && ((i == 0) || (string[i-1] == ' ')) )
		{
			if ( RANDOM_LONG(0,1000) < (capitalise_percent * 2) )
				string[i] = toupper(string[i]);
		}

		if ( isupper((byte)string[i]) && (RANDOM_LONG(0,1000) < lower_percent) )
			string[i] = tolower((byte)string[i]);

		i++;		
	}
}

// from old rcbot
void RemoveNameTags ( const char *in_string, char *out_string )
{
	int i = 0; // index of in_string
	int n = 0; // length of out_string

	out_string[0] = 0;

	int length;

	char current_char;

	char tag_start;
	int tag_size = 0;
	bool space_allowed = FALSE;
	bool inside_tag;

	if ( in_string == NULL )
		return;

	length = strlen(in_string);

	if ( length > 127 )
	{
		if ( gBotGlobals.m_iDebugLevels & BOT_DEBUG_THINK_LEVEL )
			ALERT(at_console,"Error : RemoveNameTags(): Input netname is too long!\n");

		return;
	}

	inside_tag = FALSE;

	while ( i < length )
	{
		current_char = 0;

		if ( inside_tag )
		{
			if ( ((i + 1) < length) && ( in_string[i] == '=' ) && ( in_string[i+i] == '-' ) )
			{
				inside_tag = FALSE;
				i += 2;
				continue;
			}
			else if ( (in_string[i] == ')') || (in_string[i] == ']') || (in_string[i] == '}') )
			{				
				//char temp = in_string[i];

				inside_tag = FALSE;
				
				//if ( tag_size == 0 )
				//{
				//	out_string[n++] = tag_start;
				//	out_string[n++] = temp;					
				//}
					
				i++;

				continue;
			}
			else
			{
				tag_size++;
				i++;
				continue;				
			}
		}

		if ( isalnum((byte)in_string[i]) )
		{
			current_char = in_string[i];

			space_allowed = TRUE;
		}
		else if ( (in_string[i] == '(') || (in_string[i] == '[') || (in_string[i] == '{') )
		{
			inside_tag = TRUE;
			tag_start = in_string[i];
			tag_size=0;
			i++;
			continue;
		}
		else if ( ((i + 1) < length) && ( in_string[i] == '-' ) && ( in_string[i+i] == '=' ) )
		{
			inside_tag = TRUE;

			i += 2;
			continue;
		}
		else
		{
			if ( space_allowed )
			{
				current_char = ' ';
				space_allowed = FALSE;
			}
			else
			{
				i++;
				continue;
			}
		}

		// l33t to normal
	    switch ( current_char )
		{
		case '5':
			current_char = 's';
			break;
		case '0':
			current_char = 'o';
			break;
		case '7':
			current_char = 't';
			break;
		case '3':
			current_char = 'e';
			break;
		case '1':
			current_char = 'i';
			break;
		case '4':
			current_char = 'a';
			break;
		}		

		tag_size=0;
		out_string[n] = current_char;
		n++;

		i++;
		
	}

	if ( out_string[0] != 0 )
	{
		out_string[n] = 0;

		n --;

		while ( (n > 0) && (out_string[n] == ' ') )
			out_string[n--] = 0;
	}

	if ( out_string[0] == 0 )
		strcpy(out_string,in_string);

	strlow(out_string);
/*
	// more 'l33t stuff
	// "l33t" R
	BotFunc_FillString(out_string,"|2","r",length);
	// fat "l33ty" P's I's and M's
	BotFunc_FillString(out_string,"[]D","p",length);
	BotFunc_FillString(out_string,"[]V[]","m",length);
	BotFunc_FillString(out_string,"[]","i",length);

	// http://www.bbc.co.uk/dna/h2g2/A787917
	BotFunc_FillString(out_string,"()","o",length);
	BotFunc_FillString(out_string,"|_|","u",length);
	BotFunc_FillString(out_string,"|)","d",length);
	BotFunc_FillString(out_string,"\\/\\/","w",length);
	BotFunc_FillString(out_string,"$","s",length);
*/
}

int strpos ( char *pos, char *start )
{
	return ( (int)pos - (int)start );
}

void FillStringArea ( char *string, int maxstring, char *fill, int maxfill, int start, int end )
{	
	int size = sizeof(char) * (maxstring+1);

	char *before = (char*)malloc(size);
	char *after = (char*)malloc(size);

	memset(before,0,size);
	memset(after,0,size);

	strncpy(before,string,start);	
	strncpy(after,&string[end],maxstring-end);

	sprintf(string,"%s%s%s",before,fill,after);

	if ( before != NULL )
		free(before);

	before = NULL;

	if ( after != NULL )
		free(after);

	after = NULL;
}

bool LoadHALBrainForPersonality(bot_profile_t* pBotProfile)
{
	// this function loads a HAL brain
	char brn_file[256];
	sprintf(brn_file, "%d", pBotProfile->m_iProfileId);

	char fpath[256];
	UTIL_BuildFileName(fpath, BOT_PROFILES_FOLDER, brn_file);

	if (g_megahal_brains.find(pBotProfile->m_iProfileId) == g_megahal_brains.end()) {
		g_megahal_brains[pBotProfile->m_iProfileId] = new MegaHal();
	}
	getBotBrain(pBotProfile)->load_personality(fpath);

	return (TRUE); // no error, return FALSE
}
