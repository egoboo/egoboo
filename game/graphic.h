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
#include "extensions/ogl_texture.h"
#include "file_formats/module_file.h"
#include "mesh.h"
#include "mad.h"

#include "egoboo.h"

#include <SDL.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_chr;
struct s_camera;
struct s_egoboo_config;
struct s_chr_instance;
struct s_oglx_texture_parameters;
struct s_egoboo_config;
struct Font;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// special return values
enum e_gfx_rv
{
    gfx_error   = -1,
    gfx_fail    = bfalse,
    gfx_success = btrue
};

typedef enum e_gfx_rv gfx_rv;

#define GFX_ERROR_MAX 256

struct s_gfx_error_state
{
    const char * file;
    const char * function;
    int          line;

    int          type;
    const char * string;
};
typedef struct s_gfx_error_state gfx_error_state_t;

#define GFX_ERROR_STATE_INIT { "UNKNOWN", "UNKNOWN", -1, -1, "NONE" }

struct s_gfx_error_stack
{
    size_t count;
    gfx_error_state_t lst[GFX_ERROR_MAX];
};
typedef struct s_gfx_error_stack gfx_error_stack_t;

#define GFX_ERROR_STACK_INIT { 0, GFX_ERROR_STATE_INIT }

egoboo_rv           gfx_error_add( const char * file, const char * function, int line, int id, const char * sz );
gfx_error_state_t * gfx_error_pop();
void                gfx_error_clear();

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

extern int GFX_WIDTH;
extern int GFX_HEIGHT;

//#define GFX_WIDTH                       800         ///< 640
//#define GFX_HEIGHT                      600         ///< 480

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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// An element of the do-list, an all encompassing list of all objects to be drawn by the renderer
struct s_do_list_data
{
    float   dist;
    CHR_REF chr;
};
typedef struct s_do_list_data do_list_data_t;

//--------------------------------------------------------------------------------------------

/// Structure for sorting both particles and characters based on their position from the camera
struct s_obj_registry_entity
{
    CHR_REF ichr;
    PRT_REF iprt;
    float   dist;
};
typedef struct s_obj_registry_entity obj_registry_entity_t;

#define OBJ_REGISTRY_ENTITY_INIT { MAX_CHR, MAX_PRT, 0.0f }

int obj_registry_entity_cmp( const void * pleft, const void * pright );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// OPENGL VERTEX
struct s_GLvertex
{
    GLfloat pos[4];
    GLfloat nrm[3];
    GLfloat env[2];
    GLfloat tex[2];

    GLfloat col[4];      ///< generic per-vertex lighting
    GLint   color_dir;   ///< "optimized" per-vertex directional lighting
};
typedef struct s_GLvertex GLvertex;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Which tiles are to be drawn, arranged by MPDFX_* bits
struct s_renderlist
{
    ego_mpd_t * pmesh;

    int     all_count;                               ///< Number to render, total
    int     ref_count;                               ///< ..., is reflected in the floor
    int     sha_count;                               ///< ..., is not reflected in the floor
    int     drf_count;                               ///< ..., draws character reflections
    int     ndr_count;                               ///< ..., draws no character reflections
    int     wat_count;                               ///< ..., draws no character reflections

    Uint32  all[MAXMESHRENDER];                      ///< List of which to render, total

    Uint32  ref[MAXMESHRENDER];                      ///< ..., is reflected in the floor
    Uint32  sha[MAXMESHRENDER];                      ///< ..., is not reflected in the floor

    Uint32  drf[MAXMESHRENDER];                      ///< ..., draws character reflections
    Uint32  ndr[MAXMESHRENDER];                      ///< ..., draws no character reflections

    Uint32  wat[MAXMESHRENDER];                      ///< ..., draws a water tile
};
typedef struct s_renderlist renderlist_t;

#define RENDERLIST_INIT   \
    { \
        NULL,           /* pmesh */  \
        0,              /* all_count */  \
        0,              /* ref_count */  \
        0,              /* sha_count */  \
        0,              /* drf_count */  \
        0,              /* ndr_count */  \
        0,              /* wat_count */  \
        {INVALID_TILE}, /* all[MAXMESHRENDER] */  \
        {INVALID_TILE}, /* ref[MAXMESHRENDER] */  \
        {INVALID_TILE}, /* sha[MAXMESHRENDER] */  \
        {INVALID_TILE}, /* drf[MAXMESHRENDER] */  \
        {INVALID_TILE}, /* ndr[MAXMESHRENDER] */  \
        {INVALID_TILE}  /* wat[MAXMESHRENDER] */  \
    }

extern renderlist_t renderlist;

//--------------------------------------------------------------------------------------------
//extern Uint8           lightdirectionlookup[65536];                        ///< For lighting characters
//extern float           lighttable_local[MAXLIGHTROTATION][EGO_NORMAL_COUNT];
//extern float           lighttable_global[MAXLIGHTROTATION][EGO_NORMAL_COUNT];
extern float           indextoenvirox[EGO_NORMAL_COUNT];                    ///< Environment map
extern float           lighttoenviroy[256];                                ///< Environment map
//extern Uint32          lighttospek[MAXSPEKLEVEL][256];

//--------------------------------------------------------------------------------------------
// Display messages
extern int    msgtimechange;

/// A display messages
struct s_msg
{
    int             time;                            ///< The time for this message
    char            textdisplay[MESSAGESIZE];        ///< The displayed text
};
typedef struct s_msg msg_t;

DECLARE_STATIC_ARY_TYPE( DisplayMsgAry, msg_t, MAX_MESSAGE );

DECLARE_EXTERN_STATIC_ARY( DisplayMsgAry, DisplayMsg );

//--------------------------------------------------------------------------------------------
// camera optimization

// For figuring out what to draw
#define CAM_ROTMESH_TOPSIDE                  50
#define CAM_ROTMESH_BOTTOMSIDE               50
#define CAM_ROTMESH_UP                       30
#define CAM_ROTMESH_DOWN                     30

// The ones that get used
extern int rotmesh_topside;
extern int rotmesh_bottomside;
extern int rotmesh_up;
extern int rotmesh_down;

//--------------------------------------------------------------------------------------------
// encapsulation of all graphics options
struct s_gfx_config
{
    GLuint shading;
    bool_t refon;
    Uint8  reffadeor;
    bool_t antialiasing;
    bool_t dither;
    bool_t perspective;
    bool_t phongon;
    bool_t shaon;
    bool_t shasprite;

    bool_t clearson;          ///< Do we clear every time?
    bool_t draw_background;   ///< Do we draw the background image?
    bool_t draw_overlay;      ///< Draw overlay?
    bool_t draw_water_0;      ///< Do we draw water layer 1 (TX_WATER_LOW)
    bool_t draw_water_1;      ///< Do we draw water layer 2 (TX_WATER_TOP)

    int    dynalist_max;     ///< Max number of dynamic lights to draw
    bool_t exploremode;       ///< fog of war mode for mesh display
    bool_t usefaredge;        ///< Far edge maps? (Outdoor)

    // virtual window parameters
    float vw, vh;
    float vdw, vdh;
};
typedef struct s_gfx_config gfx_config_t;

extern gfx_config_t gfx;

bool_t gfx_config_init( gfx_config_t * pgfx );
bool_t gfx_set_virtual_screen( gfx_config_t * pgfx );
bool_t gfx_synch_config( gfx_config_t * pgfx, struct s_egoboo_config * pcfg );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Minimap stuff
#define MAXBLIP        128                          ///<Max blips on the screen
extern Uint8           mapon;
extern Uint8           mapvalid;
extern Uint8           youarehereon;

extern size_t          blip_count;
extern float           blip_x[MAXBLIP];
extern float           blip_y[MAXBLIP];
extern Uint8           blip_c[MAXBLIP];

#define BILLBOARD_COUNT     (2 * MAX_CHR)
#define INVALID_BILLBOARD   BILLBOARD_COUNT

enum e_bb_opt
{
    bb_opt_none          = EMPTY_BIT_FIELD,
    bb_opt_randomize_pos = ( 1 << 0 ),      // Randomize the position of the bb to witin 1 grid
    bb_opt_randomize_vel = ( 1 << 1 ),      // Randomize the velocity of the bb. Enough to move it by 2 tiles within its lifetime.
    bb_opt_fade          = ( 1 << 2 ),      // Make the billboard fade out
    bb_opt_burn          = ( 1 << 3 ),      // Make the tint fully saturate over time.
    bb_opt_all           = FULL_BIT_FIELD   //(size_t)(~0)   Enum doesn't support unsigned integers, size_t is also unsigned.
};

/// Description of a generic bilboarded object.
/// Any graphics that can be composited onto a SDL_surface can be used
struct s_billboard_data
{
    bool_t    valid;        ///< has the billboard data been initialized?

    Uint32    time;         ///< the time when the billboard will expire
    TX_REF    tex_ref;      ///< our texture index
    fvec3_t   pos;          ///< the position of the bottom-missle of the box

    CHR_REF   ichr;         ///< the character we are attached to

    GLXvector4f tint;       ///< a color to modulate the billboard's r,g,b, and a channels
    GLXvector4f tint_add;   ///< the change in tint per update

    GLXvector4f offset;     ///< an offset to the billboard's position in world coordinates
    GLXvector4f offset_add; ///<

    float       size;
    float       size_add;
};
typedef struct s_billboard_data billboard_data_t;

billboard_data_t * billboard_data_init( billboard_data_t * pbb );
bool_t             billboard_data_free( billboard_data_t * pbb );
bool_t             billboard_data_update( billboard_data_t * pbb );
bool_t             billboard_data_printf_ttf( billboard_data_t * pbb, struct Font *font, SDL_Color color, const char * format, ... );

DECLARE_LIST_EXTERN( billboard_data_t, BillboardList, BILLBOARD_COUNT );

void               BillboardList_init_all();
void               BillboardList_update_all();
void               BillboardList_free_all();
size_t             BillboardList_get_free( Uint32 lifetime_secs );
bool_t             BillboardList_free_one( size_t ibb );
billboard_data_t * BillboardList_get_ptr( const BBOARD_REF  ibb );

#define VALID_BILLBOARD_RANGE( IBB ) ( ( (IBB) >= 0 ) && ( (IBB) < BILLBOARD_COUNT ) )
#define VALID_BILLBOARD( IBB )       ( VALID_BILLBOARD_RANGE( IBB ) && BillboardList.lst[IBB].valid )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// some lines to be drawn in the display

#define LINE_COUNT 100
struct s_line_data
{
    fvec3_t   dst;
    fvec4_t   src, color;
    int time;
};
typedef struct s_line_data line_data_t;

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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Function prototypes

void   gfx_system_begin();
void   gfx_system_end();

int    ogl_init();
void   gfx_main();
void   gfx_begin_3d( struct s_camera * pcam );
void   gfx_end_3d();
bool_t gfx_synch_oglx_texture_parameters( struct s_oglx_texture_parameters * ptex, struct s_egoboo_config * pcfg );
void   gfx_reload_all_textures();

void   request_clear_screen();
void   do_clear_screen();
bool_t flip_pages_requested();
void   request_flip_pages();
void   do_flip_pages();

void  draw_one_icon( const TX_REF icontype, float x, float y, Uint8 sparkle );
void  draw_one_font( oglx_texture_t * ptex, int fonttype, float x, float y );
void  draw_map_texture( float x, float y );
float draw_one_bar( Uint8 bartype, float x, float y, int ticks, int maxticks );
float draw_string( float x, float y, const char *format, ... );
float draw_wrap_string( const char *szText, float x, float y, int maxx );
float draw_status( const CHR_REF character, float x, float y );
void  draw_text();
void  draw_one_character_icon( const CHR_REF item, float x, float y, bool_t draw_ammo );
void  draw_cursor();
void  draw_blip( float sizeFactor, Uint8 color, float x, float y, bool_t mini_map );
void  draw_all_lines( struct s_camera * pcam );

void      render_world( struct s_camera * pcam );
void      render_shadow( const CHR_REF character );
void      render_bad_shadow( const CHR_REF character );
gfx_rv render_scene( ego_mpd_t * pmesh, struct s_camera * pcam );
bool_t    render_oct_bb( oct_bb_t * bb, bool_t draw_square, bool_t draw_diamond );
bool_t    render_aabb( aabb_t * pbbox );
gfx_rv render_all_billboards( struct s_camera * pcam );

void   make_enviro();
void   clear_messages();
bool_t dump_screenshot();
void   make_lightdirectionlookup();

int  DisplayMsg_get_free();

int debug_printf( const char *format, ... );

gfx_rv renderlist_reset( renderlist_t * prlist );
gfx_rv renderlist_make( renderlist_t * prlist, ego_mpd_t * pmesh, struct s_camera * pcam );

bool_t grid_lighting_interpolate( ego_mpd_t * pmesh, lighting_cache_t * dst, float fx, float fy );
float  grid_lighting_test( ego_mpd_t * pmesh, GLXvector3f pos, float * low_diff, float * hgh_diff );

int  get_free_line();

void init_all_graphics();
void release_all_graphics();
void delete_all_graphics();

void release_all_profile_textures();

void   load_graphics();
bool_t load_blips();
void   load_bars();
void   load_map();
bool_t load_all_global_icons();
void   load_basic_textures( );

float  get_ambient_level();

gfx_rv flash_character( const CHR_REF character, Uint8 value );