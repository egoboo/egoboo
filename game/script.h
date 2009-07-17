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
#define NEARBY      3*TILE_SIZE    //3 tiles away
#define WIDE        6*TILE_SIZE    //6 tiles away
#define NEAREST     0              //unlimited range

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
#define STORAND             15

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
    Uint32         timer;         // AI Timer
    int            x[MAXSTOR];    // Temporary values...  SetXY
    int            y[MAXSTOR];

    // ai memory from the last event
    Uint16         bumplast;        // Last character it was bumped by
    Uint16         attacklast;      // Last character it was attacked by
    Uint16         hitlast;         // Last character it hit
    Uint16         directionlast;   // Direction of last attack/healing
    Uint16         damagetypelast;  // Last damage type
    Uint16         lastitemused;    // The last item the character used
    Uint16         target_old;		// Target in the previous update

    // message handling
    Uint32         order_value;           // The last order given the character
    Uint16         order_counter;         // The rank of the character on the order chain

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
