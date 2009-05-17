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

/* Egoboo - proto.h
 * Function prototypes for a huge portion of the game code.
 */

#include "egoboo_typedef.h"

struct s_script_state;

///////////////////////////////
// INTERNAL FUNCTIONS
///////////////////////////////
// internal game functions that would never be called by a script

struct s_mesh;

// loading/saving
struct s_mesh * mesh_load( const char *modname, struct s_mesh * pmesh );

// object initialization
void  prime_names( void );

///////////////////////////////
// EXTERNAL FUNCTIONS
///////////////////////////////
// functions that might be called by a script

// the hook for deinitializing an old module
void   game_quit_module();

// the hook for initializing a new module
bool_t game_init_module( const char * modname, Uint32 seed );

// the hook for exporting all the current players and reloading them
bool_t game_update_imports();

// Exporting stuff
void export_one_character( Uint16 character, Uint16 owner, int number, bool_t is_local );
void export_all_players( bool_t require_local );
void export_all_local_players( void );

// Messages
void display_message( struct s_script_state * pstate, int message, Uint16 character );
void debug_message( const char *text );
void show_stat( Uint16 statindex );
void show_armor( Uint16 statindex );
void show_full_status( Uint16 statindex );
void show_magic_status( Uint16 statindex );

// End Text
void reset_end_text();
void append_end_text( struct s_script_state * pstate, int message, Uint16 character );

// Particles
Uint16 number_of_attached_particles( Uint16 character );
void   spawn_bump_particles( Uint16 character, Uint16 particle );
void   attach_particle_to_character( Uint16 particle, Uint16 character, int grip );
void   disaffirm_attached_particles( Uint16 character );
void   reaffirm_attached_particles( Uint16 character );
Uint16 number_of_attached_particles( Uint16 character );

// Statlist
void statlist_add( Uint16 character );
void statlist_move_to_top( Uint16 character );
void statlist_sort();

// Math
int    generate_number( int numbase, int numrand );
Uint16 terp_dir( Uint16 majordir, Uint16 minordir );
Uint16 terp_dir_fast( Uint16 majordir, Uint16 minordir );
void   getadd( int min, int value, int max, int* valuetoadd );
void   fgetadd( float min, float value, float max, float* valuetoadd );

// Action
int  action_which( char cTmp );
void chr_play_action( Uint16 character, Uint16 action, Uint8 actionready );
void chr_set_frame( Uint16 character, Uint16 action, int frame, Uint16 lip );

// Mesh
float get_level( float x, float y, bool_t waterwalk );
Uint32 mesh_get_block( float pos_x, float pos_y );
Uint32 mesh_get_tile ( float pos_x, float pos_y );

Uint32 mesh_get_block_int( struct s_mesh * pmesh, int block_x, int block_y );
Uint32 mesh_get_tile_int( struct s_mesh * pmesh, int tile_x,  int tile_y );

// Player
void set_one_player_latch( Uint16 player );
int  add_player( Uint16 character, Uint16 player, Uint32 device );

// Module
bool_t load_blip_bitmap();
void   quit_module();

//---------------------------------------------------------------------------------------------
//AI targeting
Uint16 get_target( Uint16 character, Uint32 maxdistance, TARGET_TYPE team, bool_t targetitems, bool_t targetdead, IDSZ idsz, bool_t excludeidsz);
Uint16 get_particle_target( float xpos, float ypos, float zpos, Uint16 facing,
                            Uint16 particletype, Uint8 team, Uint16 donttarget,
                            Uint16 oldtarget );

#define _PROTO_H_
