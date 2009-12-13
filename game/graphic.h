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
#include "ogl_texture.h"
#include "module_file.h"
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
#define DOLIST_SIZE (MAX_CHR + TOTAL_MAX_PRT)

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

#define GFX_WIDTH                       800         ///< 640
#define GFX_HEIGHT                      600         ///< 480

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
    float  dist;
    Uint16 chr;
};
typedef struct s_do_list_data do_list_data_t;

//--------------------------------------------------------------------------------------------
/// Structure for sorting both particles and characters based on their position from the camera
struct s_obj_registry_entity
{
    Uint16 ichr, iprt;
    float  dist;
};
typedef struct s_obj_registry_entity obj_registry_entity_t;

int obj_registry_entity_cmp( const void * pleft, const void * pright );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// OPENGL VERTEX

typedef struct
{
    GLfloat pos[4];
    GLfloat nrm[3];
    GLfloat env[2];

    GLfloat tex[2];
    GLfloat col_dir[4];
    GLint   color_dir;   ///< the vertex-dependent, directional lighting

    GLfloat col[4];      ///< the total vertex-dependent lighting (ambient + directional)
} GLvertex;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Which tiles are to be drawn, arranged by MPDFX_* bits
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
extern Uint16          msgtimechange;

/// A display messages
struct s_msg
{
    Sint16          time;                            ///< The time for this message
    char            textdisplay[MESSAGESIZE];        ///< The displayed text
};
typedef struct s_msg msg_t;

DEFINE_STACK_EXTERN( msg_t, DisplayMsg, MAX_MESSAGE );

//--------------------------------------------------------------------------------------------
/// camera optimization

#define ROTMESHTOPSIDE                  55          ///< For figuring out what to draw
#define ROTMESHBOTTOMSIDE               65
#define ROTMESHUP                       40
#define ROTMESHDOWN                     60

extern int rotmeshtopside;                                 ///< The ones that get used
extern int rotmeshbottomside;
extern int rotmeshup;
extern int rotmeshdown;

//--------------------------------------------------------------------------------------------
/// encapsulation of all graphics options
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

    int    dyna_list_max;     ///< Max number of dynamic lights to draw
    bool_t exploremode;       ///< fog of war mode for mesh display
    bool_t usefaredge;        ///< Far edge maps? (Outdoor)

    // virtual window parameters
    float vw, vh;
    float vdw, vdh;
};
typedef struct s_gfx_config gfx_config_t;

extern gfx_config_t gfx;

bool_t gfx_config_init( gfx_config_t * pgfx );
bool_t gfx_synch_config( gfx_config_t * pgfx, struct s_egoboo_config * pcfg );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern obj_registry_entity_t dolist[DOLIST_SIZE];             ///< List of which characters to draw
extern size_t                dolist_count;                  ///< How many in the list

/// Minimap stuff
#define MAXBLIP        128                          ///<Max blips on the screen
extern Uint8           mapon;
extern Uint8           mapvalid;
extern Uint8           youarehereon;
extern Uint16          numblip;
extern Uint16          blipx[MAXBLIP];
extern Uint16          blipy[MAXBLIP];
extern Uint8           blipc[MAXBLIP];


#define BILLBOARD_COUNT     (2 * MAX_CHR)
#define INVALID_BILLBOARD   BILLBOARD_COUNT

/// Description of a generic bilboarded object.
/// Any graphics that can be composited onto a SDL_surface can be used
struct s_billboard_data
{
    bool_t    valid;        ///< has the billboard data been initialized?

    Uint32    time;         ///< the time when the billboard will expire
    int       tex_ref;      ///< our texture index
    fvec3_t   pos;          ///< the position of the bottom-missle of the box

    Uint16    ichr;         ///< the character we are attached to

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

DEFINE_LIST_EXTERN( billboard_data_t, BillboardList, BILLBOARD_COUNT );

void               BillboardList_init_all();
void               BillboardList_update_all();
void               BillboardList_free_all();
int                BillboardList_get_free( Uint32 lifetime_secs );
bool_t             BillboardList_free_one( int ibb );
billboard_data_t * BillboardList_get_ptr( int ibb );

#define VALID_BILLBOARD_RANGE( IBB ) ( ( (IBB) >= 0 ) && ( (IBB) < BILLBOARD_COUNT ) )
#define VALID_BILLBOARD( IBB )       ( VALID_BILLBOARD_RANGE( IBB ) && BillboardList.lst[IBB].valid )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// some lines to be drawn in the display
#define LINE_COUNT 100
struct s_line_data
{
    fvec3_t   dst;
    fvec4_t   src, color;
    int time;
};
typedef struct s_line_data line_data_t;

extern line_data_t line_list[LINE_COUNT];

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

void   gfx_init();
int    ogl_init();
void   gfx_main();
void   gfx_begin_3d( struct s_camera * pcam );
void   gfx_end_3d();
bool_t gfx_synch_oglx_texture_parameters( struct s_oglx_texture_parameters * ptex, struct s_egoboo_config * pcfg );

void   request_clear_screen();
void   do_clear_screen();
bool_t flip_pages_requested();
void   request_flip_pages();
void   do_flip_pages();

void   dolist_sort( struct s_camera * pcam, bool_t do_reflect );
void   dolist_make( ego_mpd_t * pmesh );
bool_t dolist_add_chr( ego_mpd_t * pmesh, Uint16 cnt );
bool_t dolist_add_prt( ego_mpd_t * pmesh, Uint16 cnt );

void draw_one_icon( int icontype, int x, int y, Uint8 sparkle );
void draw_one_font( int fonttype, int x, int y );
void draw_map_texture( int x, int y );
int  draw_one_bar( Uint8 bartype, int x, int y, int ticks, int maxticks );
int  draw_string( int x, int y, const char *format, ... );
int  draw_wrap_string( const char *szText, int x, int y, int maxx );
int  draw_status( Uint16 character, int x, int y );
void draw_text();
void draw_one_character_icon( Uint16 item, int x, int y, bool_t draw_ammo );
void draw_cursor();
void draw_blip( float sizeFactor, Uint8 color, int x, int y );
void draw_all_lines( struct s_camera * pcam );

void   render_world( struct s_camera * pcam );
void   render_shadow( Uint16 character );
void   render_bad_shadow( Uint16 character );
void   render_scene( ego_mpd_t * pmesh, struct s_camera * pcam );
bool_t render_oct_bb( oct_bb_t * bb, bool_t draw_square, bool_t draw_diamond );
bool_t render_aabb( aabb_t * pbbox );
void   render_all_billboards( struct s_camera * pcam );

void   make_enviro();
void   clear_messages();
bool_t dump_screenshot();
void   make_lightdirectionlookup();

int  DisplayMsg_get_free();

int debug_printf( const char *format, ... );

void renderlist_reset();
void renderlist_make( ego_mpd_t * pmesh, struct s_camera * pcam );

bool_t grid_lighting_interpolate( ego_mpd_t * pmesh, lighting_cache_t * dst, float fx, float fy );
bool_t grid_lighting_test( ego_mpd_t * pmesh, GLXvector3f pos, float * low_diff, float * hgh_diff );

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