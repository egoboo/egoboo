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
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_ego_mpd;
struct s_camera;
struct s_script_state;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// My lil' random number table
#ifdef SWIG
// swig chokes on the definition below
#    define RANDIE_BITS    12
#    define RANDIE_COUNT 4096
#else
#    define RANDIE_BITS   12
#    define RANDIE_COUNT (1 << RANDIE_BITS)
#endif

#define RANDIE_MASK  ((Uint32)(RANDIE_COUNT - 1))
#define RANDIE       randie[randindex & RANDIE_MASK ];  randindex++; randindex &= RANDIE_MASK

extern Uint32  randindex;
extern Uint16  randie[RANDIE_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enum e_process_states
{
    proc_begin,
    proc_entering,
    proc_running,
    proc_leaving,
    proc_finish
};
typedef enum e_process_states process_state_t;

//--------------------------------------------------------------------------------------------

#define PROC_PBASE(PTR) (&( (PTR)->base ))

struct s_process_instance
{
    bool_t          valid;
    bool_t          paused;
    bool_t          killme;
    bool_t          terminated;
    process_state_t state;
    double          dtime;
};
typedef struct s_process_instance process_instance_t;

//--------------------------------------------------------------------------------------------
struct s_ego_process
{
    process_instance_t base;

    double frameDuration;
    int    menuResult;

    bool_t was_active;
    bool_t escape_requested, escape_latch;

    int    frame_next, frame_now;
};
typedef struct s_ego_process ego_process_t;

extern ego_process_t * EProc;

//--------------------------------------------------------------------------------------------
struct s_game_process
{
    process_instance_t base;

    double frameDuration;
    bool_t mod_paused, pause_key_ready;
    bool_t was_active;

    int    menu_depth;
    bool_t escape_requested, escape_latch;

    int    frame_next, frame_now;

};
typedef struct s_game_process game_process_t;

extern game_process_t * GProc;

//--------------------------------------------------------------------------------------------
struct s_menu_process
{
    process_instance_t base;

    bool_t was_active;
    bool_t escape_requested, escape_latch;

    int    frame_next, frame_now;
};
typedef struct s_menu_process menu_process_t;

extern menu_process_t * MProc;

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
    Uint16 frame_add;
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

struct s_water_layer_data
{
    Uint16    frame_add;    // Speed

    float     z;            // Base height of water
    float     amp;          // Amplitude of waves

    GLvector2 dist;         // For distant backgrounds
    Uint8     light_dir;    // direct  reflectivity 0 - 63
    Uint8     light_add;    // ambient reflectivity 0 - 63

    GLvector2 tx_add;       // Texture movement
    Uint8     alpha;        // Transparency
};
typedef struct s_water_layer_data water_data_layer_t;

struct s_water_data
{
    float   surface_level;          // Surface level for water striders
    float   douse_level;            // Surface level for torches
    Uint8   spek_start;             // Specular begins at which light value
    Uint8   spek_level;             // General specular amount (0-255)
    bool_t  is_water;               // Is it water?  ( Or lava... )
    bool_t  background_req;
    bool_t  overlay_req;

    bool_t  light;                               // Is it light ( default is alpha )

    int                layer_count;              // Number of layers
    water_data_layer_t layer[MAXWATERLAYER];     // layer data

    float   foregroundrepeat;
    float   backgroundrepeat;

};
typedef struct s_water_data water_data_t;
extern water_data_t water_data;

//--------------------------------------------------------------------------------------------

struct s_water_layer_instance
{
    Uint16    frame;        // Frame
    float     z;            // Base height of water
    GLvector2 dist;

    GLvector2 tx;           // Coordinates of texture

    float     light_dir;    // direct  reflectivity 0 - 1
    float     light_add;    // ambient reflectivity 0 - 1
};
typedef struct s_water_layer_instance water_instance_layer_t;

struct s_water_instance
{
    float  surface_level;          // Surface level for water striders
    float  douse_level;            // Surface level for torches
    Uint32 spek[256];              // Specular highlights

    water_instance_layer_t layer[MAXWATERLAYER];

    float  layer_z_add[MAXWATERLAYER][MAXWATERFRAME][WATERPOINTS];
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
// extern int    statdelay;
extern int    numstat;
extern Uint16 statlist[MAXSTAT];

//--------------------------------------------------------------------------------------------
// End text
#define MAXENDTEXT 1024

extern char   endtext[MAXENDTEXT];     // The end-module text
extern int    endtextwrite;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern bool_t    overrideslots;         // Override existing slots?
extern bool_t    screenshotkeyready;    // Ready to take screenshot?

extern struct s_ego_mpd         * PMesh;
extern struct s_camera          * PCamera;
extern struct s_module_instance * PMod;

// Pitty stuff
extern bool_t  pitskill;          // Do they kill?
extern bool_t  pitsfall;          // Do they teleport?
extern Uint32  pitx;
extern Uint32  pity;
extern Uint32  pitz;

extern Uint16  glouseangle;                                        // actually still used

// Sense enemies
extern Uint8  local_senseenemiesTeam;
extern IDSZ   local_senseenemiesID;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// the hook for deinitializing an old module
void   game_quit_module();

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

// AI targeting
Uint16 get_target( Uint16 character, Uint32 maxdistance, TARGET_TYPE team, bool_t targetitems, bool_t targetdead, IDSZ idsz, bool_t excludeidsz);
Uint16 get_particle_target( float pos_x, float pos_y, float pos_z, Uint16 facing,
                            Uint16 particletype, Uint8 team, Uint16 donttarget,
                            Uint16 oldtarget );

// object initialization
void  prime_names( void );
void  free_all_objects( void );

// Data
struct s_ego_mpd   * set_PMesh( struct s_ego_mpd * pmpd );
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

float get_mesh_level( struct s_ego_mpd * pmesh, float x, float y, bool_t waterwalk );

bool_t make_water( water_instance_t * pinst, water_data_t * pdata );
void read_wawalite( const char *modname );

bool_t game_choose_module( int imod, int seed );

process_instance_t * process_instance_init( process_instance_t * proc );
bool_t               process_instance_start( process_instance_t * proc );
bool_t               process_instance_kill( process_instance_t * proc );
bool_t               process_instance_validate( process_instance_t * proc );
bool_t               process_instance_terminate( process_instance_t * proc );
bool_t               process_instance_pause( process_instance_t * proc );
bool_t               process_instance_resume( process_instance_t * proc );
bool_t               process_instance_running( process_instance_t * proc );

ego_process_t      * ego_process_init ( ego_process_t  * eproc );
menu_process_t     * menu_process_init( menu_process_t * mproc );
game_process_t     * game_process_init( game_process_t * gproc );

void init_all_profiles();
void release_all_profiles();

void reset_players();

void expand_escape_codes( Uint16 ichr, struct s_script_state * pstate, char * src, char * src_end, char * dst, char * dst_end );
