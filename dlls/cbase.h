/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

Class Hierachy

CBaseEntity
	CBaseDelay
		CBaseToggle
			CBaseItem
			CBaseMonster
				CBaseCycler
				CBasePlayer
				CBaseGroup
*/
class CBaseMonster;
class CSquadMonster;
#define		MAX_PATH_SIZE	30 // max number of nodes available for a path.

// These are caps bits to indicate what an object's capabilities (currently used for save/restore and level transitions)
#define		FCAP_CUSTOMSAVE				0x00000001
#define		FCAP_ACROSS_TRANSITION		0x00000002		// should transfer between transitions
#define		FCAP_MUST_SPAWN				0x00000004		// Spawn after restore
#define		FCAP_DONT_SAVE				0x80000000		// Don't save this
#define		FCAP_IMPULSE_USE			0x00000008		// can be used by the player
#define		FCAP_CONTINUOUS_USE			0x00000010		// can be used by the player
#define		FCAP_ONOFF_USE				0x00000020		// can be used by the player
#define		FCAP_DIRECTIONAL_USE		0x00000040		// Player sends +/- 1 when using (currently only tracktrains)
#define		FCAP_MASTER					0x00000080		// Can be used to "master" other entities (like multisource)

// UNDONE: This will ignore transition volumes (trigger_transition), but not the PVS!!!
#define		FCAP_FORCE_TRANSITION		0x00000080		// ALWAYS goes across transitions

#include "studio.h" //NivekDev - For studiohdr_t structure.

#include "saverestore.h"
#include "schedule.h"

#ifndef MONSTEREVENT_H
#include "monsterevent.h"
#endif

#include "enginecallback.h"

/////////SvenCoop Frag Points
/*
#define AWARD_BIGKILL		7 //GARG, NIH
#define AWARD_MEDIUMKILL	5 // 5 POINT KILL
#define AWARD_FOURKILL		4 // 4 POINT KILL
#define AWARD_THREEKILL		3 // 3 POINT KILL
#define AWARD_TWOKILL		2 // 2 POINT KILL 
#define AWARD_SMALLKILL		1  //headcrabs, etc
#define AWARD_NOKILL		0  //rats, snarks
#define AWARD_KILLALLY		-5 //killed friend (barney, scientist etc)
*/

// C functions for external declarations that call the appropriate C++ methods

/* LINUX COMPILE */
#ifdef _WIN32
#define EXPORT _declspec( dllexport )
#else
#define EXPORT
#endif
/* END LINUX COMPILE */

extern "C" EXPORT int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion );
extern "C" EXPORT int GetEntityAPI2( DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion );

extern int DispatchSpawn( edict_t *pent );
extern void DispatchKeyValue( edict_t *pentKeyvalue, KeyValueData *pkvd );
extern void DispatchTouch( edict_t *pentTouched, edict_t *pentOther );
extern void DispatchUse( edict_t *pentUsed, edict_t *pentOther );
extern void DispatchThink( edict_t *pent );
extern void DispatchBlocked( edict_t *pentBlocked, edict_t *pentOther );
extern void DispatchSave( edict_t *pent, SAVERESTOREDATA *pSaveData );
extern int  DispatchRestore( edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity );
extern void	DispatchObjectCollsionBox( edict_t *pent );
extern void SaveWriteFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount );
extern void SaveReadFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount );
extern void SaveGlobalState( SAVERESTOREDATA *pSaveData );
extern void RestoreGlobalState( SAVERESTOREDATA *pSaveData );
extern void ResetGlobalState( void );

typedef enum { USE_OFF = 0, USE_ON = 1, USE_SET = 2, USE_TOGGLE = 3, USE_KILL = 4 } USE_TYPE;
// AdamR: Added USE_KILL for the trigger type feature on multi_manager

// List of bodies for combined w_ model -SvenV
typedef enum {
	SCITEM_W_WEAPONBOX = 0,
	SCITEM_W_2UZIS,
	SCITEM_W_357,
	SCITEM_W_357AMMOBOX,
	SCITEM_W_9MMAR,
	SCITEM_W_9MMARCLIP,
	SCITEM_W_9MMCLIP,
	SCITEM_W_9MMHANDGUN,
	SCITEM_W_ANTIDOTE,
	SCITEM_W_ARGRENADE,
	SCITEM_W_BATTERY,
	SCITEM_W_BGRAP,
	SCITEM_W_CHAINAMMO,
	SCITEM_W_CROSSBOW,
	SCITEM_W_CROSSBOW_CLIP,
	SCITEM_W_CROWBAR,
	SCITEM_W_DESERT_EAGLE,
	SCITEM_W_EGON,
	SCITEM_W_GAUSS,
	SCITEM_W_GAUSSAMMO,
	SCITEM_W_HGUN,
	SCITEM_W_LONGJUMP,
	SCITEM_W_M16,
	SCITEM_W_M40A1,
	SCITEM_W_M40A1CLIP,
	SCITEM_W_MEDKIT,
	SCITEM_W_MINIGUN,
	SCITEM_W_PIPE_WRENCH,
	SCITEM_W_PMEDKIT,
	SCITEM_W_RPGAMMO,
	SCITEM_W_RPG,
	SCITEM_W_SAW,
	SCITEM_W_SAW_CLIP,
	SCITEM_W_SECURITY,
	SCITEM_W_SHOTBOX,
	SCITEM_W_SHOTGUN,
	SCITEM_W_SUIT,
	SCITEM_W_UZI
} SCITEM_W;
//-SvenV End

extern void FireTargets( const char *targetName, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

typedef void (CBaseEntity::*BASEPTR)(void);
typedef void (CBaseEntity::*ENTITYFUNCPTR)(CBaseEntity *pOther );
typedef void (CBaseEntity::*USEPTR)( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );



//#define PRECACHE_SOUND(sound) ( PrecacheSound_Intercept( this, sound ) )
//#define PRECACHE_MODEL(model) ( PrecacheModel_Intercept( model ) )//Intercept GMR-SvenV
//#define SET_MODEL(entity, model) ( SetModel_Intercept( entity, model ) )//Intercept GMR-SvenV

// For CLASSIFY
#define	CLASS_NONE				0
#define CLASS_MACHINE			1
#define CLASS_PLAYER			2
#define	CLASS_HUMAN_PASSIVE		3
#define CLASS_HUMAN_MILITARY	4
#define CLASS_ALIEN_MILITARY	5
#define CLASS_ALIEN_PASSIVE		6
#define CLASS_ALIEN_MONSTER		7
#define CLASS_ALIEN_PREY		8
#define CLASS_ALIEN_PREDATOR	9
#define CLASS_INSECT			10
#define CLASS_PLAYER_ALLY		11
#define CLASS_PLAYER_BIOWEAPON	12 // hornets and snarks.launched by players
#define CLASS_ALIEN_BIOWEAPON	13 // hornets and snarks.launched by the alien menace
#define CLASS_XRACE_PITDRONE	14 // New Pitdrone monster
#define CLASS_XRACE_SHOCK		15 // For Shock Troopers, though Voltigores can use this.

#define	CLASS_BARNACLE			99 // special because no one pays attention to it, and it eats a wide cross-section of creatures.


#define	SF_NORESPAWN	( 1 << 10 )// !!!set this bit on guns and stuff that should never respawn.
#define	SF_DELAYREMOVE	( 1 << 11 )// A weapon will be removed from the map after 60sec -Sniper
#define SF_CREATEDWEAPON ( 1 << 12 ) //Wee! -Sniper
#define SF_DODAMAGE ( 1 << 13 ) //Wee! -Sniper
#define SF_GIVENITEM ( 1 << 14 ) // This item is being given to the player -- can't fall out of map -SvenV


#define MAX_REPLACEMENT_SOUNDS 128


//
// EHANDLE. Safe way to point to CBaseEntities who may die between frames
//
class EHANDLE
{
private:
	edict_t *m_pent;
	int		m_serialnumber;
public:
	edict_t *Get( void );
	edict_t *Set( edict_t *pent );

	operator int ();

	operator CBaseEntity *();

	CBaseEntity * operator = (CBaseEntity *pEntity);
	CBaseEntity * operator ->();
};

// aone - pass this to CBaseEntity::Spawn
/*typedef struct
{
	int iuser1;
	int iuser2;
	int iuser3;
	int iuser4;
	float fuser1;
	float fuser2;
	float fuser3;
	float fuser4;
} entinit_t;*/

// end aone

//Used for GetEntvar method - Protector 2009-09-19
typedef enum entvartype_s
{
    VAR_INVALID = 0,
    VAR_STRING,
    VAR_VECTOR,
    VAR_FLOAT,
    VAR_INTEGER,
    VAR_EDICT,
    VAR_BYTE4,
    VAR_BYTE2
} entvartype_t;
//Used for GetEntvar method - Protector 2009-09-19

//extended trigger filtering - Protector 2010-02-07
#define FILTER_EXCLUDE 0
#define FILTER_INCLUDE 1

//
// Base Entity.  All entity types derive from this
//
class CBaseEntity 
{
public:
	// Constructor.  Set engine to use C/C++ callback functions
	// pointers to engine data
	entvars_t *pev;		// Don't need to save/restore this pointer, the engine resets it

	// path corners
	CBaseEntity*  m_pGoalEnt; // path corner we are heading towards 
	CBaseEntity*  m_pLink;	 // used for temporary link-list operations.  
	CBaseEntity*  m_pLinkAll; // used for temporary link-list operations.  
	CBaseEntity*  m_pMyMedic; // Any entity that is attempting to revive me. -Sniper 

	// initialization functions
	virtual void	Spawn( void ) { return; }
	virtual void	Precache( void );// { return; } //-Sniper
	virtual void	KeyValue( KeyValueData* pkvd)
	{ 	
		if  (FStrEq(pkvd->szKeyName, "soundlist") )
		{
			SoundReplacementPath = ALLOC_STRING( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if  (FStrEq(pkvd->szKeyName, "classify") )
		{
			int type = atoi( pkvd->szValue );
			/*
			#define	CLASS_NONE				0
			#define CLASS_MACHINE			1
			#define CLASS_PLAYER			2
			#define	CLASS_HUMAN_PASSIVE		3
			#define CLASS_HUMAN_MILITARY	4
			#define CLASS_ALIEN_MILITARY	5
			#define CLASS_ALIEN_PASSIVE		6
			#define CLASS_ALIEN_MONSTER		7
			#define CLASS_ALIEN_PREY		8
			#define CLASS_ALIEN_PREDATOR	9
			#define CLASS_INSECT			10
			#define CLASS_PLAYER_ALLY		11
			#define CLASS_PLAYER_BIOWEAPON	12 // hornets and snarks.launched by players
			#define CLASS_ALIEN_BIOWEAPON	13 // hornets and snarks.launched by the alien menace
			#define CLASS_XRACE_PITDRONE	14 // New Pitdrone monster
			#define CLASS_XRACE_SHOCK		15 // For Shock Troopers, though Voltigores can use this.
			#define	CLASS_BARNACLE			99 // special because no one pays attention to it, and it eats a wide cross-section of creatures.
			*/

			switch( type )
			{
			case 2:		//Player ally
			case 3:		//Player ally
			case 11:	//Player ally
			case 12:	//Player ally
			case 1:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 13:
			case 14:
				m_fOverrideClass = true;
				m_iClassSelection = type;
				break;
			case -1: //Class "none"
				m_fOverrideClass = true;
				m_iClassSelection = 0;
				break;
			}
					
			pkvd->fHandled = TRUE;
		}
		//START - extended trigger filtering - Protector 2010-02-07
		else if (FStrEq(pkvd->szKeyName, "toutfiltertype") )
		{
			targetnameOutFilterType = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "coutfiltertype") )
		{
			classnameOutFilterType = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "tinfiltertype") )
		{
			targetnameInFilterType = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "cinfiltertype") )
		{
			classnameInFilterType = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "toutfilter") )
		{
			targetnameOutFilter = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "coutfilter") )
		{
			classnameOutFilter = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "tinfilter") )
		{
			targetnameInFilter = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "cinfilter") )
		{
			classnameInFilter = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		//END - extended trigger filtering - Protector 2010-02-07
		else
		{
			pkvd->fHandled = FALSE;
		}
	};

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	virtual int		ObjectCaps( void ) { return FCAP_ACROSS_TRANSITION; }
	virtual void	Activate( void ) {}
	
	// Setup the object->object collision box (pev->mins / pev->maxs is the object->world collision box)
	virtual void	SetObjectCollisionBox( void );

	int     m_iCameraIndex; //If we are using a camera, store it's index here.

	virtual int GetClassification( int cl );
	bool	m_fOverrideClass;
	int		m_iClassSelection;

	virtual int IRelationship ( CBaseEntity *pTarget );

	// Classify - returns the type of group (i.e, "houndeye", or "human military" so that monsters with different classnames
	// still realize that they are teammates. (overridden for monsters that form groups)
	virtual int Classify ( void ) { return GetClassification(CLASS_NONE); };

	virtual void DeathNotice ( entvars_t *pevChild ) {}// monster maker children use this to tell the monster maker that they have died.


	static	TYPEDESCRIPTION m_SaveData[];

	virtual void	TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int		TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	virtual int		TakeHealth( float flHealth, int bitsDamageType, int health_cap = 0 );
	virtual void	Killed( entvars_t *pevAttacker, int iGib );
	virtual int		BloodColor( void ) { return DONT_BLEED; }
	virtual void	TraceBleed( float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	virtual BOOL    IsTriggered( CBaseEntity *pActivator ) {return TRUE;}
	virtual CBaseMonster *MyMonsterPointer( void ) { return NULL;}
	virtual CSquadMonster *MySquadMonsterPointer( void ) { return NULL;}

	virtual	int		GetToggleState( void ) { return TS_AT_TOP; }
	virtual void	AddPoints( int score, BOOL bAllowNegativeScore ) {}
	virtual void	AddPointsToTeam( int score, BOOL bAllowNegativeScore ) {}
	virtual BOOL	AddPlayerItem( CBasePlayerItem *pItem ) { return 0; }
	virtual BOOL	RemovePlayerItem( CBasePlayerItem *pItem ) { return 0; }
	virtual int 	GiveAmmo( int iAmount, char *szName, int iMax ) { return -1; };
	virtual float	GetDelay( void ) { return 0; }
	virtual int		IsMoving( void ) { return pev->velocity != g_vecZero; }
	virtual void	OverrideReset( void ) {}
	virtual int		DamageDecal( int bitsDamageType );
	// This is ONLY used by the node graph to test movement through a door
	virtual void	SetToggleState( int state ) {}
	virtual void    StartSneaking( void ) {}
	virtual void    StopSneaking( void ) {}
	virtual BOOL	OnControls( entvars_t *pev ) { return FALSE; }
	virtual BOOL    IsSneaking( void ) { return FALSE; }
	virtual BOOL	IsAlive( void ) { return (pev->deadflag == DEAD_NO) && pev->health > 0; }
	virtual BOOL	IsBSPModel( void ) { return pev->solid == SOLID_BSP || pev->movetype == MOVETYPE_PUSHSTEP; }
	virtual BOOL	ReflectGauss( void ) { return ( IsBSPModel() && !pev->takedamage ); }
	virtual BOOL	HasTarget( string_t targetname ) { return FStrEq(STRING(targetname), STRING(pev->targetname) ); }
	virtual BOOL    IsInWorld( void );
	virtual	BOOL	IsMonster( void ) { return FALSE; }
	virtual	BOOL	IsPlayer( void ) { return FALSE; }
	virtual	BOOL	IsPhysX( void ) { return FALSE; }	//Added PhysX - Protector 2010-02-09
	virtual BOOL	IsNetClient( void ) { return FALSE; }
	virtual const char *TeamID( void ) { return ""; }

	// AdamR: Check if the monster is really a machine or not (even when allied)
	virtual BOOL	IsMachine(CBaseEntity *pEntity)
	{
		if (
			FClassnameIs(pEntity->edict(), "monster_turret") ||
			FClassnameIs(pEntity->edict(), "monster_miniturret") ||
			FClassnameIs(pEntity->edict(), "monster_sentry") ||
			FClassnameIs(pEntity->edict(), "monster_robogrunt")
		)
			return TRUE;

		return FALSE;
	};
	// AdamR: End

// aone
	virtual BOOL	CriticalRemove(void) {return FALSE;}	// removes the entity and all its content in critical situations!
// end aone

//	virtual void	SetActivator( CBaseEntity *pActivator ) {}
	virtual CBaseEntity *GetNextTarget( void );
	
	// fundamental callbacks
	void (CBaseEntity ::*m_pfnThink)(void);
	void (CBaseEntity ::*m_pfnTouch)( CBaseEntity *pOther );
	void (CBaseEntity ::*m_pfnUse)( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void (CBaseEntity ::*m_pfnBlocked)( CBaseEntity *pOther );

	virtual void Think( void )
	{
		if (m_pfnThink)
		{
			//try
			{
				(this->*m_pfnThink)();
			}
			//catch (...)
			{
			//	ALERT ( at_console, "Fatal Error at Think in Entity: %s %s\n", STRING(pev->classname), STRING(pev->targetname));
			}
		}
	};
	virtual void Touch( CBaseEntity *pOther )
	{
		if (m_pfnTouch)
		{
			//try
			{
				(this->*m_pfnTouch)( pOther );
			}
			//catch (...)
			{
			//	ALERT ( at_console, "Fatal Error at Touch in Entity: %s %s\n", STRING(pev->classname), STRING(pev->targetname));
			}
		}
	};
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) 
	{ 
		if (m_pfnUse) 
		{
			//try
			{
				(this->*m_pfnUse)( pActivator, pCaller, useType, value );
			}
			//catch (...)
			{
			//	ALERT ( at_console, "Fatal Error at Use in Entity: %s %s\n", STRING(pev->classname), STRING(pev->targetname));
			}
		}
	}

	virtual void Blocked( CBaseEntity *pOther )
	{
		if (m_pfnBlocked)
		{
			//try
			{
				(this->*m_pfnBlocked)( pOther );
			}
			//catch (int e)
			{
			//	ALERT ( at_console, "Fatal Error at Blocked in Entity: %s %s:(error %d)\n", STRING(pev->classname), STRING(pev->targetname), e);
			}
		}
	
	};

	// allow engine to allocate instance data
    void *operator new( size_t stAllocateBlock, entvars_t *pev )
	{
		return (void *)ALLOC_PRIVATE(ENT(pev), stAllocateBlock);
	};

	// don't use this.
#if _MSC_VER >= 1200 // only build this code if MSVC++ 6.0 or higher
	void operator delete(void *pMem, entvars_t *pev)
	{
		pev->flags |= FL_KILLME;
	};
#endif

	virtual void UpdateOnRemove( void ); //Make virtual

	// common member functions
	void EXPORT SUB_Remove( void );
	void EXPORT SUB_DoNothing( void );
	void EXPORT SUB_StartFadeOut ( void );
	void EXPORT SUB_FadeOut ( void );
	void EXPORT SUB_CallUseToggle( void ) { this->Use( this, this, USE_TOGGLE, 0 ); }
	int			ShouldToggle( USE_TYPE useType, BOOL currentState );
	void		FireBullets( ULONG	cShots, Vector  vecSrc, Vector	vecDirShooting,	Vector	vecSpread, float flDistance, int iBulletType, int iTracerFreq = 4, int iDamage = 0, entvars_t *pevAttacker = NULL , BOOL fDraw = 2);

	virtual CBaseEntity *Respawn( void ) { return NULL; }

	void SUB_UseTargets( CBaseEntity *pActivator, USE_TYPE useType, float value );
	// Do the bounding boxes of these two intersect?
	int		Intersects( CBaseEntity *pOther );
	void	MakeDormant( void );
	int		IsDormant( void );
	BOOL    IsLockedByMaster( void ) { return FALSE; }

#ifdef _DEBUG
	static CBaseEntity *Instance( edict_t *pent ) 
	{ 
		if ( !pent )
			pent = ENT(0);
		CBaseEntity *pEnt = (CBaseEntity *)GET_PRIVATE(pent); 
		ASSERT(pEnt!=NULL); 
		return pEnt; 
	}
#else
	static CBaseEntity *Instance( edict_t *pent )
	{ 
		if ( !pent )
			pent = ENT(0);
		CBaseEntity *pEnt = (CBaseEntity *)GET_PRIVATE(pent); 
		return pEnt; 
	}
#endif

	static CBaseEntity *Instance( entvars_t *pev ) { return Instance( ENT( pev ) ); }
	static CBaseEntity *Instance( int eoffset) { return Instance( ENT( eoffset) ); }

	CBaseMonster *GetMonsterPointer( entvars_t *pevMonster ) 
	{ 
		CBaseEntity *pEntity = Instance( pevMonster );
		if ( pEntity )
			return pEntity->MyMonsterPointer();
		return NULL;
	}
	CBaseMonster *GetMonsterPointer( edict_t *pentMonster ) 
	{ 
		CBaseEntity *pEntity = Instance( pentMonster );
		if ( pEntity )
			return pEntity->MyMonsterPointer();
		return NULL;
	}


	// Ugly code to lookup all functions to make sure they are exported when set.
#ifdef _DEBUG
	void FunctionCheck( void *pFunction, char *name ) 
	{ 
#ifdef _WIN32
		if (pFunction && !NAME_FOR_FUNCTION((unsigned long)(pFunction)) )
			ALERT( at_error, "No EXPORT: %s:%s (%08lx)\n", STRING(pev->classname), name, (unsigned long)pFunction );
#endif // _WIN32
	}

	BASEPTR	ThinkSet( BASEPTR func, char *name );
	ENTITYFUNCPTR TouchSet( ENTITYFUNCPTR func, char *name );
	USEPTR	UseSet( USEPTR func, char *name );
	ENTITYFUNCPTR	BlockedSet( ENTITYFUNCPTR func, char *name );
#endif


	// virtual functions used by a few classes
	
	// used by monsters that are created by the MonsterMaker
	virtual	void UpdateOwner( void ) { return; };


	//
	static CBaseEntity *Create( char *szName, const Vector &vecOrigin, const Vector &vecAngles,bool g_fCreateAndDontSpawn, edict_t *pentOwner = NULL );

	virtual BOOL FBecomeProne( void ) {return FALSE;};
	edict_t *edict() { return ENT( pev ); };
	EOFFSET eoffset( ) { return OFFSET( pev ); };
	int	  entindex( ) { return ENTINDEX( edict() ); };

	virtual Vector Center( ) { return (pev->absmax + pev->absmin) * 0.5; }; // center point of entity
	virtual Vector EyePosition( ) 
	{ 
		if( IsBSPModel() )
			return (pev->absmax + pev->absmin) * 0.5;

		return pev->origin + pev->view_ofs;
	};			// position of eyes
	virtual Vector EarPosition( ) { return pev->origin + pev->view_ofs; };	 // position of ears
	virtual Vector BodyTarget( const Vector &posSrc ){ 	return Center( ); }; // position to shoot at

	virtual int Illumination( ) { return GETENTITYILLUM( ENT( pev ) ); };

	virtual	BOOL FVisible ( CBaseEntity *pEntity, bool fIgnoreGlass = true );
	virtual	BOOL FVisible ( const Vector &vecOrigin );
	virtual BOOL FVisibleFromPos ( Vector vecTarget, Vector vecStart );


// aone -> The new "getting points without killing an entity" system
	//float frags;	// frags is also as float value here
	virtual float GetPointsForDamage(float flDamage) {return flDamage*0;};	// Overload for every entity
	//virtual void UpdateFrags(void) {pev->frags = (int)frags;}; // Update the pev->frags with the fragment frags
	virtual void GetDamagePoints(entvars_t *pevAttacker,entvars_t *pevInflictor,float flDamage);	// Get points for making some damage

	bool	m_fPlayerAlly;

	// alloc/dealloc functions
	CBaseEntity();	// donot override this
	virtual ~CBaseEntity();	// donot override this
	
	virtual void OnCreate();	// constructor - override if necessary
	virtual void OnDestroy();	// destructor - override if necessary

	virtual bool IsValidEntity() {return true;}	// return false if the entity is somehow invalid ...
// end aone

	float m_flMaximumFadeWait; //Maximum wait before an entity fades out -Sniper
	float m_flMaximumFadeWaitB;
	bool m_fCanFadeStart;

	//SOUND REPLACEMENT -Sniper
	const char *SOUNDREPLACEMENT_Find(const char *name);
	const char *MODELREPLACEMENT_Find(const char *name);//GMR-SvenV
	char SoundOriginal[MAX_REPLACEMENT_SOUNDS][MAX_REPLACEMENT_SOUNDS]; // Original Sounds
	char SoundReplacement[MAX_REPLACEMENT_SOUNDS][MAX_REPLACEMENT_SOUNDS]; // Sounds listed to replace the original sound array.
	int gcSounds;
	int SoundReplacementPath;
	
	int FindMonstersInWorld( CBaseEntity **pList, int listMax, int flagMask );

	//END SOUND REPLACEMENT

	BOOL SetupModel(void); //MOVED -Sniper
	BOOL m_fCustomModel;//MOVED -Sniper
	Vector m_vecLastOrigin;
	//PRECACHE INTERCEPTION -Sniper
//	virtual void PRECACHE_SOUND ( char *sound );

	//Method to get entvars members using strings - Protector 2009-09-19
	entvartype_s GetEntvar(const char *key, void **value);

	//Extended trigger filtering - Protector 2010-02-07
	int targetnameOutFilterType;	//exclude or include
	int classnameOutFilterType;		//exclude or include
	int targetnameInFilterType;		//exclude or include
	int classnameInFilterType;		//exclude or include
	string_t targetnameOutFilter;	//filter targetnames that can be triggered (or not if exclude)
	string_t classnameOutFilter;	//filter classnames that can be triggered (or not if exclude)
	string_t targetnameInFilter;	//filter targetnames that can trigger this (or not if exclude)
	string_t classnameInFilter;		//filter classnames that can trigger this (or not if exclude)
};


//CBaseEntityList - used to make Entity lists - Protector 2009-09-04
class CBaseEntityList
{
public:
	EHANDLE ent;
	CBaseEntityList *next;
};

// Ugly technique to override base member functions
// Normally it's illegal to cast a pointer to a member function of a derived class to a pointer to a 
// member function of a base class.  static_cast is a sleezy way around that problem.

#ifdef _DEBUG

#define SetThink( a ) ThinkSet( static_cast <void (CBaseEntity::*)(void)> (a), #a )
#define SetTouch( a ) TouchSet( static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a), #a )
#define SetUse( a ) UseSet( static_cast <void (CBaseEntity::*)(	CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )> (a), #a )
#define SetBlocked( a ) BlockedSet( static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a), #a )

#else

#define SetThink( a ) m_pfnThink = static_cast <void (CBaseEntity::*)(void)> (a)
#define SetTouch( a ) m_pfnTouch = static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a)
#define SetUse( a ) m_pfnUse = static_cast <void (CBaseEntity::*)( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )> (a)
#define SetBlocked( a ) m_pfnBlocked = static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a)

#endif


class CPointEntity : public CBaseEntity
{
public:
	void	Spawn( void );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
private:
};


typedef struct locksounds			// sounds that doors and buttons make when locked/unlocked
{
	string_t	sLockedSound;		// sound a door makes when it's locked
	string_t	sLockedSentence;	// sentence group played when door is locked
	string_t	sUnlockedSound;		// sound a door makes when it's unlocked
	string_t	sUnlockedSentence;	// sentence group played when door is unlocked

	int		iLockedSentence;		// which sentence in sentence group to play next
	int		iUnlockedSentence;		// which sentence in sentence group to play next

	float	flwaitSound;			// time delay between playing consecutive 'locked/unlocked' sounds
	float	flwaitSentence;			// time delay between playing consecutive sentences
	BYTE	bEOFLocked;				// true if hit end of list of locked sentences
	BYTE	bEOFUnlocked;			// true if hit end of list of unlocked sentences
} locksound_t;

void PlayLockSounds(entvars_t *pev, locksound_t *pls, int flocked, int fbutton);

//
// MultiSouce
//

#define MAX_MULTI_TARGETS	32 // maximum number of targets a single multi_manager entity may be assigned.
#define MS_MAX_TARGETS 32

class CMultiSource : public CPointEntity
{
public:
	void Spawn( );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int	ObjectCaps( void ) { return (CPointEntity::ObjectCaps() | FCAP_MASTER); }
	BOOL IsTriggered( CBaseEntity *pActivator );
	void EXPORT Register( void );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	EHANDLE		m_rgEntities[MS_MAX_TARGETS];
	int			m_rgTriggered[MS_MAX_TARGETS];

	int			m_iTotal;
	string_t	m_globalstate;
};


//
// generic Delay entity.
//
class CBaseDelay : public CBaseEntity
{
public:
	float		m_flDelay;
	int			m_iszKillTarget;

	virtual void	KeyValue( KeyValueData* pkvd);
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];
	// common member functions
	void SUB_UseTargets( CBaseEntity *pActivator, USE_TYPE useType, float value );
	void EXPORT DelayThink( void );
};

//Moved CTriggerCamera to cbase.h - Protector
#define SF_CAMERA_PLAYER_POSITION		1
#define SF_CAMERA_PLAYER_TARGET			2
#define SF_CAMERA_PLAYER_TAKECONTROL	4

#define SF_CAMERA_PLAYER_ALLPLAYERS		8
#define SF_CAMERA_PLAYER_FORCEVIEW		16
#define SF_CAMERA_PLAYER_INSTANTTURN	32	// Use to enable a piece of Sniper's camera code below -SvenV
#define SF_CAMERA_PLAYER_INSTANTMOVE	64	// No acceleration/decelaration crap. - Protector 2010-01-08
#define SF_CAMERA_MOUSE_CURSOR			128	// Mouse cursor. - Protector 2010-01-08

#define MOUSE_RELEASED					0
#define MOUSE_PRESSED					1
#define MOUSE_DOUBLE_PRESSED			2
#define MOUSE_DRAGGED					3

#define MOUSE_BUTTON_LEFT				0
#define MOUSE_BUTTON_RIGHT				1
#define MOUSE_BUTTON_MIDDLE				2

#define MOUSE_ACTION_NOACTION			0	//No action
#define MOUSE_ACTION_OFF				1	//Normal USE off
#define MOUSE_ACTION_ON					2	//Normal USE on
#define MOUSE_ACTION_TOGGLE				3	//Normal USE toggle
#define MOUSE_ACTION_KILL				4	//Kill remove clicked entity
#define MOUSE_ACTION_HURT				5	//Hurt clicked entity (parameter is amount)
#define MOUSE_ACTION_HEAL				6	//Heal clicked entity (parameter is amount)
#define MOUSE_ACTION_SPAWN				7	//Spawn a new entity (parameter is class)
#define MOUSE_ACTION_TELEPORT			8	//Teleport an entity here (parameter is targetname)
#define MOUSE_ACTION_TELEPORT_AWAY		9	//Teleports clicked entity away (parameter is teleport target targetname)
#define MOUSE_ACTION_EXPLOSION			10	//Create an explosion here (parameter is amount)
#define MOUSE_ACTION_DRAG				11	//Drag clicked entity

#define CAMERA_MAX_SINGLE_PLAYERS		32	//multiple players can share a single camera	(which is intended for one)

class CTriggerCamera : public CBaseDelay
{
private:
	void Move(void);

	void FindAllPlayers( void );
	void CameraPlayer( CBaseEntity *pEntity );
	void UnCameraPlayer( CBaseEntity *pEntity );
	void EnableMouse( CBaseEntity *pEntity );
	void DisableMouse( CBaseEntity *pEntity );

	int mouse_actions[3][2];
	string_t mouse_param[3][2];
	bool is_pressed[3];

	EHANDLE m_hLastDragEntity;
	Vector m_vecStartDragEntityPosition;
	Vector m_vecStartDragPosition;
	float m_flStartDragDistance;

public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT FollowTarget( void );

	void OnCreate( void );
	void MouseClicked( CBasePlayer *pPlayer, int pressed, int button, Vector pos);

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static	TYPEDESCRIPTION m_SaveData[];

	bool model_set;	//Protector

	EHANDLE m_hPlayer[CAMERA_MAX_SINGLE_PLAYERS];	//from now on, it is possible for multiple players to use a single camera independently!	- Protector 2010-07-20
	EHANDLE m_hTarget;
	EHANDLE m_pentPath; 
	int	  m_sPath;
	float m_flWait;
	float m_flReturnTime;	//maybe for a later release we can add return times for each single player, making a single camera even more independently usable	- Protector 2010-07-20
	float m_flStopTime;
	float m_moveDistance;
	float m_targetSpeed;
	float m_initialSpeed;
	float m_acceleration;
	float m_deceleration;
	float m_turnspeed; //SvenV
	int	  m_state;
};

class CBaseAnimating : public CBaseDelay
{
public:
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	// Basic Monster Animation functions
	float StudioFrameAdvance( float flInterval = 0.0 ); // accumulate animation frame time from last time called until now
	int	 GetSequenceFlags( void );
	int  LookupActivity ( int activity );
	int  LookupActivityHeaviest ( int activity );
	int  LookupSequence ( const char *label );
	
	void ResetSequenceInfo ( );
	void ResetGaitSequenceInfo ( ); //-Sniper

	void DispatchAnimEvents ( float flFutureInterval = 0.1 ); // Handle events that have happend since last time called up until X seconds into the future
	virtual void HandleAnimEvent( MonsterEvent_t *pEvent ) { return; };
	float SetBoneController ( int iController, float flValue );
	void InitBoneControllers ( void );
	float SetBlending ( int iBlender, float flValue );
	void GetBonePosition ( int iBone, Vector &origin, Vector &angles );
	void GetAutomovement( Vector &origin, Vector &angles, float flInterval = 0.1 );
	int  FindTransition( int iEndingSequence, int iGoalSequence, int *piDir );
	void GetAttachment ( int iAttachment, Vector &origin, Vector &angles );
	void SetBodygroup( int iGroup, int iValue );
	int GetBodygroup( int iGroup );
	int ExtractBbox( int sequence, float *mins, float *maxs );
	void SetSequenceBox( void );

	int GetSequenceActivityName( int sequence );

	// animation needs
	float				m_flFrameRate;		// computed FPS for current sequence
	float				m_flGroundSpeed;	// computed linear movement rate for current sequence
	float				m_flLastEventCheck;	// last time the event list was checked

	/* AdamR: Commenting out as it's too buggy for now
// g1l - for some fun stuff
	float				m_flFrameRateAdj;
	float				m_flGroundSpeedAdj;
	*/

	float				m_flLastGaitEventCheck;	// last time the event list was checked

	BOOL				m_fSequenceFinished;// flag set when StudioAdvanceFrame moves across a frame boundry
	BOOL				m_fSequenceLoops;	// true if the sequence loops
};


//
// generic Toggle entity.
//
#define	SF_ITEM_USE_ONLY	256 //  ITEM_USE_ONLY = BUTTON_USE_ONLY = DOOR_USE_ONLY!!! 

class CBaseToggle : public CBaseAnimating
{
public:
	void				KeyValue( KeyValueData *pkvd );

	TOGGLE_STATE		m_toggle_state;
	float				m_flActivateFinished;//like attack_finished, but for doors
	float				m_flMoveDistance;// how far a door should slide or rotate
	float				m_flWait;
	float				m_flLip;
	float				m_flTWidth;// for plats
	float				m_flTLength;// for plats

	Vector				m_vecPosition1;
	Vector				m_vecPosition2;
	Vector				m_vecAngle1;
	Vector				m_vecAngle2;

	int					m_cTriggersLeft;		// trigger_counter only, # of activations remaining
	float				m_flHeight;
	EHANDLE				m_hActivator;
	void (CBaseToggle::*m_pfnCallWhenMoveDone)(void);
	Vector				m_vecFinalDest;
	Vector				m_vecFinalAngle;

	int					m_bitsDamageInflict;	// DMG_ damage type that the door or tigger does

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	virtual int		GetToggleState( void ) { return m_toggle_state; }
	virtual float	GetDelay( void ) { return m_flWait; }

	// common member functions
	void LinearMove( Vector	vecDest, float flSpeed );
	void EXPORT LinearMoveDone( void );
	void AngularMove( Vector vecDestAngle, float flSpeed );
	void EXPORT AngularMoveDone( void );
	BOOL IsLockedByMaster( void );

	static float		AxisValue( int flags, const Vector &angles );
	static void			AxisDir( entvars_t *pev );
	static float		AxisDelta( int flags, const Vector &angle1, const Vector &angle2 );

	string_t m_sMaster;		// If this button has a master switch, this is the targetname.
							// A master switch must be of the multisource type. If all 
							// of the switches in the multisource have been triggered, then
							// the button will be allowed to operate. Otherwise, it will be
							// deactivated.
};
#define SetMoveDone( a ) m_pfnCallWhenMoveDone = static_cast <void (CBaseToggle::*)(void)> (a)


// people gib if their health is <= this at the time of death
#define	GIB_HEALTH_VALUE	-50 //-30

#define	ROUTE_SIZE			8 // how many waypoints a monster can store at one time
#define MAX_OLD_ENEMIES		4 // how many old enemies to remember

#define	bits_CAP_DUCK			( 1 << 0 )// crouch
#define	bits_CAP_JUMP			( 1 << 1 )// jump/leap
#define bits_CAP_STRAFE			( 1 << 2 )// strafe ( walk/run sideways)
#define bits_CAP_SQUAD			( 1 << 3 )// can form squads
#define	bits_CAP_SWIM			( 1 << 4 )// proficiently navigate in water
#define bits_CAP_CLIMB			( 1 << 5 )// climb ladders/ropes
#define bits_CAP_USE			( 1 << 6 )// open doors/push buttons/pull levers
#define bits_CAP_HEAR			( 1 << 7 )// can hear forced sounds
#define bits_CAP_AUTO_DOORS		( 1 << 8 )// can trigger auto doors
#define bits_CAP_OPEN_DOORS		( 1 << 9 )// can open manual doors
#define bits_CAP_TURN_HEAD		( 1 << 10)// can turn head, always bone controller 0

#define bits_CAP_RANGE_ATTACK1	( 1 << 11)// can do a range attack 1
#define bits_CAP_RANGE_ATTACK2	( 1 << 12)// can do a range attack 2
#define bits_CAP_MELEE_ATTACK1	( 1 << 13)// can do a melee attack 1
#define bits_CAP_MELEE_ATTACK2	( 1 << 14)// can do a melee attack 2

#define bits_CAP_FLY			( 1 << 15)// can fly, move all around

#define bits_CAP_USE_TANK		( 1 << 16)// can use func_tanks -Sniper
#define bits_CAP_FALL_DAMAGE	( 1 << 17)// can take fall damage -Sniper


#define bits_CAP_DOORS_GROUP    (bits_CAP_USE | bits_CAP_AUTO_DOORS | bits_CAP_OPEN_DOORS)


//Moveshoot caps -Sniper
#define bits_MOVESHOOT_MELEE_ATTACK1 ( 1 << 0 ) //Can run and attempt melee attack 1
#define bits_MOVESHOOT_MELEE_ATTACK2 ( 1 << 1 ) //Can run and attempt melee attack 2
#define bits_MOVESHOOT_RANGE_ATTACK1 ( 1 << 2 ) //Can run and attempt range attack 1
#define bits_MOVESHOOT_RANGE_ATTACK2 ( 1 << 3 ) //Can run and attempt range attack 2
#define bits_MOVESHOOT_RELOAD		 ( 1 << 4 ) //Can run and attempt to reload
#define bits_MOVESHOOT_CAN_ATTACK	 ( bits_MOVESHOOT_MELEE_ATTACK1 | bits_MOVESHOOT_MELEE_ATTACK2 | bits_MOVESHOOT_RANGE_ATTACK1 | bits_MOVESHOOT_RANGE_ATTACK2 )


// used by suit voice to indicate damage sustained and repaired type to player

// instant damage

#define DMG_GENERIC			0			// generic damage was done
#define DMG_CRUSH			(1 << 0)	// crushed by falling or moving object
#define DMG_BULLET			(1 << 1)	// shot
#define DMG_SLASH			(1 << 2)	// cut, clawed, stabbed
#define DMG_BURN			(1 << 3)	// heat burned
#define DMG_FREEZE			(1 << 4)	// frozen
#define DMG_FALL			(1 << 5)	// fell too far
#define DMG_BLAST			(1 << 6)	// explosive blast damage
#define DMG_CLUB			(1 << 7)	// crowbar, punch, headbutt
#define DMG_SHOCK			(1 << 8)	// electric shock
#define DMG_SONIC			(1 << 9)	// sound pulse shockwave
#define DMG_ENERGYBEAM		(1 << 10)	// laser or other high energy beam 
#define DMG_NEVERGIB		(1 << 12)	// with this bit OR'd in, no damage type will be able to gib victims upon death
#define DMG_ALWAYSGIB		(1 << 13)	// with this bit OR'd in, any damage type can be made to gib victims upon death.
#define DMG_LAUNCH			(1 << 26)	// Launch into air.
#define DMG_DROWN			(1 << 14)	// Drowning
// time-based damage
#define DMG_TIMEBASED		(~(0x3fff))	// mask for time-based damage

#define DMG_PARALYZE		(1 << 15)	// slows affected creature down
#define DMG_NERVEGAS		(1 << 16)	// nerve toxins, very bad
#define DMG_POISON			(1 << 17)	// blood poisioning
#define DMG_RADIATION		(1 << 18)	// radiation exposure
#define DMG_DROWNRECOVER	(1 << 19)	// drowning recovery
#define DMG_ACID			(1 << 20)	// toxic chemicals or acid burns
#define DMG_SLOWBURN		(1 << 21)	// in an oven
#define DMG_SLOWFREEZE		(1 << 22)	// in a subzero freezer
#define DMG_MORTAR			(1 << 23)	// Hit by air raid (done to distinguish grenade from mortar)
#define DMG_SNIPER			(1 << 24)	// Hit by A SNIPER //Sniper - New monsters
#define DMG_MEDKITHEAL		(1 << 25)   // Medkit Healing -Sniper

// these are the damage types that are allowed to gib corpses
#define DMG_GIB_CORPSE		( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB | DMG_SHOCK)

// these are the damage types that have client hud art
#define DMG_SHOWNHUD		(DMG_POISON | DMG_ACID | DMG_FREEZE | DMG_SLOWFREEZE | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK)

// NOTE: tweak these values based on gameplay feedback:

#define PARALYZE_DURATION	2		// number of 2 second intervals to take damage
#define PARALYZE_DAMAGE		1.0		// damage to take each 2 second interval

#define NERVEGAS_DURATION	2
#define NERVEGAS_DAMAGE		5.0

#define POISON_DURATION		5
#define POISON_DAMAGE		2.0

#define RADIATION_DURATION	2
#define RADIATION_DAMAGE	1.0

#define ACID_DURATION		2
#define ACID_DAMAGE			5.0

#define SLOWBURN_DURATION	2
#define SLOWBURN_DAMAGE		1.0

#define SLOWFREEZE_DURATION	2
#define SLOWFREEZE_DAMAGE	1.0


#define	itbd_Paralyze		0		
#define	itbd_NerveGas		1
#define	itbd_Poison			2
#define	itbd_Radiation		3
#define	itbd_DrownRecover	4
#define	itbd_Acid			5
#define	itbd_SlowBurn		6
#define	itbd_SlowFreeze		7
#define CDMG_TIMEBASED		8

// when calling KILLED(), a value that governs gib behavior is expected to be 
// one of these three values
#define GIB_NORMAL			0// gib if entity was overkilled
#define GIB_NEVER			1// never gib, no matter how much death damage is done ( freezing, etc )
#define GIB_ALWAYS			2// always gib ( Houndeye Shock, Barnacle Bite )
#define GIB_NOPENALTY		3

class CBaseMonster;
class CCineMonster;
class CSound;

//#include "ai_pathing.h" //Path movement stuff -Sniper
//#include "basemonster.h"

#define		SHOCK_MAX_BEAMS	8
char *ButtonSound( int sound );				// get string of button sound number


//
// Generic Button
//
class CBaseButton : public CBaseToggle
{
public:
	void Spawn( void );
	virtual void Precache( void );
	void RotSpawn( void );
	virtual void KeyValue( KeyValueData* pkvd);

	void ButtonActivate( );
	void SparkSoundCache( void );

	void EXPORT ButtonShot( void );
	void EXPORT ButtonTouch( CBaseEntity *pOther );
	void EXPORT ButtonSpark ( void );
	void EXPORT TriggerAndWait( void );
	void EXPORT ButtonReturn( void );
	void EXPORT ButtonBackHome( void );
	void EXPORT ButtonUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int		TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	enum BUTTON_CODE { BUTTON_NOTHING, BUTTON_ACTIVATE, BUTTON_RETURN };
	BUTTON_CODE	ButtonResponseToTouch( void );
	
	static	TYPEDESCRIPTION m_SaveData[];
	// Buttons that don't take damage can be IMPULSE used
	virtual int	ObjectCaps( void ) { return (CBaseToggle:: ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | (pev->takedamage?0:FCAP_IMPULSE_USE); }

	BOOL	m_fStayPushed;	// button stays pushed in until touched again?
	BOOL	m_fRotating;		// a rotating button?  default is a sliding button.

	string_t m_strChangeTarget;	// if this field is not null, this is an index into the engine string array.
							// when this button is touched, it's target entity's TARGET field will be set
							// to the button's ChangeTarget. This allows you to make a func_train switch paths, etc.

	locksound_t m_ls;			// door lock sounds
	
	BYTE	m_bLockedSound;		// ordinals from entity selection
	BYTE	m_bLockedSentence;	
	BYTE	m_bUnlockedSound;	
	BYTE	m_bUnlockedSentence;
	int		m_sounds;
};

//
// Weapons 
//

#define	BAD_WEAPON 0x00007FFF

//
// Converts a entvars_t * to a class pointer
// It will allocate the class and entity if necessary
//
template <class T> T * GetClassPtr( T *a )
{
	entvars_t *pev = (entvars_t *)a;

	// allocate entity if necessary
	if (pev == NULL)
		pev = VARS(CREATE_ENTITY());

	// get the private data
	a = (T *)GET_PRIVATE(ENT(pev));

	if (a == NULL) 
	{
		// allocate private data 
		a = new(pev) T;
		a->OnCreate();
		a->pev = pev;
	}
	return a;
}


/*
bit_PUSHBRUSH_DATA | bit_TOGGLE_DATA
bit_MONSTER_DATA
bit_DELAY_DATA
bit_TOGGLE_DATA | bit_DELAY_DATA | bit_MONSTER_DATA
bit_PLAYER_DATA | bit_MONSTER_DATA
bit_MONSTER_DATA | CYCLER_DATA
bit_LIGHT_DATA
path_corner_data
bit_MONSTER_DATA | wildcard_data
bit_MONSTER_DATA | bit_GROUP_DATA
boid_flock_data
boid_data
CYCLER_DATA
bit_ITEM_DATA
bit_ITEM_DATA | func_hud_data
bit_TOGGLE_DATA | bit_ITEM_DATA
EOFFSET
env_sound_data
env_sound_data
push_trigger_data
*/

#define TRACER_FREQ		4			// Tracers fire every 4 bullets

typedef struct _SelAmmo
{
	BYTE	Ammo1Type;
	BYTE	Ammo1;
	BYTE	Ammo2Type;
	BYTE	Ammo2;
} SelAmmo;


// this moved here from world.cpp, to allow classes to be derived from it
//=======================
// CWorld
//
// This spawns first when each level begins.
//=======================


class CWorld : public CBaseEntity
{
public:
	

	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	// aone
	void WeaponModelKey(KeyValueData* pkvd);
	void PrecacheStandardWeaponModels();
	void OnCreate();
	void OnDestroy();	//For PhysX - Protector 2009-10-21


	char sentenceTxt[256];	// new sentence.txt file
	// end aone

	char WorldSoundOriginal[256][128]; // Original Sounds
	char WorldSoundReplacement[256][128]; // Sounds listed to replace the original sound array.
	int	 gcWorldSounds; 

	//GMR-SvenV 
	char WorldModelOriginal[256][128]; // Original Sounds
	char WorldModelReplacement[256][128]; // Sounds listed to replace the original sound array.
	int	 gcWorldModels;

	const char *SOUNDREPLACEMENT_Find(const char *name);
	const char *MODELREPLACEMENT_Find(const char *name);//GMR-SvenV 
	void PrecacheGlobalCustomModels( int modelfile );//GMR-SvenV
};

extern const char* gszNoCustomMdl;

enum WEAPONMDLIDX
{
	WEAPONMDLIDX_W = 0,
	WEAPONMDLIDX_P = 1,
	WEAPONMDLIDX_V = 2,
};

const char* GetWeaponStandardModel(WEAPONMDLIDX idx,int iId,const char* alternative = NULL);

// aone - for weapon standard values
#define MAX_WEAPON_MODEL_NAME 64
typedef struct 
{
	char szV[MAX_WEAPON_MODEL_NAME];
	char szP[MAX_WEAPON_MODEL_NAME];
	char szW[MAX_WEAPON_MODEL_NAME];
} weapon_model_t;

extern const char* gszNoCustomMdl;
#define SetStandardToDefault(wpnmodel) { strcpy((wpnmodel),gszNoCustomMdl); }
#define ResetWeaponStandards(wpn) { SetStandardToDefault((wpn).szV);SetStandardToDefault((wpn).szP);SetStandardToDefault((wpn).szW); }

extern weapon_model_t g_pStandardWeaponModels[32];

#ifdef _DEBUG
#define _DEBUG_LINE(x,dbgstring) { x; ALERT(at_console,"(%s|%d): %s :\"%s\"\n",__FILE__,__LINE__,#x,dbgstring); }
#else
#define _DEBUG_LINE(x,dbgstring) x
#endif

// for cvars, its MUCH faster
extern float cvarValFloat;
extern const char* cvarValString;
#define InternCvarGetFloat(cVarVarName) {extern cvar_t cVarVarName;cvarValFloat = atof(cVarVarName.string);}
#define InternCvarGetString(cVarVarName) {extern cvar_t cVarVarName;cvarValString = cVarVarName.string;}

// end aone


