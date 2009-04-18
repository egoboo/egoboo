#pragma once

//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "egoboo_typedef.h"

#define MSGDISTANCE         2000                    // Range for SendMessageNear
#define PITNOSOUND          -256                    // Stop sound at bottom of pits...

// These are for FindPath function only
#define   MOVE_MELEE  300
#define   MOVE_RANGED  600
#define   MOVE_DISTANCE 175
#define   MOVE_RETREAT  900
#define   MOVE_CHARGE  111
#define   MOVE_FOLLOW  0

//AI targeting
#define NEARBY      3*TILESIZE    //3 tiles
#define WIDE        6*TILESIZE    //8 tiles
#define NEAREST     0           //unlimited range
#define TILESIZE    128       //Size of one texture tile in egoboo

// Character AI alerts
#define ALERTIF_SPAWNED                      ( 1 <<  0 )
#define ALERTIF_HITVULNERABLE                ( 1 <<  1 )
#define ALERTIF_ATWAYPOINT                   ( 1 <<  2 )
#define ALERTIF_ATLASTWAYPOINT               ( 1 <<  3 )
#define ALERTIF_ATTACKED                     ( 1 <<  4 )
#define ALERTIF_BUMPED                       ( 1 <<  5 )
#define ALERTIF_ORDERED                      ( 1 <<  6 )
#define ALERTIF_CALLEDFORHELP                ( 1 <<  7 )
#define ALERTIF_KILLED                       ( 1 <<  8 )
#define ALERTIF_TARGETKILLED                 ( 1 <<  9 )
#define ALERTIF_DROPPED                      ( 1 << 10 )
#define ALERTIF_GRABBED                      ( 1 << 11 )
#define ALERTIF_REAFFIRMED                   ( 1 << 12 )
#define ALERTIF_LEADERKILLED                 ( 1 << 13 )
#define ALERTIF_USED                         ( 1 << 14 )
#define ALERTIF_CLEANEDUP                    ( 1 << 15 )
#define ALERTIF_SCOREDAHIT                   ( 1 << 16 )
#define ALERTIF_HEALED                       ( 1 << 17 )
#define ALERTIF_DISAFFIRMED                  ( 1 << 18 )
#define ALERTIF_CHANGED                      ( 1 << 19 )
#define ALERTIF_INWATER                      ( 1 << 20 )
#define ALERTIF_BORED                        ( 1 << 21 )
#define ALERTIF_TOOMUCHBAGGAGE               ( 1 << 22 )
#define ALERTIF_GROGGED                      ( 1 << 23 )
#define ALERTIF_DAZED                        ( 1 << 24 )
#define ALERTIF_HITGROUND                    ( 1 << 25 )
#define ALERTIF_NOTDROPPED                   ( 1 << 26 )
#define ALERTIF_BLOCKED                      ( 1 << 27 )
#define ALERTIF_THROWN                       ( 1 << 28 )
#define ALERTIF_CRUSHED                      ( 1 << 29 )
#define ALERTIF_NOTPUTAWAY                   ( 1 << 30 )
#define ALERTIF_TAKENOUT                     ( 1 << 31 )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// AI variables

#define MAXWAY              8                       // Waypoints
#define WAYTHRESH           64                      // Threshold for reaching waypoint
#define MAXSTOR             16                      // Storage data (Used in SetXY)
#define STORAND             15                      //

struct s_ai_state
{
    // which script to run
    Uint16         type;

    // the execution pointer(s)
    size_t         exe_stt;
    size_t         exe_end;
    size_t         exe_pos;
    Uint32         opcode;

    // some script states
    Sint32         poof_time;
    bool_t         changed;
    bool_t         terminate;
    Uint32         indent;
    Uint32         indent_last;

    // who are we related to?
    Uint16         index;         // what is the index value of this character
    Uint16         target;        // Who the AI is after
    Uint16         owner;         // The character's owner
    Uint16         child;         // The character's child

    // some local storage
    Uint32         alert;         // Alerts for AI script
    int            state;         // Short term memory for AI
    int            content;       // More short term memory
    int            passage;       // The passage associated with this character
    int            timer;         // AI Timer
    int            x[MAXSTOR];    // Temporary values...  SetXY
    int            y[MAXSTOR];    //

    // ai memory from the last event
    Uint16         bumplast;        // Last character it was bumped by
    Uint16         attacklast;      // Last character it was attacked by
    Uint16         hitlast;         // Last character it hit
    Uint16         directionlast;   // Direction of last attack/healing
    Uint16         damagetypelast;  // Last damage type
    Uint16         lastitemused;    // The last item the character used
    Uint16         target_old;

    // message handling
    Uint32         order;           // The last order given the character
    Uint16         rank;           // The rank of the character on the order chain

    // waypoints
    Uint8          wp_tail;          // Which waypoint
    Uint8          wp_head;          // Where to stick next
    float          wp_pos_x[MAXWAY]; // Waypoint
    float          wp_pos_y[MAXWAY]; // Waypoint
};
typedef struct s_ai_state ai_state_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_script_state
{
    int     x;
    int     y;
    int     turn;
    int     distance;
    int     argument;
    int     operationsum;
};
typedef struct s_script_state script_state_t;


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//Prototypes
void  let_character_think( Uint16 character );

void issue_order( Uint16 character, Uint32 order );
void issue_special_order( Uint32 order, IDSZ idsz );
void set_alerts( Uint16 character );

Uint8 scr_set_AlertBit( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ClearAlertBit( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TestAlertBit( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_Alert( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ClearAlert( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TestAlert( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_Bit( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ClearBit( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TestBit( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_Bits( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ClearBits( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TestBits( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Spawned( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TimeOut( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_AtWaypoint( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_AtLastWaypoint( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Attacked( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Bumped( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Ordered( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_CalledForHelp( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_Content( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Killed( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetKilled( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ClearWaypoints( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_AddWaypoint( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_FindPath( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Compass( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_TargetArmorPrice( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_Time( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_Content( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_JoinTargetTeam( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToNearbyEnemy( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToTargetLeftHand( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToTargetRightHand( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToWhoeverAttacked( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToWhoeverBumped( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToWhoeverCalledForHelp( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToOldTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TurnModeToVelocity( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TurnModeToWatch( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TurnModeToSpin( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_BumpHeight( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetHasID( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetHasItemID( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetHoldingItemID( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetHasSkillID( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Else( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Run( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Walk( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Sneak( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DoAction( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_KeepAction( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_IssueOrder( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DropWeapons( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetDoAction( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_OpenPassage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ClosePassage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_PassageOpen( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_GoPoof( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_CostTargetItemID( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DoActionOverride( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Healed( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SendPlayerMessage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_CallForHelp( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_AddIDSZ( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_State( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_State( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetCanOpenStuff( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Grabbed( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Dropped( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToWhoeverIsHolding( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DamageTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_XIsLessThanY( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_WeatherTime( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_BumpHeight( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Reaffirmed( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_UnkeepAction( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsOnOtherTeam( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsOnHatedTeam( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_PressLatchButton( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToTargetOfLeader( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_LeaderKilled( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_BecomeLeader( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ChangeTargetArmor( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_GiveMoneyToTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DropKeys( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_LeaderIsAlive( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsOldTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToLeader( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SpawnCharacter( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_RespawnCharacter( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ChangeTile( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Used( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DropMoney( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_OldTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DetachFromHolder( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetHasVulnerabilityID( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_CleanUp( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_CleanedUp( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Sitting( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsHurt( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsAPlayer( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_PlaySound( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SpawnParticle( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsAlive( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Stop( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DisaffirmCharacter( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ReaffirmCharacter( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsSelf( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsMale( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsFemale( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToSelf( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToRider( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_AttackTurn( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_DamageType( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_BecomeSpell( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_BecomeSpellbook( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ScoredAHit( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Disaffirmed( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TranslateOrder( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToWhoeverWasHit( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToWideEnemy( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Changed( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_InWater( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Bored( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TooMuchBaggage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Grogged( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Dazed( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetHasSpecialID( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_PressTargetLatchButton( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Invisible( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ArmorIs( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_TargetGrogTime( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_TargetDazeTime( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_DamageType( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_WaterLevel( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_EnchantTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_EnchantChild( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TeleportTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_GiveExperienceToTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_IncreaseAmmo( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_UnkurseTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_GiveExperienceToTargetTeam( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Unarmed( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_RestockTargetAmmoIDAll( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_RestockTargetAmmoIDFirst( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_FlashTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_RedShift( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_GreenShift( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_BlueShift( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_Light( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_Alpha( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HitFromBehind( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HitFromFront( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HitFromLeft( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HitFromRight( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsOnSameTeam( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_KillTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_UndoEnchant( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_WaterLevel( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_CostTargetMana( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetHasAnyID( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_BumpSize( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_NotDropped( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_YIsLessThanX( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_FlyHeight( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Blocked( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsDefending( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsAttacking( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs0( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs1( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs2( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs3( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs4( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs5( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs6( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs7( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ContentIs( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TurnModeToWatchTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIsNot( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_XIsEqualToY( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DebugMessage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_BlackTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SendMessageNear( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HitGround( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_NameIsKnown( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_UsageIsKnown( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HoldingItemID( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HoldingRangedWeapon( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HoldingMeleeWeapon( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HoldingShield( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Kursed( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsKursed( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsDressedUp( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_OverWater( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Thrown( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_MakeNameKnown( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_MakeUsageKnown( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StopTargetMovement( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_XY( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_XY( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_AddXY( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_MakeAmmoKnown( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SpawnAttachedParticle( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SpawnExactParticle( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_AccelerateTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_distanceIsMoreThanTurn( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Crushed( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_MakeCrushValid( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToLowestTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_NotPutAway( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TakenOut( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_AmmoOut( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_PlaySoundLooped( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StopSound( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HealSelf( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Equip( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetHasItemIDEquipped( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_OwnerToTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToOwner( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_Frame( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_BreakPassage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_ReloadTime( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToWideBlahID( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_PoofTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ChildDoActionOverride( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SpawnPoof( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_SpeedPercent( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_ChildState( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SpawnAttachedSizedParticle( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ChangeArmor( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ShowTimer( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_FacingTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_PlaySoundVolume( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SpawnAttachedFacedParticle( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIsOdd( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToDistantEnemy( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Teleport( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_GiveStrengthToTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_GiveWisdomToTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_GiveIntelligenceToTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_GiveDexterityToTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_GiveLifeToTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_GiveManaToTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ShowMap( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ShowYouAreHere( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ShowBlipXY( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HealTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_PumpTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_CostAmmo( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_MakeSimilarNamesKnown( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SpawnAttachedHolderParticle( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetReloadTime( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_FogLevel( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_FogLevel( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_FogTAD( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_FogBottomLevel( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_FogBottomLevel( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_CorrectActionForHand( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsMounted( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SparkleIcon( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_UnsparkleIcon( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_TileXY( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TileXY( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_ShadowSize( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_OrderTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToWhoeverIsInPassage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_CharacterWasABook( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_EnchantBoostValues( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SpawnCharacterXYZ( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SpawnExactCharacterXYZ( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ChangeTargetClass( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_PlayFullSound( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SpawnExactChaseParticle( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_CreateOrder( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_OrderSpecialID( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_UnkurseTargetInventory( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsSneaking( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DropItems( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_RespawnTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetDoActionSetFrame( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetCanSeeInvisible( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToNearestBlahID( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToNearestEnemy( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToNearestFriend( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToNearestLifeform( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_FlashPassage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_FindTileInPassage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HeldInLeftHand( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_NotAnItem( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_ChildAmmo( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HitVulnerable( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsFlying( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_IdentifyTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_BeatModule( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_EndModule( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DisableExport( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_EnableExport( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_TargetState( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Equipped( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DropTargetMoney( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_TargetContent( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DropTargetKeys( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_JoinTeam( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetJoinTeam( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ClearMusicPassage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_ClearEndMessage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_AddEndMessage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_PlayMusic( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_MusicPassage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_MakeCrushInvalid( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StopMusic( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_FlashVariable( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_AccelerateUp( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_FlashVariableHeight( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_DamageTime( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs8( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs9( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs10( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs11( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs12( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs13( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs14( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_StateIs15( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsAMount( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsAPlatform( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_AddStat( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DisenchantTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DisenchantAll( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_VolumeNearestTeammate( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_AddShopPassage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetPayForArmor( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_JoinEvilTeam( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_JoinNullTeam( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_JoinGoodTeam( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_PitsKill( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToPassageID( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_MakeNameUnknown( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SpawnExactParticleEndSpawn( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SpawnPoofSpeedSpacingDamage( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_GiveExperienceToGoodTeam( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DoNothing( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_GrogTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DazeTarget( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_EnableRespawn( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_DisableRespawn( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HolderScoredAHit( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_HolderBlocked( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetHasNotFullMana( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_EnableListenSkill( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TargetToLastItemUsed( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_FollowLink( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_OperatorIsLinux( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsAWeapon( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_SomeoneIsStealing( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsASpell( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_Backstabbed( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_get_TargetDamageType( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_AddQuest( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_BeatQuestAllPlayers( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetHasQuest( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_QuestLevel( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_AddQuestAllPlayers( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_AddBlipAllEnemies( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_PitsFall( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TargetIsOwner( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_End( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_TakePicture( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_Speech( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_MoveSpeech( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_SecondMoveSpeech( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_AttackSpeech( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_AssistSpeech( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_TerrainSpeech( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_set_SelectSpeech( script_state_t * pstate, ai_state_t * pself );
Uint8 scr_OperatorIsMacintosh( script_state_t * pstate, ai_state_t * pself );