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

/// @file egolib/Script/script.h

#pragma once

#include "egolib/typedef.h"
#include "egolib/Logic/Damage.hpp"
#include "egolib/clock.h"
#include "egolib/AI/WaypointList.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct script_info_t;
struct ai_state_t;
struct script_state_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

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
#define NEARBY      3*GRID_FSIZE    ///< 3 tiles away
#define WIDE        6*GRID_FSIZE    ///< 6 tiles away
#define NEAREST     0              ///< unlimited range

//Max size of an compiled AI script
#define MAXAICOMPILESIZE    2048

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Character AI alerts
enum chr_alert_bits
{
    ALERT_NONE                           = 0,
    ALERTIF_SPAWNED                      = 1 <<  0,
    ALERTIF_HITVULNERABLE                = 1 <<  1,
    ALERTIF_ATWAYPOINT                   = 1 <<  2,
    ALERTIF_ATLASTWAYPOINT               = 1 <<  3,
    ALERTIF_ATTACKED                     = 1 <<  4,
    ALERTIF_BUMPED                       = 1 <<  5,
    ALERTIF_ORDERED                      = 1 <<  6,
    ALERTIF_CALLEDFORHELP                = 1 <<  7,
    ALERTIF_KILLED                       = 1 <<  8,
    ALERTIF_TARGETKILLED                 = 1 <<  9,
    ALERTIF_DROPPED                      = 1 << 10,
    ALERTIF_GRABBED                      = 1 << 11,
    ALERTIF_REAFFIRMED                   = 1 << 12,
    ALERTIF_LEADERKILLED                 = 1 << 13,
    ALERTIF_USED                         = 1 << 14,
    ALERTIF_CLEANEDUP                    = 1 << 15,
    ALERTIF_SCOREDAHIT                   = 1 << 16,
    ALERTIF_HEALED                       = 1 << 17,
    ALERTIF_DISAFFIRMED                  = 1 << 18,
    ALERTIF_CHANGED                      = 1 << 19,
    ALERTIF_INWATER                      = 1 << 20,
    ALERTIF_BORED                        = 1 << 21,
    ALERTIF_TOOMUCHBAGGAGE               = 1 << 22,
    ALERTIF_LEVELUP                      = 1 << 23,
    ALERTIF_CONFUSED                     = 1 << 24,
    ALERTIF_HITGROUND                    = 1 << 25,
    ALERTIF_NOTDROPPED                   = 1 << 26,
    ALERTIF_BLOCKED                      = 1 << 27,
    ALERTIF_THROWN                       = 1 << 28,
    ALERTIF_CRUSHED                      = 1 << 29,
    ALERTIF_NOTPUTAWAY                   = 1 << 30,
    ALERTIF_TAKENOUT                     = 1 << 31,

    // add in some aliases
    ALERTIF_PUTAWAY     = ALERTIF_ATLASTWAYPOINT,
    ALERTIF_NOTTAKENOUT = ALERTIF_NOTPUTAWAY
};

// swig chokes on the definition below
#if defined(SWIG)
#   define STOR_BITS            4
#   define STOR_COUNT          16                      ///< Storage data (Used in SetXY)
#   define STOR_AND            15                      ///< Storage data bitmask
#else
#   define STOR_BITS            4
#   define STOR_COUNT          (1 << STOR_BITS)        ///< Storage data (Used in SetXY)
#   define STOR_AND            (STOR_COUNT - 1)        ///< Storage data bitmask
#endif

//--------------------------------------------------------------------------------------------
// struct script_info_t
//--------------------------------------------------------------------------------------------
struct script_info_t
{
public:
    script_info_t() :
        name(),
        indent(0),
        indent_last(0),
        length(0),
        position(0),
        data{}
    {
        //ctor
    }

public:
    STRING          name;                            // Name of the script file

    uint32_t        indent;
    uint32_t        indent_last;

    uint32_t        length;                          // Actual length of the compiled ai buffer
    size_t          position;                        // Our current position in the script

    uint32_t        data[MAXAICOMPILESIZE];          // Compiled script data
};

//--------------------------------------------------------------------------------------------
// struct ai_state_t
//--------------------------------------------------------------------------------------------

/// the state variables for a script / AI
struct ai_state_t
{
    // some script states
    Sint32         poof_time;
    bool           changed;
    bool           terminate;

    // who are we related to?
    CHR_REF        index;         ///< what is the index value of this character
    CHR_REF        target;        ///< Who the AI is after
    CHR_REF        owner;         ///< The character's owner
    CHR_REF        child;         ///< The character's child

    // some local storage
    BIT_FIELD      alert;         ///< Alerts for AI script
    int            state;         ///< Short term memory for AI
    int            content;       ///< More short term memory
    int            passage;       ///< The passage associated with this character
    Uint32         timer;         ///< AI Timer
    int            x[STOR_COUNT];    ///< Temporary values...  SetXY
    int            y[STOR_COUNT];

    // ai memory from the last event
    CHR_REF        bumplast;        ///< Last character it was bumped by
    int            bumplast_time;   ///< The last time that a ALERTIF_BUMPED was sent

    CHR_REF        attacklast;      ///< Last character it was attacked by
    CHR_REF        hitlast;         ///< Last character it hit
    FACING_T       directionlast;   ///< Direction of last attack/healing
    DamageType     damagetypelast;  ///< Last damage type
    CHR_REF        lastitemused;    ///< The last item the character used
    CHR_REF        target_old;      ///< Target in the previous update

    // message handling
    Uint32         order_value;           ///< The last order given the character
    Uint16         order_counter;         ///< The rank of the character on the order chain

    // waypoints
    bool            wp_valid;            ///< is the current waypoint valid?
    waypoint_t      wp;                  ///< current waypoint
    waypoint_list_t wp_lst;              ///< Stored waypoints
    Uint32          astar_timer;         ///< Throttle on astar pathfinding

    // performance monitoring
    PROFILE_DECLARE_STRUCT;
};

ai_state_t *ai_state_ctor(ai_state_t * pself);
ai_state_t *ai_state_dtor(ai_state_t * pself);
ai_state_t *ai_state_reconstruct(ai_state_t * pself);
bool        ai_state_set_bumplast(ai_state_t * pself, const CHR_REF  ichr);
bool        ai_state_get_wp(ai_state_t * pself);
bool        ai_state_ensure_wp(ai_state_t * pself);
bool        ai_add_order(ai_state_t * pai, Uint32 value, Uint16 counter);
bool        ai_state_set_changed(ai_state_t * pai);
void        ai_state_spawn(ai_state_t * pself, const CHR_REF index, const PRO_REF iobj, Uint16 rank);

//--------------------------------------------------------------------------------------------
// struct script_state_t
//--------------------------------------------------------------------------------------------

/// The state of the scripting system
/// @details It is not persistent between one evaluation of a script and another
struct script_state_t
{
    int     x;
    int     y;
    int     turn;
    int     distance;
    int     argument;
    int     operationsum;
};

script_state_t * script_state__init( script_state_t * );

//--------------------------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

void  scr_run_chr_script( const CHR_REF character );

void issue_order( const CHR_REF character, Uint32 order );
void issue_special_order( Uint32 order, IDSZ idsz );
void set_alerts( const CHR_REF character );

void scripting_system_begin();
void scripting_system_end();
