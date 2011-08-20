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

/// @file graphic.c
/// @brief Simple Egoboo renderer
/// @details All sorts of stuff related to drawing the game

#include <assert.h>

#include <SDL/SDL_image.h>

#include <egolib/log.h>
#include <egolib/clock.h>
#include <egolib/throttle.h>
#include <egolib/font_bmp.h>
#include <egolib/font_ttf.h>
#include <egolib/vfs.h>
#include <egolib/egoboo_setup.h>
#include <egolib/strutil.h>
#include <egolib/fileutil.h>
#include <egolib/frustum.h>
#include <egolib/extensions/SDL_extensions.h>
#include <egolib/extensions/SDL_GL_extensions.h>
#include <egolib/file_formats/id_md2.h>

#include <egolib/console.h>
#if defined(USE_LUA_CONSOLE)
#    include <egolib/lua_console.h>
#endif

#include "graphic.h"
#include "graphic_prt.h"
#include "graphic_mad.h"
#include "graphic_fan.h"
#include "graphic_billboard.h"
#include "graphic_texture.h"

#include "network_server.h"
#include "mad.h"
#include "obj_BSP.h"
#include "mpd_BSP.h"
#include "player.h"
#include "collision.h"
#include "script.h"
#include "camera_system.h"
#include "input.h"
#include "passage.h"
#include "menu.h"
#include "script_compile.h"
#include "game.h"
#include "ui.h"
#include "lighting.h"
#include "egoboo.h"

#include "char.inl"
#include "particle.inl"
#include "enchant.inl"
#include "profile.inl"
#include "mesh.inl"

//--------------------------------------------------------------------------------------------
// internal structs
//--------------------------------------------------------------------------------------------

struct s_dynalight_registry;
typedef struct s_dynalight_registry dynalight_registry_t;

struct s_dynalist;
typedef struct s_dynalist dynalist_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define SPARKLE_SIZE ICON_SIZE
#define SPARKLE_AND  (SPARKLE_SIZE - 1)

#define BLIPSIZE 6

// camera frustum optimization
//#define ROTMESH_TOPSIDE                  50
//#define ROTMESH_BOTTOMSIDE               50
//#define ROTMESH_UP                       30
//#define ROTMESH_DOWN                     30

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Structure for keeping track of which dynalights are visible
struct s_dynalight_registry
{
    int         reference;
    ego_frect_t bound;
};

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

static renderlist_t * renderlist_init( renderlist_t * prlist, ego_mpd_t * pmesh );
static gfx_rv         renderlist_reset( renderlist_t * prlist );
static gfx_rv         renderlist_insert( renderlist_t * prlist, const ego_grid_info_t * pgrid, const Uint32 index );
static ego_mpd_t *    renderlist_get_pmesh( const renderlist_t * ptr );
static gfx_rv         renderlist_add_colst( renderlist_t * prlist, const BSP_leaf_pary_t * pcolst );

//--------------------------------------------------------------------------------------------
// the renderlist manager
//--------------------------------------------------------------------------------------------

struct s_renderlist_ary
{
    bool_t       started;

    size_t       free_count;
    int          free_lst[MAX_RENDER_LISTS];

    renderlist_t lst[MAX_RENDER_LISTS];
};

#define RENDERLIST_ARY_INIT_VALS                                 \
    {                                                                \
        bfalse,         /* bool_t       started                   */ \
        0,              /* size_t       free_count                */ \
        /*nothing */    /* int          free_lst[MAX_RENDER_LISTS]*/ \
        /*nothing */    /* renderlist_t lst[MAX_RENDER_LISTS]     */ \
    }

static renderlist_ary_t * renderlist_ary_begin( renderlist_ary_t * ptr );
static renderlist_ary_t * renderlist_ary_end( renderlist_ary_t * ptr );

//--------------------------------------------------------------------------------------------
struct s_renderlist_mgr
{
    bool_t           started;
    renderlist_ary_t ary;
};

#define RENDERLIST_MGR_INIT_VALS                            \
    {                                                           \
        bfalse,                  /* bool_t           started */ \
        RENDERLIST_ARY_INIT_VALS /* renderlist_ary_t ary     */ \
    }

static gfx_rv         renderlist_mgr_begin( renderlist_mgr_t * pmgr );
static gfx_rv         renderlist_mgr_end( renderlist_mgr_t * pmgr );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_dolist
{
    int                   index;                ///< A "name" for the dolist
    size_t                count;                ///< How many in the list
    obj_registry_entity_t lst[DOLIST_SIZE];     ///< List of which objects to draw
};

#define DOLIST_INIT { 0, OBJ_REGISTRY_ENTITY_INIT }

static dolist_t * dolist_init( dolist_t * pdolist, const int index );
static gfx_rv     dolist_reset( dolist_t * pdolist, const int index );
static gfx_rv     dolist_sort( dolist_t * pdolist, const camera_t * pcam, const bool_t do_reflect );
static gfx_rv     dolist_test_chr( dolist_t * pdolist, const chr_t * pchr );
static gfx_rv     dolist_add_chr_raw( dolist_t * pdolist, chr_t * pchr );
static gfx_rv     dolist_test_prt( dolist_t * pdolist, const prt_t * pprt );
static gfx_rv     dolist_add_prt_raw( dolist_t * pdolist, prt_t * pprt );
static gfx_rv     dolist_add_colst( dolist_t * pdlist, const BSP_leaf_pary_t * pcolst );

//--------------------------------------------------------------------------------------------
// the dolist manager
//--------------------------------------------------------------------------------------------

struct s_dolist_ary
{
    bool_t       started;

    size_t       free_count;
    int          free_lst[MAX_DO_LISTS];

    dolist_t lst[MAX_DO_LISTS];
};

#define DOLIST_ARY_INIT_VALS                              \
    {                                                         \
        bfalse,         /* bool_t   started                */ \
        0,              /* size_t   free_count             */ \
        /*nothing */    /* int      free_lst[MAX_DO_LISTS] */ \
        /*nothing */    /* dolist_t lst[MAX_DO_LISTS]      */ \
    }

static dolist_ary_t * dolist_ary_begin( dolist_ary_t * ptr );
static dolist_ary_t * dolist_ary_end( dolist_ary_t * ptr );

//--------------------------------------------------------------------------------------------
struct s_dolist_mgr
{
    bool_t       started;
    dolist_ary_t ary;
};

#define DOLIST_MGR_INIT_VALS                        \
    {                                                   \
        bfalse,              /* bool_t       started */ \
        DOLIST_ARY_INIT_VALS /* dolist_ary_t ary     */ \
    }

static gfx_rv     dolist_mgr_begin( dolist_mgr_t * pmgr );
static gfx_rv     dolist_mgr_end( dolist_mgr_t * pmgr );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The active dynamic lights
struct s_dynalist
{
    int         frame;                    ///< the last frame in shich the list was updated

    size_t      count;                    ///< the count
    dynalight_t lst[TOTAL_MAX_DYNA];      ///< the list
};

#define DYNALIST_INIT { -1 /* frame */, 0 /* count */ }

static gfx_rv dynalist_init( dynalist_t * pdylist );
static gfx_rv do_grid_lighting( renderlist_t * prlist, dynalist_t * pdylist, const camera_t * pcam );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

PROFILE_DECLARE( render_scene_init );
PROFILE_DECLARE( render_scene_mesh );
PROFILE_DECLARE( render_scene_solid );
PROFILE_DECLARE( render_scene_water );
PROFILE_DECLARE( render_scene_trans );

PROFILE_DECLARE( gfx_make_renderlist );
PROFILE_DECLARE( gfx_make_dolist );
PROFILE_DECLARE( do_grid_lighting );
PROFILE_DECLARE( light_fans );
PROFILE_DECLARE( gfx_update_all_chr_instance );
PROFILE_DECLARE( update_all_prt_instance );

PROFILE_DECLARE( render_scene_mesh_dolist_sort );
PROFILE_DECLARE( render_scene_mesh_ndr );
PROFILE_DECLARE( render_scene_mesh_drf_back );
PROFILE_DECLARE( render_scene_mesh_ref );
PROFILE_DECLARE( render_scene_mesh_ref_chr );
PROFILE_DECLARE( render_scene_mesh_drf_solid );
PROFILE_DECLARE( render_scene_mesh_render_shadows );

float time_draw_scene       = 0.0f;
float time_render_scene_init  = 0.0f;
float time_render_scene_mesh  = 0.0f;
float time_render_scene_solid = 0.0f;
float time_render_scene_water = 0.0f;
float time_render_scene_trans = 0.0f;

float time_render_scene_init_renderlist_make         = 0.0f;
float time_render_scene_init_dolist_make             = 0.0f;
float time_render_scene_init_do_grid_dynalight       = 0.0f;
float time_render_scene_init_light_fans              = 0.0f;
float time_render_scene_init_update_all_chr_instance = 0.0f;
float time_render_scene_init_update_all_prt_instance = 0.0f;

float time_render_scene_mesh_dolist_sort    = 0.0f;
float time_render_scene_mesh_ndr            = 0.0f;
float time_render_scene_mesh_drf_back       = 0.0f;
float time_render_scene_mesh_ref            = 0.0f;
float time_render_scene_mesh_ref_chr        = 0.0f;
float time_render_scene_mesh_drf_solid      = 0.0f;
float time_render_scene_mesh_render_shadows = 0.0f;

Uint32          game_frame_all   = 0;             ///< The total number of frames drawn so far
Uint32          menu_frame_all   = 0;             ///< The total number of frames drawn so far

int maxmessage = MAX_MESSAGE;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int GFX_WIDTH  = 800;
int GFX_HEIGHT = 600;

gfx_config_t     gfx;

float            indextoenvirox[EGO_NORMAL_COUNT];
float            lighttoenviroy[256];

Uint8   mapon         = bfalse;
Uint8   mapvalid      = bfalse;
Uint8   youarehereon  = bfalse;

size_t  blip_count    = 0;
float   blip_x[MAXBLIP];
float   blip_y[MAXBLIP];
Uint8   blip_c[MAXBLIP];

int     msgtimechange = 0;

INSTANTIATE_STATIC_ARY( DisplayMsgAry, DisplayMsg );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static gfx_error_stack_t gfx_error_stack = GFX_ERROR_STACK_INIT;

static line_data_t  line_list[LINE_COUNT];
static point_data_t point_list[POINT_COUNT];

static SDLX_video_parameters_t sdl_vparam;
static oglx_video_parameters_t ogl_vparam;

static SDL_bool _sdl_initialized_graphics = SDL_FALSE;
static bool_t   _ogl_initialized          = bfalse;

static float sinlut[MAXLIGHTROTATION];
static float coslut[MAXLIGHTROTATION];

// Interface stuff
static irect_t tabrect[NUMBAR];            // The tab rectangles
static irect_t barrect[NUMBAR];            // The bar rectangles
static irect_t bliprect[COLOR_MAX];        // The blip rectangles
static irect_t maprect;                    // The map rectangle

static bool_t  gfx_page_flip_requested  = bfalse;
static bool_t  gfx_page_clear_requested = btrue;

static float dynalight_keep = 0.9f;

egolib_timer_t gfx_update_timer;

static egolib_throttle_t gfx_throttle = EGOLIB_THROTTLE_INIT;

static dynalist_t _dynalist = DYNALIST_INIT;

static renderlist_mgr_t _renderlist_mgr_data = RENDERLIST_MGR_INIT_VALS;
static BSP_leaf_pary_t  _renderlist_colst    = DYNAMIC_ARY_INIT_VALS;

static dolist_mgr_t     _dolist_mgr_data = DOLIST_MGR_INIT_VALS;
static BSP_leaf_pary_t  _dolist_colst    = DYNAMIC_ARY_INIT_VALS;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void gfx_system_init_SDL_graphics( void );
static void gfx_reset_timers( void );

static void _flip_pages( void );
static void _debug_print( const char *text );
static int  _debug_vprintf( const char *format, va_list args );
static int  _va_draw_string( float x, float y, const char *format, va_list args );
static int  _draw_string_raw( float x, float y, const char *format, ... );

static void gfx_update_fps_clock( void );
static void gfx_update_fps( void );

static void gfx_begin_text( void );
static void gfx_end_text( void );

static void gfx_enable_texturing( void );
//static void gfx_disable_texturing( void );

static void gfx_begin_2d( void );
static void gfx_end_2d( void );

static gfx_rv light_fans( renderlist_t * prlist );

static void line_list_draw_all( const camera_t * pcam );
static int  line_list_get_free( void );
static void point_list_draw_all( const camera_t * pcam );
static int  point_list_get_free( void );

static gfx_rv render_scene_init( renderlist_t * prlist, dolist_t * pdolist, dynalist_t * pdylist, const camera_t * pcam );
static gfx_rv render_scene_mesh_ndr( const camera_t * pcam, const renderlist_t * prlist );
static gfx_rv render_scene_mesh_drf_back( const camera_t * pcam, const renderlist_t * prlist );
static gfx_rv render_scene_mesh_ref( const camera_t * pcam, const renderlist_t * prlist, const dolist_t * pdolist );
static gfx_rv render_scene_mesh_ref_chr( const camera_t * pcam, const renderlist_t * prlist );
static gfx_rv render_scene_mesh_drf_solid( const camera_t * pcam, const renderlist_t * prlist );
static gfx_rv render_scene_mesh_render_shadows( const camera_t * pcam, const dolist_t * pdolist );
static gfx_rv render_scene_mesh( const camera_t * pcam, const renderlist_t * prlist, const dolist_t * pdolist );
static gfx_rv render_scene_solid( const camera_t * pcam, dolist_t * pdolist );
static gfx_rv render_scene_trans( const camera_t * pcam, dolist_t * pdolist );
static gfx_rv render_scene( const camera_t * pcam, const int render_list_index, const int dolist_index );
static gfx_rv render_fans_by_list( const camera_t * pcam, const ego_mpd_t * pmesh, const Uint32 list[], const size_t list_size );
static void   render_shadow( const CHR_REF character );
static void   render_bad_shadow( const CHR_REF character );
static gfx_rv render_water( renderlist_t * prlist );
static void   render_shadow_sprite( float intensity, GLvertex v[] );
static gfx_rv render_world_background( const camera_t * pcam, const TX_REF texture );
static gfx_rv render_world_overlay( const camera_t * pcam, const TX_REF texture );

static gfx_rv gfx_make_dolist( dolist_t * pdolist, const camera_t * pcam );
static gfx_rv gfx_make_renderlist( renderlist_t * prlist, const camera_t * pcam );
static gfx_rv gfx_make_dynalist( dynalist_t * pdylist, const camera_t * pcam );

static float draw_one_xp_bar( float x, float y, Uint8 ticks );
static float draw_character_xp_bar( const CHR_REF character, float x, float y );
static void  draw_all_status();
static void  draw_map();
static float draw_fps( float y );
static float draw_help( float y );
static float draw_debug( float y );
static float draw_timer( float y );
static float draw_game_status( float y );
static float draw_messages( float y );
static void  draw_quad_2d( oglx_texture_t * ptex, const ego_frect_t scr_rect, const ego_frect_t tx_rect, bool_t use_alpha );
static void  draw_hud( void );
static void  draw_inventory( void );

static void gfx_disable_texturing();
static void gfx_reshape_viewport( int w, int h );

static gfx_rv gfx_capture_mesh_tile( ego_tile_info_t * ptile );
static bool_t gfx_frustum_intersects_oct( const egolib_frustum_t * pf, const oct_bb_t * poct );

static gfx_rv update_one_chr_instance( struct s_chr * pchr );
static gfx_rv gfx_update_all_chr_instance( void );
static gfx_rv gfx_update_flashing( dolist_t * pdolist );

static bool_t light_fans_throttle_update( ego_mpd_t * pmesh, ego_tile_info_t * ptile, int fan, float threshold );
static gfx_rv light_fans_update_lcache( renderlist_t * prlist );
static gfx_rv light_fans_update_clst( renderlist_t * prlist );
static bool_t sum_global_lighting( lighting_vector_t lighting );
static float calc_light_rotation( int rotation, int normal );
static float calc_light_global( int rotation, int normal, float lx, float ly, float lz );

//static void gfx_init_icon_data( void );
static void   gfx_init_bar_data( void );
static void   gfx_init_blip_data( void );
static void   gfx_init_map_data( void );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

IMPLEMENT_STATIC_ARY( DisplayMsgAry, MAX_MESSAGE );

//--------------------------------------------------------------------------------------------
// MODULE "PRIVATE" FUNCTIONS
//--------------------------------------------------------------------------------------------
void _debug_print( const char *text )
{
    /// @author ZZ
    /// @details This function sticks a message in the display queue and sets its timer

    int          slot;
    const char * src;
    char       * dst, * dst_end;
    msg_t      * pmsg;

    if ( INVALID_CSTR( text ) ) return;

    // Get a "free" message
    slot = DisplayMsg_get_free();
    pmsg = DisplayMsgAry_get_ptr( &DisplayMsg, slot );

    // Copy the message
    for ( src = text, dst = pmsg->textdisplay, dst_end = dst + MESSAGESIZE;
          CSTR_END != *src && dst < dst_end;
          src++, dst++ )
    {
        *dst = *src;
    }
    if ( dst < dst_end ) *dst = CSTR_END;

    // Set the time
    pmsg->time = cfg.message_duration;
}

//--------------------------------------------------------------------------------------------
int _debug_vprintf( const char *format, va_list args )
{
    int retval = 0;

    if ( VALID_CSTR( format ) )
    {
        STRING szTmp;

        retval = vsnprintf( szTmp, SDL_arraysize( szTmp ), format, args );
        _debug_print( szTmp );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int _va_draw_string( float x, float y, const char *format, va_list args )
{
    int cnt = 1;
    int x_stt;
    STRING szText;
    Uint8 cTmp;

    oglx_texture_t * tx_ptr = TxMenu_get_valid_ptr(( TX_REF )MENU_FONT_BMP );
    if ( NULL == tx_ptr ) return y;

    if ( vsnprintf( szText, SDL_arraysize( szText ) - 1, format, args ) <= 0 )
    {
        return y;
    }

    gfx_begin_text();
    {
        x_stt = x;
        cnt = 0;
        cTmp = szText[cnt];
        while ( CSTR_END != cTmp )
        {
            Uint8 iTmp;

            // Convert ASCII to our own little font
            if ( '~' == cTmp )
            {
                // Use squiggle for tab
                x = ( FLOOR(( float )x / ( float )TABADD ) + 1.0f ) * TABADD;
            }
            else if ( C_NEW_LINE_CHAR == cTmp )
            {
                x  = x_stt;
                y += fontyspacing;
            }
            else if ( isspace( cTmp ) )
            {
                // other whitespace
                iTmp = asciitofont[cTmp];
                x += fontxspacing[iTmp] / 2;
            }
            else
            {
                // Normal letter
                iTmp = asciitofont[cTmp];
                draw_one_font( tx_ptr, iTmp, x, y );
                x += fontxspacing[iTmp];
            }

            cnt++;
            cTmp = szText[cnt];
        }
    }
    gfx_end_text();

    y += fontyspacing;

    return y;
}

//--------------------------------------------------------------------------------------------
int _draw_string_raw( float x, float y, const char *format, ... )
{
    /// @author BB
    /// @details the same as draw string, but it does not use the gfx_begin_2d() ... gfx_end_2d()
    ///    bookends.

    va_list args;

    va_start( args, format );
    y = _va_draw_string( x, y, format, args );
    va_end( args );

    return y;
}

//--------------------------------------------------------------------------------------------
// renderlist implementation
//--------------------------------------------------------------------------------------------

renderlist_t * renderlist_init( renderlist_t * plst, ego_mpd_t * pmesh )
{
    if ( NULL == plst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL pointer to renderlist initializer" );
        return NULL;
    }

    BLANK_STRUCT_PTR( plst )

    // init the 1st element of the array
    plst->all[0] = INVALID_TILE;
    plst->ref[0] = INVALID_TILE;
    plst->sha[0] = INVALID_TILE;
    plst->drf[0] = INVALID_TILE;
    plst->ndr[0] = INVALID_TILE;
    plst->wat[0] = INVALID_TILE;

    plst->pmesh = pmesh;

    return plst;
}

//--------------------------------------------------------------------------------------------
gfx_rv renderlist_reset( renderlist_t * plst )
{
    /// @author BB
    /// @details Clear old render lists

    ego_mpd_t * pmesh;
    ego_tile_info_t * tlist;
    int cnt;

    if ( NULL == plst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return gfx_error;
    }

    pmesh = renderlist_get_pmesh( plst );
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "renderlist does not contain a mesh" );
        return gfx_error;
    }

    // clear out the inrenderlist flag for the old mesh
    tlist = pmesh->tmem.tile_list;

    for ( cnt = 0; cnt < plst->all_count; cnt++ )
    {
        Uint32 fan = plst->all[cnt];
        if ( fan < pmesh->info.tiles_count )
        {
            tlist[fan].inrenderlist       = bfalse;
            tlist[fan].inrenderlist_frame = 0;
        }
    }

    // re-initialize the renderlist
    renderlist_init( plst, plst->pmesh );

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv renderlist_insert( renderlist_t * plst, const ego_grid_info_t * pgrid, const Uint32 index )
{
    // Make sure it doesn't die ugly
    if ( NULL == plst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return gfx_error;
    }

    // check for a valid tile
    if ( NULL == pgrid || INVALID_TILE == index ) return gfx_fail;

    // we can only accept so many tiles
    if ( plst->all_count >= MAXMESHRENDER ) return gfx_fail;

    // Put each tile in basic list
    plst->all[plst->all_count] = index;
    plst->all_count++;

    // Put each tile in one other list, for shadows and relections
    if ( 0 != ego_grid_info_test_all_fx( pgrid, MPDFX_SHA ) )
    {
        plst->sha[plst->sha_count] = index;
        plst->sha_count++;
    }
    else
    {
        plst->ref[plst->ref_count] = index;
        plst->ref_count++;
    }

    if ( 0 != ego_grid_info_test_all_fx( pgrid, MPDFX_DRAWREF ) )
    {
        plst->drf[plst->drf_count] = index;
        plst->drf_count++;
    }
    else
    {
        plst->ndr[plst->ndr_count] = index;
        plst->ndr_count++;
    }

    if ( 0 != ego_grid_info_test_all_fx( pgrid, MPDFX_WATER ) )
    {
        plst->wat[plst->wat_count] = index;
        plst->wat_count++;
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * renderlist_get_pmesh( const renderlist_t * ptr )
{
    if ( NULL == ptr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return NULL;
    }

    return ptr->pmesh;
}

//--------------------------------------------------------------------------------------------
gfx_rv renderlist_attach_mesh( renderlist_t * ptr, ego_mpd_t * pmesh )
{
    if ( NULL == ptr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return gfx_error;
    }

    ptr->pmesh = pmesh;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv renderlist_add_colst( renderlist_t * prlist, const BSP_leaf_pary_t * pcolst )
{
    int j;
    BSP_leaf_t * pleaf;
    ego_mpd_t  * pmesh  = NULL;
    gfx_rv       retval = gfx_error;

    if ( NULL == prlist || NULL == pcolst )
    {
        return gfx_error;
    }

    if ( 0 == BSP_leaf_pary_get_size( pcolst ) || 0 == BSP_leaf_pary_get_top( pcolst ) )
    {
        return gfx_fail;
    }

    pmesh = renderlist_get_pmesh( prlist );
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "renderlist is not attached to a mesh" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    // transfer valid pcolst entries to the renderlist
    for ( j = 0; j < pcolst->top; j++ )
    {
        pleaf = pcolst->ary[j];
        if ( NULL == pleaf ) continue;

        if ( BSP_LEAF_TILE == pleaf->data_type )
        {
            Uint32 ifan;
            ego_tile_info_t * ptile;
            ego_grid_info_t * pgrid;

            // collided with a character
            ifan = ( Uint32 )( pleaf->index );

            ptile = mesh_get_ptile( pmesh, ifan );
            if ( NULL == ptile ) continue;

            pgrid = mesh_get_pgrid( pmesh, ifan );
            if ( NULL == pgrid ) continue;

            if ( gfx_error == gfx_capture_mesh_tile( ptile ) )
            {
                retval = gfx_error;
                break;
            }

            if ( gfx_error == renderlist_insert( prlist, pgrid, ifan ) )
            {
                retval = gfx_error;
                break;
            }
        }
        else
        {
            // how did we get here?
            log_warning( "%s-%s-%d- found non-tile in the mpd BSP\n", __FILE__, __FUNCTION__, __LINE__ );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// renderlist array implementation
//--------------------------------------------------------------------------------------------
renderlist_ary_t * renderlist_ary_begin( renderlist_ary_t * ptr )
{
    int cnt;

    if ( NULL == ptr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL pointer to renderlist array" );
        return NULL;
    }

    if ( ptr->started ) return ptr;

    for ( cnt = 0; cnt < MAX_RENDER_LISTS; cnt++ )
    {
        ptr->free_lst[cnt] = cnt;
        renderlist_init( ptr->lst + cnt, NULL );
    }
    ptr->free_count = MAX_RENDER_LISTS;

    ptr->started = btrue;

    return ptr;
}

//--------------------------------------------------------------------------------------------
renderlist_ary_t * renderlist_ary_end( renderlist_ary_t * ptr )
{
    int cnt;

    if ( NULL == ptr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL pointer to renderlist array" );
        return NULL;
    }

    if ( !ptr->started ) return ptr;

    ptr->free_count = 0;
    for ( cnt = 0; cnt < MAX_RENDER_LISTS; cnt++ )
    {
        ptr->free_lst[cnt] = -1;
        renderlist_init( ptr->lst + cnt, NULL );
    }

    ptr->started = bfalse;

    return ptr;
}

//--------------------------------------------------------------------------------------------
// renderlist manager implementation
//--------------------------------------------------------------------------------------------
gfx_rv renderlist_mgr_begin( renderlist_mgr_t * pmgr )
{
    // valid pointer?
    if ( NULL == pmgr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL pointer to renderlist manager" );
        return gfx_error;
    }

    if ( pmgr->started ) return gfx_success;

    renderlist_ary_begin( &( pmgr->ary ) );

    pmgr->started = btrue;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv renderlist_mgr_end( renderlist_mgr_t * pmgr )
{
    // valid pointer?
    if ( NULL == pmgr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL pointer to renderlist manager" );
        return gfx_error;
    }

    if ( !pmgr->started ) return gfx_success;

    renderlist_ary_end( &( pmgr->ary ) );

    // the manager is no longer running
    pmgr->started = bfalse;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
int renderlist_mgr_get_free_index( renderlist_mgr_t * pmgr )
{
    int retval = -1;

    // renderlist manager started?
    if ( gfx_success != renderlist_mgr_begin( pmgr ) )
    {
        return -1;
    }

    if ( pmgr->ary.free_count <= 0 ) return -1;

    // resduce the count
    pmgr->ary.free_count--;

    // grab the index
    retval = pmgr->ary.free_count;

    // blank the free list
    if ( retval >= 0 && retval < MAX_RENDER_LISTS )
    {
        pmgr->ary.free_lst[ retval ] = -1;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv renderlist_mgr_free_one( renderlist_mgr_t * pmgr, int index )
{
    size_t cnt;
    gfx_rv retval = gfx_error;

    // renderlist manager started?
    if ( gfx_success != renderlist_mgr_begin( pmgr ) )
    {
        return gfx_error;
    }

    // valid index range?
    if ( index < 0 || index >= MAX_RENDER_LISTS ) return gfx_error;

    // is the index already freed?
    for ( cnt = 0; cnt < pmgr->ary.free_count; cnt++ )
    {
        if ( index == pmgr->ary.free_lst[cnt] )
        {
            return gfx_fail;
        }
    }

    // push it on the list
    retval = gfx_fail;
    if ( pmgr->ary.free_count < MAX_RENDER_LISTS )
    {
        pmgr->ary.free_lst[pmgr->ary.free_count] = index;
        pmgr->ary.free_count++;
        retval = gfx_success;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
renderlist_t * renderlist_mgr_get_ptr( renderlist_mgr_t * pmgr, int index )
{
    // renderlist manager started?
    if ( gfx_success != renderlist_mgr_begin( pmgr ) )
    {
        return NULL;
    }

    // valid index range?
    if ( index < 0 || index >= MAX_RENDER_LISTS )
    {
        return NULL;
    }

    return pmgr->ary.lst + index;
}

//--------------------------------------------------------------------------------------------
// dolist implementation
//--------------------------------------------------------------------------------------------
dolist_t * dolist_init( dolist_t * plist, const int index )
{
    if ( NULL == plist ) return NULL;

    BLANK_STRUCT_PTR( plist )

    obj_registry_entity_init( plist->lst + 0 );

    // give the dolist a "name"
    plist->index = index;

    return plist;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_reset( dolist_t * plist, const int index )
{
    int cnt, entries;

    if ( NULL == plist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

    if ( plist->index < 0 || plist->index >= MAX_DO_LISTS )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist index" );
        return gfx_error;
    }

    // if there is nothing in the dolist, we are done
    if ( 0 == plist->count )
    {
        return gfx_success;
    }

    entries = MIN( plist->count, DOLIST_SIZE );
    for ( cnt = 0; cnt < entries; cnt++ )
    {
        obj_registry_entity_t * pent = plist->lst + cnt;

        // tell all valid objects that they are removed from this dolist
        if ( MAX_CHR == pent->ichr && VALID_PRT_RANGE( pent->iprt ) )
        {
            PrtList.lst[pent->iprt].inst.indolist = bfalse;
        }
        else if ( MAX_PRT == pent->iprt && VALID_CHR_RANGE( pent->ichr ) )
        {
            ChrList.lst[pent->ichr].inst.indolist = bfalse;
        }
    }
    plist->count = 0;

    // reset the dolist
    dolist_init( plist, index );

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_test_chr( dolist_t * pdlist, const chr_t * pchr )
{
    if ( NULL == pdlist )
    {
        return gfx_error;
    }

    if ( pdlist->count >= DOLIST_SIZE )
    {
        return gfx_fail;
    }

    if ( !INGAME_PCHR( pchr ) )
    {
        return gfx_fail;
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_add_chr_raw( dolist_t * pdlist, chr_t * pchr )
{
    /// @author ZZ
    /// @details This function puts a character in the list

    if ( NULL == pdlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist pointer" );
        return gfx_error;
    }

    if ( NULL == pchr )
    {
        return gfx_fail;
    }

    //// don't do anything if it's already in the list
    //if ( pchr->inst.indolist[pdlist->index] )
    //{
    //    return gfx_success;
    //}

    // don't add if it is hidden
    if ( pchr->is_hidden )
    {
        return gfx_fail;
    }

    // don't add if it's in another character's inventory
    if ( INGAME_CHR( pchr->inwhich_inventory ) )
    {
        return gfx_fail;
    }

    // fix the dolist
    pdlist->lst[pdlist->count].ichr = GET_REF_PCHR( pchr );
    pdlist->lst[pdlist->count].iprt = ( PRT_REF )MAX_PRT;
    pdlist->count++;

    // fix the instance
    pchr->inst.indolist = btrue;

    // Add any weapons
    dolist_add_chr_raw( pdlist, ChrList_get_ptr( pchr->holdingwhich[SLOT_LEFT] ) );
    dolist_add_chr_raw( pdlist, ChrList_get_ptr( pchr->holdingwhich[SLOT_RIGHT] ) );

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_test_prt( dolist_t * pdlist, const prt_t * pprt )
{
    if ( NULL == pdlist )
    {
        return gfx_error;
    }

    if ( pdlist->count >= DOLIST_SIZE )
    {
        return gfx_fail;
    }

    if ( !DISPLAY_PPRT( pprt ) )
    {
        return gfx_fail;
    }

    if ( pprt->is_hidden || 0 == pprt->size )
    {
        return gfx_fail;
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_add_prt_raw( dolist_t * pdlist, prt_t * pprt )
{
    /// @author ZZ
    /// @details This function puts a character in the list

    //if ( pprt->inst.indolist[pdlist->index] )
    //{
    //    return gfx_success;
    //}

    pdlist->lst[pdlist->count].ichr = ( CHR_REF )MAX_CHR;
    pdlist->lst[pdlist->count].iprt = GET_REF_PPRT( pprt );
    pdlist->count++;

    pprt->inst.indolist = btrue;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_add_colst( dolist_t * pdlist, const BSP_leaf_pary_t * pcolst )
{
    BSP_leaf_t * pleaf;
    int j;
    gfx_rv retval;

    if ( NULL == pdlist || NULL == pcolst )
    {
        return gfx_error;
    }

    if ( 0 == BSP_leaf_pary_get_size( pcolst ) || 0 == BSP_leaf_pary_get_top( pcolst ) )
    {
        return gfx_fail;
    }

    // assume the best
    retval = gfx_success;

    for ( j = 0; j < pcolst->top; j++ )
    {
        pleaf = pcolst->ary[j];
        if ( NULL == pleaf ) continue;

        if ( BSP_LEAF_CHR == pleaf->data_type )
        {
            CHR_REF ichr;
            chr_t * pchr;

            // collided with a character
            ichr = ( CHR_REF )( pleaf->index );

            // is it in the array?
            if ( !VALID_CHR_RANGE( ichr ) ) continue;
            pchr = ChrList_get_ptr( ichr );

            // do some more obvious tests before testing the frustum
            if ( dolist_test_chr( pdlist, pchr ) )
            {
                // Add the character
                if ( gfx_error == dolist_add_chr_raw( pdlist, pchr ) )
                {
                    retval = gfx_error;
                    break;
                }
            }
        }
        else if ( BSP_LEAF_PRT == pleaf->data_type )
        {
            PRT_REF iprt;
            prt_t * pprt;

            // collided with a particle
            iprt = ( PRT_REF )( pleaf->index );

            // is it in the array?
            if ( !VALID_PRT_RANGE( iprt ) ) continue;
            pprt = PrtList_get_ptr( iprt );

            // do some more obvious tests before testing the frustum
            if ( dolist_test_prt( pdlist, pprt ) )
            {
                // Add the particle
                if ( gfx_error == dolist_add_prt_raw( pdlist, pprt ) )
                {
                    retval = gfx_error;
                    break;
                }
            }
        }
        else
        {
            // how did we get here?
            log_warning( "%s-%s-%d- found unknown type in a dolist BSP\n", __FILE__, __FUNCTION__, __LINE__ );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_sort( dolist_t * pdlist, const camera_t * pcam, const bool_t do_reflect )
{
    /// @author ZZ
    /// @details This function orders the dolist based on distance from camera,
    ///    which is needed for reflections to properly clip themselves.
    ///    Order from closest to farthest

    Uint32    cnt;
    fvec3_t   vcam;
    size_t    count;

    if ( NULL == pdlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

    if ( pdlist->count >= DOLIST_SIZE )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist size" );
        return gfx_error;
    }

    if ( NULL == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    mat_getCamForward( pcam->mView.v, vcam.v );

    // Figure the distance of each
    count = 0;
    for ( cnt = 0; cnt < pdlist->count; cnt++ )
    {
        fvec3_t   vtmp;
        float dist;

        if ( MAX_PRT == pdlist->lst[cnt].iprt && VALID_CHR_RANGE( pdlist->lst[cnt].ichr ) )
        {
            CHR_REF ichr;
            fvec3_t pos_tmp;

            ichr = pdlist->lst[cnt].ichr;

            if ( do_reflect )
            {
                mat_getTranslate( ChrList.lst[ichr].inst.ref.matrix.v, pos_tmp.v );
            }
            else
            {
                mat_getTranslate( ChrList.lst[ichr].inst.matrix.v, pos_tmp.v );
            }

            fvec3_sub( vtmp.v, pos_tmp.v, pcam->pos.v );
        }
        else if ( MAX_CHR == pdlist->lst[cnt].ichr && VALID_PRT_RANGE( pdlist->lst[cnt].iprt ) )
        {
            PRT_REF iprt = pdlist->lst[cnt].iprt;

            if ( do_reflect )
            {
                fvec3_sub( vtmp.v, PrtList.lst[iprt].inst.pos.v, pcam->pos.v );
            }
            else
            {
                fvec3_sub( vtmp.v, PrtList.lst[iprt].inst.ref_pos.v, pcam->pos.v );
            }
        }
        else
        {
            continue;
        }

        dist = fvec3_dot_product( vtmp.v, vcam.v );
        if ( dist > 0 )
        {
            pdlist->lst[count].ichr = pdlist->lst[cnt].ichr;
            pdlist->lst[count].iprt = pdlist->lst[cnt].iprt;
            pdlist->lst[count].dist = dist;
            count++;
        }
    }
    pdlist->count = count;

    // use qsort to sort the list in-place
    if ( pdlist->count > 1 )
    {
        qsort( pdlist->lst, pdlist->count, sizeof( obj_registry_entity_t ), obj_registry_entity_cmp );
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
// dolist array implementation
//--------------------------------------------------------------------------------------------
dolist_ary_t * dolist_ary_begin( dolist_ary_t * ptr )
{
    int cnt;

    if ( NULL == ptr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL pointer to dolist array" );
        return NULL;
    }

    if ( ptr->started ) return ptr;

    for ( cnt = 0; cnt < MAX_DO_LISTS; cnt++ )
    {
        ptr->free_lst[cnt] = cnt;
        dolist_init( ptr->lst + cnt, cnt );
    }
    ptr->free_count = MAX_DO_LISTS;

    ptr->started = btrue;

    return ptr;
}

//--------------------------------------------------------------------------------------------
dolist_ary_t * dolist_ary_end( dolist_ary_t * ptr )
{
    int cnt;

    if ( NULL == ptr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL pointer to dolist array" );
        return NULL;
    }

    if ( !ptr->started ) return ptr;

    ptr->free_count = 0;
    for ( cnt = 0; cnt < MAX_DO_LISTS; cnt++ )
    {
        ptr->free_lst[cnt] = -1;
        dolist_init( ptr->lst + cnt, cnt );
    }

    ptr->started = bfalse;

    return ptr;
}

//--------------------------------------------------------------------------------------------
// dolist manager implementation
//--------------------------------------------------------------------------------------------
gfx_rv dolist_mgr_begin( dolist_mgr_t * pmgr )
{
    // valid pointer?
    if ( NULL == pmgr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL pointer to dolist manager" );
        return gfx_error;
    }

    if ( pmgr->started ) return gfx_success;

    dolist_ary_begin( &( pmgr->ary ) );

    pmgr->started = btrue;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_mgr_end( dolist_mgr_t * pmgr )
{
    // valid pointer?
    if ( NULL == pmgr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL pointer to dolist manager" );
        return gfx_error;
    }

    if ( !pmgr->started ) return gfx_success;

    dolist_ary_end( &( pmgr->ary ) );

    // the manager is no longer running
    pmgr->started = bfalse;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
int dolist_mgr_get_free_index( dolist_mgr_t * pmgr )
{
    int retval = -1;

    // dolist manager started?
    if ( gfx_success != dolist_mgr_begin( pmgr ) )
    {
        return -1;
    }

    if ( pmgr->ary.free_count <= 0 ) return -1;

    // resduce the count
    pmgr->ary.free_count--;

    // grab the index
    retval = pmgr->ary.free_count;

    // blank the free list
    if ( retval >= 0 && retval < MAX_DO_LISTS )
    {
        pmgr->ary.free_lst[ retval ] = -1;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_mgr_free_one( dolist_mgr_t * pmgr, int index )
{
    size_t cnt;
    gfx_rv retval = gfx_error;

    // dolist manager started?
    if ( gfx_success != dolist_mgr_begin( pmgr ) )
    {
        return gfx_error;
    }

    // valid index range?
    if ( index < 0 || index >= MAX_DO_LISTS ) return gfx_error;

    // is the index already freed?
    for ( cnt = 0; cnt < pmgr->ary.free_count; cnt++ )
    {
        if ( index == pmgr->ary.free_lst[cnt] )
        {
            return gfx_fail;
        }
    }

    // push it on the list
    retval = gfx_fail;
    if ( pmgr->ary.free_count < MAX_DO_LISTS )
    {
        pmgr->ary.free_lst[pmgr->ary.free_count] = index;
        pmgr->ary.free_count++;
        retval = gfx_success;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
dolist_t * dolist_mgr_get_ptr( dolist_mgr_t * pmgr, int index )
{
    // dolist manager started?
    if ( gfx_success != dolist_mgr_begin( pmgr ) )
    {
        return NULL;
    }

    // valid index range?
    if ( index < 0 || index >= MAX_DO_LISTS )
    {
        return NULL;
    }

    return pmgr->ary.lst + index;
}

//--------------------------------------------------------------------------------------------
// gfx_system INITIALIZATION
//--------------------------------------------------------------------------------------------
void gfx_system_begin()
{
    // set the graphics state
    gfx_system_init_SDL_graphics();
    gfx_system_init_OpenGL();

    // initialize the renderlist manager
    renderlist_mgr_begin( &_renderlist_mgr_data );

    // initialize the dolist manager
    dolist_mgr_begin( &_dolist_mgr_data );

    // initialize the dynalist frame
    // otherwise, it will not update until the frame count reaches whatever
    // left over or random value is in this counter
    _dynalist.frame = -1;
    _dynalist.count = 0;

    // begin the billboard system
    billboard_system_begin();

    // initialize the gfx data dtructures
    TxTexture_init_all();

    // initialize the profiling variables
    PROFILE_INIT( render_scene_init );
    PROFILE_INIT( render_scene_mesh );
    PROFILE_INIT( render_scene_solid );
    PROFILE_INIT( render_scene_water );
    PROFILE_INIT( render_scene_trans );

    PROFILE_INIT( gfx_make_renderlist );
    PROFILE_INIT( gfx_make_dolist );
    PROFILE_INIT( do_grid_lighting );
    PROFILE_INIT( light_fans );
    PROFILE_INIT( gfx_update_all_chr_instance );
    PROFILE_INIT( update_all_prt_instance );

    PROFILE_INIT( render_scene_mesh_dolist_sort );
    PROFILE_INIT( render_scene_mesh_ndr );
    PROFILE_INIT( render_scene_mesh_drf_back );
    PROFILE_INIT( render_scene_mesh_ref );
    PROFILE_INIT( render_scene_mesh_ref_chr );
    PROFILE_INIT( render_scene_mesh_drf_solid );
    PROFILE_INIT( render_scene_mesh_render_shadows );

    // init some other variables
    stabilized_game_fps        = TARGET_FPS;
    stabilized_game_fps_sum    = STABILIZED_COVER * TARGET_FPS;
    stabilized_game_fps_weight = STABILIZED_COVER;

    gfx_reset_timers();

    // allocate the specailized "collistion lists"
    if ( NULL == BSP_leaf_pary_ctor( &_dolist_colst, DOLIST_SIZE ) )
    {
        log_error( "%s-%s-%d - Could not allocate dolist collision list", __FILE__, __FUNCTION__, __LINE__ );
    }

    if ( NULL == BSP_leaf_pary_ctor( &_renderlist_colst, MAXMESHRENDER ) )
    {
        log_error( "%s-%s-%d - Could not allocate renderlist collision list", __FILE__, __FUNCTION__, __LINE__ );
    }

    BLANK_STRUCT( gfx_update_timer )
}

//--------------------------------------------------------------------------------------------
void gfx_system_end()
{
    // initialize the profiling variables
    PROFILE_FREE( render_scene_init );
    PROFILE_FREE( render_scene_mesh );
    PROFILE_FREE( render_scene_solid );
    PROFILE_FREE( render_scene_water );
    PROFILE_FREE( render_scene_trans );

    PROFILE_FREE( gfx_make_renderlist );
    PROFILE_FREE( gfx_make_dolist );
    PROFILE_FREE( do_grid_lighting );
    PROFILE_FREE( light_fans );
    PROFILE_FREE( gfx_update_all_chr_instance );
    PROFILE_FREE( update_all_prt_instance );

    PROFILE_FREE( render_scene_mesh_dolist_sort );
    PROFILE_FREE( render_scene_mesh_ndr );
    PROFILE_FREE( render_scene_mesh_drf_back );
    PROFILE_FREE( render_scene_mesh_ref );
    PROFILE_FREE( render_scene_mesh_ref_chr );
    PROFILE_FREE( render_scene_mesh_drf_solid );
    PROFILE_FREE( render_scene_mesh_render_shadows );

    // end the billboard system
    billboard_system_end();

    TxTexture_release_all();

    // de-initialize the renderlist manager
    renderlist_mgr_end( &_renderlist_mgr_data );

    // de-initialize the dolist manager
    dolist_mgr_end( &_dolist_mgr_data );

    gfx_reset_timers();

    // deallocate the specailized "collistion lists"
    if ( NULL == BSP_leaf_pary_dtor( &_dolist_colst ) )
    {
        log_warning( "%s-%s-%d - Could not deallocate dolist collision list", __FILE__, __FUNCTION__, __LINE__ );
    }

    if ( NULL == BSP_leaf_pary_dtor( &_renderlist_colst ) )
    {
        log_warning( "%s-%s-%d - Could not deallocate renderlist collision list", __FILE__, __FUNCTION__, __LINE__ );
    }

}

//--------------------------------------------------------------------------------------------
int gfx_system_init_OpenGL()
{
    gfx_system_init_SDL_graphics();

    // GL_DEBUG(glClear)) stuff
    GL_DEBUG( glClearColor )( 0.0f, 0.0f, 0.0f, 0.0f ); // Set the background black
    GL_DEBUG( glClearDepth )( 1.0f );

    // depth buffer stuff
    GL_DEBUG( glClearDepth )( 1.0f );
    GL_DEBUG( glDepthMask )( GL_TRUE );

    // do not draw hidden surfaces
    GL_DEBUG( glEnable )( GL_DEPTH_TEST );
    GL_DEBUG( glDepthFunc )( GL_LESS );

    // alpha stuff
    GL_DEBUG( glDisable )( GL_BLEND );

    // do not display the completely transparent portion
    GL_DEBUG( glEnable )( GL_ALPHA_TEST );
    GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );

    /// @todo Including backface culling here prevents the mesh from getting rendered
    /// backface culling

    // oglx_begin_culling( GL_BACK, GL_CW );            // GL_ENABLE_BIT | GL_POLYGON_BIT

    // disable OpenGL lighting
    GL_DEBUG( glDisable )( GL_LIGHTING );

    // fill mode
    GL_DEBUG( glPolygonMode )( GL_FRONT, GL_FILL );
    GL_DEBUG( glPolygonMode )( GL_BACK,  GL_FILL );

    // ?Need this for color + lighting?
    GL_DEBUG( glEnable )( GL_COLOR_MATERIAL );  // Need this for color + lighting

    // set up environment mapping
    GL_DEBUG( glTexGeni )( GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );  // Set The Texture Generation Mode For S To Sphere Mapping (NEW)
    GL_DEBUG( glTexGeni )( GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );  // Set The Texture Generation Mode For T To Sphere Mapping (NEW)

    //Initialize the motion blur buffer
    GL_DEBUG( glClearAccum )( 0.0f, 0.0f, 0.0f, 1.0f );
    GL_DEBUG( glClear )( GL_ACCUM_BUFFER_BIT );

    // Load the current graphical settings
    // gfx_system_load_assets();

    _ogl_initialized = btrue;

    return _ogl_initialized && _sdl_initialized_graphics;
}

//--------------------------------------------------------------------------------------------
void gfx_system_init_SDL_graphics()
{
    if ( _sdl_initialized_graphics ) return;

    ego_init_SDL_base();

    log_info( "Intializing SDL Video... " );
    if ( SDL_InitSubSystem( SDL_INIT_VIDEO ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

#if !defined(__APPLE__)
    {
        //Setup the cute windows manager icon, don't do this on Mac
        SDL_Surface *theSurface;
        const char * fname = "icon.bmp";
        STRING fileload;

        snprintf( fileload, SDL_arraysize( fileload ), "mp_data/%s", fname );

        theSurface = IMG_Load( vfs_resolveReadFilename( fileload ) );
        if ( NULL == theSurface )
        {
            log_warning( "Unable to load icon (%s)\n", fname );
        }
        else
        {
            SDL_WM_SetIcon( theSurface, NULL );
        }
    }
#endif

    // Set the window name
    SDL_WM_SetCaption( "Egoboo " VERSION, "Egoboo" );

#if defined(__unix__)

    // GLX doesn't differentiate between 24 and 32 bpp, asking for 32 bpp
    // will cause SDL_SetVideoMode to fail with:
    // "Unable to set video mode: Couldn't find matching GLX visual"
    if ( 32 == cfg.scrd_req ) cfg.scrd_req = 24;
    if ( 32 == cfg.scrz_req ) cfg.scrz_req = 24;

#endif

    // the flags to pass to SDL_SetVideoMode
    sdl_vparam.width                     = cfg.scrx_req;
    sdl_vparam.height                    = cfg.scry_req;
    sdl_vparam.depth                     = cfg.scrd_req;

    sdl_vparam.flags.opengl              = SDL_TRUE;
    sdl_vparam.flags.double_buf          = SDL_TRUE;
    sdl_vparam.flags.full_screen         = cfg.fullscreen_req;

    sdl_vparam.gl_att.buffer_size        = cfg.scrd_req;
    sdl_vparam.gl_att.depth_size         = cfg.scrz_req;
    sdl_vparam.gl_att.multi_buffers      = ( cfg.multisamples > 1 ) ? 1 : 0;
    sdl_vparam.gl_att.multi_samples      = cfg.multisamples;
    sdl_vparam.gl_att.accelerated_visual = GL_TRUE;

    ogl_vparam.dither         = cfg.use_dither ? GL_TRUE : GL_FALSE;
    ogl_vparam.antialiasing   = GL_TRUE;
    ogl_vparam.perspective    = cfg.use_perspective ? GL_NICEST : GL_FASTEST;
    ogl_vparam.shading        = GL_SMOOTH;
    ogl_vparam.userAnisotropy = 16.0f * MAX( 0, cfg.texturefilter_req - TX_TRILINEAR_2 );

    log_info( "Opening SDL Video Mode...\n" );

    // redirect the output of the SDL_GL_* debug functions
    SDL_GL_set_stdout( log_get_file() );

    // actually set the video mode
    if ( NULL == SDL_GL_set_mode( NULL, &sdl_vparam, &ogl_vparam, _sdl_initialized_graphics ) )
    {
        log_message( "Failed!\n" );
        log_error( "I can't get SDL to set any video mode: %s\n", SDL_GetError() );
    }
    else
    {
        GFX_WIDTH = ( float )GFX_HEIGHT / ( float )sdl_vparam.height * ( float )sdl_vparam.width;
        log_message( "Success!\n" );
    }

    _sdl_initialized_graphics = SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
void gfx_system_render_world( const camera_t * pcam, const int render_list_index, const int dolist_index )
{
    gfx_error_state_t * err_tmp;

    gfx_error_clear();

    gfx_begin_3d( pcam );
    {
        if ( gfx.draw_background )
        {
            // Render the background
            // TX_WATER_LOW for waterlow.bmp
            render_world_background( pcam, ( TX_REF )TX_WATER_LOW );
        }

        render_scene( pcam, render_list_index, dolist_index );

        if ( gfx.draw_overlay )
        {
            // Foreground overlay
            // TX_WATER_TOP is watertop.bmp
            render_world_overlay( pcam, ( TX_REF )TX_WATER_TOP );
        }

        if ( pcam->motion_blur > 0 )
        {
            //Do motion blur
            GL_DEBUG( glAccum )( GL_MULT, pcam->motion_blur );
            GL_DEBUG( glAccum )( GL_ACCUM, 1.0f - pcam->motion_blur );
            GL_DEBUG( glAccum )( GL_RETURN, 1.0f );
        }
    }
    gfx_end_3d();

    // Render the billboards
    billboard_system_render_all( pcam );

    err_tmp = gfx_error_pop();
    if ( NULL != err_tmp )
    {
        printf( "**** Encountered graphics errors in frame %d ****\n\n", game_frame_all );
        while ( NULL != err_tmp )
        {
            printf( "vvvv\n" );
            printf(
                "\tfile     == %s\n"
                "\tline     == %d\n"
                "\tfunction == %s\n"
                "\tcode     == %d\n"
                "\tstring   == %s\n",
                err_tmp->file, err_tmp->line, err_tmp->function, err_tmp->type, err_tmp->string );
            printf( "^^^^\n\n" );

            err_tmp = gfx_error_pop();
        }
        printf( "****\n\n" );
    }
}

//--------------------------------------------------------------------------------------------
void gfx_system_main()
{
    /// @author ZZ
    /// @details This function does all the drawing stuff

    camera_system_render_all( gfx_system_render_world );

    draw_hud();

    gfx_request_flip_pages();
}

//--------------------------------------------------------------------------------------------
bool_t gfx_system_set_virtual_screen( gfx_config_t * pgfx )
{
    float kx, ky;

    if ( NULL == pgfx ) return bfalse;

    kx = ( float )GFX_WIDTH  / ( float )sdl_scr.x;
    ky = ( float )GFX_HEIGHT / ( float )sdl_scr.y;

    if ( kx == ky )
    {
        pgfx->vw = sdl_scr.x;
        pgfx->vh = sdl_scr.y;
    }
    else if ( kx > ky )
    {
        pgfx->vw = sdl_scr.x * kx / ky;
        pgfx->vh = sdl_scr.y;
    }
    else
    {
        pgfx->vw = sdl_scr.x;
        pgfx->vh = sdl_scr.y * ky / kx;
    }

    pgfx->vdw = ( GFX_WIDTH  - pgfx->vw ) * 0.5f;
    pgfx->vdh = ( GFX_HEIGHT - pgfx->vh ) * 0.5f;

    ui_set_virtual_screen( pgfx->vw, pgfx->vh, GFX_WIDTH, GFX_HEIGHT );

    //gfx_calc_rotmesh();

    return btrue;
}

//--------------------------------------------------------------------------------------------
renderlist_mgr_t * gfx_system_get_renderlist_mgr()
{
    if ( gfx_success != renderlist_mgr_begin( &_renderlist_mgr_data ) )
    {
        return NULL;
    }

    return &_renderlist_mgr_data;
}

//--------------------------------------------------------------------------------------------
dolist_mgr_t * gfx_system_get_dolist_mgr()
{
    if ( gfx_success != dolist_mgr_begin( &_dolist_mgr_data ) )
    {
        return NULL;
    }

    return &_dolist_mgr_data;
}

//--------------------------------------------------------------------------------------------
void gfx_system_load_assets()
{
    /// @author ZF
    /// @details This function loads all the graphics based on the game settings

    GLenum quality;

    // Check if the computer graphic driver supports anisotropic filtering

    if ( !ogl_caps.anisotropic_supported )
    {
        if ( tex_params.texturefilter >= TX_ANISOTROPIC )
        {
            tex_params.texturefilter = TX_TRILINEAR_2;
            log_warning( "Your graphics driver does not support anisotropic filtering.\n" );
        }
    }

    // Enable prespective correction?
    if ( gfx.perspective ) quality = GL_NICEST;
    else quality = GL_FASTEST;
    GL_DEBUG( glHint )( GL_PERSPECTIVE_CORRECTION_HINT, quality );

    // Enable dithering?
    if ( gfx.dither )
    {
        GL_DEBUG( glHint )( GL_GENERATE_MIPMAP_HINT, GL_NICEST );
        GL_DEBUG( glEnable )( GL_DITHER );
    }
    else
    {
        GL_DEBUG( glHint )( GL_GENERATE_MIPMAP_HINT, GL_FASTEST );
        GL_DEBUG( glDisable )( GL_DITHER );                          // ENABLE_BIT
    }

    // Enable Gouraud shading? (Important!)
    GL_DEBUG( glShadeModel )( gfx.shading );         // GL_LIGHTING_BIT

    // Enable antialiasing?
    if ( gfx.antialiasing )
    {
        GL_DEBUG( glEnable )( GL_MULTISAMPLE_ARB );

        GL_DEBUG( glEnable )( GL_LINE_SMOOTH );
        GL_DEBUG( glHint )( GL_LINE_SMOOTH_HINT,    GL_NICEST );

        GL_DEBUG( glEnable )( GL_POINT_SMOOTH );
        GL_DEBUG( glHint )( GL_POINT_SMOOTH_HINT,   GL_NICEST );

        GL_DEBUG( glDisable )( GL_POLYGON_SMOOTH );
        GL_DEBUG( glHint )( GL_POLYGON_SMOOTH_HINT,    GL_FASTEST );

        // PLEASE do not turn this on unless you use
        // GL_DEBUG(glEnable)(GL_BLEND);
        // GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // before every single draw command
        //
        // GL_DEBUG(glEnable)(GL_POLYGON_SMOOTH);
        // GL_DEBUG(glHint)(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    }
    else
    {
        GL_DEBUG( glDisable )( GL_MULTISAMPLE_ARB );
        GL_DEBUG( glDisable )( GL_POINT_SMOOTH );
        GL_DEBUG( glDisable )( GL_LINE_SMOOTH );
        GL_DEBUG( glDisable )( GL_POLYGON_SMOOTH );
    }
}

//--------------------------------------------------------------------------------------------
void gfx_system_init_all_graphics()
{
    gfx_init_bar_data();
    gfx_init_blip_data();
    gfx_init_map_data();
    font_bmp_init();

    billboard_system_init();
    TxTexture_init_all();

    PROFILE_RESET( render_scene_init );
    PROFILE_RESET( render_scene_mesh );
    PROFILE_RESET( render_scene_solid );
    PROFILE_RESET( render_scene_water );
    PROFILE_RESET( render_scene_trans );

    PROFILE_RESET( gfx_make_renderlist );
    PROFILE_RESET( gfx_make_dolist );
    PROFILE_RESET( do_grid_lighting );
    PROFILE_RESET( light_fans );
    PROFILE_RESET( gfx_update_all_chr_instance );
    PROFILE_RESET( update_all_prt_instance );

    PROFILE_RESET( render_scene_mesh_dolist_sort );
    PROFILE_RESET( render_scene_mesh_ndr );
    PROFILE_RESET( render_scene_mesh_drf_back );
    PROFILE_RESET( render_scene_mesh_ref );
    PROFILE_RESET( render_scene_mesh_ref_chr );
    PROFILE_RESET( render_scene_mesh_drf_solid );
    PROFILE_RESET( render_scene_mesh_render_shadows );

    stabilized_game_fps        = TARGET_FPS;
    stabilized_game_fps_sum    = STABILIZED_COVER * TARGET_FPS;
    stabilized_game_fps_weight = STABILIZED_COVER;
}

//--------------------------------------------------------------------------------------------
void gfx_system_release_all_graphics()
{
    gfx_init_bar_data();
    gfx_init_blip_data();
    gfx_init_map_data();

    BillboardList_free_all();
    TxTexture_release_all();
}

//--------------------------------------------------------------------------------------------
void gfx_system_delete_all_graphics()
{
    gfx_init_bar_data();
    gfx_init_blip_data();
    gfx_init_map_data();

    BillboardList_free_all();
    TxTexture_delete_all();
}

//--------------------------------------------------------------------------------------------
void gfx_system_load_basic_textures()
{
    /// @author ZZ
    /// @details This function loads the standard textures for a module

    // Particle sprites
    TxTexture_load_one_vfs( "mp_data/particle_trans", ( TX_REF )TX_PARTICLE_TRANS, TRANSCOLOR );
    prt_set_texture_params(( TX_REF )TX_PARTICLE_TRANS );

    TxTexture_load_one_vfs( "mp_data/particle_light", ( TX_REF )TX_PARTICLE_LIGHT, INVALID_KEY );
    prt_set_texture_params(( TX_REF )TX_PARTICLE_LIGHT );

    // Module background tiles
    TxTexture_load_one_vfs( "mp_data/tile0", ( TX_REF )TX_TILE_0, TRANSCOLOR );
    TxTexture_load_one_vfs( "mp_data/tile1", ( TX_REF )TX_TILE_1, TRANSCOLOR );
    TxTexture_load_one_vfs( "mp_data/tile2", ( TX_REF )TX_TILE_2, TRANSCOLOR );
    TxTexture_load_one_vfs( "mp_data/tile3", ( TX_REF )TX_TILE_3, TRANSCOLOR );

    // Water textures
    TxTexture_load_one_vfs( "mp_data/watertop", ( TX_REF )TX_WATER_TOP, TRANSCOLOR );
    TxTexture_load_one_vfs( "mp_data/waterlow", ( TX_REF )TX_WATER_LOW, TRANSCOLOR );

    // The phong map
    TxTexture_load_one_vfs( "mp_data/phong", ( TX_REF )TX_PHONG, TRANSCOLOR );

    PROFILE_RESET( render_scene_init );
    PROFILE_RESET( render_scene_mesh );
    PROFILE_RESET( render_scene_solid );
    PROFILE_RESET( render_scene_water );
    PROFILE_RESET( render_scene_trans );

    PROFILE_RESET( gfx_make_renderlist );
    PROFILE_RESET( gfx_make_dolist );
    PROFILE_RESET( do_grid_lighting );
    PROFILE_RESET( light_fans );
    PROFILE_RESET( gfx_update_all_chr_instance );
    PROFILE_RESET( update_all_prt_instance );

    PROFILE_RESET( render_scene_mesh_dolist_sort );
    PROFILE_RESET( render_scene_mesh_ndr );
    PROFILE_RESET( render_scene_mesh_drf_back );
    PROFILE_RESET( render_scene_mesh_ref );
    PROFILE_RESET( render_scene_mesh_ref_chr );
    PROFILE_RESET( render_scene_mesh_drf_solid );
    PROFILE_RESET( render_scene_mesh_render_shadows );

    stabilized_game_fps        = TARGET_FPS;
    stabilized_game_fps_sum    = STABILIZED_COVER * TARGET_FPS;
    stabilized_game_fps_weight = STABILIZED_COVER;
}

//--------------------------------------------------------------------------------------------
void gfx_system_make_enviro()
{
    /// @author ZZ
    /// @details This function sets up the environment mapping table
    int cnt;
    float x, y, z;

    // Find the environment map positions
    for ( cnt = 0; cnt < EGO_NORMAL_COUNT; cnt++ )
    {
        x = kMd2Normals[cnt][0];
        y = kMd2Normals[cnt][1];
        indextoenvirox[cnt] = ATAN2( y, x ) * INV_TWO_PI;
    }

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        z = cnt / INV_FF;  // Z is between 0 and 1
        lighttoenviroy[cnt] = z;
    }
}

//--------------------------------------------------------------------------------------------
void gfx_system_reload_all_textures()
{
    /// @author BB
    /// @details function is called when the graphics mode is changed or the program is
    /// restored from a minimized state. Otherwise, all OpenGL bitmaps return to a random state.

    //TxTitleImage_reload_all();
    TxTexture_reload_all();
    TxMenu_reload_all();
}

//--------------------------------------------------------------------------------------------
// 2D RENDERER FUNCTIONS
//--------------------------------------------------------------------------------------------
int debug_printf( const char *format, ... )
{
    va_list args;
    int retval;

    va_start( args, format );
    retval = _debug_vprintf( format, args );
    va_end( args );

    return retval;
}

//--------------------------------------------------------------------------------------------
void draw_blip( float sizeFactor, Uint8 color, float x, float y, bool_t mini_map )
{
    /// @author ZZ
    /// @details This function draws a single blip
    ego_frect_t tx_rect, sc_rect;

    float width, height;
    float loc_x, loc_y;

    //Adjust the position values so that they fit inside the minimap
    if ( mini_map )
    {
        loc_x = x * MAPSIZE / PMesh->gmem.edge_x;
        loc_y = ( y * MAPSIZE / PMesh->gmem.edge_y ) + sdl_scr.y - MAPSIZE;
    }
    else
    {
        loc_x = x;
        loc_y = y;
    }

    //Now draw it
    if ( loc_x > 0.0f && loc_y > 0.0f )
    {
        oglx_texture_t * ptex = TxTexture_get_valid_ptr(( TX_REF )TX_BLIP );

        tx_rect.xmin = ( float )bliprect[color].left   / ( float )oglx_texture_getTextureWidth( ptex );
        tx_rect.xmax = ( float )bliprect[color].right  / ( float )oglx_texture_getTextureWidth( ptex );
        tx_rect.ymin = ( float )bliprect[color].top    / ( float )oglx_texture_getTextureHeight( ptex );
        tx_rect.ymax = ( float )bliprect[color].bottom / ( float )oglx_texture_getTextureHeight( ptex );

        width  = sizeFactor * ( bliprect[color].right  - bliprect[color].left );
        height = sizeFactor * ( bliprect[color].bottom - bliprect[color].top );

        sc_rect.xmin = loc_x - ( width / 2 );
        sc_rect.xmax = loc_x + ( width / 2 );
        sc_rect.ymin = loc_y - ( height / 2 );
        sc_rect.ymax = loc_y + ( height / 2 );

        draw_quad_2d( ptex, sc_rect, tx_rect, btrue );
    }
}

//--------------------------------------------------------------------------------------------
float draw_icon_texture( oglx_texture_t * ptex, float x, float y, Uint8 sparkle_color, Uint32 sparkle_timer, float size )
{
    float       width, height;
    ego_frect_t tx_rect, sc_rect;

    if ( NULL == ptex )
    {
        // defaults
        tx_rect.xmin = 0.0f;
        tx_rect.xmax = 1.0f;
        tx_rect.ymin = 0.0f;
        tx_rect.ymax = 1.0f;
    }
    else
    {
        tx_rect.xmin = 0.0f;
        tx_rect.xmax = ( float )ptex->imgW / ( float )ptex->base.width;
        tx_rect.ymin = 0.0f;
        tx_rect.ymax = ( float )ptex->imgW / ( float )ptex->base.width;
    }

    width  = ICON_SIZE;
    height = ICON_SIZE;

    // handle non-default behavior
    if ( size >= 0.0f )
    {
        float factor_wid = ( float )size / width;
        float factor_hgt = ( float )size / height;
        float factor = MIN( factor_wid, factor_hgt );

        width *= factor;
        height *= factor;
    }

    sc_rect.xmin = x;
    sc_rect.xmax = x + width;
    sc_rect.ymin = y;
    sc_rect.ymax = y + height;

    draw_quad_2d( ptex, sc_rect, tx_rect, bfalse );

    if ( NOSPARKLE != sparkle_color )
    {
        int         position;
        float       loc_blip_x, loc_blip_y;

        position = sparkle_timer & SPARKLE_AND;

        loc_blip_x = x + position * ( width / SPARKLE_SIZE );
        loc_blip_y = y;
        draw_blip( 0.5f, sparkle_color, loc_blip_x, loc_blip_y, bfalse );

        loc_blip_x = x + width;
        loc_blip_y = y + position * ( height / SPARKLE_SIZE );
        draw_blip( 0.5f, sparkle_color, loc_blip_x, loc_blip_y, bfalse );

        loc_blip_x = loc_blip_x - position  * ( width / SPARKLE_SIZE );
        loc_blip_y = y + height;
        draw_blip( 0.5f, sparkle_color, loc_blip_x, loc_blip_y, bfalse );

        loc_blip_x = x;
        loc_blip_y = loc_blip_y - position * ( height / SPARKLE_SIZE );
        draw_blip( 0.5f, sparkle_color, loc_blip_x, loc_blip_y, bfalse );
    }

    return y + height;
}

//--------------------------------------------------------------------------------------------
float draw_game_icon( const TX_REF icontype, float x, float y, Uint8 sparkle_color, Uint32 sparkle_timer, float size )
{
    /// @author ZZ
    /// @details This function draws an icon

    return draw_icon_texture( TxTexture_get_valid_ptr( icontype ), x, y, sparkle_color, sparkle_timer, size );
}

//--------------------------------------------------------------------------------------------
float draw_menu_icon( const TX_REF icontype, float x, float y, Uint8 sparkle_color, Uint32 sparkle_timer, float size )
{
    return draw_icon_texture( TxMenu_get_valid_ptr( icontype ), x, y, sparkle_color, sparkle_timer, size );
}

//--------------------------------------------------------------------------------------------
void draw_one_font( oglx_texture_t * ptex, int fonttype, float x_stt, float y_stt )
{
    /// @author GAC
    /// @details Very nasty version for starters.  Lots of room for improvement.
    /// @author ZZ
    /// @details This function draws a letter or number

    GLfloat dx, dy, border;

    ego_frect_t tx_rect, sc_rect;

    sc_rect.xmin  = x_stt;
    sc_rect.xmax  = x_stt + fontrect[fonttype].w;
    sc_rect.ymin  = y_stt + fontoffset - fontrect[fonttype].h;
    sc_rect.ymax  = y_stt + fontoffset;

    dx = 2.0f / 512.0f;
    dy = 1.0f / 256.0f;
    border = 1.0f / 512.0f;

    tx_rect.xmin = fontrect[fonttype].x * dx;
    tx_rect.xmax = tx_rect.xmin + fontrect[fonttype].w * dx;
    tx_rect.ymin = fontrect[fonttype].y * dy;
    tx_rect.ymax = tx_rect.ymin + fontrect[fonttype].h * dy;

    // shrink the texture size slightly
    tx_rect.xmin += border;
    tx_rect.xmax -= border;
    tx_rect.ymin += border;
    tx_rect.ymax -= border;

    draw_quad_2d( ptex, sc_rect, tx_rect, btrue );
}

//--------------------------------------------------------------------------------------------
void draw_map_texture( float x, float y )
{
    /// @author ZZ
    /// @details This function draws the map

    ego_frect_t sc_rect, tx_rect;

    oglx_texture_t * ptex = TxTexture_get_valid_ptr(( TX_REF )TX_MAP );
    if ( NULL == ptex ) return;

    sc_rect.xmin = x;
    sc_rect.xmax = x + MAPSIZE;
    sc_rect.ymin = y;
    sc_rect.ymax = y + MAPSIZE;

    tx_rect.xmin = 0;
    tx_rect.xmax = ( float )ptex->imgW / ( float )ptex->base.width;
    tx_rect.ymin = 0;
    tx_rect.ymax = ( float )ptex->imgH / ( float )ptex->base.height;

    draw_quad_2d( ptex, sc_rect, tx_rect, bfalse );
}

//--------------------------------------------------------------------------------------------
float draw_one_xp_bar( float x, float y, Uint8 ticks )
{
    /// @author ZF
    /// @details This function draws a xp bar and returns the y position for the next one

    int width, height;
    Uint8 cnt;
    ego_frect_t tx_rect, sc_rect;

    ticks = MIN( ticks, NUMTICK );

    gfx_enable_texturing();               // Enable texture mapping
    GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

    //---- Draw the tab (always colored)

    width = 16;
    height = XPTICK;

    tx_rect.xmin = 0;
    tx_rect.xmax = 32.00f / 128;
    tx_rect.ymin = XPTICK / 16;
    tx_rect.ymax = XPTICK * 2 / 16;

    sc_rect.xmin = x;
    sc_rect.xmax = x + width;
    sc_rect.ymin = y;
    sc_rect.ymax = y + height;

    draw_quad_2d( TxTexture_get_valid_ptr(( TX_REF )TX_XP_BAR ), sc_rect, tx_rect, btrue );

    x += width;

    //---- Draw the filled ones
    tx_rect.xmin = 0.0f;
    tx_rect.xmax = 32 / 128.0f;
    tx_rect.ymin = XPTICK / 16.0f;
    tx_rect.ymax = 2 * XPTICK / 16.0f;

    width  = XPTICK;
    height = XPTICK;

    for ( cnt = 0; cnt < ticks; cnt++ )
    {
        sc_rect.xmin = x + ( cnt * width );
        sc_rect.xmax = x + ( cnt * width ) + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d( TxTexture_get_valid_ptr(( TX_REF )TX_XP_BAR ), sc_rect, tx_rect, btrue );
    }

    //---- Draw the remaining empty ones
    tx_rect.xmin = 0;
    tx_rect.xmax = 32 / 128.0f;
    tx_rect.ymin = 0;
    tx_rect.ymax = XPTICK / 16.0f;

    for ( /*nothing*/; cnt < NUMTICK; cnt++ )
    {
        sc_rect.xmin = x + ( cnt * width );
        sc_rect.xmax = x + ( cnt * width ) + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d( TxTexture_get_valid_ptr(( TX_REF )TX_XP_BAR ), sc_rect, tx_rect, btrue );
    }

    return y + height;
}

//--------------------------------------------------------------------------------------------
float draw_one_bar( Uint8 bartype, float x_stt, float y_stt, int ticks, int maxticks )
{
    /// @author ZZ
    /// @details This function draws a bar and returns the y position for the next one

    const float scale = 1.0f;

    float       width, height;
    ego_frect_t tx_rect, sc_rect;
    oglx_texture_t * tx_ptr;

    float tx_width, tx_height, img_width;
    float tab_width, tick_width, tick_height;

    int total_ticks = maxticks;
    int tmp_bartype = bartype;

    float x_left = x_stt;
    float x = x_stt;
    float y = y_stt;

    if ( maxticks <= 0 || ticks < 0 || bartype > NUMBAR ) return y;

    // limit the values to reasonable ones
    if ( total_ticks > MAXTICK ) total_ticks = MAXTICK;
    if ( ticks       > total_ticks ) ticks = total_ticks;

    // grab a pointer to the bar texture
    tx_ptr = TxTexture_get_valid_ptr(( TX_REF )TX_BARS );

    // allow the bitmap to be scaled to arbitrary size
    tx_width   = 128.0f;
    tx_height  = 128.0f;
    img_width  = 112.0f;
    if ( NULL != tx_ptr )
    {
        tx_width  = tx_ptr->base.width;
        tx_height = tx_ptr->base.height;
        img_width = tx_ptr->imgW;
    }

    // calculate the bar parameters
    tick_width  = img_width / 14.0f;
    tick_height = img_width / 7.0f;
    tab_width   = img_width / 3.5f;

    //---- Draw the tab
    tmp_bartype = bartype;

    tx_rect.xmin  = 0.0f       / tx_width;
    tx_rect.xmax  = tab_width  / tx_width;
    tx_rect.ymin  = tick_height * ( tmp_bartype + 0 ) / tx_height;
    tx_rect.ymax  = tick_height * ( tmp_bartype + 1 ) / tx_height;

    width  = ( tx_rect.xmax - tx_rect.xmin ) * scale * tx_width;
    height = ( tx_rect.ymax - tx_rect.ymin ) * scale * tx_height;

    sc_rect.xmin = x;
    sc_rect.xmax = x + width;
    sc_rect.ymin = y;
    sc_rect.ymax = y + height;

    draw_quad_2d( tx_ptr, sc_rect, tx_rect, btrue );

    // make the new left-hand margin after the tab
    x_left = x_stt + width;
    x      = x_left;

    //---- Draw the full rows of ticks
    while ( ticks >= NUMTICK )
    {
        tmp_bartype = bartype;

        tx_rect.xmin  = tab_width  / tx_width;
        tx_rect.xmax  = img_width  / tx_width;
        tx_rect.ymin  = tick_height * ( tmp_bartype + 0 ) / tx_height;
        tx_rect.ymax  = tick_height * ( tmp_bartype + 1 ) / tx_height;

        width  = ( tx_rect.xmax - tx_rect.xmin ) * scale * tx_width;
        height = ( tx_rect.ymax - tx_rect.ymin ) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d( tx_ptr, sc_rect, tx_rect, btrue );

        y += height;
        ticks -= NUMTICK;
        total_ticks -= NUMTICK;
    }

    if ( ticks > 0 )
    {
        int full_ticks = NUMTICK - ticks;
        int empty_ticks = NUMTICK - ( MIN( NUMTICK, total_ticks ) - ticks );

        //---- draw a partial row of full ticks
        tx_rect.xmin  = tab_width  / tx_width;
        tx_rect.xmax  = ( img_width - tick_width * full_ticks )  / tx_width;
        tx_rect.ymin  = tick_height * ( tmp_bartype + 0 ) / tx_height;
        tx_rect.ymax  = tick_height * ( tmp_bartype + 1 ) / tx_height;

        width  = ( tx_rect.xmax - tx_rect.xmin ) * scale * tx_width;
        height = ( tx_rect.ymax - tx_rect.ymin ) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d( tx_ptr, sc_rect, tx_rect, btrue );

        // move to the right after drawing the full ticks
        x += width;

        //---- draw a partial row of empty ticks
        tmp_bartype = 0;

        tx_rect.xmin  = tab_width  / tx_width;
        tx_rect.xmax  = ( img_width - tick_width * empty_ticks )  / tx_width;
        tx_rect.ymin  = tick_height * ( tmp_bartype + 0 ) / tx_height;
        tx_rect.ymax  = tick_height * ( tmp_bartype + 1 ) / tx_height;

        width  = ( tx_rect.xmax - tx_rect.xmin ) * scale * tx_width;
        height = ( tx_rect.ymax - tx_rect.ymin ) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d( tx_ptr, sc_rect, tx_rect, btrue );

        y += height;
        ticks = 0;
        total_ticks -= NUMTICK;
    }

    // reset the x position
    x = x_left;

    // Draw full rows of empty ticks
    while ( total_ticks >= NUMTICK )
    {
        tmp_bartype = 0;

        tx_rect.xmin  = tab_width  / tx_width;
        tx_rect.xmax  = img_width  / tx_width;
        tx_rect.ymin  = tick_height * ( tmp_bartype + 0 ) / tx_height;
        tx_rect.ymax  = tick_height * ( tmp_bartype + 1 ) / tx_height;

        width  = ( tx_rect.xmax - tx_rect.xmin ) * scale * tx_width;
        height = ( tx_rect.ymax - tx_rect.ymin ) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d( tx_ptr, sc_rect, tx_rect, btrue );

        y += height;
        total_ticks -= NUMTICK;
    }

    // Draw the last of the empty ones
    if ( total_ticks > 0 )
    {
        int remaining = NUMTICK - total_ticks;

        //---- draw a partial row of empty ticks
        tmp_bartype = 0;

        tx_rect.xmin  = tab_width  / tx_width;
        tx_rect.xmax  = ( img_width - tick_width * remaining )  / tx_width;
        tx_rect.ymin  = tick_height * ( tmp_bartype + 0 ) / tx_height;
        tx_rect.ymax  = tick_height * ( tmp_bartype + 1 ) / tx_height;

        width  = ( tx_rect.xmax - tx_rect.xmin ) * scale * tx_width;
        height = ( tx_rect.ymax - tx_rect.ymin ) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d( tx_ptr, sc_rect, tx_rect, btrue );

        y += height;
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_string( float x, float y, const char *format, ... )
{
    /// @author ZZ
    /// @details This function spits a line of null terminated text onto the backbuffer
    ///
    /// details BB@> Uses gfx_begin_2d() ... gfx_end_2d() so that the function can basically be called from anywhere
    ///    The way they are currently implemented, this breaks the icon drawing in draw_status() if
    ///    you use draw_string() and then draw_icon(). Use _draw_string_raw(), instead.

    va_list args;

    gfx_begin_2d();
    {
        va_start( args, format );
        y = _va_draw_string( x, y, format, args );
        va_end( args );
    }
    gfx_end_2d();

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_wrap_string( const char *szText, float x, float y, int maxx )
{
    /// @author ZZ
    /// @details This function spits a line of null terminated text onto the backbuffer,
    ///    wrapping over the right side and returning the new y value

    int stt_x = x;
    Uint8 cTmp = szText[0];
    int newy = y + fontyspacing;
    Uint8 newword = btrue;
    int cnt = 1;

    oglx_texture_t * tx_ptr = TxMenu_get_valid_ptr(( TX_REF )MENU_FONT_BMP );
    if ( NULL == tx_ptr ) return y;

    gfx_begin_text();

    maxx = maxx + stt_x;

    while ( CSTR_END != cTmp )
    {
        // Check each new word for wrapping
        if ( newword )
        {
            int endx = x + font_bmp_length_of_word( szText + cnt - 1 );

            newword = bfalse;
            if ( endx > maxx )
            {
                // Wrap the end and cut off spaces and tabs
                x = stt_x + fontyspacing;
                y += fontyspacing;
                newy += fontyspacing;

                while ( ' ' == cTmp || '~' == cTmp )
                {
                    cTmp = szText[cnt];
                    cnt++;
                }
            }
        }
        else
        {
            Uint8 iTmp;

            if ( '~' == cTmp )
            {
                // Use squiggle for tab
                x = ( FLOOR(( float )x / ( float )TABADD ) + 1.0f ) * TABADD;
            }
            else if ( C_NEW_LINE_CHAR == cTmp )
            {
                x = stt_x;
                y += fontyspacing;
                newy += fontyspacing;
            }
            else if ( isspace( cTmp ) )
            {
                // other whitespace
                iTmp = asciitofont[cTmp];
                x += fontxspacing[iTmp] / 2;
            }
            else
            {
                // Normal letter
                iTmp = asciitofont[cTmp];
                draw_one_font( tx_ptr, iTmp, x, y );
                x += fontxspacing[iTmp];
            }

            cTmp = szText[cnt];
            cnt++;

            if ( '~' == cTmp || C_NEW_LINE_CHAR == cTmp || C_CARRIAGE_RETURN_CHAR == cTmp || isspace( cTmp ) )
            {
                newword = btrue;
            }
        }
    }

    gfx_end_text();
    return newy;
}

//--------------------------------------------------------------------------------------------
void draw_one_character_icon( const CHR_REF item, float x, float y, bool_t draw_ammo, Uint8 draw_sparkle )
{
    /// @author BB
    /// @details Draw an icon for the given item at the position <x,y>.
    ///     If the object is invalid, draw the null icon instead of failing
    ///     If NOSPARKLE is specified the default item sparkle will be used (default behaviour)

    TX_REF icon_ref;

    chr_t * pitem = !INGAME_CHR( item ) ? NULL : ChrList_get_ptr( item );

    // grab the icon reference
    icon_ref = chr_get_txtexture_icon_ref( item );

    // draw the icon
    if ( draw_sparkle == NOSPARKLE ) draw_sparkle = ( NULL == pitem ) ? NOSPARKLE : pitem->sparkle;
    draw_game_icon( icon_ref, x, y, draw_sparkle, update_wld, -1 );

    // draw the ammo, if requested
    if ( draw_ammo && ( NULL != pitem ) )
    {
        if ( 0 != pitem->ammomax && pitem->ammoknown )
        {
            cap_t * pitem_cap = chr_get_pcap( item );

            if (( NULL != pitem_cap && !pitem_cap->isstackable ) || pitem->ammo > 1 )
            {
                // Show amount of ammo left
                _draw_string_raw( x, y - 8, "%2d", pitem->ammo );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
float draw_character_xp_bar( const CHR_REF character, float x, float y )
{
    chr_t * pchr;
    cap_t * pcap;

    if ( !INGAME_CHR( character ) ) return y;
    pchr = ChrList_get_ptr( character );

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return y;

    //Draw the small XP progress bar
    if ( pchr->experiencelevel < MAXLEVEL )
    {
        Uint8  curlevel    = pchr->experiencelevel + 1;
        Uint32 xplastlevel = pcap->experience_forlevel[curlevel-1];
        Uint32 xpneed      = pcap->experience_forlevel[curlevel];

        float fraction = ( float )MAX( pchr->experience - xplastlevel, 0 ) / ( float )MAX( xpneed - xplastlevel, 1 );
        int   numticks = fraction * NUMTICK;

        y = draw_one_xp_bar( x, y, numticks );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_status( const CHR_REF character, float x, float y )
{
    /// @author ZZ
    /// @details This function shows a character's icon, status and inventory
    ///    The x,y coordinates are the top left point of the image to draw

    STRING generictext;
    int life_pips, life_pips_max;
    int mana_pips, mana_pips_max;

    chr_t * pchr;
    cap_t * pcap;

    if ( !INGAME_CHR( character ) ) return y;
    pchr = ChrList_get_ptr( character );

    pcap = chr_get_pcap( character );
    if ( NULL == pcap ) return y;

    life_pips      = SFP8_TO_SINT( pchr->life );
    life_pips_max  = SFP8_TO_SINT( pchr->life_max );
    mana_pips      = SFP8_TO_SINT( pchr->mana );
    mana_pips_max  = SFP8_TO_SINT( pchr->mana_max );

    // make a short name for the actual display
    chr_get_name( character, CHRNAME_CAPITAL, generictext, 7 );
    generictext[7] = CSTR_END;

    // draw the name
    y = _draw_string_raw( x + 8, y, generictext );

    // draw the character's money
    y = _draw_string_raw( x + 8, y, "$%4d", pchr->money ) + 8;

    // draw the character's main icon
    draw_one_character_icon( character, x + 40, y, bfalse, NOSPARKLE );

    // draw the left hand item icon
    draw_one_character_icon( pchr->holdingwhich[SLOT_LEFT], x + 8, y, btrue, NOSPARKLE );

    // draw the right hand item icon
    draw_one_character_icon( pchr->holdingwhich[SLOT_RIGHT], x + 72, y, btrue, NOSPARKLE );

    // skip to the next row
    y += 32;

    //Draw the small XP progress bar
    y = draw_character_xp_bar( character, x + 16, y );

    // Draw the life_pips bar
    if ( pchr->alive )
    {
        y = draw_one_bar( pchr->life_color, x, y, life_pips, life_pips_max );
    }
    else
    {
        y = draw_one_bar( 0, x, y, 0, life_pips_max );  // Draw a black bar
    }

    // Draw the mana_pips bar
    if ( mana_pips_max > 0 )
    {
        y = draw_one_bar( pchr->mana_color, x, y, mana_pips, mana_pips_max );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
void draw_all_status()
{
    int cnt, tnc;
    int y;
    ego_frect_t screen;
    ext_camera_list_t * pclst;

    if ( !StatusList.on ) return;

    // connect each status object with its camera
    status_list_update_cameras( &StatusList );

    // get the camera list
    pclst = camera_system_get_list();

    for ( cnt = 0; cnt < MAX_CAMERAS; cnt++ )
    {
        // find the camera
        ext_camera_t * pext = camera_list_get_ext_camera_index( pclst, cnt );
        if ( NULL == pext ) continue;

        // grab the screen
        if ( !ext_camera_get_screen( pext, &screen ) ) continue;

        // draw all attached status
        y = screen.ymin;
        for ( tnc = 0; tnc < StatusList.count; tnc++ )
        {
            status_list_element_t * pelem = StatusList.lst + tnc;

            if ( cnt == pelem->camera_index )
            {
                y = draw_status( pelem->who, screen.xmax - BARX, y );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void draw_map()
{
    size_t cnt;

    // Map display
    if ( !mapvalid || !mapon ) return;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );
    {

        GL_DEBUG( glEnable )( GL_BLEND );                               // GL_COLOR_BUFFER_BIT
        GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

        GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f );
        draw_map_texture( 0, sdl_scr.y - MAPSIZE );

        GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

        // If one of the players can sense enemies via ESP, draw them as blips on the map
        if ( TEAM_MAX != local_stats.sense_enemies_team )
        {
            CHR_REF ichr;

            for ( ichr = 0; ichr < MAX_CHR && blip_count < MAXBLIP; ichr++ )
            {
                chr_t * pchr;
                cap_t * pcap;

                if ( !INGAME_CHR( ichr ) ) continue;
                pchr = ChrList_get_ptr( ichr );

                pcap = chr_get_pcap( ichr );
                if ( NULL == pcap ) continue;

                // Show only teams that will attack the player
                if ( team_hates_team( pchr->team, local_stats.sense_enemies_team ) )
                {
                    // Only if they match the required IDSZ ([NONE] always works)
                    if ( local_stats.sense_enemies_idsz == IDSZ_NONE ||
                         local_stats.sense_enemies_idsz == pcap->idsz[IDSZ_PARENT] ||
                         local_stats.sense_enemies_idsz == pcap->idsz[IDSZ_TYPE  ] )
                    {
                        // Inside the map?
                        if ( pchr->pos.x < PMesh->gmem.edge_x && pchr->pos.y < PMesh->gmem.edge_y )
                        {
                            // Valid colors only
                            blip_x[blip_count] = pchr->pos.x;
                            blip_y[blip_count] = pchr->pos.y;
                            blip_c[blip_count] = COLOR_RED; // Red blips
                            blip_count++;
                        }
                    }
                }
            }
        }

        // draw all the blips
        for ( cnt = 0; cnt < blip_count; cnt++ )
        {
            draw_blip( 0.75f, blip_c[cnt], blip_x[cnt], blip_y[cnt], btrue );
        }
        blip_count = 0;

        // Show local player position(s)
        if ( youarehereon && ( update_wld & 8 ) )
        {
            PLA_REF iplayer;
            for ( iplayer = 0; iplayer < MAX_PLAYER; iplayer++ )
            {
                if ( !PlaStack.lst[iplayer].valid ) continue;

                if ( NULL == PlaStack.lst[iplayer].pdevice )
                {
                    CHR_REF ichr = PlaStack.lst[iplayer].index;
                    if ( INGAME_CHR( ichr ) && ChrList.lst[ichr].alive )
                    {
                        draw_blip( 0.75f, COLOR_WHITE, ChrList.lst[ichr].pos.x, ChrList.lst[ichr].pos.y, btrue );
                    }
                }
            }
        }

        // draw the camera(s)
        //if ( update_wld & 2 )
        //{
        //    ext_camera_iterator_t * it;

        //    for( it = camera_list_iterator_begin(); NULL != it; it = camera_list_iterator_next( it ) )
        //    {
        //        fvec3_t tmp_diff;

        //        camera_t * pcam = camera_list_iterator_get_camera(it);
        //        if( NULL == pcam ) continue;

        //        draw_blip( 0.75f, COLOR_PURPLE, pcam->pos.x, pcam->pos.y, btrue );
        //    }
        //    it = camera_list_iterator_end(it);
        //}
    }
    ATTRIB_POP( __FUNCTION__ )
}

//--------------------------------------------------------------------------------------------
float draw_fps( float y )
{
    // FPS text

    parser_state_t * ps = script_compiler_get_state();

    if ( outofsync )
    {
        y = _draw_string_raw( 0, y, "OUT OF SYNC" );
    }

    if ( script_compiler_error( ps ) )
    {
        y = _draw_string_raw( 0, y, "SCRIPT ERROR ( see \"/debug/log.txt\" )" );
    }

    if ( fpson )
    {
        y = _draw_string_raw( 0, y, "%2.3f FPS, %2.3f UPS, Update lag = %d", stabilized_game_fps, stabilized_game_ups, update_lag );

        //Extra debug info
        if ( cfg.dev_mode )
        {

#    if defined(DEBUG_BSP)
            y = _draw_string_raw( 0, y, "BSP chr %d/%d - BSP prt %d/%d", chr_BSP_root.count, MAX_CHR - ChrList_count_free(), prt_BSP_root.count, maxparticles - PrtList_count_free() );
            y = _draw_string_raw( 0, y, "BSP infinite %d", chr_BSP_root.tree.infinite.count + prt_BSP_root.tree.infinite.count );
            y = _draw_string_raw( 0, y, "BSP collisions %d", CHashList_inserted );
            //y = _draw_string_raw( 0, y, "chr-mesh tests %04d - prt-mesh tests %04d", chr_stoppedby_tests + chr_pressure_tests, prt_stoppedby_tests + prt_pressure_tests );
#    endif

#if defined(DEBUG_RENDERLIST)
            y = _draw_string_raw( 0, y, "Renderlist tiles %d/%d", renderlist.all_count, PMesh->info.tiles_count );
#endif

#if defined(_DEBUG)

#    if defined(DEBUG_PROFILE_DISPLAY) && defined(_DEBUG)

#        if defined(DEBUG_PROFILE_RENDER) && defined(_DEBUG)
            y = _draw_string_raw( 0, y, "estimated max FPS %2.3f UPS %4.2f GFX %4.2f", est_max_fps, est_max_ups, est_max_gfx );
            y = _draw_string_raw( 0, y, "gfx:total %2.4f, render:total %2.4f", est_render_time, time_draw_scene );
            y = _draw_string_raw( 0, y, "render:init %2.4f,  render:mesh %2.4f", time_render_scene_init, time_render_scene_mesh );
            y = _draw_string_raw( 0, y, "render:solid %2.4f, render:water %2.4f", time_render_scene_solid, time_render_scene_water );
            y = _draw_string_raw( 0, y, "render:trans %2.4f", time_render_scene_trans );
#        endif

#        if defined(DEBUG_PROFILE_MESH) && defined(_DEBUG)
            y = _draw_string_raw( 0, y, "mesh:total %2.4f", time_render_scene_mesh );
            y = _draw_string_raw( 0, y, "mesh:dolist_sort %2.4f, mesh:ndr %2.4f", time_render_scene_mesh_dolist_sort , time_render_scene_mesh_ndr );
            y = _draw_string_raw( 0, y, "mesh:drf_back %2.4f, mesh:ref %2.4f", time_render_scene_mesh_drf_back, time_render_scene_mesh_ref );
            y = _draw_string_raw( 0, y, "mesh:ref_chr %2.4f, mesh:drf_solid %2.4f", time_render_scene_mesh_ref_chr, time_render_scene_mesh_drf_solid );
            y = _draw_string_raw( 0, y, "mesh:render_shadows %2.4f", time_render_scene_mesh_render_shadows );
#        endif

#        if defined(DEBUG_PROFILE_INIT) && defined(_DEBUG)
            y = _draw_string_raw( 0, y, "init:total %2.4f", time_render_scene_init );
            y = _draw_string_raw( 0, y, "init:gfx_make_renderlist %2.4f, init:gfx_make_dolist %2.4f", time_render_scene_init_renderlist_make, time_render_scene_init_dolist_make );
            y = _draw_string_raw( 0, y, "init:do_grid_lighting %2.4f, init:light_fans %2.4f", time_render_scene_init_do_grid_dynalight, time_render_scene_init_light_fans );
            y = _draw_string_raw( 0, y, "init:gfx_update_all_chr_instance %2.4f", time_render_scene_init_update_all_chr_instance );
            y = _draw_string_raw( 0, y, "init:update_all_prt_instance %2.4f", time_render_scene_init_update_all_prt_instance );
#        endif

#    endif
#endif
        }
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_help( float y )
{
    if ( SDLKEYDOWN( SDLK_F1 ) )
    {
        // In-Game help
        y = _draw_string_raw( 0, y, "!!!MOUSE HELP!!!" );
        y = _draw_string_raw( 0, y, "~~Go to input settings to change" );
        y = _draw_string_raw( 0, y, "Default settings" );
        y = _draw_string_raw( 0, y, "~~Left Click to use an item" );
        y = _draw_string_raw( 0, y, "~~Left and Right Click to grab" );
        y = _draw_string_raw( 0, y, "~~Middle Click to jump" );
        y = _draw_string_raw( 0, y, "~~A and S keys do stuff" );
        y = _draw_string_raw( 0, y, "~~Right Drag to move camera" );
    }
    if ( SDLKEYDOWN( SDLK_F2 ) )
    {
        // In-Game help
        y = _draw_string_raw( 0, y, "!!!JOYSTICK HELP!!!" );
        y = _draw_string_raw( 0, y, "~~Go to input settings to change." );
        y = _draw_string_raw( 0, y, "~~Hit the buttons" );
        y = _draw_string_raw( 0, y, "~~You'll figure it out" );
    }
    if ( SDLKEYDOWN( SDLK_F3 ) )
    {
        // In-Game help
        y = _draw_string_raw( 0, y, "!!!KEYBOARD HELP!!!" );
        y = _draw_string_raw( 0, y, "~~Go to input settings to change." );
        y = _draw_string_raw( 0, y, "Default settings" );
        y = _draw_string_raw( 0, y, "~~TGB control left hand" );
        y = _draw_string_raw( 0, y, "~~YHN control right hand" );
        y = _draw_string_raw( 0, y, "~~Keypad to move and jump" );
        y = _draw_string_raw( 0, y, "~~Number keys for stats" );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_debug( float y )
{
    if ( !cfg.dev_mode ) return y;

    if ( SDLKEYDOWN( SDLK_F5 ) )
    {
        CHR_REF ichr;
        PLA_REF ipla;

        // Debug information
        y = _draw_string_raw( 0, y, "!!!DEBUG MODE-5!!!" );
        y = _draw_string_raw( 0, y, "~~CAM %f %f %f", PCamera->pos.x, PCamera->pos.y, PCamera->pos.z );
        ipla = ( PLA_REF )0;
        ichr = PlaStack.lst[ipla].index;
        y = _draw_string_raw( 0, y, "~~PLA0DEF %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f",
                              ChrList.lst[ichr].damage_resistance[DAMAGE_SLASH],
                              ChrList.lst[ichr].damage_resistance[DAMAGE_CRUSH],
                              ChrList.lst[ichr].damage_resistance[DAMAGE_POKE ],
                              ChrList.lst[ichr].damage_resistance[DAMAGE_HOLY ],
                              ChrList.lst[ichr].damage_resistance[DAMAGE_EVIL ],
                              ChrList.lst[ichr].damage_resistance[DAMAGE_FIRE ],
                              ChrList.lst[ichr].damage_resistance[DAMAGE_ICE  ],
                              ChrList.lst[ichr].damage_resistance[DAMAGE_ZAP  ] );

        ichr = PlaStack.lst[ipla].index;
        y = _draw_string_raw( 0, y, "~~PLA0 %5.1f %5.1f", ChrList.lst[ichr].pos.x / GRID_FSIZE, ChrList.lst[ichr].pos.y / GRID_FSIZE );

        ipla = ( PLA_REF )1;
        ichr = PlaStack.lst[ipla].index;
        y = _draw_string_raw( 0, y, "~~PLA1 %5.1f %5.1f", ChrList.lst[ichr].pos.x / GRID_FSIZE, ChrList.lst[ichr].pos.y / GRID_FSIZE );
    }

    if ( SDLKEYDOWN( SDLK_F6 ) )
    {
        // More debug information
        STRING text;

        y = _draw_string_raw( 0, y, "!!!DEBUG MODE-6!!!" );
        y = _draw_string_raw( 0, y, "~~FREEPRT %d", PrtList_count_free() );
        y = _draw_string_raw( 0, y, "~~FREECHR %d", ChrList_count_free() );
        y = _draw_string_raw( 0, y, "~~MACHINE %d", egonet_get_local_machine() );
        if ( PMod->exportvalid ) snprintf( text, SDL_arraysize( text ), "~~EXPORT: TRUE" );
        else                    snprintf( text, SDL_arraysize( text ), "~~EXPORT: FALSE" );
        y = _draw_string_raw( 0, y, text, PMod->exportvalid );
        y = _draw_string_raw( 0, y, "~~PASS %d/%d", ShopStack.count, PassageStack.count );
        y = _draw_string_raw( 0, y, "~~NETPLAYERS %d", egonet_get_client_count() );
        y = _draw_string_raw( 0, y, "~~DAMAGEPART %d", damagetile.part_gpip );

        // y = _draw_string_raw( 0, y, "~~FOGAFF %d", fog_data.affects_water );
    }

    if ( SDLKEYDOWN( SDLK_F7 ) )
    {
        // White debug mode
        y = _draw_string_raw( 0, y, "!!!DEBUG MODE-7!!!" );
        y = _draw_string_raw( 0, y, "CAM <%f, %f, %f, %f>", PCamera->mView.CNV( 0, 0 ), PCamera->mView.CNV( 1, 0 ), PCamera->mView.CNV( 2, 0 ), PCamera->mView.CNV( 3, 0 ) );
        y = _draw_string_raw( 0, y, "CAM <%f, %f, %f, %f>", PCamera->mView.CNV( 0, 1 ), PCamera->mView.CNV( 1, 1 ), PCamera->mView.CNV( 2, 1 ), PCamera->mView.CNV( 3, 1 ) );
        y = _draw_string_raw( 0, y, "CAM <%f, %f, %f, %f>", PCamera->mView.CNV( 0, 2 ), PCamera->mView.CNV( 1, 2 ), PCamera->mView.CNV( 2, 2 ), PCamera->mView.CNV( 3, 2 ) );
        y = _draw_string_raw( 0, y, "CAM <%f, %f, %f, %f>", PCamera->mView.CNV( 0, 3 ), PCamera->mView.CNV( 1, 3 ), PCamera->mView.CNV( 2, 3 ), PCamera->mView.CNV( 3, 3 ) );
        y = _draw_string_raw( 0, y, "CAM center <%f, %f>", PCamera->center.x, PCamera->center.y );
        y = _draw_string_raw( 0, y, "CAM turn %d %d", PCamera->turn_mode, PCamera->turn_time );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_timer( float y )
{
    int fifties, seconds, minutes;

    if ( timeron )
    {
        fifties = ( timervalue % 50 ) << 1;
        seconds = (( timervalue / 50 ) % 60 );
        minutes = ( timervalue / 3000 );
        y = _draw_string_raw( 0, y, "=%d:%02d:%02d=", minutes, seconds, fifties );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_game_status( float y )
{

    if ( egonet_get_waitingforclients() )
    {
        y = _draw_string_raw( 0, y, "Waiting for players... " );
    }
    else if ( ServerState.player_count > 0 )
    {
        if ( local_stats.allpladead || PMod->respawnanytime )
        {
            if ( PMod->respawnvalid && cfg.difficulty < GAME_HARD )
            {
                y = _draw_string_raw( 0, y, "PRESS SPACE TO RESPAWN" );
            }
            else
            {
                y = _draw_string_raw( 0, y, "PRESS ESCAPE TO QUIT" );
            }
        }
        else if ( PMod->beat )
        {
            y = _draw_string_raw( 0, y, "VICTORY!  PRESS ESCAPE" );
        }
    }
    else
    {
        y = _draw_string_raw( 0, y, "ERROR: MISSING PLAYERS" );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_messages( float y )
{
    int cnt, tnc;

    // Messages
    if ( messageon )
    {
        // Display the messages
        tnc = DisplayMsg.count;
        for ( cnt = 0; cnt < maxmessage; cnt++ )
        {
            if ( DisplayMsg.ary[tnc].time > 0 )
            {
                y = draw_wrap_string( DisplayMsg.ary[tnc].textdisplay, 0, y, sdl_scr.x - wraptolerance );
                if ( DisplayMsg.ary[tnc].time > msgtimechange )
                {
                    DisplayMsg.ary[tnc].time -= msgtimechange;
                }
                else
                {
                    DisplayMsg.ary[tnc].time = 0;
                }
            }

            tnc = ( tnc + 1 ) % maxmessage;
        }

        msgtimechange = 0;
    }

    return y;
}

//--------------------------------------------------------------------------------------------
void draw_hud()
{
    /// @author ZZ
    /// @details draw in-game heads up display

    int y;

    gfx_begin_2d();
    {
        draw_map();

        draw_inventory();

        draw_all_status();

        y = draw_fps( 0 );
        y = draw_help( y );
        y = draw_debug( y );
        y = draw_timer( y );
        y = draw_game_status( y );

        // Network message input
        if ( keyb.chat_mode )
        {
            char buffer[CHAT_BUFFER_SIZE + 128] = EMPTY_CSTR;

            snprintf( buffer, SDL_arraysize( buffer ), "%s > %s%s", cfg.network_messagename, net_chat.buffer, HAS_NO_BITS( update_wld, 8 ) ? "x" : "+" );

            y = draw_wrap_string( buffer, 0, y, sdl_scr.x - wraptolerance );
        }

        y = draw_messages( y );
    }
    gfx_end_2d();
}

//--------------------------------------------------------------------------------------------
void draw_inventory()
{
    //ZF> This renders the open inventories of all local players
    PLA_REF ipla;
    player_t * ppla;
    GLXvector4f background_color = { 0.66f, 0.0f, 0.0f, 0.95f };

    CHR_REF ichr;
    chr_t *pchr;

    PLA_REF draw_list[MAX_LOCAL_PLAYERS];
    size_t cnt, draw_list_length = 0;

    float sttx, stty;
    int width, height;

    static int lerp_time[MAX_LOCAL_PLAYERS] = { 0 };

    //don't draw inventories while menu is open
    if ( GProc->mod_paused ) return;

    //figure out what we have to draw
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        //valid player?
        ppla = PlaStack_get_ptr( ipla );
        if ( !ppla->valid ) continue;

        //draw inventory?
        if ( !ppla->draw_inventory ) continue;
        ichr = ppla->index;

        //valid character?
        if ( !INGAME_CHR( ichr ) ) continue;
        pchr = ChrList_get_ptr( ichr );

        //don't draw inventories of network players
        if ( !pchr->islocalplayer ) continue;

        draw_list[draw_list_length++] = ipla;
    }

    //figure out size and position of the inventory
    width = 180;
    height = 140;

    sttx = 0;
    stty = GFX_HEIGHT / 2 - height / 2;
    stty -= height * ( draw_list_length - 1 );
    stty = MAX( 0, stty );

    //now draw each inventory
    for ( cnt = 0; cnt < draw_list_length; cnt++ )
    {
        size_t i;
        STRING buffer;
        int icon_count, item_count, weight_sum, max_weight;
        float x, y, edgex;

        //Figure out who this is
        ipla = draw_list[cnt];
        ppla = PlaStack_get_ptr( ipla );

        ichr = ppla->index;
        pchr = ChrList_get_ptr( ichr );

        //handle inventories sliding into view
        ppla->inventory_lerp = MIN( ppla->inventory_lerp, width );
        if ( ppla->inventory_lerp > 0 && lerp_time[cnt] < SDL_GetTicks() )
        {
            lerp_time[cnt] = SDL_GetTicks() + 1;
            ppla->inventory_lerp = MAX( 0, ppla->inventory_lerp - 16 );
        }

        //set initial positions
        x = sttx - ppla->inventory_lerp;
        y = stty;

        //threshold to start a new row
        edgex = sttx + width + 5 - 32;

        //calculate max carry weight
        max_weight = 200 + FP8_TO_FLOAT( pchr->strength ) * FP8_TO_FLOAT( pchr->strength );

        //draw the backdrop
        ui_drawButton( 0, x, y, width, height, background_color );
        x += 5;

        //draw title
        draw_wrap_string( chr_get_name( ichr, CHRNAME_CAPITAL, NULL, 0 ), x, y, x + width );
        y += 32;

        //draw each inventory icon
        weight_sum = 0;
        icon_count = 0;
        item_count = 0;
        for ( i = 0; i < MAXINVENTORY; i++ )
        {
            CHR_REF item = pchr->inventory[i];

            //calculate the sum of the weight of all items in inventory
            if ( INGAME_CHR( item ) ) weight_sum += chr_get_pcap( item )->weight;

            //draw icon
            draw_one_character_icon( item, x, y, btrue, item_count == ppla->selected_item ? COLOR_WHITE : NOSPARKLE );
            icon_count++;
            item_count++;
            x += 32 + 5;

            //new row?
            if ( x >= edgex || icon_count >= MAXINVENTORY / 2 )
            {
                x = sttx + 5 - ppla->inventory_lerp;
                y += 32 + 5;
                icon_count = 0;
            }
        }

        //Draw weight
        x = sttx + 5 - ppla->inventory_lerp;
        y = stty + height - 42;
        snprintf( buffer, SDL_arraysize( buffer ), "Weight: %d/%d", weight_sum, max_weight );
        draw_wrap_string( buffer, x, y, sttx + width + 5 );

        //prepare drawing the next inventory
        stty += height + 10;
    }

}

//--------------------------------------------------------------------------------------------
void draw_mouse_cursor()
{
    int     x, y;
    oglx_texture_t * pcursor;

    if ( !mous.on )
    {
        SDL_ShowCursor( SDL_DISABLE );
        return;
    }

    pcursor = TxMenu_get_valid_ptr( MENU_CURSOR );

    // Invalid texture?
    if ( NULL == pcursor )
    {
        SDL_ShowCursor( SDL_ENABLE );
    }
    else
    {
        oglx_frect_t tx_tmp, sc_tmp;

        // Hide the SDL mouse
        SDL_ShowCursor( SDL_DISABLE );

        x = ABS( mous.x );
        y = ABS( mous.y );

        if ( oglx_texture_getSize( pcursor, tx_tmp, sc_tmp ) )
        {
            ego_frect_t tx_rect, sc_rect;

            tx_rect.xmin = tx_tmp[0];
            tx_rect.ymin = tx_tmp[1];
            tx_rect.xmax = tx_tmp[2];
            tx_rect.ymax = tx_tmp[3];

            sc_rect.xmin = x + sc_tmp[0];
            sc_rect.ymin = y + sc_tmp[1];
            sc_rect.xmax = x + sc_tmp[2];
            sc_rect.ymax = y + sc_tmp[3];

            draw_quad_2d( pcursor, sc_rect, tx_rect, btrue );
        }
    }
}

//--------------------------------------------------------------------------------------------
void draw_quad_2d( oglx_texture_t * ptex, const ego_frect_t scr_rect, const ego_frect_t tx_rect, const bool_t use_alpha )
{
    ATTRIB_PUSH( __FUNCTION__, GL_CURRENT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT )
    {
        GLboolean texture_1d_enabled, texture_2d_enabled;

        texture_1d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_1D );
        texture_2d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_2D );

        if ( NULL == ptex || INVALID_GL_ID == ptex->base.binding )
        {
            GL_DEBUG( glDisable )( GL_TEXTURE_1D );                           // GL_ENABLE_BIT
            GL_DEBUG( glDisable )( GL_TEXTURE_2D );                           // GL_ENABLE_BIT
        }
        else
        {
            GL_DEBUG( glEnable )( ptex->base.target );                        // GL_ENABLE_BIT
            oglx_texture_Bind( ptex );
        }

        GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f );                      // GL_CURRENT_BIT

        if ( use_alpha )
        {
            GL_DEBUG( glEnable )( GL_BLEND );                                 // GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

            GL_DEBUG( glEnable )( GL_ALPHA_TEST );                            // GL_ENABLE_BIT
            GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );                      // GL_COLOR_BUFFER_BIT
        }
        else
        {
            GL_DEBUG( glDisable )( GL_BLEND );                                 // GL_ENABLE_BIT
            GL_DEBUG( glDisable )( GL_ALPHA_TEST );                            // GL_ENABLE_BIT
        }

        GL_DEBUG( glBegin )( GL_QUADS );
        {
            GL_DEBUG( glTexCoord2f )( tx_rect.xmin, tx_rect.ymax ); GL_DEBUG( glVertex2f )( scr_rect.xmin, scr_rect.ymax );
            GL_DEBUG( glTexCoord2f )( tx_rect.xmax, tx_rect.ymax ); GL_DEBUG( glVertex2f )( scr_rect.xmax, scr_rect.ymax );
            GL_DEBUG( glTexCoord2f )( tx_rect.xmax, tx_rect.ymin ); GL_DEBUG( glVertex2f )( scr_rect.xmax, scr_rect.ymin );
            GL_DEBUG( glTexCoord2f )( tx_rect.xmin, tx_rect.ymin ); GL_DEBUG( glVertex2f )( scr_rect.xmin, scr_rect.ymin );
        }
        GL_DEBUG_END();

        // fix the texture enabling
        if ( texture_1d_enabled )
        {
            GL_DEBUG( glEnable )( GL_TEXTURE_1D );
        }
        else if ( texture_2d_enabled )
        {
            GL_DEBUG( glEnable )( GL_TEXTURE_2D );
        }
    }
    ATTRIB_POP( __FUNCTION__ );
}

//--------------------------------------------------------------------------------------------
// 3D RENDERER FUNCTIONS
//--------------------------------------------------------------------------------------------
void render_shadow_sprite( float intensity, GLvertex v[] )
{
    int i;

    if ( intensity*255.0f < 1.0f ) return;

    GL_DEBUG( glColor4f )( intensity, intensity, intensity, 1.0f );

    GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
    {
        for ( i = 0; i < 4; i++ )
        {
            GL_DEBUG( glTexCoord2fv )( v[i].tex );
            GL_DEBUG( glVertex3fv )( v[i].pos );
        }
    }
    GL_DEBUG_END();
}

//--------------------------------------------------------------------------------------------
void render_shadow( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function draws a NIFTY shadow

    GLvertex v[4];

    TX_REF  itex;
    int     itex_style;
    float   x, y;
    float   level;
    float   height, size_umbra, size_penumbra;
    float   alpha, alpha_umbra, alpha_penumbra;
    chr_t * pchr;
    ego_tile_info_t * ptile;

    if ( IS_ATTACHED_CHR( character ) ) return;
    pchr = ChrList_get_ptr( character );

    // if the character is hidden, not drawn at all, so no shadow
    if ( pchr->is_hidden || 0 == pchr->shadow_size ) return;

    // no shadow if off the mesh
    ptile = mesh_get_ptile( PMesh, pchr->onwhichgrid );
    if ( NULL == ptile ) return;

    // no shadow if invalid tile image
    if ( TILE_IS_FANOFF( *ptile ) ) return;

    // no shadow if completely transparent
    alpha = ( 255 == pchr->inst.light ) ? pchr->inst.alpha  * INV_FF : ( pchr->inst.alpha - pchr->inst.light ) * INV_FF;

    /// @test ZF@> The previous test didn't work, but this one does
    //if ( alpha * 255 < 1 ) return;
    if ( pchr->inst.light <= INVISIBLE || pchr->inst.alpha <= INVISIBLE ) return;

    // much reduced shadow if on a reflective tile
    if ( 0 != mesh_test_fx( PMesh, pchr->onwhichgrid, MPDFX_DRAWREF ) )
    {
        alpha *= 0.1f;
    }
    if ( alpha < INV_FF ) return;

    // Original points
    level = pchr->enviro.floor_level;
    level += SHADOWRAISE;
    height = pchr->inst.matrix.CNV( 3, 2 ) - level;
    if ( height < 0 ) height = 0;

    size_umbra    = 1.5f * ( pchr->bump.size - height / 30.0f );
    size_penumbra = 1.5f * ( pchr->bump.size + height / 30.0f );

    alpha *= 0.3f;
    alpha_umbra = alpha_penumbra = alpha;
    if ( height > 0 )
    {
        float factor_penumbra = ( 1.5f ) * (( pchr->bump.size ) / size_penumbra );
        float factor_umbra    = ( 1.5f ) * (( pchr->bump.size ) / size_umbra );

        factor_umbra    = MAX( 1.0f, factor_umbra );
        factor_penumbra = MAX( 1.0f, factor_penumbra );

        alpha_umbra    *= 1.0f / factor_umbra / factor_umbra / 1.5f;
        alpha_penumbra *= 1.0f / factor_penumbra / factor_penumbra / 1.5f;

        alpha_umbra    = CLIP( alpha_umbra,    0.0f, 1.0f );
        alpha_penumbra = CLIP( alpha_penumbra, 0.0f, 1.0f );
    }

    x = pchr->inst.matrix.CNV( 3, 0 );
    y = pchr->inst.matrix.CNV( 3, 1 );

    // Choose texture.
    itex = TX_PARTICLE_LIGHT;
    oglx_texture_Bind( TxTexture_get_valid_ptr( itex ) );

    itex_style = prt_get_texture_style( itex );
    if ( itex_style < 0 ) itex_style = 0;

    // GOOD SHADOW
    v[0].tex[SS] = CALCULATE_PRT_U0( itex_style, 238 );
    v[0].tex[TT] = CALCULATE_PRT_V0( itex_style, 238 );

    v[1].tex[SS] = CALCULATE_PRT_U1( itex_style, 255 );
    v[1].tex[TT] = CALCULATE_PRT_V0( itex_style, 238 );

    v[2].tex[SS] = CALCULATE_PRT_U1( itex_style, 255 );
    v[2].tex[TT] = CALCULATE_PRT_V1( itex_style, 255 );

    v[3].tex[SS] = CALCULATE_PRT_U0( itex_style, 238 );
    v[3].tex[TT] = CALCULATE_PRT_V1( itex_style, 255 );

    if ( size_penumbra > 0 )
    {
        v[0].pos[XX] = x + size_penumbra;
        v[0].pos[YY] = y - size_penumbra;
        v[0].pos[ZZ] = level;

        v[1].pos[XX] = x + size_penumbra;
        v[1].pos[YY] = y + size_penumbra;
        v[1].pos[ZZ] = level;

        v[2].pos[XX] = x - size_penumbra;
        v[2].pos[YY] = y + size_penumbra;
        v[2].pos[ZZ] = level;

        v[3].pos[XX] = x - size_penumbra;
        v[3].pos[YY] = y - size_penumbra;
        v[3].pos[ZZ] = level;

        render_shadow_sprite( alpha_penumbra, v );
    };

    if ( size_umbra > 0 )
    {
        v[0].pos[XX] = x + size_umbra;
        v[0].pos[YY] = y - size_umbra;
        v[0].pos[ZZ] = level + 0.1f;

        v[1].pos[XX] = x + size_umbra;
        v[1].pos[YY] = y + size_umbra;
        v[1].pos[ZZ] = level + 0.1f;

        v[2].pos[XX] = x - size_umbra;
        v[2].pos[YY] = y + size_umbra;
        v[2].pos[ZZ] = level + 0.1f;

        v[3].pos[XX] = x - size_umbra;
        v[3].pos[YY] = y - size_umbra;
        v[3].pos[ZZ] = level + 0.1f;

        render_shadow_sprite( alpha_umbra, v );
    };
}

//--------------------------------------------------------------------------------------------
void render_bad_shadow( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function draws a sprite shadow

    GLvertex v[4];

    TX_REF  itex;
    int     itex_style;
    float   size, x, y;
    float   level, height, height_factor, alpha;
    chr_t * pchr;
    ego_tile_info_t * ptile;

    if ( IS_ATTACHED_CHR( character ) ) return;
    pchr = ChrList_get_ptr( character );

    // if the character is hidden, not drawn at all, so no shadow
    if ( pchr->is_hidden || 0 == pchr->shadow_size ) return;

    // no shadow if off the mesh
    ptile = mesh_get_ptile( PMesh, pchr->onwhichgrid );
    if ( NULL == ptile ) return;

    // no shadow if invalid tile image
    if ( TILE_IS_FANOFF( *ptile ) ) return;

    // no shadow if completely transparent or completely glowing
    alpha = ( 255 == pchr->inst.light ) ? pchr->inst.alpha  * INV_FF : ( pchr->inst.alpha - pchr->inst.light ) * INV_FF;

    /// @test ZF@> previous test didn't work, but this one does
    //if ( alpha * 255 < 1 ) return;
    if ( pchr->inst.light <= INVISIBLE || pchr->inst.alpha <= INVISIBLE ) return;

    // much reduced shadow if on a reflective tile
    if ( 0 != mesh_test_fx( PMesh, pchr->onwhichgrid, MPDFX_DRAWREF ) )
    {
        alpha *= 0.1f;
    }
    if ( alpha < INV_FF ) return;

    // Original points
    level = pchr->enviro.floor_level;
    level += SHADOWRAISE;
    height = pchr->inst.matrix.CNV( 3, 2 ) - level;
    height_factor = 1.0f - height / ( pchr->shadow_size * 5.0f );
    if ( height_factor <= 0.0f ) return;

    // how much transparency from height
    alpha *= height_factor * 0.5f + 0.25f;
    if ( alpha < INV_FF ) return;

    x = pchr->inst.matrix.CNV( 3, 0 );
    y = pchr->inst.matrix.CNV( 3, 1 );

    size = pchr->shadow_size * height_factor;

    v[0].pos[XX] = ( float ) x + size;
    v[0].pos[YY] = ( float ) y - size;
    v[0].pos[ZZ] = ( float ) level;

    v[1].pos[XX] = ( float ) x + size;
    v[1].pos[YY] = ( float ) y + size;
    v[1].pos[ZZ] = ( float ) level;

    v[2].pos[XX] = ( float ) x - size;
    v[2].pos[YY] = ( float ) y + size;
    v[2].pos[ZZ] = ( float ) level;

    v[3].pos[XX] = ( float ) x - size;
    v[3].pos[YY] = ( float ) y - size;
    v[3].pos[ZZ] = ( float ) level;

    // Choose texture and matrix
    itex = TX_PARTICLE_LIGHT;
    oglx_texture_Bind( TxTexture_get_valid_ptr( itex ) );

    itex_style = prt_get_texture_style( itex );
    if ( itex_style < 0 ) itex_style = 0;

    v[0].tex[SS] = CALCULATE_PRT_U0( itex_style, 236 );
    v[0].tex[TT] = CALCULATE_PRT_V0( itex_style, 236 );

    v[1].tex[SS] = CALCULATE_PRT_U1( itex_style, 253 );
    v[1].tex[TT] = CALCULATE_PRT_V0( itex_style, 236 );

    v[2].tex[SS] = CALCULATE_PRT_U1( itex_style, 253 );
    v[2].tex[TT] = CALCULATE_PRT_V1( itex_style, 253 );

    v[3].tex[SS] = CALCULATE_PRT_U0( itex_style, 236 );
    v[3].tex[TT] = CALCULATE_PRT_V1( itex_style, 253 );

    render_shadow_sprite( alpha, v );
}

//--------------------------------------------------------------------------------------------
gfx_rv render_fans_by_list( const camera_t * pcam, const ego_mpd_t * pmesh, const Uint32 list[], const size_t list_size )
{
    Uint32 cnt;
    TX_REF tx;

    if ( NULL == pmesh ) pmesh = PMesh;
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid mesh" );
        return gfx_error;
    }

    if ( NULL == list )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL tile list" );
        return gfx_error;
    }

    if ( 0 == list_size ) return gfx_success;

    if ( meshnotexture )
    {
        meshlasttexture = ( Uint16 )( ~0 );
        oglx_texture_Bind( NULL );

        for ( cnt = 0; cnt < list_size; cnt++ )
        {
            render_fan( pmesh, list[cnt] );
        }
    }
    else
    {
        for ( tx = TX_TILE_0; tx <= TX_TILE_3; tx++ )
        {
            meshlasttexture = tx;
            oglx_texture_Bind( TxTexture_get_valid_ptr( tx ) );

            for ( cnt = 0; cnt < list_size; cnt++ )
            {
                render_fan( pmesh, list[cnt] );
            }
        }
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
// render_scene FUNCTIONS
//--------------------------------------------------------------------------------------------
gfx_rv render_scene_init( renderlist_t * prlist, dolist_t * pdolist, dynalist_t * pdylist, const camera_t * pcam )
{
    gfx_rv retval;
    ego_mpd_t * pmesh;

    // assume the best;
    retval = gfx_success;

    PROFILE_BEGIN( gfx_make_renderlist );
    {
        // Which tiles can be displayed
        if ( gfx_error == gfx_make_renderlist( prlist, pcam ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( gfx_make_renderlist );

    pmesh = renderlist_get_pmesh( prlist );
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist mesh" );
        return gfx_error;
    }

    PROFILE_BEGIN( gfx_make_dolist );
    {
        // determine which objects are visible
        if ( gfx_error == gfx_make_dolist( pdolist, pcam ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( gfx_make_dolist );

    // put off sorting the dolist until later
    // because it has to be sorted differently for reflected and non-reflected objects
    // dolist_sort( pcam, bfalse );

    PROFILE_BEGIN( do_grid_lighting );
    {
        // figure out the terrain lighting
        if ( gfx_error == do_grid_lighting( prlist, pdylist, pcam ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( do_grid_lighting );

    PROFILE_BEGIN( light_fans );
    {
        // apply the lighting to the characters and particles
        if ( gfx_error == light_fans( prlist ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( light_fans );

    PROFILE_BEGIN( gfx_update_all_chr_instance );
    {
        // make sure the characters are ready to draw
        if ( gfx_error == gfx_update_all_chr_instance() )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( gfx_update_all_chr_instance );

    PROFILE_BEGIN( update_all_prt_instance );
    {
        // make sure the particles are ready to draw
        if ( gfx_error == update_all_prt_instance( pcam ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( update_all_prt_instance );

    // do the flashing for kursed objects
    if ( gfx_error == gfx_update_flashing( pdolist ) )
    {
        retval = gfx_error;
    }

    time_render_scene_init_renderlist_make         = PROFILE_QUERY( gfx_make_renderlist ) * TARGET_FPS;
    time_render_scene_init_dolist_make             = PROFILE_QUERY( gfx_make_dolist ) * TARGET_FPS;
    time_render_scene_init_do_grid_dynalight       = PROFILE_QUERY( do_grid_lighting ) * TARGET_FPS;
    time_render_scene_init_light_fans              = PROFILE_QUERY( light_fans ) * TARGET_FPS;
    time_render_scene_init_update_all_chr_instance = PROFILE_QUERY( gfx_update_all_chr_instance ) * TARGET_FPS;
    time_render_scene_init_update_all_prt_instance = PROFILE_QUERY( update_all_prt_instance ) * TARGET_FPS;

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh_ndr( const camera_t * pcam, const renderlist_t * prlist )
{
    /// @author BB
    /// @details draw all tiles that do not reflect characters

    gfx_rv retval;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
    {
        // store the surface depth
        GL_DEBUG( glDepthMask )( GL_TRUE );         // GL_DEPTH_BUFFER_BIT

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );       // GL_DEPTH_BUFFER_BIT

        // no transparency
        GL_DEBUG( glDisable )( GL_BLEND );          // GL_ENABLE_BIT

        // draw draw front and back faces of polygons
        oglx_end_culling();      // GL_ENABLE_BIT

        // do not display the completely transparent portion
        // use alpha test to allow the thatched roof tiles to look like thatch
        GL_DEBUG( glEnable )( GL_ALPHA_TEST );      // GL_ENABLE_BIT
        // speed-up drawing of surfaces with alpha == 0.0f sections
        GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );   // GL_COLOR_BUFFER_BIT

        // reduce texture hashing by loading up each texture only once
        if ( gfx_error == render_fans_by_list( pcam, prlist->pmesh, prlist->ndr, prlist->ndr_count ) )
        {
            retval = gfx_error;
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh_drf_back( const camera_t * pcam, const renderlist_t * prlist )
{
    /// @author BB
    /// @details draw the reflective tiles, but turn off the depth buffer
    ///               this blanks out any background that might've been drawn

    gfx_rv retval;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
    {
        // DO NOT store the surface depth
        GL_DEBUG( glDepthMask )( GL_FALSE );        // GL_DEPTH_BUFFER_BIT

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );       // GL_DEPTH_BUFFER_BIT

        // black out any backgound, but allow the background to show through any holes in the floor
        GL_DEBUG( glEnable )( GL_BLEND );                              // GL_ENABLE_BIT
        // use the alpha channel to modulate the transparency
        GL_DEBUG( glBlendFunc )( GL_ZERO, GL_ONE_MINUS_SRC_ALPHA );    // GL_COLOR_BUFFER_BIT

        // do not display the completely transparent portion
        // use alpha test to allow the thatched roof tiles to look like thatch
        GL_DEBUG( glEnable )( GL_ALPHA_TEST );      // GL_ENABLE_BIT
        // speed-up drawing of surfaces with alpha == 0.0f sections
        GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );   // GL_COLOR_BUFFER_BIT

        // reduce texture hashing by loading up each texture only once
        if ( gfx_error == render_fans_by_list( pcam, prlist->pmesh, prlist->drf, prlist->drf_count ) )
        {
            retval = gfx_error;
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh_ref( const camera_t * pcam, const renderlist_t * prlist, const dolist_t * pdolist )
{
    /// @author BB
    /// @details Render all reflected objects

    int cnt;
    gfx_rv retval;

    ego_mpd_t * pmesh;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return gfx_error;
    }

    if ( NULL == pdolist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

    if ( pdolist->count >= DOLIST_SIZE )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist size" );
        return gfx_error;
    }

    pmesh = renderlist_get_pmesh( prlist );
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist mesh" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__,  GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT );
    {
        // don't write into the depth buffer (disable glDepthMask for transparent objects)
        // turn off the depth mask by default. Can cause glitches if used improperly.
        GL_DEBUG( glDepthMask )( GL_FALSE );      // GL_DEPTH_BUFFER_BIT

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );    // GL_ENABLE_BIT
        // surfaces must be closer to the camera to be drawn
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );     // GL_DEPTH_BUFFER_BIT

        for ( cnt = (( int )pdolist->count ) - 1; cnt >= 0; cnt-- )
        {
            if ( MAX_PRT == pdolist->lst[cnt].iprt && MAX_CHR != pdolist->lst[cnt].ichr )
            {
                CHR_REF ichr;
                Uint32 itile;

                // cull backward facing polygons
                // use couter-clockwise orientation to determine backfaces
                oglx_begin_culling( GL_BACK, MPD_REF_CULL );            // GL_ENABLE_BIT | GL_POLYGON_BIT

                // allow transparent objects
                GL_DEBUG( glEnable )( GL_BLEND );                 // GL_ENABLE_BIT
                // use the alpha channel to modulate the transparency
                GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

                ichr  = pdolist->lst[cnt].ichr;
                itile = ChrList.lst[ichr].onwhichgrid;

                if ( mesh_grid_is_valid( pmesh, itile ) && ( 0 != mesh_test_fx( pmesh, itile, MPDFX_DRAWREF ) ) )
                {
                    GL_DEBUG( glColor4f )( 1, 1, 1, 1 );          // GL_CURRENT_BIT

                    if ( gfx_error == render_one_mad_ref( pcam, ichr ) )
                    {
                        retval = gfx_error;
                    }
                }
            }
            else if ( MAX_CHR == pdolist->lst[cnt].ichr && MAX_PRT != pdolist->lst[cnt].iprt )
            {
                Uint32 itile;
                PRT_REF iprt;

                // draw draw front and back faces of polygons
                oglx_end_culling();                     // GL_ENABLE_BIT

                // render_one_prt_ref() actually sets its own blend function, but just to be safe
                // allow transparent objects
                GL_DEBUG( glEnable )( GL_BLEND );                    // GL_ENABLE_BIT
                // set the default particle blending
                GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );     // GL_COLOR_BUFFER_BIT

                iprt = pdolist->lst[cnt].iprt;
                itile = PrtList.lst[iprt].onwhichgrid;

                if ( mesh_grid_is_valid( pmesh, itile ) && ( 0 != mesh_test_fx( pmesh, itile, MPDFX_DRAWREF ) ) )
                {
                    GL_DEBUG( glColor4f )( 1, 1, 1, 1 );          // GL_CURRENT_BIT

                    if ( gfx_error == render_one_prt_ref( iprt ) )
                    {
                        retval = gfx_error;
                    }
                }
            }
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh_ref_chr( const camera_t * pcam, const renderlist_t * prlist )
{
    /// @brief   BB@> Render the shadow floors ( let everything show through )
    /// @author BB
    /// @details turn on the depth mask, so that no objects under the floor will show through
    ///               this assumes that the floor is not partially transparent...

    gfx_rv retval;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
    {
        // set the depth of these tiles
        GL_DEBUG( glDepthMask )( GL_TRUE );                   // GL_DEPTH_BUFFER_BIT

        GL_DEBUG( glEnable )( GL_BLEND );                     // GL_ENABLE_BIT
        GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE );      // GL_COLOR_BUFFER_BIT

        // draw draw front and back faces of polygons
        oglx_end_culling();                // GL_ENABLE_BIT

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );                // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );                 // GL_DEPTH_BUFFER_BIT

        // reduce texture hashing by loading up each texture only once
        if ( gfx_error == render_fans_by_list( pcam, prlist->pmesh, prlist->drf, prlist->drf_count ) )
        {
            retval = gfx_error;
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh_drf_solid( const camera_t * pcam, const renderlist_t * prlist )
{
    /// @brief BB@> Render the shadow floors as normal solid floors

    gfx_rv retval;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
    {
        // no transparency
        GL_DEBUG( glDisable )( GL_BLEND );                    // GL_ENABLE_BIT

        // draw draw front and back faces of polygons
        oglx_end_culling();                // GL_ENABLE_BIT

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );                // GL_ENABLE_BIT

        // store the surface depth
        GL_DEBUG( glDepthMask )( GL_TRUE );                   // GL_DEPTH_BUFFER_BIT

        // do not display the completely transparent portion
        // use alpha test to allow the thatched roof tiles to look like thatch
        GL_DEBUG( glEnable )( GL_ALPHA_TEST );                // GL_ENABLE_BIT
        // speed-up drawing of surfaces with alpha = 0.0f sections
        GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );          // GL_COLOR_BUFFER_BIT

        // reduce texture hashing by loading up each texture only once
        if ( gfx_error == render_fans_by_list( pcam, prlist->pmesh, prlist->drf, prlist->drf_count ) )
        {
            retval = gfx_error;
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh_render_shadows( const camera_t * pcam, const dolist_t * pdolist )
{
    /// @author BB
    /// @details Render the shadows

    size_t cnt;
    int    tnc;

    if ( NULL == pdolist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

    if ( pdolist->count >= DOLIST_SIZE )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist size" );
        return gfx_error;
    }

    if ( !gfx.shaon ) return gfx_success;

    // don't write into the depth buffer (disable glDepthMask for transparent objects)
    GL_DEBUG( glDepthMask )( GL_FALSE );

    // do not draw hidden surfaces
    GL_DEBUG( glEnable )( GL_DEPTH_TEST );

    GL_DEBUG( glEnable )( GL_BLEND );
    GL_DEBUG( glBlendFunc )( GL_ZERO, GL_ONE_MINUS_SRC_COLOR );

    // keep track of the number of shadows actually rendered
    tnc = 0;

    if ( gfx.shasprite )
    {
        // Bad shadows
        for ( cnt = 0; cnt < pdolist->count; cnt++ )
        {
            CHR_REF ichr = pdolist->lst[cnt].ichr;
            if ( !VALID_CHR_RANGE( ichr ) ) continue;

            if ( 0 == ChrList.lst[ichr].shadow_size ) continue;

            render_bad_shadow( ichr );
            tnc++;
        }
    }
    else
    {
        // Good shadows for me
        for ( cnt = 0; cnt < pdolist->count; cnt++ )
        {
            CHR_REF ichr = pdolist->lst[cnt].ichr;
            if ( !VALID_CHR_RANGE( ichr ) ) continue;

            if ( 0 == ChrList.lst[ichr].shadow_size ) continue;

            render_shadow( ichr );
            tnc++;
        }
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh( const camera_t * pcam, const renderlist_t * prlist, const dolist_t * pdolist )
{
    /// @author BB
    /// @details draw the mesh and any reflected objects

    gfx_rv retval;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    // advance the animation of all animated tiles
    animate_all_tiles( prlist->pmesh );

    PROFILE_BEGIN( render_scene_mesh_ndr );
    {
        // draw all tiles that do not reflect characters
        if ( gfx_error == render_scene_mesh_ndr( pcam, prlist ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( render_scene_mesh_ndr );

    //--------------------------------
    // draw the reflective tiles and any reflected objects
    if ( gfx.refon )
    {
        PROFILE_BEGIN( render_scene_mesh_drf_back );
        {
            // blank out the background behind reflective tiles

            if ( gfx_error == render_scene_mesh_drf_back( pcam, prlist ) )
            {
                retval = gfx_error;
            }
        }
        PROFILE_END( render_scene_mesh_drf_back );

        PROFILE_BEGIN( render_scene_mesh_ref );
        {
            // Render all reflected objects
            if ( gfx_error == render_scene_mesh_ref( pcam, prlist, pdolist ) )
            {
                retval = gfx_error;
            }
        }
        PROFILE_END( render_scene_mesh_ref );

        PROFILE_BEGIN( render_scene_mesh_ref_chr );
        {
            // Render the shadow floors
            if ( gfx_error == render_scene_mesh_ref_chr( pcam, prlist ) )
            {
                retval = gfx_error;
            }
        }
        PROFILE_END( render_scene_mesh_ref_chr );
    }
    else
    {
        PROFILE_BEGIN( render_scene_mesh_drf_solid );
        {
            // Render the shadow floors as normal solid floors
            if ( gfx_error == render_scene_mesh_drf_solid( pcam, prlist ) )
            {
                retval = gfx_error;
            }
        }
        PROFILE_END( render_scene_mesh_drf_solid );
    }

#if defined(RENDER_HMAP) && defined(_DEBUG)

    // render the heighmap
    for ( cnt = 0; cnt < prlist->all_count; cnt++ )
    {
        render_hmap_fan( pmesh, prlist->all[cnt] );
    }

#endif

    PROFILE_BEGIN( render_scene_mesh_render_shadows );
    {
        // Render the shadows
        if ( gfx_error == render_scene_mesh_render_shadows( pcam, pdolist ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( render_scene_mesh_render_shadows );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_solid( const camera_t * pcam, dolist_t * pdolist )
{
    /// @detaile BB@> Render all solid objects

    Uint32 cnt;
    gfx_rv retval;

    if ( NULL == pdolist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

    if ( pdolist->count >= DOLIST_SIZE )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist size" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT )
    {
        // scan for solid objects
        for ( cnt = 0; cnt < pdolist->count; cnt++ )
        {
            // solid objects draw into the depth buffer for hidden surface removal
            GL_DEBUG( glDepthMask )( GL_TRUE );                     // GL_ENABLE_BIT

            // do not draw hidden surfaces
            GL_DEBUG( glEnable )( GL_DEPTH_TEST );                  // GL_ENABLE_BIT
            GL_DEBUG( glDepthFunc )( GL_LESS );                   // GL_DEPTH_BUFFER_BIT

            GL_DEBUG( glEnable )( GL_ALPHA_TEST );                 // GL_ENABLE_BIT
            GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );             // GL_COLOR_BUFFER_BIT

            if ( MAX_PRT == pdolist->lst[cnt].iprt && VALID_CHR_RANGE( pdolist->lst[cnt].ichr ) )
            {
                if ( gfx_error == render_one_mad_solid( pcam, pdolist->lst[cnt].ichr ) )
                {
                    retval = gfx_error;
                }
            }
            else if ( MAX_CHR == pdolist->lst[cnt].ichr && VALID_PRT_RANGE( pdolist->lst[cnt].iprt ) )
            {
                // draw draw front and back faces of polygons
                oglx_end_culling();              // GL_ENABLE_BIT

                if ( gfx_error == render_one_prt_solid( pdolist->lst[cnt].iprt ) )
                {
                    retval = gfx_error;
                }
            }
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_trans( const camera_t * pcam, dolist_t * pdolist )
{
    /// @author BB
    /// @details draw transparent objects

    int cnt;
    gfx_rv retval;

    if ( NULL == pdolist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

    if ( pdolist->count >= DOLIST_SIZE )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist size" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT )
    {
        //---- set the the transparency parameters

        // don't write into the depth buffer (disable glDepthMask for transparent objects)
        GL_DEBUG( glDepthMask )( GL_FALSE );                   // GL_DEPTH_BUFFER_BIT

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );                // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );                 // GL_DEPTH_BUFFER_BIT

        // Now render all transparent and light objects
        for ( cnt = (( int )pdolist->count ) - 1; cnt >= 0; cnt-- )
        {
            if ( MAX_PRT == pdolist->lst[cnt].iprt && MAX_CHR != pdolist->lst[cnt].ichr )
            {
                if ( gfx_error == render_one_mad_trans( pcam, pdolist->lst[cnt].ichr ) )
                {
                    retval = gfx_error;
                }
            }
            else if ( MAX_CHR == pdolist->lst[cnt].ichr && MAX_PRT != pdolist->lst[cnt].iprt )
            {
                // this is a particle
                if ( gfx_error == render_one_prt_trans( pdolist->lst[cnt].iprt ) )
                {
                    retval = gfx_error;
                }
            }
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene( const camera_t * pcam, const int render_list_index, const int dolist_index )
{
    /// @author ZZ
    /// @details This function draws 3D objects

    gfx_rv retval;
    renderlist_t * prlist = NULL;
    dolist_t     * pdlist = NULL;

    if ( NULL == pcam ) pcam = pcam;
    if ( NULL == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    prlist = renderlist_mgr_get_ptr( &_renderlist_mgr_data, render_list_index );
    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "could lock a renderlist" );
        return gfx_error;
    }

    pdlist = dolist_mgr_get_ptr( &_dolist_mgr_data, dolist_index );
    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "could lock a dolist" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    PROFILE_BEGIN( render_scene_init );
    {
        if ( gfx_error == render_scene_init( prlist, pdlist, &_dynalist, pcam ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( render_scene_init );

    PROFILE_BEGIN( render_scene_mesh );
    {
        PROFILE_BEGIN( render_scene_mesh_dolist_sort );
        {
            // sort the dolist for reflected objects
            // reflected characters and objects are drawn in this pass
            if ( gfx_error == dolist_sort( pdlist, pcam, btrue ) )
            {
                retval = gfx_error;
            }
        }
        PROFILE_END( render_scene_mesh_dolist_sort );

        // do the render pass for the mesh
        if ( gfx_error == render_scene_mesh( pcam, prlist, pdlist ) )
        {
            retval = gfx_error;
        }

        time_render_scene_mesh_dolist_sort    = PROFILE_QUERY( render_scene_mesh_dolist_sort ) * TARGET_FPS;
        time_render_scene_mesh_ndr            = PROFILE_QUERY( render_scene_mesh_ndr ) * TARGET_FPS;
        time_render_scene_mesh_drf_back       = PROFILE_QUERY( render_scene_mesh_drf_back ) * TARGET_FPS;
        time_render_scene_mesh_ref            = PROFILE_QUERY( render_scene_mesh_ref ) * TARGET_FPS;
        time_render_scene_mesh_ref_chr        = PROFILE_QUERY( render_scene_mesh_ref_chr ) * TARGET_FPS;
        time_render_scene_mesh_drf_solid      = PROFILE_QUERY( render_scene_mesh_drf_solid ) * TARGET_FPS;
        time_render_scene_mesh_render_shadows = PROFILE_QUERY( render_scene_mesh_render_shadows ) * TARGET_FPS;
    }
    PROFILE_END( render_scene_mesh );

    PROFILE_BEGIN( render_scene_solid );
    {
        // sort the dolist for non-reflected objects
        if ( gfx_error == dolist_sort( pdlist, pcam, bfalse ) )
        {
            retval = gfx_error;
        }

        // do the render pass for solid objects
        if ( gfx_error == render_scene_solid( pcam, pdlist ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( render_scene_solid );

    PROFILE_BEGIN( render_scene_water );
    {
        // draw the water
        if ( gfx_error == render_water( prlist ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( render_scene_water );

    PROFILE_BEGIN( render_scene_trans );
    {
        // do the render pass for transparent objects
        if ( gfx_error == render_scene_trans( pcam, pdlist ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( render_scene_trans );

#if defined(_DEBUG)
    render_all_prt_attachment();
#endif

#if defined(DRAW_LISTS)
    // draw some debugging lines
    line_list_draw_all( pcam );
    point_list_draw_all( pcam );
#endif

#if defined(DRAW_PRT_BBOX)
    render_all_prt_bbox();
#endif

    time_render_scene_init  = PROFILE_QUERY( render_scene_init ) * TARGET_FPS;
    time_render_scene_mesh  = PROFILE_QUERY( render_scene_mesh ) * TARGET_FPS;
    time_render_scene_solid = PROFILE_QUERY( render_scene_solid ) * TARGET_FPS;
    time_render_scene_water = PROFILE_QUERY( render_scene_water ) * TARGET_FPS;
    time_render_scene_trans = PROFILE_QUERY( render_scene_trans ) * TARGET_FPS;

    time_draw_scene = time_render_scene_init + time_render_scene_mesh + time_render_scene_solid + time_render_scene_water + time_render_scene_trans;

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_world_background( const camera_t * pcam, const TX_REF texture )
{
    /// @author ZZ
    /// @details This function draws the large background
    GLvertex vtlist[4];
    int i;
    float z0, Qx, Qy;
    float light = 1.0f, intens = 1.0f, alpha = 1.0f;

    float xmag, Cx_0, Cx_1;
    float ymag, Cy_0, Cy_1;

    ego_mpd_info_t * pinfo;
    grid_mem_t     * pgmem;
    oglx_texture_t   * ptex;
    water_instance_layer_t * ilayer;

    if ( NULL == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    pinfo = &( PMesh->info );
    pgmem = &( PMesh->gmem );

    // which layer
    ilayer = water.layer + 0;

    // the "official" camera height
    z0 = 1500;

    // clip the waterlayer uv offset
    ilayer->tx.x = ilayer->tx.x - ( float )FLOOR( ilayer->tx.x );
    ilayer->tx.y = ilayer->tx.y - ( float )FLOOR( ilayer->tx.y );

    // determine the constants for the x-coordinate
    xmag = water.backgroundrepeat / 4 / ( 1.0f + z0 * ilayer->dist.x ) / GRID_FSIZE;
    Cx_0 = xmag * ( 1.0f +  pcam->pos.z       * ilayer->dist.x );
    Cx_1 = -xmag * ( 1.0f + ( pcam->pos.z - z0 ) * ilayer->dist.x );

    // determine the constants for the y-coordinate
    ymag = water.backgroundrepeat / 4 / ( 1.0f + z0 * ilayer->dist.y ) / GRID_FSIZE;
    Cy_0 = ymag * ( 1.0f +  pcam->pos.z       * ilayer->dist.y );
    Cy_1 = -ymag * ( 1.0f + ( pcam->pos.z - z0 ) * ilayer->dist.y );

    // Figure out the coordinates of its corners
    Qx = -pgmem->edge_x;
    Qy = -pgmem->edge_y;
    vtlist[0].pos[XX] = Qx;
    vtlist[0].pos[YY] = Qy;
    vtlist[0].pos[ZZ] = pcam->pos.z - z0;
    vtlist[0].tex[SS] = Cx_0 * Qx + Cx_1 * pcam->pos.x + ilayer->tx.x;
    vtlist[0].tex[TT] = Cy_0 * Qy + Cy_1 * pcam->pos.y + ilayer->tx.y;

    Qx = 2 * pgmem->edge_x;
    Qy = -pgmem->edge_y;
    vtlist[1].pos[XX] = Qx;
    vtlist[1].pos[YY] = Qy;
    vtlist[1].pos[ZZ] = pcam->pos.z - z0;
    vtlist[1].tex[SS] = Cx_0 * Qx + Cx_1 * pcam->pos.x + ilayer->tx.x;
    vtlist[1].tex[TT] = Cy_0 * Qy + Cy_1 * pcam->pos.y + ilayer->tx.y;

    Qx = 2 * pgmem->edge_x;
    Qy = 2 * pgmem->edge_y;
    vtlist[2].pos[XX] = Qx;
    vtlist[2].pos[YY] = Qy;
    vtlist[2].pos[ZZ] = pcam->pos.z - z0;
    vtlist[2].tex[SS] = Cx_0 * Qx + Cx_1 * pcam->pos.x + ilayer->tx.x;
    vtlist[2].tex[TT] = Cy_0 * Qy + Cy_1 * pcam->pos.y + ilayer->tx.y;

    Qx = -pgmem->edge_x;
    Qy = 2 * pgmem->edge_y;
    vtlist[3].pos[XX] = Qx;
    vtlist[3].pos[YY] = Qy;
    vtlist[3].pos[ZZ] = pcam->pos.z - z0;
    vtlist[3].tex[SS] = Cx_0 * Qx + Cx_1 * pcam->pos.x + ilayer->tx.x;
    vtlist[3].tex[TT] = Cy_0 * Qy + Cy_1 * pcam->pos.y + ilayer->tx.y;

    light = water.light ? 1.0f : 0.0f;
    alpha = ilayer->alpha * INV_FF;

    if ( gfx.usefaredge )
    {
        float fcos;

        intens = light_a * ilayer->light_add;

        fcos = light_nrm[kZ];
        if ( fcos > 0.0f )
        {
            intens += fcos * fcos * light_d * ilayer->light_dir;
        }

        intens = CLIP( intens, 0.0f, 1.0f );
    }

    ptex = TxTexture_get_valid_ptr( texture );

    oglx_texture_Bind( ptex );

    ATTRIB_PUSH( __FUNCTION__, GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT );
    {
        // flat shading
        GL_DEBUG( glShadeModel )( GL_FLAT );      // GL_LIGHTING_BIT

        // don't write into the depth buffer (disable glDepthMask for transparent objects)
        GL_DEBUG( glDepthMask )( GL_FALSE );      // GL_DEPTH_BUFFER_BIT

        // essentially disable the depth test without calling glDisable( GL_DEPTH_TEST )
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_ALWAYS );     // GL_DEPTH_BUFFER_BIT

        // draw draw front and back faces of polygons
        oglx_end_culling();    // GL_ENABLE_BIT

        if ( alpha > 0.0f )
        {
            ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT );
            {
                GL_DEBUG( glColor4f )( intens, intens, intens, alpha );             // GL_CURRENT_BIT

                if ( alpha >= 1.0f )
                {
                    GL_DEBUG( glDisable )( GL_BLEND );                               // GL_ENABLE_BIT
                }
                else
                {
                    GL_DEBUG( glEnable )( GL_BLEND );                               // GL_ENABLE_BIT
                    GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT
                }

                GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
                {
                    for ( i = 0; i < 4; i++ )
                    {
                        GL_DEBUG( glTexCoord2fv )( vtlist[i].tex );
                        GL_DEBUG( glVertex3fv )( vtlist[i].pos );
                    }
                }
                GL_DEBUG_END();
            }
            ATTRIB_POP( __FUNCTION__ );
        }

        if ( light > 0.0f )
        {
            ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT );
            {
                GL_DEBUG( glDisable )( GL_BLEND );                           // GL_COLOR_BUFFER_BIT
                GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE );            // GL_COLOR_BUFFER_BIT

                GL_DEBUG( glColor4f )( light, light, light, 1.0f );         // GL_CURRENT_BIT

                GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
                {
                    for ( i = 0; i < 4; i++ )
                    {
                        GL_DEBUG( glTexCoord2fv )( vtlist[i].tex );
                        GL_DEBUG( glVertex3fv )( vtlist[i].pos );
                    }
                }
                GL_DEBUG_END();
            }
            ATTRIB_POP( __FUNCTION__ );
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_world_overlay( const camera_t * pcam, const TX_REF texture )
{
    /// @author ZZ
    /// @details This function draws the large foreground

    float alpha, ftmp;
    fvec3_t   vforw_wind, vforw_cam;
    TURN_T default_turn;

    oglx_texture_t           * ptex;

    water_instance_layer_t * ilayer = water.layer + 1;

    if ( NULL == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    vforw_wind.x = ilayer->tx_add.x;
    vforw_wind.y = ilayer->tx_add.y;
    vforw_wind.z = 0;
    fvec3_self_normalize( vforw_wind.v );

    mat_getCamForward( pcam->mView.v, vforw_cam.v );
    fvec3_self_normalize( vforw_cam.v );

    // make the texture begin to disappear if you are not looking straight down
    ftmp = fvec3_dot_product( vforw_wind.v, vforw_cam.v );

    alpha = ( 1.0f - ftmp * ftmp ) * ( ilayer->alpha * INV_FF );

    if ( alpha != 0.0f )
    {
        GLvertex vtlist[4];
        int i;
        float size;
        float sinsize, cossize;
        float x, y, z;
        float loc_foregroundrepeat;

        // Figure out the screen coordinates of its corners
        x = sdl_scr.x << 6;
        y = sdl_scr.y << 6;
        z = 0;
        size = x + y + 1;
        default_turn = ( 3 * 2047 ) & TRIG_TABLE_MASK;
        sinsize = turntosin[default_turn] * size;
        cossize = turntocos[default_turn] * size;
        loc_foregroundrepeat = water.foregroundrepeat * MIN( x / sdl_scr.x, y / sdl_scr.x );

        vtlist[0].pos[XX] = x + cossize;
        vtlist[0].pos[YY] = y - sinsize;
        vtlist[0].pos[ZZ] = z;
        vtlist[0].tex[SS] = ilayer->tx.x;
        vtlist[0].tex[TT] = ilayer->tx.y;

        vtlist[1].pos[XX] = x + sinsize;
        vtlist[1].pos[YY] = y + cossize;
        vtlist[1].pos[ZZ] = z;
        vtlist[1].tex[SS] = ilayer->tx.x + loc_foregroundrepeat;
        vtlist[1].tex[TT] = ilayer->tx.y;

        vtlist[2].pos[XX] = x - cossize;
        vtlist[2].pos[YY] = y + sinsize;
        vtlist[2].pos[ZZ] = z;
        vtlist[2].tex[SS] = ilayer->tx.x + loc_foregroundrepeat;
        vtlist[2].tex[TT] = ilayer->tx.y + loc_foregroundrepeat;

        vtlist[3].pos[XX] = x - sinsize;
        vtlist[3].pos[YY] = y - cossize;
        vtlist[3].pos[ZZ] = z;
        vtlist[3].tex[SS] = ilayer->tx.x;
        vtlist[3].tex[TT] = ilayer->tx.y + loc_foregroundrepeat;

        ptex = TxTexture_get_valid_ptr( texture );

        ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT | GL_HINT_BIT );
        {
            // make sure that the texture is as smooth as possible
            GL_DEBUG( glHint )( GL_POLYGON_SMOOTH_HINT, GL_NICEST );          // GL_HINT_BIT

            // flat shading
            GL_DEBUG( glShadeModel )( GL_FLAT );                             // GL_LIGHTING_BIT

            // don't write into the depth buffer (disable glDepthMask for transparent objects)
            GL_DEBUG( glDepthMask )( GL_FALSE );                             // GL_DEPTH_BUFFER_BIT

            // essentially disable the depth test without calling glDisable( GL_DEPTH_TEST )
            GL_DEBUG( glEnable )( GL_DEPTH_TEST );                           // GL_ENABLE_BIT
            GL_DEBUG( glDepthFunc )( GL_ALWAYS );                            // GL_DEPTH_BUFFER_BIT

            // draw draw front and back faces of polygons
            oglx_end_culling();                           // GL_ENABLE_BIT

            // do not display the completely transparent portion
            GL_DEBUG( glEnable )( GL_ALPHA_TEST );                            // GL_ENABLE_BIT
            GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );                      // GL_COLOR_BUFFER_BIT

            // make the texture a filter
            GL_DEBUG( glEnable )( GL_BLEND );                                 // GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );  // GL_COLOR_BUFFER_BIT

            oglx_texture_Bind( ptex );

            GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - ABS( alpha ) );
            GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
            for ( i = 0; i < 4; i++ )
            {
                GL_DEBUG( glTexCoord2fv )( vtlist[i].tex );
                GL_DEBUG( glVertex3fv )( vtlist[i].pos );
            }
            GL_DEBUG_END();
        }
        ATTRIB_POP( __FUNCTION__ );
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_water( renderlist_t * prlist )
{
    /// @author ZZ
    /// @details This function draws all of the water fans

    int cnt;
    gfx_rv retval;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    // Bottom layer first
    if ( gfx.draw_water_1 )
    {
        for ( cnt = 0; cnt < prlist->wat_count; cnt++ )
        {
            if ( gfx_error == render_water_fan( prlist->pmesh, prlist->wat[cnt], 1 ) )
            {
                retval = gfx_error;
            }
        }
    }

    // Top layer second
    if ( gfx.draw_water_0 )
    {
        for ( cnt = 0; cnt < prlist->wat_count; cnt++ )
        {
            if ( gfx_error == render_water_fan( prlist->pmesh, prlist->wat[cnt], 0 ) )
            {
                retval = gfx_error;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// gfx_config_t FUNCTIONS
//--------------------------------------------------------------------------------------------
bool_t gfx_config_download_from_egoboo_config( gfx_config_t * pgfx, egoboo_config_t * pcfg )
{
    // call gfx_config_init(), even if the config data is invalid
    if ( !gfx_config_init( pgfx ) ) return bfalse;

    // if there is no config data, do not proceed
    if ( NULL == pcfg ) return bfalse;

    pgfx->antialiasing = pcfg->multisamples > 0;

    pgfx->refon        = pcfg->reflect_allowed;
    pgfx->reffadeor    = pcfg->reflect_fade ? 0 : 255;

    pgfx->shaon        = pcfg->shadow_allowed;
    pgfx->shasprite    = pcfg->shadow_sprite;

    pgfx->shading      = pcfg->gouraud_req ? GL_SMOOTH : GL_FLAT;
    pgfx->dither       = pcfg->use_dither;
    pgfx->perspective  = pcfg->use_perspective;
    pgfx->phongon      = pcfg->use_phong;

    pgfx->draw_background = pcfg->background_allowed && water.background_req;
    pgfx->draw_overlay    = pcfg->overlay_allowed && water.overlay_req;

    pgfx->dynalist_max = CLIP( pcfg->dyna_count_req, 0, TOTAL_MAX_DYNA );

    pgfx->draw_water_0 = !pgfx->draw_overlay && ( water.layer_count > 0 );
    pgfx->clearson     = !pgfx->draw_background;
    pgfx->draw_water_1 = !pgfx->draw_background && ( water.layer_count > 1 );

    gfx_system_set_virtual_screen( pgfx );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t gfx_config_init( gfx_config_t * pgfx )
{
    if ( NULL == pgfx ) return bfalse;

    pgfx->shading          = GL_SMOOTH;
    pgfx->refon            = btrue;
    pgfx->reffadeor        = 0;
    pgfx->antialiasing     = bfalse;
    pgfx->dither           = bfalse;
    pgfx->perspective      = bfalse;
    pgfx->phongon          = btrue;
    pgfx->shaon            = btrue;
    pgfx->shasprite        = btrue;

    pgfx->clearson         = btrue;   // Do we clear every time?
    pgfx->draw_background  = bfalse;   // Do we draw the background image?
    pgfx->draw_overlay     = bfalse;   // Draw overlay?
    pgfx->draw_water_0     = btrue;   // Do we draw water layer 1 (TX_WATER_LOW)
    pgfx->draw_water_1     = btrue;   // Do we draw water layer 2 (TX_WATER_TOP)

    pgfx->dynalist_max    = 8;

    //gfx_calc_rotmesh();

    return btrue;
}

//--------------------------------------------------------------------------------------------
// oglx_texture_parameters_t FUNCTIONS
//--------------------------------------------------------------------------------------------
bool_t oglx_texture_parameters_download_gfx( oglx_texture_parameters_t * ptex, egoboo_config_t * pcfg )
{
    /// @author BB
    /// @details synch the texture parameters with the video mode

    if ( NULL == ptex || NULL == pcfg ) return bfalse;

    if ( ogl_caps.maxAnisotropy <= 1.0f )
    {
        ptex->userAnisotropy = 0.0f;
        ptex->texturefilter  = ( TX_FILTERS )MIN( pcfg->texturefilter_req, TX_TRILINEAR_2 );
    }
    else
    {
        ptex->texturefilter  = ( TX_FILTERS )MIN( pcfg->texturefilter_req, TX_FILTER_COUNT );
        ptex->userAnisotropy = ogl_caps.maxAnisotropy * MAX( 0, ( int )ptex->texturefilter - ( int )TX_TRILINEAR_2 );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
// gfx_error FUNCTIONS
//--------------------------------------------------------------------------------------------
egolib_rv gfx_error_add( const char * file, const char * function, int line, int id, const char * sz )
{
    gfx_error_state_t * pstate;

    // too many errors?
    if ( gfx_error_stack.count >= GFX_ERROR_MAX ) return rv_fail;

    // grab an error state
    pstate = gfx_error_stack.lst + gfx_error_stack.count;
    gfx_error_stack.count++;

    // where is the error
    strncpy( pstate->file,     file,     SDL_arraysize( pstate->file ) );
    strncpy( pstate->function, function, SDL_arraysize( pstate->function ) );
    pstate->line = line;

    // what is the error
    pstate->type     = id;
    strncpy( pstate->string, sz, SDL_arraysize( pstate->string ) );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
gfx_error_state_t * gfx_error_pop()
{
    gfx_error_state_t * retval;

    if ( 0 == gfx_error_stack.count || gfx_error_stack.count >= GFX_ERROR_MAX ) return NULL;

    retval = gfx_error_stack.lst + gfx_error_stack.count;
    gfx_error_stack.count--;

    return retval;
}

//--------------------------------------------------------------------------------------------
void gfx_error_clear()
{
    gfx_error_stack.count = 0;
}

//--------------------------------------------------------------------------------------------
// grid_lighting FUNCTIONS
//--------------------------------------------------------------------------------------------
float grid_lighting_test( ego_mpd_t * pmesh, GLXvector3f pos, float * low_diff, float * hgh_diff )
{
    int ix, iy, cnt;
    Uint32 fan[4];
    float u, v;

    const lighting_cache_t * cache_list[4];
    ego_grid_info_t  * pgrid;

    if ( NULL == pmesh ) pmesh = PMesh;
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid mesh" );
        return 0.0f;
    }

    ix = FLOOR( pos[XX] / GRID_FSIZE );
    iy = FLOOR( pos[YY] / GRID_FSIZE );

    fan[0] = mesh_get_tile_int( pmesh, ix,     iy );
    fan[1] = mesh_get_tile_int( pmesh, ix + 1, iy );
    fan[2] = mesh_get_tile_int( pmesh, ix,     iy + 1 );
    fan[3] = mesh_get_tile_int( pmesh, ix + 1, iy + 1 );

    for ( cnt = 0; cnt < 4; cnt++ )
    {
        cache_list[cnt] = NULL;

        pgrid = mesh_get_pgrid( pmesh, fan[cnt] );
        if ( NULL == pgrid )
        {
            cache_list[cnt] = NULL;
        }
        else
        {
            cache_list[cnt] = &( pgrid->cache );
        }
    }

    u = pos[XX] / GRID_FSIZE - ix;
    v = pos[YY] / GRID_FSIZE - iy;

    return lighting_cache_test( cache_list, u, v, low_diff, hgh_diff );
}

//--------------------------------------------------------------------------------------------
bool_t grid_lighting_interpolate( const ego_mpd_t * pmesh, lighting_cache_t * dst, const fvec2_base_t pos )
{
    int ix, iy, cnt;
    Uint32 fan[4];
    float u, v;
    fvec2_t tpos;

    ego_grid_info_t  * pgrid;
    const lighting_cache_t * cache_list[4];

    if ( NULL == pmesh ) pmesh = PMesh;
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid mesh" );
        return bfalse;
    }

    if ( NULL == dst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "no valid lighting cache" );
        return bfalse;
    }

    if ( NULL == pos )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "no valid position" );
        return bfalse;
    }

    // calculate the "tile position"
    fvec2_scale( tpos.v, pos, 1.0f / GRID_FSIZE );

    // grab this tile's coordinates
    ix = FLOOR( tpos.x );
    iy = FLOOR( tpos.y );

    // find the tile id for the surrounding tiles
    fan[0] = mesh_get_tile_int( pmesh, ix,     iy );
    fan[1] = mesh_get_tile_int( pmesh, ix + 1, iy );
    fan[2] = mesh_get_tile_int( pmesh, ix,     iy + 1 );
    fan[3] = mesh_get_tile_int( pmesh, ix + 1, iy + 1 );

    for ( cnt = 0; cnt < 4; cnt++ )
    {
        pgrid = mesh_get_pgrid( pmesh, fan[cnt] );

        if ( NULL == pgrid )
        {
            cache_list[cnt] = NULL;
        }
        else
        {
            cache_list[cnt] = &( pgrid->cache );
        }
    }

    // grab the coordinates relative to the parent tile
    u = tpos.x - ix;
    v = tpos.y - iy;

    return lighting_cache_interpolate( dst, cache_list, u, v );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void gfx_update_fps_clock()
{
    /// @author ZZ
    /// @details This function updates the graphics timers

    int gfx_clock_diff;

    bool_t free_running = btrue;

    // are the graphics updates free running?
    free_running = btrue;
    if ( process_running( PROC_PBASE( GProc ) ) )
    {
        free_running = GProc->fps_timer.free_running;
    }
    else if ( process_running( PROC_PBASE( MProc ) ) )
    {
        free_running = MProc->gui_timer.free_running;
    }

    // make sure at least one tick has passed
    if ( !egolib_timer_throttle( &gfx_update_timer, 1 ) ) return;

    // calculate the time since the from the last update
    if ( !single_frame_mode )
    {
        // update the graphics clock the normal way
        egolib_throttle_update( &gfx_throttle );
    }
    else
    {
        // do a single-frame update
        egolib_throttle_update_diff( &gfx_throttle, TICKS_PER_SEC / ( float )cfg.framelimit );
    }

    // measure the time change
    gfx_clock_diff = gfx_throttle.time_now - gfx_throttle.time_lst;

    // update the game fps
    if ( process_running( PROC_PBASE( GProc ) ) )
    {
        game_fps_clock += gfx_clock_diff;
    }

    // update the menu fps
    if ( process_running( PROC_PBASE( MProc ) ) )
    {
        menu_fps_clock += gfx_clock_diff;
    }
}

//--------------------------------------------------------------------------------------------
void gfx_update_fps()
{
    // at fold = 0.60f, it will take approximately 9 updates for the
    // weight of the first value to be reduced to 1%
    const float fold = STABILIZED_KEEP;
    const float fnew = STABILIZED_COVER;

    // update the clock
    gfx_update_fps_clock();

    // update the game fps
    if ( process_running( PROC_PBASE( GProc ) ) )
    {
        if ( game_fps_loops > 0 && game_fps_clock > 0 )
        {
            stabilized_game_fps_sum    = fold * stabilized_game_fps_sum    + fnew * ( float ) game_fps_loops / (( float ) game_fps_clock / TICKS_PER_SEC );
            stabilized_game_fps_weight = fold * stabilized_game_fps_weight + fnew;

            // Don't allow the counters to overflow. 0x15555555 is 1/3 of the maximum Sint32 value
            if ( game_fps_loops > 0x15555555 || game_fps_clock > 0x15555555 )
            {
                game_fps_loops = 0;
                game_fps_clock = 0;
            }
        };

        if ( stabilized_game_fps_weight > 0.5f )
        {
            stabilized_game_fps = stabilized_game_fps_sum / stabilized_game_fps_weight;
        }
    }

    // update the menu fps
    if ( process_running( PROC_PBASE( MProc ) ) )
    {
        if ( menu_fps_loops > 0 && menu_fps_clock > 0 )
        {
            stabilized_menu_fps_sum    = fold * stabilized_menu_fps_sum    + fnew * ( float ) menu_fps_loops / (( float ) menu_fps_clock / TICKS_PER_SEC );
            stabilized_menu_fps_weight = fold * stabilized_menu_fps_weight + fnew;

            // Don't allow the counters to overflow. 0x15555555 is 1/3 of the maximum Sint32 value
            if ( menu_fps_loops > 0x15555555 || menu_fps_clock > 0x15555555 )
            {
                menu_fps_loops = 0;
                menu_fps_clock = 0;
            }
        };

        if ( stabilized_menu_fps_weight > 0.5f )
        {
            stabilized_menu_fps = stabilized_menu_fps_sum / stabilized_menu_fps_weight;
        }
    }

    // choose the correct fps to display
    if ( process_running( PROC_PBASE( GProc ) ) )
    {
        stabilized_fps = stabilized_game_fps;
    }
    else if ( process_running( PROC_PBASE( MProc ) ) )
    {
        stabilized_fps = stabilized_menu_fps;
    }
}

//--------------------------------------------------------------------------------------------
// LINE IMPLENTATION
//--------------------------------------------------------------------------------------------
void line_list_init()
{
    /// @details  BB@> initialize the list so that no lines are valid

    int cnt;

    for ( cnt = 0; cnt < LINE_COUNT; cnt++ )
    {
        line_list[cnt].time = -1;
    }
}

//--------------------------------------------------------------------------------------------
int line_list_get_free()
{
    /// @details  BB@> get the 1st free line

    int cnt;

    for ( cnt = 0; cnt < LINE_COUNT; cnt++ )
    {
        if ( line_list[cnt].time < 0 )
        {
            break;
        }
    }

    return cnt < LINE_COUNT ? cnt : -1;
}

//--------------------------------------------------------------------------------------------
bool_t line_list_add( const float src_x, const float src_y, const float src_z, const float pos_x, const float dst_y, const float dst_z, const int duration )
{
    int iline = line_list_get_free();

    if ( iline == LINE_COUNT ) return bfalse;

    //Source
    line_list[iline].src.x = src_x;
    line_list[iline].src.y = src_y;
    line_list[iline].src.z = src_z;

    //Destination
    line_list[iline].dst.x = pos_x;
    line_list[iline].dst.y = dst_y;
    line_list[iline].dst.z = dst_z;

    //White color
    line_list[iline].color.r = 1.00f;
    line_list[iline].color.g = 1.00f;
    line_list[iline].color.b = 1.00f;
    line_list[iline].color.a = 0.50f;

    line_list[iline].time = egoboo_get_ticks() + duration;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void line_list_draw_all( const camera_t * pcam )
{
    /// @author BB
    /// @details draw some lines for debugging purposes

    int cnt, ticks;

    GLboolean texture_1d_enabled, texture_2d_enabled;

    texture_1d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_1D );
    texture_2d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_2D );

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    if ( texture_1d_enabled ) GL_DEBUG( glDisable )( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glDisable )( GL_TEXTURE_2D );

    gfx_begin_3d( pcam );
    {
        ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
        {
            // flat shading
            GL_DEBUG( glShadeModel )( GL_FLAT );     // GL_LIGHTING_BIT

            // don't write into the depth buffer (disable glDepthMask for transparent objects)
            GL_DEBUG( glDepthMask )( GL_FALSE );     // GL_DEPTH_BUFFER_BIT

            // do not draw hidden surfaces
            GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT
            GL_DEBUG( glDepthFunc )( GL_LEQUAL );    // GL_DEPTH_BUFFER_BIT

            // draw draw front and back faces of polygons
            oglx_end_culling();   // GL_ENABLE_BIT

            GL_DEBUG( glDisable )( GL_BLEND );       // GL_ENABLE_BIT

            // we do not want texture mapped lines
            GL_DEBUG( glDisable )( GL_TEXTURE_2D );  // GL_ENABLE_BIT

            ticks = egoboo_get_ticks();

            for ( cnt = 0; cnt < LINE_COUNT; cnt++ )
            {
                if ( line_list[cnt].time < 0 ) continue;

                if ( line_list[cnt].time < ticks )
                {
                    line_list[cnt].time = -1;
                    continue;
                }

                GL_DEBUG( glColor4fv )( line_list[cnt].color.v );       // GL_CURRENT_BIT
                GL_DEBUG( glBegin )( GL_LINES );
                {
                    GL_DEBUG( glVertex3fv )( line_list[cnt].src.v );
                    GL_DEBUG( glVertex3fv )( line_list[cnt].dst.v );
                }
                GL_DEBUG_END();
            }
        }
        ATTRIB_POP( __FUNCTION__ );
    }
    gfx_end_3d();

    // fix the texture enabling
    if ( texture_1d_enabled )
    {
        GL_DEBUG( glEnable )( GL_TEXTURE_1D );
    }
    else if ( texture_2d_enabled )
    {
        GL_DEBUG( glEnable )( GL_TEXTURE_2D );
    }

}

//--------------------------------------------------------------------------------------------
// POINT IMPLENTATION
//--------------------------------------------------------------------------------------------
void point_list_init()
{
    /// @details  BB@> initialize the list so that no points are valid

    int cnt;

    for ( cnt = 0; cnt < POINT_COUNT; cnt++ )
    {
        point_list[cnt].time = -1;
    }
}

//--------------------------------------------------------------------------------------------
bool_t point_list_add( const float x, const float y, const float z, const int duration )
{
    int ipoint = point_list_get_free();

    if ( ipoint == POINT_COUNT ) return bfalse;

    //position
    point_list[ipoint].src.x = x;
    point_list[ipoint].src.y = y;
    point_list[ipoint].src.z = z;

    //Red color
    point_list[ipoint].color.r = 1.00f;
    point_list[ipoint].color.g = 0.00f;
    point_list[ipoint].color.b = 0.00f;
    point_list[ipoint].color.a = 0.50f;

    point_list[ipoint].time = egoboo_get_ticks() + duration;

    return btrue;
}

//--------------------------------------------------------------------------------------------
int point_list_get_free()
{
    int cnt;

    for ( cnt = 0; cnt < POINT_COUNT; cnt++ )
    {
        if ( point_list[cnt].time < 0 )
        {
            break;
        }
    }

    return cnt < POINT_COUNT ? cnt : -1;
}

//--------------------------------------------------------------------------------------------
void point_list_draw_all( const camera_t * pcam )
{
    /// @author BB
    /// @details draw some points for debugging purposes

    int cnt, ticks;

    GLboolean texture_1d_enabled, texture_2d_enabled;

    texture_1d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_1D );
    texture_2d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_2D );

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    if ( texture_1d_enabled ) GL_DEBUG( glDisable )( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glDisable )( GL_TEXTURE_2D );

    gfx_begin_3d( pcam );
    {
        ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
        {
            // flat shading
            GL_DEBUG( glShadeModel )( GL_FLAT );     // GL_LIGHTING_BIT

            // don't write into the depth buffer (disable glDepthMask for transparent objects)
            GL_DEBUG( glDepthMask )( GL_FALSE );     // GL_DEPTH_BUFFER_BIT

            // do not draw hidden surfaces
            GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT
            GL_DEBUG( glDepthFunc )( GL_LEQUAL );    // GL_DEPTH_BUFFER_BIT

            // draw draw front and back faces of polygons
            oglx_end_culling();   // GL_ENABLE_BIT

            GL_DEBUG( glDisable )( GL_BLEND );       // GL_ENABLE_BIT

            // we do not want texture mapped points
            GL_DEBUG( glDisable )( GL_TEXTURE_2D );  // GL_ENABLE_BIT

            ticks = egoboo_get_ticks();

            for ( cnt = 0; cnt < POINT_COUNT; cnt++ )
            {
                if ( point_list[cnt].time < 0 ) continue;

                if ( point_list[cnt].time < ticks )
                {
                    point_list[cnt].time = -1;
                    continue;
                }

                GL_DEBUG( glColor4fv )( point_list[cnt].color.v );       // GL_CURRENT_BIT
                GL_DEBUG( glBegin )( GL_POINTS );
                {
                    GL_DEBUG( glVertex3fv )( point_list[cnt].src.v );
                }
                GL_DEBUG_END();
            }
        }
        ATTRIB_POP( __FUNCTION__ );
    }
    gfx_end_3d();

    // fix the texture enabling
    if ( texture_1d_enabled )
    {
        GL_DEBUG( glEnable )( GL_TEXTURE_1D );
    }
    else if ( texture_2d_enabled )
    {
        GL_DEBUG( glEnable )( GL_TEXTURE_2D );
    }
}

//--------------------------------------------------------------------------------------------
// AXIS BOUNDING BOX IMPLEMENTATION(S)
//--------------------------------------------------------------------------------------------
bool_t render_aabb( aabb_t * pbbox )
{
    GLXvector3f * pmin, * pmax;
    GLint matrix_mode[1];

    if ( NULL == pbbox ) return bfalse;

    // save the matrix mode
    GL_DEBUG( glGetIntegerv )( GL_MATRIX_MODE, matrix_mode );

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    {
        pmin = &( pbbox->mins );
        pmax = &( pbbox->maxs );

        // !!!! there must be an optimized way of doing this !!!!

        GL_DEBUG( glBegin )( GL_QUADS );
        {
            // Front Face
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );

            // Back Face
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );

            // Top Face
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );

            // Bottom Face
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );

            // Right face
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );

            // Left Face
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
        }
        GL_DEBUG_END();
    }
    // Restore the GL_MODELVIEW matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // restore the matrix mode
    GL_DEBUG( glMatrixMode )( matrix_mode[0] );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t render_oct_bb( oct_bb_t * bb, bool_t draw_square, bool_t draw_diamond )
{
    bool_t retval = bfalse;

    if ( NULL == bb ) return bfalse;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT );
    {
        // don't write into the depth buffer (disable glDepthMask for transparent objects)
        GL_DEBUG( glDepthMask )( GL_FALSE );

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );

        // fix the poorly chosen normals...
        // draw draw front and back faces of polygons
        oglx_end_culling();                 // GL_ENABLE_BIT

        // make them transparent
        GL_DEBUG( glEnable )( GL_BLEND );
        GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        // choose a "white" texture
        oglx_texture_Bind( NULL );

        //------------------------------------------------
        // DIAGONAL BBOX
        if ( draw_diamond )
        {
            float p1_x, p1_y;
            float p2_x, p2_y;

            GL_DEBUG( glColor4f )( 0.5f, 1.0f, 1.0f, 0.1f );

            p1_x = 0.5f * ( bb->maxs[OCT_XY] - bb->maxs[OCT_YX] );
            p1_y = 0.5f * ( bb->maxs[OCT_XY] + bb->maxs[OCT_YX] );
            p2_x = 0.5f * ( bb->maxs[OCT_XY] - bb->mins[OCT_YX] );
            p2_y = 0.5f * ( bb->maxs[OCT_XY] + bb->mins[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb->maxs[OCT_XY] - bb->mins[OCT_YX] );
            p1_y = 0.5f * ( bb->maxs[OCT_XY] + bb->mins[OCT_YX] );
            p2_x = 0.5f * ( bb->mins[OCT_XY] - bb->mins[OCT_YX] );
            p2_y = 0.5f * ( bb->mins[OCT_XY] + bb->mins[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb->mins[OCT_XY] - bb->mins[OCT_YX] );
            p1_y = 0.5f * ( bb->mins[OCT_XY] + bb->mins[OCT_YX] );
            p2_x = 0.5f * ( bb->mins[OCT_XY] - bb->maxs[OCT_YX] );
            p2_y = 0.5f * ( bb->mins[OCT_XY] + bb->maxs[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb->mins[OCT_XY] - bb->maxs[OCT_YX] );
            p1_y = 0.5f * ( bb->mins[OCT_XY] + bb->maxs[OCT_YX] );
            p2_x = 0.5f * ( bb->maxs[OCT_XY] - bb->maxs[OCT_YX] );
            p2_y = 0.5f * ( bb->maxs[OCT_XY] + bb->maxs[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            retval = btrue;
        }

        //------------------------------------------------
        // SQUARE BBOX
        if ( draw_square )
        {
            GL_DEBUG( glColor4f )( 1.0f, 0.5f, 1.0f, 0.1f );

            // XZ FACE, min Y
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // YZ FACE, min X
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // XZ FACE, max Y
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // YZ FACE, max X
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // XY FACE, min Z
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // XY FACE, max Z
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            retval = btrue;
        }

    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
// obj_registry_entity_t IMPLEMENTATION
//--------------------------------------------------------------------------------------------

obj_registry_entity_t * obj_registry_entity_init( obj_registry_entity_t * ptr )
{
    if ( NULL == ptr ) return NULL;

    BLANK_STRUCT_PTR( ptr )

    ptr->ichr = MAX_CHR;
    ptr->iprt = MAX_PRT;

    return ptr;
}

//--------------------------------------------------------------------------------------------
int obj_registry_entity_cmp( const void * pleft, const void * pright )
{
    obj_registry_entity_t * dleft  = ( obj_registry_entity_t * ) pleft;
    obj_registry_entity_t * dright = ( obj_registry_entity_t * ) pright;

    int   rv;
    float diff;

    diff = dleft->dist - dright->dist;

    if ( diff < 0.0f )
    {
        rv = -1;
    }
    else if ( diff > 0.0f )
    {
        rv = 1;
    }
    else
    {
        rv = 0;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
// DisplayMsg IMPLEMENTATION
//--------------------------------------------------------------------------------------------
int DisplayMsg_get_free()
{
    /// @author ZZ
    /// @details This function finds the best message to use
    /// Pick the first one

    int tnc = DisplayMsg.count;

    DisplayMsg.count++;
    DisplayMsg.count %= maxmessage;

    return tnc;
}

//--------------------------------------------------------------------------------------------
// ASSET INITIALIZATION
//--------------------------------------------------------------------------------------------

void gfx_init_bar_data()
{
    Uint8 cnt;

    // Initialize the life and mana bars
    for ( cnt = 0; cnt < NUMBAR; cnt++ )
    {
        tabrect[cnt].left = 0;
        tabrect[cnt].right = TABX;
        tabrect[cnt].top = cnt * BARY;
        tabrect[cnt].bottom = ( cnt + 1 ) * BARY;

        barrect[cnt].left = TABX;
        barrect[cnt].right = BARX;  // This is reset whenever a bar is drawn
        barrect[cnt].top = tabrect[cnt].top;
        barrect[cnt].bottom = tabrect[cnt].bottom;

    }
}

//--------------------------------------------------------------------------------------------
void gfx_init_blip_data()
{
    int cnt;

    // Set up the rectangles
    for ( cnt = 0; cnt < COLOR_MAX; cnt++ )
    {
        bliprect[cnt].left   = cnt * BLIPSIZE;
        bliprect[cnt].right  = cnt * BLIPSIZE + BLIPSIZE;
        bliprect[cnt].top    = 0;
        bliprect[cnt].bottom = BLIPSIZE;
    }

    youarehereon = bfalse;
    blip_count      = 0;
}

//--------------------------------------------------------------------------------------------
void gfx_init_map_data()
{
    /// @author ZZ
    /// @details This function releases all the map images

    // Set up the rectangles
    maprect.left   = 0;
    maprect.right  = MAPSIZE;
    maprect.top    = 0;
    maprect.bottom = MAPSIZE;

    mapvalid = bfalse;
    mapon    = bfalse;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_load_bars()
{
    /// @author ZZ
    /// @details This function loads the status bar bitmap

    const char * pname = EMPTY_CSTR;
    TX_REF load_rv = INVALID_GL_ID;
    gfx_rv retval  = gfx_success;

    pname = "mp_data/bars";
    load_rv = TxTexture_load_one_vfs( pname, ( TX_REF )TX_BARS, TRANSCOLOR );
    if ( INVALID_TX_TEXTURE == load_rv )
    {
        log_warning( "%s - Cannot load file! (\"%s\")\n", __FUNCTION__, pname );
        retval = gfx_fail;
    }

    pname = "mp_data/xpbar";
    load_rv = TxTexture_load_one_vfs( pname, ( TX_REF )TX_XP_BAR, TRANSCOLOR );
    if ( INVALID_TX_TEXTURE == load_rv )
    {
        log_warning( "%s - Cannot load file! (\"%s\")\n", __FUNCTION__, pname );
        retval = gfx_fail;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_load_map()
{
    /// @author ZZ
    /// @details This function loads the map bitmap

    const char* szMap = "mp_data/plan";
    TX_REF load_rv = INVALID_GL_ID;
    gfx_rv retval  = gfx_success;

    // Turn it all off
    mapon        = bfalse;
    youarehereon = bfalse;
    blip_count   = 0;

    // Load the images
    load_rv = TxTexture_load_one_vfs( szMap, ( TX_REF )TX_MAP, INVALID_KEY );
    if ( INVALID_TX_TEXTURE == load_rv )
    {
        log_debug( "%s - Cannot load file! (\"%s\")\n", __FUNCTION__, szMap );
        retval   = gfx_fail;
        mapvalid = bfalse;
    }
    else
    {
        retval   = gfx_success;
        mapvalid = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_load_blips()
{
    /// @author ZZ
    /// @details This function loads the blip bitmaps

    const char * pname = EMPTY_CSTR;
    TX_REF load_rv = INVALID_GL_ID;
    gfx_rv retval  = gfx_success;

    pname = "mp_data/blip";
    load_rv = TxTexture_load_one_vfs( pname, ( TX_REF )TX_BLIP, INVALID_KEY );
    if ( INVALID_TX_TEXTURE == load_rv )
    {
        log_warning( "%s - Blip bitmap not loaded! (\"%s\")\n", __FUNCTION__, pname );
        retval = gfx_fail;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_load_icons()
{
    const char * pname = EMPTY_CSTR;
    TX_REF load_rv = INVALID_GL_ID;
    gfx_rv retval  = gfx_success;

    pname = "mp_data/nullicon";
    load_rv = TxTexture_load_one_vfs( pname, ( TX_REF )TX_ICON_NULL, INVALID_KEY );
    if ( INVALID_TX_TEXTURE == load_rv )
    {
        log_warning( "%s - cannot load \"empty hand\" icon! (\"%s\")\n", __FUNCTION__, pname );
        retval = gfx_fail;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// MODE CONTROL
//--------------------------------------------------------------------------------------------
void gfx_begin_text()
{
    // do not use the ATTRIB_PUSH macro, since the glPopAttrib() is in a different function
    GL_DEBUG( glPushAttrib )( GL_CURRENT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );

    GL_DEBUG( glEnable )( GL_TEXTURE_2D );

    // do not display the completely transparent portion
    GL_DEBUG( glEnable )( GL_ALPHA_TEST );                               // GL_ENABLE_BIT
    GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );                            // GL_COLOR_BUFFER_BIT

    GL_DEBUG( glEnable )( GL_BLEND );                                    // GL_COLOR_BUFFER_BIT
    GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );     // GL_COLOR_BUFFER_BIT

    // don't worry about hidden surfaces
    GL_DEBUG( glDisable )( GL_DEPTH_TEST );                              // GL_ENABLE_BIT

    // draw draw front and back faces of polygons
    oglx_end_culling();                               // GL_ENABLE_BIT

    GL_DEBUG( glColor4f )( 1, 1, 1, 1 );                                // GL_CURRENT_BIT
}

//--------------------------------------------------------------------------------------------
void gfx_end_text()
{
    // do not use the ATTRIB_POP macro, since the glPushAttrib() is in a different function
    GL_DEBUG( glPopAttrib )();
}

//--------------------------------------------------------------------------------------------
void gfx_enable_texturing()
{
    if ( !GL_DEBUG( glIsEnabled )( GL_TEXTURE_2D ) )
    {
        GL_DEBUG( glEnable )( GL_TEXTURE_2D );
    }
}

//--------------------------------------------------------------------------------------------
void gfx_disable_texturing()
{
    if ( GL_DEBUG( glIsEnabled )( GL_TEXTURE_2D ) )
    {
        GL_DEBUG( glDisable )( GL_TEXTURE_2D );
    }
}

//--------------------------------------------------------------------------------------------
void gfx_begin_3d( const camera_t * pcam )
{
    // store the GL_PROJECTION matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glLoadMatrixf )( pcam->mProjection.v );

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glLoadMatrixf )( pcam->mView.v );
}

//--------------------------------------------------------------------------------------------
void gfx_end_3d()
{
    // Restore the GL_MODELVIEW matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // Restore the GL_PROJECTION matrix
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPopMatrix )();
}

//--------------------------------------------------------------------------------------------
void gfx_begin_2d( void )
{

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_SCISSOR_BIT );

    // Reset the Projection Matrix
    // Set up an orthogonal projection
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glLoadIdentity )();
    GL_DEBUG( glOrtho )( 0, sdl_scr.x, sdl_scr.y, 0, -1, 1 );

    // Reset the Modelview Matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glLoadIdentity )();

    // remove any scissor test
    GL_DEBUG( glDisable )( GL_SCISSOR_TEST );

    // don't worry about hidden surfaces
    GL_DEBUG( glDisable )( GL_DEPTH_TEST );

    // stop culling backward facing polygons
    oglx_end_culling();                            // GL_ENABLE_BIT
}

//--------------------------------------------------------------------------------------------
void gfx_end_2d( void )
{
    // get the old modelview matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // get the old projection matrix
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPopMatrix )();

    // ATTRIB_POP()
    // - restores the culling mode
    // - restores the culling depth-testing mode
    // - restores the SCISSOR mode
    ATTRIB_POP( __FUNCTION__ );

    // leave the MatrixMode in GL_MODELVIEW
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
}

//--------------------------------------------------------------------------------------------
void gfx_reshape_viewport( int w, int h )
{
    GL_DEBUG( glViewport )( 0, 0, w, h );
}

//--------------------------------------------------------------------------------------------
void gfx_request_clear_screen()
{
    gfx_page_clear_requested = btrue;
}

//--------------------------------------------------------------------------------------------
void gfx_do_clear_screen()
{
    bool_t game_needs_clear, menu_needs_clear;

    if ( !gfx_page_clear_requested ) return;

    // clear the depth buffer if anything was drawn
    GL_DEBUG( glDepthMask )( GL_TRUE );
    GL_DEBUG( glClear )( GL_DEPTH_BUFFER_BIT );

    // does the game need a clear?
    game_needs_clear = bfalse;
    if ( process_running( PROC_PBASE( GProc ) ) && PROC_PBASE( GProc )->state > proc_begin )
    {
        game_needs_clear = gfx.clearson;
    }

    // does the menu need a clear?
    menu_needs_clear = bfalse;
    if ( process_running( PROC_PBASE( MProc ) ) && PROC_PBASE( MProc )->state > proc_begin )
    {
        menu_needs_clear = mnu_draw_background;
    }

    // if anything needs a clear, do it
    if ( game_needs_clear || menu_needs_clear )
    {
        GL_DEBUG( glClear )( GL_COLOR_BUFFER_BIT );

        gfx_page_clear_requested = bfalse;

        gfx_clear_loops++;
    }
}

//--------------------------------------------------------------------------------------------
void gfx_do_flip_pages()
{
    bool_t try_flip;

    try_flip = bfalse;
    if ( process_running( PROC_PBASE( GProc ) ) && PROC_PBASE( GProc )->state > proc_begin )
    {
        try_flip = gfx_page_flip_requested;
    }
    else if ( process_running( PROC_PBASE( MProc ) ) && PROC_PBASE( MProc )->state > proc_begin )
    {
        try_flip = gfx_page_flip_requested;
    }

    if ( try_flip )
    {
        gfx_page_flip_requested = bfalse;
        _flip_pages();

        gfx_page_clear_requested = btrue;
    }
}

//--------------------------------------------------------------------------------------------
void gfx_request_flip_pages()
{
    gfx_page_flip_requested = btrue;
}

//--------------------------------------------------------------------------------------------
bool_t gfx_flip_pages_requested()
{
    return gfx_page_flip_requested;
}

//--------------------------------------------------------------------------------------------
void _flip_pages()
{
    //GL_DEBUG( glFlush )();

    // draw the console on top of everything
    egolib_console_draw_all();

    SDL_GL_SwapBuffers();

    if ( process_running( PROC_PBASE( MProc ) ) )
    {
        menu_fps_loops++;
        menu_frame_all++;
    }

    if ( process_running( PROC_PBASE( GProc ) ) )
    {
        game_fps_loops++;
        game_frame_all++;
    }

    // update the frames per second
    gfx_update_fps();

    if ( screenshot_requested )
    {
        screenshot_requested = bfalse;

        // take the screenshot NOW, since we have just updated the screen buffer
        if ( !dump_screenshot() )
        {
            debug_printf( "Error writing screenshot!" );    // send a failure message to the screen
            log_warning( "Error writing screenshot\n" );    // Log the error in log.txt
        }
    }
}

//--------------------------------------------------------------------------------------------
// LIGHTING FUNCTIONS
//--------------------------------------------------------------------------------------------
bool_t light_fans_throttle_update( ego_mpd_t * pmesh, ego_tile_info_t * ptile, int fan, float threshold )
{
    grid_mem_t * pgmem = NULL;
    bool_t       retval = bfalse;

    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "no valid mesh" );
        return gfx_error;
    }
    pgmem = &( pmesh->gmem );

    if ( NULL == ptile )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "no valid tile" );
        return gfx_error;
    }

#if defined(CLIP_LIGHT_FANS) && !defined(CLIP_ALL_LIGHT_FANS)

    // visible fans based on the update "need"
    retval = mesh_test_corners( pmesh, ptile, threshold );

    // update every 4 fans even if there is no need
    if ( !retval )
    {
        int ix, iy;

        // use a kind of checkerboard pattern
        ix = fan % pgmem->grids_x;
        iy = fan / pgmem->grids_x;
        if ( 0 != ((( ix ^ iy ) + game_frame_all ) & 0x03 ) )
        {
            retval = btrue;
        }
    }

#else
    retval = btrue;
#endif

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv light_fans_update_lcache( renderlist_t * prlist )
{
    const int frame_skip = 1 << 2;
#if defined(CLIP_ALL_LIGHT_FANS)
    const int frame_mask = frame_skip - 1;
#endif

    int    entry;
    float  local_mesh_lighting_keep;

    /// @note we are measuring the change in the intensity at the corner of a tile (the "delta") as
    /// a fraction of the current intensity. This is because your eye is much more sensitive to
    /// intensity differences when the intensity is low.
    ///
    /// @note it is normally assumed that 64 colors of gray can make a smoothly colored black and white picture
    /// which means that the threshold could be set as low as 1/64 = 0.015625.
    const float delta_threshold = 0.05f;

    ego_mpd_t      * pmesh;
    tile_mem_t     * ptmem;
    grid_mem_t     * pgmem;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return gfx_error;
    }

    pmesh = renderlist_get_pmesh( prlist );
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist mesh" );
        return gfx_error;
    }

#if defined(CLIP_ALL_LIGHT_FANS)
    // update all visible fans once every 4 frames
    if ( 0 != ( game_frame_all & frame_mask ) ) return gfx_success;
#endif

    ptmem = &( pmesh->tmem );
    pgmem = &( pmesh->gmem );

#if !defined(CLIP_LIGHT_FANS)
    // update only every frame
    local_mesh_lighting_keep = 0.9f;
#else
    // update only every 4 frames
    local_mesh_lighting_keep = POW( 0.9f, frame_skip );
#endif

    // cache the grid lighting
    for ( entry = 0; entry < prlist->all_count; entry++ )
    {
        bool_t reflective;
        int fan;
        float delta;
        ego_tile_info_t * ptile;
        ego_grid_info_t * pgrid;

        // which tile?
        fan = prlist->all[entry];

        // grab a pointer to the tile
        ptile = mesh_get_ptile( pmesh, fan );
        if ( NULL == ptile ) continue;

        // Test to see whether the lcache was already updated
        // - ptile->lcache_frame < 0 means that the cache value is invalid.
        // - ptile->lcache_frame is updated inside mesh_light_corners()
#if defined(CLIP_LIGHT_FANS)
        // clip the updated on each individual tile
        if ( ptile->lcache_frame >= 0 && ( Uint32 )ptile->lcache_frame + frame_skip >= game_frame_all )
#else
        // let the function clip all tile updates
        if ( ptile->lcache_frame >= 0 && ( Uint32 )ptile->lcache_frame >= game_frame_all )
#endif
        {
            continue;
        }

        // did someone else request an update?
        if ( !ptile->request_lcache_update )
        {
            // is someone else did not request an update, do we need an one?
            ptile->request_lcache_update = light_fans_throttle_update( pmesh, ptile, fan, delta_threshold );
        }

        // if there's no need for an update, go to the next tile
        if ( !ptile->request_lcache_update ) continue;

        // is the tile reflective?
        pgrid = mesh_get_pgrid( pmesh, fan );
        reflective = ( 0 != ego_grid_info_test_all_fx( pgrid, MPDFX_DRAWREF ) );

        // light the corners of this tile
        delta = mesh_light_corners( prlist->pmesh, ptile, reflective, local_mesh_lighting_keep );

#if defined(CLIP_LIGHT_FANS)
        // use the actual maximum change in the intensity at a tile corner to
        // signal whether we need to calculate the next stage
        ptile->request_clst_update = ( delta > delta_threshold );
#else
        // make sure that mesh_light_corners() did not return an "error value"
        ptile->request_clst_update = ( delta > 0.0f );
#endif
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv light_fans_update_clst( renderlist_t * prlist )
{
    /// @author BB
    /// @details update the tile's color list, if needed

    gfx_rv retval;
    int numvertices;
    int ivrt, vertex;
    float light;
    int    entry;
    Uint32 fan;

    ego_tile_info_t * ptile = NULL;
    ego_mpd_t       * pmesh = NULL;
    tile_mem_t      * ptmem = NULL;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return gfx_error;
    }

    pmesh = renderlist_get_pmesh( prlist );
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist mesh" );
        return gfx_error;
    }

    // alias the tile memory
    ptmem = &( pmesh->tmem );

    // assume the best
    retval = gfx_success;

    // use the grid to light the tiles
    for ( entry = 0; entry < prlist->all_count; entry++ )
    {
        fan = prlist->all[entry];
        if ( INVALID_TILE == fan ) continue;

        // valid tile?
        ptile = mesh_get_ptile( pmesh, fan );
        if ( NULL == ptile )
        {
            retval = gfx_fail;
            continue;
        }

        // do nothing if this tile has not been marked as needing an update
        if ( !ptile->request_clst_update )
        {
            continue;
        }

        // do nothing if the color list has already been computed this update
        if ( ptile->clst_frame  >= 0 && ( Uint32 )ptile->clst_frame >= game_frame_all )
        {
            continue;
        }

        if ( ptile->type < MPD_FAN_TYPE_MAX )
        {
            numvertices = tile_dict[ptile->type].numvertices;
        }
        else
        {
            numvertices = 4;
        }

        // copy the 1st 4 vertices
        for ( ivrt = 0, vertex = ptile->vrtstart; ivrt < 4; ivrt++, vertex++ )
        {
            GLXvector3f * pcol = ptmem->clst + vertex;

            light = ptile->lcache[ivrt];

            ( *pcol )[RR] = ( *pcol )[GG] = ( *pcol )[BB] = INV_FF * CLIP( light, 0.0f, 255.0f );
        };

        for ( /* nothing */ ; ivrt < numvertices; ivrt++, vertex++ )
        {
            bool_t was_calculated;
            GLXvector3f * pcol, * ppos;

            pcol = ptmem->clst + vertex;
            ppos = ptmem->plst + vertex;

            light = 0;
            was_calculated = mesh_interpolate_vertex( ptmem, ptile, *ppos, &light );
            if ( !was_calculated ) continue;

            ( *pcol )[RR] = ( *pcol )[GG] = ( *pcol )[BB] = INV_FF * CLIP( light, 0.0f, 255.0f );
        };

        // clear out the deltas
        BLANK_ARY( ptile->d1_cache );
        BLANK_ARY( ptile->d2_cache );

        // untag this tile
        ptile->request_clst_update = bfalse;
        ptile->clst_frame          = game_frame_all;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv light_fans( renderlist_t * prlist )
{
    if ( gfx_error == light_fans_update_lcache( prlist ) )
    {
        return gfx_error;
    }

    if ( gfx_error == light_fans_update_clst( prlist ) )
    {
        return gfx_error;
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
float get_ambient_level()
{
    /// @author BB
    /// @details get the actual global ambient level

    float glob_amb, min_amb;

    glob_amb = 0.0f;
    min_amb  = 0.0f;
    if ( gfx.usefaredge )
    {
        // for outside modules, max light_a means bright sunlight
        glob_amb = light_a * 255.0f;
    }
    else
    {
        // for inside modules, max light_a means dingy dungeon lighting
        glob_amb = light_a * 32.0f;
    }

    // determine the minimum ambient, based on darkvision
    min_amb = INVISIBLE / 4;
    if ( local_stats.seedark_mag != 1.0f )
    {
        // start with the global light
        min_amb  = glob_amb;

        // give a iny boost in the case of no light_a
        if ( local_stats.seedark_mag > 0.0f ) min_amb += 1.0f;

        // light_a can be quite dark, so we need a large magnification
        min_amb *= local_stats.seedark_mag * local_stats.seedark_mag;
        min_amb *= local_stats.seedark_mag * local_stats.seedark_mag;
        min_amb *= local_stats.seedark_mag;
    }

    return 255; // MAX( glob_amb, min_amb );
}

//--------------------------------------------------------------------------------------------
bool_t sum_global_lighting( lighting_vector_t lighting )
{
    /// @author BB
    /// @details do ambient lighting. if the module is inside, the ambient lighting
    /// is reduced by up to a factor of 8. It is still kept just high enough
    /// so that ordnary objects will not be made invisible. This was breaking some of the AIs

    int cnt;
    float glob_amb;

    if ( NULL == lighting ) return bfalse;

    glob_amb = get_ambient_level();

    for ( cnt = 0; cnt < LVEC_AMB; cnt++ )
    {
        lighting[cnt] = 0.0f;
    }
    lighting[LVEC_AMB] = glob_amb;

    if ( !gfx.usefaredge ) return btrue;

    // do "outside" directional lighting (i.e. sunlight)
    lighting_vector_sum( lighting, light_nrm, light_d * 255, 0.0f );

    return btrue;
}

//--------------------------------------------------------------------------------------------
// SEMI OBSOLETE FUNCTIONS
//--------------------------------------------------------------------------------------------
void draw_cursor()
{
    /// @author ZZ
    /// @details This function implements a mouse cursor

    oglx_texture_t * tx_ptr = TxMenu_get_valid_ptr(( TX_REF )MENU_FONT_BMP );

    if ( input_cursor.x < 6 )  input_cursor.x = 6;
    if ( input_cursor.x > sdl_scr.x - 16 )  input_cursor.x = sdl_scr.x - 16;

    if ( input_cursor.y < 8 )  input_cursor.y = 8;
    if ( input_cursor.y > sdl_scr.y - 24 )  input_cursor.y = sdl_scr.y - 24;

    // Needed to setup text mode
    gfx_begin_text();
    {
        draw_one_font( tx_ptr, 95, input_cursor.x - 5, input_cursor.y - 7 );
    }
    // Needed when done with text mode
    gfx_end_text();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
gfx_rv dynalist_init( dynalist_t * pdylist )
{
    if ( NULL == pdylist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dynalist" );
        return gfx_error;
    }

    pdylist->count        = 0;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_make_dynalist( dynalist_t * pdylist, const camera_t * pcam )
{
    /// @author ZZ
    /// @details This function figures out which particles are visible, and it sets up dynamic
    ///    lighting

    size_t   tnc;
    fvec3_t  vdist;

    float         distance   = 0.0f;
    dynalight_t * plight     = NULL;

    float         distance_max = 0.0f;
    dynalight_t * plight_max   = NULL;

    // make sure we have a dynalist
    if ( NULL == pdylist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dynalist" );
        return gfx_error;
    }

    // do not update the dynalist more than once a frame
    if ( pdylist->frame >= 0 && ( Uint32 )pdylist->frame >= game_frame_all )
    {
        return gfx_success;
    }

    // Don't really make a list, just set to visible or not
    dynalist_init( pdylist );

    PRT_BEGIN_LOOP_DISPLAY( iprt, prt_bdl )
    {
        dynalight_info_t * pprt_dyna = &( prt_bdl.prt_ptr->dynalight );

        // is the light on?
        if ( !pprt_dyna->on || 0.0f == pprt_dyna->level ) continue;

        // reset the dynalight pointer
        plight = NULL;

        // find the distance to the camera
        fvec3_sub( vdist.v, prt_get_pos_v_const( prt_bdl.prt_ptr ), pcam->track_pos.v );
        distance = vdist.x * vdist.x + vdist.y * vdist.y + vdist.z * vdist.z;

        // insert the dynalight
        if ( pdylist->count < gfx.dynalist_max &&  pdylist->count < TOTAL_MAX_DYNA )
        {
            if ( 0 == pdylist->count )
            {
                distance_max = distance;
            }
            else
            {
                distance_max = MAX( distance_max, distance );
            }

            // grab a new light from the list
            plight = pdylist->lst + pdylist->count;
            pdylist->count++;

            if ( distance_max == distance )
            {
                plight_max = plight;
            }
        }
        else if ( distance < distance_max )
        {
            plight = plight_max;

            // find the new maximum distance
            distance_max = pdylist->lst[0].distance;
            plight_max   = pdylist->lst + 0;
            for ( tnc = 1; tnc < gfx.dynalist_max; tnc++ )
            {
                if ( pdylist->lst[tnc].distance > distance_max )
                {
                    plight_max   = pdylist->lst + tnc;
                    distance_max = plight->distance;
                }
            }
        }

        if ( NULL != plight )
        {
            plight->distance = distance;
            prt_get_pos( prt_bdl.prt_ptr, plight->pos.v );
            plight->level    = pprt_dyna->level;
            plight->falloff  = pprt_dyna->falloff;
        }
    }
    PRT_END_LOOP();

    // the list is updated, so update the frame count
    pdylist->frame = game_frame_all;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv do_grid_lighting( renderlist_t * prlist, dynalist_t * pdylist, const camera_t * pcam )
{
    /// @author ZZ
    /// @details Do all tile lighting, dynamic and global

    size_t cnt;
    Uint32 fan;
    int    tnc, entry;
    int ix, iy;
    float x0, y0, local_keep;
    bool_t needs_dynalight;
    ego_mpd_t * pmesh;

    lighting_vector_t global_lighting;

    size_t               reg_count = 0;
    dynalight_registry_t reg[TOTAL_MAX_DYNA];

    ego_frect_t mesh_bound, light_bound;

    ego_mpd_info_t  * pinfo;
    grid_mem_t      * pgmem;
    tile_mem_t      * ptmem;
    oct_bb_t        * poct;

    dynalight_t fake_dynalight;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return gfx_error;
    }

    if ( NULL == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    pmesh = renderlist_get_pmesh( prlist );
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist mesh" );
        return gfx_error;
    }

    pinfo = &( pmesh->info );
    pgmem = &( pmesh->gmem );
    ptmem = &( pmesh->tmem );

    // find a bounding box for the "frustum"
    mesh_bound.xmin = pgmem->edge_x;
    mesh_bound.xmax = 0;
    mesh_bound.ymin = pgmem->edge_y;
    mesh_bound.ymax = 0;
    for ( entry = 0; entry < prlist->all_count; entry++ )
    {
        fan = prlist->all[entry];
        if ( fan >= pinfo->tiles_count ) continue;

        poct = &( ptmem->tile_list[fan].oct );

        mesh_bound.xmin = MIN( mesh_bound.xmin, poct->mins[OCT_X] );
        mesh_bound.xmax = MAX( mesh_bound.xmax, poct->maxs[OCT_X] );
        mesh_bound.ymin = MIN( mesh_bound.ymin, poct->mins[OCT_Y] );
        mesh_bound.ymax = MAX( mesh_bound.ymax, poct->maxs[OCT_Y] );
    }

    // is the visible mesh list empty?
    if ( mesh_bound.xmin >= mesh_bound.xmax || mesh_bound.ymin >= mesh_bound.ymax )
        return gfx_success;

    // clear out the dynalight registry
    reg_count = 0;

    // refresh the dynamic light list
    gfx_make_dynalist( pdylist, pcam );

    // assume no dynamic lighting
    needs_dynalight = bfalse;

    // assume no "extra help" for systems with only flat lighting
    BLANK_STRUCT( fake_dynalight )

    // initialize the light_bound
    light_bound.xmin = pgmem->edge_x;
    light_bound.xmax = 0;
    light_bound.ymin = pgmem->edge_y;
    light_bound.ymax = 0;

    // make bounding boxes for each dynamic light
    if ( GL_FLAT != gfx.shading )
    {
        for ( cnt = 0; cnt < pdylist->count; cnt++ )
        {
            float radius;
            ego_frect_t ftmp;

            dynalight_t * pdyna = pdylist->lst + cnt;

            if ( pdyna->falloff <= 0.0f || 0.0f == pdyna->level ) continue;

            radius = SQRT( pdyna->falloff * 765.0f * 0.5f );

            // find the intersection with the frustum boundary
            ftmp.xmin = MAX( pdyna->pos.x - radius, mesh_bound.xmin );
            ftmp.xmax = MIN( pdyna->pos.x + radius, mesh_bound.xmax );
            ftmp.ymin = MAX( pdyna->pos.y - radius, mesh_bound.ymin );
            ftmp.ymax = MIN( pdyna->pos.y + radius, mesh_bound.ymax );

            // check to see if it intersects the "frustum"
            if ( ftmp.xmin >= ftmp.xmax || ftmp.ymin >= ftmp.ymax ) continue;

            reg[reg_count].bound     = ftmp;
            reg[reg_count].reference = cnt;
            reg_count++;

            // determine the maxumum bounding box that encloses all valid lights
            light_bound.xmin = MIN( light_bound.xmin, ftmp.xmin );
            light_bound.xmax = MAX( light_bound.xmax, ftmp.xmax );
            light_bound.ymin = MIN( light_bound.ymin, ftmp.ymin );
            light_bound.ymax = MAX( light_bound.ymax, ftmp.ymax );
        }

        // are there any dynalights visible?
        if ( reg_count > 0 && light_bound.xmax >= light_bound.xmin && light_bound.ymax >= light_bound.ymin )
        {
            needs_dynalight = btrue;
        }
    }
    else
    {
        float dyna_weight = 0.0f;
        float dyna_weight_sum = 0.0f;

        fvec3_t       diff;
        dynalight_t * pdyna;

        // evaluate all the lights at the camera position
        for ( cnt = 0; cnt < pdylist->count; cnt++ )
        {
            pdyna = pdylist->lst + cnt;

            // evaluate the intensity at the camera
            diff.x = pdyna->pos.x - pcam->center.x;
            diff.y = pdyna->pos.y - pcam->center.y;
            diff.z = pdyna->pos.z - pcam->center.z - 90.0f;   // evaluated at the "head height" of a character

            dyna_weight = ABS( dyna_lighting_intensity( pdyna, diff.v ) );

            fake_dynalight.distance += dyna_weight * pdyna->distance;
            fake_dynalight.falloff  += dyna_weight * pdyna->falloff;
            fake_dynalight.level    += dyna_weight * pdyna->level;
            fake_dynalight.pos.x    += dyna_weight * ( pdyna->pos.x - pcam->center.x );
            fake_dynalight.pos.y    += dyna_weight * ( pdyna->pos.y - pcam->center.y );
            fake_dynalight.pos.z    += dyna_weight * ( pdyna->pos.z - pcam->center.z );

            dyna_weight_sum         += dyna_weight;
        }

        // use a singel dynalight to represent the sum of all dynalights
        if ( dyna_weight_sum > 0.0f )
        {
            float radius;
            ego_frect_t ftmp;

            fake_dynalight.distance /= dyna_weight_sum;
            fake_dynalight.falloff  /= dyna_weight_sum;
            fake_dynalight.level    /= dyna_weight_sum;
            fake_dynalight.pos.x    = fake_dynalight.pos.x / dyna_weight_sum + pcam->center.x;
            fake_dynalight.pos.y    = fake_dynalight.pos.y / dyna_weight_sum + pcam->center.y;
            fake_dynalight.pos.z    = fake_dynalight.pos.z / dyna_weight_sum + pcam->center.z;

            radius = SQRT( fake_dynalight.falloff * 765.0f * 0.5f );

            // find the intersection with the frustum boundary
            ftmp.xmin = MAX( fake_dynalight.pos.x - radius, mesh_bound.xmin );
            ftmp.xmax = MIN( fake_dynalight.pos.x + radius, mesh_bound.xmax );
            ftmp.ymin = MAX( fake_dynalight.pos.y - radius, mesh_bound.ymin );
            ftmp.ymax = MIN( fake_dynalight.pos.y + radius, mesh_bound.ymax );

            // make a fake light bound
            light_bound = ftmp;

            // register the fake dynalight
            reg[reg_count].bound     = ftmp;
            reg[reg_count].reference = -1;
            reg_count++;

            // let the downstream calc know we are coming
            needs_dynalight = btrue;
        }
    }

    // sum up the lighting from global sources
    sum_global_lighting( global_lighting );

    // make the grids update their lighting every 4 frames
    local_keep = POW( dynalight_keep, 4 );

    // Add to base light level in normal mode
    for ( entry = 0; entry < prlist->all_count; entry++ )
    {
        bool_t resist_lighting_calculation = btrue;

        int                dynalight_count = 0;
        ego_grid_info_t  * pgrid = NULL;
        lighting_cache_t * pcache_old = NULL;
        lighting_cache_t   cache_new;

        // grab each grid box in the "frustum"
        fan = prlist->all[entry];

        // a valid tile?
        pgrid = mesh_get_pgrid( pmesh, fan );
        if ( NULL == pgrid ) continue;

        // do not update this more than once a frame
        if ( pgrid->cache_frame >= 0 && ( Uint32 )pgrid->cache_frame >= game_frame_all ) continue;

        ix = fan % pinfo->tiles_x;
        iy = fan / pinfo->tiles_x;

        // Resist the lighting calculation?
        // This is a speedup for lighting calculations so that
        // not every light-tile calculation is done every single frame
        resist_lighting_calculation = ( 0 != ((( ix + iy ) ^ game_frame_all ) & 0x03 ) );

        if ( resist_lighting_calculation ) continue;

        // this is not a "bad" grid box, so grab the lighting info
        pcache_old = &( pgrid->cache );

        lighting_cache_init( &cache_new );

        // copy the global lighting
        for ( tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++ )
        {
            cache_new.low.lighting[tnc] = global_lighting[tnc];
            cache_new.hgh.lighting[tnc] = global_lighting[tnc];
        };

        // do we need any dynamic lighting at all?
        if ( needs_dynalight )
        {
            // calculate the local lighting

            ego_frect_t fgrid_rect;

            x0 = ix * GRID_FSIZE;
            y0 = iy * GRID_FSIZE;

            // check this grid vertex relative to the measured light_bound
            fgrid_rect.xmin = x0 - GRID_FSIZE * 0.5f;
            fgrid_rect.xmax = x0 + GRID_FSIZE * 0.5f;
            fgrid_rect.ymin = y0 - GRID_FSIZE * 0.5f;
            fgrid_rect.ymax = y0 + GRID_FSIZE * 0.5f;

            // check the bounding box of this grid vs. the bounding box of the lighting
            if ( fgrid_rect.xmin <= light_bound.xmax && fgrid_rect.xmax >= light_bound.xmin )
            {
                if ( fgrid_rect.ymin <= light_bound.ymax && fgrid_rect.ymax >= light_bound.ymin )
                {
                    // this grid has dynamic lighting. add it.
                    for ( cnt = 0; cnt < reg_count; cnt++ )
                    {
                        fvec3_t       nrm;
                        dynalight_t * pdyna;

                        // does this dynamic light intersects this grid?
                        if ( fgrid_rect.xmin > reg[cnt].bound.xmax || fgrid_rect.xmax < reg[cnt].bound.xmin ) continue;
                        if ( fgrid_rect.ymin > reg[cnt].bound.ymax || fgrid_rect.ymax < reg[cnt].bound.ymin ) continue;

                        dynalight_count++;

                        // this should be a valid intersection, so proceed
                        tnc = reg[cnt].reference;
                        if ( tnc < 0 )
                        {
                            pdyna = &fake_dynalight;
                        }
                        else
                        {
                            pdyna = pdylist->lst + tnc;
                        }

                        nrm.x = pdyna->pos.x - x0;
                        nrm.y = pdyna->pos.y - y0;
                        nrm.z = pdyna->pos.z - ptmem->bbox.mins[ZZ];
                        sum_dyna_lighting( pdyna, cache_new.low.lighting, nrm.v );

                        nrm.z = pdyna->pos.z - ptmem->bbox.maxs[ZZ];
                        sum_dyna_lighting( pdyna, cache_new.hgh.lighting, nrm.v );
                    }
                }
            }
        }
        else if ( GL_FLAT == gfx.shading )
        {
            // evaluate the intensity at the camera
        }

        // blend in the global lighting every single time
        // average this in with the existing lighting
        lighting_cache_blend( pcache_old, &cache_new, local_keep );

        // find the max intensity
        lighting_cache_max_light( pcache_old );

        pgrid->cache_frame = game_frame_all;
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_capture_mesh_tile( ego_tile_info_t * ptile )
{
    if ( NULL == ptile )
    {
        return gfx_fail;
    }

    // Flag the tile as in the renderlist
    ptile->inrenderlist = btrue;

    // if the tile was not in the renderlist last frame, then we need to force a lighting update of this tile
    if ( ptile->inrenderlist_frame < 0 )
    {
        ptile->request_lcache_update = btrue;
        ptile->lcache_frame        = -1;
    }
    else
    {
        Uint32 last_frame = ( game_frame_all > 0 ) ? game_frame_all - 1 : 0;

        if (( Uint32 ) ptile->inrenderlist_frame < last_frame )
        {
            ptile->request_lcache_update = btrue;
        }
    }

    // make sure to cache the frame number of this update
    ptile->inrenderlist_frame = game_frame_all;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_make_renderlist( renderlist_t * prlist, const camera_t * pcam )
{
    gfx_rv      retval;
    bool_t      local_allocation;

    // Make sure there is a renderlist
    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
        return gfx_error;
    }

    // make sure that we have a valid camera
    if ( NULL == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    // because the main loop of the program will always flip the
    // page before rendering the 1st frame of the actual game,
    // game_frame_all will always start at 1
    if ( 1 != ( game_frame_all & 3 ) )
    {
        return gfx_success;
    }

    // reset the renderlist
    if ( gfx_error == renderlist_reset( prlist ) )
    {
        return gfx_error;
    }

    // has the colst been allocated?
    local_allocation = bfalse;
    if ( 0 == BSP_leaf_pary_get_size( &_renderlist_colst ) )
    {
        // allocate a BSP_leaf_pary to return the detected nodes
        local_allocation = btrue;
        if ( NULL == BSP_leaf_pary_ctor( &_renderlist_colst, MAXMESHRENDER ) )
        {
            gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "Could not allocate collision list" );
            return gfx_error;
        }
    }

    // assume the best
    retval = gfx_success;

    // get the tiles in the center of the view
    _renderlist_colst.top = 0;
    mpd_BSP_collide_frustum( &( mpd_BSP_root ), &( pcam->frustum_big ), NULL, &_renderlist_colst );

    // transfer valid _renderlist_colst entries to the dolist
    if ( gfx_error == renderlist_add_colst( prlist, &_renderlist_colst ) )
    {
        retval = gfx_error;
        goto gfx_make_renderlist_exit;
    }

gfx_make_renderlist_exit:

    // if there was a local allocation, make sure to deallocate
    if ( local_allocation )
    {
        BSP_leaf_pary_dtor( &_renderlist_colst );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_make_dolist( dolist_t * pdlist, const camera_t * pcam )
{
    /// @author ZZ
    /// @details This function finds the characters that need to be drawn and puts them in the list

    gfx_rv retval;
    bool_t local_allocation;

    // assume the best
    retval = gfx_success;

    if ( NULL == pdlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

    if ( pdlist->count >= DOLIST_SIZE )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist size" );
        return gfx_error;
    }

    // Remove everyone from the dolist
    dolist_reset( pdlist, pdlist->index );

    // has the colst been allocated?
    local_allocation = bfalse;
    if ( 0 == BSP_leaf_pary_get_size( &_dolist_colst ) )
    {
        // allocate a BSP_leaf_pary to return the detected nodes
        local_allocation = btrue;
        if ( NULL == BSP_leaf_pary_ctor( &_dolist_colst, DOLIST_SIZE ) )
        {
            gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "Could not allocate collision list" );
            return gfx_error;
        }
    }

    // collide the characters with the frustum
    _dolist_colst.top = 0;
    obj_BSP_collide_frustum( &chr_BSP_root, &( pcam->frustum ), chr_BSP_is_visible, &_dolist_colst );

    // transfer valid _dolist_colst entries to the dolist
    if ( gfx_error == dolist_add_colst( pdlist, &_dolist_colst ) )
    {
        retval = gfx_error;
        goto gfx_make_dolist_exit;
    }

    // collide the particles with the frustum
    _dolist_colst.top = 0;
    obj_BSP_collide_frustum( &prt_BSP_root, &( pcam->frustum ), prt_BSP_is_visible, &_dolist_colst );

    // transfer valid _dolist_colst entries to the dolist
    if ( gfx_error == dolist_add_colst( pdlist, &_dolist_colst ) )
    {
        retval = gfx_error;
        goto gfx_make_dolist_exit;
    }

gfx_make_dolist_exit:

    // if there was a local allocation, make sure to deallocate
    if ( local_allocation )
    {
        BSP_leaf_pary_dtor( &_dolist_colst );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// UTILITY FUNCTIONS
//--------------------------------------------------------------------------------------------
bool_t dump_screenshot()
{
    /// @author BB
    /// @details dumps the current screen (GL context) to a new bitmap file
    /// right now it dumps it to whatever the current directory is

    // returns btrue if successful, bfalse otherwise

    int i;
    bool_t savefound = bfalse;
    bool_t saved     = bfalse;
    STRING szFilename, szResolvedFilename;

    // find a valid file name
    savefound = bfalse;
    i = 0;
    while ( !savefound && ( i < 100 ) )
    {
        snprintf( szFilename, SDL_arraysize( szFilename ), "ego%02d.bmp", i );

        // lame way of checking if the file already exists...
        savefound = !vfs_exists( szFilename );
        if ( !savefound )
        {
            i++;
        }
    }

    if ( !savefound ) return bfalse;

    // convert the file path to the correct write path
    strncpy( szResolvedFilename, vfs_resolveWriteFilename( szFilename ), SDL_arraysize( szFilename ) );

    // if we are not using OpenGL, use SDL to dump the screen
    if ( HAS_NO_BITS( sdl_scr.pscreen->flags, SDL_OPENGL ) )
    {
        SDL_SaveBMP( sdl_scr.pscreen, szResolvedFilename );
        return bfalse;
    }

    // we ARE using OpenGL
    GL_DEBUG( glPushClientAttrib )( GL_CLIENT_PIXEL_STORE_BIT ) ;
    {
        SDL_Surface *temp;

        // create a SDL surface
        temp = SDL_CreateRGBSurface( SDL_SWSURFACE, sdl_scr.x, sdl_scr.y, 24, sdl_r_mask, sdl_g_mask, sdl_b_mask, 0 );

        if ( NULL == temp )
        {
            //Something went wrong
            SDL_FreeSurface( temp );
            return bfalse;
        }

        //Now lock the surface so that we can read it
        if ( -1 != SDL_LockSurface( temp ) )
        {
            SDL_Rect rect;

            memcpy( &rect, &( sdl_scr.pscreen->clip_rect ), sizeof( SDL_Rect ) );
            if ( 0 == rect.w && 0 == rect.h )
            {
                rect.w = sdl_scr.x;
                rect.h = sdl_scr.y;
            }
            if ( rect.w > 0 && rect.h > 0 )
            {
                int y;
                Uint8 * pixels;

                GL_DEBUG( glGetError )();

                //// use the allocated screen to tell OpenGL about the row length (including the lapse) in pixels
                //// stolen from SDL ;)
                // GL_DEBUG(glPixelStorei)(GL_UNPACK_ROW_LENGTH, temp->pitch / temp->format->BytesPerPixel );
                // EGOBOO_ASSERT( GL_NO_ERROR == GL_DEBUG(glGetError)() );

                //// since we have specified the row actual length and will give a pointer to the actual pixel buffer,
                //// it is not necesssaty to mess with the alignment
                // GL_DEBUG(glPixelStorei)(GL_UNPACK_ALIGNMENT, 1 );
                // EGOBOO_ASSERT( GL_NO_ERROR == GL_DEBUG(glGetError)() );

                // ARGH! Must copy the pixels row-by-row, since the OpenGL video memory is flipped vertically
                // relative to the SDL Screen memory

                // this is supposed to be a DirectX thing, so it needs to be tested out on glx
                // there should probably be [SCREENSHOT_INVERT] and [SCREENSHOT_VALID] keys in setup.txt
                pixels = ( Uint8 * )temp->pixels;
                for ( y = rect.y; y < rect.y + rect.h; y++ )
                {
                    GL_DEBUG( glReadPixels )( rect.x, ( rect.h - y ) - 1, rect.w, 1, GL_RGB, GL_UNSIGNED_BYTE, pixels );
                    pixels += temp->pitch;
                }
                EGOBOO_ASSERT( GL_NO_ERROR == GL_DEBUG( glGetError )() );
            }

            SDL_UnlockSurface( temp );

            // Save the file as a .bmp
            saved = ( -1 != SDL_SaveBMP( temp, szResolvedFilename ) );
        }

        // free the SDL surface
        SDL_FreeSurface( temp );
        if ( saved )
        {
            // tell the user what we did
            debug_printf( "Saved to %s", szFilename );
        }
    }
    GL_DEBUG( glPopClientAttrib )();

    return savefound && saved;
}

//--------------------------------------------------------------------------------------------
void clear_messages()
{
    /// @author ZZ
    /// @details This function empties the message buffer
    int cnt;

    cnt = 0;

    while ( cnt < MAX_MESSAGE )
    {
        DisplayMsg.ary[cnt].time = 0;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
float calc_light_rotation( int rotation, int normal )
{
    /// @author ZZ
    /// @details This function helps make_lighttable
    fvec3_t   nrm, nrm2;
    float sinrot, cosrot;

    nrm.x = kMd2Normals[normal][0];
    nrm.y = kMd2Normals[normal][1];
    nrm.z = kMd2Normals[normal][2];

    sinrot = sinlut[rotation];
    cosrot = coslut[rotation];

    nrm2.x = cosrot * nrm.x + sinrot * nrm.y;
    nrm2.y = cosrot * nrm.y - sinrot * nrm.x;
    nrm2.z = nrm.z;

    return ( nrm2.x < 0 ) ? 0 : ( nrm2.x * nrm2.x );
}

//--------------------------------------------------------------------------------------------
float calc_light_global( int rotation, int normal, float lx, float ly, float lz )
{
    /// @author ZZ
    /// @details This function helps make_lighttable
    float fTmp;
    fvec3_t   nrm, nrm2;
    float sinrot, cosrot;

    nrm.x = kMd2Normals[normal][0];
    nrm.y = kMd2Normals[normal][1];
    nrm.z = kMd2Normals[normal][2];

    sinrot = sinlut[rotation];
    cosrot = coslut[rotation];

    nrm2.x = cosrot * nrm.x + sinrot * nrm.y;
    nrm2.y = cosrot * nrm.y - sinrot * nrm.x;
    nrm2.z = nrm.z;

    fTmp = nrm2.x * lx + nrm2.y * ly + nrm2.z * lz;
    if ( fTmp < 0 ) fTmp = 0;

    return fTmp * fTmp;
}

//--------------------------------------------------------------------------------------------
void gfx_reset_timers()
{
    egolib_throttle_reset( &gfx_throttle );
    gfx_clear_loops = 0;
}

//--------------------------------------------------------------------------------------------
bool_t gfx_frustum_intersects_oct( const egolib_frustum_t * pf, const oct_bb_t * poct )
{
    bool_t retval = bfalse;
    aabb_t aabb;

    if ( NULL == pf || NULL == poct ) return bfalse;

    retval = bfalse;
    if ( aabb_from_oct_bb( &aabb, poct ) )
    {
        retval = ( frustum_intersects_aabb( pf->data, aabb.mins, aabb.maxs ) > geometry_outside );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_update_flashing( dolist_t * pdolist )
{
    gfx_rv retval;
    Uint32 i;

    if ( NULL == pdolist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

    if ( pdolist->count >= DOLIST_SIZE )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist size" );
        return gfx_error;
    }

    retval = gfx_success;
    for ( i = 0; i < pdolist->count; i++ )
    {
        float tmp_seekurse_level;

        chr_t          * pchr;
        chr_instance_t * pinst;

        CHR_REF ichr = pdolist->lst[i].ichr;

        pchr = ChrList_get_ptr( ichr );
        if ( !DEFINED_PCHR( pchr ) ) continue;

        pinst = &( pchr->inst );

        // Do flashing
        if ( DONTFLASH != pchr->flashand )
        {
            if ( HAS_NO_BITS( game_frame_all, pchr->flashand ) )
            {
                if ( gfx_error == chr_instance_flash( pinst, 255 ) )
                {
                    retval = gfx_error;
                }
            }
        }

        // Do blacking
        // having one holy player in your party will cause the effect, BUT
        // having some non-holy players will dilute it
        tmp_seekurse_level = MIN( local_stats.seekurse_level, 1.0f );
        if (( local_stats.seekurse_level > 0.0f ) && pchr->iskursed && 1.0f != tmp_seekurse_level )
        {
            if ( HAS_NO_BITS( game_frame_all, SEEKURSEAND ) )
            {
                if ( gfx_error == chr_instance_flash( pinst, 255.0f *( 1.0f - tmp_seekurse_level ) ) )
                {
                    retval = gfx_error;
                }
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_update_all_chr_instance()
{
    CHR_REF cnt;
    gfx_rv retval;
    chr_t * pchr;
    gfx_rv tmp_rv;

    // assume the best
    retval = gfx_success;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !ALLOCATED_CHR( cnt ) ) continue;
        pchr = ChrList_get_ptr( cnt );

        if ( !mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) ) continue;

        tmp_rv = update_one_chr_instance( pchr );

        // deal with return values
        if ( gfx_error == tmp_rv )
        {
            retval = gfx_error;
        }
        else if ( gfx_success == tmp_rv )
        {
            // the instance has changed, refresh the collision bound
            chr_update_collision_size( pchr, btrue );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// chr_instance_t FUNCTIONS
//--------------------------------------------------------------------------------------------
gfx_rv update_one_chr_instance( chr_t * pchr )
{
    chr_instance_t * pinst;
    gfx_rv retval;

    if ( !ACTIVE_PCHR( pchr ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, GET_INDEX_PCHR( pchr ), "invalid character" );
        return gfx_error;
    }
    pinst = &( pchr->inst );

    // only update once per frame
    if ( pinst->update_frame >= 0 && ( Uint32 )pinst->update_frame >= game_frame_all )
    {
        return gfx_success;
    }

    // make sure that the vertices are interpolated
    retval = chr_instance_update_vertices( pinst, -1, -1, btrue );
    if ( gfx_error == retval )
    {
        return gfx_error;
    }

    // do the basic lighting
    chr_instance_update_lighting_base( pinst, pchr, bfalse );

    // set the update_frame to the current frame
    pinst->update_frame = game_frame_all;

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_flash( chr_instance_t * pinst, Uint8 value )
{
    /// @author ZZ
    /// @details This function sets a character's lighting

    size_t     cnt;
    float      flash_val = value * INV_FF;
    GLvertex * pv;

    if ( NULL == pinst ) return gfx_error;

    // flash the ambient color
    pinst->color_amb = flash_val;

    // flash the directional lighting
    pinst->color_amb = flash_val;
    for ( cnt = 0; cnt < pinst->vrt_count; cnt++ )
    {
        pv = pinst->vrt_lst + cnt;

        pv->color_dir = flash_val;
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
// OBSOLETE
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// projection_bitmap_t
//--------------------------------------------------------------------------------------------

//struct s_projection_bitmap;
//typedef struct s_projection_bitmap projection_bitmap_t;

//#define PROJECTION_BITMAP_ROWS 128

//struct s_projection_bitmap
//{
//    int y_stt;
//    int row_cnt;
//
//    int row_stt[PROJECTION_BITMAP_ROWS];
//    int row_end[PROJECTION_BITMAP_ROWS];
//};

//static projection_bitmap_t * projection_bitmap_ctor( projection_bitmap_t * ptr );

//projection_bitmap_t * projection_bitmap_ctor( projection_bitmap_t * ptr )
//{
//    if ( NULL == ptr ) return ptr;
//
//    ptr->y_stt   = 0;
//    ptr->row_cnt = 0;
//
//    return ptr;
//}

//--------------------------------------------------------------------------------------------
// cam_corner_info_t
//--------------------------------------------------------------------------------------------

//struct s_cam_corner_info;
//typedef struct s_cam_corner_info cam_corner_info_t;

//struct s_cam_corner_info
//{
//    float                   lowx;
//    float                   highx;
//    float                   lowy;
//    float                   highy;
//
//    float                   x[4];
//    float                   y[4];
//    int                     listlowtohighy[4];
//};

//void gfx_calc_rotmesh()
//{
//    static const float assumed_aspect_ratio = 4.0f / 3.0f;
//
//    float aspect_ratio = ( float )sdl_scr.x / ( float )sdl_scr.y;
//
//    // Matrix init stuff (from remove.c)
//    rotmesh_topside    = aspect_ratio / assumed_aspect_ratio * ROTMESH_TOPSIDE ;
//    rotmesh_bottomside = aspect_ratio / assumed_aspect_ratio * ROTMESH_BOTTOMSIDE;
//    rotmesh_up         = aspect_ratio / assumed_aspect_ratio * ROTMESH_UP;
//    rotmesh_down       = aspect_ratio / assumed_aspect_ratio * ROTMESH_DOWN;
//}

//--------------------------------------------------------------------------------------------
//void get_clip_planes( const camera_t * pcam, GLfloat planes[] )
//{
//    // clip plane normals point IN
//
//    GLfloat clip[16];
//    GLfloat _proj[16];
//    GLfloat _modl[16];
//    GLfloat t;
//
//    const GLfloat * proj = NULL;
//    const GLfloat * modl = NULL;
//
//    // get the matrices...
//    if ( NULL != pcam )
//    {
//        // ...from the camera
//        proj = pcam->mProjection_big.v;
//        modl = pcam->mView.v;
//    }
//    else
//    {
//        // ... from the current OpenGL state
//
//        // The Current PROJECTION Matrix
//        proj = _proj;
//        glGetFloatv( GL_PROJECTION_MATRIX, _proj );
//
//        // The Current MODELVIEW Matrix
//        modl = _modl;
//        glGetFloatv( GL_MODELVIEW_MATRIX, _modl );
//    }
//
//    // Multiply Projection By Modelview
//    clip[ MAT_IDX( 0, 0 )] = modl[ MAT_IDX( 0, 0 )] * proj[ MAT_IDX( 0, 0 )] + modl[ MAT_IDX( 0, 1 )] * proj[ MAT_IDX( 1, 0 )] + modl[ MAT_IDX( 0, 2 )] * proj[ MAT_IDX( 2, 0 )] + modl[ MAT_IDX( 0, 3 )] * proj[ MAT_IDX( 3, 0 )];
//    clip[ MAT_IDX( 0, 1 )] = modl[ MAT_IDX( 0, 0 )] * proj[ MAT_IDX( 0, 1 )] + modl[ MAT_IDX( 0, 1 )] * proj[ MAT_IDX( 1, 1 )] + modl[ MAT_IDX( 0, 2 )] * proj[ MAT_IDX( 2, 1 )] + modl[ MAT_IDX( 0, 3 )] * proj[ MAT_IDX( 3, 1 )];
//    clip[ MAT_IDX( 0, 2 )] = modl[ MAT_IDX( 0, 0 )] * proj[ MAT_IDX( 0, 2 )] + modl[ MAT_IDX( 0, 1 )] * proj[ MAT_IDX( 1, 2 )] + modl[ MAT_IDX( 0, 2 )] * proj[ MAT_IDX( 2, 2 )] + modl[ MAT_IDX( 0, 3 )] * proj[ MAT_IDX( 3, 2 )];
//    clip[ MAT_IDX( 0, 3 )] = modl[ MAT_IDX( 0, 0 )] * proj[ MAT_IDX( 0, 3 )] + modl[ MAT_IDX( 0, 1 )] * proj[ MAT_IDX( 1, 3 )] + modl[ MAT_IDX( 0, 2 )] * proj[ MAT_IDX( 2, 3 )] + modl[ MAT_IDX( 0, 3 )] * proj[ MAT_IDX( 3, 3 )];
//
//    clip[ MAT_IDX( 1, 0 )] = modl[ MAT_IDX( 1, 0 )] * proj[ MAT_IDX( 0, 0 )] + modl[ MAT_IDX( 1, 1 )] * proj[ MAT_IDX( 1, 0 )] + modl[ MAT_IDX( 1, 2 )] * proj[ MAT_IDX( 2, 0 )] + modl[ MAT_IDX( 1, 3 )] * proj[ MAT_IDX( 3, 0 )];
//    clip[ MAT_IDX( 1, 1 )] = modl[ MAT_IDX( 1, 0 )] * proj[ MAT_IDX( 0, 1 )] + modl[ MAT_IDX( 1, 1 )] * proj[ MAT_IDX( 1, 1 )] + modl[ MAT_IDX( 1, 2 )] * proj[ MAT_IDX( 2, 1 )] + modl[ MAT_IDX( 1, 3 )] * proj[ MAT_IDX( 3, 1 )];
//    clip[ MAT_IDX( 1, 2 )] = modl[ MAT_IDX( 1, 0 )] * proj[ MAT_IDX( 0, 2 )] + modl[ MAT_IDX( 1, 1 )] * proj[ MAT_IDX( 1, 2 )] + modl[ MAT_IDX( 1, 2 )] * proj[ MAT_IDX( 2, 2 )] + modl[ MAT_IDX( 1, 3 )] * proj[ MAT_IDX( 3, 2 )];
//    clip[ MAT_IDX( 1, 3 )] = modl[ MAT_IDX( 1, 0 )] * proj[ MAT_IDX( 0, 3 )] + modl[ MAT_IDX( 1, 1 )] * proj[ MAT_IDX( 1, 3 )] + modl[ MAT_IDX( 1, 2 )] * proj[ MAT_IDX( 2, 3 )] + modl[ MAT_IDX( 1, 3 )] * proj[ MAT_IDX( 3, 3 )];
//
//    clip[ MAT_IDX( 2, 0 )] = modl[ MAT_IDX( 2, 0 )] * proj[ MAT_IDX( 0, 0 )] + modl[ MAT_IDX( 2, 1 )] * proj[ MAT_IDX( 1, 0 )] + modl[ MAT_IDX( 2, 2 )] * proj[ MAT_IDX( 2, 0 )] + modl[ MAT_IDX( 2, 3 )] * proj[ MAT_IDX( 3, 0 )];
//    clip[ MAT_IDX( 2, 1 )] = modl[ MAT_IDX( 2, 0 )] * proj[ MAT_IDX( 0, 1 )] + modl[ MAT_IDX( 2, 1 )] * proj[ MAT_IDX( 1, 1 )] + modl[ MAT_IDX( 2, 2 )] * proj[ MAT_IDX( 2, 1 )] + modl[ MAT_IDX( 2, 3 )] * proj[ MAT_IDX( 3, 1 )];
//    clip[ MAT_IDX( 2, 2 )] = modl[ MAT_IDX( 2, 0 )] * proj[ MAT_IDX( 0, 2 )] + modl[ MAT_IDX( 2, 1 )] * proj[ MAT_IDX( 1, 2 )] + modl[ MAT_IDX( 2, 2 )] * proj[ MAT_IDX( 2, 2 )] + modl[ MAT_IDX( 2, 3 )] * proj[ MAT_IDX( 3, 2 )];
//    clip[ MAT_IDX( 2, 3 )] = modl[ MAT_IDX( 2, 0 )] * proj[ MAT_IDX( 0, 3 )] + modl[ MAT_IDX( 2, 1 )] * proj[ MAT_IDX( 1, 3 )] + modl[ MAT_IDX( 2, 2 )] * proj[ MAT_IDX( 2, 3 )] + modl[ MAT_IDX( 2, 3 )] * proj[ MAT_IDX( 3, 3 )];
//
//    clip[ MAT_IDX( 3, 0 )] = modl[ MAT_IDX( 3, 0 )] * proj[ MAT_IDX( 0, 0 )] + modl[ MAT_IDX( 3, 1 )] * proj[ MAT_IDX( 1, 0 )] + modl[ MAT_IDX( 3, 2 )] * proj[ MAT_IDX( 2, 0 )] + modl[ MAT_IDX( 3, 3 )] * proj[ MAT_IDX( 3, 0 )];
//    clip[ MAT_IDX( 3, 1 )] = modl[ MAT_IDX( 3, 0 )] * proj[ MAT_IDX( 0, 1 )] + modl[ MAT_IDX( 3, 1 )] * proj[ MAT_IDX( 1, 1 )] + modl[ MAT_IDX( 3, 2 )] * proj[ MAT_IDX( 2, 1 )] + modl[ MAT_IDX( 3, 3 )] * proj[ MAT_IDX( 3, 1 )];
//    clip[ MAT_IDX( 3, 2 )] = modl[ MAT_IDX( 3, 0 )] * proj[ MAT_IDX( 0, 2 )] + modl[ MAT_IDX( 3, 1 )] * proj[ MAT_IDX( 1, 2 )] + modl[ MAT_IDX( 3, 2 )] * proj[ MAT_IDX( 2, 2 )] + modl[ MAT_IDX( 3, 3 )] * proj[ MAT_IDX( 3, 2 )];
//    clip[ MAT_IDX( 3, 3 )] = modl[ MAT_IDX( 3, 0 )] * proj[ MAT_IDX( 0, 3 )] + modl[ MAT_IDX( 3, 1 )] * proj[ MAT_IDX( 1, 3 )] + modl[ MAT_IDX( 3, 2 )] * proj[ MAT_IDX( 2, 3 )] + modl[ MAT_IDX( 3, 3 )] * proj[ MAT_IDX( 3, 3 )];
//
//    // Extract The Numbers For The RIGHT Plane
//    planes[MAT_IDX( 0, 0 )] = clip[ MAT_IDX( 0, 3 )] - clip[ MAT_IDX( 0, 0 )];
//    planes[MAT_IDX( 0, 1 )] = clip[ MAT_IDX( 1, 3 )] - clip[ MAT_IDX( 1, 0 )];
//    planes[MAT_IDX( 0, 2 )] = clip[ MAT_IDX( 2, 3 )] - clip[ MAT_IDX( 2, 0 )];
//    planes[MAT_IDX( 0, 3 )] = clip[ MAT_IDX( 3, 3 )] - clip[ MAT_IDX( 3, 0 )];
//
//    // Normalize The Result
//    t = SQRT( planes[MAT_IDX( 0, 0 )] * planes[MAT_IDX( 0, 0 )] + planes[MAT_IDX( 0, 1 )] * planes[MAT_IDX( 0, 1 )] + planes[MAT_IDX( 0, 2 )] * planes[MAT_IDX( 0, 2 )] );
//    planes[MAT_IDX( 0, 0 )] /= t;
//    planes[MAT_IDX( 0, 1 )] /= t;
//    planes[MAT_IDX( 0, 2 )] /= t;
//    planes[MAT_IDX( 0, 3 )] /= t;
//
//    // Extract The Numbers For The LEFT Plane
//    planes[MAT_IDX( 1, 0 )] = clip[ MAT_IDX( 0, 3 )] + clip[ MAT_IDX( 0, 0 )];
//    planes[MAT_IDX( 1, 1 )] = clip[ MAT_IDX( 1, 3 )] + clip[ MAT_IDX( 1, 0 )];
//    planes[MAT_IDX( 1, 2 )] = clip[ MAT_IDX( 2, 3 )] + clip[ MAT_IDX( 2, 0 )];
//    planes[MAT_IDX( 1, 3 )] = clip[ MAT_IDX( 3, 3 )] + clip[ MAT_IDX( 3, 0 )];
//
//    // Normalize The Result
//    t = SQRT( planes[MAT_IDX( 1, 0 )] * planes[MAT_IDX( 1, 0 )] + planes[MAT_IDX( 1, 1 )] * planes[MAT_IDX( 1, 1 )] + planes[MAT_IDX( 1, 2 )] * planes[MAT_IDX( 1, 2 )] );
//    planes[MAT_IDX( 1, 0 )] /= t;
//    planes[MAT_IDX( 1, 1 )] /= t;
//    planes[MAT_IDX( 1, 2 )] /= t;
//    planes[MAT_IDX( 1, 3 )] /= t;
//
//    // Extract The BOTTOM Plane
//    planes[MAT_IDX( 2, 0 )] = clip[ MAT_IDX( 0, 3 )] + clip[ MAT_IDX( 0, 1 )];
//    planes[MAT_IDX( 2, 1 )] = clip[ MAT_IDX( 1, 3 )] + clip[ MAT_IDX( 1, 1 )];
//    planes[MAT_IDX( 2, 2 )] = clip[ MAT_IDX( 2, 3 )] + clip[ MAT_IDX( 2, 1 )];
//    planes[MAT_IDX( 2, 3 )] = clip[ MAT_IDX( 3, 3 )] + clip[ MAT_IDX( 3, 1 )];
//
//    // Normalize The Result
//    t = SQRT( planes[MAT_IDX( 2, 0 )] * planes[MAT_IDX( 2, 0 )] + planes[MAT_IDX( 2, 1 )] * planes[MAT_IDX( 2, 1 )] + planes[MAT_IDX( 2, 2 )] * planes[MAT_IDX( 2, 2 )] );
//    planes[MAT_IDX( 2, 0 )] /= t;
//    planes[MAT_IDX( 2, 1 )] /= t;
//    planes[MAT_IDX( 2, 2 )] /= t;
//    planes[MAT_IDX( 2, 3 )] /= t;
//
//    // Extract The TOP Plane
//    planes[MAT_IDX( 3, 0 )] = clip[ MAT_IDX( 0, 3 )] - clip[ MAT_IDX( 0, 1 )];
//    planes[MAT_IDX( 3, 1 )] = clip[ MAT_IDX( 1, 3 )] - clip[ MAT_IDX( 1, 1 )];
//    planes[MAT_IDX( 3, 2 )] = clip[ MAT_IDX( 2, 3 )] - clip[ MAT_IDX( 2, 1 )];
//    planes[MAT_IDX( 3, 3 )] = clip[ MAT_IDX( 3, 3 )] - clip[ MAT_IDX( 3, 1 )];
//
//    // Normalize The Result
//    t = SQRT( planes[MAT_IDX( 3, 0 )] * planes[MAT_IDX( 3, 0 )] + planes[MAT_IDX( 3, 1 )] * planes[MAT_IDX( 3, 1 )] + planes[MAT_IDX( 3, 2 )] * planes[MAT_IDX( 3, 2 )] );
//    planes[MAT_IDX( 3, 0 )] /= t;
//    planes[MAT_IDX( 3, 1 )] /= t;
//    planes[MAT_IDX( 3, 2 )] /= t;
//    planes[MAT_IDX( 3, 3 )] /= t;
//
//    // Extract The FAR Plane
//    planes[MAT_IDX( 4, 0 )] = clip[ MAT_IDX( 0, 3 )] - clip[ MAT_IDX( 0, 2 )];
//    planes[MAT_IDX( 4, 1 )] = clip[ MAT_IDX( 1, 3 )] - clip[ MAT_IDX( 1, 2 )];
//    planes[MAT_IDX( 4, 2 )] = clip[ MAT_IDX( 2, 3 )] - clip[ MAT_IDX( 2, 2 )];
//    planes[MAT_IDX( 4, 3 )] = clip[ MAT_IDX( 3, 3 )] - clip[ MAT_IDX( 3, 2 )];
//
//    // Normalize The Result
//    t = SQRT( planes[MAT_IDX( 4, 0 )] * planes[MAT_IDX( 4, 0 )] + planes[MAT_IDX( 4, 1 )] * planes[MAT_IDX( 4, 1 )] + planes[MAT_IDX( 4, 2 )] * planes[MAT_IDX( 4, 2 )] );
//    planes[MAT_IDX( 4, 0 )] /= t;
//    planes[MAT_IDX( 4, 1 )] /= t;
//    planes[MAT_IDX( 4, 2 )] /= t;
//    planes[MAT_IDX( 4, 3 )] /= t;
//
//    // Extract The NEAR Plane
//    planes[MAT_IDX( 5, 0 )] = clip[ MAT_IDX( 0, 3 )] + clip[ MAT_IDX( 0, 2 )];
//    planes[MAT_IDX( 5, 1 )] = clip[ MAT_IDX( 1, 3 )] + clip[ MAT_IDX( 1, 2 )];
//    planes[MAT_IDX( 5, 2 )] = clip[ MAT_IDX( 2, 3 )] + clip[ MAT_IDX( 2, 2 )];
//    planes[MAT_IDX( 5, 3 )] = clip[ MAT_IDX( 3, 3 )] + clip[ MAT_IDX( 3, 2 )];
//
//    // Normalize The Result
//    t = SQRT( planes[MAT_IDX( 5, 0 )] * planes[MAT_IDX( 5, 0 )] + planes[MAT_IDX( 5, 1 )] * planes[MAT_IDX( 5, 1 )] + planes[MAT_IDX( 5, 2 )] * planes[MAT_IDX( 5, 2 )] );
//    planes[MAT_IDX( 5, 0 )] /= t;
//    planes[MAT_IDX( 5, 1 )] /= t;
//    planes[MAT_IDX( 5, 2 )] /= t;
//    planes[MAT_IDX( 5, 3 )] /= t;
//}
//
//--------------------------------------------------------------------------------------------
//gfx_rv gfx_project_cam_view( const camera_t * pcam, cam_corner_info_t * pinfo )
//{
//    /// @author ZZ
/// @details This function figures out where the corners of the view area
//    ///    go when projected onto the plane of the PMesh.  Used later for
//    ///    determining which mesh fans need to be rendered
//
//    enum
//    {
//        plane_RIGHT  = 0,
//        plane_LEFT   = 1,
//        plane_BOTTOM = 2,
//        plane_TOP    = 3,
//        plane_FAR    = 4,
//        plane_NEAR   = 5,
//
//        // other values
//        plane_SIZE = 4
//    };
//
//    enum
//    {
//        plane_offset_RIGHT  = plane_RIGHT  * plane_SIZE,
//        plane_offset_LEFT   = plane_LEFT   * plane_SIZE,
//        plane_offset_BOTTOM = plane_BOTTOM * plane_SIZE,
//        plane_offset_TOP    = plane_TOP    * plane_SIZE,
//        plane_offset_FAR    = plane_FAR    * plane_SIZE,
//        plane_offset_NEAR   = plane_NEAR   * plane_SIZE
//    };
//
//    enum
//    {
//        corner_TOP_LEFT = 0,
//        corner_TOP_RIGHT,
//        corner_BOTTOM_RIGHT,
//        corner_BOTTOM_LEFT,
//        corner_COUNT
//    };
//
//    int cnt, tnc, extra[2];
//    fvec3_t point;
//    int corner_count;
//    fvec3_t corner_pos[corner_COUNT], corner_dir[corner_COUNT];
//
//    // six planes with 4 components.
//    // the plane i starts at planes + 4*i
//    // plane_offset_RIGHT, plane_offset_LEFT, plane_offset_BOTTOM, plane_offset_TOP, plane_offset_FAR, plane_offset_NEAR
//    GLfloat planes[ 6 * 4 ];
//
//    GLfloat ground_plane[4] = {0.0f, 0.0f, 1.0f, 0.0f};
//
//    if ( NULL == pinfo )
//    {
//        return gfx_fail;
//    }
//
//    if ( NULL == pcam )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
//        return gfx_error;
//    }
//
//    get_clip_planes( pcam, planes );
//
//    // the number of valid points
//    corner_count = 0;
//
//    // grab the coefficients of the rays at the 4 "folds" in the frustum
//    two_plane_intersection( corner_pos[corner_TOP_LEFT].v, corner_dir[corner_TOP_LEFT].v, planes + plane_offset_LEFT, planes + plane_offset_TOP );
//    if ( corner_dir[corner_TOP_LEFT].z < 0.0f )
//    {
//        float t = corner_pos[corner_TOP_LEFT].z / corner_dir[corner_TOP_LEFT].z;
//        pinfo->x[corner_count] = corner_pos[corner_TOP_LEFT].x + corner_dir[corner_TOP_LEFT].x * t;
//        pinfo->y[corner_count] = corner_pos[corner_TOP_LEFT].y + corner_dir[corner_TOP_LEFT].y * t;
//        corner_count++;
//    }
//
//    two_plane_intersection( corner_pos[corner_TOP_RIGHT].v, corner_dir[corner_TOP_RIGHT].v, planes + plane_offset_TOP, planes + plane_offset_RIGHT );
//    if ( corner_dir[corner_TOP_RIGHT].z < 0.0f )
//    {
//        float t = corner_pos[corner_TOP_RIGHT].z / corner_dir[corner_TOP_RIGHT].z;
//        pinfo->x[corner_count] = corner_pos[corner_TOP_RIGHT].x + corner_dir[corner_TOP_RIGHT].x * t;
//        pinfo->y[corner_count] = corner_pos[corner_TOP_RIGHT].y + corner_dir[corner_TOP_RIGHT].y * t;
//        corner_count++;
//    }
//
//    two_plane_intersection( corner_pos[corner_BOTTOM_RIGHT].v, corner_dir[corner_BOTTOM_RIGHT].v, planes + plane_offset_RIGHT, planes + plane_offset_BOTTOM );
//    if ( corner_dir[corner_BOTTOM_RIGHT].z < 0.0f )
//    {
//        float t = corner_pos[corner_BOTTOM_RIGHT].z / corner_dir[corner_BOTTOM_RIGHT].z;
//        pinfo->x[corner_count] = corner_pos[corner_BOTTOM_RIGHT].x + corner_dir[corner_BOTTOM_RIGHT].x * t;
//        pinfo->y[corner_count] = corner_pos[corner_BOTTOM_RIGHT].y + corner_dir[corner_BOTTOM_RIGHT].y * t;
//        corner_count++;
//    }
//
//    two_plane_intersection( corner_pos[corner_BOTTOM_LEFT].v, corner_dir[corner_BOTTOM_LEFT].v, planes + plane_offset_BOTTOM, planes + plane_offset_LEFT );
//    if ( corner_dir[corner_BOTTOM_LEFT].z < 0.0f )
//    {
//        float t = corner_pos[corner_BOTTOM_LEFT].z / corner_dir[corner_BOTTOM_LEFT].z;
//        pinfo->x[corner_count] = corner_pos[corner_BOTTOM_LEFT].x + corner_dir[corner_BOTTOM_LEFT].x * t;
//        pinfo->y[corner_count] = corner_pos[corner_BOTTOM_LEFT].y + corner_dir[corner_BOTTOM_LEFT].y * t;
//        corner_count++;
//    }
//
//    if ( corner_count < 4 )
//    {
//        // try plane_offset_RIGHT, plane_offset_FAR, and GROUND
//        if ( three_plane_intersection( point.v, planes + plane_offset_RIGHT, planes + plane_offset_FAR, ground_plane ) )
//        {
//            pinfo->x[corner_count] = point.x;
//            pinfo->y[corner_count] = point.y;
//            corner_count++;
//        }
//    }
//
//    if ( corner_count < 4 )
//    {
//        // try plane_offset_LEFT, plane_offset_FAR, and GROUND
//        if ( three_plane_intersection( point.v, planes + plane_offset_LEFT, planes + plane_offset_FAR, ground_plane ) )
//        {
//            pinfo->x[corner_count] = point.x;
//            pinfo->y[corner_count] = point.y;
//            corner_count++;
//        }
//    }
//
//    if ( corner_count < 4 )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "not enough corner points intersecting with the ground" );
//        return gfx_error;
//    }
//
//    // Get the extreme values
//    pinfo->lowx = pinfo->x[0];
//    pinfo->lowy = pinfo->y[0];
//    pinfo->highx = pinfo->x[0];
//    pinfo->highy = pinfo->y[0];
//    pinfo->listlowtohighy[0] = 0;
//    pinfo->listlowtohighy[3] = 0;
//
//    // sort the corners
//    for ( cnt = 0; cnt < 4; cnt++ )
//    {
//        if ( pinfo->x[cnt] < pinfo->lowx )
//        {
//            pinfo->lowx = pinfo->x[cnt];
//        }
//
//        if ( pinfo->x[cnt] > pinfo->highx )
//        {
//            pinfo->highx = pinfo->x[cnt];
//        }
//
//        if ( pinfo->y[cnt] < pinfo->lowy )
//        {
//            pinfo->lowy = pinfo->y[cnt];
//            pinfo->listlowtohighy[0] = cnt;
//        }
//
//        if ( pinfo->y[cnt] > pinfo->highy )
//        {
//            pinfo->highy = pinfo->y[cnt];
//            pinfo->listlowtohighy[3] = cnt;
//        }
//    }
//
//    // Figure out the order of points
//    for ( cnt = 0, tnc = 0; cnt < 4; cnt++ )
//    {
//        if ( cnt != pinfo->listlowtohighy[0] && cnt != pinfo->listlowtohighy[3] )
//        {
//            extra[tnc] = cnt;
//            tnc++;
//        }
//    }
//
//    pinfo->listlowtohighy[1] = extra[1];
//    pinfo->listlowtohighy[2] = extra[0];
//    if ( pinfo->y[extra[0]] < pinfo->y[extra[1]] )
//    {
//        pinfo->listlowtohighy[1] = extra[0];
//        pinfo->listlowtohighy[2] = extra[1];
//    }
//
//    return gfx_success;
//}

//--------------------------------------------------------------------------------------------
//gfx_rv gfx_make_projection_bitmap( renderlist_t * prlist, cam_corner_info_t * pinfo, projection_bitmap_t * pbmp )
//{
//    int cnt, grid_x, grid_y;
//    int row, run;
//    int leftedge[4], leftedge_count;
//    int rightedge[4], rightedge_count;
//    int x, center_dx, center_dy, center_x0, center_y0;
//    int corner_x[4], corner_y[4];
//    int corner_stt, corner_end;
//
//    ego_mpd_t       * pmesh;
//
//    if ( NULL == prlist )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
//
//        return gfx_error;
//    }
//
//    if ( NULL == pinfo )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL camera frustum data" );
//
//        return gfx_error;
//    }
//
//    if ( NULL == pbmp )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL projection data" );
//
//        return gfx_error;
//    }
//
//    pmesh = renderlist_get_pmesh( prlist );
//    if ( NULL == pmesh )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL mesh" );
//        return gfx_error;
//    }
//
//    // Make life simpler
//    for ( cnt = 0; cnt < 4; cnt++ )
//    {
//        corner_x[cnt] = pinfo->x[pinfo->listlowtohighy[cnt]];
//        corner_y[cnt] = pinfo->y[pinfo->listlowtohighy[cnt]];
//    }
//
//    // It works better this way...
//    corner_y[0] -= 128;
//    corner_y[3] += 128;
//
//    // Find the center line
//    center_dx = corner_x[3] - corner_x[0];
//    center_dy = corner_y[3] - corner_y[0];
//    center_x0 = corner_x[0];
//    center_y0 = corner_y[0];
//
//    if ( center_dy < 1 )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "the corner's y-coordinates are not sorted properly" );
//        return gfx_error;
//    }
//
//    // Find the points in each edge
//
//    // bottom
//    // corner 0 is on both the left and right edges
//    leftedge[0] = 0;  leftedge_count = 1;
//    rightedge[0] = 0;  rightedge_count = 1;
//
//    // middle
//    if ( corner_x[1] < ( ( center_dx * ( corner_y[1] - corner_y[0] ) ) / center_dy ) + center_x0 )
//    {
//        // corner 1 is on the left edge
//
//        leftedge[leftedge_count] = 1;
//        leftedge_count++;
//
//        corner_x[1] -= 512;
//    }
//    else
//    {
//        // corner 1 is on the right edge
//
//        rightedge[rightedge_count] = 1;
//        rightedge_count++;
//
//        corner_x[1] += 512;
//    }
//
//    // middle
//    if ( corner_x[2] < (( center_dx * ( corner_y[2] - corner_y[0] )) / center_dy ) + center_x0 )
//    {
//        // corner 2 is on the left edge
//
//        leftedge[leftedge_count] = 2;
//        leftedge_count++;
//
//        corner_x[2] -= 512;
//    }
//    else
//    {
//        // corner 2 is on the right edge
//
//        rightedge[rightedge_count] = 2;
//        rightedge_count++;
//
//        corner_x[2] += 512;
//    }
//
//    // top
//    // corner 3 is on the left and the right edge
//    leftedge[leftedge_count] = 3;  leftedge_count++;
//    rightedge[rightedge_count] = 3;  rightedge_count++;
//
//    // Make the left edge
//    row = 0;
//    for ( cnt = 1; cnt < leftedge_count && row < PROJECTION_BITMAP_ROWS; cnt++ )
//    {
//        int grid_x, x_stt, x_end;
//        int grid_y, y_stt, y_end;
//        int grid_y_stt, grid_y_end;
//
//        corner_stt = leftedge[cnt-1];
//        corner_end = leftedge[cnt];
//
//        y_stt = corner_y[corner_stt];
//        y_end = corner_y[corner_end];
//
//        x_stt = corner_x[corner_stt];
//        x_end = corner_x[corner_end];
//
//        edge_dy   = y_end - y_stt;
//        edge_dx   = x_end - x_stt;
//        edge_dx_step = edge_dx / edge_dy;
//
//        grid_y_stt = y_stt / GRID_ISIZE;
//        grid_y_end = y_end / GRID_ISIZE;
//
//        grid_x_stt = x_stt / GRID_ISIZE;
//        grid_x_end = x_end / GRID_ISIZE;
//
//        if( grid_y_stt < 0 )
//        {
//            grid_y_stt = 0;
//
//            // move the x starting point
//            grid_x_stt -= grid_y_stt * edge_step;
//        }
//
//        if( grid_y_end > pmesh->info.tiles_y )
//        {
//            grid_y_end = pmesh->info.tiles_y;
//        }
//
//        for ( grid_y = grid_y_stt; grid_y <= grid_y_end && row < PROJECTION_BITMAP_ROWS; grid_y++  )
//        {
//            if ( grid_y >= 0 && grid_y < pmesh->info.tiles_y )
//            {
//                grid_x = x / GRID_ISIZE;
//                pbmp->corner_stt[row] = CLIP( grid_x, 0, pmesh->info.tiles_x - 1 );
//                row++;
//            }
//        }
//    }
//    pbmp->row_cnt = row;
//
//    // Make the right edge ( rowrun )
//    grid_y = corner_y[0] / GRID_ISIZE;
//    row = 0;
//
//    for( cnt = 1; cnt < rightedge_count; cnt++ )
//    {
//        corner_stt = rightedge[cnt-1];
//        corner_end = rightedge[cnt];
//
//        x = corner_x[corner_stt] + 128;
//
//        edge_dy = corner_y[corner_end] - corner_y[corner_stt];
//        edge_dx = 0;
//        if ( edge_dy > 0 )
//        {
//            edge_dx = (( corner_x[corner_end] - corner_x[corner_stt] ) * GRID_ISIZE ) / edge_dy;
//        }
//
//        run = corner_y[corner_end] / GRID_ISIZE;
//
//        for ( /* nothing */; grid_y < run; grid_y++ )
//        {
//            if ( grid_y >= 0 && grid_y < pmesh->info.tiles_y )
//            {
//                grid_x = x / GRID_ISIZE;
//                pbmp->corner_end[row] = CLIP( grid_x, 0, pmesh->info.tiles_x - 1 );
//                row++;
//            }
//
//            x += edge_dx;
//        }
//    }
//
//    if ( pbmp->row_cnt != row )
//    {
//        log_warning( "ROW error (%i, %i)\n", pbmp->row_cnt, row );
//        return gfx_fail;
//    }
//
//    // set the starting y value
//    pbmp->y_stt = corner_y[0] / GRID_ISIZE;
//
//    return gfx_success;
//}

//--------------------------------------------------------------------------------------------
//gfx_rv gfx_make_renderlist( renderlist_t * prlist, const camera_t * pcam )
//{
//    /// @author ZZ
/// @details This function figures out which mesh fans to draw
//
//    int cnt, grid_x, grid_y;
//    int row;
//
//    gfx_rv              retval;
//    cam_corner_info_t   corner;
//    projection_bitmap_t proj;
//
//    ego_mpd_t * pmesh = NULL;
//
//
//    // Make sure there is a renderlist
//    if ( NULL == prlist )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist" );
//        return gfx_error;
//    }
//
//    // make sure the renderlist is attached to a mesh
//    pmesh = renderlist_get_pmesh( prlist );
//    if ( NULL == pmesh )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "renderlist is not attached to a mesh" );
//        return gfx_error;
//    }
//
//    // make sure that we have a valid camera
//    if ( NULL == pcam )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
//        return gfx_error;
//    }
//
//    // because the main loop of the program will always flip the
//    // page before rendering the 1st frame of the actual game,
//    // game_frame_all will always start at 1
//    if ( 1 != ( game_frame_all & 3 ) )
//    {
//        return gfx_success;
//    }
//
//    // Find the render area corners.
//    // if this fails, we cannot have a valid list of corners, so this function fails
//    if ( gfx_error == gfx_project_cam_view( pcam, &corner ) )
//    {
//        return gfx_error;
//    }
//
//    // assume the best
//    retval = gfx_success;
//
//    // reset the renderlist
//    if ( gfx_error == renderlist_reset( prlist ) )
//    {
//        retval = gfx_error;
//    }
//
//    // scan through each fan... not as fact as colliding with the BSP, but maybe fast enough
//    tile_count = pmesh->info.tiles_count;
//    for ( fan = 0; fan < tile_count; fan++ )
//    {
//        inview = egolib_frustum_intersects_ego_aabb( &(pcam->frustum), mm->tilelst[fan].bbox.mins.v, mm->tilelst[fan].bbox.maxs.v );
//
//    }
//
//    // get the actual projection "bitmap"
//    gfx_make_projection_bitmap( prlist, &corner, &proj );
//
//    // fill the renderlist from the projected view
//    grid_y = proj.y_stt;
//    grid_y = CLIP( grid_y, 0, pmesh->info.tiles_y - 1 );
//    for ( row = 0; row < proj.row_cnt; row++, grid_y++ )
//    {
//        for ( grid_x = proj.row_stt[row]; grid_x <= proj.row_end[row] && prlist->all_count < MAXMESHRENDER; grid_x++ )
//        {
//            cnt = pmesh->gmem.tilestart[grid_y] + grid_x;
//
//            if ( gfx_error == gfx_capture_mesh_tile( mesh_get_ptile( pmesh, cnt ) ) )
//            {
//                retval = gfx_error;
//                goto gfx_make_renderlist_exit;
//            }
//
//            if ( gfx_error == renderlist_insert( prlist, mesh_get_pgrid( pmesh, cnt ), cnt ) )
//            {
//                retval = gfx_error;
//                goto gfx_make_renderlist_exit;
//            }
//        }
//    }
//
//gfx_make_renderlist_exit:
//    return retval;
//}

