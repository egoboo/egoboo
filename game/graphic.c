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

#include "graphic.h"
#include "graphic_prt.h"
#include "graphic_mad.h"
#include "graphic_fan.h"

#include "char.inl"
#include "particle.inl"
#include "enchant.inl"
#include "mad.h"
#include "profile.inl"
#include "mesh.inl"

#include "collision.h"

#include "log.h"
#include "script.h"
#include "camera.h"
#include "id_md2.h"
#include "input.h"
#include "network.h"
#include "passage.h"
#include "menu.h"
#include "script_compile.h"
#include "game.h"
#include "ui.h"
#include "texture.h"
#include "clock.h"
#include "font_bmp.h"
#include "lighting.h"

#include "SDL_extensions.h"
#include "SDL_GL_extensions.h"

#include "egoboo_vfs.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"

#if defined(USE_LUA_CONSOLE)
#    include "lua_console.h"
#else
#    include "egoboo_console.h"
#endif

#include "egoboo_fileutil.h"
#include "egoboo.h"

#include <SDL_image.h>

#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define SPARKLESIZE 28
#define SPARKLEADD 2
#define BLIPSIZE 6

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Structure for keeping track of which dynalights are visible
struct s_dynalight_registry
{
    int         reference;
    ego_frect_t bound;
};

typedef struct s_dynalight_registry dynalight_registry_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static SDL_bool _sdl_initialized_graphics = SDL_FALSE;
static bool_t   _ogl_initialized          = bfalse;

static float sinlut[MAXLIGHTROTATION];
static float coslut[MAXLIGHTROTATION];

// Camera optimization stuff
static float                   cornerx[4];
static float                   cornery[4];
static int                     cornerlistlowtohighy[4];
static float                   cornerlowx;
static float                   cornerhighx;
static float                   cornerlowy;
static float                   cornerhighy;

static float       dyna_distancetobeat;           // The number to beat
static int         dyna_list_count = 0;           // Number of dynamic lights
static dynalight_t dyna_list[TOTAL_MAX_DYNA];

// Interface stuff
static irect_t             iconrect;                   // The 32x32 icon rectangle

static irect_t             tabrect[NUMBAR];            // The tab rectangles
static irect_t             barrect[NUMBAR];            // The bar rectangles
static irect_t             bliprect[COLOR_MAX];        // The blip rectangles
static irect_t             maprect;                    // The map rectangle

static bool_t             gfx_page_flip_requested  = bfalse;
static bool_t             gfx_page_clear_requested = btrue;

static float dynalight_keep = 0.9f;

INSTANTIATE_LIST( ACCESS_TYPE_NONE, billboard_data_t, BillboardList, BILLBOARD_COUNT );

PROFILE_DECLARE( render_scene_init );
PROFILE_DECLARE( render_scene_mesh );
PROFILE_DECLARE( render_scene_solid );
PROFILE_DECLARE( render_scene_water );
PROFILE_DECLARE( render_scene_trans );

PROFILE_DECLARE( renderlist_make );
PROFILE_DECLARE( dolist_make );
PROFILE_DECLARE( do_grid_lighting );
PROFILE_DECLARE( light_fans );
PROFILE_DECLARE( update_all_chr_instance );
PROFILE_DECLARE( prt_instance_update_all );

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

int GFX_WIDTH  = 800;
int GFX_HEIGHT = 600;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

gfx_config_t     gfx;

SDLX_video_parameters_t sdl_vparam;
oglx_video_parameters_t ogl_vparam;

size_t                dolist_count = 0;
obj_registry_entity_t dolist[DOLIST_SIZE];

renderlist_t     renderlist = {0, 0, 0, 0, 0, 0};         // zero all the counters at startup

float            indextoenvirox[EGO_NORMAL_COUNT];
float            lighttoenviroy[256];

int rotmeshtopside;
int rotmeshbottomside;
int rotmeshup;
int rotmeshdown;

Uint8   mapon         = bfalse;
Uint8   mapvalid      = bfalse;
Uint8   youarehereon  = bfalse;

size_t  blip_count    = 0;
float   blip_x[MAXBLIP];
float   blip_y[MAXBLIP];
Uint8   blip_c[MAXBLIP];

int     msgtimechange = 0;

INSTANTIATE_STATIC_ARY( DisplayMsgAry, DisplayMsg );

line_data_t line_list[LINE_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void gfx_init_SDL_graphics();

static void _flip_pages();
static void _debug_print( const char *text );
static int  _debug_vprintf( const char *format, va_list args );
static int  _va_draw_string( int x, int y, const char *format, va_list args );
static int  _draw_string_raw( int x, int y, const char *format, ... );

static void project_view( camera_t * pcam );

static void init_icon_data();
static void init_bar_data();
static void init_blip_data();
static void init_map_data();

static bool_t render_billboard( struct s_camera * pcam, billboard_data_t * pbb, float scale );

static void gfx_update_timers();

static void gfx_begin_text();
static void gfx_end_text();

static void gfx_enable_texturing();
static void gfx_disable_texturing();

static void gfx_begin_2d( void );
static void gfx_end_2d( void );

static void light_fans( renderlist_t * prlist );
static void render_water( renderlist_t * prlist );

static void   gfx_make_dynalist( camera_t * pcam );

//--------------------------------------------------------------------------------------------
// MODULE "PRIVATE" FUNCTIONS
//--------------------------------------------------------------------------------------------
void _debug_print( const char *text )
{
    /// @details ZZ@> This function sticks a message in the display queue and sets its timer

    int          slot;
    const char * src;
    char       * dst, * dst_end;
    msg_t      * pmsg;

    if ( INVALID_CSTR( text ) ) return;

    // Get a "free" message
    slot = DisplayMsg_get_free();
    pmsg = DisplayMsg.ary + slot;

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
int _va_draw_string( int x, int y, const char *format, va_list args )
{
    int cnt = 1;
    int x_stt;
    STRING szText;
    Uint8 cTmp;

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
            // Convert ASCII to our own little font
            if ( '~' == cTmp )
            {
                // Use squiggle for tab
                x = ( x & TABAND ) + TABADD;
            }
            else if ( '\n' == cTmp )
            {
                x  = x_stt;
                y += fontyspacing;
            }
            else
            {
                // Normal letter
                cTmp = asciitofont[cTmp];
                draw_one_font( cTmp, x, y );
                x += fontxspacing[cTmp];
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
int _draw_string_raw( int x, int y, const char *format, ... )
{
    /// @details BB@> the same as draw string, but it does not use the gfx_begin_2d() ... gfx_end_2d()
    ///    bookends.

    va_list args;

    va_start( args, format );
    y = _va_draw_string( x, y, format, args );
    va_end( args );

    return y;
}

//--------------------------------------------------------------------------------------------
// MODULE INITIALIZATION
//--------------------------------------------------------------------------------------------
void gfx_system_begin()
{
    // set the graphics state
    gfx_init_SDL_graphics();
    ogl_init();

    // initialize the gfx data dtructures
    BillboardList_free_all();
    TxTexture_init_all();

    // initialize the profiling variables
    PROFILE_INIT( render_scene_init );
    PROFILE_INIT( render_scene_mesh );
    PROFILE_INIT( render_scene_solid );
    PROFILE_INIT( render_scene_water );
    PROFILE_INIT( render_scene_trans );

    PROFILE_INIT( renderlist_make );
    PROFILE_INIT( dolist_make );
    PROFILE_INIT( do_grid_lighting );
    PROFILE_INIT( light_fans );
    PROFILE_INIT( update_all_chr_instance );
    PROFILE_INIT( prt_instance_update_all );

    PROFILE_INIT( render_scene_mesh_dolist_sort );
    PROFILE_INIT( render_scene_mesh_ndr );
    PROFILE_INIT( render_scene_mesh_drf_back );
    PROFILE_INIT( render_scene_mesh_ref );
    PROFILE_INIT( render_scene_mesh_ref_chr );
    PROFILE_INIT( render_scene_mesh_drf_solid );
    PROFILE_INIT( render_scene_mesh_render_shadows );

    // init some other variables
    stabilized_fps_sum    = 0.1f * TARGET_FPS;
    stabilized_fps_weight = 0.1f;
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

    PROFILE_FREE( renderlist_make );
    PROFILE_FREE( dolist_make );
    PROFILE_FREE( do_grid_lighting );
    PROFILE_FREE( light_fans );
    PROFILE_FREE( update_all_chr_instance );
    PROFILE_FREE( prt_instance_update_all );

    PROFILE_FREE( render_scene_mesh_dolist_sort );
    PROFILE_FREE( render_scene_mesh_ndr );
    PROFILE_FREE( render_scene_mesh_drf_back );
    PROFILE_FREE( render_scene_mesh_ref );
    PROFILE_FREE( render_scene_mesh_ref_chr );
    PROFILE_FREE( render_scene_mesh_drf_solid );
    PROFILE_FREE( render_scene_mesh_render_shadows );

    BillboardList_free_all();
    TxTexture_release_all();
}

//--------------------------------------------------------------------------------------------
int ogl_init()
{
    gfx_init_SDL_graphics();

    // GL_DEBUG(glClear)) stuff
    GL_DEBUG( glClearColor )( 0.0f, 0.0f, 0.0f, 0.0f ); // Set the background black
    GL_DEBUG( glClearDepth )( 1.0f );

    // depth buffer stuff
    GL_DEBUG( glClearDepth )( 1.0f );
    GL_DEBUG( glDepthMask )( GL_TRUE );
    GL_DEBUG( glEnable )( GL_DEPTH_TEST );
    GL_DEBUG( glDepthFunc )( GL_LESS );

    // alpha stuff
    GL_DEBUG( glDisable )( GL_BLEND );
    GL_DEBUG( glEnable )( GL_ALPHA_TEST );
    GL_DEBUG( glAlphaFunc )( GL_GREATER, 0 );

    /// @todo Including backface culling here prevents the mesh from getting rendered
    // backface culling
    // GL_DEBUG(glEnable)(GL_CULL_FACE);
    // GL_DEBUG(glFrontFace)(GL_CW);            // GL_POLYGON_BIT
    // GL_DEBUG(glCullFace)(GL_BACK);

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
	GL_DEBUG( glClearAccum )( 0.0, 0.0, 0.0, 1.0 );
	GL_DEBUG( glClear )( GL_ACCUM_BUFFER_BIT );

    // Load the current graphical settings
    // load_graphics();

    _ogl_initialized = btrue;

    return _ogl_initialized && _sdl_initialized_graphics;
}

//--------------------------------------------------------------------------------------------
void gfx_init_SDL_graphics()
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
        char * fname = "icon.bmp";
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
    if ( cfg.scrd_req == 32 ) cfg.scrd_req = 24;
	if ( cfg.scrz_req == 32 ) cfg.scrz_req = 24;

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
bool_t gfx_set_virtual_screen( gfx_config_t * pgfx )
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

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t gfx_synch_config( gfx_config_t * pgfx, egoboo_config_t * pcfg )
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

    pgfx->shading      = pcfg->gourard_req ? GL_SMOOTH : GL_FLAT;
    pgfx->dither       = pcfg->use_dither;
    pgfx->perspective  = pcfg->use_perspective;
    pgfx->phongon      = pcfg->use_phong;

    pgfx->draw_background = pcfg->background_allowed && water.background_req;
    pgfx->draw_overlay    = pcfg->overlay_allowed && water.overlay_req;

    pgfx->dyna_list_max = CLIP( pcfg->dyna_count_req, 0, TOTAL_MAX_DYNA );

    pgfx->draw_water_0 = !pgfx->draw_overlay && ( water.layer_count > 0 );
    pgfx->clearson     = !pgfx->draw_background;
    pgfx->draw_water_1 = !pgfx->draw_background && ( water.layer_count > 1 );

    gfx_set_virtual_screen( pgfx );

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

    pgfx->dyna_list_max    = 8;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t gfx_synch_oglx_texture_parameters( oglx_texture_parameters_t * ptex, egoboo_config_t * pcfg )
{
    //// @details BB@> synch the texture parameters with the video mode

    if ( NULL == ptex || NULL == pcfg ) return GL_FALSE;

    if ( ogl_caps.maxAnisotropy == 0.0f )
    {
        ptex->userAnisotropy = 0.0f;
        ptex->texturefilter  = ( TX_FILTERS )MIN( pcfg->texturefilter_req, TX_TRILINEAR_2 );
    }
    else
    {
        ptex->texturefilter  = ( TX_FILTERS )MIN( pcfg->texturefilter_req, TX_FILTER_COUNT );
        ptex->userAnisotropy = ogl_caps.maxAnisotropy * MAX( 0, ( int )ptex->texturefilter - ( int )TX_TRILINEAR_2 );
    }

    return GL_TRUE;
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
void draw_blip( float sizeFactor, Uint8 color, int x, int y, bool_t mini_map )
{
    /// @details ZZ@> This function draws a single blip
    frect_t txrect;
    float   width, height;

    //Adjust the position values so that they fit inside the minimap
    if ( mini_map )
    {
        x = x * MAPSIZE / PMesh->gmem.edge_x;
        y = ( y * MAPSIZE / PMesh->gmem.edge_y ) + sdl_scr.y - MAPSIZE;
    }

    //Now draw it
    if ( x > 0 && y > 0 )
    {
        oglx_texture_t * ptex = TxTexture_get_ptr(( TX_REF )TX_BLIP );

        gfx_enable_texturing();
        GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f );
        GL_DEBUG( glNormal3f )( 0.0f, 0.0f, 1.0f );

        oglx_texture_Bind( ptex );

        txrect.left   = ( float )bliprect[color].left   / ( float )oglx_texture_GetTextureWidth( ptex );
        txrect.right  = ( float )bliprect[color].right  / ( float )oglx_texture_GetTextureWidth( ptex );
        txrect.top    = ( float )bliprect[color].top    / ( float )oglx_texture_GetTextureHeight( ptex );
        txrect.bottom = ( float )bliprect[color].bottom / ( float )oglx_texture_GetTextureHeight( ptex );

        width  = bliprect[color].right  - bliprect[color].left;
        height = bliprect[color].bottom - bliprect[color].top;

        width  *= sizeFactor;
        height *= sizeFactor;

        GL_DEBUG( glBegin )( GL_QUADS );
        {
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.bottom ); GL_DEBUG( glVertex2f )( x - ( width / 2 ), y + ( height / 2 ) );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.bottom ); GL_DEBUG( glVertex2f )( x + ( width / 2 ), y + ( height / 2 ) );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.top ); GL_DEBUG( glVertex2f )( x + ( width / 2 ), y - ( height / 2 ) );
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.top ); GL_DEBUG( glVertex2f )( x - ( width / 2 ), y - ( height / 2 ) );
        }
        GL_DEBUG_END();
    }
}

//--------------------------------------------------------------------------------------------
void draw_one_icon( const TX_REF by_reference icontype, int x, int y, Uint8 sparkle )
{
    /// @details ZZ@> This function draws an icon
    int     position, blip_x, blip_y;
    int     width, height;
    frect_t txrect;

    oglx_texture_t * ptex = TxTexture_get_ptr( icontype );

    gfx_enable_texturing();    // Enable texture mapping
    GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f );

    oglx_texture_Bind( ptex );

    txrect.left   = (( float )iconrect.left ) / 32.0f;
    txrect.right  = (( float )iconrect.right ) / 32.0f;
    txrect.top    = (( float )iconrect.top ) / 32.0f;
    txrect.bottom = (( float )iconrect.bottom ) / 32.0f;

    width  = iconrect.right  - iconrect.left;
    height = iconrect.bottom - iconrect.top;

    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.bottom ); GL_DEBUG( glVertex2i )( x,         y + height );
        GL_DEBUG( glTexCoord2f )( txrect.right, txrect.bottom ); GL_DEBUG( glVertex2i )( x + width, y + height );
        GL_DEBUG( glTexCoord2f )( txrect.right, txrect.top ); GL_DEBUG( glVertex2i )( x + width, y );
        GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.top ); GL_DEBUG( glVertex2i )( x,         y );
    }
    GL_DEBUG_END();

    if ( sparkle != NOSPARKLE )
    {
        position = update_wld & 0x1F;
        position = ( SPARKLESIZE * position >> 5 );

        blip_x = x + SPARKLEADD + position;
        blip_y = y + SPARKLEADD;
        draw_blip( 0.5f, sparkle, blip_x, blip_y, bfalse );

        blip_x = x + SPARKLEADD + SPARKLESIZE;
        blip_y = y + SPARKLEADD + position;
        draw_blip( 0.5f, sparkle, blip_x, blip_y, bfalse );

        blip_x = blip_x - position;
        blip_y = y + SPARKLEADD + SPARKLESIZE;
        draw_blip( 0.5f, sparkle, blip_x, blip_y, bfalse );

        blip_x = x + SPARKLEADD;
        blip_y = blip_y - position;
        draw_blip( 0.5f, sparkle, blip_x, blip_y, bfalse );
    }
}

//--------------------------------------------------------------------------------------------
void draw_one_font( int fonttype, int x, int y )
{
    /// @details ZZ@> This function draws a letter or number
    /// GAC@> Very nasty version for starters.  Lots of room for improvement.

    GLfloat dx, dy, fx1, fx2, fy1, fy2, border;
    GLuint x2, y2;

    y  += fontoffset;
    x2  = x + fontrect[fonttype].w;
    y2  = y - fontrect[fonttype].h;

    dx = 2.0f / 512.0f;
    dy = 1.0f / 256.0f;
    border = 1.0f / 512.0f;

    fx1 = fontrect[fonttype].x * dx + border;
    fx2 = ( fontrect[fonttype].x + fontrect[fonttype].w ) * dx - border;
    fy1 = fontrect[fonttype].y * dy + border;
    fy2 = ( fontrect[fonttype].y + fontrect[fonttype].h ) * dy - border;

    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glTexCoord2f )( fx1, fy2 );   GL_DEBUG( glVertex2i )( x, y );
        GL_DEBUG( glTexCoord2f )( fx2, fy2 );   GL_DEBUG( glVertex2i )( x2, y );
        GL_DEBUG( glTexCoord2f )( fx2, fy1 );   GL_DEBUG( glVertex2i )( x2, y2 );
        GL_DEBUG( glTexCoord2f )( fx1, fy1 );   GL_DEBUG( glVertex2i )( x, y2 );
    }
    GL_DEBUG_END();
}

//--------------------------------------------------------------------------------------------
void draw_map_texture( int x, int y )
{
    /// @details ZZ@> This function draws the map
    gfx_enable_texturing();

    oglx_texture_Bind( TxTexture_get_ptr(( TX_REF )TX_MAP ) );

    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glTexCoord2f )( 0.0f, 1.0f ); GL_DEBUG( glVertex2i )( x,           y + MAPSIZE );
        GL_DEBUG( glTexCoord2f )( 1.0f, 1.0f ); GL_DEBUG( glVertex2i )( x + MAPSIZE, y + MAPSIZE );
        GL_DEBUG( glTexCoord2f )( 1.0f, 0.0f ); GL_DEBUG( glVertex2i )( x + MAPSIZE, y );
        GL_DEBUG( glTexCoord2f )( 0.0f, 0.0f ); GL_DEBUG( glVertex2i )( x,           y );
    }
    GL_DEBUG_END();
}

//--------------------------------------------------------------------------------------------
int draw_one_xp_bar( int x, int y, Uint8 ticks )
{
    /// @details ZF@> This function draws a xp bar and returns the y position for the next one

    int width, height;
    Uint8 cnt;
    frect_t txrect;

    ticks = MIN( ticks, NUMTICK );

    gfx_enable_texturing();               // Enable texture mapping
    GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

    // Draw the tab (always colored)
    oglx_texture_Bind( TxTexture_get_ptr(( TX_REF )TX_XP_BAR ) );

    txrect.left   = 0;
    txrect.right  = 32.00f / 128;
    txrect.top    = XPTICK / 16;
    txrect.bottom = XPTICK * 2 / 16;

    width = 16;
    height = XPTICK;

    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.bottom ); GL_DEBUG( glVertex2i )( x,         y + height );
        GL_DEBUG( glTexCoord2f )( txrect.right, txrect.bottom ); GL_DEBUG( glVertex2i )( x + width, y + height );
        GL_DEBUG( glTexCoord2f )( txrect.right, txrect.top ); GL_DEBUG( glVertex2i )( x + width, y );
        GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.top ); GL_DEBUG( glVertex2i )( x,         y );
    }
    GL_DEBUG_END();
    x += 16;

    // Draw the filled ones
    txrect.left   = 0;
    txrect.right  = 32.00f / 128;
    txrect.top    = XPTICK / 16;
    txrect.bottom = XPTICK * 2 / 16;

    width  = XPTICK;
    height = XPTICK;

    for ( cnt = 0; cnt < ticks; cnt++ )
    {
        oglx_texture_Bind( TxTexture_get_ptr(( TX_REF )TX_XP_BAR ) );
        GL_DEBUG( glBegin )( GL_QUADS );
        {
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.bottom ); GL_DEBUG( glVertex2i )(( cnt * width ) + x,         y + height );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.bottom ); GL_DEBUG( glVertex2i )(( cnt * width ) + x + width, y + height );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.top ); GL_DEBUG( glVertex2i )(( cnt * width ) + x + width, y );
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.top ); GL_DEBUG( glVertex2i )(( cnt * width ) + x,         y );
        }
        GL_DEBUG_END();
    }

    // Draw the remaining empty ones
    txrect.left   = 0;
    txrect.right  = 32.00f / 128;
    txrect.top    = 0;
    txrect.bottom = XPTICK / 16;

    width = XPTICK;
    height = XPTICK;

    for ( /*nothing*/; cnt < NUMTICK; cnt++ )
    {
        oglx_texture_Bind( TxTexture_get_ptr(( TX_REF )TX_XP_BAR ) );
        GL_DEBUG( glBegin )( GL_QUADS );
        {
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.bottom ); GL_DEBUG( glVertex2i )(( cnt * width ) + x,         y + height );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.bottom ); GL_DEBUG( glVertex2i )(( cnt * width ) + x + width, y + height );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.top ); GL_DEBUG( glVertex2i )(( cnt * width ) + x + width, y );
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.top ); GL_DEBUG( glVertex2i )(( cnt * width ) + x,         y );
        }
        GL_DEBUG_END();
    }

    return y + XPTICK;
}

//--------------------------------------------------------------------------------------------
int draw_one_bar( Uint8 bartype, int x, int y, int ticks, int maxticks )
{
    /// @details ZZ@> This function draws a bar and returns the y position for the next one

    int     noticks;
    int     width, height;
    frect_t txrect;

    if ( maxticks <= 0 || ticks < 0 || bartype > NUMBAR ) return y;

    gfx_enable_texturing();               // Enable texture mapping
    GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

    // Draw the tab
    oglx_texture_Bind( TxTexture_get_ptr(( TX_REF )TX_BARS ) );

    txrect.left   = tabrect[bartype].left   / 128.0f;
    txrect.right  = tabrect[bartype].right  / 128.0f;
    txrect.top    = tabrect[bartype].top    / 128.0f;
    txrect.bottom = tabrect[bartype].bottom / 128.0f;

    width  = tabrect[bartype].right  - tabrect[bartype].left;
    height = tabrect[bartype].bottom - tabrect[bartype].top;

    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.bottom ); GL_DEBUG( glVertex2i )( x,         y + height );
        GL_DEBUG( glTexCoord2f )( txrect.right, txrect.bottom ); GL_DEBUG( glVertex2i )( x + width, y + height );
        GL_DEBUG( glTexCoord2f )( txrect.right, txrect.top ); GL_DEBUG( glVertex2i )( x + width, y );
        GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.top ); GL_DEBUG( glVertex2i )( x,         y );
    }
    GL_DEBUG_END();

    // Error check
    if ( maxticks > MAXTICK ) maxticks = MAXTICK;
    if ( ticks > maxticks ) ticks = maxticks;

    // use the bars texture
    oglx_texture_Bind( TxTexture_get_ptr(( TX_REF )TX_BARS ) );

    // Draw the full rows of ticks
    x += TABX;

    while ( ticks >= NUMTICK )
    {
        barrect[bartype].right = BARX;

        txrect.left   = barrect[bartype].left   / 128.0f;
        txrect.right  = barrect[bartype].right  / 128.0f;
        txrect.top    = barrect[bartype].top    / 128.0f;
        txrect.bottom = barrect[bartype].bottom / 128.0f;

        width  = barrect[bartype].right - barrect[bartype].left;
        height = barrect[bartype].bottom - barrect[bartype].top;

        GL_DEBUG( glBegin )( GL_QUADS );
        {
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.bottom ); GL_DEBUG( glVertex2i )( x,         y + height );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.bottom ); GL_DEBUG( glVertex2i )( x + width, y + height );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.top ); GL_DEBUG( glVertex2i )( x + width, y );
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.top ); GL_DEBUG( glVertex2i )( x,         y );
        }
        GL_DEBUG_END();

        y += BARY;
        ticks -= NUMTICK;
        maxticks -= NUMTICK;
    }

    // Draw any partial rows of ticks
    if ( maxticks > 0 )
    {
        // Draw the filled ones
        barrect[bartype].right = ( ticks << 3 ) + TABX;

        txrect.left   = barrect[bartype].left   / 128.0f;
        txrect.right  = barrect[bartype].right  / 128.0f;
        txrect.top    = barrect[bartype].top    / 128.0f;
        txrect.bottom = barrect[bartype].bottom / 128.0f;

        width = barrect[bartype].right - barrect[bartype].left;
        height = barrect[bartype].bottom - barrect[bartype].top;

        GL_DEBUG( glBegin )( GL_QUADS );
        {
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.bottom ); GL_DEBUG( glVertex2i )( x,         y + height );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.bottom ); GL_DEBUG( glVertex2i )( x + width, y + height );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.top ); GL_DEBUG( glVertex2i )( x + width, y );
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.top ); GL_DEBUG( glVertex2i )( x,         y );
        }
        GL_DEBUG_END();

        // Draw the empty ones
        noticks = maxticks - ticks;
        if ( noticks > ( NUMTICK - ticks ) ) noticks = ( NUMTICK - ticks );

        barrect[0].right = ( noticks << 3 ) + TABX;
        oglx_texture_Bind( TxTexture_get_ptr(( TX_REF )TX_BARS ) );

        txrect.left   = barrect[0].left   / 128.0f;
        txrect.right  = barrect[0].right  / 128.0f;
        txrect.top    = barrect[0].top    / 128.0f;
        txrect.bottom = barrect[0].bottom / 128.0f;

        width = barrect[0].right - barrect[0].left;
        height = barrect[0].bottom - barrect[0].top;

        GL_DEBUG( glBegin )( GL_QUADS );
        {
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.bottom ); GL_DEBUG( glVertex2i )(( ticks << 3 ) + x,         y + height );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.bottom ); GL_DEBUG( glVertex2i )(( ticks << 3 ) + x + width, y + height );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.top ); GL_DEBUG( glVertex2i )(( ticks << 3 ) + x + width, y );
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.top ); GL_DEBUG( glVertex2i )(( ticks << 3 ) + x,         y );
        }
        GL_DEBUG_END();

        maxticks -= NUMTICK;
        y += BARY;
    }

    // Draw full rows of empty ticks
    while ( maxticks >= NUMTICK )
    {
        barrect[0].right = BARX;

        txrect.left   = barrect[0].left   / 128.0f;
        txrect.right  = barrect[0].right  / 128.0f;
        txrect.top    = barrect[0].top    / 128.0f;
        txrect.bottom = barrect[0].bottom / 128.0f;

        width = barrect[0].right - barrect[0].left;
        height = barrect[0].bottom - barrect[0].top;

        GL_DEBUG( glBegin )( GL_QUADS );
        {
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.bottom );   GL_DEBUG( glVertex2i )( x,         y + height );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.bottom );   GL_DEBUG( glVertex2i )( x + width, y + height );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.top );   GL_DEBUG( glVertex2i )( x + width, y );
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.top );   GL_DEBUG( glVertex2i )( x,         y );
        }
        GL_DEBUG_END();

        y += BARY;
        maxticks -= NUMTICK;
    }

    // Draw the last of the empty ones
    if ( maxticks > 0 )
    {
        barrect[0].right = ( maxticks << 3 ) + TABX;
        oglx_texture_Bind( TxTexture_get_ptr(( TX_REF )TX_BARS ) );

        txrect.left   = barrect[0].left   / 128.0f;
        txrect.right  = barrect[0].right  / 128.0f;
        txrect.top    = barrect[0].top    / 128.0f;
        txrect.bottom = barrect[0].bottom / 128.0f;

        width = barrect[0].right - barrect[0].left;
        height = barrect[0].bottom - barrect[0].top;

        GL_DEBUG( glBegin )( GL_QUADS );
        {
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.bottom ); GL_DEBUG( glVertex2i )( x,         y + height );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.bottom ); GL_DEBUG( glVertex2i )( x + width, y + height );
            GL_DEBUG( glTexCoord2f )( txrect.right, txrect.top ); GL_DEBUG( glVertex2i )( x + width, y );
            GL_DEBUG( glTexCoord2f )( txrect.left,  txrect.top ); GL_DEBUG( glVertex2i )( x,         y );
        }
        GL_DEBUG_END();

        maxticks -= NUMTICK;
        y += BARY;
    }

    return y;
}

//--------------------------------------------------------------------------------------------
int draw_string( int x, int y, const char *format, ... )
{
    /// @details ZZ@> This function spits a line of null terminated text onto the backbuffer
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
int draw_wrap_string( const char *szText, int x, int y, int maxx )
{
    /// @details ZZ@> This function spits a line of null terminated text onto the backbuffer,
    ///    wrapping over the right side and returning the new y value

    int stt_x = x;
    Uint8 cTmp = szText[0];
    int newy = y + fontyspacing;
    Uint8 newword = btrue;
    int cnt = 1;

    gfx_begin_text();

    maxx = maxx + stt_x;

    while ( cTmp != 0 )
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
            if ( '~' == cTmp )
            {
                // Use squiggle for tab
                x = ( x & TABAND ) + TABADD;
            }
            else if ( '\n' == cTmp )
            {
                x = stt_x;
                y += fontyspacing;
                newy += fontyspacing;
            }
            else
            {
                // Normal letter
                cTmp = asciitofont[cTmp];
                draw_one_font( cTmp, x, y );
                x += fontxspacing[cTmp];
            }

            cTmp = szText[cnt];
            if ( '~' == cTmp || ' ' == cTmp )
            {
                newword = btrue;
            }

            cnt++;
        }
    }

    gfx_end_text();
    return newy;
}

//--------------------------------------------------------------------------------------------
void draw_one_character_icon( const CHR_REF by_reference item, int x, int y, bool_t draw_ammo )
{
    /// @details BB@> Draw an icon for the given item at the position <x,y>.
    ///     If the object is invalid, draw the null icon instead of failing

    TX_REF icon_ref;
    Uint8  draw_sparkle;

    chr_t * pitem = !INGAME_CHR( item ) ? NULL : ChrList.lst + item;

    // grab the icon reference
    icon_ref = chr_get_icon_ref( item );

    // draw the icon
    draw_sparkle = ( NULL == pitem ) ? NOSPARKLE : pitem->sparkle;
    draw_one_icon( icon_ref, x, y, draw_sparkle );

    // draw the ammo, if requested
    if ( draw_ammo && ( NULL != pitem ) )
    {
        if ( pitem->ammomax != 0 && pitem->ammoknown )
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
int draw_character_xp_bar( const CHR_REF by_reference character, int x, int y )
{
    chr_t * pchr;
    cap_t * pcap;

    if ( !INGAME_CHR( character ) ) return y;
    pchr = ChrList.lst + character;

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
int draw_status( const CHR_REF by_reference character, int x, int y )
{
    /// @details ZZ@> This function shows a character's icon, status and inventory
    ///    The x,y coordinates are the top left point of the image to draw

    int cnt;
    char cTmp;
    char *readtext;
    STRING generictext;
    int life, lifemax;
    int mana, manamax;

    chr_t * pchr;
    cap_t * pcap;

    if ( !INGAME_CHR( character ) ) return y;
    pchr = ChrList.lst + character;

    pcap = chr_get_pcap( character );
    if ( NULL == pcap ) return y;

    life     = FP8_TO_INT( pchr->life );
    lifemax  = FP8_TO_INT( pchr->lifemax );
    mana     = FP8_TO_INT( pchr->mana );
    manamax  = FP8_TO_INT( pchr->manamax );

    // grab the character's display name
    readtext = ( char * )chr_get_name( character, CHRNAME_CAPITAL );

    // make a short name for the actual display
    for ( cnt = 0; cnt < 7; cnt++ )
    {
        cTmp = readtext[cnt];

        if ( ' ' == cTmp || CSTR_END == cTmp )
        {
            generictext[cnt] = CSTR_END;
            break;
        }
        else
        {
            generictext[cnt] = cTmp;
        }
    }
    generictext[7] = CSTR_END;

    // draw the name
    y = _draw_string_raw( x + 8, y, generictext );

    // draw the character's money
    y = _draw_string_raw( x + 8, y, "$%4d", pchr->money ) + 8;

    // draw the character's main icon
    draw_one_character_icon( character, x + 40, y, bfalse );

    // draw the left hand item icon
    draw_one_character_icon( pchr->holdingwhich[SLOT_LEFT], x + 8, y, btrue );

    // draw the right hand item icon
    draw_one_character_icon( pchr->holdingwhich[SLOT_RIGHT], x + 72, y, btrue );

    // skip to the next row
    y += 32;

    //Draw the small XP progress bar
    y = draw_character_xp_bar( character, x + 16, y );

    // Draw the life bar
    if ( pchr->alive )
    {
        y = draw_one_bar( pchr->lifecolor, x, y, life, lifemax );
    }
    else
    {
        y = draw_one_bar( 0, x, y, 0, lifemax );  // Draw a black bar
    }

    // Draw the mana bar
    if ( manamax > 0 )
    {
        y = draw_one_bar( pchr->manacolor, x, y, mana, manamax );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
int draw_all_status( int y )
{
    int cnt;

    if ( StatusList_on )
    {
        for ( cnt = 0; cnt < StatusList_count && y < sdl_scr.y; cnt++ )
        {
            y = draw_status( StatusList[cnt], sdl_scr.x - BARX, y );
        }
    }

    return y;
}

//--------------------------------------------------------------------------------------------
void draw_map()
{
    int cnt;

    // Map display
    if ( !mapvalid || !mapon ) return;

    ATTRIB_PUSH( "draw_map()", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );
    {

        GL_DEBUG( glEnable )( GL_BLEND );                               // GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT
        GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

        GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f );
        draw_map_texture( 0, sdl_scr.y - MAPSIZE );

        GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

        // If one of the players can sense enemies via EMP, draw them as blips on the map
        if ( TEAM_MAX != local_senseenemiesTeam )
        {
            CHR_REF ichr;

            for ( ichr = 0; ichr < MAX_CHR && blip_count < MAXBLIP; ichr++ )
            {
                chr_t * pchr;
                cap_t * pcap;

                if ( !INGAME_CHR( ichr ) ) continue;
                pchr = ChrList.lst + ichr;

                pcap = chr_get_pcap( ichr );
                if ( NULL == pcap ) continue;

                // Show only teams that will attack the player
                if ( team_hates_team( pchr->team, local_senseenemiesTeam ) )
                {
                    // Only if they match the required IDSZ ([NONE] always works)
                    if ( local_senseenemiesID == IDSZ_NONE ||
                        local_senseenemiesID == pcap->idsz[IDSZ_PARENT] ||
                        local_senseenemiesID == pcap->idsz[IDSZ_TYPE  ] )
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

                if ( INPUT_BITS_NONE != PlaStack.lst[iplayer].device.bits )
                {
                    CHR_REF ichr = PlaStack.lst[iplayer].index;
                    if ( INGAME_CHR( ichr ) && ChrList.lst[ichr].alive )
                    {
                        draw_blip( 0.75f, COLOR_WHITE, ChrList.lst[ichr].pos.x, ChrList.lst[ichr].pos.y, btrue );
                    }
                }
            }
        }

        //// draw the camera
        // if ( update_wld & 2 )
        // {
        //   draw_blip( 0.75f, COLOR_PURPLE, GET_MAP_X(PMesh, PCamera->pos.x), GET_MAP_Y(PMesh, PCamera->pos.y));
        // }
    }
    ATTRIB_POP( "draw_map()" )
}

//--------------------------------------------------------------------------------------------
int draw_fps( int y )
{
    // FPS text

    if ( outofsync )
    {
        y = _draw_string_raw( 0, y, "OUT OF SYNC" );
    }

    if ( parseerror )
    {
        y = _draw_string_raw( 0, y, "SCRIPT ERROR ( see \"/debug/log.txt\" )" );
    }

    if ( fpson )
    {
        y = _draw_string_raw( 0, y, "%2.3f FPS, %2.3f UPS, Update lag = %d", stabilized_fps, stabilized_ups, update_lag );

#if defined(_DEBUG)

#    if defined(DEBUG_BSP) && defined(_DEBUG)
        y = _draw_string_raw( 0, y, "BSP chr %d/%d - BSP prt %d/%d", BSP_chr_count, MAX_CHR - chr_count_free(), BSP_prt_count, maxparticles - prt_count_free() );
        y = _draw_string_raw( 0, y, "BSP collisions %d", CHashList_inserted );
        y = _draw_string_raw( 0, y, "chr-mesh tests %04d - prt-mesh tests %04d", chr_stoppedby_tests + chr_pressure_tests, prt_stoppedby_tests + prt_pressure_tests );
#    endif

#if defined(DEBUG_RENDERLIST) && defined(_DEBUG)
        y = _draw_string_raw( 0, y, "Renderlist tiles %d/%d", renderlist.all_count, PMesh->info.tiles_count  );
#endif

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
        y = _draw_string_raw( 0, y, "init:renderlist_make %2.4f, init:dolist_make %2.4f", time_render_scene_init_renderlist_make, time_render_scene_init_dolist_make );
        y = _draw_string_raw( 0, y, "init:do_grid_lighting %2.4f, init:light_fans %2.4f", time_render_scene_init_do_grid_dynalight, time_render_scene_init_light_fans );
        y = _draw_string_raw( 0, y, "init:update_all_chr_instance %2.4f", time_render_scene_init_update_all_chr_instance );
        y = _draw_string_raw( 0, y, "init:prt_instance_update_all %2.4f", time_render_scene_init_update_all_prt_instance );
#        endif

#    endif

#endif
    }

    return y;
}

//--------------------------------------------------------------------------------------------
int draw_help( int y )
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
int draw_debug( int y )
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
        y = _draw_string_raw( 0, y, "~~PLA0DEF %d %d %d %d %d %d %d %d",
            ChrList.lst[ichr].damagemodifier[DAMAGE_SLASH] & 3,
            ChrList.lst[ichr].damagemodifier[DAMAGE_CRUSH] & 3,
            ChrList.lst[ichr].damagemodifier[DAMAGE_POKE ] & 3,
            ChrList.lst[ichr].damagemodifier[DAMAGE_HOLY ] & 3,
            ChrList.lst[ichr].damagemodifier[DAMAGE_EVIL ] & 3,
            ChrList.lst[ichr].damagemodifier[DAMAGE_FIRE ] & 3,
            ChrList.lst[ichr].damagemodifier[DAMAGE_ICE  ] & 3,
            ChrList.lst[ichr].damagemodifier[DAMAGE_ZAP  ] & 3 );

        ichr = PlaStack.lst[ipla].index;
        y = _draw_string_raw( 0, y, "~~PLA0 %5.1f %5.1f", ChrList.lst[ichr].pos.x / GRID_SIZE, ChrList.lst[ichr].pos.y / GRID_SIZE );

        ipla = ( PLA_REF )1;
        ichr = PlaStack.lst[ipla].index;
        y = _draw_string_raw( 0, y, "~~PLA1 %5.1f %5.1f", ChrList.lst[ichr].pos.x / GRID_SIZE, ChrList.lst[ichr].pos.y / GRID_SIZE );
    }

    if ( SDLKEYDOWN( SDLK_F6 ) )
    {
        // More debug information
        STRING text;

        y = _draw_string_raw( 0, y, "!!!DEBUG MODE-6!!!" );
        y = _draw_string_raw( 0, y, "~~FREEPRT %d", prt_count_free() );
        y = _draw_string_raw( 0, y, "~~FREECHR %d", chr_count_free() );
        y = _draw_string_raw( 0, y, "~~MACHINE %d", local_machine );
        if ( PMod->exportvalid ) snprintf( text, SDL_arraysize( text ), "~~EXPORT: TRUE" );
        else                    snprintf( text, SDL_arraysize( text ), "~~EXPORT: FALSE" );
        y = _draw_string_raw( 0, y, text, PMod->exportvalid );
        y = _draw_string_raw( 0, y, "~~PASS %d/%d", ShopStack.count, PassageStack.count );
        y = _draw_string_raw( 0, y, "~~NETPLAYERS %d", numplayer );
        y = _draw_string_raw( 0, y, "~~DAMAGEPART %d", damagetile.parttype );

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
int draw_timer( int y )
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
int draw_game_status( int y )
{

    if ( PNet->waitingforplayers )
    {
        y = _draw_string_raw( 0, y, "Waiting for players... " );
    }

    if ( numplayer > 0 )
    {
        if ( local_allpladead || PMod->respawnanytime )
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
    else y = _draw_string_raw( 0, y, "ERROR: MISSING PLAYERS" );

    return y;
}

//--------------------------------------------------------------------------------------------
int draw_messages( int y )
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
void draw_text()
{
    /// @details ZZ@> draw in-game heads up display

    int y;

    gfx_begin_2d();
    {
        draw_map();

        y = draw_all_status( 0 );

        y = draw_fps( 0 );
        y = draw_help( y );
        y = draw_debug( y );
        y = draw_timer( y );
        y = draw_game_status( y );

        // Network message input
        if ( console_mode )
        {
            char buffer[KEYB_BUFFER_SIZE + 128] = EMPTY_CSTR;

            snprintf( buffer, SDL_arraysize( buffer ), "%s > %s%s", cfg.network_messagename, keyb.buffer, HAS_NO_BITS( update_wld, 8 ) ? "x" : "+" );

            y = draw_wrap_string( buffer, 0, y, sdl_scr.x - wraptolerance );
        }

        y = draw_messages( y );
    }
    gfx_end_2d();
}

//--------------------------------------------------------------------------------------------
// 3D RENDERER FUNCTIONS
//--------------------------------------------------------------------------------------------
void project_view( camera_t * pcam )
{
    /// @details ZZ@> This function figures out where the corners of the view area
    ///    go when projected onto the plane of the PMesh->  Used later for
    ///    determining which mesh fans need to be rendered

    int cnt, tnc, extra[4];
    float ztemp;
    float numstep;
    float zproject;
    float xfin, yfin, zfin;
    fmat_4x4_t mTemp;

    // Range
    ztemp = ( pcam->pos.z );

    // Topleft
    mTemp = MatrixMult( RotateY( -rotmeshtopside * PI / 360 ), PCamera->mView );
    mTemp = MatrixMult( RotateX( rotmeshup * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             // 2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->pos.x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      // 0,2
        yfin = pcam->pos.y + ( numstep * mTemp.CNV( 1, 2 ) );    // 1,2
        zfin = 0;
        cornerx[0] = xfin;
        cornery[0] = yfin;
    }

    // Topright
    mTemp = MatrixMult( RotateY( rotmeshtopside * PI / 360 ), PCamera->mView );
    mTemp = MatrixMult( RotateX( rotmeshup * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             // 2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->pos.x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      // 0,2
        yfin = pcam->pos.y + ( numstep * mTemp.CNV( 1, 2 ) );    // 1,2
        zfin = 0;
        cornerx[1] = xfin;
        cornery[1] = yfin;
    }

    // Bottomright
    mTemp = MatrixMult( RotateY( rotmeshbottomside * PI / 360 ), PCamera->mView );
    mTemp = MatrixMult( RotateX( -rotmeshdown * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             // 2,2

    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->pos.x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      // 0,2
        yfin = pcam->pos.y + ( numstep * mTemp.CNV( 1, 2 ) );    // 1,2
        zfin = 0;
        cornerx[2] = xfin;
        cornery[2] = yfin;
    }

    // Bottomleft
    mTemp = MatrixMult( RotateY( -rotmeshbottomside * PI / 360 ), PCamera->mView );
    mTemp = MatrixMult( RotateX( -rotmeshdown * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             // 2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->pos.x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      // 0,2
        yfin = pcam->pos.y + ( numstep * mTemp.CNV( 1, 2 ) );    // 1,2
        zfin = 0;
        cornerx[3] = xfin;
        cornery[3] = yfin;
    }

    // Get the extreme values
    cornerlowx = cornerx[0];
    cornerlowy = cornery[0];
    cornerhighx = cornerx[0];
    cornerhighy = cornery[0];
    cornerlistlowtohighy[0] = 0;
    cornerlistlowtohighy[3] = 0;

    for ( cnt = 0; cnt < 4; cnt++ )
    {
        if ( cornerx[cnt] < cornerlowx )
            cornerlowx = cornerx[cnt];
        if ( cornery[cnt] < cornerlowy )
        {
            cornerlowy = cornery[cnt];
            cornerlistlowtohighy[0] = cnt;
        }
        if ( cornerx[cnt] > cornerhighx )
            cornerhighx = cornerx[cnt];
        if ( cornery[cnt] > cornerhighy )
        {
            cornerhighy = cornery[cnt];
            cornerlistlowtohighy[3] = cnt;
        }
    }

    // Figure out the order of points
    for ( cnt = 0, tnc = 0; cnt < 4; cnt++ )
    {
        if ( cnt != cornerlistlowtohighy[0] && cnt != cornerlistlowtohighy[3] )
        {
            extra[tnc] = cnt;
            tnc++;
        }
    }

    cornerlistlowtohighy[1] = extra[1];
    cornerlistlowtohighy[2] = extra[0];
    if ( cornery[extra[0]] < cornery[extra[1]] )
    {
        cornerlistlowtohighy[1] = extra[0];
        cornerlistlowtohighy[2] = extra[1];
    }
}

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
void render_shadow( const CHR_REF by_reference character )
{
    /// @details ZZ@> This function draws a NIFTY shadow
    GLvertex v[4];

    TX_REF  itex;
    int     itex_style;
    float   x, y;
    float   level;
    float   height, size_umbra, size_penumbra;
    float   alpha, alpha_umbra, alpha_penumbra;
    chr_t * pchr;

	if ( IS_ATTACHED_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    // if the character is hidden, not drawn at all, so no shadow
    if ( pchr->is_hidden || pchr->shadow_size == 0 ) return;

    // no shadow if off the mesh
    if ( !mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) ) return;

    // no shadow if invalid tile image
    if ( TILE_IS_FANOFF( PMesh->tmem.tile_list[pchr->onwhichgrid] ) ) return;

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
    oglx_texture_Bind( TxTexture_get_ptr( itex ) );

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
void render_bad_shadow( const CHR_REF by_reference character )
{
    /// @details ZZ@> This function draws a sprite shadow

    GLvertex v[4];

    TX_REF  itex;
    int     itex_style;
    float   size, x, y;
    float   level, height, height_factor, alpha;
    chr_t * pchr;

    if ( IS_ATTACHED_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    // if the character is hidden, not drawn at all, so no shadow
    if ( pchr->is_hidden || pchr->shadow_size == 0 ) return;

    // no shadow if off the mesh
    if ( !mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) ) return;

    // no shadow if invalid tile image
    if ( TILE_IS_FANOFF( PMesh->tmem.tile_list[pchr->onwhichgrid] ) ) return;

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
    oglx_texture_Bind( TxTexture_get_ptr( itex ) );

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
void update_all_chr_instance()
{
    CHR_REF cnt;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        chr_t * pchr;

        if ( !INGAME_CHR( cnt ) ) continue;
        pchr = ChrList.lst + cnt;

        if ( !mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) ) continue;

        if ( rv_success == chr_update_instance( pchr ) )
        {
            // the instance has changed, refresh the collision bound
            chr_update_collision_size( pchr, btrue );
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t render_fans_by_list( ego_mpd_t * pmesh, Uint32 list[], size_t list_size )
{
    Uint32 cnt;
    TX_REF tx;

    if ( NULL == pmesh || NULL == list || 0 == list_size ) return bfalse;

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
            oglx_texture_Bind( TxTexture_get_ptr( tx ) );

            for ( cnt = 0; cnt < list_size; cnt++ )
            {
                render_fan( pmesh, list[cnt] );
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void render_scene_init( ego_mpd_t * pmesh, camera_t * pcam )
{

    PROFILE_BEGIN( renderlist_make );
    {
        // Which tiles can be displayed
        renderlist_make( pmesh, pcam );
    }
    PROFILE_END( renderlist_make );

    PROFILE_BEGIN( dolist_make );
    {
        // determine which objects are visible
        dolist_make( renderlist.pmesh );
    }
    PROFILE_END( dolist_make );

    // put off sorting the dolist until later
    // because it has to be sorted differently for reflected and non-reflected objects
    // dolist_sort( pcam, bfalse );

    PROFILE_BEGIN( do_grid_lighting );
    {
        // figure out the terrain lighting
        do_grid_lighting( renderlist.pmesh, pcam );
    }
    PROFILE_END( do_grid_lighting );

    PROFILE_BEGIN( light_fans );
    {
        // apply the lighting to the characters and particles
        light_fans( &renderlist );
    }
    PROFILE_END( light_fans );

    PROFILE_BEGIN( update_all_chr_instance );
    {
        // make sure the characters are ready to draw
        update_all_chr_instance();
    }
    PROFILE_END( update_all_chr_instance );

    PROFILE_BEGIN( prt_instance_update_all );
    {
        // make sure the particles are ready to draw
        prt_instance_update_all( pcam );
    }
    PROFILE_END( prt_instance_update_all );

    time_render_scene_init_renderlist_make         = PROFILE_QUERY( renderlist_make ) * TARGET_FPS;
    time_render_scene_init_dolist_make             = PROFILE_QUERY( dolist_make ) * TARGET_FPS;
    time_render_scene_init_do_grid_dynalight       = PROFILE_QUERY( do_grid_lighting ) * TARGET_FPS;
    time_render_scene_init_light_fans              = PROFILE_QUERY( light_fans ) * TARGET_FPS;
    time_render_scene_init_update_all_chr_instance = PROFILE_QUERY( update_all_chr_instance ) * TARGET_FPS;
    time_render_scene_init_update_all_prt_instance = PROFILE_QUERY( prt_instance_update_all ) * TARGET_FPS;
}

//--------------------------------------------------------------------------------------------
void render_scene_mesh( renderlist_t * prlist )
{
    /// @details BB@> draw the mesh and any reflected objects

    int cnt;
    ego_mpd_t * pmesh;

    if ( NULL == prlist ) return;

    if ( NULL == prlist->pmesh ) return;
    pmesh = prlist->pmesh;

    // advance the animation of all animated tiles
    animate_all_tiles( prlist->pmesh );

    PROFILE_BEGIN( render_scene_mesh_ndr );
    {
        //---------------------------------------------
        // draw all tiles that do not reflect characters
        ATTRIB_PUSH( "render_scene_mesh() - ndr", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
        {
            GL_DEBUG( glDepthMask )( GL_TRUE );         // GL_DEPTH_BUFFER_BIT - store the surface depth

            GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT - do not draw hidden surfaces
            GL_DEBUG( glDepthFunc )( GL_LEQUAL );       // GL_DEPTH_BUFFER_BIT

            GL_DEBUG( glDisable )( GL_BLEND );          // GL_ENABLE_BIT - no transparency
            GL_DEBUG( glDisable )( GL_CULL_FACE );      // GL_ENABLE_BIT - draw front and back of tiles

            GL_DEBUG( glEnable )( GL_ALPHA_TEST );      // GL_ENABLE_BIT - use alpha test to allow the thatched roof tiles to look like thatch
            GL_DEBUG( glAlphaFunc )( GL_GREATER, 0 );   // GL_COLOR_BUFFER_BIT - speed-up drawing of surfaces with alpha == 0 sections

            // reduce texture hashing by loading up each texture only once
            render_fans_by_list( pmesh, prlist->ndr, prlist->ndr_count );
        }
        ATTRIB_POP( "render_scene_mesh() - ndr" );
    }
    PROFILE_END( render_scene_mesh_ndr );

    //--------------------------------
    // draw the reflective tiles and any reflected objects
    if ( gfx.refon )
    {
        PROFILE_BEGIN( render_scene_mesh_drf_back );
        {
            //------------------------------
            // draw the reflective tiles, but turn off the depth buffer
            // this blanks out any background that might've been drawn

            ATTRIB_PUSH( "render_scene_mesh() - drf - back", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
            {
                GL_DEBUG( glDepthMask )( GL_FALSE );        // GL_DEPTH_BUFFER_BIT - DO NOT store the surface depth

                GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT - do not draw hidden surfaces
                GL_DEBUG( glDepthFunc )( GL_LEQUAL );       // GL_DEPTH_BUFFER_BIT

                // black out any backgound, but allow the background to show through any holes in the floor
                GL_DEBUG( glEnable )( GL_BLEND );                   // GL_ENABLE_BIT - allow transparent surfaces
                GL_DEBUG( glBlendFunc )( GL_ZERO, GL_ONE_MINUS_SRC_ALPHA );    // GL_COLOR_BUFFER_BIT - use the alpha channel to modulate the transparency

                GL_DEBUG( glEnable )( GL_ALPHA_TEST );      // GL_ENABLE_BIT - use alpha test to allow the thatched roof tiles to look like thatch
                GL_DEBUG( glAlphaFunc )( GL_GREATER, 0 );   // GL_COLOR_BUFFER_BIT - speed-up drawing of surfaces with alpha == 0 sections

                // reduce texture hashing by loading up each texture only once
                render_fans_by_list( pmesh, prlist->drf, prlist->drf_count );
            }
            ATTRIB_POP( "render_scene_mesh() - drf - back" );
        }
        PROFILE_END( render_scene_mesh_drf_back );

        PROFILE_BEGIN( render_scene_mesh_ref );
        {
            //------------------------------
            // Render all reflected objects
            ATTRIB_PUSH( "render_scene_mesh() - reflected characters",  GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT );
            {
                GL_DEBUG( glDepthMask )( GL_FALSE );      // GL_DEPTH_BUFFER_BIT - turn off the depth mask by default. Can cause glitches if used improperly.

                GL_DEBUG( glEnable )( GL_DEPTH_TEST );    // GL_ENABLE_BIT - do not draw hidden surfaces
                GL_DEBUG( glDepthFunc )( GL_LEQUAL );     // GL_DEPTH_BUFFER_BIT - surfaces must be closer to the camera to be drawn

                for ( cnt = (( int )dolist_count ) - 1; cnt >= 0; cnt-- )
                {
                    if ( TOTAL_MAX_PRT == dolist[cnt].iprt && INGAME_CHR( dolist[cnt].ichr ) )
                    {
                        CHR_REF ichr;
                        Uint32 itile;

                        GL_DEBUG( glEnable )( GL_CULL_FACE );   // GL_ENABLE_BIT - cull backward facing faces
                        GL_DEBUG( glFrontFace )( GL_CCW );      // GL_POLYGON_BIT - use couter-clockwise orientation to determine backfaces

                        GL_DEBUG( glEnable )( GL_BLEND );                 // GL_ENABLE_BIT - allow transparent objects
                        GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT - use the alpha channel to modulate the transparency

                        ichr  = dolist[cnt].ichr;
                        itile = ChrList.lst[ichr].onwhichgrid;

                        if ( mesh_grid_is_valid( pmesh, itile ) && ( 0 != mesh_test_fx( pmesh, itile, MPDFX_DRAWREF ) ) )
                        {
                            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );          // GL_CURRENT_BIT
                            render_one_mad_ref( ichr );
                        }
                    }
                    else if ( MAX_CHR == dolist[cnt].ichr && DISPLAY_PRT( dolist[cnt].iprt ) )
                    {
                        Uint32 itile;
                        PRT_REF iprt;
                        iprt = dolist[cnt].iprt;
                        itile = PrtList.lst[iprt].onwhichgrid;

                        GL_DEBUG( glDisable )( GL_CULL_FACE );

                        // render_one_prt_ref() actually sets its own blend function, but just to be safe
                        GL_DEBUG( glEnable )( GL_BLEND );                    // GL_ENABLE_BIT - allow transparent objects
                        GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );     // GL_COLOR_BUFFER_BIT - set the default particle blending

                        if ( mesh_grid_is_valid( pmesh, itile ) && ( 0 != mesh_test_fx( pmesh, itile, MPDFX_DRAWREF ) ) )
                        {
                            render_one_prt_ref( iprt );
                        }
                    }
                }

            }
            ATTRIB_POP( "render_scene_mesh() - reflected characters" )
        }
        PROFILE_END( render_scene_mesh_ref );

        PROFILE_BEGIN( render_scene_mesh_ref_chr );
        {
            //------------------------------
            // Render the shadow floors ( let everything show through )
            // turn on the depth mask, so that no objects under the floor will show through
            // this assumes that the floor is not partially transparent...
            ATTRIB_PUSH( "render_scene_mesh() - drf - front", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
            {
                GL_DEBUG( glDepthMask )( GL_TRUE );                   // GL_DEPTH_BUFFER_BIT - set the depth of these tiles

                GL_DEBUG( glEnable )( GL_BLEND );                     // GL_ENABLE_BIT
                GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE );      // GL_COLOR_BUFFER_BIT

                GL_DEBUG( glDisable )( GL_CULL_FACE );                // GL_ENABLE_BIT - draw all faces

                GL_DEBUG( glEnable )( GL_DEPTH_TEST );                // GL_ENABLE_BIT - do not draw hidden surfaces
                GL_DEBUG( glDepthFunc )( GL_LEQUAL );                 // GL_DEPTH_BUFFER_BIT

                // reduce texture hashing by loading up each texture only once
                render_fans_by_list( pmesh, prlist->drf, prlist->drf_count );
            }
            ATTRIB_POP( "render_scene_mesh() - drf - front" )
        }
        PROFILE_END( render_scene_mesh_ref_chr );
    }
    else
    {
        PROFILE_BEGIN( render_scene_mesh_drf_solid );
        {
            //------------------------------
            // Render the shadow floors as normal solid floors

            ATTRIB_PUSH( "render_scene_mesh() - drf - solid", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
            {
                GL_DEBUG( glDisable )( GL_BLEND );          // GL_ENABLE_BIT - no transparency
                GL_DEBUG( glDisable )( GL_CULL_FACE );      // GL_ENABLE_BIT - draw front and back of tiles

                GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT - do not draw hidden surfaces
                GL_DEBUG( glDepthMask )( GL_TRUE );         // GL_DEPTH_BUFFER_BIT - store the surface depth

                GL_DEBUG( glEnable )( GL_ALPHA_TEST );      // GL_ENABLE_BIT - use alpha test to allow the thatched roof tiles to look like thatch
                GL_DEBUG( glAlphaFunc )( GL_GREATER, 0 );   // GL_COLOR_BUFFER_BIT - speed-up drawing of surfaces with alpha == 0 sections

                // reduce texture hashing by loading up each texture only once
                render_fans_by_list( pmesh, prlist->drf, prlist->drf_count );
            }
            ATTRIB_POP( "render_scene_mesh() - drf - solid" );
        }
        PROFILE_END( render_scene_mesh_drf_solid );
    }

#if defined(RENDER_HMAP) && defined(_DEBUG)
    //------------------------------
    // render the heighmap
    for ( cnt = 0; cnt < prlist->all_count; cnt++ )
    {
        render_hmap_fan( pmesh, prlist->all[cnt] );
    }
#endif

    PROFILE_BEGIN( render_scene_mesh_render_shadows );
    {
        //------------------------------
        // Render the shadows
        if ( gfx.shaon )
        {
            GL_DEBUG( glDepthMask )( GL_FALSE );
            GL_DEBUG( glEnable )( GL_DEPTH_TEST );

            GL_DEBUG( glEnable )( GL_BLEND );
            GL_DEBUG( glBlendFunc )( GL_ZERO, GL_ONE_MINUS_SRC_COLOR );

            if ( gfx.shasprite )
            {
                // Bad shadows
                for ( cnt = 0; cnt < dolist_count; cnt++ )
                {
                    CHR_REF ichr = dolist[cnt].ichr;
                    if ( 0 == ChrList.lst[ichr].shadow_size ) continue;

                    render_bad_shadow( ichr );
                }
            }
            else
            {
                // Good shadows for me
                for ( cnt = 0; cnt < dolist_count; cnt++ )
                {
                    CHR_REF ichr = dolist[cnt].ichr;
                    if ( 0 == ChrList.lst[ichr].shadow_size ) continue;

                    render_shadow( ichr );
                }
            }
        }
    }
    PROFILE_END( render_scene_mesh_render_shadows );
}

//--------------------------------------------------------------------------------------------
void render_scene_solid()
{
    Uint32 cnt;

    //------------------------------
    // Render all solid objects
    for ( cnt = 0; cnt < dolist_count; cnt++ )
    {
        GL_DEBUG( glDepthMask )( GL_TRUE );

        GL_DEBUG( glEnable )( GL_DEPTH_TEST );
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );

        GL_DEBUG( glEnable )( GL_ALPHA_TEST );
        GL_DEBUG( glAlphaFunc )( GL_GREATER, 0 );

        GL_DEBUG( glDisable )( GL_BLEND );

        GL_DEBUG( glDisable )( GL_CULL_FACE );

        if ( TOTAL_MAX_PRT == dolist[cnt].iprt )
        {
            GLXvector4f tint;
            chr_instance_t * pinst = chr_get_pinstance( dolist[cnt].ichr );

            if ( NULL != pinst && pinst->alpha == 255 && pinst->light == 255 )
            {
                chr_instance_get_tint( pinst, tint, CHR_SOLID );

                render_one_mad( dolist[cnt].ichr, tint, CHR_SOLID );
            }
        }
        else if ( MAX_CHR == dolist[cnt].ichr && DISPLAY_PRT( dolist[cnt].iprt ) )
        {
            GL_DEBUG( glDisable )( GL_CULL_FACE );

            render_one_prt_solid( dolist[cnt].iprt );
        }
    }

    // Render the solid billboards
    render_all_billboards( PCamera );

    // daw some debugging lines
    draw_all_lines( PCamera );
}

//--------------------------------------------------------------------------------------------
void render_scene_trans()
{
    /// @details BB@> draw transparent objects

    int cnt;
    GLXvector4f tint;

    // set the the transparency parameters
    GL_DEBUG( glDepthMask )( GL_FALSE );                   // GL_DEPTH_BUFFER_BIT

    GL_DEBUG( glEnable )( GL_DEPTH_TEST );                // GL_ENABLE_BIT
    GL_DEBUG( glDepthFunc )( GL_LEQUAL );                 // GL_DEPTH_BUFFER_BIT

    // Now render all transparent and light objects
    for ( cnt = (( int )dolist_count ) - 1; cnt >= 0; cnt-- )
    {
        if ( TOTAL_MAX_PRT == dolist[cnt].iprt && INGAME_CHR( dolist[cnt].ichr ) )
        {
            CHR_REF  ichr = dolist[cnt].ichr;
            chr_t * pchr = ChrList.lst + ichr;
            chr_instance_t * pinst = &( pchr->inst );

            GL_DEBUG( glEnable )( GL_CULL_FACE );         // GL_ENABLE_BIT
            GL_DEBUG( glFrontFace )( GL_CW );             // GL_POLYGON_BIT

            if ( pinst->alpha != 255 && pinst->light == 255 )
            {
                GL_DEBUG( glEnable )( GL_BLEND );                                     // GL_ENABLE_BIT
                GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );      // GL_COLOR_BUFFER_BIT

                chr_instance_get_tint( pinst, tint, CHR_ALPHA );

                render_one_mad( ichr, tint, CHR_ALPHA );
            }

            if ( pinst->light != 255 )
            {
                GL_DEBUG( glEnable )( GL_BLEND );           // GL_ENABLE_BIT
                GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE );  // GL_COLOR_BUFFER_BIT

                chr_instance_get_tint( pinst, tint, CHR_LIGHT );

                render_one_mad( ichr, tint, CHR_LIGHT );
            }

            if ( gfx.phongon && pinst->sheen > 0 )
            {
                GL_DEBUG( glEnable )( GL_BLEND );             // GL_ENABLE_BIT
                GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE );    // GL_COLOR_BUFFER_BIT

                chr_instance_get_tint( pinst, tint, CHR_PHONG );

                render_one_mad( ichr, tint, CHR_PHONG );
            }
        }
        else if ( MAX_CHR == dolist[cnt].ichr && DISPLAY_PRT( dolist[cnt].iprt ) )
        {
            render_one_prt_trans( dolist[cnt].iprt );
        }
    }
}

//--------------------------------------------------------------------------------------------
void render_scene( ego_mpd_t * pmesh, camera_t * pcam )
{
    /// @details ZZ@> This function draws 3D objects

    if ( NULL == pcam ) pcam = PCamera;
    if ( NULL == pmesh ) pmesh = PMesh;

    PROFILE_BEGIN( render_scene_init );
    {
        render_scene_init( pmesh, pcam );
    }
    PROFILE_END( render_scene_init );

    PROFILE_BEGIN( render_scene_mesh );
    {
        PROFILE_BEGIN( render_scene_mesh_dolist_sort );
        {
            // sort the dolist for reflected objects
            // reflected characters and objects are drawn in this pass
            dolist_sort( pcam, btrue );
        }
        PROFILE_END( render_scene_mesh_dolist_sort );

        // do the render pass for the mesh
        render_scene_mesh( &renderlist );

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
        dolist_sort( pcam, bfalse );

        // do the render pass for solid objects
        render_scene_solid();
    }
    PROFILE_END( render_scene_solid );

    PROFILE_BEGIN( render_scene_water );
    {
        // draw the water
        render_water( &renderlist );
    }
    PROFILE_END( render_scene_water );

    PROFILE_BEGIN( render_scene_trans );
    {
        // do the render pass for transparent objects
        render_scene_trans();
    }
    PROFILE_END( render_scene_trans );

#if defined(_DEBUG)
    //render_all_prt_attachment();
#endif

#if defined(_DEBUG) && defined(DEBUG_PRT_BBOX)
    render_all_prt_bbox();
#endif

    time_render_scene_init  = PROFILE_QUERY( render_scene_init ) * TARGET_FPS;
    time_render_scene_mesh  = PROFILE_QUERY( render_scene_mesh ) * TARGET_FPS;
    time_render_scene_solid = PROFILE_QUERY( render_scene_solid ) * TARGET_FPS;
    time_render_scene_water = PROFILE_QUERY( render_scene_water ) * TARGET_FPS;
    time_render_scene_trans = PROFILE_QUERY( render_scene_trans ) * TARGET_FPS;

    time_draw_scene = time_render_scene_init + time_render_scene_mesh + time_render_scene_solid + time_render_scene_water + time_render_scene_trans;
}

//--------------------------------------------------------------------------------------------
void render_world_background( const TX_REF by_reference texture )
{
    /// @details ZZ@> This function draws the large background
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

    pinfo = &( PMesh->info );
    pgmem = &( PMesh->gmem );

    // which layer
    ilayer = water.layer + 0;

    // the "official" camera height
    z0 = 1500;

    // clip the waterlayer uv offset
    ilayer->tx.x = ilayer->tx.x - ( float )floor( ilayer->tx.x );
    ilayer->tx.y = ilayer->tx.y - ( float )floor( ilayer->tx.y );

    // determine the constants for the x-coordinate
    xmag = water.backgroundrepeat / 4 / ( 1.0f + z0 * ilayer->dist.x ) / GRID_SIZE;
    Cx_0 = xmag * ( 1.0f +  PCamera->pos.z       * ilayer->dist.x );
    Cx_1 = -xmag * ( 1.0f + ( PCamera->pos.z - z0 ) * ilayer->dist.x );

    // determine the constants for the y-coordinate
    ymag = water.backgroundrepeat / 4 / ( 1.0f + z0 * ilayer->dist.y ) / GRID_SIZE;
    Cy_0 = ymag * ( 1.0f +  PCamera->pos.z       * ilayer->dist.y );
    Cy_1 = -ymag * ( 1.0f + ( PCamera->pos.z - z0 ) * ilayer->dist.y );

    // Figure out the coordinates of its corners
    Qx = -pgmem->edge_x;
    Qy = -pgmem->edge_y;
    vtlist[0].pos[XX] = Qx;
    vtlist[0].pos[YY] = Qy;
    vtlist[0].pos[ZZ] = PCamera->pos.z - z0;
    vtlist[0].tex[SS] = Cx_0 * Qx + Cx_1 * PCamera->pos.x + ilayer->tx.x;
    vtlist[0].tex[TT] = Cy_0 * Qy + Cy_1 * PCamera->pos.y + ilayer->tx.y;

    Qx = 2 * pgmem->edge_x;
    Qy = -pgmem->edge_y;
    vtlist[1].pos[XX] = Qx;
    vtlist[1].pos[YY] = Qy;
    vtlist[1].pos[ZZ] = PCamera->pos.z - z0;
    vtlist[1].tex[SS] = Cx_0 * Qx + Cx_1 * PCamera->pos.x + ilayer->tx.x;
    vtlist[1].tex[TT] = Cy_0 * Qy + Cy_1 * PCamera->pos.y + ilayer->tx.y;

    Qx = 2 * pgmem->edge_x;
    Qy = 2 * pgmem->edge_y;
    vtlist[2].pos[XX] = Qx;
    vtlist[2].pos[YY] = Qy;
    vtlist[2].pos[ZZ] = PCamera->pos.z - z0;
    vtlist[2].tex[SS] = Cx_0 * Qx + Cx_1 * PCamera->pos.x + ilayer->tx.x;
    vtlist[2].tex[TT] = Cy_0 * Qy + Cy_1 * PCamera->pos.y + ilayer->tx.y;

    Qx = -pgmem->edge_x;
    Qy = 2 * pgmem->edge_y;
    vtlist[3].pos[XX] = Qx;
    vtlist[3].pos[YY] = Qy;
    vtlist[3].pos[ZZ] = PCamera->pos.z - z0;
    vtlist[3].tex[SS] = Cx_0 * Qx + Cx_1 * PCamera->pos.x + ilayer->tx.x;
    vtlist[3].tex[TT] = Cy_0 * Qy + Cy_1 * PCamera->pos.y + ilayer->tx.y;

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

    ptex = TxTexture_get_ptr( texture );

    oglx_texture_Bind( ptex );

    ATTRIB_PUSH( "render_world_background()", GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT );
    {
        GL_DEBUG( glShadeModel )( GL_FLAT );      // GL_LIGHTING_BIT - Flat shade this
        GL_DEBUG( glDepthMask )( GL_FALSE );      // GL_DEPTH_BUFFER_BIT
        GL_DEBUG( glDepthFunc )( GL_ALWAYS );     // GL_DEPTH_BUFFER_BIT
        GL_DEBUG( glDisable )( GL_CULL_FACE );    // GL_ENABLE_BIT

        if ( alpha > 0.0f )
        {
            ATTRIB_PUSH( "render_world_background() - alpha", GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT );
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
            ATTRIB_POP( "render_world_background() - alpha" );
        }

        if ( light > 0.0f )
        {
            ATTRIB_PUSH( "render_world_background() - glow", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT );
            {
                GL_DEBUG( glDisable )( GL_BLEND );                           // GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT
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
            ATTRIB_POP( "render_world_background() - glow" );
        }
    }
    ATTRIB_POP( "render_world_background()" );
}

//--------------------------------------------------------------------------------------------
void render_world_overlay( const TX_REF by_reference texture )
{
    /// @details ZZ@> This function draws the large foreground

    float alpha, ftmp;
    fvec3_t   vforw_wind, vforw_cam;

    oglx_texture_t           * ptex;

    water_instance_layer_t * ilayer = water.layer + 1;

    vforw_wind.x = ilayer->tx_add.x;
    vforw_wind.y = ilayer->tx_add.y;
    vforw_wind.z = 0;
    vforw_wind = fvec3_normalize( vforw_wind.v );

    vforw_cam = mat_getCamForward( PCamera->mView );
    vforw_cam = fvec3_normalize( vforw_cam.v );

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
        sinsize = turntosin[( 3*2047 ) & TRIG_TABLE_MASK] * size;
        cossize = turntocos[( 3*2047 ) & TRIG_TABLE_MASK] * size;
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

        ptex = TxTexture_get_ptr( texture );

        ATTRIB_PUSH( "render_world_overlay()", GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT );
        {
            GL_DEBUG( glHint )( GL_POLYGON_SMOOTH_HINT, GL_NICEST );          // GL_HINT_BIT - make sure that the texture is as smooth as possible

            GL_DEBUG( glShadeModel )( GL_FLAT );    // GL_LIGHTING_BIT - Flat shade this

            GL_DEBUG( glDepthMask )( GL_FALSE );    // GL_DEPTH_BUFFER_BIT
            GL_DEBUG( glDepthFunc )( GL_ALWAYS );   // GL_DEPTH_BUFFER_BIT

            GL_DEBUG( glDisable )( GL_CULL_FACE );  // GL_ENABLE_BIT

            GL_DEBUG( glEnable )( GL_ALPHA_TEST );  // GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT
            GL_DEBUG( glAlphaFunc )( GL_GREATER, 0 ); // GL_COLOR_BUFFER_BIT

            GL_DEBUG( glEnable )( GL_BLEND );                               // GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );  // GL_COLOR_BUFFER_BIT - make the texture a filter

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
        ATTRIB_POP( "render_world_overlay()" );
    }
}

//--------------------------------------------------------------------------------------------
void render_world( camera_t * pcam )
{
    gfx_begin_3d( pcam );
    {
        if ( gfx.draw_background )
        {
            // Render the background
            render_world_background(( TX_REF )TX_WATER_LOW ); // TX_WATER_LOW for waterlow.bmp
        }

        render_scene( PMesh, pcam );

        if ( gfx.draw_overlay )
        {
            // Foreground overlay
            render_world_overlay(( TX_REF )TX_WATER_TOP ); // TX_WATER_TOP is watertop.bmp
        }

		if ( pcam->motion_blur > 0 )
		{
			//Do motion blur
			GL_DEBUG( glAccum )( GL_MULT, pcam->motion_blur );
			GL_DEBUG( glAccum )( GL_ACCUM, 1.00f - pcam->motion_blur );
			GL_DEBUG( glAccum )( GL_RETURN, 1.0 );
		}
    }
    gfx_end_3d();
}

//--------------------------------------------------------------------------------------------
void gfx_main()
{
    /// @details ZZ@> This function does all the drawing stuff

    render_world( PCamera );
    draw_text();

    request_flip_pages();
}

//--------------------------------------------------------------------------------------------
// UTILITY FUNCTIONS
//--------------------------------------------------------------------------------------------
bool_t dump_screenshot()
{
    /// @details BB@> dumps the current screen (GL context) to a new bitmap file
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
    strncpy( szResolvedFilename, vfs_resolveWriteFilename( szFilename ), sizeof( szFilename ) );

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
    /// @details ZZ@> This function empties the message buffer
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
    /// @details ZZ@> This function helps make_lighttable
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
    /// @details ZZ@> This function helps make_lighttable
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
void make_enviro( void )
{
    /// @details ZZ@> This function sets up the environment mapping table
    int cnt;
    float x, y, z;

    // Find the environment map positions
    for ( cnt = 0; cnt < EGO_NORMAL_COUNT; cnt++ )
    {
        x = kMd2Normals[cnt][0];
        y = kMd2Normals[cnt][1];
        indextoenvirox[cnt] = ATAN2( y, x ) / TWO_PI;
    }

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        z = cnt / 255.0f;  // Z is between 0 and 1
        lighttoenviroy[cnt] = z;
    }
}

//--------------------------------------------------------------------------------------------
float grid_lighting_test( ego_mpd_t * pmesh, GLXvector3f pos, float * low_diff, float * hgh_diff )
{
    int ix, iy, cnt;
    Uint32 fan[4];
    float u, v;

    ego_grid_info_t  * glist;
    lighting_cache_t * cache_list[4];

    if ( NULL == pmesh ) return bfalse;
    glist = pmesh->gmem.grid_list;

    ix = floor( pos[XX] / GRID_SIZE );
    iy = floor( pos[YY] / GRID_SIZE );

    fan[0] = mesh_get_tile_int( pmesh, ix,     iy );
    fan[1] = mesh_get_tile_int( pmesh, ix + 1, iy );
    fan[2] = mesh_get_tile_int( pmesh, ix,     iy + 1 );
    fan[3] = mesh_get_tile_int( pmesh, ix + 1, iy + 1 );

    for ( cnt = 0; cnt < 4; cnt++ )
    {
        cache_list[cnt] = NULL;
        if ( mesh_grid_is_valid( pmesh, fan[cnt] ) )
        {
            cache_list[cnt] = &( glist[fan[cnt]].cache );
        }
    }

    u = pos[XX] / GRID_SIZE - ix;
    v = pos[YY] / GRID_SIZE - iy;

    return lighting_cache_test( cache_list, u, v, low_diff, hgh_diff );
}

//--------------------------------------------------------------------------------------------
bool_t grid_lighting_interpolate( ego_mpd_t * pmesh, lighting_cache_t * dst, float fx, float fy )
{
    int ix, iy, cnt;
    Uint32 fan[4];
    float u, v;

    ego_grid_info_t  * glist;
    lighting_cache_t * cache_list[4];

    if ( NULL == pmesh ) return bfalse;
    glist = pmesh->gmem.grid_list;

    ix = floor( fx / GRID_SIZE );
    iy = floor( fy / GRID_SIZE );

    fan[0] = mesh_get_tile_int( pmesh, ix,     iy );
    fan[1] = mesh_get_tile_int( pmesh, ix + 1, iy );
    fan[2] = mesh_get_tile_int( pmesh, ix,     iy + 1 );
    fan[3] = mesh_get_tile_int( pmesh, ix + 1, iy + 1 );

    for ( cnt = 0; cnt < 4; cnt++ )
    {
        cache_list[cnt] = NULL;
        if ( mesh_grid_is_valid( pmesh, fan[cnt] ) )
        {
            cache_list[cnt] = &( glist[fan[cnt]].cache );
        }
    }

    u = fx / GRID_SIZE - ix;
    v = fy / GRID_SIZE - iy;

    return lighting_cache_interpolate( dst, cache_list, u, v );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void gfx_update_timers()
{
    /// @details ZZ@> This function updates the graphics timers

    const float fold = 0.77f;
    const float fnew = 1.0f - fold;

    static int gfx_clock_last = 0;
    static int gfx_clock      = 0;
    static int gfx_clock_stt  = -1;

    int dclock;

    if ( gfx_clock_stt < 0 )
    {
        gfx_clock_stt  = egoboo_get_ticks();
        gfx_clock_last = gfx_clock_stt;
        gfx_clock      = gfx_clock_stt;
    }

    gfx_clock_last = gfx_clock;
    gfx_clock      = egoboo_get_ticks() - gfx_clock_stt;
    dclock         = gfx_clock - gfx_clock_last;

    // if there has been a gap in time (the module was loading, for instance)
    // make sure we do not count that gap
    if ( dclock > TICKS_PER_SEC / 5 )
    {
        return;
    }
    fps_clock += dclock;

    if ( fps_loops > 0 && fps_clock > 0 )
    {
        stabilized_fps_sum    = fold * stabilized_fps_sum    + fnew * ( float ) fps_loops / (( float ) fps_clock / TICKS_PER_SEC );
        stabilized_fps_weight = fold * stabilized_fps_weight + fnew;

        // blank these every so often so that the numbers don't overflow
        if ( fps_loops > 10 * TARGET_FPS )
        {
            fps_loops = 0;
            fps_clock = 0;
        }
    };

    if ( stabilized_fps_weight > 0.5f )
    {
        stabilized_fps = stabilized_fps_sum / stabilized_fps_weight;
    }
}

//--------------------------------------------------------------------------------------------
// BILLBOARD DATA IMPLEMENTATION
//--------------------------------------------------------------------------------------------
billboard_data_t * billboard_data_init( billboard_data_t * pbb )
{
    if ( NULL == pbb ) return pbb;

    memset( pbb, 0, sizeof( *pbb ) );

    pbb->tex_ref = INVALID_TX_TEXTURE;
    pbb->ichr    = ( CHR_REF )MAX_CHR;

    pbb->tint[RR] = pbb->tint[GG] = pbb->tint[BB] = pbb->tint[AA] = 1.0f;
    pbb->tint_add[AA] -= 1.0f / 100.0f;

    pbb->size = 1.0f;
    pbb->size_add -= 1.0f / 200.0f;

    pbb->offset_add[ZZ] += 127 / 50.0f * 2.0f;

    return pbb;
}

//--------------------------------------------------------------------------------------------
bool_t billboard_data_free( billboard_data_t * pbb )
{
    if ( NULL == pbb || !pbb->valid ) return bfalse;

    // free any allocated texture
    TxTexture_free_one( pbb->tex_ref );

    billboard_data_init( pbb );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t billboard_data_update( billboard_data_t * pbb )
{
    fvec3_t     vup, pos_new;
    chr_t     * pchr;
    float       height, offset;

    if ( NULL == pbb || !pbb->valid ) return bfalse;

    if ( !INGAME_CHR( pbb->ichr ) ) return bfalse;
    pchr = ChrList.lst + pbb->ichr;

    // determine where the new position should be
    chr_getMatUp( pchr, &vup );

    height = pchr->bump.height;
    offset = MIN( pchr->bump.height * 0.5f, pchr->bump.size );

    pos_new.x = pchr->pos.x + vup.x * ( height + offset );
    pos_new.y = pchr->pos.y + vup.y * ( height + offset );
    pos_new.z = pchr->pos.z + vup.z * ( height + offset );

    // allow the billboards to be a bit bouncy
    pbb->pos.x = pbb->pos.x * 0.5f + pos_new.x * 0.5f;
    pbb->pos.y = pbb->pos.y * 0.5f + pos_new.y * 0.5f;
    pbb->pos.z = pbb->pos.z * 0.5f + pos_new.z * 0.5f;

    pbb->size += pbb->size_add;

    pbb->tint[RR] += pbb->tint_add[RR];
    pbb->tint[GG] += pbb->tint_add[GG];
    pbb->tint[BB] += pbb->tint_add[BB];
    pbb->tint[AA] += pbb->tint_add[AA];

    pbb->offset[XX] += pbb->offset_add[XX];
    pbb->offset[YY] += pbb->offset_add[YY];
    pbb->offset[ZZ] += pbb->offset_add[ZZ];

    // automatically kill a billboard that is no longer useful
    if ( pbb->tint[AA] == 0.0f || pbb->size == 0.0f )
    {
        billboard_data_free( pbb );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t billboard_data_printf_ttf( billboard_data_t * pbb, Font *font, SDL_Color color, const char * format, ... )
{
    va_list args;
    int rv;
    oglx_texture_t * ptex;
    float texCoords[4];

    if ( NULL == pbb || !pbb->valid ) return bfalse;

    // release any existing texture in case there is an error
    ptex = TxTexture_get_ptr( pbb->tex_ref );
    oglx_texture_Release( ptex );

    va_start( args, format );
    rv = fnt_vprintf( font, color, &( ptex->surface ), ptex->base.binding, texCoords, format, args );
    va_end( args );

    ptex->base_valid = bfalse;
    oglx_grab_texture_state( GL_TEXTURE_2D, 0, ptex );

    ptex->alpha = 1.0f;
    ptex->imgW  = ptex->surface->w;
    ptex->imgH  = ptex->surface->h;
    strncpy( ptex->name, "billboard text", SDL_arraysize( ptex->name ) );

    return ( rv >= 0 );
}

//--------------------------------------------------------------------------------------------
// BILLBOARD IMPLEMENTATION
//--------------------------------------------------------------------------------------------
void BillboardList_clear_data()
{
    /// @details BB@> reset the free billboard list.

    int cnt;

    for ( cnt = 0; cnt < BILLBOARD_COUNT; cnt++ )
    {
        BillboardList.free_ref[cnt] = cnt;
    }
    BillboardList.free_count = cnt;
}

//--------------------------------------------------------------------------------------------
void BillboardList_init_all()
{
    BBOARD_REF cnt;

    for ( cnt = 0; cnt < BILLBOARD_COUNT; cnt++ )
    {
        billboard_data_init( BillboardList.lst + cnt );
    }

    BillboardList_clear_data();
}

//--------------------------------------------------------------------------------------------
void BillboardList_update_all()
{
    BBOARD_REF cnt;
    Uint32     ticks;

    ticks = egoboo_get_ticks();

    for ( cnt = 0; cnt < BILLBOARD_COUNT; cnt++ )
    {
        bool_t is_invalid;

        billboard_data_t * pbb = BillboardList.lst + cnt;

        if ( !pbb->valid ) continue;

        is_invalid = bfalse;
        if ( ticks >= pbb->time || NULL == TxTexture_get_ptr( pbb->tex_ref ) )
        {
            is_invalid = btrue;
        }

        if ( !INGAME_CHR( pbb->ichr ) || INGAME_CHR( ChrList.lst[pbb->ichr].attachedto ) )
        {
            is_invalid = btrue;
        }

        if ( is_invalid )
        {
            // the billboard has expired

            // unlink it from the character
            if ( INGAME_CHR( pbb->ichr ) )
            {
                ChrList.lst[pbb->ichr].ibillboard = INVALID_BILLBOARD;
            }

            // deallocate the billboard
            BillboardList_free_one( REF_TO_INT( cnt ) );
        }
        else
        {
            billboard_data_update( BillboardList.lst + cnt );
        }
    }
}

//--------------------------------------------------------------------------------------------
void BillboardList_free_all()
{
    BBOARD_REF cnt;

    for ( cnt = 0; cnt < BILLBOARD_COUNT; cnt++ )
    {
        if ( !BillboardList.lst[cnt].valid ) continue;

        billboard_data_update( BillboardList.lst + cnt );
    }
}

//--------------------------------------------------------------------------------------------
size_t BillboardList_get_free( Uint32 lifetime_secs )
{
    TX_REF             itex = ( TX_REF )INVALID_TX_TEXTURE;
    size_t             ibb  = INVALID_BILLBOARD;
    billboard_data_t * pbb  = NULL;

    if ( BillboardList.free_count <= 0 ) return INVALID_BILLBOARD;

    if ( 0 == lifetime_secs ) return INVALID_BILLBOARD;

    itex = TxTexture_get_free(( TX_REF )INVALID_TX_TEXTURE );
    if ( INVALID_TX_TEXTURE == itex ) return INVALID_BILLBOARD;

    // grab the top index
    BillboardList.free_count--;
    BillboardList.update_guid++;

    ibb = BillboardList.free_ref[BillboardList.free_count];

    if ( VALID_BILLBOARD_RANGE( ibb ) )
    {
        pbb = BillboardList.lst + ( BBOARD_REF )ibb;

        billboard_data_init( pbb );

        pbb->tex_ref = itex;
        pbb->time    = egoboo_get_ticks() + lifetime_secs * TICKS_PER_SEC;
        pbb->valid   = btrue;
    }
    else
    {
        // the billboard allocation returned an ivaild value
        // deallocate the texture
        TxTexture_free_one( itex );

        ibb = INVALID_BILLBOARD;
    }

    return ibb;
}

//--------------------------------------------------------------------------------------------
bool_t BillboardList_free_one( size_t ibb )
{
    billboard_data_t * pbb;

    if ( !VALID_BILLBOARD_RANGE( ibb ) ) return bfalse;
    pbb = BillboardList.lst + ( BBOARD_REF )ibb;

    billboard_data_free( pbb );

#if defined(_DEBUG)
    {
        int cnt;
        // determine whether this texture is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < BillboardList.free_count; cnt++ )
        {
            if ( ibb == BillboardList.free_ref[cnt] ) return bfalse;
        }
    }
#endif

    if ( BillboardList.free_count >= BILLBOARD_COUNT )
        return bfalse;

    // do not put anything below TX_LAST back onto the SDL_free stack
    BillboardList.free_ref[BillboardList.free_count] = ibb;

    BillboardList.free_count++;
    BillboardList.update_guid++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
billboard_data_t * BillboardList_get_ptr( const BBOARD_REF by_reference  ibb )
{
    if ( !VALID_BILLBOARD( ibb ) ) return NULL;

    return BillboardList.lst + ibb;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t render_billboard( camera_t * pcam, billboard_data_t * pbb, float scale )
{
    int i;
    GLvertex vtlist[4];
    float x1, y1;
    float w, h;
    fvec4_t   vector_up, vector_right;

    oglx_texture_t     * ptex;

    if ( NULL == pbb || !pbb->valid ) return bfalse;

    // do not display for objects that are mounted or being held
    if ( INGAME_CHR( pbb->ichr ) && INGAME_CHR( ChrList.lst[pbb->ichr].attachedto ) ) return bfalse;

    ptex = TxTexture_get_ptr( pbb->tex_ref );

    oglx_texture_Bind( ptex );

    w = oglx_texture_GetImageWidth( ptex );
    h = oglx_texture_GetImageHeight( ptex );

    x1 = w  / ( float ) oglx_texture_GetTextureWidth( ptex );
    y1 = h  / ( float ) oglx_texture_GetTextureHeight( ptex );

    vector_right.x =  pcam->mView.CNV( 0, 0 ) * w * scale * pbb->size;
    vector_right.y =  pcam->mView.CNV( 1, 0 ) * w * scale * pbb->size;
    vector_right.z =  pcam->mView.CNV( 2, 0 ) * w * scale * pbb->size;

    vector_up.x    = -pcam->mView.CNV( 0, 1 ) * h * scale * pbb->size;
    vector_up.y    = -pcam->mView.CNV( 1, 1 ) * h * scale * pbb->size;
    vector_up.z    = -pcam->mView.CNV( 2, 1 ) * h * scale * pbb->size;

    // @todo this billboard stuff needs to be implemented as a OpenGL transform

    // bottom left
    vtlist[0].pos[XX] = pbb->pos.x + ( -vector_right.x - 0 * vector_up.x );
    vtlist[0].pos[YY] = pbb->pos.y + ( -vector_right.y - 0 * vector_up.y );
    vtlist[0].pos[ZZ] = pbb->pos.z + ( -vector_right.z - 0 * vector_up.z );
    vtlist[0].tex[SS] = x1;
    vtlist[0].tex[TT] = y1;

    // top left
    vtlist[1].pos[XX] = pbb->pos.x + ( -vector_right.x + 2 * vector_up.x );
    vtlist[1].pos[YY] = pbb->pos.y + ( -vector_right.y + 2 * vector_up.y );
    vtlist[1].pos[ZZ] = pbb->pos.z + ( -vector_right.z + 2 * vector_up.z );
    vtlist[1].tex[SS] = x1;
    vtlist[1].tex[TT] = 0;

    // top right
    vtlist[2].pos[XX] = pbb->pos.x + ( vector_right.x + 2 * vector_up.x );
    vtlist[2].pos[YY] = pbb->pos.y + ( vector_right.y + 2 * vector_up.y );
    vtlist[2].pos[ZZ] = pbb->pos.z + ( vector_right.z + 2 * vector_up.z );
    vtlist[2].tex[SS] = 0;
    vtlist[2].tex[TT] = 0;

    // bottom right
    vtlist[3].pos[XX] = pbb->pos.x + ( vector_right.x - 0 * vector_up.x );
    vtlist[3].pos[YY] = pbb->pos.y + ( vector_right.y - 0 * vector_up.y );
    vtlist[3].pos[ZZ] = pbb->pos.z + ( vector_right.z - 0 * vector_up.z );
    vtlist[3].tex[SS] = 0;
    vtlist[3].tex[TT] = y1;

    ATTRIB_PUSH( "render_billboard", GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );
    {
        GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
        GL_DEBUG( glPushMatrix )();
        GL_DEBUG( glTranslatef )( pbb->offset[XX], pbb->offset[YY], pbb->offset[ZZ] );

        GL_DEBUG( glShadeModel )( GL_FLAT );    // GL_LIGHTING_BIT - Flat shade this

        if ( pbb->tint[AA] < 1.0f )
        {
            GL_DEBUG( glEnable )( GL_BLEND );                             // GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT
            GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );  // GL_COLOR_BUFFER_BIT

            GL_DEBUG( glEnable )( GL_ALPHA_TEST );  // GL_COLOR_BUFFER_BIT GL_ENABLE_BIT
            GL_DEBUG( glAlphaFunc )( GL_GREATER, 0 ); // GL_COLOR_BUFFER_BIT
        }

        // Go on and draw it
        GL_DEBUG( glBegin )( GL_QUADS );
        {
            GL_DEBUG( glColor4fv) ( pbb->tint );

            for ( i = 0; i < 4; i++ )
            {
                GL_DEBUG( glTexCoord2fv )( vtlist[i].tex );
                GL_DEBUG( glVertex3fv )( vtlist[i].pos );
            }
        }
        GL_DEBUG_END();

        GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
        GL_DEBUG( glPopMatrix )();
    }
    ATTRIB_POP( "render_billboard" );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void render_all_billboards( camera_t * pcam )
{
    BBOARD_REF cnt;

    if ( NULL == pcam ) pcam = PCamera;
    if ( NULL == pcam ) return;

    gfx_begin_3d( pcam );
    {
        ATTRIB_PUSH( "render_all_billboards()", GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );
        {
            GL_DEBUG( glShadeModel )( GL_FLAT );    // GL_LIGHTING_BIT - Flat shade this
            GL_DEBUG( glDepthMask )( GL_FALSE );    // GL_DEPTH_BUFFER_BIT
            GL_DEBUG( glDepthFunc )( GL_LEQUAL );   // GL_DEPTH_BUFFER_BIT
            GL_DEBUG( glDisable )( GL_CULL_FACE );  // GL_ENABLE_BIT

            GL_DEBUG( glEnable )( GL_BLEND );                                     // GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );      // GL_COLOR_BUFFER_BIT

            GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f );

            for ( cnt = 0; cnt < BILLBOARD_COUNT; cnt++ )
            {
                billboard_data_t * pbb = BillboardList.lst + cnt;

                if ( !pbb->valid ) continue;

                render_billboard( pcam, pbb, 0.75 );
            }
        }
        ATTRIB_POP( "render_all_billboards()" );
    }
    gfx_end_3d();
}

//--------------------------------------------------------------------------------------------
// LINE IMPLENTATION
//--------------------------------------------------------------------------------------------
int get_free_line()
{
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
void draw_all_lines( camera_t * pcam )
{
    /// @details BB@> draw some lines for debugging purposes

    int cnt, ticks;

    gfx_begin_3d( pcam );
    {
        ATTRIB_PUSH( "render_all_billboards()", GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
        {
            GL_DEBUG( glShadeModel )( GL_FLAT );     // GL_LIGHTING_BIT - Flat shade this
            GL_DEBUG( glDepthMask )( GL_FALSE );     // GL_DEPTH_BUFFER_BIT
            GL_DEBUG( glDepthFunc )( GL_LEQUAL );    // GL_DEPTH_BUFFER_BIT
            GL_DEBUG( glDisable )( GL_CULL_FACE );   // GL_ENABLE_BIT

            GL_DEBUG( glDisable )( GL_BLEND );       // GL_ENABLE_BIT

            GL_DEBUG( glDisable )( GL_TEXTURE_2D );  // GL_ENABLE_BIT - we do not want texture mapped lines

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
                    GL_DEBUG(glVertex3fv) ( line_list[cnt].src.v );
                    GL_DEBUG(glVertex3fv) ( line_list[cnt].dst.v );
                }
                GL_DEBUG_END();
            }
        }
        ATTRIB_POP( "render_all_billboards()" );
    }
    gfx_end_3d();
}

//--------------------------------------------------------------------------------------------
// AXIS BOUNDING BOX IMPLEMENTATION(S)
//--------------------------------------------------------------------------------------------
bool_t render_aabb( aabb_t * pbbox )
{
    GLXvector3f * pmin, * pmax;

    if ( NULL == pbbox ) return bfalse;

    GL_DEBUG( glPushMatrix )();
    {
        pmin = &( pbbox->mins );
        pmax = &( pbbox->maxs );

        // !!!! there must be an optimized way of doing this !!!!

        GL_DEBUG( glBegin )( GL_QUADS );
        {
            // Front Face
            GL_DEBUG(glVertex3f) (( *pmin )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmax )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmax )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmin )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );

            // Back Face
            GL_DEBUG(glVertex3f) (( *pmin )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmin )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmax )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmax )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );

            // Top Face
            GL_DEBUG(glVertex3f) (( *pmin )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmin )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmax )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmax )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );

            // Bottom Face
            GL_DEBUG(glVertex3f) (( *pmin )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmax )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmax )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmin )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );

            // Right face
            GL_DEBUG(glVertex3f) (( *pmax )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmax )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmax )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmax )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );

            // Left Face
            GL_DEBUG(glVertex3f) (( *pmin )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmin )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmin )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG(glVertex3f) (( *pmin )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
        }
        GL_DEBUG_END();
    }
    GL_DEBUG( glPopMatrix )();

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t render_oct_bb( oct_bb_t * bb, bool_t draw_square, bool_t draw_diamond )
{
    bool_t retval = bfalse;

    if ( NULL == bb ) return bfalse;

    ATTRIB_PUSH( "render_oct_bb", GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT );
    {
        // don't write into the depth buffer
        GL_DEBUG( glDepthMask )( GL_FALSE );
        GL_DEBUG( glEnable ) ( GL_DEPTH_TEST );

        // fix the poorly chosen normals...
        GL_DEBUG( glDisable ) ( GL_CULL_FACE );

        // make them transparent
        GL_DEBUG( glEnable ) ( GL_BLEND );
        GL_DEBUG( glBlendFunc ) ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        // choose a "white" texture
        oglx_texture_Bind( NULL );

        //------------------------------------------------
        // DIAGONAL BBOX
        if ( draw_diamond )
        {
            float p1_x, p1_y;
            float p2_x, p2_y;

            GL_DEBUG( glColor4f ) ( 0.5, 1, 1, 0.1 );

            p1_x = 0.5f * ( bb->maxs[OCT_XY] - bb->maxs[OCT_YX] );
            p1_y = 0.5f * ( bb->maxs[OCT_XY] + bb->maxs[OCT_YX] );
            p2_x = 0.5f * ( bb->maxs[OCT_XY] - bb->mins[OCT_YX] );
            p2_y = 0.5f * ( bb->maxs[OCT_XY] + bb->mins[OCT_YX] );

            GL_DEBUG(glBegin) ( GL_QUADS );
            GL_DEBUG(glVertex3f) ( p1_x, p1_y, bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( p2_x, p2_y, bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( p2_x, p2_y, bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( p1_x, p1_y, bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb->maxs[OCT_XY] - bb->mins[OCT_YX] );
            p1_y = 0.5f * ( bb->maxs[OCT_XY] + bb->mins[OCT_YX] );
            p2_x = 0.5f * ( bb->mins[OCT_XY] - bb->mins[OCT_YX] );
            p2_y = 0.5f * ( bb->mins[OCT_XY] + bb->mins[OCT_YX] );

            GL_DEBUG(glBegin) ( GL_QUADS );
            GL_DEBUG(glVertex3f) ( p1_x, p1_y, bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( p2_x, p2_y, bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( p2_x, p2_y, bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( p1_x, p1_y, bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb->mins[OCT_XY] - bb->mins[OCT_YX] );
            p1_y = 0.5f * ( bb->mins[OCT_XY] + bb->mins[OCT_YX] );
            p2_x = 0.5f * ( bb->mins[OCT_XY] - bb->maxs[OCT_YX] );
            p2_y = 0.5f * ( bb->mins[OCT_XY] + bb->maxs[OCT_YX] );

            GL_DEBUG(glBegin) ( GL_QUADS );
            GL_DEBUG(glVertex3f) ( p1_x, p1_y, bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( p2_x, p2_y, bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( p2_x, p2_y, bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( p1_x, p1_y, bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb->mins[OCT_XY] - bb->maxs[OCT_YX] );
            p1_y = 0.5f * ( bb->mins[OCT_XY] + bb->maxs[OCT_YX] );
            p2_x = 0.5f * ( bb->maxs[OCT_XY] - bb->maxs[OCT_YX] );
            p2_y = 0.5f * ( bb->maxs[OCT_XY] + bb->maxs[OCT_YX] );

            GL_DEBUG(glBegin) ( GL_QUADS );
            GL_DEBUG(glVertex3f) ( p1_x, p1_y, bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( p2_x, p2_y, bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( p2_x, p2_y, bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( p1_x, p1_y, bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            retval = btrue;
        }

        //------------------------------------------------
        // SQUARE BBOX
        if ( draw_square )
        {
            GL_DEBUG( glColor4f ) ( 1, 0.5, 1, 0.1 );

            // XZ FACE, min Y
            GL_DEBUG(glBegin) ( GL_QUADS );
            GL_DEBUG(glVertex3f) ( bb->mins[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->mins[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // YZ FACE, min X
            GL_DEBUG(glBegin) ( GL_QUADS );
            GL_DEBUG(glVertex3f) ( bb->mins[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->mins[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // XZ FACE, max Y
            GL_DEBUG(glBegin) ( GL_QUADS );
            GL_DEBUG(glVertex3f) ( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // YZ FACE, max X
            GL_DEBUG(glBegin) ( GL_QUADS );
            GL_DEBUG(glVertex3f) ( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // XY FACE, min Z
            GL_DEBUG(glBegin) ( GL_QUADS );
            GL_DEBUG(glVertex3f) ( bb->mins[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // XY FACE, max Z
            GL_DEBUG(glBegin) ( GL_QUADS );
            GL_DEBUG(glVertex3f) ( bb->mins[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG(glVertex3f) ( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            retval = btrue;
        }

    }
    ATTRIB_POP( "render_oct_bb" );

    return retval;
}

//--------------------------------------------------------------------------------------------
// GRAPHICS OPTIMIZATIONS
//--------------------------------------------------------------------------------------------
bool_t dolist_add_chr( ego_mpd_t * pmesh, const CHR_REF by_reference ichr )
{
    /// ZZ@> This function puts a character in the list
    Uint32 itile;
    chr_t * pchr;
    cap_t * pcap;
    chr_instance_t * pinst;

    if ( dolist_count >= DOLIST_SIZE ) return bfalse;

    if ( !INGAME_CHR( ichr ) ) return bfalse;
    pchr  = ChrList.lst + ichr;
    pinst = &( pchr->inst );

    if ( pinst->indolist ) return btrue;

    pcap = chr_get_pcap( ichr );
    if ( NULL == pcap ) return bfalse;

    itile = pchr->onwhichgrid;
    if ( !mesh_grid_is_valid( pmesh, itile ) ) return bfalse;

    if ( pmesh->tmem.tile_list[itile].inrenderlist )
    {
        dolist[dolist_count].ichr = ichr;
        dolist[dolist_count].iprt = TOTAL_MAX_PRT;
        dolist_count++;

        pinst->indolist = btrue;
    }
    else if ( pcap->alwaysdraw )
    {
        // Double check for large/special objects

        dolist[dolist_count].ichr = ichr;
        dolist[dolist_count].iprt = TOTAL_MAX_PRT;
        dolist_count++;

        pinst->indolist = btrue;
    }

    if ( pinst->indolist )
    {
        // Add its weapons too
        dolist_add_chr( pmesh, pchr->holdingwhich[SLOT_LEFT] );
        dolist_add_chr( pmesh, pchr->holdingwhich[SLOT_RIGHT] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t dolist_add_prt( ego_mpd_t * pmesh, const PRT_REF by_reference iprt )
{
    /// ZZ@> This function puts a character in the list
    prt_t * pprt;
    prt_instance_t * pinst;

    if ( dolist_count >= DOLIST_SIZE ) return bfalse;

    if ( !DISPLAY_PRT( iprt ) ) return bfalse;
    pprt = PrtList.lst + iprt;
    pinst = &( pprt->inst );

    if ( pinst->indolist ) return btrue;

    if ( 0 == pinst->size || pprt->is_hidden || !mesh_grid_is_valid( pmesh, pprt->onwhichgrid ) ) return bfalse;

    dolist[dolist_count].ichr = ( CHR_REF )MAX_CHR;
    dolist[dolist_count].iprt = iprt;
    dolist_count++;

    pinst->indolist = btrue;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void dolist_make( ego_mpd_t * pmesh )
{
    /// @details ZZ@> This function finds the characters that need to be drawn and puts them in the list

    int cnt;
    CHR_REF ichr;

    // Remove everyone from the dolist
    for ( cnt = 0; cnt < dolist_count; cnt++ )
    {
        if ( TOTAL_MAX_PRT == dolist[cnt].iprt && MAX_CHR != dolist[cnt].ichr )
        {
            ChrList.lst[ dolist[cnt].ichr ].inst.indolist = bfalse;
        }
        else if ( MAX_CHR == dolist[cnt].ichr && TOTAL_MAX_PRT != dolist[cnt].iprt )
        {
            PrtList.lst[ dolist[cnt].iprt ].inst.indolist = bfalse;
        }
    }
    dolist_count = 0;

    // Now fill it up again
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        if ( INGAME_CHR( ichr ) && !ChrList.lst[ichr].pack.is_packed )
        {
            // Add the character
            dolist_add_chr( pmesh, ichr );
        }
    }

    PRT_BEGIN_LOOP_DISPLAY( iprt, prt_bdl )
    {
        if ( mesh_grid_is_valid( pmesh, prt_bdl.prt_ptr->onwhichgrid ) )
        {
            // Add the character
            dolist_add_prt( pmesh, prt_bdl.prt_ref );
        }
    }
    PRT_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void dolist_sort( camera_t * pcam, bool_t do_reflect )
{
    /// @details ZZ@> This function orders the dolist based on distance from camera,
    ///    which is needed for reflections to properly clip themselves.
    ///    Order from closest to farthest

    Uint32    cnt;
    fvec3_t   vcam;
    size_t    count;

    vcam = mat_getCamForward( pcam->mView );

    // Figure the distance of each
    count = 0;
    for ( cnt = 0; cnt < dolist_count; cnt++ )
    {
        fvec3_t   vtmp;
        float dist;

        if ( TOTAL_MAX_PRT == dolist[cnt].iprt && INGAME_CHR( dolist[cnt].ichr ) )
        {
            CHR_REF ichr;
            fvec3_t pos_tmp;

            ichr = dolist[cnt].ichr;

            if ( do_reflect )
            {
                pos_tmp = mat_getTranslate( ChrList.lst[ichr].inst.ref.matrix );
            }
            else
            {
                pos_tmp = mat_getTranslate( ChrList.lst[ichr].inst.matrix );
            }

            vtmp = fvec3_sub( pos_tmp.v, pcam->pos.v );
        }
        else if ( MAX_CHR == dolist[cnt].ichr && DISPLAY_PRT( dolist[cnt].iprt ) )
        {
            PRT_REF iprt = dolist[cnt].iprt;

            if ( do_reflect )
            {
                vtmp = fvec3_sub( PrtList.lst[iprt].inst.pos.v, pcam->pos.v );
            }
            else
            {
                vtmp = fvec3_sub( PrtList.lst[iprt].inst.ref_pos.v, pcam->pos.v );
            }
        }
        else
        {
            continue;
        }

        dist = fvec3_dot_product( vtmp.v, vcam.v );
        if ( dist > 0 )
        {
            dolist[count].ichr = dolist[cnt].ichr;
            dolist[count].iprt = dolist[cnt].iprt;
            dolist[count].dist = dist;
            count++;
        }
    }
    dolist_count = count;

    // use qsort to sort the list in-place
    qsort( dolist, dolist_count, sizeof( obj_registry_entity_t ), obj_registry_entity_cmp );
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
void renderlist_reset()
{
    /// @details BB@> Clear old render lists

    if ( NULL != renderlist.pmesh )
    {
        int cnt;

        // clear out the inrenderlist flag for the old mesh
        ego_tile_info_t * tlist = renderlist.pmesh->tmem.tile_list;

        for ( cnt = 0; cnt < renderlist.all_count; cnt++ )
        {
            Uint32 fan = renderlist.all[cnt];
            if ( fan < renderlist.pmesh->info.tiles_count )
            {
                tlist[fan].inrenderlist       = bfalse;
                tlist[fan].inrenderlist_frame = 0;
            }
        }

        renderlist.pmesh = NULL;
    }

    renderlist.all_count = 0;
    renderlist.ref_count = 0;
    renderlist.sha_count = 0;
    renderlist.drf_count = 0;
    renderlist.ndr_count = 0;
    renderlist.wat_count = 0;
}

//--------------------------------------------------------------------------------------------
void renderlist_make( ego_mpd_t * pmesh, camera_t * pcam )
{
    /// @details ZZ@> This function figures out which mesh fans to draw
    int cnt, grid_x, grid_y;
    int row, run, numrow;
    int corner_x[4], corner_y[4];
    int leftnum, leftlist[4];
    int rightnum, rightlist[4];
    int rowstt[128], rowend[128];
    int x, stepx, divx, basex;
    int from, to;

    ego_tile_info_t * tlist;

    if ( 0 != ( frame_all & 7 ) ) return;

    // reset the renderlist
    renderlist_reset();

    // Make sure it doesn't die ugly
    if ( NULL == pcam ) return;

    // Find the render area corners
    project_view( pcam );

    if ( NULL == pmesh ) return;

    renderlist.pmesh = pmesh;
    tlist = pmesh->tmem.tile_list;

    // It works better this way...
    cornery[cornerlistlowtohighy[3]] += 256;

    // Make life simpler
    for ( cnt = 0; cnt < 4; cnt++ )
    {
        corner_x[cnt] = cornerx[cornerlistlowtohighy[cnt]];
        corner_y[cnt] = cornery[cornerlistlowtohighy[cnt]];
    }

    // Find the center line
    divx = corner_y[3] - corner_y[0]; if ( divx < 1 ) return;
    stepx = corner_x[3] - corner_x[0];
    basex = corner_x[0];

    // Find the points in each edge
    leftlist[0] = 0;  leftnum = 1;
    rightlist[0] = 0;  rightnum = 1;
    if ( corner_x[1] < ( stepx*( corner_y[1] - corner_y[0] ) / divx ) + basex )
    {
        leftlist[leftnum] = 1;  leftnum++;
        cornerx[1] -= 512;
    }
    else
    {
        rightlist[rightnum] = 1;  rightnum++;
        cornerx[1] += 512;
    }
    if ( corner_x[2] < ( stepx*( corner_y[2] - corner_y[0] ) / divx ) + basex )
    {
        leftlist[leftnum] = 2;  leftnum++;
        cornerx[2] -= 512;
    }
    else
    {
        rightlist[rightnum] = 2;  rightnum++;
        cornerx[2] += 512;
    }

    leftlist[leftnum] = 3;  leftnum++;
    rightlist[rightnum] = 3;  rightnum++;

    // Make the left edge ( rowstt )
    grid_y = corner_y[0] >> TILE_BITS;
    row = 0;
    cnt = 1;
    while ( cnt < leftnum )
    {
        from = leftlist[cnt-1];  to = leftlist[cnt];
        x = corner_x[from];
        divx = corner_y[to] - corner_y[from];
        stepx = 0;
        if ( divx > 0 )
        {
            stepx = (( corner_x[to] - corner_x[from] ) << TILE_BITS ) / divx;
        }

        x -= 256;
        run = corner_y[to] >> TILE_BITS;
        while ( grid_y < run )
        {
            if ( grid_y >= 0 && grid_y < pmesh->info.tiles_y )
            {
                grid_x = x >> TILE_BITS;
                if ( grid_x < 0 )  grid_x = 0;
                if ( grid_x >= pmesh->info.tiles_x )  grid_x = pmesh->info.tiles_x - 1;

                rowstt[row] = grid_x;
                row++;
            }

            x += stepx;
            grid_y++;
        }

        cnt++;
    }
    numrow = row;

    // Make the right edge ( rowrun )
    grid_y = corner_y[0] >> TILE_BITS;
    row = 0;
    cnt = 1;
    while ( cnt < rightnum )
    {
        from = rightlist[cnt-1];  to = rightlist[cnt];
        x = corner_x[from];
         x+=128;
        divx = corner_y[to] - corner_y[from];
        stepx = 0;
        if ( divx > 0 )
        {
            stepx = (( corner_x[to] - corner_x[from] ) << TILE_BITS ) / divx;
        }

        run = corner_y[to] >> TILE_BITS;

        while ( grid_y < run )
        {
            if ( grid_y >= 0 && grid_y < pmesh->info.tiles_y )
            {
                grid_x = x >> TILE_BITS;
                if ( grid_x < 0 )  grid_x = 0;
                if ( grid_x >= pmesh->info.tiles_x - 1 )  grid_x = pmesh->info.tiles_x - 1;// -2

                rowend[row] = grid_x;
                row++;
            }

            x += stepx;
            grid_y++;
        }

        cnt++;
    }

    if ( numrow != row )
    {
        log_error( "ROW error (%i, %i)\n", numrow, row );
    }

    // fill the renderlist from the projected view
    grid_y = corner_y[0] / TILE_ISIZE;
    grid_y = CLIP( grid_y, 0, pmesh->info.tiles_y - 1 );
    for ( row = 0; row < numrow; row++, grid_y++ )
    {
        for ( grid_x = rowstt[row]; grid_x <= rowend[row] && renderlist.all_count < MAXMESHRENDER; grid_x++ )
        {
            cnt = pmesh->gmem.tilestart[grid_y] + grid_x;

            // Flag the tile as in the renderlist
            tlist[cnt].inrenderlist       = btrue;

            // if the tile was not in the renderlist last frame, then we need to force a lighting update of this tile
            if ( tlist[cnt].inrenderlist_frame < frame_all - 1 )
            {
                tlist[cnt].needs_lighting_update = btrue;
            }

            // make sure to cache the frame number of this update
            tlist[cnt].inrenderlist_frame = frame_all;

            // Put each tile in basic list
            renderlist.all[renderlist.all_count] = cnt;
            renderlist.all_count++;

            // Put each tile in one other list, for shadows and relections
            if ( 0 != mesh_test_fx( pmesh, cnt, MPDFX_SHA ) )
            {
                renderlist.sha[renderlist.sha_count] = cnt;
                renderlist.sha_count++;
            }
            else
            {
                renderlist.ref[renderlist.ref_count] = cnt;
                renderlist.ref_count++;
            }

            if ( 0 != mesh_test_fx( pmesh, cnt, MPDFX_DRAWREF ) )
            {
                renderlist.drf[renderlist.drf_count] = cnt;
                renderlist.drf_count++;
            }
            else
            {
                renderlist.ndr[renderlist.ndr_count] = cnt;
                renderlist.ndr_count++;
            }

            if ( 0 != mesh_test_fx( pmesh, cnt, MPDFX_WATER ) )
            {
                renderlist.wat[renderlist.wat_count] = cnt;
                renderlist.wat_count++;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int DisplayMsg_get_free()
{
    /// ZZ@> This function finds the best message to use
    /// Pick the first one

    int tnc = DisplayMsg.count;

    DisplayMsg.count++;
    DisplayMsg.count %= maxmessage;

    return tnc;
}

//--------------------------------------------------------------------------------------------
// ASSET INITIALIZATION
//--------------------------------------------------------------------------------------------
void init_icon_data()
{
    /// @details ZZ@> This function sets the icon pointers to NULL

    iconrect.left = 0;
    iconrect.right = 32;
    iconrect.top = 0;
    iconrect.bottom = 32;
}

//--------------------------------------------------------------------------------------------
void init_bar_data()
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
void init_blip_data()
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
void init_map_data()
{
    /// @details ZZ@> This function releases all the map images

    // Set up the rectangles
    maprect.left   = 0;
    maprect.right  = MAPSIZE;
    maprect.top    = 0;
    maprect.bottom = MAPSIZE;

    mapvalid = bfalse;
    mapon    = bfalse;
}

//--------------------------------------------------------------------------------------------
void init_all_graphics()
{
    init_icon_data();
    init_bar_data();
    init_blip_data();
    init_map_data();
    font_bmp_init();

    BillboardList_free_all();
    TxTexture_init_all();

    PROFILE_RESET( render_scene_init );
    PROFILE_RESET( render_scene_mesh );
    PROFILE_RESET( render_scene_solid );
    PROFILE_RESET( render_scene_water );
    PROFILE_RESET( render_scene_trans );

    PROFILE_RESET( renderlist_make );
    PROFILE_RESET( dolist_make );
    PROFILE_RESET( do_grid_lighting );
    PROFILE_RESET( light_fans );
    PROFILE_RESET( update_all_chr_instance );
    PROFILE_RESET( prt_instance_update_all );

    PROFILE_RESET( render_scene_mesh_dolist_sort );
    PROFILE_RESET( render_scene_mesh_ndr );
    PROFILE_RESET( render_scene_mesh_drf_back );
    PROFILE_RESET( render_scene_mesh_ref );
    PROFILE_RESET( render_scene_mesh_ref_chr );
    PROFILE_RESET( render_scene_mesh_drf_solid );
    PROFILE_RESET( render_scene_mesh_render_shadows );

    stabilized_fps_sum    = 0.0f;
    stabilized_fps_weight = 0.0f;
}

//--------------------------------------------------------------------------------------------
void release_all_graphics()
{
    init_icon_data();
    init_bar_data();
    init_blip_data();
    init_map_data();

    BillboardList_free_all();
    TxTexture_release_all();
}

//--------------------------------------------------------------------------------------------
void delete_all_graphics()
{
    init_icon_data();
    init_bar_data();
    init_blip_data();
    init_map_data();

    BillboardList_free_all();
    TxTexture_delete_all();
}

//--------------------------------------------------------------------------------------------
bool_t load_all_global_icons()
{
    /// @details ZF@> Load all the global icons used in all modules

    // Setup
    bool_t result = bfalse;

    // Now load every icon
    result = INVALID_TX_TEXTURE != TxTexture_load_one_vfs( "mp_data/nullicon", ( TX_REF )ICON_NULL, INVALID_KEY );
    result = INVALID_TX_TEXTURE != TxTexture_load_one_vfs( "mp_data/keybicon", ( TX_REF )ICON_KEYB, INVALID_KEY );
    result = INVALID_TX_TEXTURE != TxTexture_load_one_vfs( "mp_data/mousicon", ( TX_REF )ICON_MOUS, INVALID_KEY );
    result = INVALID_TX_TEXTURE != TxTexture_load_one_vfs( "mp_data/joyaicon", ( TX_REF )ICON_JOYA, INVALID_KEY );
    result = INVALID_TX_TEXTURE != TxTexture_load_one_vfs( "mp_data/joybicon", ( TX_REF )ICON_JOYB, INVALID_KEY );

    return result;
}

//--------------------------------------------------------------------------------------------
void load_basic_textures( /* const char *modname */ )
{
    /// @details ZZ@> This function loads the standard textures for a module

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

    // Texture 7 is the phong map
    TxTexture_load_one_vfs( "mp_data/phong", ( TX_REF )TX_PHONG, TRANSCOLOR );

    PROFILE_RESET( render_scene_init );
    PROFILE_RESET( render_scene_mesh );
    PROFILE_RESET( render_scene_solid );
    PROFILE_RESET( render_scene_water );
    PROFILE_RESET( render_scene_trans );

    PROFILE_RESET( renderlist_make );
    PROFILE_RESET( dolist_make );
    PROFILE_RESET( do_grid_lighting );
    PROFILE_RESET( light_fans );
    PROFILE_RESET( update_all_chr_instance );
    PROFILE_RESET( prt_instance_update_all );

    PROFILE_RESET( render_scene_mesh_dolist_sort );
    PROFILE_RESET( render_scene_mesh_ndr );
    PROFILE_RESET( render_scene_mesh_drf_back );
    PROFILE_RESET( render_scene_mesh_ref );
    PROFILE_RESET( render_scene_mesh_ref_chr );
    PROFILE_RESET( render_scene_mesh_drf_solid );
    PROFILE_RESET( render_scene_mesh_render_shadows );

    stabilized_fps_sum    = 0.0f;
    stabilized_fps_weight = 0.0f;
}

//--------------------------------------------------------------------------------------------
void load_bars()
{
    /// @details ZZ@> This function loads the status bar bitmap

    const char * pname;

    pname = "mp_data/bars";
    if ( INVALID_TX_TEXTURE == TxTexture_load_one_vfs( pname, ( TX_REF )TX_BARS, TRANSCOLOR ) )
    {
        log_warning( "load_bars() - Cannot load file! (\"%s\")\n", pname );
    }

    pname = "mp_data/xpbar";
    if ( INVALID_TX_TEXTURE == TxTexture_load_one_vfs( pname, ( TX_REF )TX_XP_BAR, TRANSCOLOR ) )
    {
        log_warning( "load_bars() - Cannot load file! (\"%s\")\n", pname );
    }
}

//--------------------------------------------------------------------------------------------
void load_map()
{
    /// @details ZZ@> This function loads the map bitmap

    const char* szMap;

    // Turn it all off
    mapvalid = bfalse;
    mapon = bfalse;
    youarehereon = bfalse;
    blip_count = 0;

    // Load the images
    szMap = "mp_data/plan";
    if ( INVALID_TX_TEXTURE == TxTexture_load_one_vfs( szMap, ( TX_REF )TX_MAP, INVALID_KEY ) )
    {
        log_debug( "load_map() - Cannot load file! (\"%s\")\n", szMap );
    }
    else
    {
        mapvalid = btrue;
    }
}

//--------------------------------------------------------------------------------------------
bool_t load_blips()
{
    /// ZZ@> This function loads the blip bitmaps
    if ( INVALID_TX_TEXTURE == TxTexture_load_one_vfs( "mp_data/blip", ( TX_REF )TX_BLIP, INVALID_KEY ) )
    {
        log_warning( "Blip bitmap not loaded! (\"mp_data/blip\")\n" );
        return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void load_graphics()
{
    /// @details ZF@> This function loads all the graphics based on the game settings

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
        GL_DEBUG( glDisable )( GL_DITHER );
    }

    // Enable gourad shading? (Important!)
    GL_DEBUG( glShadeModel )( gfx.shading );

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
//--------------------------------------------------------------------------------------------
void do_chr_flashing()
{
    Uint32 i;

    for ( i = 0; i < dolist_count; i++ )
    {
        CHR_REF ichr = dolist[i].ichr;

        if ( !INGAME_CHR( ichr ) ) continue;

        // Do flashing
        if ( HAS_NO_BITS( true_frame, ChrList.lst[ichr].flashand ) && ChrList.lst[ichr].flashand != DONTFLASH )
        {
            flash_character( ichr, 255 );
        }

        // Do blacking
        if ( HAS_NO_BITS( true_frame, SEEKURSEAND ) && local_seekurse && ChrList.lst[ichr].iskursed )
        {
            flash_character( ichr, 0 );
        }
    }
}

//--------------------------------------------------------------------------------------------
void flash_character( const CHR_REF by_reference character, Uint8 value )
{
    /// @details ZZ@> This function sets a character's lighting

    chr_instance_t * pinst = chr_get_pinstance( character );

    if ( NULL != pinst )
    {
        pinst->color_amb = value;
    }
}

//--------------------------------------------------------------------------------------------
// MODE CONTROL
//--------------------------------------------------------------------------------------------
void gfx_begin_text()
{
    gfx_enable_texturing();    // Enable texture mapping

    oglx_texture_Bind( TxTexture_get_ptr(( TX_REF )TX_FONT ) );

    GL_DEBUG( glEnable )( GL_ALPHA_TEST );
    GL_DEBUG( glAlphaFunc )( GL_GREATER, 0 );

    GL_DEBUG( glEnable )( GL_BLEND );
    GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    GL_DEBUG( glDisable )( GL_DEPTH_TEST );
    GL_DEBUG( glDisable )( GL_CULL_FACE );

    GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
}

//--------------------------------------------------------------------------------------------
void gfx_end_text()
{
    GL_DEBUG( glDisable )( GL_BLEND );
    GL_DEBUG( glDisable )( GL_ALPHA_TEST );
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
void gfx_begin_3d( camera_t * pcam )
{
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glLoadMatrixf )( pcam->mProjection.v );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glLoadMatrixf )( pcam->mView.v );
}

//--------------------------------------------------------------------------------------------
void gfx_end_3d()
{
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPopMatrix )();
}

//--------------------------------------------------------------------------------------------
void gfx_begin_2d( void )
{
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glLoadIdentity )();                // Reset The Projection Matrix
    GL_DEBUG( glOrtho )( 0, sdl_scr.x, sdl_scr.y, 0, -1, 1 );     // Set up an orthogonal projection

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glLoadIdentity )();

    GL_DEBUG( glDisable )( GL_DEPTH_TEST );
    GL_DEBUG( glDisable )( GL_CULL_FACE );
}

//--------------------------------------------------------------------------------------------
void gfx_end_2d( void )
{
    GL_DEBUG( glEnable )( GL_CULL_FACE );
    GL_DEBUG( glEnable )( GL_DEPTH_TEST );
}

//--------------------------------------------------------------------------------------------
void gfx_reshape_viewport( int w, int h )
{
    GL_DEBUG( glViewport )( 0, 0, w, h );
}

//--------------------------------------------------------------------------------------------
void request_clear_screen()
{
    gfx_page_clear_requested = btrue;
}

//--------------------------------------------------------------------------------------------
void do_clear_screen()
{
    bool_t try_clear;

    try_clear = bfalse;
    if ( process_running( PROC_PBASE( GProc ) ) && PROC_PBASE( GProc )->state > proc_begin )
    {
        try_clear = gfx_page_clear_requested;
    }
    else if ( process_running( PROC_PBASE( MProc ) ) && PROC_PBASE( MProc )->state > proc_begin )
    {
        try_clear = gfx_page_clear_requested;
    }

    if ( try_clear )
    {
        bool_t game_needs_clear, menu_needs_clear;

        gfx_page_clear_requested = bfalse;

        // clear the depth buffer no matter what
        GL_DEBUG( glDepthMask )( GL_TRUE );
        GL_DEBUG( glClear )( GL_DEPTH_BUFFER_BIT );

        // clear the color buffer only if necessary
        game_needs_clear = gfx.clearson && process_running( PROC_PBASE( GProc ) );
        menu_needs_clear = mnu_draw_background && process_running( PROC_PBASE( MProc ) );

        if ( game_needs_clear || menu_needs_clear )
        {
            GL_DEBUG( glClear )( GL_COLOR_BUFFER_BIT );
        }
    }
}

//--------------------------------------------------------------------------------------------
void do_flip_pages()
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
void request_flip_pages()
{
    gfx_page_flip_requested = btrue;
}

//--------------------------------------------------------------------------------------------
bool_t flip_pages_requested()
{
    return gfx_page_flip_requested;
}

//--------------------------------------------------------------------------------------------
void _flip_pages()
{
    GL_DEBUG( glFlush )();

    // draw the console on top of everything
    egoboo_console_draw_all();

    frame_all++;
    frame_fps++;
    fps_loops++;

    SDL_GL_SwapBuffers();

    gfx_update_timers();

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
void light_fans( renderlist_t * prlist )
{
    int    entry;
    Uint8  type;
    float  light;
    int    numvertices, vertex, needs_interpolation_count;
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

    if ( NULL == prlist ) return;

#if defined(CLIP_ALL_LIGHT_FANS)
    // update all visible fans once every 4 uodates
    if ( 0 != ( frame_all & 0x03 ) ) return;
#endif

    pmesh = prlist->pmesh;
    if ( NULL == pmesh ) return;

    ptmem = &( pmesh->tmem );
    pgmem = &( pmesh->gmem );

#if !defined(CLIP_LIGHT_FANS)
    // update only every frame
    local_mesh_lighting_keep = 0.9f;
#else
    // update only every 4 frames
    local_mesh_lighting_keep = POW( 0.9f, 4 );
#endif

    // cache the grid lighting
    needs_interpolation_count = 0;
    for ( entry = 0; entry < prlist->all_count; entry++ )
    {
        bool_t needs_update;
        int fan;
        float delta;
        ego_tile_info_t * ptile;

        fan = prlist->all[entry];
        if ( !mesh_grid_is_valid( pmesh, fan ) ) continue;

        ptile = ptmem->tile_list + fan;

#if defined(CLIP_LIGHT_FANS) && !defined(CLIP_ALL_LIGHT_FANS)

        // visible fans based on the update "need"
        needs_update = mesh_test_corners( pmesh, fan, delta_threshold );

        // update every 4 fans even if there is no need
        if ( !needs_update )
        {
            int ix, iy;

            // use a kind of checkerboard pattern
            ix = fan % pgmem->grids_x;
            iy = fan / pgmem->grids_x;
            if ( 0 != ((( ix ^ iy ) + frame_all ) & 0x03 ) )
            {
                needs_update = btrue;
            }
        }

#else
        needs_update = btrue;
#endif

        // does thit tile need a lighting update?
        ptile->needs_lighting_update = needs_update;

        // if there's no need for an update, go to the next tile
        if ( !needs_update ) continue;

        delta = mesh_light_corners( prlist->pmesh, fan, local_mesh_lighting_keep );

#if defined(CLIP_LIGHT_FANS)
        // use the actual measured change in the intensity at the tile edge to
        // signal whether we need to calculate the next stage
        ptile->needs_lighting_update = ( delta > delta_threshold );
#endif

        // optimize the use of the second loop
        if ( ptile->needs_lighting_update )
        {
            numvertices = tile_dict[ptile->type].numvertices;

            if ( numvertices <= 4 )
            {
                int ivrt;

                vertex = ptile->vrtstart;

                // the second loop is not needed at all
                for ( ivrt = 0; ivrt < numvertices; ivrt++, vertex++ )
                {
                    light = ptile->lcache[ivrt];

                    light = CLIP( light, 0.0f, 255.0f );
                    ptmem->clst[vertex][RR] =
                        ptmem->clst[vertex][GG] =
                        ptmem->clst[vertex][BB] = light * INV_FF;
                };

                // clear out the deltas
                memset( ptile->d1_cache, 0, sizeof( ptile->d1_cache ) );
                memset( ptile->d2_cache, 0, sizeof( ptile->d2_cache ) );

                // everything has been handled. no need to do this in another loop.
                ptile->needs_lighting_update = bfalse;
            }
            else
            {
                // the clst cannot be updated at this time. defer it to the next loop.
                needs_interpolation_count++;
            }
        }
    }

    // can we avoid this whole loop?
    if ( needs_interpolation_count > 0 )
    {
        // use the grid to light the tiles
        for ( entry = 0; entry < prlist->all_count; entry++ )
        {
            int ivrt;
            Uint32 fan;
            ego_tile_info_t * ptile;

            fan = prlist->all[entry];
            if ( !mesh_grid_is_valid( pmesh, fan ) ) continue;

            ptile = ptmem->tile_list + fan;

            if ( !ptile->needs_lighting_update ) continue;

            type        = ptile->type;
            numvertices = tile_dict[type].numvertices;
            vertex      = ptile->vrtstart;

            // copy the 1st 4 vertices
            for ( ivrt = 0; ivrt < 4; ivrt++, vertex++ )
            {
                light = ptile->lcache[ivrt];

                light = CLIP( light, 0.0f, 255.0f );
                ptmem->clst[vertex][RR] =
                    ptmem->clst[vertex][GG] =
                    ptmem->clst[vertex][BB] = light * INV_FF;
            };

            for ( /* nothing */ ; ivrt < numvertices; ivrt++, vertex++ )
            {
                light = 0;
                mesh_interpolate_vertex( ptmem, fan, ptmem->plst[vertex], &light );

                light = CLIP( light, 0.0f, 255.0f );
                ptmem->clst[vertex][RR] =
                    ptmem->clst[vertex][GG] =
                    ptmem->clst[vertex][BB] = light * INV_FF;
            };

            // clear out the deltas
            memset( ptile->d1_cache, 0, sizeof( ptile->d1_cache ) );
            memset( ptile->d2_cache, 0, sizeof( ptile->d2_cache ) );

            // untag this tile
            ptile->needs_lighting_update = bfalse;
        }
    }
}

//--------------------------------------------------------------------------------------------
float get_ambient_level()
{
    /// @details BB@> get the actual global ambient level

    float glob_amb, min_amb;

    glob_amb = 0.0f;
    min_amb  = 0.0f;
    if ( gfx.usefaredge )
    {
        glob_amb = light_a * 255.0f;
    }
    else
    {
        glob_amb = light_a * 32.0f;
    }

    // determine the minimum ambient, based on darkvision
    min_amb = INVISIBLE;
    if ( local_seedark_level > 0 )
    {
        min_amb = 52.0f * light_a * ( 1 + local_seedark_level );
    }

    return MAX( glob_amb, min_amb );
}

//--------------------------------------------------------------------------------------------
bool_t sum_global_lighting( lighting_vector_t lighting )
{
    /// @details BB@> do ambient lighting. if the module is inside, the ambient lighting
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
    /// ZZ@> This function implements a mouse cursor

    if ( cursor.x < 6 )  cursor.x = 6;
    if ( cursor.x > sdl_scr.x - 16 )  cursor.x = sdl_scr.x - 16;

    if ( cursor.y < 8 )  cursor.y = 8;
    if ( cursor.y > sdl_scr.y - 24 )  cursor.y = sdl_scr.y - 24;

    // Needed to setup text mode
    gfx_begin_text();
    {
        draw_one_font( 95, cursor.x - 5, cursor.y - 7 );
    }
    // Needed when done with text mode
    gfx_end_text();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void gfx_make_dynalist( camera_t * pcam )
{
    /// @details ZZ@> This function figures out which particles are visible, and it sets up dynamic
    ///    lighting

    int tnc, slot;
    fvec3_t  vdist;
    float    distance;

    // Don't really make a list, just set to visible or not
    dyna_list_count = 0;
    dyna_distancetobeat = 1e12;
    PRT_BEGIN_LOOP_DISPLAY( iprt, prt_bdl )
    {
        prt_bdl.prt_ptr->inview = bfalse;

        if ( !mesh_grid_is_valid( PMesh, prt_bdl.prt_ptr->onwhichgrid ) ) continue;

        prt_bdl.prt_ptr->inview = PMesh->tmem.tile_list[prt_bdl.prt_ptr->onwhichgrid].inrenderlist;

        // Set up the lights we need
        if ( !prt_bdl.prt_ptr->dynalight.on ) continue;

        vdist = fvec3_sub( prt_get_pos_v(prt_bdl.prt_ptr), pcam->track_pos.v );

        distance = vdist.x * vdist.x + vdist.y * vdist.y + vdist.z * vdist.z;
        if ( distance < dyna_distancetobeat )
        {
            bool_t found = bfalse;
            if ( dyna_list_count < gfx.dyna_list_max )
            {
                // Just add the light
                slot = dyna_list_count;
                dyna_list[slot].distance = distance;
                dyna_list_count++;
                found = btrue;
            }
            else
            {
                // Overwrite the worst one
                slot = 0;
                dyna_distancetobeat = dyna_list[0].distance;
                for ( tnc = 1; tnc < gfx.dyna_list_max; tnc++ )
                {
                    if ( dyna_list[tnc].distance > dyna_distancetobeat )
                    {
                        slot = tnc;
                        found = btrue;
                    }
                }

                if ( found )
                {
                    dyna_list[slot].distance = distance;

                    // Find the new distance to beat
                    dyna_distancetobeat = dyna_list[0].distance;
                    for ( tnc = 1; tnc < gfx.dyna_list_max; tnc++ )
                    {
                        if ( dyna_list[tnc].distance > dyna_distancetobeat )
                        {
                            dyna_distancetobeat = dyna_list[tnc].distance;
                        }
                    }
                }
            }

            if ( found )
            {
                dyna_list[slot].pos     = prt_get_pos( prt_bdl.prt_ptr );
                dyna_list[slot].level   = prt_bdl.prt_ptr->dynalight.level;
                dyna_list[slot].falloff = prt_bdl.prt_ptr->dynalight.falloff;
            }
        }
    }
    PRT_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void do_grid_lighting( ego_mpd_t * pmesh, camera_t * pcam )
{
    /// @details ZZ@> Do all tile lighting, dynamic and global

    int   cnt, tnc, fan, entry;
    int ix, iy;
    float x0, y0, local_keep;
    bool_t needs_dynalight;

    lighting_vector_t global_lighting;

    int                  reg_count;
    dynalight_registry_t reg[TOTAL_MAX_DYNA];

    ego_frect_t mesh_bound, light_bound;

    ego_mpd_info_t  * pinfo;
    grid_mem_t      * pgmem;
    tile_mem_t      * ptmem;
    ego_grid_info_t * glist;

    if ( NULL == pmesh ) return;
    pinfo = &( pmesh->info );
    pgmem = &( pmesh->gmem );
    ptmem = &( pmesh->tmem );

    glist = pgmem->grid_list;

    // refresh the dynamic light list
    gfx_make_dynalist( pcam );

    // find a bounding box for the "frustum"
    mesh_bound.xmin = pgmem->edge_x;
    mesh_bound.xmax = 0;
    mesh_bound.ymin = pgmem->edge_y;
    mesh_bound.ymax = 0;
    for ( entry = 0; entry < renderlist.all_count; entry++ )
    {
        fan = renderlist.all[entry];
        if ( !mesh_grid_is_valid( pmesh, fan ) ) continue;

        ix = fan % pinfo->tiles_x;
        iy = fan / pinfo->tiles_x;

        x0 = ix * GRID_SIZE;
        y0 = iy * GRID_SIZE;

        mesh_bound.xmin = MIN( mesh_bound.xmin, x0 - GRID_SIZE / 2 );
        mesh_bound.xmax = MAX( mesh_bound.xmax, x0 + GRID_SIZE / 2 );
        mesh_bound.ymin = MIN( mesh_bound.ymin, y0 - GRID_SIZE / 2 );
        mesh_bound.ymax = MAX( mesh_bound.ymax, y0 + GRID_SIZE / 2 );
    }

    if ( mesh_bound.xmin >= mesh_bound.xmax || mesh_bound.ymin >= mesh_bound.ymax ) return;

    // make bounding boxes for each dynamic light
    reg_count = 0;
    light_bound.xmin = pgmem->edge_x;
    light_bound.xmax = 0;
    light_bound.ymin = pgmem->edge_y;
    light_bound.ymax = 0;
    for ( cnt = 0; cnt < dyna_list_count; cnt++ )
    {
        float radius;
        ego_frect_t ftmp;

        dynalight_t * pdyna = dyna_list + cnt;

        if ( pdyna->falloff <= 0 ) continue;

        radius = SQRT( pdyna->falloff * 765.0f / 2.0f );

        ftmp.xmin = floor(( pdyna->pos.x - radius ) / GRID_SIZE ) * GRID_SIZE;
        ftmp.xmax = ceil(( pdyna->pos.x + radius ) / GRID_SIZE ) * GRID_SIZE;
        ftmp.ymin = floor(( pdyna->pos.y - radius ) / GRID_SIZE ) * GRID_SIZE;
        ftmp.ymax = ceil(( pdyna->pos.y + radius ) / GRID_SIZE ) * GRID_SIZE;

        // check to see if it intersects the "frustum"
        if ( ftmp.xmin <= mesh_bound.xmax && ftmp.xmax >= mesh_bound.xmin )
        {
            if ( ftmp.ymin <= mesh_bound.ymax && ftmp.ymax >= mesh_bound.ymin )
            {
                reg[reg_count].bound     = ftmp;
                reg[reg_count].reference = cnt;
                reg_count++;

                // determine the maxumum bounding box that encloses all valid lights
                light_bound.xmin = MIN( light_bound.xmin, ftmp.xmin );
                light_bound.xmax = MAX( light_bound.xmax, ftmp.xmax );
                light_bound.ymin = MIN( light_bound.ymin, ftmp.ymin );
                light_bound.ymax = MAX( light_bound.ymax, ftmp.ymax );
            }
        }
    }

    needs_dynalight = btrue;
    if ( 0 == reg_count || light_bound.xmin >= light_bound.xmax || light_bound.ymin >= light_bound.ymax )
    {
        needs_dynalight = bfalse;
    }

    // sum up the lighting from global sources
    sum_global_lighting( global_lighting );

    // make the grids update their lighting every 4 frames
    local_keep = POW( dynalight_keep, 4 );

    // Add to base light level in normal mode
    for ( entry = 0; entry < renderlist.all_count; entry++ )
    {
        bool_t resist_lighting_calculation = btrue;

        // grab each grid box in the "frustum"
        fan = renderlist.all[entry];
        if ( !mesh_grid_is_valid( pmesh, fan ) ) continue;

        ix = fan % pinfo->tiles_x;
        iy = fan / pinfo->tiles_x;

        // Resist the lighting calculation?
        // This is a speedup for lighting calculations so that
        // not every light-tile calculation is done every single frame
        resist_lighting_calculation = ( 0 != ((( ix ^ iy ) + frame_all ) & 0x03 ) );

        if ( !resist_lighting_calculation )
        {
            lighting_cache_t * pcache_old;
            lighting_cache_t   cache_new;

            int    dynalight_count = 0;

            // this is not a "bad" grid box, so grab the lighting info
            pcache_old = &( glist[fan].cache );

            lighting_cache_init( &cache_new );

            // copy the global lighting
            for ( tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++ )
            {
                cache_new.low.lighting[tnc] = global_lighting[tnc];
                cache_new.hgh.lighting[tnc] = global_lighting[tnc];
            };

            // do we need any dynamic lighting at all?
            if ( gfx.shading != GL_FLAT && needs_dynalight )
            {
                // calculate the local lighting

                ego_frect_t fgrid_rect;

                x0 = ix * GRID_SIZE;
                y0 = iy * GRID_SIZE;

                // check this grid vertex relative to the measured light_bound
                fgrid_rect.xmin = x0 - GRID_SIZE / 2;
                fgrid_rect.xmax = x0 + GRID_SIZE / 2;
                fgrid_rect.ymin = y0 - GRID_SIZE / 2;
                fgrid_rect.ymax = y0 + GRID_SIZE / 2;

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
                            pdyna = dyna_list + tnc;

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

            // blend in the global lighting every single time

            // average this in with the existing lighting
            lighting_cache_blend( pcache_old, &cache_new, local_keep );

            // find the max intensity
            lighting_cache_max_light( pcache_old );
        }
    }
}

//--------------------------------------------------------------------------------------------
void render_water( renderlist_t * prlist )
{
    /// @details ZZ@> This function draws all of the water fans

    int cnt;

    // Bottom layer first
    if ( gfx.draw_water_1 /* && water.layer[1].z > -water.layer[1].amp */ )
    {
        for ( cnt = 0; cnt < prlist->wat_count; cnt++ )
        {
            render_water_fan( prlist->pmesh, prlist->wat[cnt], 1 );
        }
    }

    // Top layer second
    if ( gfx.draw_water_0 /* && water.layer[0].z > -water.layer[0].amp */ )
    {
        for ( cnt = 0; cnt < prlist->wat_count; cnt++ )
        {
            render_water_fan( prlist->pmesh, prlist->wat[cnt], 0 );
        }
    }
}

//--------------------------------------------------------------------------------------------
void gfx_reload_all_textures()
{
    TxTitleImage_reload_all();
    TxTexture_reload_all();
}
