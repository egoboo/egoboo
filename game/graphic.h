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

/// @file game/graphic.h

#include "game/egoboo_typedef.h"

#include <SDL.h>

#include "egolib/extensions/ogl_texture.h"
#include "egolib/file_formats/module_file.h"

#include "game/mesh.h"
#include "game/mad.h"
#include "game/camera_system.h"
#include "game/egoboo.h"

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

struct s_chr;
struct s_camera;
struct s_egoboo_config;
struct s_chr_instance;
struct s_oglx_texture_parameters;
struct s_egoboo_config;
struct s_Font;

//--------------------------------------------------------------------------------------------
// internal structs
//--------------------------------------------------------------------------------------------

struct s_renderlist;
typedef struct s_renderlist     renderlist_t;

struct s_renderlist_ary;
typedef struct s_renderlist_ary renderlist_ary_t;

struct s_renderlist_mgr;
typedef struct s_renderlist_mgr renderlist_mgr_t;

struct s_dolist_data;
typedef struct s_dolist_data dolist_data_t;

struct s_dolist;
typedef struct s_dolist      dolist_t;

struct s_dolist_ary;
typedef struct s_dolist_ary  dolist_ary_t;

struct s_dolist_mgr;
typedef struct s_dolist_mgr  dolist_mgr_t;

struct s_gfx_error_state;
typedef struct s_gfx_error_state gfx_error_state_t;

struct s_gfx_error_stack;
typedef struct s_gfx_error_stack gfx_error_stack_t;

struct s_obj_registry_entity;
typedef struct s_obj_registry_entity obj_registry_entity_t;

struct s_GLvertex;
typedef struct s_GLvertex GLvertex;

struct s_gfx_config;
typedef struct s_gfx_config gfx_config_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the default icon size in pixels
#define ICON_SIZE 32

/// the max number of do lists that can exist
#define MAX_DO_LISTS MAX_CAMERAS

/// the max number of render lists that can exist
#define MAX_RENDER_LISTS 4

/// max number of blips on the minimap
#define MAXBLIP        128                          ///<Max blips on the screen

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// special return values
enum e_gfx_rv
{
    gfx_error   = -1,
    gfx_fail    = ego_false,
    gfx_success = ego_true
};

// this typedef must be after the enum definition or gcc has a fit
typedef enum e_gfx_rv gfx_rv;

#define GFX_ERROR_MAX 256

struct s_gfx_error_state
{
    STRING file;
    STRING function;
    int    line;

    int    type;
    STRING string;
};

#define GFX_ERROR_STATE_INIT { "UNKNOWN", "UNKNOWN", -1, -1, "NONE" }

struct s_gfx_error_stack
{
    size_t count;
    gfx_error_state_t lst[GFX_ERROR_MAX];
};

#define GFX_ERROR_STACK_INIT { 0, { GFX_ERROR_STATE_INIT } }

egolib_rv           gfx_error_add( const char * file, const char * function, int line, int id, const char * sz );
gfx_error_state_t * gfx_error_pop( void );
void                gfx_error_clear( void );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define DOLIST_SIZE (MAX_CHR + MAX_PRT)

#define MAXMESHRENDER             1024                       ///< Max number of tiles to draw

#define MAPSIZE 96

#define TABX                            32// 16      ///< Size of little name tag on the bar
#define BARX                            112// 216         ///< Size of bar
#define BARY                            16// 8
#define NUMTICK                         10// 50          ///< Number of ticks per row
#define TICKX                           8// 4           ///< X size of each tick
#define MAXTICK                         (NUMTICK*10) ///< Max number of ticks to draw
#define XPTICK                          6.00f

#define NUMBAR                          6               ///< Number of status bars
#define NUMXPBAR                        2               ///< Number of xp bars

#define MAXLIGHTLEVEL                   16          ///< Number of premade light intensities
#define MAXSPEKLEVEL                    16          ///< Number of premade specularities
#define MAXLIGHTROTATION                256         ///< Number of premade light maps

#define DONTFLASH                       255
#define SEEKURSEAND                     31          ///< Blacking flash

#define SHADOWRAISE                       5

/// The supported colors of bars and blips
enum e_color
{
    COLOR_WHITE = 0,
    COLOR_RED,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_PURPLE,
    COLOR_MAX
};

// the map file only supports 256 texture images
#define MESH_IMG_COUNT 256

#define VALID_MESH_TX_RANGE(VAL) ( ((VAL)>=0) && ((VAL)<MESH_IMG_COUNT) )

//#define CALC_OFFSET_X(IMG) ((( (IMG) >> 0 ) & 7 ) / 8.0f)
//#define CALC_OFFSET_Y(IMG) ((( (IMG) >> 3 ) & 7 ) / 8.0f)

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// An element of the do-list, an all encompassing list of all objects to be drawn by the renderer
struct s_dolist_data
{
    float   dist;
    CHR_REF chr;
};

//--------------------------------------------------------------------------------------------

/// Structure for sorting both particles and characters based on their position from the camera
struct s_obj_registry_entity
{
    CHR_REF ichr;
    PRT_REF iprt;
    float   dist;
};

#define OBJ_REGISTRY_ENTITY_INIT { MAX_CHR, MAX_PRT, 0.0f }

obj_registry_entity_t * obj_registry_entity_init( obj_registry_entity_t * ptr );
int                     obj_registry_entity_cmp( const void * pleft, const void * pright );

//--------------------------------------------------------------------------------------------
// encapsulation of all graphics options
struct s_gfx_config
{
    GLuint shading;
    ego_bool refon;
    Uint8  reffadeor;
    ego_bool antialiasing;
    ego_bool dither;
    ego_bool perspective;
    ego_bool phongon;
    ego_bool shaon;
    ego_bool shasprite;

    ego_bool clearson;          ///< Do we clear every time?
    ego_bool draw_background;   ///< Do we draw the background image?
    ego_bool draw_overlay;      ///< Draw overlay?
    ego_bool draw_water_0;      ///< Do we draw water layer 1 (TX_WATER_LOW)
    ego_bool draw_water_1;      ///< Do we draw water layer 2 (TX_WATER_TOP)

    size_t dynalist_max;     ///< Max number of dynamic lights to draw
    ego_bool exploremode;       ///< fog of war mode for mesh display
    ego_bool usefaredge;        ///< Far edge maps? (Outdoor)

    // virtual window parameters
    float vw, vh;
    float vdw, vdh;
};

ego_bool gfx_config_init( gfx_config_t * pgfx );
ego_bool gfx_system_set_virtual_screen( gfx_config_t * pgfx );
ego_bool gfx_config_download_from_egoboo_config( gfx_config_t * pgfx, struct s_egoboo_config * pcfg );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern float time_render_scene_init;
extern float time_render_scene_mesh;
extern float time_render_scene_solid;
extern float time_render_scene_water;
extern float time_render_scene_trans;

extern float time_render_scene_init_renderlist_make;
extern float time_render_scene_init_dolist_make;
extern float time_render_scene_init_do_grid_dynalight;
extern float time_render_scene_init_light_fans;
extern float time_render_scene_init_update_all_chr_instance;
extern float time_render_scene_init_update_all_prt_instance;

extern float time_render_scene_mesh_dolist_sort;
extern float time_render_scene_mesh_ndr;
extern float time_render_scene_mesh_drf_back;
extern float time_render_scene_mesh_ref;
extern float time_render_scene_mesh_ref_chr;
extern float time_render_scene_mesh_drf_solid;
extern float time_render_scene_mesh_render_shadows;

extern Uint32          game_frame_all;             ///< The total number of frames drawn so far
extern Uint32          menu_frame_all;             ///< The total number of frames drawn so far

extern gfx_config_t gfx;

extern Uint8           mapon;
extern Uint8           mapvalid;
extern Uint8           youarehereon;

extern size_t          blip_count;
extern float           blip_x[MAXBLIP];
extern float           blip_y[MAXBLIP];
extern Uint8           blip_c[MAXBLIP];

extern int GFX_WIDTH;
extern int GFX_HEIGHT;

//extern Uint8           lightdirectionlookup[65536];                        ///< For lighting characters
//extern float           lighttable_local[MAXLIGHTROTATION][EGO_NORMAL_COUNT];
//extern float           lighttable_global[MAXLIGHTROTATION][EGO_NORMAL_COUNT];
extern float           indextoenvirox[EGO_NORMAL_COUNT];                    ///< Environment map
extern float           lighttoenviroy[256];                                ///< Environment map
//extern Uint32          lighttospek[MAXSPEKLEVEL][256];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Function prototypes

void   gfx_system_begin( void );
void   gfx_system_end( void );
int    gfx_system_init_OpenGL( void );
void   gfx_system_main( void );
void   gfx_system_reload_all_textures( void );
void   gfx_system_make_enviro( void );
void   gfx_system_init_all_graphics( void );
void   gfx_system_release_all_graphics( void );
void   gfx_system_delete_all_graphics( void );
void   gfx_system_load_assets( void );
void   gfx_system_load_basic_textures( void );

renderlist_mgr_t * gfx_system_get_renderlist_mgr( const struct s_camera * pcam );
dolist_mgr_t * gfx_system_get_dolist_mgr( void );

// the render engine callback
void   gfx_system_render_world( const struct s_camera * pcam, const int render_list_index, const int dolist_index );

void   gfx_request_clear_screen( void );
void   gfx_do_clear_screen( void );
ego_bool gfx_flip_pages_requested( void );
void   gfx_request_flip_pages( void );
void   gfx_do_flip_pages( void );

float draw_icon_texture( oglx_texture_t * ptex, float x, float y, Uint8 sparkle_color, Uint32 sparkle_timer, float size );
float draw_menu_icon( const TX_REF icontype, float x, float y, Uint8 sparkle, Uint32 delta_update, float size );
float draw_game_icon( const TX_REF icontype, float x, float y, Uint8 sparkle, Uint32 delta_update, float size );
void  draw_map_texture( float x, float y );
float draw_one_bar( Uint8 bartype, float x, float y, int ticks, int maxticks );
float draw_status( const CHR_REF character, float x, float y );
void  draw_one_character_icon( const CHR_REF item, float x, float y, ego_bool draw_ammo, Uint8 sparkle_override );
void  draw_cursor( void );
void  draw_blip( float sizeFactor, Uint8 color, float x, float y, ego_bool mini_map );

//void   make_lightdirectionlookup( void );

ego_bool grid_lighting_interpolate( const ego_mesh_t * pmesh, lighting_cache_t * dst, const fvec2_base_t pos );
float  grid_lighting_test( ego_mesh_t * pmesh, GLXvector3f pos, float * low_diff, float * hgh_diff );

void release_all_profile_textures( void );

gfx_rv gfx_load_blips( void );
gfx_rv gfx_load_bars( void );
gfx_rv gfx_load_map( void );
gfx_rv gfx_load_icons( void );

float  get_ambient_level( void );

void   draw_mouse_cursor( void );

gfx_rv chr_instance_flash( struct s_chr_instance * pinst, Uint8 value );

//void gfx_calc_rotmesh( void );

int            renderlist_mgr_get_free_idx( renderlist_mgr_t * ptr );
gfx_rv         renderlist_mgr_free_one( renderlist_mgr_t * ptr, int index );
renderlist_t * renderlist_mgr_get_ptr( renderlist_mgr_t * pmgr, int index );

int        dolist_mgr_get_free_idx( dolist_mgr_t * ptr );
gfx_rv     dolist_mgr_free_one( dolist_mgr_t * ptr, int index );
dolist_t * dolist_mgr_get_ptr( dolist_mgr_t * pmgr, int index );

gfx_rv renderlist_attach_mesh( renderlist_t * ptr, ego_mesh_t * pmesh );
gfx_rv renderlist_attach_camera( renderlist_t * ptr, const struct s_camera * pcam );

ego_bool oglx_texture_parameters_download_gfx( struct s_oglx_texture_parameters * ptex, struct s_egoboo_config * pcfg );

struct s_oglx_texture * gfx_get_mesh_tx_sml( int which );
struct s_oglx_texture * gfx_get_mesh_tx_big( int which );