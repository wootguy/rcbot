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
//////////////////////////////////////////////////
// RCBOT : Paul Murphy @ {cheeseh@rcbot.net}
//
// (http://www.rcbot.net)
//
// Based on botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot_weapons.h
//
//////////////////////////////////////////////////
//
// Weapons available in game and weapons that bots have
// including ammo info etc.
//

#ifdef HLCOOP_BUILD
#include "hlcoop.h"
#else
#include "mmlib.h"
#endif

#include "bot_const.h"
#include "bot.h"
#include "waypoint.h"
#include "weaponinfo.h"
#include "bot_weapons.h"
#include "perceptron.h"

#include <queue>
using namespace std;

// begin -- TS metamod weapon restriction plugin

const char *pszTSWeaponModels[] = {"0","models/w_glock18.mdl","2 No gun","models/w_uzi.mdl"
,"models/w_m3.mdl","models/w_m4.mdl","models/w_mp5sd.mdl","models/w_mp5k.mdl"
,"models/w_berettas.mdl","models/w_mk23.mdl","models/w_mk23_akimbo.mdl","models/w_usas.mdl"
,"models/w_desert.mdl","models/w_ak47.mdl","models/w_fnh.mdl","models/w_aug.mdl"
,"models/w_uzi.mdl","models/w_tmp.mdl","models/w_m82.mdl","models/w_pdw.mdl"
,"models/w_spas12.mdl","models/w_gold.mdl","models/w_glock22.mdl","models/w_ump.mdl"
,"models/w_m61.mdl","models/w_knife.mdl","models/w_mossberg.mdl","models/w_m16.mdl"
,"models/w_ruger.mdl","29 no gun here","30 no gun here","models/w_bull.mdl"
,"models/w_m60.mdl","models/w_sawedoff.mdl","models/w_katana.mdl","models/w_sealknife.mdl"};

// end

extern CBotGlobals gBotGlobals;

void CWeapon :: SetWeapon ( int iId, const char *szClassname, int iPrimAmmoMax, int iSecAmmoMax, int iHudSlot, int iHudPosition, int iFlags, int iAmmoIndex1, int iAmmoIndex2 )
{
	if ( m_szClassname == NULL )
		m_szClassname = gBotGlobals.m_Strings.GetString(szClassname);

	m_iId = iId;
	m_iPrimAmmoMax = iPrimAmmoMax;
	m_iSecAmmoMax = iSecAmmoMax;
	m_iHudSlot = iHudSlot;
	m_iHudPosition = iHudPosition;

	m_iAmmoIndex1 = iAmmoIndex1;
	m_iAmmoIndex2 = iAmmoIndex2;

	m_bRegistered = TRUE;
}

BOOL CBotWeapon :: CanReload ( void )
{
	if ( IsMelee() )
		return FALSE;

	if ( m_iAmmo1 )
	{
		return (*m_iAmmo1 > 0);
	}
			
	return FALSE;
}

BOOL CWeapon :: IsPrimary ( void )
{
	switch ( gBotGlobals.m_iCurrentMod )
	{
	case MOD_NS:
		return HudSlot() == 0;
		break;
	default:
		return FALSE;
	}
}

BOOL CWeapon :: IsSecondary ( void )
{
	switch ( gBotGlobals.m_iCurrentMod )
	{
	case MOD_NS:
		return HudSlot() == 1;
		break;
	default:
		return FALSE;
	}
}

void CBotWeapon :: SetWeapon ( int iId, int *iAmmoList )
{
	CBotWeapon();
	
	m_iId = iId;
	m_pWeaponInfo = gBotGlobals.m_Weapons.GetWeapon(iId);
	
	if ( m_pWeaponInfo != NULL )
	{		
		// this really should not be NULL!!!
		//assert(m_pWeaponInfo != NULL);
		
		m_bHasWeapon = TRUE;
		
		m_iAmmo1 = NULL;
		m_iAmmo2 = NULL;
		
		if ( m_pWeaponInfo )
		{
			int iAmmoIndex1 = m_pWeaponInfo->m_iAmmoIndex1;
			int iAmmoIndex2 = m_pWeaponInfo->m_iAmmoIndex2;
			
			if ( iAmmoList && (iAmmoIndex1 != -1) )
				m_iAmmo1 = &iAmmoList[iAmmoIndex1];
			
			if ( iAmmoList && (iAmmoIndex2 != -1) )
				m_iAmmo2 = &iAmmoList[iAmmoIndex2];
		}
		
	}
}

void CBotWeapon :: setHasWeapon ( BOOL bVal )
{
	m_bHasWeapon = bVal;
}

void CBotWeapons :: AddWeapon ( int iId )
{		
	m_Weapons[iId].SetWeapon(iId,m_iAmmo);
}

void CWeapons :: AddWeapon ( int iId, const char *szClassname, int iPrimAmmoMax, int iSecAmmoMax, int iHudSlot, int iHudPosition, int iFlags, int iAmmoIndex1, int iAmmoIndex2 )
{
	if (( iId >= 0 ) && ( iId < MAX_WEAPONS ))
	{
		if ( m_Weapons[iId] == NULL )
		{			
			weapon_preset_t *pPreset = gBotGlobals.m_WeaponPresets.GetPreset(gBotGlobals.m_iCurrentMod,iId);

			if ( pPreset == NULL )
			{
				m_Weapons[iId] = new CWeapon();
			}
			else 
			{
				m_Weapons[iId] = new CWeaponPreset(pPreset);
					
			}
			
			m_Weapons[iId]->SetWeapon(iId,szClassname,iPrimAmmoMax,iSecAmmoMax,iHudSlot,iHudPosition,iFlags,iAmmoIndex1,iAmmoIndex2);
		}
		else {
			// call again because weapons with the same ID in sven can change types (sven mp5 -> hl mp5)
			m_Weapons[iId]->SetWeapon(iId, szClassname, iPrimAmmoMax, iSecAmmoMax, iHudSlot, iHudPosition, iFlags, iAmmoIndex1, iAmmoIndex2);
		}
	}
}


void CWeaponPresets :: ReadPresets ( void )
{
	weapon_preset_t sWeaponPreset;

	FILE *fp;

	char filename[512];

	UTIL_BuildFileName(filename,BOT_WEAPON_PRESETS_FILE);

	fp = fopen(filename,"r");

	if ( fp == NULL )
		return;

	int iLength;
	char buffer[256];

	int iModId = 0;
	int iWeaponId = 0;
	int iValue;

	// bSkipMod will be true when the weapons are not for the current mod
	// and do not need to be loaded
	BOOL bSkipMod = FALSE;

	memset(&sWeaponPreset,0,sizeof(weapon_preset_t));

	while ( fgets(buffer, 255, fp) != NULL )
	{
	   if (buffer[0] == '#')
		   continue;

	   iLength = strlen(buffer);

	   if ( iLength <= 0 ) // blank line...
		   continue;

	   if ( buffer[iLength-1] == '\n' )
	   {
		   buffer[--iLength] = 0;
	   }

	   if ( iLength == 0 ) // blank line...
		   continue;

#ifdef __linux__
	   if ( buffer[iLength-1] == '\r' )
	   {
		   buffer[--iLength] = 0;
	   }

	   if ( iLength == 0 ) // blank line...
		   continue;
#endif

	   if ( sscanf(buffer,"[mod_id=%d]",&iValue) == 1 )
	   {
		   bSkipMod = ( iValue != gBotGlobals.m_iCurrentMod );
			
		   if ( !bSkipMod )
			iModId = iValue;	

		   continue;
	   }

	   if ( bSkipMod )
		   continue;
	   
	   if ( sscanf(buffer,"[weapon_id=%d]",&iValue) == 1 )
	   {
		   iWeaponId = iValue;

		   memset(&sWeaponPreset,0,sizeof(weapon_preset_t));

		   sWeaponPreset.m_iId = iWeaponId;
		   sWeaponPreset.m_iModId = (short int)iModId;	
		   continue;
	   }
	   
	   if ( sscanf(buffer,"underwater=%d",&iValue) == 1 )
	   {
		   sWeaponPreset.m_bCanFireUnderWater = iValue;

		   continue;
	   }
	   
	   if ( sscanf(buffer,"primaryfire=%d",&iValue) == 1 )
	   {
		   sWeaponPreset.m_bHasPrimaryFire = iValue;

		   continue;
	   }
	   
	   if ( sscanf(buffer,"secondaryfire=%d",&iValue) == 1 )
	   {
		   sWeaponPreset.m_bHasSecondaryFire = iValue;
		   continue;
	   }
	   
	   if ( sscanf(buffer,"primary_min_range=%d",&iValue) == 1 )
	   {
		   sWeaponPreset.m_fPrimMaxRange = iValue;
		   continue;
	   }
	   
	   if ( sscanf(buffer,"primary_max_range=%d",&iValue) == 1 )
	   {
		   sWeaponPreset.m_fPrimMaxRange = iValue;
		   continue;
	   }
	   
	   if ( sscanf(buffer,"secondary_min_range=%d",&iValue) == 1 )
	   {
		   sWeaponPreset.m_fSecMinRange = iValue;
		   continue;
	   }
	   
	   if ( sscanf(buffer,"secondary_max_range=%d",&iValue) == 1 )
	   {
		   sWeaponPreset.m_fSecMaxRange = iValue;
		   continue;
	   }

	   if ( sscanf(buffer,"is_melee=%d",&iValue) == 1 )
	   {
		   sWeaponPreset.m_bIsMelee = iValue;
		   continue;
	   }

	   if ( sscanf(buffer,"priority=%d",&iValue) == 1 )
	   {
		   sWeaponPreset.m_iPriority = iValue;
		   continue;
	   }

	   if ( strcmp(buffer,"[/weapon]") == 0 )
			m_Presets.Push(sWeaponPreset);
	}
	
	fclose(fp);
	
	
}

BOOL CBotWeapon :: 	HasWeapon ( edict_t *pEdict )
{	
	if ( pEdict )
	{
		switch ( gBotGlobals.m_iCurrentMod )
		{
		case MOD_NS:

			if ( !gBotGlobals.IsConfigSettingOn(BOT_CONFIG_NOT_NS3_FINAL) )
				return m_bHasWeapon;

			if ( pEdict->v.team == TEAM_ALIEN )
			{
				if ( m_bHasWeapon )
				{			
					edict_t *pWeapon = NULL;
					
					char *szClassname = this->GetClassname();
					
					if ( szClassname == NULL )
						return FALSE; // error
					
					while ( (pWeapon = UTIL_FindEntityByClassname(pWeapon,szClassname)) != NULL )
					{
						if ( !(pWeapon->v.effects & EF_NODRAW) )
							continue;
						
						if ( pWeapon->v.origin == pEdict->v.origin )
							break;
					}
					
					if ( pWeapon )
					{
						/*if ( !gBotGlobals.IsConfigSettingOn(BOT_CONFIG_NOT_NS3_FINAL) )
						{
							if ( m_pWeaponInfo )
								return (m_pWeaponInfo->HudSlot())<=(UTIL_GetNumHives()+1);
							
							return FALSE;
						}
						else*/ if ( pWeapon->v.iuser3 != 1 )
						{
							return FALSE;
						}
					}
					
					return TRUE;
				}
				
				return FALSE;
			}
			break;
		default:
			break;
		}
	}

	return m_bHasWeapon;
}

// Fill arrays, each element represents a weapon id
// 1 indicates available, 0 indicates that it is not
// available...
void GetNoWeaponArray ( short int *Array )
{
	memset(Array,0,sizeof(short int)*MAX_WEAPONS);
}

void GetArrayOfExplosives ( short int *Array )
{
	int i;

	for ( i = 0; i < MAX_WEAPONS; i ++ )
	{
		switch ( i )
		{
		case VALVE_WEAPON_HANDGRENADE:
		case VALVE_WEAPON_RPG:
		case VALVE_WEAPON_TRIPMINE:
		case VALVE_WEAPON_SATCHEL:
			Array[i] = 1;
		}
	}
}

//
// Get the best weapon Id number for a bot facing an enemy pEnemy.
//
int CBotWeapons :: GetBestWeaponId( CBot *pBot, edict_t *pEnemy )
{	
	priority_queue<CBotWeapon*,vector<CBotWeapon*>,CompareBotWeapon> Weapons;
	priority_queue<CBotWeapon*,vector<CBotWeapon*>,CompareBotWeapon> otherWeapons;
//	dataQueue<int> Weapons;
//	dataQueue<int> otherWeapons;
	
	CBotWeapon *pWeapon;
	//CWeapon *pWeaponInfo;

	edict_t *pEdict = pBot->m_pEdict;
	
	Vector vEnemyOrigin;
	float fEnemyDist;

	BOOL bEnemyIsElectrified = FALSE;
	BOOL bEnemyTooHigh = FALSE;
	BOOL bUnderwater = (pEdict->v.waterlevel == 3);
	BOOL bIsDMC = (gBotGlobals.m_iCurrentMod == MOD_DMC);

	BOOL bIsBattleGrounds = (gBotGlobals.m_iCurrentMod == MOD_BG);
	BOOL bWantToMelee = FALSE;

	bool bIsBreakable = pEnemy && FStrEq("func_breakable", STRING(pEnemy->v.classname));

	short int iAllowedWeapons[MAX_WEAPONS];

	for ( int i = 0; i < MAX_WEAPONS; i ++ )
		iAllowedWeapons[i] = 1;	

	if ( pEnemy )
	{
		vEnemyOrigin = EntityOrigin(pEnemy);
		fEnemyDist = pBot->DistanceFrom(vEnemyOrigin);
		bEnemyTooHigh = ( vEnemyOrigin.z > (pBot->pev->origin.z + MAX_JUMP_HEIGHT) );

		if (bIsBreakable) {
			bEnemyTooHigh = (pEnemy->v.absmin.z > (pBot->pev->origin.z + MAX_JUMP_HEIGHT) );
		}
	}
	else
	{
		vEnemyOrigin = pBot->pev->origin;
		fEnemyDist = 0;
	}

	switch ( gBotGlobals.m_iCurrentMod )
	{
	case MOD_NS:
		if ( pBot->IsAlien() )
		{
			if ( pEnemy )
			{
				if ( (pEnemy->v.iuser3 == AVH_USER3_BREAKABLE) || EntityIsMarineStruct(pEnemy) )
				{
					// cant leap structures etc...
					iAllowedWeapons[NS_WEAPON_UMBRA] = 0;
					iAllowedWeapons[NS_WEAPON_SPORES] = 0;
					iAllowedWeapons[NS_WEAPON_WEBSPINNER] = 0;
					iAllowedWeapons[NS_WEAPON_METABOLIZE] = 0;
					iAllowedWeapons[NS_WEAPON_PARASITE] = 0;
					iAllowedWeapons[NS_WEAPON_BLINK] = 0;
					// Abilities
					iAllowedWeapons[NS_ABILITY_LEAP] = 0;
					iAllowedWeapons[NS_ABILITY_CHARGE] = 0;
					iAllowedWeapons[NS_WEAPON_PRIMALSCREAM] = 0;
					iAllowedWeapons[NS_WEAPON_HEALINGSPRAY] = 0;
					iAllowedWeapons[NS_WEAPON_STOMP] = 0;
					iAllowedWeapons[NS_WEAPON_DEVOUR] = 0;					
				}
				else if ( pEnemy->v.iuser3 == AVH_USER3_MARINE_PLAYER )
				{
					iAllowedWeapons[NS_WEAPON_BILEBOMB] = 0;

					// heavy armour
					if ( pEnemy->v.iuser4 & MASK_UPGRADE_13 )
					{
						iAllowedWeapons[NS_WEAPON_SPORES] = 0;
					}

					if ( pEnemy->v.iuser4 & MASK_PLAYER_STUNNED )
						iAllowedWeapons[NS_WEAPON_STOMP] = 0;
				}
			}			
		}
		else
			iAllowedWeapons[NS_WEAPON_MINE] = 0;

		break;
	case MOD_TFC:
		if ( pEnemy && (pEnemy->v.flags & FL_MONSTER) )
		{
			iAllowedWeapons[TF_WEAPON_MEDIKIT] = 0;
		}
		if ( pBot->hasFlag() )
		{
			iAllowedWeapons[TF_WEAPON_KNIFE] = 0;
			iAllowedWeapons[TF_WEAPON_AXE] = 0;
			iAllowedWeapons[TF_WEAPON_SPANNER] = 0;
		}
		break;
	case MOD_BG:
		if ( pBot->m_pCurrentWeapon != NULL )
		{
			// want to melee true if needing to reload OR enemy within melee range
			// AND random factor due to skill
			BOOL bMeleeRangeCheck = (pEnemy && (fEnemyDist<80.0));
			BOOL bMaxRangeCheck = (pEnemy && (fEnemyDist<512.0));
			
			bWantToMelee = ( (bMeleeRangeCheck || (pBot->m_pCurrentWeapon->NeedToReload()) && (RANDOM_LONG(MIN_BOT_SKILL,MAX_BOT_SKILL) < pBot->m_Profile.m_iSkill)) && bMaxRangeCheck );
		}

		if ( pEnemy != NULL )
		{
			// melee breakables.
			if ( FStrEq( STRING(pEnemy->v.classname), "func_breakable" ) )
				bWantToMelee = TRUE;
		}
		break;
	default:
		break;
	}
	
	if ( pEnemy )
	{
		switch ( gBotGlobals.m_iCurrentMod )
		{
		case MOD_SVENCOOP:
		if (bIsBreakable)
		{
			iAllowedWeapons[SVEN_WEAPON_GRAPPLE] = 0; // grapple can't damage breakables

			if ( pEnemy->v.spawnflags & SF_BREAK_EXPLOSIVES )
			{
				GetNoWeaponArray(iAllowedWeapons);
				GetArrayOfExplosives(iAllowedWeapons);//bExplosives = pEnemy->v.spawnflags & 512;
				
				float fEnemyDist = (vEnemyOrigin - pBot->GetGunPosition()).Length();

				if ( pBot->HasWeapon(VALVE_WEAPON_MP5) )
				{
					CBotWeapon *pWeapon = pBot->m_Weapons.GetWeapon(VALVE_WEAPON_MP5);
						
					if ( pWeapon->SecondaryAmmo() > 0 && pWeapon->SecondaryInRange(fEnemyDist))
						iAllowedWeapons[VALVE_WEAPON_MP5] = 1;
				}
				if (pBot->HasWeapon(SVEN_WEAPON_M16))
				{
					CBotWeapon* pWeapon = pBot->m_Weapons.GetWeapon(SVEN_WEAPON_M16);

					if (pWeapon->SecondaryAmmo() > 0 && pWeapon->SecondaryInRange(fEnemyDist)) {
						iAllowedWeapons[SVEN_WEAPON_M16] = 1;
					}
				}
			}
			if ((pEnemy->v.spawnflags & SF_BREAK_INSTANT))
			{
				// TODO: don't know which weapon it's weak to because the "weapon" keyvalue is private
				// and if the entity was created by angelscript then the KeyValue hook is never called.
				// It will eventually break with any weapon.
				bool hasInstantBreakWep = pBot->HasWeapon(VALVE_WEAPON_CROWBAR) || pBot->HasWeapon(SVEN_WEAPON_PIPEWRENCH);

				if (hasInstantBreakWep || pEnemy->v.health > 1000) {
					GetNoWeaponArray(iAllowedWeapons);
					iAllowedWeapons[VALVE_WEAPON_CROWBAR] = 1;
					iAllowedWeapons[SVEN_WEAPON_PIPEWRENCH] = 1;
				}
			}
		}
		else if ( FStrEq("monster_gargantua",STRING(pEnemy->v.classname)) )
		{
			// only use explosives & egon on garg
			GetNoWeaponArray(iAllowedWeapons);
			GetArrayOfExplosives(iAllowedWeapons);
			iAllowedWeapons[VALVE_WEAPON_EGON] = 1;	

			float fEnemyDist = (vEnemyOrigin - pBot->GetGunPosition()).Length();

			if ( pBot->HasWeapon(VALVE_WEAPON_MP5) )
			{
				CBotWeapon *pWeapon = pBot->m_Weapons.GetWeapon(VALVE_WEAPON_MP5);


				if ( pWeapon->SecondaryAmmo() > 0 && pWeapon->SecondaryInRange(fEnemyDist))
					iAllowedWeapons[VALVE_WEAPON_MP5] = 1;
			}
			if (pBot->HasWeapon(SVEN_WEAPON_M16))
			{
				CBotWeapon* pWeapon = pBot->m_Weapons.GetWeapon(SVEN_WEAPON_M16);

				if (pWeapon->SecondaryAmmo() > 0 && pWeapon->SecondaryInRange(fEnemyDist))
					iAllowedWeapons[SVEN_WEAPON_M16] = 1;
			}
		}
		break;
		case MOD_NS:
			if ( pBot->IsMarine() )
			{
				switch ( pEnemy->v.iuser3 )
				{
					// Use knife or welder(better) for things you dont want to waste ammo on it
				case AVH_USER3_DEFENSE_CHAMBER:
				case AVH_USER3_ALIENRESTOWER:
				case AVH_USER3_MOVEMENT_CHAMBER:
					if ( fEnemyDist < REACHABLE_RANGE )
					{
						if ( pBot->HasWeapon(NS_WEAPON_WELDER) )
							return NS_WEAPON_WELDER;
						
						return NS_WEAPON_KNIFE;
					}
					break;
				default:
					break;
				}
			}
			else
			{				
				// probably electrified				
				bEnemyIsElectrified = (pEnemy->v.iuser4 & MASK_UPGRADE_11);
			}
			break;
		default:
			break;
		}
	}

	if (pEnemy && gBotGlobals.IsDebugLevelOn(BOT_DEBUG_WEAPON_LEVEL)) {
		DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "Target: %s", STRING(pEnemy->v.classname));
	}

	for ( int i = 1; i < MAX_WEAPONS; i ++ )
	{
		pWeapon = &m_Weapons[i];

		if (iAllowedWeapons[i] == 0) {
			if (pWeapon->HasWeapon(pBot->m_pEdict) && gBotGlobals.IsDebugLevelOn(BOT_DEBUG_WEAPON_LEVEL)) {
				DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "%s - not allowed to use", pWeapon->GetClassname());
			}
			continue;
		}
		/*
		if ( bExplosives )
		{
			switch ( gBotGlobals.m_iCurrentMod )
				// ignore weapons that dont do explosive damage to enemies
			{
			case MOD_SVENCOOP:
				if ( ( i != VALVE_WEAPON_HANDGRENADE ) && (i != VALVE_WEAPON_RPG) )
					continue;
			default:
				break;
			}
		}*/

		// use the weapon code to see if we have the weapon in DMC
		if ((!bIsDMC && !pBot->HasWeapon(i)) || !pWeapon->HasWeapon(pBot->m_pEdict)) {
			continue;
		}

		if ( gBotGlobals.IsNS() )
		{
			if ( pWeapon->GetID() == NS_WEAPON_STOMP )
			{
				if ( pEnemy )
				{
					if ( pEnemy->v.velocity.Length() < 1 )
						continue;
				}
			}
			else if ( pWeapon->GetID() == NS_WEAPON_PARASITE )
			{
				if ( pEnemy )
				{
					// already parasited
					if ( pEnemy->v.iuser4 & MASK_PARASITED )
						continue;
				}
			}
		}

		if ( bUnderwater )
		{
			if (!pWeapon->CanBeUsedUnderWater()) {
				if (gBotGlobals.IsDebugLevelOn(BOT_DEBUG_WEAPON_LEVEL)) {
					DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "%s - Can't use underwater", pWeapon->GetClassname());
				}
				continue;
			}
		}
		
		if ( pWeapon->IsMelee() )
		{
			if ( bEnemyTooHigh )
			{
				// too high to hit...
				if (gBotGlobals.IsDebugLevelOn(BOT_DEBUG_WEAPON_LEVEL)) {
					DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "%s - target too high for melee", pWeapon->GetClassname());
				}
				continue;
			}
			else if ( pEnemy && bEnemyIsElectrified )
			{
				vector<ga_value> inputs;

				int iweap = 0;

				if ( pBot->m_pCurrentWeapon )
					iweap = pBot->m_pCurrentWeapon->GetID();

				inputs.push_back(pBot->pev->health/pBot->pev->max_health);
				inputs.push_back(pEnemy->v.health/pEnemy->v.max_health);
				inputs.push_back(pBot->pev->size.Length()/pEnemy->v.size.Length());
				inputs.push_back(pBot->NS_AmountOfEnergy()/100);
				inputs.push_back(iweap/MAX_WEAPONS);
				inputs.push_back(pBot->onGround());
				inputs.push_back(pBot->pev->velocity.Length()/pBot->pev->maxspeed);

				pBot->dec_attackElectrified->input(&inputs);
				pBot->dec_attackElectrified->execute();

				pBot->m_pElectricEnemy = pEnemy;

				inputs.clear();

				if ( pBot->dec_attackElectrified->trained() && !pBot->dec_attackElectrified->fired() )//pBot->IsSkulk() )
				{
				// will be electrified if I try to attack...
					if (gBotGlobals.IsDebugLevelOn(BOT_DEBUG_WEAPON_LEVEL)) {
						DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "%s - target is too electrified for melee", pWeapon->GetClassname());
					}
					continue;
				}
			}
			

			otherWeapons.push(pWeapon);
			if (gBotGlobals.IsDebugLevelOn(BOT_DEBUG_WEAPON_LEVEL)) {
				DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "%s - USABLE (priority %d)", pWeapon->GetClassname(), pWeapon->GetPriority());
			}
			Weapons.push(pWeapon);//.Add(i);	
			continue;
		}

		if ( !pWeapon->CanShootPrimary(pEdict,fEnemyDist,pBot->m_fDistanceFromWall) )
		{
			//if ( !pWeapon->CanShootSecondary() )
			if (gBotGlobals.IsDebugLevelOn(BOT_DEBUG_WEAPON_LEVEL)) {
				DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "%s - can't shoot primary", pWeapon->GetClassname());
			}
			continue;
		}
		
		otherWeapons.push(pWeapon);//.Add(i);

		if ( bIsBattleGrounds )
		{
			// dont want to do other stuff except melee
			if ( bWantToMelee )
				continue;
		}

		if ( pWeapon->NeedToReload() )
		{
			if (gBotGlobals.IsDebugLevelOn(BOT_DEBUG_WEAPON_LEVEL)) {
				DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "%s - USABLE (priority %d)", pWeapon->GetClassname(), pWeapon->GetPriority());
			}
			Weapons.push(pWeapon);//.Add(i);
			continue;
		}

		if (pWeapon->OutOfAmmo()) {
			if (gBotGlobals.IsDebugLevelOn(BOT_DEBUG_WEAPON_LEVEL)) {
				DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "%s - out of ammo for", pWeapon->GetClassname());
			}
			continue;
		}

		if (gBotGlobals.IsDebugLevelOn(BOT_DEBUG_WEAPON_LEVEL)) {
			DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "%s - USABLE (priority %d)", pWeapon->GetClassname(), pWeapon->GetPriority());
		}
		Weapons.push(pWeapon);//.Add(i);
	}

	if ( Weapons.empty() )//.IsEmpty() )
	{
		if ( bIsBattleGrounds && bWantToMelee )
		{
			//Weapons._delete();
			//Weapons = otherWeapons;

			if ( otherWeapons.empty() )//.IsEmpty() )
			{
				if ( gBotGlobals.IsMod(MOD_TS) )
					return 36;
				return 0;
			}
			else
			{
				pWeapon = otherWeapons.top();

				return pWeapon->GetID();
			}
		}
		else 
		{
			pBot->SetWeaponsNeeded(iAllowedWeapons);
			pBot->UpdateCondition(BOT_CONDITION_NEED_WEAPONS);
			//otherWeapons._delete();

			if ( !otherWeapons.empty() )
			{
				pWeapon = otherWeapons.top();
				
				if (gBotGlobals.IsDebugLevelOn(BOT_DEBUG_WEAPON_LEVEL)) {
					if (pWeapon)
						DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "%d held weps. Best wep %s (priority %d) has %d clip and %d ammo", Weapons.size(), pWeapon->GetClassname(), pWeapon->GetPriority(), pWeapon->getClip(), pWeapon->PrimaryAmmo());
					DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "---------------------------------------------------");
				}

				return pWeapon->GetID();
			}			
			
			if ( gBotGlobals.IsMod(MOD_TS) )
				return 36;

			if (gBotGlobals.IsDebugLevelOn(BOT_DEBUG_WEAPON_LEVEL)) {
				DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "No usable weapons!"); 
				DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "---------------------------------------------------");
			}
			return 0;
		}
	}

	pWeapon = Weapons.top();
	
	if (gBotGlobals.IsDebugLevelOn(BOT_DEBUG_WEAPON_LEVEL)) {
		if (pWeapon)
			DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "%d usable weps. Best wep %s (priority %d) has %d clip and %d ammo", Weapons.size(), pWeapon->GetClassname(), pWeapon->GetPriority(), pWeapon->getClip(), pWeapon->PrimaryAmmo());
		else 
			DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "No usable weapons!");
		DebugMessage(BOT_DEBUG_WEAPON_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "---------------------------------------------------");
	}

	return pWeapon->GetID();

	/*

	//dataStack<int> WeaponChoice;
	dataQueue<int> tempQueue = Weapons;

	int iHighestMeleePriority = -1;
	int iHighestNonMeleePriority = -1;

	int iPriority;	

	int iBestMeleeWeaponId = 0;
	int iBestNonMeleeWeaponId = 0;

	while ( !tempQueue.IsEmpty() )
	{
		i = tempQueue.ChooseFrom();
		
		pWeaponInfo = gBotGlobals.m_Weapons.GetWeapon(i);
		
		if ( pWeaponInfo )
		{
			iPriority = pWeaponInfo->GetPriority();

			if ( pWeaponInfo->IsMelee() )
			{
				if ( iPriority > iHighestMeleePriority )
				{
					iHighestMeleePriority = iPriority;
					iBestMeleeWeaponId = i;
				}
			}
			else
			{
				if ( iPriority > iHighestNonMeleePriority )
				{
					iHighestNonMeleePriority = iPriority;
					iBestNonMeleeWeaponId = i;
				}
			}
		}
	}


	Weapons._delete();

//	if ( !otherWeapons.IsEmpty() )
//		otherWeapons._delete();

	if ( iBestNonMeleeWeaponId != 0 )
	{
		// see if melee has higher priority than this
		if ( (!gBotGlobals.IsMod(MOD_TFC)||(pBot->pev->playerclass==TFC_CLASS_SPY)) && iBestMeleeWeaponId != 0 ) 
		{
			CWeapon *pMeleeWeapon = gBotGlobals.m_Weapons.GetWeapon(iBestMeleeWeaponId);
			CWeapon *pNonMeleeWeapon = gBotGlobals.m_Weapons.GetWeapon(iBestNonMeleeWeaponId);

			if ( pMeleeWeapon && pNonMeleeWeapon )
			{
				if ( pMeleeWeapon->PrimaryInRange(fEnemyDist) && (pMeleeWeapon->GetPriority() > pNonMeleeWeapon->GetPriority()) )
					return iBestMeleeWeaponId;
			}
		}

		return iBestNonMeleeWeaponId;
	
	}
	
	if ( gBotGlobals.IsMod(MOD_TS) )
		return 36;

	return iBestMeleeWeaponId;*/
}

BOOL CBotWeapon :: NeedToReload ( void )
{
	switch ( gBotGlobals.m_iCurrentMod )
	{
	case MOD_TS:
		return ( !m_iClip && (m_iReserve > 0) );
	case MOD_BUMPERCARS:
	case MOD_DMC:
		return FALSE;
	default:
		break;
	}

	if ( m_iAmmo1 )
	{
		return ( !m_iClip && (*m_iAmmo1 > 0) );
	}
	
	return FALSE;		
}

BOOL CBotWeapon :: CanShootPrimary ( edict_t *pEdict, float flFireDist, float flWallDist )
{
	if ( m_pWeaponInfo == NULL )
		return TRUE;

	if ( gBotGlobals.m_iCurrentMod == MOD_DMC )
		return (this->PrimaryAmmo() > 0);

	if ( this->OutOfAmmo() )
		return FALSE;
	if (( pEdict->v.waterlevel == 3 )&&(CanBeUsedUnderWater() == FALSE))
		return FALSE;

	if ( !m_pWeaponInfo->CanUsePrimary() )
		return FALSE;

	if ( gBotGlobals.IsMod(MOD_SVENCOOP) || gBotGlobals.IsMod(MOD_HL_DM) )
	{
		if ( GetID() == VALVE_WEAPON_RPG || GetID() == SVEN_WEAPON_DISPLACER)
		{
			if ( !m_pWeaponInfo->PrimaryInRange(flWallDist) )
				return FALSE;
		}
	}

	if ( !m_pWeaponInfo->PrimaryInRange(flFireDist) )
		return FALSE;
	
	return TRUE;
}

BOOL CBotWeapons :: HasWeapon ( edict_t *pEdict, char *szClassname )
{
	int i;
	const char *pClassname;
	
	for ( i = 1; i < MAX_WEAPONS; i ++ )
	{
		if ( HasWeapon(pEdict,i) )
		{				
			if ( (pClassname = m_Weapons[i].GetClassname()) != NULL )
			{
				if ( FStrEq(pClassname,szClassname) )
					return TRUE;
			}				
		}
	}
	
	return FALSE;
}
