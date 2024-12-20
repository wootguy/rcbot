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
// bot_navigate.cpp
//
//////////////////////////////////////////////////
//
// Bot navigation & waypoint handling etc.
//

#include "extdll.h"

#ifdef HLCOOP_BUILD
#include "hlcoop.h"
#else
#include "mmlib.h"
#endif

#include "bot.h"
#include "waypoint.h"

#include <vector>
#include <queue>
using namespace std;

static FILE *fp;

extern WAYPOINTS waypoints;//[MAX_WAYPOINTS];
extern CWaypointLocations WaypointLocations;
extern CBotGlobals gBotGlobals;


AStarNode::AStarNode()
{
	memset(this,0,sizeof(AStarNode));
	m_iWaypoint=-1;
}

BOOL AStarNode::heuristicSet ()
{
	return (flags & FL_ASTAR_HEURISTIC)==FL_ASTAR_HEURISTIC;
}

void AStarNode::setHeuristic (float botDist, float goalDist,BOOL bIsTeleport)
{	
	if ( bIsTeleport )
		m_fHeuristic = goalDist;
	else
		m_fHeuristic = botDist+goalDist;
		
	flags |= FL_ASTAR_HEURISTIC;
}

BOOL AStarNode::hasParent ()
{
	return (flags & FL_ASTAR_PARENT)==FL_ASTAR_PARENT;
}

BOOL AStarNode::isOpen ()
{
	return (flags & FL_ASTAR_OPEN)==FL_ASTAR_OPEN;
}

BOOL AStarNode::isClosed ()
{
	return (flags & FL_ASTAR_CLOSED)==FL_ASTAR_CLOSED;
}

void AStarNode::unClose ()
{
	flags &= ~FL_ASTAR_CLOSED;
}

void AStarNode::close ()
{
	flags |= FL_ASTAR_CLOSED;
}

void AStarNode::unOpen ()
{
	flags &= ~FL_ASTAR_OPEN;
}

void AStarNode::open ()
{
	flags |= FL_ASTAR_OPEN;
}

float AStarNode::getHeuristic ()
{
	return m_fHeuristic;
}

short int AStarNode::getParent ()
{
	return m_iParent;
}

void AStarNode::setParent ( int iWpt )
{
	m_iParent = iWpt;
	flags |= FL_ASTAR_PARENT;
}

bool AStarNode :: precedes (AStarNode *b)
{
	// lowest cost first
	return (m_fCost+getHeuristic()) < (b->m_fCost+b->getHeuristic());
}

bool AStarNode::operator()(AStarNode *a, AStarNode *b)
{
	return a<b;
}

bool AStarNode::operator<(AStarNode *b)
{
	return precedes(b);
}


int BotNavigate_AStarAlgo ( CBot *pBot, int iFrom, int iTo, BOOL bContinue, bool testOnly)
{
	dataStack<int> sTempList;
	//priority_queue<AStarNode*,vector<AStarNode*>,CompareAStarNode> *sOpenList;
	AStarOpenList *sOpenList;
	
	AStarNode *aPathsFound;
	AStarNode *sCurrNodeInfo = NULL; // Info of the current node
	AStarNode *sSuccNodeInfo = NULL; // Info of the succesor to the current node

	float fMinCost;
	float fOldCost;
	float fCurCost;

	int iNextWaypoint = -1;
//	int iNode;
	int iCurrentNode;
	int iSuccNode;
	BOOL bFoundGoal = FALSE;
	BOOL bDeleteLoopPath = FALSE;
	
	WAYPOINT *pSuccWpt;
	WAYPOINT *pCurWpt;

	// PATH INFO, For use with finding successors
	PATH *pPath;
	int iPathIndex;

	sOpenList = &pBot->sOpenList;
	aPathsFound = pBot->aPathsFound;

	// Initialise the paths found info

	TraceResult tr;

 	if ( (iTo == -1) || (iFrom == -1) )
	// Can't find a path from/to invalid waypoints
	{
		if ( gBotGlobals.IsDebugLevelOn(BOT_DEBUG_NAV_LEVEL) )
			DebugMessage(BOT_DEBUG_NAV_LEVEL,NULL,0,"AStarAlgo() %s has no goal/current waypoint.",pBot->m_szBotName);

		return -1;
	}

	/*if ( pBot->m_iLastFailedGoalWaypoint == iTo )
	{
		if ( gBotGlobals.IsDebugLevelOn(BOT_DEBUG_NAV_LEVEL) )
			DebugMessage(BOT_DEBUG_NAV_LEVEL,NULL,0,"AStarAlgo() %s failed to go to waypoint %d previously, not finding path.",pBot->m_szBotName,iTo);

		// Reset so we can try again later
		//pBot->m_iLastFailedGoalWaypoint = -1;

		return -1;
	}*/

	if ( !bContinue || testOnly)
	{
//		int i;

		//
		//sOpenList->Destroy();

		memset(aPathsFound,0,sizeof(AStarNode)*MAX_WAYPOINTS);

	//	for ( i = 0; i < MAX_WAYPOINTS; i ++ )
		//	aPathsFound[i].m_iParent = -1;

		if ( gBotGlobals.IsDebugLevelOn(BOT_DEBUG_NAV_LEVEL) )
		{
			DebugMessage(BOT_DEBUG_NAV_LEVEL,NULL,0,"AStarAlgo() %s finding path from %d to %d",pBot->m_szBotName,iFrom,iTo);
		}

		// The first node must be open

		// empty list
		//while ( !sOpenList->empty() )
		//	sOpenList->pop();
		sOpenList->destroy();
		//sOpenList->Init();
		AStarNode *curr = &aPathsFound[iFrom];
		curr->open();
		curr->m_iWaypoint = iFrom;
		curr->setHeuristic(pBot->DistanceFrom(waypoints[iFrom].origin),((waypoints[iFrom].origin-waypoints[iTo].origin).Length()));
		//sOpenList->push(&aPathsFound[iFrom]);
		sOpenList->add(&aPathsFound[iFrom]);
		pBot->iDetPackWaypoints.Clear();
	//	sOpenList->Push(iFrom);
	}
	else
	{
		if ( gBotGlobals.IsDebugLevelOn(BOT_DEBUG_NAV_LEVEL) )
		{
			DebugMessage(BOT_DEBUG_NAV_LEVEL,NULL,0,"AStarAlgo() %s continuing to find path from %d to %d",pBot->m_szBotName,iFrom,iTo);
		}
	}

	//pBot->m_vGoalOrigin = WaypointOrigin(iTo);

	sCurrNodeInfo = &aPathsFound[iFrom];

	int iLoops = 0;
	//extern int num_waypoints;
	const int iMaxLoops = pBot->m_Profile.m_iPathRevs;//gBotGlobals.m_iMaxPathRevs; // max loops per tick
	
	BOOL bBotCanUseFlyWpt;
	BOOL bBotCanUseWallStickWpt;
	
	bBotCanUseFlyWpt = gBotGlobals.IsMod(MOD_NS) && ((pBot->IsFade() && pBot->hasBlink()) || pBot->IsLerk() || (pBot->IsMarine() && pBot->HasJetPack()));
	bBotCanUseWallStickWpt = gBotGlobals.IsMod(MOD_TS) || (gBotGlobals.IsMod(MOD_NS) && bBotCanUseFlyWpt || pBot->IsSkulk());

	if (gBotGlobals.IsMod(MOD_SVENCOOP) && pBot->HasWeapon(SVEN_WEAPON_GRAPPLE)) {
		bBotCanUseFlyWpt = true;
	}

	Vector vGoalWptOrigin = WaypointOrigin(iTo);
	Vector vOrigin;

	int iSuccNodeFlags;

	BOOL m_bIsLerk = ( pBot->IsLerk() );

	int iBotTeam = pBot->GetTeam();
	BOOL bCanThrowGren = pBot->HasWeapon(VALVE_WEAPON_HANDGRENADE);

	BOOL bIsTeleport;
//	CBaseToggle *pToggle;
//	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pBot->m_pEdict);
	// Limit iLoops just incase we run into an infinite loop, or very long loop
	// which can sometimes happen
	int iPrevWpt = -1;	
	BOOL bDetpackWpt;

	while ( (bFoundGoal == FALSE) && !sOpenList->empty()/*IsEmpty()*/ && ((iLoops < iMaxLoops) || testOnly))
	{		

		iLoops++;

		iCurrentNode = -1;

		fMinCost = 0;

		sCurrNodeInfo = sOpenList->top();
		iCurrentNode = sCurrNodeInfo->m_iWaypoint;
		sCurrNodeInfo->unOpen();
		sOpenList->pop();		

		// We've found a goal once the current node is the goal waypoint.
        bFoundGoal = ( iCurrentNode == iTo );

		if ( bFoundGoal )
			break;

		if (!testOnly)
			pBot->m_FailedGoals.Remove(iCurrentNode);

		pCurWpt = &waypoints[iCurrentNode];
		vOrigin = WaypointOrigin(iCurrentNode);
        
        pPath = NULL;
        iPathIndex = 0;

		bDetpackWpt = (pCurWpt->flags & W_FL_TFC_DETPACK);

		bDeleteLoopPath = FALSE;

        while( (iSuccNode = WaypointFindPath(&pPath, &iPathIndex, iCurrentNode, iBotTeam)) != -1) 
		// Generate each state node_successor that can come after current node
        {
			if( iSuccNode == iCurrentNode )
            {
				bDeleteLoopPath = TRUE;
                continue;
            }
			// going backwards :S Not good
			if (( iPrevWpt != -1 ) && ( iPrevWpt == iSuccNode ))
			{
				continue;
			}

			// See if the bot has had problems going through this path so it can ignore it
			// for now...
			
			// Remove returns TRUE if successfully removed (i.e. in list)
			// this keeps the paths in until the bot has finished searching the whole
			// path
			if(!testOnly && pBot->m_stFailedPaths.Violate(pPath) )
			{
				// Forget this successor for now
				continue;
			}

			iSuccNodeFlags = WaypointFlags(iSuccNode);

			if ( (iBotTeam != -1) && (iSuccNodeFlags & W_FL_TEAM_SPECIFIC) &&
				((iSuccNodeFlags & W_FL_TEAM) != iBotTeam))
			{
				continue;
			}
			
			// unreachable waypoint
			if ( iSuccNodeFlags & W_FL_UNREACHABLE )
				continue;
			
			if ( bDetpackWpt )
			{
				if ( iSuccNodeFlags & W_FL_OPENS_LATER )
				{
					if ( gBotGlobals.IsMod(MOD_TFC) && (pBot->pev->playerclass == TFC_CLASS_DEMOMAN) )
					{
						// first do a traceline
						UTIL_TraceLine(vOrigin,WaypointOrigin(iSuccNode),ignore_monsters,dont_ignore_glass,NULL,&tr);						
						
						// closed
						if ( tr.flFraction < 1.0 )
						{													
							if ( !pBot->iDetPackWaypoints.IsMember(iCurrentNode) )
								pBot->iDetPackWaypoints.Add(iCurrentNode);
						}						
					}
				}
			}

			if ( iSuccNodeFlags & W_FL_CROUCH )
			{
				if ( gBotGlobals.IsNS() && pBot->IsOnos() )
					continue;
			}
			
			if ( gBotGlobals.IsMod(MOD_SVENCOOP) || gBotGlobals.IsMod(MOD_TFC) )
			{ 
				if ( !gBotGlobals.IsMod(MOD_TFC) && ((iSuccNodeFlags & W_FL_GREN_THROW) && !bCanThrowGren) )
					continue; // can't throw a grenade at a "throw gren waypoint"

				if ( iSuccNodeFlags & W_FL_PAIN )
				{
					edict_t *pent = NULL;
					BOOL bFound = FALSE;
					edict_t *pNearest = NULL;
					float fNearestDist = 0;

					while ( (pent = UTIL_FindEntityInSphere(pent,WaypointOrigin(iSuccNode),64)) != NULL )
					{
						if ( !strcmp("trigger_hurt",STRING(pent->v.classname)) )							
						{
							if ( !(pent->v.spawnflags & 8) && (pent->v.solid == SOLID_TRIGGER) )
							{
								float fDist = (AbsOrigin(pent)-WaypointOrigin(iSuccNode)).Length();

								if ( !pNearest || (fDist < fNearestDist) )
								{
									pNearest = pent;
									fNearestDist = fDist;
								}
							}

						}
					}

					if ( pNearest )
						continue;	
				}
			}
			
			if ( gBotGlobals.IsNS() && iSuccNodeFlags & W_FL_WALL_STICK )
			{
				if ( !bBotCanUseWallStickWpt )
				{
					// bot cant climb/fly
					continue;				
				}
			}
			
			if ( (iSuccNodeFlags & W_FL_FLY) && !bBotCanUseFlyWpt )
			{
				// if its also a wall stick waypoint and can use it
				if ( (iSuccNodeFlags & W_FL_WALL_STICK) && bBotCanUseWallStickWpt )
				{
					// nothing
				}
				else // bot can't fly			
					continue;
			}

			
			if ( iSuccNodeFlags & W_FL_OPENS_LATER ) 
			{				
				// Svencoop open later waypoint
				// check if the bot can go through to the next area
				// if not then skip this waypoint from being read
				UTIL_TraceLine(vOrigin,WaypointOrigin(iSuccNode),ignore_monsters,dont_ignore_glass,NULL,&tr);
				
				// first do a traceline

				if ( tr.flFraction < 1.0 )
					// hit something?
				{
					if ( gBotGlobals.m_iCurrentMod == MOD_SVENCOOP )
					{
						CBaseEntity *pentArea;
						CBaseEntity *pentActivator;
						char *szClassname;
						eMasterType iMasterVal;

						szClassname = (char*)STRING(tr.pHit->v.classname);

						if ( !tr.pHit )
							continue; // hit something we can't check
						if ( strcmp(szClassname,"worldspawn") == 0 )
							continue; // hit a wall that can't be opened
						
						pentArea = (CBaseEntity*)GET_PRIVATE(tr.pHit);
						pentActivator = (CBaseEntity*)GET_PRIVATE(pBot->m_pEdict);
						
						if ( pentArea && pentActivator && !pentArea->IsTriggered(pentActivator) )
							continue; // it can't be opened yet..
						
						// get master value
						iMasterVal = gBotGlobals.m_Masters.EntityCanFire(tr.pHit,pBot->m_pEdict);
											
						switch ( iMasterVal )
						{
						case MASTER_NONE:
							// bot cant open it without using something?
							{
								char *szTargetname = (char*)STRING(tr.pHit->v.targetname);
								
								if ( szTargetname && *szTargetname )
								{
									edict_t *pTarget = UTIL_FindEntityByTarget(NULL,szTargetname);
										
									if ( pTarget )
									{
										if ( gBotGlobals.m_Masters.EntityCanFire(pTarget,pBot->m_pEdict) == MASTER_TRIGGERED )
											break;										
									}

									continue;
								}
							}
							break;
						case MASTER_FAULT:
						case MASTER_NOT_TRIGGERED:
							continue;
						case MASTER_TRIGGERED:
							break;
						default:
							break;

						}							
					}
					else
					{
						continue;
					}
				}// didn't hit anything, so its open
			}
			
			fCurCost = sCurrNodeInfo->m_fCost;
			
			bIsTeleport = (pCurWpt->flags & W_FL_TELEPORT)==W_FL_TELEPORT;
			// if not a teleport add cost from taking this path
			if ( !bIsTeleport )
				fCurCost += (waypoints[iSuccNode].origin - waypoints[iCurrentNode].origin).Length();
			
            // for each node_successor of node_current                      
            // Set the cost of node_successor to be the cost of node_current plus the cost to get to node_successor from node_current  
			
            sSuccNodeInfo = &aPathsFound[iSuccNode];
            
            if( sSuccNodeInfo->isOpen() || (sSuccNodeInfo->isClosed()))
            {     
				if ( sSuccNodeInfo->hasParent() ) // Already has a parent path? Seems to speed up A*
				{
					fOldCost = sSuccNodeInfo->m_fCost;

					if ( fOldCost < fCurCost ) // The current path stored is better than this one
						continue; // Ignore this route...
				}
				else
					continue;
            }

			pSuccWpt = &waypoints[iSuccNode];
            
			sSuccNodeInfo->unClose();

            sSuccNodeInfo->setParent(iCurrentNode);  
			
			sSuccNodeInfo->m_fCost = fCurCost;

			// Make the successor node open.			
			sSuccNodeInfo->m_iWaypoint = iSuccNode;

			// stays fixed, function only does this once to save time
			if ( !sSuccNodeInfo->heuristicSet () )
				sSuccNodeInfo->setHeuristic(pBot->DistanceFrom(pSuccWpt->origin),(vGoalWptOrigin - pSuccWpt->origin).Length(),bIsTeleport);

			if ( !sSuccNodeInfo->isOpen() )
			{
				// open list REQUIRES cost and heuristic set for prioritising!!!
				sOpenList->add(sSuccNodeInfo);
				sSuccNodeInfo->open();
			}
        } 
        
		sCurrNodeInfo->close();

		iPrevWpt = iCurrentNode;

		// Has this waypoint got a path to itself? (delete the path)...
        if ( bDeleteLoopPath ) 
            WaypointDeletePath(iCurrentNode,iCurrentNode);
	
	}

	if (testOnly) {
		return bFoundGoal ? 0 : -1;
	}

	// looped out! too many loops
	if ( !bFoundGoal && (iLoops >= iMaxLoops) )
		return -2; // tell the program we need to continue next time

	// free all the stuff in the openlist
	while ( !sOpenList->empty() )
		sOpenList->pop();

	// Bot could have just failed a lot of paths and ignored them
	// Clear the failed waypoints / goals structure
	if ( !bFoundGoal )
	{
		if ( gBotGlobals.IsDebugLevelOn(BOT_DEBUG_NAV_LEVEL) )
			DebugMessage(BOT_DEBUG_NAV_LEVEL,NULL,0,"AStarAlgo() %s could not find path from wpt: %d to wpt: %d",pBot->m_szBotName,iFrom,iTo);

		if ( !pBot->m_stFailedPaths.ClearViolatedPaths() )
		{
			if ( !pBot->m_FailedGoals.IsMember(iTo) )
			{
				pBot->m_FailedGoals.Push(iTo);
				pBot->m_fNextClearFailedGoals = gpGlobals->time + RANDOM_FLOAT(5.0,10.0);
			}
			else
				pBot->m_FailedGoals.Remove(iTo);
		}

		return -1;
	}

	if ( bFoundGoal )
	{
        iCurrentNode = iTo;

		iLoops = 0;

		// Free any previous path info
		pBot->m_stBotPaths.Destroy();
		
        while ( (iCurrentNode != -1) && (iCurrentNode != iFrom) && (iLoops < MAX_WAYPOINTS) )
        {       
			iLoops ++;
            // Add to the bots waypoint visit stack (path)
			pBot->m_stBotPaths.Push(iCurrentNode);			

			iNextWaypoint = iCurrentNode;

			sSuccNodeInfo = &aPathsFound[iCurrentNode];

			// Update bots next waypoint after current waypoint
			//if ( sSuccNodeInfo->m_iParent == iFrom )
			//	pBot->m_iNextWaypoint = iCurrentNode;

			iCurrentNode = sSuccNodeInfo->getParent();
        }

		if ( iLoops == MAX_WAYPOINTS )
		{
			if ( gBotGlobals.IsDebugLevelOn(BOT_DEBUG_NAV_LEVEL) )
				DebugMessage(BOT_DEBUG_NAV_LEVEL,NULL,0,"AStarAlgo() %s Too many waypoints to go to",pBot->m_szBotName);

			pBot->m_stBotPaths.Destroy();

			return -1;
		}
	}

	return iNextWaypoint;
}

void BotTurnAtWall( CBot *pBot, TraceResult *tr )
{
   edict_t *pEdict = pBot->m_pEdict;
   Vector Normal;
   float Y, Y1, Y2, D1, D2, Z;

   // Find the normal vector from the trace result.  The normal vector will
   // be a vector that is perpendicular to the surface from the TraceResult.

   Normal = UTIL_VecToAngles(tr->vecPlaneNormal);

   // Since the bot keeps it's view angle in -180 < x < 180 degrees format,
   // and since TraceResults are 0 < x < 360, we convert the bot's view
   // angle (yaw) to the same format at TraceResult.

   Y = pEdict->v.v_angle.y;
   Y = Y + 180;
   if (Y > 359) Y -= 360;

   // Turn the normal vector around 180 degrees (i.e. make it point towards
   // the wall not away from it.  That makes finding the angles that the
   // bot needs to turn a little easier.

   Normal.y = Normal.y - 180;
   if (Normal.y < 0)
   Normal.y += 360;

   // Here we compare the bots view angle (Y) to the Normal - 90 degrees (Y1)
   // and the Normal + 90 degrees (Y2).  These two angles (Y1 & Y2) represent
   // angles that are parallel to the wall surface, but heading in opposite
   // directions.  We want the bot to choose the one that will require the
   // least amount of turning (saves time) and have the bot head off in that
   // direction.

   Y1 = Normal.y - 90;
   if (RANDOM_LONG(1, 100) <= 50)
   {
      Y1 = Y1 - RANDOM_FLOAT(5.0, 20.0);
   }
   if (Y1 < 0) Y1 += 360;

   Y2 = Normal.y + 90;
   if (RANDOM_LONG(1, 100) <= 50)
   {
      Y2 = Y2 + RANDOM_FLOAT(5.0, 20.0);
   }
   if (Y2 > 359) Y2 -= 360;

   // D1 and D2 are the difference (in degrees) between the bot's current
   // angle and Y1 or Y2 (respectively).

   D1 = fabs(Y - Y1);
   if (D1 > 179) D1 = fabs(D1 - 360);
   D2 = fabs(Y - Y2);
   if (D2 > 179) D2 = fabs(D2 - 360);

   // If difference 1 (D1) is more than difference 2 (D2) then the bot will
   // have to turn LESS if it heads in direction Y1 otherwise, head in
   // direction Y2.  I know this seems backwards, but try some sample angles
   // out on some graph paper and go through these equations using a
   // calculator, you'll see what I mean.

   if (D1 > D2)
      Z = Y1;
   else
      Z = Y2;

   // convert from TraceResult 0 to 360 degree format back to bot's
   // -180 to 180 degree format.

   if (Z > 180)
      UTIL_FixFloatAngle(&Z);

   pEdict->v.ideal_yaw = Z;
}

BOOL BotCantMoveForward( CBot *pBot, TraceResult *tr )
{
   edict_t *pEdict = pBot->m_pEdict;

   // use some TraceLines to determine if anything is blocking the current
   // path of the bot.

   Vector v_src, v_forward;

   UTIL_MakeVectors( pEdict->v.v_angle );

   // first do a trace from the bot's eyes forward...

   v_src = pEdict->v.origin + pEdict->v.view_ofs;  // EyePosition()
   v_forward = v_src + gpGlobals->v_forward * 40;

   // trace from the bot's eyes straight forward...
   UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
      return !BotCanDuckUnder(pBot);  // bot's head will hit something
   }

   // bot's head is clear, check at waist level...

   v_src = pEdict->v.origin;
   v_forward = v_src + gpGlobals->v_forward * 40;

   // trace from the bot's waist straight forward...
   UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
      return !BotCanJumpUp(pBot);  // bot's body will hit something
   }

   return FALSE;  // bot can move forward, return false
}


BOOL BotCanJumpUp( CBot *pBot ) // BotCanJumpUp : By : Botman
{
   // What I do here is trace 3 lines straight out, one unit higher than
   // the highest normal jumping distance.  I trace once at the center of
   // the body, once at the right side, and once at the left side.  If all
   // three of these TraceLines don't hit an obstruction then I know the
   // area to jump to is clear.  I then need to trace from head level,
   // above where the bot will jump to, downward to see if there is anything
   // blocking the jump.  There could be a narrow opening that the body
   // will not fit into.  These horizontal and vertical TraceLines seem
   // to catch most of the problems with falsely trying to jump on something
   // that the bot can not get onto.

   TraceResult tr;
   Vector v_jump, v_source, v_dest;
   edict_t *pEdict = pBot->m_pEdict;

   // convert current view angle to vectors for TraceLine math...

   v_jump = pEdict->v.v_angle;
   v_jump.x = 0;  // reset pitch to 0 (level horizontally)
   v_jump.z = 0;  // reset roll to 0 (straight up and down)

   UTIL_MakeVectors( v_jump );

   // use center of the body first...

   // maximum jump height is 45, so check one unit above that (46)
   v_source = pEdict->v.origin + Vector(0, 0, -36 + (MAX_JUMP_HEIGHT+1));
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at maximum jump height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * 16 + Vector(0, 0, -36 + 46);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at maximum jump height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * -16 + Vector(0, 0, -36 + 46);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at maximum jump height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now trace from head level downward to check for obstructions...

   // start of trace is 24 units in front of bot, 72 units above head...
   v_source = pEdict->v.origin + gpGlobals->v_forward * 24;

   // offset 72 units from top of head (72 + 36)
   v_source.z = v_source.z + 108;

   // end point of trace is 99 units straight down from start...
   // (99 is 108 minus the jump limit height which is 45 - 36 = 9)
   v_dest = v_source + Vector(0, 0, -99);

   // trace a line straight down toward the ground...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * 16 + gpGlobals->v_forward * 24;
   v_source.z = v_source.z + 108;
   v_dest = v_source + Vector(0, 0, -99);

   // trace a line straight down toward the ground...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * -16 + gpGlobals->v_forward * 24;
   v_source.z = v_source.z + 108;
   v_dest = v_source + Vector(0, 0, -99);

   // trace a line straight down toward the ground...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   return TRUE;
}


BOOL BotCanDuckUnder( CBot *pBot )
{
   // What I do here is trace 3 lines straight out, one unit higher than
   // the ducking height.  I trace once at the center of the body, once
   // at the right side, and once at the left side.  If all three of these
   // TraceLines don't hit an obstruction then I know the area to duck to
   // is clear.  I then need to trace from the ground up, 72 units, to make
   // sure that there is something blocking the TraceLine.  Then we know
   // we can duck under it.

   TraceResult tr;
   Vector v_duck, v_source, v_dest;
   edict_t *pEdict = pBot->m_pEdict;

   // convert current view angle to vectors for TraceLine math...

   v_duck = pEdict->v.v_angle;
   v_duck.x = 0;  // reset pitch to 0 (level horizontally)
   v_duck.z = 0;  // reset roll to 0 (straight up and down)

   UTIL_MakeVectors( v_duck );

   // use center of the body first...

   // duck height is 36, so check one unit above that (37)
   v_source = pEdict->v.origin + Vector(0, 0, -36 + 37);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * 16 + Vector(0, 0, -36 + 37);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * -16 + Vector(0, 0, -36 + 37);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now trace from the ground up to check for object to duck under...

   // start of trace is 24 units in front of bot near ground...
   v_source = pEdict->v.origin + gpGlobals->v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up

   // end point of trace is 72 units straight up from start...
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction >= 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * 16 + gpGlobals->v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction >= 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * -16 + gpGlobals->v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction >= 1.0)
      return FALSE;

   return TRUE;
}

BOOL BotCheckWallOnLeft( CBot *pBot )
{
   edict_t *pEdict = pBot->m_pEdict;
   Vector v_src, v_left;
   TraceResult tr;

   UTIL_MakeVectors( pEdict->v.v_angle );

   // do a trace to the left...

   v_src = pEdict->v.origin;
   v_left = v_src + gpGlobals->v_right * -40;  // 40 units to the left

   UTIL_TraceLine( v_src, v_left, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
   {
      if (pBot->m_fWallAtLeftTime < 1.0)
         pBot->m_fWallAtLeftTime = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}


BOOL BotCheckWallOnRight( CBot *pBot )
{
   edict_t *pEdict = pBot->m_pEdict;
   Vector v_src, v_right;
   TraceResult tr;

   UTIL_MakeVectors( pEdict->v.v_angle );

   // do a trace to the right...

   v_src = pEdict->v.origin;
   v_right = v_src + gpGlobals->v_right * 40;  // 40 units to the right

   UTIL_TraceLine( v_src, v_right, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
   {
      if (pBot->m_fWallAtRightTime < 1.0)
         pBot->m_fWallAtRightTime = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}

int BotNavigate_FindNextWaypoint ( CBot *pBot )
{
	extern CWaypointLocations WaypointLocations;

	int iIndex = -1;	
	
	if ( (pBot->m_iWaypointGoalIndex != -1)/* && (pBot->m_iPrevWaypointGoalIndex == pBot->m_iWaypointGoalIndex)*/ )
	{
		// Need to find path?
		if ( !pBot->m_stBotPaths.IsEmpty() )
		// Still some more waypoints to go before we reach the goal
		{
			iIndex = pBot->m_stBotPaths.Pop();

			if ( iIndex != pBot->m_iCurrentWaypointIndex )
			{
				// Bot missed a waypoint
				if ( gBotGlobals.IsDebugLevelOn(BOT_DEBUG_NAV_LEVEL) )				
					DebugMessage(BOT_DEBUG_NAV_LEVEL,NULL,0,"%s missed out a waypoint in path",pBot->m_szBotName);
				// rework path

				// See if current waypoint index is somewhere else in the stack...

				while ( !pBot->m_stBotPaths.IsEmpty() )
				{
					iIndex = *pBot->m_stBotPaths.GetHeadInfoPointer();

					if ( iIndex == pBot->m_iCurrentWaypointIndex )
						break;

					// pop the front node and discard it.
					pBot->m_stBotPaths.Pop();
					// Keep looping until its found or not.
				}

				if ( !pBot->m_stBotPaths.IsEmpty() ) // found it
					return iIndex;
				
				if ( pBot->m_iPrevWaypointIndex != -1 )
				{
					// Has a previous waypoint
					// we can work out the path the bot tried to use.
					PATH *pPath = BotNavigate_FindPathFromTo(pBot->m_iPrevWaypointIndex,pBot->m_iCurrentWaypointIndex,pBot->m_iTeam);

					if ( pPath )
						pBot->m_stFailedPaths.AddFailedPath(pPath);									
				}
			}
			else if ( !pBot->m_stBotPaths.IsEmpty() )
			{
				// still got waypoints to go...

				iIndex = *pBot->m_stBotPaths.GetHeadInfoPointer();

				return iIndex;
			}	
			else
			{
				// reached goal, will drop below and get new goal :D

				// pBot->m_iWaypointGoalIndex = -1;

				if ( gBotGlobals.IsDebugLevelOn(BOT_DEBUG_NAV_LEVEL) )				
					DebugMessage(BOT_DEBUG_NAV_LEVEL,NULL,0,"%s reached goal waypoint",pBot->m_szBotName);				
			}
		}		
	}	

	// Bot needs to find a goal...
	pBot->m_stBotPaths.Destroy();
	// Just incase, free the goto stack

	if ( pBot->m_iWaypointGoalIndex == -1 )
	{
		// Lets get a new goal
		pBot->m_iWaypointGoalIndex = WaypointFindRandomGoal(pBot->m_pEdict,pBot->m_iTeam,&pBot->m_FailedGoals);
	}

	if ( pBot->m_iCurrentWaypointIndex == -1 )
		pBot->m_iCurrentWaypointIndex = WaypointLocations.NearestWaypoint(pBot->pev->origin,REACHABLE_RANGE,pBot->m_iLastFailedWaypoint,TRUE,FALSE,TRUE);

	if ( pBot->m_iCurrentWaypointIndex == -1 )
	{
		if ( gBotGlobals.IsDebugLevelOn(BOT_DEBUG_NAV_LEVEL) )
				DebugMessage(BOT_DEBUG_NAV_LEVEL,NULL,0,"BotFindNextWaypoint() %s has no current waypoint",pBot->m_szBotName);

		return FALSE;
	}	

	return -1;
}

extern PATH* paths[MAX_WAYPOINTS];

BOOL BotNavigate_UpdateWaypoint ( CBot *pBot )
// updates the bot's current waypoint
// checks if the bot has touched its waypoint
// if yes, then it picks the next waypoint
{
	float fDistance;

	WAYPOINT *wCurWpt;

	Vector vBotOrigin = pBot->pev->origin;
	Vector vWptOrigin;
	Vector vMoveVector;

	int iCurrWpt;
	int iWptFlags;

	BOOL bTouchedWpt = FALSE;

	iCurrWpt = pBot->m_iCurrentWaypointIndex;	

	// No goal index
	if ( pBot->m_iWaypointGoalIndex == -1 )
	{		
		if ( pBot->m_pLastEnemy && EntityIsAlive(pBot->m_pLastEnemy) )
			pBot->m_CurrentLookTask = BOT_LOOK_TASK_SEARCH_FOR_LAST_ENEMY;
		else
			pBot->m_CurrentLookTask = BOT_LOOK_TASK_LOOK_AROUND;
		
		if ( pBot->m_iCurrentWaypointIndex == -1 )
		{
			if ( pBot->m_bNotFollowingWaypoint )
			{
				pBot->m_iCurrentWaypointIndex = WaypointLocations.NearestWaypoint(vBotOrigin,200.0,-1,FALSE);
			}

			if ( pBot->m_iCurrentWaypointIndex == -1 )
			{
				BOOL getNew = !pBot->m_bMoveToIsValid;

				if ( !getNew )
				{
					getNew = ( pBot->DistanceFrom(pBot->m_vMoveToVector,TRUE) < BOT_WAYPOINT_TOUCH_DIST );
				}
				
				if ( getNew )
				{
					// use my awful non-navigated method
					pBot->SetMoveVector(BotNavigate_ScanFOV(pBot));
					
					pBot->m_bNotFollowingWaypoint = TRUE;
				}
			}
			//else
			//	pBot->m_bMoveToIsValid = TRUE;
		}
	}

	if ( iCurrWpt != -1 )
	{		
		// Bot can't see this waypoint...?
		if ( !pBot->HasCondition(BOT_CONDITION_SEE_NEXT_WAYPOINT) )
		{
			float fMaxWaypointSeeTime = 2.0;

			if ( gBotGlobals.IsNS() && pBot->IsSkulk() && (pBot->m_iCurrentWaypointFlags & W_FL_WALL_STICK) )
				fMaxWaypointSeeTime = 10.0;
			// Bot hasn't seen this waypoint for two seconds
			if ( pBot->m_fLastSeeWaypoint && (pBot->m_fLastSeeWaypoint + fMaxWaypointSeeTime) <= gpGlobals->time )
			{
				//Clear this waypoint, get a new one and flush path info.

				PATH *pFailed = BotNavigate_FindPathFromTo(pBot->m_iPrevWaypointIndex,pBot->m_iCurrentWaypointIndex,pBot->m_iTeam);
				
				if ( pFailed )
					pBot->m_stFailedPaths.AddFailedPath(pFailed);

				pBot->m_iCurrentWaypointIndex = WaypointLocations.NearestWaypoint(vBotOrigin,REACHABLE_RANGE,pBot->m_iLastFailedWaypoint,TRUE,FALSE,TRUE);
				iCurrWpt = pBot->m_iCurrentWaypointIndex;

				CBotTask *pCurrentTask = pBot->m_Tasks.CurrentTask();

				if ( pCurrentTask )
				{
					pCurrentTask->SetPathInfo(FALSE);
				}

				if ( pBot->m_iCurrentWaypointIndex != -1 )
					pBot->m_fPrevWaypointDist = WaypointDistance(pBot->pev->origin,pBot->m_iCurrentWaypointIndex);

				pBot->m_fLastSeeWaypoint = 0.0;
			}
			else if ( pBot->m_fLastSeeWaypoint == 0 )
				pBot->m_fLastSeeWaypoint = gpGlobals->time;
		}		
	}

	if ( iCurrWpt == -1 )
	{
		// no current waypoint, get one
		pBot->m_iCurrentWaypointIndex = WaypointLocations.NearestWaypoint(vBotOrigin,REACHABLE_RANGE,pBot->m_iLastFailedWaypoint,TRUE,FALSE,TRUE);
		
		if (pBot->m_iCurrentWaypointIndex == -1)
		{
			if ( gBotGlobals.IsDebugLevelOn(BOT_DEBUG_NAV_LEVEL) )
				DebugMessage(BOT_DEBUG_NAV_LEVEL,NULL,0,"UpdateWaypoint() %s can't find a current waypoint",pBot->m_szBotName);		
			
			return FALSE;
		}
		else
			pBot->m_fPrevWaypointDist = WaypointDistance(pBot->pev->origin,pBot->m_iCurrentWaypointIndex);

		iCurrWpt = pBot->m_iCurrentWaypointIndex;
	}

	pBot->m_bNotFollowingWaypoint = FALSE;

	wCurWpt = &waypoints[iCurrWpt];

	vWptOrigin = wCurWpt->origin;
	iWptFlags = wCurWpt->flags;

	fDistance = 0;

	vMoveVector = vWptOrigin;
	
	// bot is not climbing
	if ( pBot->GetClimbType() != BOT_CLIMB_NONE )
		fDistance = (vWptOrigin - vBotOrigin).Length();
	else
	{
		float fzComp = (vWptOrigin.z - pBot->pev->origin.z);
		
		if ( fzComp < 0 )
		{
			fzComp = -fzComp;
		}

		if ( fzComp < 72 )
			fDistance = (vWptOrigin - vBotOrigin).Length2D();
		else
			fDistance = (vWptOrigin - vBotOrigin).Length();
	}

	// bot trying to get on a ladder/fly waypoint and is currently on a ladder
	if ( WaypointFlagsOnLadderOrFly(iWptFlags) || (pBot->GetClimbType() == BOT_CLIMB_CLIMBING) )
	{
		int iNextWaypoint = pBot->GetNextWaypoint();

		// ERR!!!
		if ( iNextWaypoint == iCurrWpt )
		{
			bTouchedWpt = TRUE;			
		}
		else if ( pBot->m_iCurrentWaypointFlags & W_FL_FLY )
		{
			if ( !pBot->CanFly() ) // WTF?????
				bTouchedWpt = TRUE;
			else
				bTouchedWpt = (vWptOrigin.z < pBot->pev->absmin.z) && ((vWptOrigin - vBotOrigin).Length2D() < 96);
		}
		else if ( (iNextWaypoint != -1) && WaypointFlagsOnLadderOrFly(waypoints[iNextWaypoint].flags) )
		{
			int iLadderDir = pBot->GetLadderDir();
			int iNextLadderDir = pBot->GetLadderDir(TRUE);
			BOOL bTouchingLadder = (gBotGlobals.IsNS() && pBot->IsLerk()) || ( (vWptOrigin - vBotOrigin).Length2D() <= (pBot->pev->size.Length2D()/2) );

			bTouchedWpt = bTouchingLadder;

			if ( !bTouchingLadder )
				bTouchedWpt = FALSE;
			else
			{
				switch ( iLadderDir )
				{
				case 0:
					bTouchedWpt = TRUE;
					break;
				case -1:
					switch ( iNextLadderDir )
					{
					case 0:
						bTouchedWpt = TRUE;
						break;
					case -1:
						bTouchedWpt = (vWptOrigin.z >= vBotOrigin.z);
						break;
					case 1:
						// bot has gone passed the waypoint and is going in the right direction
						bTouchedWpt = TRUE;
						break;
					}
					break;
				case 1:
					switch ( iNextLadderDir )
					{
					case 0:
						bTouchedWpt = TRUE;
						break;
					case -1:
						// bot has gone passed the waypoint and is going in the right direction
						bTouchedWpt = TRUE;
						break;
					case 1:
						bTouchedWpt = (vWptOrigin.z <= vBotOrigin.z);
						break;
					}
					break;
				}
			}
		}	
		else
		{
			//
			// On ladder, and current waypoint trying to get to is not on ladder
			// and is some 2d distance away.. jump off ladder
			if ( pBot->IsOnLadder() && !(pBot->m_iCurrentWaypointFlags & W_FL_LADDER) && (iNextWaypoint != -1) && (pBot->m_iPrevWaypointIndex != -1))
			{
				if ( (WaypointOrigin(pBot->m_iCurrentWaypointIndex).z <= WaypointOrigin(pBot->m_iPrevWaypointIndex).z) && ((vBotOrigin - WaypointOrigin(iNextWaypoint)).Length2D() > 100.0) )
				{
					if ( RANDOM_LONG(0,1)) 
						pBot->Jump();
				}
			}
			// Make lerk stay away from the ladder, it will bounce off it
			// otherwise make sure we are near it 2 dimensionally aswell
			if ( pBot->IsLerk() || ((vWptOrigin - vBotOrigin).Length2D() <= (pBot->pev->size.Length2D()/2)) )
			{
				float fzComp = (vWptOrigin.z - pBot->pev->origin.z);
				
				if ( fzComp < 0 )
				{						
					fzComp = -fzComp;
				}
				
				bTouchedWpt = (fzComp <= (pBot->pev->size.z/2));
			}
			else
				bTouchedWpt = FALSE;
		}
	}
	// human tower waypoint
	else if ( iWptFlags & W_FL_HUMAN_TOWER )
	{
		// bot needs to check if a bot/player is already crouching here
		edict_t *pPlayer = NULL;

		while ( (pPlayer = UTIL_FindEntityInSphere(pPlayer,vWptOrigin,24.0)) != NULL )
		{
			if ( pPlayer->v.flags & FL_CLIENT )
			{
				if ( !*STRING(pPlayer->v.netname) )
					continue;
				if ( ( pPlayer->v.flags & FL_DUCKING ) || ( pPlayer->v.button & IN_DUCK ) )
				{
					break;
				}
			}
		}

		if ( pBot->pev->origin.z < (vWptOrigin.z - 16.0) ) // not high enough yet
			bTouchedWpt = FALSE;
		// player in way so cant get much closer, start human tower here.
		else if ( pPlayer )
		{
			bTouchedWpt = (fDistance <= 100);
		}
		else // get closer before crouching
			bTouchedWpt = (fDistance <= 24);

	}
	else if (iWptFlags & W_FL_PAIN)
	{
		bool hasAnyPath = false;
		PATH* p = paths[iCurrWpt];

		while (p != NULL && !hasAnyPath)
		{
			int n = 0;

			while (n < MAX_PATH_INDEX) {
				if (p->index[n] != -1) {
					hasAnyPath = true;
					break;
				}

				n++;
			}

			p = p->next;  // go to next node in linked list
		}
		if (!hasAnyPath && pBot->IsAlive() && (pBot->pev->origin - vWptOrigin).Length() < 32) {
			if (gBotGlobals.IsDebugLevelOn(BOT_DEBUG_THINK_LEVEL)) {
				DebugMessage(BOT_DEBUG_THINK_LEVEL, gBotGlobals.m_pListenServerEdict, 0, "%s killed themselves after reaching a pain/death waypoint with no outward paths", pBot->m_szBotName);
			}
			FakeClientCommand(pBot->m_pEdict, "gibme");
		}
	}

	/*
	
	if ( (!pBot->IsSkulk() && !pBot->IsLerk()) && (pBot->IsOnLadder() || FBitSet(pBot->m_iCurrentWaypointFlags,W_FL_LADDER)) )
		// Is the bot on a ladder or going on a ladder waypoint??
	{
		if ( fDistance < 8 )
			bTouchedWpt = TRUE;
		else if ( !pBot->IsOnLadder() )
			bTouchedWpt = FALSE;
		else if ( pBot->IsSkulk() && !pBot->IsWallSticking() )
			bTouchedWpt = FALSE;
		else
		{
			float fMin;
			short int siLadderDir = pBot->m_siLadderDir;
			
			entvars_t *pev = &pBot->m_pEdict->v;
			
			fMin = pev->absmin.z;
			
			if ( siLadderDir == BOT_LADDER_UNKNOWN )
				// Hasn't figured out a ldder direction yet...
			{
				bTouchedWpt = TRUE;
			}
			else if ( siLadderDir == BOT_LADDER_UP )
				// If the bot is going up we want the bots feet to be 
				// above the botton of the waypoint
			{
				bTouchedWpt = fMin >= (vWptOrigin.z - (pev->size.z/2));
			}
			else if ( siLadderDir == BOT_LADDER_DOWN )
				// If going doing make sure the bots feet are about 
				// the same or lower than the waypoint origin.
			{
				bTouchedWpt = fMin <= vWptOrigin.z;
			}
		}
	}*/
	else if ( (iWptFlags & W_FL_STAY_NEAR) || (iWptFlags & W_FL_CROUCH) )
	{
		if ( fDistance <= 24.0 )
			bTouchedWpt = TRUE;
		else if ( fDistance <= 64.0 )
		{
			bTouchedWpt = ( PlayerNearVector(vWptOrigin,24.0) != NULL );
		}
	}
	else if ( (iWptFlags & W_FL_JUMP) || (iWptFlags & W_FL_CROUCHJUMP) || (gBotGlobals.IsMod(MOD_TS) && (iWptFlags & W_FL_STUNT)) )
	{
		if ( fDistance < 32.0 )
			bTouchedWpt = TRUE;
	}
	else if ( (pBot->DistanceFrom(vWptOrigin,TRUE) < BOT_WAYPOINT_TOUCH_DIST) && (fabs(vWptOrigin.z-vBotOrigin.z) <= pBot->pev->size.z)  )
	{
		bTouchedWpt = TRUE;
	}
	else if ( pBot->pev->waterlevel > 1)
	{
		bTouchedWpt = fDistance < (BOT_WAYPOINT_TOUCH_DIST * 2);
	}
	// normal waypoint walking
	else if ( pBot->m_pEnemy == NULL )
	{
		bTouchedWpt = (fDistance - 1) > pBot->m_fPrevWaypointDist;
	}

	// Acceptable touch distance
	if ( bTouchedWpt )
	{
		CBotTask *currentTask = pBot->m_Tasks.CurrentTask();

		pBot->touchedWpt();

		if ( currentTask )
		{
			if ( currentTask->Task() == BOT_TASK_FIND_PATH )
			{
				// don't take too long to reach next waypoint
				currentTask->SetTimeToComplete(RANDOM_FLOAT(20.0,30.0));
			}
		}

		pBot->m_fPrevWaypointDist = 9999.0;

		if ( pBot->m_iCurrentWaypointIndex == pBot->m_iWaypointGoalIndex )
		{
			if ( !pBot->m_FailedGoals.IsMember(pBot->m_iCurrentWaypointIndex) )
			{
				pBot->m_FailedGoals.Push(pBot->m_iCurrentWaypointIndex);
				pBot->m_fNextClearFailedGoals = gpGlobals->time + RANDOM_FLOAT(5.0,10.0);
			}
			else
				pBot->m_FailedGoals.Remove(pBot->m_iCurrentWaypointIndex);
		}

		// make sure bot resets angles if its still on ladder
		pBot->UnSetLadderAngleAndMovement();
		// update the previous waypoint
		pBot->m_iPrevWaypointIndex = pBot->m_iCurrentWaypointIndex;

		pBot->m_iCurrentWaypointFlags = WaypointFlags(pBot->m_iCurrentWaypointIndex);

		if ( pBot->m_iCurrentWaypointFlags & W_FL_JUMP )		
			pBot->JumpAndDuck();		
		else if ( pBot->m_iCurrentWaypointFlags & W_FL_CROUCHJUMP )
			pBot->DuckAndJump();
		if ( gBotGlobals.IsMod(MOD_TS) && ( pBot->m_iCurrentWaypointFlags & W_FL_STUNT ) )
		{
			BOOL kungfu = ( !pBot->m_pCurrentWeapon || (pBot->m_pCurrentWeapon->GetID()==36) );

			if ( (kungfu&&!pBot->m_pEnemy) || !kungfu )
				pBot->AltButton();
		}

		// pushable waypoint -- need a pushable here
		if ( pBot->m_iCurrentWaypointFlags & W_FL_PUSHABLE )
		{
			// find nearest pushable

			char *szEntity[1] = {"func_pushable"};
			
			// not already trying to push a pushable
			if ( !pBot->m_Tasks.HasTask(BOT_TASK_PUSH_PUSHABLE) )
			{
				// get nearest pushable
				edict_t *pPushable = UTIL_FindNearestEntity(szEntity,1,vWptOrigin,512,FALSE);
				
				if ( pPushable )
				{
					// if its too far away from the waypoint push it to the waypoint
					if ( !UTIL_AcceptablePushableVector(pPushable,vWptOrigin) )
					{
						int iSchedId = pBot->m_Tasks.GetNewScheduleId();
						
						pBot->AddPriorityTask(CBotTask(BOT_TASK_PUSH_PUSHABLE,iSchedId,pPushable,0,0,vWptOrigin));
						Vector vPush = UTIL_GetDesiredPushableVector(vWptOrigin,pPushable);
						pBot->AddPriorityTask(CBotTask(BOT_TASK_FIND_PATH,iSchedId,NULL,0,0,vPush));
						//WaypointDrawBeam(gBotGlobals.m_pListenServerEdict,vWptOrigin,vPush,20,10,200,200,200,100,10);
					}
				}
				
			}
		}

		// Reached objective waypoint???
		if ( pBot->m_iWaypointGoalIndex == pBot->m_iCurrentWaypointIndex )
		{
			CBotTask *m_CurrentTask = pBot->m_Tasks.CurrentTask();

			if ( m_CurrentTask )
			{
				if ( m_CurrentTask->Task() == BOT_TASK_FIND_PATH )
				{
					Vector vMoveTo = m_CurrentTask->TaskVector();
					edict_t *pMoveTo = m_CurrentTask->TaskEdict();
					if ( m_CurrentTask->TaskFloat() < 0 )
						pBot->m_Tasks.FinishedCurrentTask();
					else
					{
						pBot->m_Tasks.FinishedCurrentTask();
						
						if ( ((vMoveTo.z - pBot->pev->origin.z) <= MAX_JUMP_HEIGHT) && (pBot->DistanceFrom(vMoveTo) > BOT_WAYPOINT_TOUCH_DIST) )
							pBot->AddPriorityTask(CBotTask(BOT_TASK_MOVE_TO_VECTOR,pBot->m_Tasks.GetNewScheduleId(),pMoveTo,0,0,vMoveTo));						
					}
				}
			}

			pBot->StopMoving();

			pBot->m_iWaypointGoalIndex = -1;
			
			return FALSE;
		}
		
		int iPrevFlags = pBot->m_iCurrentWaypointFlags;

		pBot->m_iPrevWaypointIndex = pBot->m_iCurrentWaypointIndex;
		pBot->m_iCurrentWaypointIndex = BotNavigate_FindNextWaypoint(pBot);	

		if ( gBotGlobals.IsConfigSettingOn(BOT_CONFIG_BOTS_WAIT_FOR_BOTS) && (pBot->m_iCurrentWaypointIndex != -1) )
		{
			CBot *pOtherBot;
			
			if ( (pOtherBot = gBotGlobals.otherBotAtWaypoint(pBot,pBot->m_iCurrentWaypointIndex)) != NULL )
				pBot->AddPriorityTask(CBotTask(BOT_TASK_WAIT_FOR_BOT_AT_WPT,0,pOtherBot->m_pEdict,pBot->m_iCurrentWaypointIndex));
		}
		
		if ( pBot->m_iCurrentWaypointIndex == -1 )
		{
			//pBot->m_iWaypointGoalIndex = -1;			
			pBot->m_iCurrentWaypointFlags = 0;
			
			if ( gBotGlobals.IsDebugLevelOn(BOT_DEBUG_NAV_LEVEL) )
				DebugMessage(BOT_DEBUG_NAV_LEVEL,NULL,0,"UpdateWaypoint() %s has no current waypoint",pBot->m_szBotName);
			
			return FALSE;
		}
		else
		{
			vWptOrigin = WaypointOrigin(pBot->m_iCurrentWaypointIndex);
			pBot->m_iCurrentWaypointFlags = WaypointFlags(pBot->m_iCurrentWaypointIndex);

			if ( !gBotGlobals.IsMod(MOD_TFC) && (pBot->HasWeapon(VALVE_WEAPON_HANDGRENADE)  && (pBot->m_iCurrentWaypointFlags & W_FL_GREN_THROW)) )
			{
				pBot->AddPriorityTask(CBotTask(BOT_TASK_THROW_GRENADE,0,NULL,0,0,vWptOrigin));
			}
			else if ( (pBot->pev->flags & FL_ONGROUND) && (pBot->pev->waterlevel < 3) && (((pBot->m_iCurrentWaypointFlags & W_FL_JUMP)&&(iPrevFlags & W_FL_CROUCH)) || ((pBot->m_iCurrentWaypointFlags & W_FL_JUMP)&&(pBot->m_iCurrentWaypointFlags & W_FL_STAY_NEAR))) )
			{
				pBot->AddPriorityTask(CBotTask(BOT_TASK_WAIT_AND_FACE_VECTOR,0,NULL,0,0.75,vWptOrigin));
			}
			
			/*if ( pBot->m_iCurrentWaypointFlags & (W_FL_JUMP | W_FL_CROUCHJUMP) )
			{
				// stand still and face waypoint.
				pBot->AddPriorityTask(CBotTask(BOT_TASK_FACE_VECTOR,0,NULL,0,0.0,vWptOrigin));
			}*/

			int iNextwpt = pBot->GetNextWaypoint();

			if ( pBot->m_iCurrentWaypointFlags & W_FL_WAIT_FOR_LIFT )
				pBot->AddPriorityTask(CBotTask(BOT_TASK_WAIT_FOR_LIFT,0,NULL,0,0,vWptOrigin));
			else if ( (pBot->m_iCurrentWaypointFlags & (W_FL_JUMP|W_FL_CROUCHJUMP)) && ((iNextwpt != -1) && (WaypointFlags(iNextwpt) & W_FL_WAIT_FOR_LIFT)) )
				pBot->AddPriorityTask(CBotTask(BOT_TASK_WAIT_FOR_LIFT,0,NULL,0,0,WaypointOrigin(iNextwpt)));
			else if ( !pBot->m_Tasks.HasSchedule(BOT_SCHED_USE_LIFT) )
			{								
				BOOL bCheckNext = ( (iNextwpt != -1) && (pBot->m_iCurrentWaypointFlags & W_FL_CHECK_LIFT) );
				
				if ( bCheckNext && CheckLift(pBot,WaypointOrigin(pBot->m_iCurrentWaypointIndex),WaypointOrigin(iNextwpt)) )
				{
					;
				}
				else if ( CheckLift(pBot,WaypointOrigin(pBot->m_iPrevWaypointIndex),WaypointOrigin(pBot->m_iCurrentWaypointIndex)) )
				{
					;
				}
			}
		}

		BOOL bOnLadder = pBot->IsOnLadder();//pBot->GetClimbType() == BOT_CLIMB_CLIMBING;
		BOOL bWptOnLadder = (pBot->m_iCurrentWaypointFlags & W_FL_LADDER) == W_FL_LADDER;

		if ( !bOnLadder && bWptOnLadder )
		{
			if ( (vWptOrigin.z - 16)>pBot->pev->origin.z )
			{
				// try jumping onto ladder to avoid being stuck at the bottom.
				pBot->Jump();
			}
		}
		else if ( gBotGlobals.IsNS() && gBotGlobals.IsConfigSettingOn(BOT_CONFIG_BLINKING) && pBot->IsFade() && pBot->hasBlink() && (pBot->m_iCurrentWaypointFlags & (W_FL_WALL_STICK|W_FL_FLY)) )
		{
			pBot->AddPriorityTask(CBotTask(BOT_TASK_BLINK));
		}
		else if ( bOnLadder && !bWptOnLadder )
		{
			pBot->m_fStartDuckTime = gpGlobals->time;
			pBot->m_fEndDuckTime = gpGlobals->time + 1.0;
		}
		else if ( pBot->m_iCurrentWaypointFlags & W_FL_HUMAN_TOWER )
		{
			pBot->AddPriorityTask(CBotTask(BOT_TASK_HUMAN_TOWER,0,NULL,0,0,vWptOrigin));
		}
		/*else if ( pBot->m_iCurrentWaypointFlags & W_FL_TANK )//pBot->m_Tasks.HasSchedule(BOT_SCHED_USE_TANK))
		{
			edict_t *pTank = NULL;

			while ( (pTank = UTIL_FindEntityInSphere(pTank,vWptOrigin,120.0)) != NULL )
			{
				if ( strcmp(STRING(pTank->v.classname),"func_tank_controls") )
					break;
			}

			if ( pTank )
			{
				if ( pBot->CanUseTank(pTank) )
					pBot->UseTank(pTank);
			}
		}*/

	//	pBot->m_fPrevWaypointDist = WaypointDistance(pBot->pev->origin,pBot->m_iCurrentWaypointIndex);
	}
	else
	{
		pBot->m_fPrevWaypointDist = fDistance;
	}

	if ( pBot->m_iCurrentWaypointIndex == -1 )
	{
		pBot->m_iCurrentWaypointFlags = 0;

		if ( gBotGlobals.IsDebugLevelOn(BOT_DEBUG_NAV_LEVEL) )
			DebugMessage(BOT_DEBUG_NAV_LEVEL,NULL,0,"UpdateWaypoint() %s has no current waypoint",pBot->m_szBotName);

		return FALSE;
	}
	
	pBot->m_iCurrentWaypointFlags = WaypointFlags(pBot->m_iCurrentWaypointIndex);

	// Wall stick waypoint
	if ( gBotGlobals.IsNS() && (pBot->IsSkulk() && (pBot->m_iCurrentWaypointFlags & (W_FL_WALL_STICK | W_FL_LADDER)) && (pBot->m_iPrevWaypointIndex != -1)))
	{		
		Vector vWptOrigin = WaypointOrigin(pBot->m_iCurrentWaypointIndex);
		Vector vTemp = WaypointOrigin(pBot->m_iPrevWaypointIndex);

		vTemp.z = vWptOrigin.z;

		Vector vComp = (vWptOrigin - vTemp).Normalize();

		pBot->SetMoveVector(vWptOrigin+(vComp*(gBotGlobals.m_fWallStickTol)));

		pBot->m_CurrentLookTask = BOT_LOOK_TASK_NEXT_WAYPOINT;
		//WaypointDrawBeam(gBotGlobals.m_pListenServerEdict,pBot->pev->origin,pBot->m_vMoveToVector,20,10,200,200,200,100,10);
	}
	else
		pBot->SetMoveVector(WaypointOrigin(pBot->m_iCurrentWaypointIndex));

	return TRUE;
}

PATH *BotNavigate_FindPathFromTo ( int iFrom, int iTo, int iTeam )
{
	PATH *pPath = NULL;
	int iPathIndex = 0;
	int iIndex;
	
	while ( (iIndex = WaypointFindPath(&pPath,&iPathIndex,iFrom,iTeam)) != -1 )
	{
		if ( iIndex == iTo )
		{
			return pPath;
		}
	}

	return NULL;
}

/*CLocation :: CLocation (const char *szLocationName,edict_t *pEntity)
{
	FILE *fp;
	//char filename[256];
	
	this->Init();

	m_pEntity = pEntity;

	if ( szLocationName == NULL )
	{
		BotMessage(NULL,0,"Warning: CLocation() Received NULL Location");
		return;
	}

	fp = fopen("ns\\titles.txt","r");
	
	if ( fp == NULL )
	{
		GiveName(szLocationName);
		return;
	}
	
	char buffer[256];
	int buffer_len;
	
	BOOL found = FALSE;
	
	while ( fgets(buffer, 255, fp) != NULL)
	{
		buffer[255] = 0;

		buffer_len = strlen(buffer);
		
		if ( buffer[buffer_len-1] == '\n' )			
			buffer[--buffer_len] = 0;
		
		if ( found == FALSE )
		{
			if ( strcmp(szLocationName,buffer) == 0 )
				found = TRUE;
		}
		else
		{
			if ( buffer[0] == '{' )
				continue;
			else
			{
				GiveName(buffer);
				break;
			}
		}
	}

	if ( found == FALSE )
	{
		GiveName(szLocationName);
	}

	fclose(fp);
}

void CLocation :: GiveName ( const char *szLocationName )
{
	m_szLocationName = (char*)malloc((sizeof(char)*strlen(szLocationName))+1);
	
	if ( m_szLocationName == NULL )
	{
		BotMessage(NULL,0,"Can't Allocate Memory For Location Name : %s",szLocationName);
		return;
	}
	
	strcpy(m_szLocationName,szLocationName);	
}*/

Vector BotNavigate_ScanFOV ( CBot *pBot )
// Called when bot can't find a nearby waypoint
// instead fire tracelines in FOV and move to the furthest
// point it can find

// Thanks PM's racc bot source for some info pointers. (racc.bots-united.com)
{
	const float fFov = 60.0;
	int iStep = 0;

	entvars_t *pev = &pBot->m_pEdict->v;

	float fStartAngle = pev->angles.y - fFov;	
	float fAngle;

	int iMinStep = -60;
	int iMaxStep = 60;

	float fMaxDistance = BOT_WAYPOINT_TOUCH_DIST*2;
	float fDistance;

	dataUnconstArray<Vector> vPositions;

	Vector vStart = pBot->GetGunPosition();
	Vector vEnd;
	Vector vAngles;

	TraceResult tr;

	UTIL_FixFloatAngle(&fStartAngle);

	// angle
	fAngle = fStartAngle;

	for ( iStep = iMinStep; iStep <= iMaxStep; iStep += 10 )
	{
		fAngle = (float)iStep;

		vAngles = pBot->m_pEdict->v.angles;

		vAngles.y = vAngles.y + (fAngle-(iMaxStep/2));

		UTIL_FixFloatAngle(&vAngles.y);

		UTIL_MakeVectors(vAngles);

		vEnd = vStart + (gpGlobals->v_forward * REACHABLE_RANGE);

		UTIL_TraceLine(vStart,vEnd,ignore_monsters,ignore_glass,pBot->m_pEdict,&tr);

		vEnd = tr.vecEndPos;

		fDistance = pBot->DistanceFrom(vEnd);

		if ( fDistance > fMaxDistance )
		{
			vPositions.Clear();

			fMaxDistance = fDistance;

			vPositions.Add(vEnd);
		}
		else if ( !vPositions.IsEmpty() && (fDistance == fMaxDistance) )
		{
			vPositions.Add(vEnd);
		}
	}

	float fHighDistance = 0;

	////////////
	// copy for velocity
	UTIL_FixFloatAngle(&fStartAngle);

	fAngle = fStartAngle;

	for ( iStep = iMinStep; iStep <= iMaxStep; iStep += 15 )
	{
		fAngle = (float)iStep;

		vAngles = UTIL_VecToAngles(pBot->m_pEdict->v.velocity);

		vAngles.y = vAngles.y + (fAngle-(iMaxStep/2));

		UTIL_FixFloatAngle(&vAngles.y);

		UTIL_MakeVectors(vAngles);

		vEnd = vStart + (gpGlobals->v_forward * REACHABLE_RANGE);

		UTIL_TraceLine(vStart,vEnd,ignore_monsters,ignore_glass,pBot->m_pEdict,&tr);

		vEnd = tr.vecEndPos;

		fDistance = pBot->DistanceFrom(vEnd) * 2; //(velocity gets double distance!)

		if ( iStep == 30 )
			fHighDistance = fDistance;

		if ( fDistance > fMaxDistance )
		{
			vPositions.Clear();

			fMaxDistance = fDistance;

			vPositions.Add(vEnd);
		}
		else if ( !vPositions.IsEmpty() && (fDistance == fMaxDistance) )
		{
			vPositions.Add(vEnd);
		}
	}

	Vector vLow = pBot->pev->origin;
	vLow.z -= (pBot->pev->size.z/3);
	float fLowDistance;

	UTIL_TraceLine(vLow,vLow + pBot->pev->velocity*2,ignore_monsters,ignore_glass,pBot->m_pEdict,&tr);

	vEnd = tr.vecEndPos;

	fLowDistance = pBot->DistanceFrom(vEnd);

	if ( (fLowDistance < 64) && (fLowDistance < fHighDistance) )
		pBot->Jump();

///////////////////////////////////
	if ( vPositions.IsEmpty() )
	{
		// turn around

		pBot->m_CurrentLookTask = BOT_LOOK_TASK_NEXT_WAYPOINT;

		UTIL_MakeVectors(Vector(0,pev->angles.y,0));

		UTIL_TraceLine(vStart,vStart + (-gpGlobals->v_forward * REACHABLE_RANGE),ignore_monsters,ignore_glass,pev->pContainingEntity,&tr);

		return tr.vecEndPos;
	}

	Vector vPosition = vPositions.Random();

	vPositions.Clear();

	return vPosition;
}


BOOL CheckLift ( CBot *pBot, Vector vCheckOrigin, Vector vCheckToOrigin )
{
	TraceResult tr;
	
	edict_t *pHit = NULL;
	
	//if ( gBotGlobals.IsDebugLevelOn(BOT_DEBUG_NAV_LEVEL) )
	//	WaypointDrawBeam(gBotGlobals.m_Clients.GetClientByIndex(0)->GetPlayer(),vBotOrigin,vCheckOrigin,16,8,200,200,200,200,10);
	
	Vector vComp = (vCheckToOrigin-vCheckOrigin).Normalize();

	(*g_engfuncs.pfnTraceHull)(vCheckOrigin,(vCheckOrigin + (vComp * 4096.0)),ignore_monsters,point_hull,pBot->m_pEdict,&tr);

	if ( tr.flFraction < 1.0 ) // hit something
		pHit = tr.pHit;
	
	if ( pHit )
	{                   
		if ( pHit && (pHit->v.targetname) )
		{                       
			if ( ( strcmp("func_seethroughdoor",STRING(pHit->v.classname)) == 0 ) ||
				( strncmp("func_door",STRING(pHit->v.classname),9) == 0 ) ||
				( strncmp("func_plat",STRING(pHit->v.classname),9) == 0 ) ||
				( strncmp("func_train",STRING(pHit->v.classname),9) == 0 ))
			{
				char *szTargetname = (char*)STRING(pHit->v.targetname);
				
				// a way to find out if this is a lift (big enough for the bot to walk on)
				BOOL bIsLift =  ( ( pHit->v.movedir.z ) && 
					( (pHit->v.size.x > pBot->pev->size.z) &&
					(pHit->v.size.y > pBot->pev->size.z) ) );
				
				if ( BotFunc_EntityIsMoving(&pHit->v) )
				{
					
				}
				else if ( *szTargetname ) // lift needs a button
				{
					// list of tasks to use it...
					
					edict_t *pButton;
					int iWaitForLiftWpt = -1;
					
					Vector vButtonOrigin;
					
					pButton = BotFunc_FindNearestButton(pBot->pev->origin+pBot->pev->view_ofs,&pHit->v,&vButtonOrigin);
					
					int iNewScheduleId = pBot->m_Tasks.GetNewScheduleId();
					
					if ( pButton )
					{
						BOOL bNormalDoor = TRUE;
						
						pBot->m_Tasks.FlushTasks();
						
						pBot->m_stFailedPaths.ClearFailedPaths();
						
						/*if ( vBotOrigin.z > vCheckOrigin.z )
						{
							bIsLift = FALSE;// just use Normal door technique.
							// going down
						}*/
						
						// if its rotating, you must walk through it and not wait...
						// (if angle velocity is more than 0)
						if ( bIsLift )
						{
							
							iWaitForLiftWpt = WaypointFindNearestGoal(pBot->GetGunPosition(),pBot->m_pEdict,REACHABLE_RANGE,pBot->m_iTeam,W_FL_LIFT,&pBot->m_FailedGoals);
							
							if ( iWaitForLiftWpt == -1 )
								iWaitForLiftWpt = pBot->m_iPrevWaypointIndex;
							
							if ( iWaitForLiftWpt != -1 )
							{
								edict_t *pOtherButton = BotFunc_FindNearestButton(WaypointOrigin(iWaitForLiftWpt),&pHit->v);
								
								if ( pOtherButton )
								{
									pBot->AddTask(CBotTask(BOT_TASK_FIND_PATH,iNewScheduleId,pOtherButton,0,-1.0));
									pBot->AddTask(CBotTask(BOT_TASK_USE,iNewScheduleId,pOtherButton,0,1.0));
								}
								else
								{
									pBot->AddTask(CBotTask(BOT_TASK_FIND_PATH,iNewScheduleId,NULL,0,0.0,vButtonOrigin));
									pBot->AddTask(CBotTask(BOT_TASK_USE,iNewScheduleId,pButton,0,1.0));
								}
								
								
								pBot->AddTask(CBotTask(BOT_TASK_FIND_PATH,iNewScheduleId,NULL,iWaitForLiftWpt,-1.0));
								pBot->AddTask(CBotTask(BOT_TASK_WAIT_FOR_ENTITY,iNewScheduleId,pHit));

								pBot->m_Tasks.SetTimeToCompleteSchedule(iNewScheduleId,RANDOM_FLOAT(10,20));
								
								// found a lift waypoint.. mark as not a normal door
								bNormalDoor = FALSE;
								//if ( pHit->v.movedir.z )
								//{                                     
								//  pBot->AddTask(CBotTask(BOT_TASK_USE_DOOR_BUTTON,pHit));
								//}
							}                               
						}
						
						if ( bNormalDoor )
						{
							// if its a typical door, press the button only
							if ( (gBotGlobals.m_iCurrentMod == MOD_DMC) && (pButton->v.health > 0) )                                    
							{
								// if DMC and the button can  be shot, just shoot it, dont walk over to it.
								pBot->AddTask(CBotTask(BOT_TASK_USE,iNewScheduleId,pButton,-1,1.0));
							}
							else
							{
								//int iWpt = WaypointLocations.NearestWaypoint(vButtonOrigin,100,-1,TRUE,FALSE,FALSE,&pBot->m_FailedGoals);
								
								pBot->AddTask(CBotTask(BOT_TASK_FIND_PATH,iNewScheduleId,NULL,0,0.0,vButtonOrigin));
								pBot->AddTask(CBotTask(BOT_TASK_USE,iNewScheduleId,pButton,0,1.0));
							}
							
							//PATH *pIgnore = BotNavigate_FindPathFromTo(pBot->m_iPrevWaypointIndex,pBot->m_iCurrentWaypointIndex,pBot->m_iTeam);
							
							//if ( pIgnore )
							//  pBot->m_stFailedPaths.AddFailedPath(pIgnore);
							
						}
						
						pBot->AddTask(CBotTask(BOT_TASK_FIND_PATH,0,NULL,0,0,pBot->m_vGoalOrigin.GetVector()));
					}
					else
					{
						BOOL bFail = TRUE;
						
						// Button not found, see if there is a lift button waypoint nearby
						
						if ( !pBot->m_Tasks.HasSchedule(BOT_SCHED_USE_LIFT) )
							// Not already using a lift?
						{
							float fRange = pHit->v.size.Length2D();
							
							// get the lift button waypoint
							int iWpt = WaypointFindNearestGoal(pBot->GetGunPosition(),pBot->m_pEdict,fRange,-1,W_FL_LIFT,&pBot->m_FailedGoals);
							
							if ( iWpt != -1 )
							{
								// look for possible buttons
								char *szClassnames[3] = {"func_button","button_target","func_rot_button"};
								
								// check nearby the lift button waypoint
								edict_t *pButton = UTIL_FindNearestEntity(szClassnames,3,WaypointOrigin(iWpt),fRange,TRUE);
								
								if ( pButton )
								{
									// if a button if found use this one 
									
									int iScheduleId = pBot->m_Tasks.GetNewScheduleId();
									
									// Bot was below the lift so wait for the lift to descend
									pBot->AddPriorityTask(CBotTask(BOT_TASK_WAIT_FOR_ENTITY,iScheduleId,pHit));                                             
									pBot->AddPriorityTask(CBotTask(BOT_TASK_FIND_PATH,iScheduleId,NULL,iWpt,-1));
									pBot->AddPriorityTask(CBotTask(BOT_TASK_USE,iScheduleId,pButton));
									pBot->AddPriorityTask(CBotTask(BOT_TASK_FIND_PATH,iScheduleId,pButton));
									
									// make sure we update these tasks so we know we are using a lift
									pBot->m_Tasks.GiveSchedIdDescription(iScheduleId,BOT_SCHED_USE_LIFT);
									
									bFail = FALSE;
								}
							}
						}
						///// mucks up
					/*	
						if ( bFail )
						{
							
							PATH *pFailed = BotNavigate_FindPathFromTo(pBot->m_iPrevWaypointIndex,pBot->m_iCurrentWaypointIndex,pBot->m_iTeam);
							
							if ( pFailed )
								pBot->m_stFailedPaths.AddFailedPath(pFailed);
							
							CBotTask *pCurrentTask = pBot->m_Tasks.CurrentTask();
							
							if ( pCurrentTask )
							{
								pCurrentTask->SetPathInfo(FALSE);
							}
						}*/
					}
                }
			} 
		}
	}

	return pBot->m_Tasks.HasSchedule(BOT_SCHED_USE_LIFT);
}
