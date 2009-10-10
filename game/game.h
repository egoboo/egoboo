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
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_ego_mpd;
struct s_camera;
struct s_script_state;
struct s_mod_file;

struct s_wawalite_animtile;
struct s_wawalite_damagetile;
struct s_wawalite_weather;
struct s_wawalite_water;
struct s_wawalite_fog;

struct s_chr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define EXPKEEP 0.85f                                ///< Experience to keep when respawning

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// The various states that a process can occupy
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

/// grab a pointer to the process_t of any object that "inherits" this type
#define PROC_PBASE(PTR) (&( (PTR)->base ))

/// A rudimantary implementation of "non-preemptive multitasking" in egoboo.
/// @details All other process types "inherit" from this one
struct s_process_instance
{
    bool_t          valid;
    bool_t          paused;
    bool_t          killme;
    bool_t          terminated;
    process_state_t state;
    double          dtime;
};
typedef struct s_process_instance process_t;

//--------------------------------------------------------------------------------------------
/// a process that controls the master loop of the program
struct s_ego_process
{
    process_t base;

    double frameDuration;
    int    menuResult;

    bool_t was_active;
    bool_t escape_requested, escape_latch;

    int    ticks_next, ticks_now;

    char * argv0;
};
typedef struct s_ego_process ego_process_t;

extern ego_process_t * EProc;

//--------------------------------------------------------------------------------------------
/// a process that controls a single game
struct s_game_process
{
    process_t base;

    double frameDuration;
    bool_t mod_paused, pause_key_ready;
    bool_t was_active;

    int    menu_depth;
    bool_t escape_requested, escape_latch;

    int    fps_ticks_next, fps_ticks_now;
    int    ups_ticks_next, ups_ticks_now;

};
typedef struct s_game_process game_process_t;

extern game_process_t * GProc;

//--------------------------------------------------------------------------------------------
/// a process that controls the menu system
struct s_menu_process
{
    process_t base;

    bool_t was_active;
    bool_t escape_requested, escape_latch;

    int    ticks_next, ticks_now;
};
typedef struct s_menu_process menu_process_t;

extern menu_process_t * MProc;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define TILESOUNDTIME 16
#define TILEREAFFIRMAND  3

#define MAXWATERLAYER 2                             ///< Maximum water layers
#define MAXWATERFRAME 512                           ///< Maximum number of wave frames
#define WATERFRAMEAND (MAXWATERFRAME-1)
#define WATERPOINTS 4                               ///< Points in a water fan

/// The bitmasks for various in-game actions
enum e_latchbutton_bits
{
    LATCHBUTTON_LEFT      = ( 1 << 0 ),                      ///< Character button presses
    LATCHBUTTON_RIGHT     = ( 1 << 1 ),
    LATCHBUTTON_JUMP      = ( 1 << 2 ),
    LATCHBUTTON_ALTLEFT   = ( 1 << 3 ),                      ///< ( Alts are for grab/drop )
    LATCHBUTTON_ALTRIGHT  = ( 1 << 4 ),
    LATCHBUTTON_PACKLEFT  = ( 1 << 5 ),                     ///< ( Packs are for inventory cycle )
    LATCHBUTTON_PACKRIGHT = ( 1 << 6 ),
    LATCHBUTTON_RESPAWN   = ( 1 << 7 )
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// The actual state of the animated tiles in-game
struct s_animtile_instance
{
    int    update_and;             ///< New tile every 7 frames
    Uint16 frame_and;
    Uint16 base_and;
    Uint16 frame_add;
};
typedef struct s_animtile_instance animtile_instance_t;

extern Uint32              animtile_update_and;
extern animtile_instance_t animtile[2];

//--------------------------------------------------------------------------------------------
/// The actual in-game state of the damage tiles
struct s_damagetile_instance
{
    IPair   amount;                    ///< Amount of damage
    int    type;

    int    parttype;
    Uint32 partand;
    int    sound;

    Sint16  sound_time;
    Uint16  min_distance;
};
typedef struct s_damagetile_instance damagetile_instance_t;
extern damagetile_instance_t damagetile;

//--------------------------------------------------------------------------------------------
/// The data descibing the weather state
struct s_weather_instance
{
    int   timer_reset;
    bool_t  over_water;

    Uint16  iplayer;
    int   time;                ///< 0 is no weather
};
typedef struct s_weather_instance weather_instance_t;
extern weather_instance_t weather;

//--------------------------------------------------------------------------------------------
/// The data descibing the state of a water layer
struct s_water_layer_instance
{
    Uint16    frame;        ///< Frame
    Uint32    frame_add;      ///< Speed

    float     z;            ///< Base height of water
    float     amp;            ///< Amplitude of waves

    fvec2_t   dist;

    fvec2_t   tx;           ///< Coordinates of texture

    float     light_dir;    ///< direct  reflectivity 0 - 1
    float     light_add;    ///< ambient reflectivity 0 - 1
    Uint8     alpha;        ///< Transparency

    fvec2_t   tx_add;            ///< Texture movement
};
typedef struct s_water_layer_instance water_instance_layer_t;

/// The data descibing the water state
struct s_water_instance
{
    float  surface_level;          ///< Surface level for water striders
    float  douse_level;            ///< Surface level for torches
    bool_t is_water;         ///< Is it water?  ( Or lava... )
    bool_t overlay_req;
    bool_t background_req;
    bool_t light;            ///< Is it light ( default is alpha )

    float  foregroundrepeat;
    float  backgroundrepeat;

    Uint32 spek[256];              ///< Specular highlights

    int                    layer_count;
    water_instance_layer_t layer[MAXWATERLAYER];

    float  layer_z_add[MAXWATERLAYER][MAXWATERFRAME][WATERPOINTS];
};

typedef struct s_water_instance water_instance_t;
extern water_instance_t water;

//--------------------------------------------------------------------------------------------
/// The in-game fog state
/// @warn Fog is currently not used
struct s_fog_instance
{
    bool_t  on;            ///< Do ground fog?
    float   top, bottom;
    Uint8   red, grn, blu;
    float   distance;
};
typedef struct s_fog_instance fog_instance_t;
extern fog_instance_t fog;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// the module data that the game needs
struct s_game_module
{
    Uint8   importamount;               ///< Number of imports for this module
    bool_t  exportvalid;                ///< Can it export?
    Uint8   playeramount;               ///< How many players?
    bool_t  importvalid;                ///< Can it import?
    bool_t  respawnvalid;               ///< Can players respawn with Spacebar?
    bool_t  respawnanytime;             ///< True if it's a small level...
    STRING  loadname;                     ///< Module load names

    bool_t  active;                     ///< Is the control loop still going?
    bool_t  beat;                       ///< Show Module Ended text?
    Uint32  seed;                       ///< The module seed
    Uint32  randsave;
};
typedef struct s_game_module game_module_t;

//--------------------------------------------------------------------------------------------
/// Status displays

#define MAXSTAT             16                      ///< Maximum status displays

extern bool_t StatusList_on;
extern int    StatusList_count;
extern Uint16 StatusList[MAXSTAT];

//--------------------------------------------------------------------------------------------
/// End text
#define MAXENDTEXT 1024

extern char   endtext[MAXENDTEXT];     ///< The end-module text
extern size_t endtext_carat;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern bool_t    overrideslots;         ///< Override existing slots?
extern bool_t    screenshotkeyready;    ///< Ready to take screenshot?

extern struct s_ego_mpd         * PMesh;
extern struct s_camera          * PCamera;
extern struct s_game_module * PMod;

/// Pitty stuff
struct s_pit_info
{
    bool_t     kill;          ///< Do they kill?
    bool_t     teleport;      ///< Do they teleport?
    fvec3_t    teleport_pos;
};
typedef struct s_pit_info pit_info_t;

extern pit_info_t pits;

extern Uint16  glouseangle;                                        ///< actually still used

/// Sense enemies
extern Uint8  local_senseenemiesTeam;
extern IDSZ   local_senseenemiesID;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the hook for deinitializing an old module
void   game_quit_module();

/// the hook for exporting all the current players and reloading them
bool_t game_update_imports();
void   game_finish_module();
bool_t game_begin_module( const char * modname, Uint32 seed );

/// Exporting stuff
void export_one_character( Uint16 character, Uint16 owner, int number, bool_t is_local );
void export_all_players( bool_t require_local );

/// Messages
void show_stat( Uint16 statindex );
void show_armor( Uint16 statindex );
void show_full_status( Uint16 statindex );
void show_magic_status( Uint16 statindex );

/// End Text
void reset_end_text();

/// Particles
Uint16 number_of_attached_particles( Uint16 character );
int    spawn_bump_particles( Uint16 character, Uint16 particle );
void   attach_particle_to_character( Uint16 particle, Uint16 character, int vertex_offset );
void   disaffirm_attached_particles( Uint16 character );
void   reaffirm_attached_particles( Uint16 character );
Uint16 number_of_attached_particles( Uint16 character );

/// Statlist
void statlist_add( Uint16 character );
void statlist_move_to_top( Uint16 character );
void statlist_sort();

/// Math
Uint16 terp_dir( Uint16 majordir, Uint16 minordir );
Uint16 terp_dir_fast( Uint16 majordir, Uint16 minordir );
void   getadd( int min, int value, int max, int* valuetoadd );
void   fgetadd( float min, float value, float max, float* valuetoadd );

/// Player
void   set_one_player_latch( Uint16 player );
bool_t add_player( Uint16 character, Uint16 player, Uint32 device );

/// AI targeting
Uint16 chr_find_target( struct s_chr * psrc, float max_dist2, TARGET_TYPE target_type, bool_t target_items, 
					   bool_t target_dead, IDSZ target_idsz, bool_t exclude_idsz, bool_t target_players );
Uint16 prt_find_target( float pos_x, float pos_y, float pos_z, Uint16 facing,
                            Uint16 particletype, Uint8 team, Uint16 donttarget,
                            Uint16 oldtarget );

/// object initialization
void  free_all_objects( void );

/// Data
struct s_ego_mpd   * set_PMesh( struct s_ego_mpd * pmpd );
struct s_camera * set_PCamera( struct s_camera * pcam );

bool_t upload_animtile_data( animtile_instance_t pinst[], struct s_wawalite_animtile * pdata, size_t animtile_count );
bool_t upload_damagetile_data( damagetile_instance_t * pinst, struct s_wawalite_damagetile * pdata );
bool_t upload_weather_data( weather_instance_t * pinst, struct s_wawalite_weather * pdata );
bool_t upload_water_data( water_instance_t * pinst, struct s_wawalite_water * pdata );
bool_t upload_fog_data( fog_instance_t * pinst, struct s_wawalite_fog * pdata );

float get_mesh_level( struct s_ego_mpd * pmesh, float x, float y, bool_t waterwalk );

bool_t make_water( water_instance_t * pinst, struct s_wawalite_water * pdata );

bool_t game_choose_module( int imod, int seed );

process_t * process_init( process_t * proc );
bool_t      process_start( process_t * proc );
bool_t      process_kill( process_t * proc );
bool_t      process_validate( process_t * proc );
bool_t      process_terminate( process_t * proc );
bool_t      process_pause( process_t * proc );
bool_t      process_resume( process_t * proc );
bool_t      process_running( process_t * proc );

ego_process_t      * ego_process_init ( ego_process_t  * eproc, int argc, char **argv );
menu_process_t     * menu_process_init( menu_process_t * mproc );
game_process_t     * game_process_init( game_process_t * gproc );

void expand_escape_codes( Uint16 ichr, struct s_script_state * pstate, char * src, char * src_end, char * dst, char * dst_end );

void upload_wawalite();

void ego_init_SDL_base();

bool_t game_module_setup( game_module_t * pinst, struct s_mod_file * pdata, const char * loadname, Uint32 seed );
bool_t game_module_init ( game_module_t * pinst );
bool_t game_module_reset( game_module_t * pinst, Uint32 seed );
bool_t game_module_start( game_module_t * pinst );
bool_t game_module_stop ( game_module_t * pinst );

bool_t check_target( struct s_chr * psrc, Uint16 ichr_test, TARGET_TYPE target_type, bool_t target_items, bool_t target_dead, IDSZ target_idsz, bool_t exclude_idsz, bool_t target_players );

void do_weather_spawn_particles();
void attach_particles();