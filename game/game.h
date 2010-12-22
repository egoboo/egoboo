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
#include "egoboo_process.h"

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

struct s_menu_process;

struct s_chr;
struct s_prt;
struct s_prt_bundle;

struct s_Import_list;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define EXPKEEP 0.85f                                ///< Experience to keep when respawning

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

/// The bitmasks used by the check_target() function which is used in various character search
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
enum e_time {
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
//--------------------------------------------------------------------------------------------
//#define TILESOUNDTIME 16                ///< Not used anywhere
#define TILE_REAFFIRM_AND  3

#define MAXWATERLAYER 2                             ///< Maximum water layers
#define MAXWATERFRAME 512                           ///< Maximum number of wave frames
#define WATERFRAMEAND (MAXWATERFRAME-1)
#define WATERPOINTS 4                               ///< Points in a water fan

//Inventory stuff
#define MAXINVENTORY        6
#define MAXIMPORTOBJECTS    (MAXINVENTORY + 2)      ///< left hand + right hand + MAXINVENTORY
#define MAXIMPORTPERPLAYER  (1 + MAXIMPORTOBJECTS)  ///< player + MAXIMPORTOBJECTS

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
    int     damagetype;

    int    part_gpip;
    Uint32 partand;
    int    sound_index;

    //Sint16  sound_time;           // this is not used anywhere in the game
    //Uint16  min_distance;           // this is not used anywhere in the game
};
typedef struct s_damagetile_instance damagetile_instance_t;
extern damagetile_instance_t damagetile;

//--------------------------------------------------------------------------------------------

/// The data descibing the weather state
struct s_weather_instance
{
    int     timer_reset;        ///< How long between each spawn?
    bool_t  over_water;         ///< Only spawn over water?
    int     part_gpip;           ///< Which particle to spawn?

    PLA_REF iplayer;
    int     time;                ///< 0 is no weather
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

    fvec2_t   dist;         ///< some indication of "how far away" the layer is if it is an overlay

    fvec2_t   tx;           ///< Coordinates of texture

    float     light_dir;    ///< direct  reflectivity 0 - 1
    float     light_add;    ///< ambient reflectivity 0 - 1

    Uint8     alpha;        ///< layer transparency

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
    bool_t  exportreset;                ///< Allow to export when module is reset?
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
// Status displays

#define MAXSTAT             10                      ///< Maximum status displays

extern bool_t  StatusList_on;
extern int     StatusList_count;
extern CHR_REF StatusList[MAXSTAT];

//--------------------------------------------------------------------------------------------
// End text
#define MAXENDTEXT 1024

extern char   endtext[MAXENDTEXT];     ///< The end-module text
extern size_t endtext_carat;

//--------------------------------------------------------------------------------------------
// Imports

struct s_Import_element
{
    STRING    srcDir;
    STRING    dstDir;
    STRING    name;

    size_t    player;           ///< Which player is this?
    BIT_FIELD bits;             ///< Input bits for this player
    int       slot;             ///< which slot it it to be loaded into
};
typedef struct s_Import_element Import_element_t;

bool_t Import_element_init( Import_element_t * );

//--------------------------------------------------------------------------------------------
#define MAX_IMPORTS 16

struct s_Import_list
{
    size_t                count;              ///< Number of imports
    Import_element_t lst[MAX_IMPORTS];
};
typedef struct s_Import_list Import_list_t;

#define IMPORT_LIST_INIT {0}

bool_t    Import_list_init( Import_list_t * );
egoboo_rv Import_list_from_players( Import_list_t * imp_lst );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern bool_t    overrideslots;          ///< Override existing slots?

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

extern FACING_T  glouseangle;                                        ///< actually still used

extern bool_t activate_spawn_file_active;

extern Import_list_t ImportList;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the hook for deinitializing an old module
void   game_quit_module();

/// the hook for exporting all the current players and reloading them
bool_t game_update_imports();
void   game_finish_module();
bool_t game_begin_module( const char * modname, Uint32 seed );

/// Exporting stuff
egoboo_rv export_one_character( const CHR_REF character, const CHR_REF owner, int chr_obj_index, bool_t is_local );
egoboo_rv export_all_players( bool_t require_local );

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
void statlist_move_to_top( const CHR_REF character );
void statlist_sort();

/// Player
void   set_one_player_latch( const PLA_REF player );
bool_t add_player( const CHR_REF character, const PLA_REF player, Uint32 device );

/// AI targeting
CHR_REF chr_find_target( struct s_chr * psrc, float max_dist, IDSZ idsz, BIT_FIELD targeting_bits );
CHR_REF prt_find_target( float pos_x, float pos_y, float pos_z, FACING_T facing,
                         const PIP_REF particletype, const TEAM_REF team, const CHR_REF donttarget, const CHR_REF oldtarget );

/// object initialization
void  free_all_objects( void );

/// Data
struct s_ego_mpd * set_PMesh( struct s_ego_mpd * pmpd );
struct s_camera  * set_PCamera( struct s_camera * pcam );

bool_t upload_animtile_data( animtile_instance_t pinst[], struct s_wawalite_animtile * pdata, size_t animtile_count );
bool_t upload_damagetile_data( damagetile_instance_t * pinst, struct s_wawalite_damagetile * pdata );
bool_t upload_weather_data( weather_instance_t * pinst, struct s_wawalite_weather * pdata );
bool_t upload_water_data( water_instance_t * pinst, struct s_wawalite_water * pdata );
bool_t upload_fog_data( fog_instance_t * pinst, struct s_wawalite_fog * pdata );

float get_mesh_level( struct s_ego_mpd * pmesh, float x, float y, bool_t waterwalk );

bool_t make_water( water_instance_t * pinst, struct s_wawalite_water * pdata );

bool_t game_choose_module( int imod, int seed );

int                  game_do_menu( struct s_menu_process * mproc );
game_process_t     * game_process_init( game_process_t * gproc );

void expand_escape_codes( const CHR_REF ichr, struct s_script_state * pstate, char * src, char * src_end, char * dst, char * dst_end );

void upload_wawalite();

bool_t game_module_setup( game_module_t * pinst, struct s_mod_file * pdata, const char * loadname, Uint32 seed );
bool_t game_module_init( game_module_t * pinst );
bool_t game_module_reset( game_module_t * pinst, Uint32 seed );
bool_t game_module_start( game_module_t * pinst );
bool_t game_module_stop( game_module_t * pinst );

bool_t check_target( struct s_chr * psrc, const CHR_REF ichr_test, IDSZ idsz, BIT_FIELD targeting_bits );

void attach_all_particles();

struct s_wawalite_data * read_wawalite();
bool_t write_wawalite( const char *modname, struct s_wawalite_data * pdata );

Uint8 get_alpha( int alpha, float seeinvis_mag );
Uint8 get_light( int alpha, float seedark_mag );

bool_t do_shop_drop( const CHR_REF idropper, const CHR_REF iitem );

bool_t do_shop_buy( const CHR_REF ipicker, const CHR_REF ichr );
bool_t do_shop_steal( const CHR_REF ithief, const CHR_REF iitem );
bool_t can_grab_item_in_shop( const CHR_REF ichr, const CHR_REF iitem );

bool_t get_chr_regeneration( struct s_chr * pchr, int *pliferegen, int * pmanaregen );

float get_chr_level( struct s_ego_mpd * pmesh, struct s_chr * pchr );

int do_game_proc_run( game_process_t * gproc, double frameDuration );

egoboo_rv move_water( water_instance_t * pwater );

void disenchant_character( const CHR_REF ichr );

// manage the game's vfs mount points
bool_t game_setup_vfs_paths( const char * mod_path );

void cleanup_character_enchants( struct s_chr * pchr );

bool_t attach_one_particle( struct s_prt_bundle * pbdl_prt );

bool_t attach_chr_to_platform( struct s_chr * pchr, struct s_chr * pplat );
bool_t attach_prt_to_platform( struct s_prt * pprt, struct s_chr * pplat );

bool_t detach_character_from_platform( struct s_chr * pchr );
bool_t detach_particle_from_platform( struct s_prt * pprt );

egoboo_rv game_copy_imports( struct s_Import_list * imp_lst );

bool_t check_time( Uint32 check );
