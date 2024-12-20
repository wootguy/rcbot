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
#pragma once

#ifdef HLCOOP_BUILD
#include "hlcoop.h"
#else
#include "mmlib.h"
#endif

#include "megahal.h"

// bot HAL Markov model order
#define BOT_HAL_MODEL_ORDER 5

#ifndef CBot
class CBot;
#endif

#ifndef bot_profile_t
struct bot_profile_t;
#endif

class MehaHal;

void HumanizeString ( char *string );
void RemoveNameTags ( const char *in_string, char *out_string );

void BotChat (CBot *pBot);
void BotSayText (CBot *pBot);
void BotSayAudio (CBot *pBot);
void BotTalk (CBot *pBot, char *sound_path);

void BotChatReply ( CBot *pBot, char *szMsg, edict_t *pSender, char *szReplyMsg );
bool LoadHALBrainForPersonality (bot_profile_t*pBotProfile);

MegaHal* getBotBrain(bot_profile_t* pBotProfile);