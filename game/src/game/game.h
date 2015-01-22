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

/// @file game/game.h

#pragma once

#include "game/egoboo_typedef.h"
#include "game/input.h"

//--------------------------------------------------------------------------------------------
// forward declaration of external structs
//--------------------------------------------------------------------------------------------

// Forward declarations.
struct ego_mesh_t;
struct script_state_t;
struct mod_file_t;

struct wawalite_animtile_t;
struct wawalite_damagetile_t;
struct wawalite_weather_t;
struct wawalite_water_t;
struct wawalite_fog_t;

struct menu_process_t;

struct chr_t;
struct s_prt;
struct s_prt_bundle;

struct s_import_list;
class CameraSystem;
class AudioSystem;
class GameModule;

//--------------------------------------------------------------------------------------------
// forward declaration of internal structs
//--------------------------------------------------------------------------------------------

struct s_game_process;
typedef struct s_game_process game_process_t;

struct s_animtile_instance;
typedef struct s_animtile_instance animtile_instance_t;

struct s_damagetile_instance;
typedef struct s_damagetile_instance damagetile_instance_t;

struct s_weather_instance;
typedef struct s_weather_instance weather_instance_t;

struct s_water_layer_instance;
typedef struct s_water_layer_instance water_instance_layer_t;

struct s_water_instance;
typedef struct s_water_instance water_instance_t;

struct s_fog_instance;
typedef struct s_fog_instance fog_instance_t;

struct s_import_element;
typedef struct s_import_element import_element_t;

struct s_import_list;
typedef struct s_import_list import_list_t;

struct s_pit_info;
typedef struct s_pit_info pit_info_t;

struct s_status_list_element;
typedef struct s_status_list_element status_list_element_t;

struct s_status_list;
typedef struct s_status_list status_list_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define EXPKEEP 0.85f                                ///< Experience to keep when respawning

// tile
// #defne TILESOUNDTIME 16
#define TILE_REAFFIRM_AND  3

// water
#define MAXWATERLAYER 2                                    ///< Maximum water layers
#define MAXWATERFRAME (1 << 10)                            ///< Maximum number of wave frames
#define WATERFRAMEAND (MAXWATERFRAME-1)
#define WATERPOINTS 4                                      ///< Points in a water fan

// inventory
#define MAXINVENTORY        6                              ///< maximum number of objects in an inventory

// status list
#define MAX_STATUS          10                             ///< Maximum status displays

// end text
#define MAXENDTEXT 1024                                    ///< longest end text message

// imports
#define MAX_IMPORTS 16
#define MAX_IMPORT_OBJECTS     ( MAXINVENTORY + 2 )        ///< left hand + right hand + MAXINVENTORY
#define MAX_IMPORT_PER_PLAYER  ( 1 + MAX_IMPORT_OBJECTS )  ///< player + MAX_IMPORT_OBJECTS

//--------------------------------------------------------------------------------------------

/// The bitmasks for various in-game actions
enum e_latchbutton_bits
{
    LATCHBUTTON_LEFT      = ( 1 << 0 ),                      ///< Character button presses
    LATCHBUTTON_RIGHT     = ( 1 << 1 ),
    LATCHBUTTON_JUMP      = ( 1 << 2 ),
    LATCHBUTTON_ALTLEFT   = ( 1 << 3 ),                      ///< ( Alts are for grab/drop )
    LATCHBUTTON_ALTRIGHT  = ( 1 << 4 ),
    LATCHBUTTON_PACKLEFT  = ( 1 << 5 ),                      ///< ( Used by AI script for inventory cycle )
    LATCHBUTTON_PACKRIGHT = ( 1 << 6 ),                      ///< ( Used by AI script for inventory cycle )
    LATCHBUTTON_RESPAWN   = ( 1 << 7 )
};

//--------------------------------------------------------------------------------------------

/// The possible pre-defined orders
enum e_order
{
    ORDER_NONE  = 0,
    ORDER_ATTACK,
    ORDER_ASSIST,
    ORDER_STAND,
    ORDER_TERRAIN,
    ORDER_COUNT
};

//--------------------------------------------------------------------------------------------

/// The bitmasks used by the chr_check_target() function which is used in various character search
/// functions like chr_find_target() or find_object_in_passage()
enum e_targeting_bits
{
    TARGET_DEAD         = ( 1 << 0 ),       ///< Target dead stuff
    TARGET_ENEMIES      = ( 1 << 1 ),       ///< Target enemies
    TARGET_FRIENDS      = ( 1 << 2 ),       ///< Target friends
    TARGET_ITEMS        = ( 1 << 3 ),       ///< Target items
    TARGET_INVERTID     = ( 1 << 4 ),       ///< Target everything but the specified IDSZ
    TARGET_PLAYERS      = ( 1 << 5 ),       ///< Target only players
    TARGET_SKILL        = ( 1 << 6 ),       ///< Target needs the specified skill IDSZ
    TARGET_QUEST        = ( 1 << 7 ),       ///< Target needs the specified quest IDSZ
    TARGET_SELF         = ( 1 << 8 )        ///< Allow self as a target?
};

//--------------------------------------------------------------------------------------------
// Enums used with the check_time() function to determine if something time or date related is true
enum e_time
{
    SEASON_HALLOWEEN,       //Is it halloween?
    SEASON_CHRISTMAS,       //Is it christmas time?
    TIME_NIGHT,             //Is it night?
    TIME_DAY                //Is it day?
};

//--------------------------------------------------------------------------------------------

/// a process that controls a single game
struct s_game_process
{
    process_t base;

    double frameDuration;
    bool mod_paused, pause_key_ready;
    bool was_active;

    int    menu_depth;
    bool escape_requested, escape_latch;

    egolib_timer_t fps_timer;
    egolib_timer_t ups_timer;
};

game_process_t * game_process_init( game_process_t * gproc );
int              game_process_run( game_process_t * gproc, double frameDuration );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The actual state of the animated tiles in-game
struct s_animtile_instance
{
    int    update_and;            ///< how often to update the tile

    Uint16 frame_and;             ///< how many images within the "tile set"?
    Uint16 base_and;              ///< animated "tile set"
    Uint16 frame_add;             ///< which image within the tile set?
    Uint16 frame_add_old;         ///< the frame offset, the last time it was updated
    Uint32 frame_update_old;
};

bool upload_animtile_data( animtile_instance_t dst[], const struct s_wawalite_animtile * src, const size_t animtile_count );

//--------------------------------------------------------------------------------------------

/// The actual in-game state of the damage tiles
struct s_damagetile_instance
{
    IPair   amount;                    ///< Amount of damage
    int     damagetype;

    int    part_gpip;
    Uint32 partand;
    int    sound_index;

    //Sint16  sound_time;           // this is not used anywhere in the game
    //Uint16  min_distance;           // this is not used anywhere in the game
};

bool upload_damagetile_data( damagetile_instance_t * dst, const struct s_wawalite_damagetile * src );

//--------------------------------------------------------------------------------------------

/// The data descibing the weather state
struct s_weather_instance
{
    int     timer_reset;        ///< How long between each spawn?
    bool  over_water;         ///< Only spawn over water?
    int     part_gpip;           ///< Which particle to spawn?

    PLA_REF iplayer;
    int     time;                ///< 0 is no weather
};

bool upload_weather_data( weather_instance_t * dst, const struct s_wawalite_weather * src );

//--------------------------------------------------------------------------------------------

/// The data descibing the state of a water layer
struct s_water_layer_instance
{
    Uint16    frame;        ///< Frame
    Uint32    frame_add;    ///< Speed

    float     z;            ///< Base height of this water layer
    float     amp;          ///< Amplitude of waves

    fvec2_t   dist;         ///< some indication of "how far away" the layer is if it is an overlay

    fvec2_t   tx;           ///< Coordinates of texture

    float     light_dir;    ///< direct  reflectivity 0 - 1
    float     light_add;    ///< ambient reflectivity 0 - 1

    Uint8     alpha;        ///< layer transparency

    fvec2_t   tx_add;            ///< Texture movement
};

float water_instance_layer_get_level( water_instance_layer_t * ptr );

//--------------------------------------------------------------------------------------------

/// The data descibing the water state
struct s_water_instance
{
    float  surface_level;          ///< Surface level for water striders
    float  douse_level;            ///< Surface level for torches
    bool is_water;               ///< Is it water?  ( Or lava... )
    bool overlay_req;
    bool background_req;
    bool light;                  ///< Is it light ( default is alpha )

    float  foregroundrepeat;
    float  backgroundrepeat;

    Uint32 spek[256];              ///< Specular highlights

    int                    layer_count;
    water_instance_layer_t layer[MAXWATERLAYER];

    float  layer_z_add[MAXWATERLAYER][MAXWATERFRAME][WATERPOINTS];
};

float     water_instance_get_water_level( water_instance_t * pinst );
egolib_rv water_instance_move( water_instance_t * pwater );
bool    water_instance_make( water_instance_t * pinst, const wawalite_water_t * pdata );
bool    water_instance_set_douse_level( water_instance_t * pinst, float level );

bool    upload_water_data( water_instance_t * dst, const wawalite_water_t * src );

//--------------------------------------------------------------------------------------------

/// The in-game fog state
/// @warn Fog is currently not used
struct s_fog_instance
{
    bool  on;            ///< Do ground fog?
    float   top, bottom;
    Uint8   red, grn, blu;
    float   distance;
};

bool upload_fog_data( fog_instance_t * dst, const struct s_wawalite_fog * src );

//--------------------------------------------------------------------------------------------
// Imports
struct s_import_element
{
    STRING          srcDir;
    STRING          dstDir;
    STRING          name;

    size_t          local_player_num;   ///< Local player number (player 1, 2, 3 or 4)
    size_t          player;             ///< Which player is this?
    int             slot;               ///< which slot it it to be loaded into
};

bool import_element_init( import_element_t * );

//--------------------------------------------------------------------------------------------

struct s_import_list
{
    size_t           count;              ///< Number of imports
    import_element_t lst[MAX_IMPORTS];
};

#define IMPORT_LIST_INIT {0}

bool    import_list_init( import_list_t * imp_lst );
egolib_rv import_list_from_players( import_list_t * imp_lst );

//--------------------------------------------------------------------------------------------

/// Pitty stuff
struct s_pit_info
{
    bool     kill;          ///< Do they kill?
    bool     teleport;      ///< Do they teleport?
    fvec3_t    teleport_pos;
};

#define PIT_INFO_INIT { false /* kill */, false /* teleport */, fvec3_t::zero /* teleport_pos */ }

//--------------------------------------------------------------------------------------------

/// Status display info
struct s_status_list_element
{
    int camera_index;
    CHR_REF who;
};

#define STATUS_LIST_ELEMENT_INIT { -1 /* camera_ptr */, MAX_CHR /* who */ }

//--------------------------------------------------------------------------------------------

/// List of objects with status displays
struct s_status_list
{
    bool                on;
    size_t                count;
    status_list_element_t lst[MAX_STATUS];
};

#define STATUS_LIST_INIT { false /* on */, 0 /* count */, {STATUS_LIST_ELEMENT_INIT} /* lst */ }

bool status_list_update_cameras( status_list_t * plst );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// various global pointers
extern game_process_t  *GProc;
extern ego_mesh_t *PMesh;

//TODO: remove this global
extern CameraSystem _cameraSystem;
extern AudioSystem  _audioSystem;
extern GameModule *PMod;

// special terrain and wawalite-related data structs
extern animtile_instance_t animtile[2];
extern damagetile_instance_t damagetile;
extern weather_instance_t weather;
extern water_instance_t water;
extern fog_instance_t fog;

extern status_list_t StatusList;

// End text
extern char   endtext[MAXENDTEXT];     ///< The end-module text
extern size_t endtext_carat;

extern pit_info_t pits;

extern bool    overrideslots;          ///< Override existing slots?
extern FACING_T  glouseangle;            ///< global return value from prt_find_target() - actually still used

extern import_list_t ImportList;

// various clocks and timers
extern Sint32          clock_wld;             ///< The sync clock
extern Uint32          clock_enc_stat;        ///< For character stat regeneration
extern Uint32          clock_chr_stat;        ///< For enchant stat regeneration
extern Uint32          clock_pit;             ///< For pit kills
extern Uint32          update_wld;            ///< The number of times the game has been updated
extern Uint32          true_update;
extern Uint32          true_frame;
extern int             update_lag;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the hook for deinitializing an old module
void   game_quit_module();

/// the hook for exporting all the current players and reloading them
bool game_update_imports();
void   game_finish_module();
bool game_begin_module( const char * modname, Uint32 seed );

/// Exporting stuff
egolib_rv export_one_character( const CHR_REF character, const CHR_REF owner, int chr_obj_index, bool is_local );
egolib_rv export_all_players( bool require_local );

/// Messages
void show_stat( int statindex );
void show_armor( int statindex );
void show_full_status( int statindex );
void show_magic_status( int statindex );

/// End Text
void reset_end_text();

/// Particles
int     number_of_attached_particles( const CHR_REF character );
int     spawn_bump_particles( const CHR_REF character, const PRT_REF particle );
struct s_prt * place_particle_at_vertex( struct s_prt * pprt, const CHR_REF character, int vertex_offset );
void    disaffirm_attached_particles( const CHR_REF character );
int     reaffirm_attached_particles( const CHR_REF character );

/// Statlist
void statlist_add( const CHR_REF character );
/// @brief This function puts the character on top of the status list.
void statlist_move_to_top( const CHR_REF character );
void statlist_sort();

/// Player
void   set_one_player_latch( const PLA_REF player );
bool add_player( const CHR_REF character, const PLA_REF player, input_device_t *pdevice );

/// AI targeting
bool  chr_check_target( chr_t * psrc, const CHR_REF ichr_test, IDSZ idsz, const BIT_FIELD targeting_bits );
CHR_REF chr_find_target( chr_t * psrc, float max_dist, IDSZ idsz, const BIT_FIELD targeting_bits );
CHR_REF prt_find_target( fvec3_t& pos, FACING_T facing, const PIP_REF ipip, const TEAM_REF team, const CHR_REF donttarget, const CHR_REF oldtarget );

/// object initialization
void  free_all_objects();

/// Data
ego_mesh_t *set_PMesh( ego_mesh_t * pmpd );

float get_mesh_level( ego_mesh_t * pmesh, float x, float y, bool waterwalk );

bool game_choose_module( int imod, int seed );

int    game_do_menu( struct menu_process_t * mproc );

void expand_escape_codes( const CHR_REF ichr, script_state_t * pstate, char * src, char * src_end, char * dst, char * dst_end );

void attach_all_particles();

Uint8 get_alpha( int alpha, float seeinvis_mag );
Uint8 get_light( int alpha, float seedark_mag );

bool do_shop_drop( const CHR_REF idropper, const CHR_REF iitem );

bool do_shop_buy( const CHR_REF ipicker, const CHR_REF ichr );
bool do_shop_steal( const CHR_REF ithief, const CHR_REF iitem );
bool can_grab_item_in_shop( const CHR_REF ichr, const CHR_REF iitem );

bool get_chr_regeneration( chr_t * pchr, int *pliferegen, int * pmanaregen );

float get_chr_level( ego_mesh_t * pmesh, chr_t * pchr );

void disenchant_character( const CHR_REF ichr );

void cleanup_character_enchants( chr_t * pchr );

bool attach_one_particle( struct s_prt_bundle * pbdl_prt );

bool attach_chr_to_platform( chr_t * pchr, chr_t * pplat );
bool attach_prt_to_platform( struct s_prt * pprt, chr_t * pplat );

bool detach_character_from_platform( chr_t * pchr );
bool detach_particle_from_platform( struct s_prt * pprt );

egolib_rv game_copy_imports( struct s_import_list * imp_lst );

bool check_time( Uint32 check );
void   game_update_timers();

// wawalite functions
struct wawalite_data_t * read_wawalite_vfs();
bool write_wawalite_vfs( const wawalite_data_t * pdata );
bool wawalite_finalize( wawalite_data_t * pdata );
void   upload_wawalite();
