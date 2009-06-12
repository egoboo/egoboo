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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_mesh;
struct s_camera;
struct s_script_state;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define TILESOUNDTIME 16
#define TILEREAFFIRMAND  3

#define MAXWATERLAYER 2                             // Maximum water layers
#define MAXWATERFRAME 512                           // Maximum number of wave frames
#define WATERFRAMEAND (MAXWATERFRAME-1)
#define WATERPOINTS 4                               // Points in a water fan

enum e_latchbutton
{
    LATCHBUTTON_LEFT      = ( 1 << 0 ),                      // Character button presses
    LATCHBUTTON_RIGHT     = ( 1 << 1 ),
    LATCHBUTTON_JUMP      = ( 1 << 2 ),
    LATCHBUTTON_ALTLEFT   = ( 1 << 3 ),                      // ( Alts are for grab/drop )
    LATCHBUTTON_ALTRIGHT  = ( 1 << 4 ),
    LATCHBUTTON_PACKLEFT  = ( 1 << 5 ),                     // ( Packs are for inventory cycle )
    LATCHBUTTON_PACKRIGHT = ( 1 << 6 ),
    LATCHBUTTON_RESPAWN   = ( 1 << 7 )
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// animtile data in the wawalite.txt file
struct s_animtile_data
{
    int    update_and;             // New tile every 7 frames
    Uint16 frame_and;              // Only 4 frames for small tiles
    Uint16 frame_add;              // Current frame

};
typedef struct s_animtile_data animtile_data_t;
extern animtile_data_t animtile_data;

//--------------------------------------------------------------------------------------------
struct s_animtile_instance
{
    Uint16 frame_and;
    Uint16 base_and;
};
typedef struct s_animtile_instance animtile_instance_t;
extern animtile_instance_t animtile[2];

//--------------------------------------------------------------------------------------------
// damagetile data in the wawalite.txt file
struct s_damagetile_data
{
    Sint16  parttype;
    Sint16  partand;
    Sint16  sound;
    Uint8   type;                      // Type of damage
    int     amount;                    // Amount of damage
};
typedef struct s_damagetile_data damagetile_data_t;
extern damagetile_data_t damagetile_data;

//--------------------------------------------------------------------------------------------
struct s_damagetile_instance
{
    Sint16  sound_time;
    Uint16  min_distance;
};
typedef struct s_damagetile_instance damagetile_instance_t;
extern damagetile_instance_t damagetile;

//--------------------------------------------------------------------------------------------
// weather data in the wawalite.txt file
struct s_weather_data
{
    int over_water;           // Only spawn over water?
    int timer_reset;          // Rate at which weather particles spawn
};
typedef struct s_weather_data weather_data_t;
extern weather_data_t weather_data;

//--------------------------------------------------------------------------------------------
struct s_weather_instance
{
    Uint16  iplayer;
    int     time;                // 0 is no weather
};
typedef struct s_weather_instance weather_instance_t;
extern weather_instance_t weather;

//--------------------------------------------------------------------------------------------
// water data in the wawalite.txt file
struct s_water_data
{
    float   surface_level;          // Surface level for water striders
    float   douse_level;            // Surface level for torches
    Uint8   spek_start;           // Specular begins at which light value
    Uint8   spek_level;           // General specular amount (0-255)
    bool_t  is_water;          // Is it water?  ( Or lava... )
    bool_t  background_req;
    bool_t  overlay_req;

    Uint8   light;                               // Is it light ( default is alpha )
    Uint8   light_level[MAXWATERLAYER];          // General light amount (0-63)
    Uint8   light_add[MAXWATERLAYER];            // Ambient light amount (0-63)

    int     layer_count;                         // Number of layers
    float   layer_z[MAXWATERLAYER];              // Base height of water
    Uint8   layer_alpha[MAXWATERLAYER];          // Transparency
    float   layer_u_add[MAXWATERLAYER];          // Texture movement
    float   layer_v_add[MAXWATERLAYER];
    Uint16  layer_frame_add[MAXWATERLAYER];      // Speed
    float   layer_dist_x[MAXWATERLAYER];         // For distant backgrounds
    float   layer_dist_y[MAXWATERLAYER];
    float   layer_amp[MAXWATERLAYER];   // Amplitude of waves

    float   foregroundrepeat;
    float   backgroundrepeat;

};
typedef struct s_water_data water_data_t;
extern water_data_t water_data;

//--------------------------------------------------------------------------------------------
struct s_water_instance
{
    float  surface_level;          // Surface level for water striders
    float  douse_level;            // Surface level for torches
    Uint32 spek[256];              // Specular highlights

    Uint16 layer_frame[MAXWATERLAYER]; // Frame
    float  layer_z[MAXWATERLAYER];     // Base height of water
    float  layer_u[MAXWATERLAYER];     // Coordinates of texture
    float  layer_v[MAXWATERLAYER];

    float  layer_z_add[MAXWATERLAYER][MAXWATERFRAME][WATERPOINTS];
    Uint8  layer_color[MAXWATERLAYER][MAXWATERFRAME][WATERPOINTS];
};

typedef struct s_water_instance water_instance_t;
extern water_instance_t water;

//--------------------------------------------------------------------------------------------
// fog data in the wawalite.txt file
struct s_fog_data
{
    float  top, bottom;
    Uint8  red, grn, blu;
    bool_t affects_water;
};
typedef struct s_fog_data fog_data_t;
extern fog_data_t fog_data;

//--------------------------------------------------------------------------------------------
struct s_fog_instance
{
    bool_t  on;            // Do ground fog?
    float   top, bottom;
    Uint8   red, grn, blu;
    float   distance;
};
typedef struct s_fog_instance fog_instance_t;
extern fog_instance_t fog;

//--------------------------------------------------------------------------------------------
// Status displays

#define MAXSTAT             16                      // Maximum status displays

extern bool_t staton;
extern int    statdelay;
extern int    numstat;
extern Uint16 statlist[MAXSTAT];

//--------------------------------------------------------------------------------------------
//End text
#define MAXENDTEXT 1024

extern char   endtext[MAXENDTEXT];     // The end-module text
extern int    endtextwrite;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern bool_t    gamepaused;            // Is the game paused?
extern bool_t    pausekeyready;         // Ready to pause game?
extern bool_t    overrideslots;         //Override existing slots?
extern bool_t    screenshotkeyready;    // Ready to take screenshot?

extern struct s_mesh   * PMesh;
extern struct s_camera * PCamera;

//Pitty stuff
extern bool_t  pitskill;          // Do they kill?
extern bool_t  pitsfall;          // Do they teleport?
extern Uint32  pitx;
extern Uint32  pity;
extern Uint32  pitz;

extern Uint16  glouseangle;                                        // actually still used

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// the hook for deinitializing an old module
void   game_quit_module();

// the hook for initializing a new module
bool_t game_init_module( const char * modname, Uint32 seed );

// the hook for exporting all the current players and reloading them
bool_t game_update_imports();
void   game_finish_module();
bool_t game_begin_module( const char * modname, Uint32 seed );

// Exporting stuff
void export_one_character( Uint16 character, Uint16 owner, int number, bool_t is_local );
void export_all_players( bool_t require_local );

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
void   attach_particle_to_character( Uint16 particle, Uint16 character, int vertex_offset );
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

// Player
void set_one_player_latch( Uint16 player );
int  add_player( Uint16 character, Uint16 player, Uint32 device );

//AI targeting
Uint16 get_target( Uint16 character, Uint32 maxdistance, TARGET_TYPE team, bool_t targetitems, bool_t targetdead, IDSZ idsz, bool_t excludeidsz);
Uint16 get_particle_target( float pos_x, float pos_y, float pos_z, Uint16 facing,
                            Uint16 particletype, Uint8 team, Uint16 donttarget,
                            Uint16 oldtarget );

// object initialization
void  prime_names( void );
void  free_all_objects( void );

// Data
struct s_mesh   * set_PMesh( struct s_mesh * pmpd );
struct s_camera * set_PCamera( struct s_camera * pcam );

bool_t animtile_data_init( animtile_data_t * pdata );
bool_t animtile_instance_init( animtile_instance_t pinst[], animtile_data_t * pdata );

bool_t damagetile_data_init( damagetile_data_t * pdata );
bool_t damagetile_instance_init( damagetile_instance_t * pinst, damagetile_data_t * pdata );

bool_t weather_data_init( weather_data_t * pdata );
bool_t weather_instance_init( weather_instance_t * pinst, weather_data_t * pdata );

bool_t water_data_init( water_data_t * pdata );
bool_t water_instance_init( water_instance_t * pinst, water_data_t * pdata );

bool_t fog_data_init( fog_data_t * pdata );
bool_t fog_instance_init( fog_instance_t * pinst, fog_data_t * pdata );

bool_t animtile_data_init( animtile_data_t * pdata );
bool_t animtile_instance_init( animtile_instance_t pinst[], animtile_data_t * pdata );

float get_mesh_level( struct s_mesh * pmesh, float x, float y, bool_t waterwalk );

bool_t make_water( water_instance_t * pinst, water_data_t * pdata );
void read_wawalite( const char *modname );
