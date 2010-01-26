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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "egoboo_typedef.h"

#include "clock.h"

#define MSGDISTANCE         2000                    ///< Range for SendMessageNear
#define PITNOSOUND          -256                    ///< Stop sound at bottom of pits...

/// These are for FindPath function only
#define   MOVE_MELEE  300
#define   MOVE_RANGED  600
#define   MOVE_DISTANCE 175
#define   MOVE_RETREAT  900
#define   MOVE_CHARGE  111
#define   MOVE_FOLLOW  0

/// Camera bounds/edge of the map
#define EDGE                128

/// AI targeting
#define NEARBY      3*GRID_SIZE    ///< 3 tiles away
#define WIDE        6*GRID_SIZE    ///< 6 tiles away
#define NEAREST     0              ///< unlimited range

/// Character AI alerts
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
/// AI variables
#define MAXWAY              8                       ///< Waypoints
#define WAYTHRESH           (TILE_ISIZE >> 1)       ///< Threshold for reaching waypoint (GRID_SIZE/2)

// swig chokes on the definition below
#ifdef SWIG
#   define STOR_BITS            4
#   define STOR_COUNT          16                      ///< Storage data (Used in SetXY)
#   define STOR_AND            15                      ///< Storage data bitmask
#else
#   define STOR_BITS            4
#   define STOR_COUNT          (1 << STOR_BITS)        ///< Storage data (Used in SetXY)
#   define STOR_AND            (STOR_COUNT - 1)        ///< Storage data bitmask
#endif

typedef float waypoint_t[3];

struct s_waypoint_list
{
    int          tail;         ///< Which waypoint
    int          head;         ///< Where to stick next
    waypoint_t   pos[MAXWAY];  ///< Waypoint
};
typedef struct s_waypoint_list waypoint_list_t;

bool_t waypoint_list_peek( waypoint_list_t * plst, waypoint_t wp );
bool_t waypoint_list_push( waypoint_list_t * plst, int x, int y );
bool_t waypoint_list_reset( waypoint_list_t * plst );
bool_t waypoint_list_clear( waypoint_list_t * plst );
bool_t waypoint_list_empty( waypoint_list_t * plst );
bool_t waypoint_list_finished( waypoint_list_t * plst );
bool_t waypoint_list_advance( waypoint_list_t * plst );

/// the state variables for a script / AI
struct s_ai_state
{
    // which script to run
    REF_T          type;

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
    REF_T          index;         ///< what is the index value of this character
    REF_T          target;        ///< Who the AI is after
    REF_T          owner;         ///< The character's owner
    REF_T          child;         ///< The character's child

    // some local storage
    Uint32         alert;         ///< Alerts for AI script
    int            state;         ///< Short term memory for AI
    int            content;       ///< More short term memory
    int            passage;       ///< The passage associated with this character
    Uint32         timer;         ///< AI Timer
    int            x[STOR_COUNT];    ///< Temporary values...  SetXY
    int            y[STOR_COUNT];

    // ai memory from the last event
    REF_T          bumplast;        ///< Last character it was bumped by
    int            bumplast_time;   ///< The last time that a ALERTIF_BUMPED was sent

    REF_T          attacklast;      ///< Last character it was attacked by
    REF_T          hitlast;         ///< Last character it hit
    FACING_T       directionlast;   ///< Direction of last attack/healing
    Uint16         damagetypelast;  ///< Last damage type
    REF_T          lastitemused;    ///< The last item the character used
    REF_T          target_old;      ///< Target in the previous update

    // message handling
    Uint32         order_value;           ///< The last order given the character
    Uint16         order_counter;         ///< The rank of the character on the order chain

    // waypoints
    bool_t          wp_valid;            ///< is the current waypoint valid?
    waypoint_t      wp;                  ///< current waypoint
    waypoint_list_t wp_lst;              ///< Stored waypoints

    // performance monitoring
    PROFILE_DECLARE_STRUCT;
};
typedef struct s_ai_state ai_state_t;

ai_state_t * ai_state_ctor( ai_state_t * pself );
ai_state_t * ai_state_dtor( ai_state_t * pself );
bool_t       ai_state_set_bumplast( ai_state_t * pself, REF_T ichr );
bool_t       ai_state_get_wp( ai_state_t * pself );
bool_t       ai_state_ensure_wp( ai_state_t * pself );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// The state of the scripting system
/// @delails It is not persistent between one evaluation of a script and another
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
/// Prototypes
void  scr_run_chr_script( REF_T character );

void issue_order( REF_T character, Uint32 order );
void issue_special_order( Uint32 order, IDSZ idsz );
void set_alerts( REF_T character );

void scripting_system_begin();
void scripting_system_end();
