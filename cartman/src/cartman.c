#include "cartman.h"

#include "cartman_mpd.h"
#include "cartman_functions.h"
#include "cartman_input.h"
#include "cartman_gui.h"
#include "cartman_gfx.h"
#include "cartman_math.inl"

#include <egolib.h>

#include <stdio.h>          // For printf and such
#include <fcntl.h>          // For fast file i/o
#include <math.h>
#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_cart_mouse_data;
typedef struct s_cart_mouse_data cart_mouse_data_t;

struct s_light;
typedef struct s_light light_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_light
{
    int           x;
    int           y;
    Uint8 level;
    int           radius;
};

//--------------------------------------------------------------------------------------------
struct s_cart_mouse_data
{
    // click/drag window
    int             win_id;
    Uint16          win_mode;
    cartman_mpd_t * pmesh;

    // click location
    float   xpos, ypos;
    int     onfan;
    int     xfan, yfan;

    // click data
    Uint8   type;       // Tile fantype
    Uint8   fx;         // Tile effects
    Uint8   tx;         // Tile texture
    Uint8   upper;      // Tile upper bits
    Uint16  presser;    // Random add for tiles

    // Rectangle drawing
    int     rect_draw;   // draw it
    int     rect_drag;   // which window id
    int     rect_done;   // which window id
    float   rect_x0;     //
    float   rect_y0;     //
    float   rect_x1;     //
    float   rect_y1;     //
};

static cart_mouse_data_t * cart_mouse_data_ctor( cart_mouse_data_t * );
static void cart_mouse_data_toggle_fx( int fxmask );

// helper functions
static void cart_mouse_data_mesh_set_tile( Uint16 tiletoset );
static void cart_mouse_data_flatten_mesh();
static void cart_mouse_data_clear_mesh();
static void cart_mouse_data_three_e_mesh();
static void cart_mouse_data_mesh_replace_tile( bool_t tx_only, bool_t at_floor_level );
static void cart_mouse_data_mesh_set_fx();
static void cart_mouse_data_rect_select();
static void cart_mouse_data_rect_unselect();
static void cart_mouse_data_mesh_replace_fx();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

STRING egoboo_path = { "" };

int     onscreen_count = 0;
Uint32  onscreen_vert[MAXPOINTS];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static cart_mouse_data_t mdata = { -1 };

static float cartman_zoom_hrz = 1.0f;
static float cartman_zoom_vrt = 1.0f;

static STRING  loadname;        // Text

static int     brushsize = 3;      // Size of raise/lower terrain brush
static int     brushamount = 50;   // Amount of raise/lower

static float   debugx = -1;        // Blargh
static float   debugy = -1;        //

static bool_t addinglight = bfalse;

static int numlight;
static light_t light_lst[MAXLIGHT];

static int ambi = 22;
static int ambicut = 1;
static int direct = 16;

static bool_t _sdl_atexit_registered = bfalse;
static bool_t _ttf_atexit_registered = bfalse;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// ui functions
static void draw_cursor_in_window( window_t * pwin );
static void unbound_mouse();
static void bound_mouse();
static bool_t cartman_check_keys( const char *modname, cartman_mpd_t * pmesh );
static bool_t cartman_check_mouse( const char *modname, cartman_mpd_t * pmesh );
static void   cartman_check_input( const char *modname, cartman_mpd_t * pmesh );

// loading
static bool_t load_module( const char *modname, cartman_mpd_t * pmesh );
static void load_basic_textures( const char *modname );
static void cartman_create_mesh( cartman_mpd_t * pmesh );

// saving
static void cartman_save_mesh( const char *modname, cartman_mpd_t * pmesh );

// gfx functions
static void load_all_windows( cartman_mpd_t * pmesh );

static void render_tile_window( window_t * pwin, float zoom_hrz, float zoom_vrt );
static void render_fx_window( window_t * pwin, float zoom_hrz, float zoom_vrt );
static void render_vertex_window( window_t * pwin, float zoom_hrz, float zoom_vrt );
static void render_side_window( window_t * pwin, float zoom_hrz, float zoom_vrt );
static void render_window( window_t * pwin );
static void render_all_windows();

static void draw_window_background( window_t * pwin );
static void draw_all_windows( void );
static void draw_lotsa_stuff( cartman_mpd_t * pmesh );

static void draw_main( cartman_mpd_t * pmesh );

// camera stuff
static void move_camera( cartman_mpd_info_t * pinfo );
static void bound_camera( cartman_mpd_info_t * pinfo );

// misc
static void mesh_calc_vrta( cartman_mpd_t * pmesh );
static void fan_calc_vrta( cartman_mpd_t * pmesh, int fan );
static int  vertex_calc_vrta( cartman_mpd_t * pmesh, Uint32 vert );
static void make_onscreen();
static void onscreen_add_fan( cartman_mpd_t * pmesh, Uint32 fan );
static void ease_up_mesh( cartman_mpd_t * pmesh, float zoom_vrt );

// cartman versions of these functions
static int cartman_get_vertex( cartman_mpd_t * pmesh, int x, int y, int num );

// light functions
static void add_light( int x, int y, int radius, int level );
static void alter_light( int x, int y );
static void draw_light( int number, window_t * pwin, float zoom_hrz );

static void cartman_check_mouse_side( window_t * pwin, float zoom_hrz, float zoom_vrt );
static void cartman_check_mouse_tile( window_t * pwin, float zoom_hrz, float zoom_vrt );
static void cartman_check_mouse_fx( window_t * pwin, float zoom_hrz, float zoom_vrt );
static void cartman_check_mouse_vertex( window_t * pwin, float zoom_hrz, float zoom_vrt );

// shutdown function
static void main_end( void );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C"
{
#endif
    extern bool_t config_download( egoboo_config_t * pcfg );
    extern bool_t config_upload( egoboo_config_t * pcfg );
#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "standard.c"           // Some functions that I always use

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void add_light( int x, int y, int radius, int level )
{
    if ( numlight >= MAXLIGHT )  numlight = MAXLIGHT - 1;

    light_lst[numlight].x = x;
    light_lst[numlight].y = y;
    light_lst[numlight].radius = radius;
    light_lst[numlight].level = level;
    numlight++;
}

//--------------------------------------------------------------------------------------------
void alter_light( int x, int y )
{
    int radius, level;

    numlight--;
    if ( numlight < 0 )  numlight = 0;

    level = abs( light_lst[numlight].y - y );

    radius = abs( light_lst[numlight].x - x );
    if ( radius > MAXRADIUS / cartman_zoom_hrz )  radius = MAXRADIUS / cartman_zoom_hrz;
    if ( radius < MINRADIUS / cartman_zoom_hrz )  radius = MINRADIUS / cartman_zoom_hrz;

    light_lst[numlight].radius = radius;
    if ( level > MAP_MAXLEVEL ) level = MAP_MAXLEVEL;
    if ( level < MAP_MINLEVEL ) level = MAP_MINLEVEL;
    light_lst[numlight].level = level;

    numlight++;
}

//--------------------------------------------------------------------------------------------
void draw_light( int number, window_t * pwin, float zoom_hrz )
{
    int xdraw, ydraw, radius;
    Uint8 color;

    xdraw = ( light_lst[number].x / FOURNUM * zoom_hrz ) - cam.x + ( pwin->surfacex >> 1 ) - SMALLXY;
    ydraw = ( light_lst[number].y / FOURNUM * zoom_hrz ) - cam.y + ( pwin->surfacey >> 1 ) - SMALLXY;
    radius = abs( light_lst[number].radius ) / FOURNUM * zoom_hrz;
    color = light_lst[number].level >> 3;

    //color = MAKE_BGR(pwin->bmp, color, color, color);
    //circle(pwin->bmp, xdraw, ydraw, radius, color);
}

//--------------------------------------------------------------------------------------------
void draw_cursor_in_window( window_t * pwin )
{
    int x, y;

    if ( NULL == pwin || !pwin->on ) return;

    if ( -1 != mdata.win_id && pwin->id != mdata.win_id )
    {
        int size = POINT_SIZE( 10 );

        x = pwin->x + ( mos.x - window_lst[mdata.win_id].x );
        y = pwin->y + ( mos.y - window_lst[mdata.win_id].y );

        ogl_draw_sprite_2d( &tx_pointon, x - size / 2, y - size / 2, size, size );
    }

}

//--------------------------------------------------------------------------------------------
int cartman_get_vertex( cartman_mpd_t * pmesh, int x, int y, int num )
{
    int vert;

    if ( NULL == pmesh ) pmesh = &mesh;

    vert = cartman_mpd_get_vertex( pmesh,  x, y, num );

    if ( vert == -1 )
    {
        return vert;
        printf( "BAD GET_VERTEX NUMBER(2nd), %d at %d, %d...\n", num, x, y );
        exit( -1 );
    }

    return vert;
}

//--------------------------------------------------------------------------------------------
void onscreen_add_fan( cartman_mpd_t * pmesh, Uint32 fan )
{
    // ZZ> This function flags a fan's points as being "onscreen"
    int cnt;
    Uint32 vert, fan_type, vert_count;

    if ( NULL == pmesh ) pmesh = &mesh;

    fan_type    = pmesh->fan[fan].type;
    vert_count  = tile_dict[fan_type].numvertices;

    for ( cnt = 0, vert = pmesh->fan[fan].vrtstart;
          cnt < vert_count && CHAINEND != vert;
          cnt++, vert = pmesh->vrt[vert].next )
    {
        if ( vert >= MPD_VERTICES_MAX ) break;
        if ( onscreen_count >= MAXPOINTS ) break;

        if ( VERTEXUNUSED == pmesh->vrt[vert].a ) continue;

        onscreen_vert[onscreen_count] = vert;
        onscreen_count++;
    }
}

//--------------------------------------------------------------------------------------------
void make_onscreen( cartman_mpd_t * pmesh )
{
    int mapx, mapy;
    int mapxstt, mapystt;
    int mapxend, mapyend;
    int fan;

    if ( NULL == pmesh ) pmesh = &mesh;

    mapxstt = FLOOR(( cam.x - cam.w  * 0.5f ) / TILE_FSIZE ) - 1.0f;
    mapystt = FLOOR(( cam.y - cam.h  * 0.5f ) / TILE_FSIZE ) - 1.0f;

    mapxend = CEIL(( cam.x + cam.w  * 0.5f ) / TILE_FSIZE ) + 1.0f;
    mapyend = CEIL(( cam.y + cam.h  * 0.5f ) / TILE_FSIZE ) + 1.0f;

    onscreen_count = 0;
    for ( mapy = mapystt; mapy <= mapyend; mapy++ )
    {
        if ( mapy < 0 || mapy >= pmesh->info.tiles_y ) continue;

        for ( mapx = mapxstt; mapx <= mapxend; mapx++ )
        {
            if ( mapx < 0 || mapx >= pmesh->info.tiles_x ) continue;

            fan = cartman_mpd_get_fan( pmesh, mapx, mapy );
            if ( fan < 0 || fan >= MPD_TILE_MAX ) continue;

            onscreen_add_fan( pmesh, fan );
        }
    }
}

//--------------------------------------------------------------------------------------------
void load_basic_textures( const char *modname )
{
    // ZZ> This function loads the standard textures for a module
    STRING newloadname;
    SDL_Surface *bmptemp;       // A temporary bitmap

    make_newloadname( modname, SLASH_STR "gamedat" SLASH_STR "tile0.bmp", newloadname );
    bmptemp = cartman_LoadIMG( newloadname );
    get_tiles( bmptemp );
    SDL_FreeSurface( bmptemp );

    make_newloadname( modname, SLASH_STR "gamedat" SLASH_STR "tile1.bmp", newloadname );
    bmptemp = cartman_LoadIMG( newloadname );
    get_tiles( bmptemp );
    SDL_FreeSurface( bmptemp );

    make_newloadname( modname, SLASH_STR "gamedat" SLASH_STR "tile2.bmp", newloadname );
    bmptemp = cartman_LoadIMG( newloadname );
    get_tiles( bmptemp );
    SDL_FreeSurface( bmptemp );

    make_newloadname( modname, SLASH_STR "gamedat" SLASH_STR "tile3.bmp", newloadname );
    bmptemp = cartman_LoadIMG( newloadname );
    get_tiles( bmptemp );
    SDL_FreeSurface( bmptemp );
}

//--------------------------------------------------------------------------------------------
bool_t load_module( const char *modname, cartman_mpd_t * pmesh )
{
    STRING mod_path = EMPTY_CSTR;

    if ( NULL == pmesh ) pmesh = &mesh;

    sprintf( mod_path, "%s" SLASH_STR "modules" SLASH_STR "%s", egoboo_path, modname );

    if ( !setup_init_module_vfs_paths( modname ) )
    {
        return bfalse;
    }

    //  show_name(mod_path);
    load_basic_textures( mod_path );
    if ( NULL == cartman_mpd_load_vfs( /*mod_path,*/ pmesh ) )
    {
        cartman_create_mesh( pmesh );
    }

    //read_wawalite( mod_path );

    numlight = 0;
    addinglight = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void render_tile_window( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    oglx_texture_t * tx_tile;
    float x, y;
    int mapx, mapxstt, mapxend;
    int mapy, mapystt, mapyend;
    int fan;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_TILE ) ) return;

    if ( NULL == pwin->pmesh ) pwin->pmesh = &mesh;

    glPushAttrib( GL_SCISSOR_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT );
    {
        // set the viewport transformation
        glViewport( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        // clip the viewport
        glEnable( GL_SCISSOR_TEST );
        glScissor( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        cartman_begin_ortho_camera_hrz( pwin, &cam, zoom_hrz, zoom_hrz );
        {
            mapxstt = FLOOR(( cam.x - cam.w  * 0.5f ) / TILE_FSIZE ) - 1.0f;
            mapystt = FLOOR(( cam.y - cam.h  * 0.5f ) / TILE_FSIZE ) - 1.0f;

            mapxend = CEIL(( cam.x + cam.w  * 0.5f ) / TILE_FSIZE ) + 1.0f;
            mapyend = CEIL(( cam.y + cam.h  * 0.5f ) / TILE_FSIZE ) + 1.0f;

            for ( mapy = mapystt; mapy <= mapyend; mapy++ )
            {
                if ( mapy < 0 || mapy >= pwin->pmesh->info.tiles_y ) continue;
                y = mapy * TILE_ISIZE;

                for ( mapx = mapxstt; mapx <= mapxend; mapx++ )
                {
                    if ( mapx < 0 || mapx >= pwin->pmesh->info.tiles_x ) continue;
                    x = mapx * TILE_ISIZE;

                    fan     = cartman_mpd_get_fan( pwin->pmesh, mapx, mapy );

                    tx_tile = NULL;
                    if ( fan >= 0 && fan < MPD_TILE_MAX )
                    {
                        tx_tile = tile_at( pwin->pmesh, fan );
                    }

                    tx_tile = NULL;
                    if ( fan >= 0 && fan < MPD_TILE_MAX )
                    {
                        tx_tile = tile_at( pwin->pmesh, fan );
                    }

                    if ( NULL != tx_tile )
                    {
                        draw_top_tile( x, y, fan, tx_tile, bfalse, pwin->pmesh );
                    }
                }
            }
        }
        cartman_end_ortho_camera();

        // force OpenGL to execute these commands
        glFlush();

        //cnt = 0;
        //while (cnt < numlight)
        //{
        //    draw_light(cnt, pwin);
        //    cnt++;
        //}
    }
    glPopAttrib();

}

//--------------------------------------------------------------------------------------------
void render_fx_window( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    oglx_texture_t * tx_tile;
    float x, y;
    int mapx, mapxstt, mapxend;
    int mapy, mapystt, mapyend;
    int fan;
    float zoom_fx;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_FX ) ) return;

    if ( NULL == pwin->pmesh ) pwin->pmesh = &mesh;

    zoom_fx = ( zoom_hrz < 1.0f ) ? zoom_hrz : 1.0f;

    glPushAttrib( GL_SCISSOR_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT );
    {
        // set the viewport transformation
        glViewport( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        // clip the viewport
        glEnable( GL_SCISSOR_TEST );
        glScissor( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        cartman_begin_ortho_camera_hrz( pwin, &cam, zoom_hrz, zoom_hrz );
        {
            mapxstt = FLOOR(( cam.x - cam.w  * 0.5f ) / TILE_FSIZE ) - 1.0f;
            mapystt = FLOOR(( cam.y - cam.h  * 0.5f ) / TILE_FSIZE ) - 1.0f;

            mapxend = CEIL(( cam.x + cam.w  * 0.5f ) / TILE_FSIZE ) + 1.0f;
            mapyend = CEIL(( cam.y + cam.h  * 0.5f ) / TILE_FSIZE ) + 1.0f;

            for ( mapy = mapystt; mapy <= mapyend; mapy++ )
            {
                if ( mapy < 0 || mapy >= pwin->pmesh->info.tiles_y ) continue;
                y = mapy * TILE_ISIZE;

                for ( mapx = mapxstt; mapx <= mapxend; mapx++ )
                {
                    if ( mapx < 0 || mapx >= pwin->pmesh->info.tiles_x ) continue;
                    x = mapx * TILE_ISIZE;

                    fan     = cartman_mpd_get_fan( pwin->pmesh, mapx, mapy );

                    tx_tile = NULL;
                    if ( fan >= 0 && fan < MPD_TILE_MAX )
                    {
                        tx_tile = tile_at( pwin->pmesh, fan );
                    }

                    if ( NULL != tx_tile )
                    {
                        ogl_draw_sprite_2d( tx_tile, x, y, TILE_ISIZE, TILE_ISIZE );

                        // water is whole tile
                        draw_tile_fx( x, y, pwin->pmesh->fan[fan].fx, 4.0f * zoom_fx );
                    }
                }
            }
        }
        cartman_end_ortho_camera();

    }
    glPopAttrib();
}

//--------------------------------------------------------------------------------------------
void render_vertex_window( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    int mapx, mapxstt, mapxend;
    int mapy, mapystt, mapyend;
    int fan;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_VERTEX ) ) return;

    if ( NULL == pwin->pmesh ) pwin->pmesh = &mesh;

    glPushAttrib( GL_SCISSOR_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT );
    {
        // set the viewport transformation
        glViewport( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        // clip the viewport
        glEnable( GL_SCISSOR_TEST );
        glScissor( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        cartman_begin_ortho_camera_hrz( pwin, &cam, zoom_hrz, zoom_hrz );
        {
            mapxstt = FLOOR(( cam.x - cam.w * 0.5f ) / TILE_FSIZE ) - 1.0f;
            mapystt = FLOOR(( cam.y - cam.h * 0.5f ) / TILE_FSIZE ) - 1.0f;

            mapxend = CEIL(( cam.x + cam.w * 0.5f ) / TILE_FSIZE ) + 1;
            mapyend = CEIL(( cam.y + cam.h * 0.5f ) / TILE_FSIZE ) + 1;

            for ( mapy = mapystt; mapy <= mapyend; mapy++ )
            {
                if ( mapy < 0 || mapy >= pwin->pmesh->info.tiles_y ) continue;

                for ( mapx = mapxstt; mapx <= mapxend; mapx++ )
                {
                    if ( mapx < 0 || mapx >= pwin->pmesh->info.tiles_x ) continue;

                    fan = cartman_mpd_get_fan( pwin->pmesh, mapx, mapy );
                    if ( fan >= 0 && fan < MPD_TILE_MAX )
                    {
                        draw_top_fan( pwin, fan, zoom_hrz );
                    }
                }
            }

            if ( mdata.rect_draw && pwin->id == mdata.rect_drag )
            {
                float color[4];
                float x_min, x_max;
                float y_min, y_max;

                OGL_MAKE_COLOR_4( color, 0x3F, 16 + ( timclock&15 ), 16 + ( timclock&15 ), 0 );

                x_min = mdata.rect_x0;
                x_max = mdata.rect_x1;
                if ( x_min > x_max ) { float tmp = x_max; x_max = x_min; x_min = tmp; }

                y_min = mdata.rect_y0;
                y_max = mdata.rect_y1;
                if ( y_min > y_max ) { float tmp = y_max; y_max = y_min; y_min = tmp; }

                ogl_draw_box( x_min, y_min, x_max - x_min, y_max - y_min, color );
            }
        }
        cartman_end_ortho_camera();

        // force OpenGL to execute these commands
        glFlush();
    }
    glPopAttrib();
}

//--------------------------------------------------------------------------------------------
void render_side_window( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    int mapx, mapxstt, mapxend;
    int mapy, mapystt, mapyend;
    int fan;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_SIDE ) ) return;

    if ( NULL == pwin->pmesh ) pwin->pmesh = &mesh;

    glPushAttrib( GL_SCISSOR_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT );
    {
        // set the viewport transformation
        glViewport( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        // clip the viewport
        glEnable( GL_SCISSOR_TEST );
        glScissor( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        cartman_begin_ortho_camera_vrt( pwin, &cam, zoom_hrz, zoom_vrt / 3.0f );
        {
            mapxstt = FLOOR(( cam.x - cam.w * 0.5f ) / TILE_FSIZE ) - 1.0f;
            mapystt = FLOOR(( cam.y - cam.h * 0.5f ) / TILE_FSIZE ) - 1.0f;

            mapxend = CEIL(( cam.x + cam.w * 0.5f ) / TILE_FSIZE ) + 1;
            mapyend = CEIL(( cam.y + cam.h * 0.5f ) / TILE_FSIZE ) + 1;

            for ( mapy = mapystt; mapy <= mapyend; mapy++ )
            {
                if ( mapy < 0 || mapy >= pwin->pmesh->info.tiles_y ) continue;

                for ( mapx = mapxstt; mapx <= mapxend; mapx++ )
                {
                    if ( mapx < 0 || mapx >= pwin->pmesh->info.tiles_x ) continue;

                    fan = cartman_mpd_get_fan( pwin->pmesh, mapx, mapy );
                    if ( fan < 0 || fan >= MPD_TILE_MAX ) continue;

                    draw_side_fan( pwin, fan, zoom_hrz, zoom_vrt );
                }
            }

            if ( mdata.rect_draw && pwin->id == mdata.rect_drag )
            {
                float color[4];
                float x_min, x_max;
                float y_min, y_max;

                OGL_MAKE_COLOR_4( color, 0x3F, 16 + ( timclock&15 ), 16 + ( timclock&15 ), 0 );

                x_min = mdata.rect_x0;
                x_max = mdata.rect_x1;
                if ( x_min > x_max ) { float tmp = x_max; x_max = x_min; x_min = tmp; }

                y_min = mdata.rect_y0;
                y_max = mdata.rect_y1;
                if ( y_min > y_max ) { float tmp = y_max; y_max = y_min; y_min = tmp; }

                ogl_draw_box( x_min, y_min, x_max - x_min, y_max - y_min, color );
            }
        }
        cartman_end_ortho_camera();

        // force OpenGL to execute these commands
        glFlush();
    }
    glPopAttrib();
}

//--------------------------------------------------------------------------------------------
void render_window( window_t * pwin )
{
    if ( NULL == pwin || !pwin->on ) return;

    glPushAttrib( GL_SCISSOR_BIT );
    {
        glEnable( GL_SCISSOR_TEST );
        glScissor( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        make_onscreen( pwin->pmesh );

        if ( HAS_BITS( pwin->mode, WINMODE_TILE ) )
        {
            render_tile_window( pwin, cartman_zoom_hrz, cartman_zoom_vrt );
        }

        if ( HAS_BITS( pwin->mode, WINMODE_VERTEX ) )
        {
            render_vertex_window( pwin, cartman_zoom_hrz, cartman_zoom_vrt );
        }

        if ( HAS_BITS( pwin->mode, WINMODE_SIDE ) )
        {
            render_side_window( pwin, cartman_zoom_hrz, cartman_zoom_vrt );
        }

        if ( HAS_BITS( pwin->mode, WINMODE_FX ) )
        {
            render_fx_window( pwin, cartman_zoom_hrz, cartman_zoom_vrt );
        }

        draw_cursor_in_window( pwin );

    }
    glPopAttrib();
}

//--------------------------------------------------------------------------------------------
void render_all_windows( void )
{
    int cnt;

    for ( cnt = 0; cnt < MAXWIN; cnt++ )
    {
        render_window( window_lst + cnt );
    }
}

//--------------------------------------------------------------------------------------------
void load_all_windows( cartman_mpd_t * pmesh )
{
    int cnt;

    if ( NULL == pmesh ) pmesh = &mesh;

    for ( cnt = 0; cnt < MAXWIN; cnt++ )
    {
        window_lst[cnt].on = bfalse;
        oglx_texture_Release( &( window_lst[cnt].tex ) );
    }

    load_window( window_lst + 0, 0, "data" SLASH_STR "window.png", 180, 16,  7, 9, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, WINMODE_VERTEX, pmesh );
    load_window( window_lst + 1, 1, "data" SLASH_STR "window.png", 410, 16,  7, 9, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, WINMODE_TILE,   pmesh );
    load_window( window_lst + 2, 2, "data" SLASH_STR "window.png", 180, 248, 7, 9, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, WINMODE_SIDE,   pmesh );
    load_window( window_lst + 3, 3, "data" SLASH_STR "window.png", 410, 248, 7, 9, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, WINMODE_FX,     pmesh );
}

//--------------------------------------------------------------------------------------------
void draw_window_background( window_t * pwin )
{
    if ( NULL == pwin || !pwin->on ) return;

    ogl_draw_sprite_2d( &( pwin->tex ), pwin->x, pwin->y, pwin->surfacex, pwin->surfacey );
}

//--------------------------------------------------------------------------------------------
void draw_all_windows( void )
{
    int cnt;

    for ( cnt = 0; cnt < MAXWIN; cnt++ )
    {
        draw_window_background( window_lst + cnt );
    }
}

//--------------------------------------------------------------------------------------------
void bound_camera( cartman_mpd_info_t * pinfo )
{
    if ( cam.x < 0 )
    {
        cam.x = 0;
    }
    else if ( cam.x > pinfo->edgex )
    {
        cam.x = pinfo->edgex;
    }

    if ( cam.y < 0 )
    {
        cam.y = 0;
    }
    else if ( cam.y > pinfo->edgey )
    {
        cam.y = pinfo->edgey;
    }

    if ( cam.z < -pinfo->edgez )
    {
        cam.z = -pinfo->edgez;
    }
    else if ( cam.y > pinfo->edgey )
    {
        cam.y = pinfo->edgez;
    }

}

//--------------------------------------------------------------------------------------------
void unbound_mouse()
{
    if ( !mos.drag )
    {
        mos.tlx = 0;
        mos.tly = 0;
        mos.brx = sdl_scr.x - 1;
        mos.bry = sdl_scr.y - 1;
    }
}

//--------------------------------------------------------------------------------------------
void bound_mouse()
{
    if ( mdata.win_id != -1 )
    {
        mos.tlx = window_lst[mdata.win_id].x + window_lst[mdata.win_id].borderx;
        mos.tly = window_lst[mdata.win_id].y + window_lst[mdata.win_id].bordery;
        mos.brx = mos.tlx + window_lst[mdata.win_id].surfacex - 1;
        mos.bry = mos.tly + window_lst[mdata.win_id].surfacey - 1;
    }
}

//--------------------------------------------------------------------------------------------
int vertex_calc_vrta( cartman_mpd_t * pmesh, Uint32 vert )
{
    int newa, cnt;
    float x, y, z;
    float brx, bry, brz, deltaz;
    float newlevel, distance, disx, disy;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( CHAINEND == vert || vert >= MPD_VERTICES_MAX ) return ~0;

    if ( VERTEXUNUSED == pmesh->vrt[vert].a ) return ~0;

    // To make life easier
    x = pmesh->vrt[vert].x;
    y = pmesh->vrt[vert].y;
    z = pmesh->vrt[vert].z;

    // Directional light
    brx = x + 64;
    bry = y + 64;
    brz = cartman_mpd_get_level( pmesh,  brx, y ) +
          cartman_mpd_get_level( pmesh,  x, bry ) +
          cartman_mpd_get_level( pmesh,  x + 46, y + 46 );
    if ( z < -128 ) z = -128;
    if ( brz < -128*3 ) brz = -128 * 3;
    deltaz = z + z + z - brz;
    newa = ( deltaz * direct / 256.0f );

    // Point lights !!!BAD!!!
    newlevel = 0;
    cnt = 0;
    while ( cnt < numlight )
    {
        disx = x - light_lst[cnt].x;
        disy = y - light_lst[cnt].y;
        distance = sqrt(( float )( disx * disx + disy * disy ) );
        if ( distance < light_lst[cnt].radius )
        {
            newlevel += (( light_lst[cnt].level * ( light_lst[cnt].radius - distance ) ) / light_lst[cnt].radius );
        }
        cnt++;
    }
    newa += newlevel;

    // Bounds
    if ( newa < -ambicut ) newa = -ambicut;
    newa += ambi;
    pmesh->vrt[vert].a = CLIP( newa, 1, 255 );

    // Edge fade
    //dist = dist_from_border( pmesh->vrt[vert].x, pmesh->vrt[vert].y );
    //if ( dist <= FADEBORDER )
    //{
    //    newa = newa * dist / FADEBORDER;
    //    if ( newa == VERTEXUNUSED )  newa = 1;
    //    pmesh->vrt[vert].a = newa;
    //}

    return newa;
}

//--------------------------------------------------------------------------------------------
void fan_calc_vrta( cartman_mpd_t * pmesh, int fan )
{
    int num, cnt;
    Uint8 type;
    Uint32 vert;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( fan < 0 || fan >= MPD_TILE_MAX ) return;

    type = pmesh->fan[fan].type;
    if ( type >= MPD_FAN_TYPE_MAX ) return;

    num = tile_dict[type].numvertices;
    if ( 0 == num ) return;

    for ( cnt = 0, vert = pmesh->fan[fan].vrtstart;
          cnt < num && CHAINEND != vert;
          cnt++, vert = pmesh->vrt[vert].next )
    {
        vertex_calc_vrta( pmesh, vert );
    }
}

//--------------------------------------------------------------------------------------------
void mesh_calc_vrta( cartman_mpd_t * pmesh )
{
    int mapx, mapy;
    int fan;

    if ( NULL == pmesh ) pmesh = &mesh;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            fan = cartman_mpd_get_fan( pmesh, mapx, mapy );

            fan_calc_vrta( pmesh, fan );
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_camera( cartman_mpd_info_t * pinfo )
{
    if (( -1 != mdata.win_id ) && ( MOUSE_PRESSED( SDL_BUTTON_MIDDLE ) || CART_KEYDOWN( SDLK_m ) ) )
    {
        cam.x += mos.x - mos.x_old;
        cam.y += mos.y - mos.y_old;

        mos.x = mos.x_old;
        mos.y = mos.y_old;

        bound_camera( pinfo );
    }
}

//--------------------------------------------------------------------------------------------
void cartman_check_mouse_side( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    int    mpix_x, mpix_z;
    float  mpos_x, mpos_z;
    bool_t inside;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_SIDE ) ) return;

    if ( NULL == pwin->pmesh ) pwin->pmesh = &mesh;

    mpix_x = mos.x - ( pwin->x + pwin->borderx + pwin->surfacex / 2 );
    mpix_z = mos.y - ( pwin->y + pwin->bordery + pwin->surfacey / 2 );

    inside = ( mpix_x >= -( pwin->surfacex / 2 ) ) && ( mpix_x <= ( pwin->surfacex / 2 ) ) &&
             ( mpix_z >= -( pwin->surfacey / 2 ) ) && ( mpix_z <= ( pwin->surfacey / 2 ) );

    mpos_x = SCREEN_TO_REAL( mpix_x, cam.x, zoom_hrz );
    mpos_z = SCREEN_TO_REAL( mpix_z, cam.z, zoom_vrt );

    if ( pwin->id == mdata.rect_drag && !inside )
    {
        // scroll the window
        int dmpix_x = 0, dmpix_z = 0;

        if ( mpix_x < - pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x + pwin->surfacex / 2;
        }
        else if ( mpix_x > pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x - pwin->surfacex / 2;
        }

        if ( mpix_z < - pwin->surfacex / 2 )
        {
            dmpix_z = mpix_z + pwin->surfacey / 2;
        }
        else if ( mpix_z > pwin->surfacey / 2 )
        {
            dmpix_z = mpix_z - pwin->surfacey / 2;
        }

        if ( 0 != dmpix_x && 0 != dmpix_z )
        {
            cam.x += dmpix_x * FOURNUM / zoom_hrz;
            cam.z += dmpix_z * FOURNUM / zoom_vrt;

            bound_camera( &( pwin->pmesh->info ) );
        }
    }
    else if ( inside )
    {
        mdata.win_id    = pwin->id;
        mdata.win_mode  = pwin->mode;
        mdata.pmesh     = pwin->pmesh;
        mdata.xpos      = mpos_x;
        mdata.ypos      = mpos_z;
        mdata.xfan      = FLOOR( mpix_x / TILE_FSIZE );
        mdata.yfan      = -1;

        debugx = mpos_x;
        debugy = mpos_z;

        if ( MOUSE_PRESSED( SDL_BUTTON_LEFT ) )
        {
            if ( -1 == mdata.rect_drag )
            {
                mdata.rect_draw = btrue;
                mdata.rect_drag = pwin->id;
                mdata.rect_done = -1;

                mdata.rect_x0 = mdata.rect_x1 = mpos_x;
                mdata.rect_y0 = mdata.rect_y1 = mpos_z;
            }
            else if ( pwin->id == mdata.rect_drag )
            {
                mdata.rect_x1 = mpos_x;
                mdata.rect_y1 = mpos_z;
            }
        }
        else
        {
            if ( pwin->id == mdata.rect_drag )
            {
                mdata.rect_drag = -1;
                mdata.rect_done = pwin->id;
            }
        }

        if ( pwin->id == mdata.rect_done )
        {
            if ( select_count() > 0 && !CART_KEYMOD( KMOD_ALT ) && !CART_KEYDOWN( SDLK_MODE ) &&
                 !CART_KEYMOD( KMOD_LCTRL ) && !CART_KEYMOD( KMOD_RCTRL ) )
            {
                select_clear();
            }

            if ( CART_KEYMOD( KMOD_ALT ) || CART_KEYDOWN( SDLK_MODE ) )
            {
                cart_mouse_data_rect_unselect();
            }
            else
            {
                cart_mouse_data_rect_select();
            }

            mdata.rect_draw = bfalse;
            mdata.rect_drag   = -1;
            mdata.rect_done = -1;
        }

        if ( MOUSE_PRESSED( SDL_BUTTON_RIGHT ) )
        {
            move_select( pwin->pmesh, mos.cx / zoom_hrz, 0, - mos.cy / zoom_vrt );
            bound_mouse();
        }

        if ( CART_KEYDOWN( SDLK_y ) )
        {
            move_select( pwin->pmesh, 0, 0, -mos.cy / zoom_vrt );
            bound_mouse();
        }

        if ( CART_KEYDOWN( SDLK_u ) )
        {
            if ( mdata.type >= ( MPD_FAN_TYPE_MAX >> 1 ) )
            {
                move_mesh_z( mdata.pmesh, -mos.cy / zoom_vrt, mdata.tx, 0xC0 );
            }
            else
            {
                move_mesh_z( mdata.pmesh, -mos.cy / zoom_vrt, mdata.tx, 0xF0 );
            }
            bound_mouse();
        }

        if ( CART_KEYDOWN( SDLK_n ) )
        {
            if ( CART_KEYDOWN( SDLK_RSHIFT ) )
            {
                // Move the first 16 up and down
                move_mesh_z( mdata.pmesh, -mos.cy / zoom_vrt, 0, 0xF0 );
            }
            else
            {
                // Move the entire mesh up and down
                move_mesh_z( mdata.pmesh, -mos.cy / zoom_vrt, 0, 0 );
            }
            bound_mouse();
        }
    }
}

//--------------------------------------------------------------------------------------------
void cartman_check_mouse_tile( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    int fan_tmp;
    int mpix_x, mpix_y;
    float mpos_x, mpos_y;
    bool_t inside;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_TILE ) ) return;

    if ( NULL == pwin->pmesh ) pwin->pmesh = &mesh;

    mpix_x = mos.x - ( pwin->x + pwin->borderx + pwin->surfacex / 2 );
    mpix_y = mos.y - ( pwin->y + pwin->bordery + pwin->surfacey / 2 );

    inside = ( mpix_x >= -( pwin->surfacex / 2 ) ) && ( mpix_x <= ( pwin->surfacex / 2 ) ) &&
             ( mpix_y >= -( pwin->surfacey / 2 ) ) && ( mpix_y <= ( pwin->surfacey / 2 ) );

    mpos_x = SCREEN_TO_REAL( mpix_x, cam.x, zoom_hrz );
    mpos_y = SCREEN_TO_REAL( mpix_y, cam.y, zoom_hrz );

    if ( pwin->id == mdata.rect_drag && !inside )
    {
        // scroll the window
        int dmpix_x = 0, dmpix_y = 0;

        if ( mpix_x < - pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x + pwin->surfacex / 2;
        }
        else if ( mpix_x > pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x - pwin->surfacex / 2;
        }

        if ( mpix_y < - pwin->surfacex / 2 )
        {
            dmpix_y = mpix_y + pwin->surfacey / 2;
        }
        else if ( mpix_y > pwin->surfacey / 2 )
        {
            dmpix_y = mpix_y - pwin->surfacey / 2;
        }

        if ( 0 != dmpix_x && 0 != dmpix_y )
        {
            cam.x += dmpix_x  * FOURNUM / zoom_hrz;
            cam.y += dmpix_y  * FOURNUM / zoom_hrz;

            bound_camera( &( pwin->pmesh->info ) );
        }
    }
    else if ( inside )
    {
        mdata.win_id    = pwin->id;
        mdata.win_mode  = pwin->mode;
        mdata.pmesh     = pwin->pmesh;
        mdata.xpos      = mpos_x;
        mdata.ypos      = mpos_y;
        mdata.xfan      = FLOOR( mpos_x / TILE_FSIZE );
        mdata.yfan      = FLOOR( mpos_y / TILE_FSIZE );

        debugx = mpos_x;
        debugy = mpos_y;

        // update mdata.onfan only if the tile is valid
        fan_tmp = cartman_mpd_get_fan( pwin->pmesh, mdata.xfan, mdata.yfan );
        if ( -1 != fan_tmp && fan_tmp < MPD_TILE_MAX ) mdata.onfan = fan_tmp;

        if ( MOUSE_PRESSED( SDL_BUTTON_LEFT ) )
        {
            cart_mouse_data_mesh_replace_tile( CART_KEYDOWN( SDLK_t ), CART_KEYDOWN( SDLK_v ) );
        }

        if ( MOUSE_PRESSED( SDL_BUTTON_RIGHT ) )
        {
            // force an update of mdata.onfan
            mdata.onfan = fan_tmp;

            if ( mdata.onfan >= 0 && mdata.onfan < MPD_TILE_MAX )
            {
                mdata.type  = pwin->pmesh->fan[mdata.onfan].type;
                mdata.tx    = TILE_GET_LOWER_BITS( pwin->pmesh->fan[mdata.onfan].tx_bits );
                mdata.upper = TILE_GET_UPPER_BITS( pwin->pmesh->fan[mdata.onfan].tx_bits );
            }
            else
            {
                mdata.type  = 0;
                mdata.tx    = TILE_GET_LOWER_BITS( MPD_FANOFF );
                mdata.upper = TILE_GET_UPPER_BITS( MPD_FANOFF );
            }
        }

        if ( !CART_KEYDOWN( SDLK_k ) )
        {
            addinglight = bfalse;
        }
        if ( CART_KEYDOWN( SDLK_k ) && !addinglight )
        {
            add_light( mdata.xpos, mdata.ypos, MINRADIUS / zoom_hrz, MAP_MAXLEVEL / zoom_hrz );
            addinglight = btrue;
        }
        if ( addinglight )
        {
            alter_light( mdata.xpos, mdata.ypos );
        }
    }
}

//--------------------------------------------------------------------------------------------
void cartman_check_mouse_fx( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    int mpix_x, mpix_y;
    float mpos_x, mpos_y;
    bool_t inside;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_FX ) ) return;

    if ( NULL == pwin->pmesh ) pwin->pmesh = &mesh;

    mpix_x = mos.x - ( pwin->x + pwin->borderx + pwin->surfacex / 2 );
    mpix_y = mos.y - ( pwin->y + pwin->bordery + pwin->surfacey / 2 );

    inside = ( mpix_x >= -( pwin->surfacex / 2 ) ) && ( mpix_x <= ( pwin->surfacex / 2 ) ) &&
             ( mpix_y >= -( pwin->surfacey / 2 ) ) && ( mpix_y <= ( pwin->surfacey / 2 ) );

    mpos_x = SCREEN_TO_REAL( mpix_x, cam.x, zoom_hrz );
    mpos_y = SCREEN_TO_REAL( mpix_y, cam.y, zoom_hrz );

    if ( pwin->id == mdata.rect_drag && !inside )
    {
        // scroll the window
        int dmpix_x = 0, dmpix_y = 0;

        if ( mpix_x < - pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x + pwin->surfacex / 2;
        }
        else if ( mpix_x > pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x - pwin->surfacex / 2;
        }

        if ( mpix_y < - pwin->surfacex / 2 )
        {
            dmpix_y = mpix_y + pwin->surfacey / 2;
        }
        else if ( mpix_y > pwin->surfacey / 2 )
        {
            dmpix_y = mpix_y - pwin->surfacey / 2;
        }

        if ( 0 != dmpix_x && 0 != dmpix_y )
        {
            cam.x += dmpix_x * FOURNUM / zoom_hrz;
            cam.y += dmpix_y * FOURNUM / zoom_hrz;

            bound_camera( &( pwin->pmesh->info ) );
        }
    }
    else if ( inside )
    {
        int fan_tmp;

        mdata.win_id    = pwin->id;
        mdata.win_mode  = pwin->mode;
        mdata.pmesh     = pwin->pmesh;
        mdata.xpos      = mpos_x;
        mdata.ypos      = mpos_y;
        mdata.xfan      = FLOOR( mpos_x / TILE_FSIZE );
        mdata.yfan      = FLOOR( mpos_y / TILE_FSIZE );

        debugx = mpos_x;
        debugy = mpos_y;

        fan_tmp = cartman_mpd_get_fan( pwin->pmesh, mdata.xfan, mdata.yfan );
        if ( -1 != fan_tmp && fan_tmp < MPD_TILE_MAX ) mdata.onfan = fan_tmp;

        if ( MOUSE_PRESSED( SDL_BUTTON_LEFT ) )
        {
            if ( !CART_KEYDOWN( SDLK_LSHIFT ) )
            {
                cart_mouse_data_mesh_set_fx();
            }
            else
            {
                cart_mouse_data_mesh_replace_fx();
            }
        }

        if ( MOUSE_PRESSED( SDL_BUTTON_RIGHT ) )
        {
            mdata.onfan = fan_tmp;

            if ( mdata.onfan >= 0 && mdata.onfan < MPD_TILE_MAX )
            {
                mdata.fx = pwin->pmesh->fan[mdata.onfan].fx;
            }
            else
            {
                mdata.fx = MPDFX_WALL | MPDFX_IMPASS;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void cartman_check_mouse_vertex( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    int mpix_x, mpix_y;
    float mpos_x, mpos_y;
    bool_t inside;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_VERTEX ) ) return;

    if ( NULL == pwin->pmesh ) pwin->pmesh = &mesh;

    mpix_x = mos.x - ( pwin->x + pwin->surfacex / 2 );
    mpix_y = mos.y - ( pwin->y + pwin->surfacey / 2 );

    inside = ( mpix_x >= -( pwin->surfacex / 2 ) ) && ( mpix_x <= ( pwin->surfacex / 2 ) ) &&
             ( mpix_y >= -( pwin->surfacey / 2 ) ) && ( mpix_y <= ( pwin->surfacey / 2 ) );

    mpos_x = SCREEN_TO_REAL( mpix_x, cam.x, zoom_hrz );
    mpos_y = SCREEN_TO_REAL( mpix_y, cam.y, zoom_hrz );

    if ( pwin->id == mdata.rect_drag && !inside )
    {
        // scroll the window
        int dmpix_x = 0, dmpix_y = 0;

        if ( mpix_x < - pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x + pwin->surfacex / 2;
        }
        else if ( mpix_x > pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x - pwin->surfacex / 2;
        }

        if ( mpix_y < - pwin->surfacex / 2 )
        {
            dmpix_y = mpix_y + pwin->surfacey / 2;
        }
        else if ( mpix_y > pwin->surfacey / 2 )
        {
            dmpix_y = mpix_y - pwin->surfacey / 2;
        }

        if ( 0 != dmpix_x && 0 != dmpix_y )
        {
            cam.x += dmpix_x * FOURNUM / zoom_hrz;
            cam.y += dmpix_y * FOURNUM / zoom_hrz;

            bound_camera( &( pwin->pmesh->info ) );
        }
    }
    else if ( inside )
    {
        mdata.win_id    = pwin->id;
        mdata.win_mode  = pwin->mode;
        mdata.pmesh     = pwin->pmesh;
        mdata.xpos      = mpos_x;
        mdata.ypos      = mpos_y;
        mdata.xfan      = FLOOR( mpos_x / TILE_FSIZE );
        mdata.yfan      = FLOOR( mpos_y / TILE_FSIZE );

        debugx = mpos_x;
        debugy = mpos_y;

        if ( MOUSE_PRESSED( SDL_BUTTON_LEFT ) )
        {
            if ( -1 == mdata.rect_drag )
            {
                mdata.rect_draw = btrue;
                mdata.rect_drag = pwin->id;
                mdata.rect_done = -1;

                mdata.rect_x0 = mdata.rect_x1 = mpos_x;
                mdata.rect_y0 = mdata.rect_y1 = mpos_y;
            }
            else if ( pwin->id == mdata.rect_drag )
            {
                mdata.rect_x1 = mpos_x;
                mdata.rect_y1 = mpos_y;
            }
        }
        else
        {
            if ( pwin->id == mdata.rect_drag )
            {
                mdata.rect_drag = -1;
                mdata.rect_done = pwin->id;
            }
        }

        if ( pwin->id == mdata.rect_done )
        {
            if ( select_count() > 0 && !CART_KEYMOD( KMOD_ALT ) && !CART_KEYDOWN( SDLK_MODE ) &&
                 !CART_KEYMOD( KMOD_LCTRL ) && !CART_KEYMOD( KMOD_RCTRL ) )
            {
                select_clear();
            }
            if ( CART_KEYMOD( KMOD_ALT ) || CART_KEYDOWN( SDLK_MODE ) )
            {
                cart_mouse_data_rect_unselect();
            }
            else
            {
                cart_mouse_data_rect_select();
            }

            mdata.rect_draw = bfalse;
            mdata.rect_drag = -1;
            mdata.rect_done = -1;
        }

        if ( MOUSE_PRESSED( SDL_BUTTON_RIGHT ) )
        {
            move_select( pwin->pmesh, mos.cx / zoom_vrt, mos.cy / zoom_vrt, 0 );
            bound_mouse();
        }

        if ( CART_KEYDOWN( SDLK_f ) )
        {
            //    fix_corners(mdata.xpos>>7, mdata.ypos>>7);
            fix_vertices( pwin->pmesh,  FLOOR( mdata.xpos / TILE_FSIZE ), FLOOR( mdata.ypos / TILE_FSIZE ) );
        }

        if ( CART_KEYDOWN( SDLK_p ) || ( MOUSE_PRESSED( SDL_BUTTON_RIGHT ) && 0 == select_count() ) )
        {
            raise_mesh( mdata.pmesh, onscreen_vert, onscreen_count, mdata.xpos, mdata.ypos, brushamount, brushsize );
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t cartman_check_mouse( const char * modulename, cartman_mpd_t * pmesh )
{
    int cnt;

    if ( !mos.on ) return bfalse;

    if ( NULL == pmesh ) pmesh = &mesh;

    unbound_mouse();
    move_camera( &( pmesh->info ) );

    // place this after move_camera()
    update_mouse();

    // handle all window-specific commands
    //if( mos.drag && NULL != mos.drag_window )
    //{
    //    // we are dragging something in a specific window
    //    cartman_check_mouse_tile( mos.drag_window, cartman_zoom_hrz, 1.0f / 16.0f );
    //    cartman_check_mouse_vertex( mos.drag_window, cartman_zoom_hrz, 1.0f / 16.0f );
    //    cartman_check_mouse_side( mos.drag_window, cartman_zoom_hrz, 1.0f / 16.0f );
    //    cartman_check_mouse_fx( mos.drag_window, cartman_zoom_hrz, 1.0f / 16.0f );
    //}
    //else
    {
        mdata.win_id = -1;

        for ( cnt = 0; cnt < MAXWIN; cnt++ )
        {
            window_t * pwin = window_lst + cnt;

            cartman_check_mouse_tile( pwin, cartman_zoom_hrz, 1.0f / 16.0f );
            cartman_check_mouse_vertex( pwin, cartman_zoom_hrz, 1.0f / 16.0f );
            cartman_check_mouse_side( pwin, cartman_zoom_hrz, 1.0f / 16.0f );
            cartman_check_mouse_fx( pwin, cartman_zoom_hrz, 1.0f / 16.0f );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void ease_up_mesh( cartman_mpd_t * pmesh, float zoom_vrt )
{
    // ZZ> This function lifts the entire mesh

    if ( NULL == pmesh ) pmesh = &mesh;

    mos.y = mos.y_old;
    mos.x = mos.x_old;

    mesh_move( pmesh, 0, 0, -mos.cy / zoom_vrt );
}

//--------------------------------------------------------------------------------------------
bool_t cartman_check_keys( const char * modname, cartman_mpd_t * pmesh )
{
    if ( !check_keys( 20 ) ) return bfalse;

    if ( NULL == pmesh ) pmesh = &mesh;

    // Hurt
    if ( CART_KEYDOWN( SDLK_h ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_DAMAGE );
        key.delay = KEYDELAY;
    }
    // Impassable
    if ( CART_KEYDOWN( SDLK_i ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_IMPASS );
        key.delay = KEYDELAY;
    }
    // Barrier
    if ( CART_KEYDOWN( SDLK_b ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_WALL );
        key.delay = KEYDELAY;
    }
    // Overlay
    if ( CART_KEYDOWN( SDLK_o ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_WATER );
        key.delay = KEYDELAY;
    }
    // Reflective
    if ( CART_KEYDOWN( SDLK_r ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_SHA );
        key.delay = KEYDELAY;
    }
    // Draw reflections
    if ( CART_KEYDOWN( SDLK_d ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_DRAWREF );
        key.delay = KEYDELAY;
    }
    // Animated
    if ( CART_KEYDOWN( SDLK_a ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_ANIM );
        key.delay = KEYDELAY;
    }
    // Slippy
    if ( CART_KEYDOWN( SDLK_s ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_SLIPPY );
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_g ) )
    {
        fix_mesh( pmesh );
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_z ) )
    {
        if ( mdata.onfan >= 0 || mdata.onfan < MPD_TILE_MAX )
        {
            Uint16 tx_bits = pmesh->fan[mdata.onfan].tx_bits;
            cart_mouse_data_mesh_set_tile( tx_bits );
        }
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_x ) )
    {
        if ( mdata.onfan >= 0 || mdata.onfan < MPD_TILE_MAX )
        {
            Uint8  type    = pmesh->fan[mdata.onfan].type;
            Uint16 tx_bits = pmesh->fan[mdata.onfan].tx_bits;

            if ( type >= ( MPD_FAN_TYPE_MAX >> 1 ) )
            {
                trim_mesh_tile( pmesh, tx_bits, 0xC0 );
            }
            else
            {
                trim_mesh_tile( pmesh, tx_bits, 0xF0 );
            }
        }

        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_e ) )
    {
        ease_up_mesh( pmesh, cartman_zoom_vrt );
    }

    if ( CART_KEYDOWN( SDLK_LEFTBRACKET ) || CART_KEYDOWN( SDLK_RIGHTBRACKET ) )
    {
        select_verts_connected( pmesh );
    }
    if ( CART_KEYDOWN( SDLK_8 ) )
    {
        cart_mouse_data_three_e_mesh();
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_j ) )
    {
        if ( 0 == select_count() ) { jitter_mesh( pmesh ); }
        else { mesh_select_jitter( pmesh ); }
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_w ) )
    {
        //impass_edges(2);
        mesh_calc_vrta( pmesh );
        cartman_save_mesh( modname, pmesh );
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_SPACE ) )
    {
        mesh_select_weld( pmesh );
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_INSERT ) )
    {
        mdata.type = ( mdata.type - 1 ) % MPD_FAN_TYPE_MAX;
        while ( 0 == tile_dict_lines[mdata.type].count )
        {
            mdata.type = ( mdata.type - 1 ) % MPD_FAN_TYPE_MAX;
        }
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_DELETE ) )
    {
        mdata.type = ( mdata.type + 1 ) % MPD_FAN_TYPE_MAX;
        while ( 0 == tile_dict_lines[mdata.type].count )
        {
            mdata.type = ( mdata.type + 1 ) % MPD_FAN_TYPE_MAX;
        }
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_KP_PLUS ) )
    {
        mdata.tx = ( mdata.tx + 1 ) & 0xFF;
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_KP_MINUS ) )
    {
        mdata.tx = ( mdata.tx - 1 ) & 0xFF;
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_UP ) || CART_KEYDOWN( SDLK_LEFT ) || CART_KEYDOWN( SDLK_DOWN ) || CART_KEYDOWN( SDLK_RIGHT ) )
    {
        if ( CART_KEYDOWN( SDLK_RIGHT ) )
        {
            cam.x += 8 * CAMRATE;
        }
        if ( CART_KEYDOWN( SDLK_LEFT ) )
        {
            cam.x -= 8 * CAMRATE;
        }

        if ( WINMODE_SIDE == mdata.win_mode )
        {
            if ( CART_KEYDOWN( SDLK_DOWN ) )
            {
                cam.z += 8 * CAMRATE * ( pmesh->info.edgez / DEFAULT_Z_SIZE );
            }
            if ( CART_KEYDOWN( SDLK_UP ) )
            {
                cam.z -= 8 * CAMRATE * ( pmesh->info.edgez / DEFAULT_Z_SIZE );
            }
        }
        else
        {
            if ( CART_KEYDOWN( SDLK_DOWN ) )
            {
                cam.y += 8 * CAMRATE;
            }
            if ( CART_KEYDOWN( SDLK_UP ) )
            {
                cam.y -= 8 * CAMRATE;
            }
        }
        bound_camera( &( pmesh->info ) );
    }

    if ( CART_KEYDOWN( SDLK_PLUS ) || CART_KEYDOWN( SDLK_EQUALS ) )
    {
        cartman_zoom_hrz *= 2;
        if ( cartman_zoom_hrz > 4 )
        {
            cartman_zoom_hrz = 4;
        }
        else
        {
            key.delay = KEYDELAY;
        }
    }

    if ( CART_KEYDOWN( SDLK_MINUS ) || CART_KEYDOWN( SDLK_UNDERSCORE ) )
    {
        cartman_zoom_hrz /= 2;
        if ( cartman_zoom_hrz < 0.25f )
        {
            cartman_zoom_hrz = 0.25f;
        }
        else
        {
            key.delay = KEYDELAY;
        }
    }

    //------------------
    // from cartman_check_mouse_side() and cartman_check_mouse_tile() functions
    if ( CART_KEYDOWN( SDLK_f ) )
    {
        cart_mouse_data_flatten_mesh();
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_q ) )
    {
        fix_walls( pmesh );
        key.delay = KEYDELAY;
    }

    //------------------
    // "fixed" jeys

    if ( CART_KEYDOWN( SDLK_5 ) )
    {
        mesh_select_set_z_no_bound( pmesh, -8000 * 4 );
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_6 ) )
    {
        mesh_select_set_z_no_bound( pmesh, -127 * 4 );
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_7 ) )
    {
        mesh_select_set_z_no_bound( pmesh, 127 * 4 );
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN_MOD( SDLK_c, KMOD_SHIFT ) )
    {
        cart_mouse_data_clear_mesh();
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN_MOD( SDLK_l, KMOD_SHIFT ) )
    {
        level_vrtz( pmesh );
    }

    // brush size
    if ( CART_KEYDOWN( SDLK_END ) || CART_KEYDOWN( SDLK_KP1 ) )
    {
        brushsize = 0;
    }
    if ( CART_KEYDOWN( SDLK_PAGEDOWN ) || CART_KEYDOWN( SDLK_KP3 ) )
    {
        brushsize = 1;
    }
    if ( CART_KEYDOWN( SDLK_HOME ) || CART_KEYDOWN( SDLK_KP7 ) )
    {
        brushsize = 2;
    }
    if ( CART_KEYDOWN( SDLK_PAGEUP ) || CART_KEYDOWN( SDLK_KP9 ) )
    {
        brushsize = 3;
    }

    // presser
    if ( CART_KEYDOWN( SDLK_1 ) )
    {
        mdata.presser = 0;
    }
    if ( CART_KEYDOWN( SDLK_2 ) )
    {
        mdata.presser = 1;
    }
    if ( CART_KEYDOWN( SDLK_3 ) )
    {
        mdata.presser = 2;
    }
    if ( CART_KEYDOWN( SDLK_4 ) )
    {
        mdata.presser = 3;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t check_input_mouse( SDL_Event * pevt )
{
    bool_t handled = bfalse;

    if ( NULL == pevt || !mos.on ) return bfalse;

    if ( 0 == mos.b )
    {
        mos.drag = bfalse;
        mos.drag_begin = bfalse;

        // set mdata??
    }

    switch ( pevt->type )
    {
        case SDL_MOUSEBUTTONDOWN:
            switch ( pevt->button.button )
            {
                case SDL_BUTTON_LEFT:
                    mos.b |= SDL_BUTTON( SDL_BUTTON_LEFT );
                    break;

                case SDL_BUTTON_MIDDLE:
                    mos.b |= SDL_BUTTON( SDL_BUTTON_MIDDLE );
                    break;

                case SDL_BUTTON_RIGHT:
                    mos.b |= SDL_BUTTON( SDL_BUTTON_RIGHT );
                    break;
            }
            ui.pending_click = btrue;
            handled = btrue;
            break;

        case SDL_MOUSEBUTTONUP:
            switch ( pevt->button.button )
            {
                case SDL_BUTTON_LEFT:
                    mos.b &= ~SDL_BUTTON( SDL_BUTTON_LEFT );
                    break;

                case SDL_BUTTON_MIDDLE:
                    mos.b &= ~SDL_BUTTON( SDL_BUTTON_MIDDLE );
                    break;

                case SDL_BUTTON_RIGHT:
                    mos.b &= ~SDL_BUTTON( SDL_BUTTON_RIGHT );
                    break;
            }
            ui.pending_click = bfalse;
            handled = btrue;
            break;

        case SDL_MOUSEMOTION:
            mos.b = pevt->motion.state;
            if ( mos.drag )
            {
                if ( 0 != mos.b )
                {
                    mos.brx = pevt->motion.x;
                    mos.bry = pevt->motion.y;
                }
                else
                {
                    mos.drag = bfalse;
                }
            }

            if ( mos.relative )
            {
                mos.cx = pevt->motion.xrel;
                mos.cy = pevt->motion.yrel;
            }
            else
            {
                mos.x = pevt->motion.x;
                mos.y = pevt->motion.y;
            }
            break;
    }

    if ( 0 != mos.b )
    {
        if ( mos.drag_begin )
        {
            // start dragging
            mos.drag = btrue;
        }
        else if ( !mos.drag )
        {
            // set the dragging to begin the next mouse time the mouse moves
            mos.drag_begin = btrue;

            // initialize the drag rect
            mos.tlx = mos.x;
            mos.tly = mos.y;

            mos.brx = mos.x;
            mos.bry = mos.y;

            // set the drag window
            mos.drag_window = find_window( mos.x, mos.y );
            mos.drag_mode   = ( NULL == mos.drag_window ) ? 0 : mos.drag_mode;
        }
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
bool_t check_input_keyboard( SDL_Event * pevt )
{
    bool_t handled = bfalse;

    if ( NULL == pevt || !key.on ) return bfalse;

    switch ( pevt->type )
    {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            key.state = pevt->key.state;
            key.needs_update = btrue;
            handled = btrue;
            break;
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
void draw_lotsa_stuff( cartman_mpd_t * pmesh )
{
    int x, cnt, todo, tile, add;

    if ( NULL == pmesh ) pmesh = &mesh;

#if defined(_DEBUG)
    // Tell which tile we're in
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, 226, NULL,
                      "X = %6.2f", debugx );
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, 234, NULL,
                      "Y = %6.2f", debugy );
#endif

    // Tell user what keys are important
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, sdl_scr.y - 120, NULL,
                      "O = Overlay (Water)" );
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, sdl_scr.y - 112, NULL,
                      "R = Reflective" );
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, sdl_scr.y - 104, NULL,
                      "D = Draw Reflection" );
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, sdl_scr.y - 96, NULL,
                      "A = Animated" );
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, sdl_scr.y - 88, NULL,
                      "B = Barrier (Slit)" );
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, sdl_scr.y - 80, NULL,
                      "I = Impassable (Wall)" );
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, sdl_scr.y - 72, NULL,
                      "H = Hurt" );
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, sdl_scr.y - 64, NULL,
                      "S = Slippy" );

    // Vertices left
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, sdl_scr.y - 56, NULL,
                      "Vertices %d", pmesh->vrt_free );

    // Misc data
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, sdl_scr.y - 40, NULL,
                      "Ambient   %d", ambi );
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, sdl_scr.y - 32, NULL,
                      "Ambicut   %d", ambicut );
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, sdl_scr.y - 24, NULL,
                      "Direct    %d", direct );
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, sdl_scr.y - 16, NULL,
                      "Brush amount %d", brushamount );
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, sdl_scr.y - 8, NULL,
                      "Brush size   %d", brushsize );

    // Cursor
    //if (mos.x >= 0 && mos.x < sdl_scr.x && mos.y >= 0 && mos.y < sdl_scr.y)
    //{
    //    draw_sprite(theSurface, bmpcursor, mos.x, mos.y);
    //}

    // Tile picks
    todo = 0;
    tile = 0;
    add  = 1;
    if ( mdata.tx < MAXTILE )
    {
        switch ( mdata.presser )
        {
            case 0:
                todo = 1;
                tile = mdata.tx;
                add = 1;
                break;
            case 1:
                todo = 2;
                tile = mdata.tx & 0xFE;
                add = 1;
                break;
            case 2:
                todo = 4;
                tile = mdata.tx & 0xFC;
                add = 1;
                break;
            case 3:
                todo = 4;
                tile = mdata.tx & 0xF8;
                add = 2;
                break;
        }

        x = 0;
        for ( cnt = 0; cnt < todo; cnt++ )
        {
            if ( mdata.type >= ( MPD_FAN_TYPE_MAX >> 1 ) )
            {
                ogl_draw_sprite_2d( tx_bigtile + tile, x, 0, SMALLXY, SMALLXY );
            }
            else
            {
                ogl_draw_sprite_2d( tx_smalltile + tile, x, 0, SMALLXY, SMALLXY );
            }
            x += SMALLXY;
            tile += add;
        }

        fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, 32, NULL,
                          "Tile 0x%02x 0x%02x", mdata.upper, mdata.tx );
        fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, 40, NULL,
                          "Eats %d verts", tile_dict[mdata.type].numvertices );
        if ( mdata.type >= ( MPD_FAN_TYPE_MAX >> 1 ) )
        {
            fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, 56, NULL,
                              "63x63 Tile" );
        }
        else
        {
            fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, 56, NULL,
                              "31x31 Tile" );
        }
        draw_schematic( NULL, mdata.type, 0, 64 );
    }

    // FX selection
    draw_tile_fx( 0, 193, mdata.fx, 1.0f );

    if ( numattempt > 0 )
    {
        fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, 0, NULL,
                          "numwritten %d/%d", numwritten, numattempt );
    }

#if defined(_DEBUG)
    fnt_drawText_OGL( gfx_font_ptr, cart_white, 0, 0, NULL,
                      "<%f, %f>", mos.x, mos.y );
#endif

}

//--------------------------------------------------------------------------------------------
void draw_main( cartman_mpd_t * pmesh )
{
    bool_t recalc_lighting = bfalse;

    if ( NULL == pmesh ) pmesh = &mesh;

    glClear( GL_COLOR_BUFFER_BIT );

    ogl_beginFrame();
    {
        int itmp;

        render_all_windows();

        draw_all_windows();

        itmp = ambi;
        draw_slider( 0, 250, 19, 350, &ambi,          0, 200 );
        if ( itmp != ambi ) recalc_lighting = btrue;

        itmp = ambicut;
        draw_slider( 20, 250, 39, 350, &ambicut,       0, ambi );
        if ( itmp != ambicut ) recalc_lighting = btrue;

        itmp = direct;
        draw_slider( 40, 250, 59, 350, &direct,        0, 100 );
        if ( itmp != direct ) recalc_lighting = btrue;

        draw_slider( 60, 250, 79, 350, &brushamount, -50,  50 );

        draw_lotsa_stuff( pmesh );
    }
    ogl_endFrame();

    if ( recalc_lighting )
    {
        mesh_calc_vrta( pmesh );
    }

    dunframe++;
    secframe++;

    SDL_GL_SwapBuffers();
}

//--------------------------------------------------------------------------------------------
void main_end( void )
{
    // Ending statistics
    show_info();

    // end the graphics system
    gfx_system_end();

    // end the graphics system
    setup_clear_base_vfs_paths();
}

//--------------------------------------------------------------------------------------------
int main( int argcnt, char* argtext[] )
{
    char modulename[100];
    STRING fname;

    // char *blah[3];

    //blah[0] = malloc(256); strcpy(blah[0], "");
    //blah[1] = malloc(256); strcpy(blah[1], "/home/bgbirdsey/egoboo");
    //blah[2] = malloc(256); strcpy(blah[2], "advent" );

    //argcnt = 3;
    //argtext = blah;

    // register the function to be called to deinitialize the program
    atexit( main_end );

    // construct some global variables
    mouse_ctor( &mos );
    keyboard_ctor( &key );
    cart_mouse_data_ctor( &mdata );

    // initial text for the console
    show_info();

    // grab the egoboo directory and the module name from the command line
    if ( argcnt < 2 || argcnt > 3 )
    {
        printf( "USAGE: CARTMAN [PATH] MODULE ( without .MOD )\n\n" );
        exit( 0 );
    }
    else if ( argcnt < 3 )
    {
        sprintf( egoboo_path, "%s", "." );
        sprintf( modulename, "%s.mod", argtext[1] );
    }
    else if ( argcnt < 4 )
    {
        size_t len = strlen( argtext[1] );
        char * pstr = argtext[1];
        if ( pstr[0] == '\"' )
        {
            pstr[len-1] = '\0';
            pstr++;
        }
        sprintf( egoboo_path, "%s", pstr );
        sprintf( modulename, "%s.mod", argtext[2] );
    }

    // initialize the virtual file system
    vfs_init( egoboo_path );
    setup_init_base_vfs_paths();

    // register the logging code
    log_init( vfs_resolveWriteFilename( "/debug/log.txt" ) );
    log_setLoggingLevel( 2 );
    atexit( log_shutdown );

    if ( !setup_read_vfs() )
    {
        log_error( "Cannot load the setup file \"%s\".\n", fname );
    }
    config_download( &cfg );

    // initialize the SDL elements
    cartman_init_SDL_base();
    gfx_system_begin();

    make_randie();                      // Random number table

    // Load the module
    if ( !load_module( modulename, &mesh ) )
    {
        log_error( "%s - cannot load module %s.\n", __FUNCTION__, modulename );
    }

    fill_fpstext();                     // Make the FPS text
    load_all_windows( &mesh );          // Load windows
    create_imgcursor();                 // Make cursor image
    load_img();                         // Load cartman icons

    dunframe   = 0;                     // Timer resets
    worldclock = 0;
    timclock   = 0;
    while ( btrue )  // Main loop
    {
        if ( CART_KEYDOWN( SDLK_ESCAPE ) || CART_KEYDOWN( SDLK_F1 ) ) break;

        cartman_check_input( modulename, &mesh );

        draw_main( &mesh );

        SDL_Delay( 1 );

        timclock = SDL_GetTicks() >> 3;
    }

    exit( 0 );                      // End
}

//--------------------------------------------------------------------------------------------
static bool_t _sdl_initialized_base = bfalse;
void cartman_init_SDL_base()
{
    if ( _sdl_initialized_base ) return;

    log_info( "Initializing SDL version %d.%d.%d... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
    if ( SDL_Init( 0 ) < 0 )
    {
        log_message( "Failure!\n" );
        log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    if ( !_sdl_atexit_registered )
    {
        atexit( SDL_Quit );
        _sdl_atexit_registered = bfalse;
    }

    log_info( "Intializing SDL Timing Services... " );
    if ( SDL_InitSubSystem( SDL_INIT_TIMER ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    log_info( "Intializing SDL Event Threading... " );
    if ( SDL_InitSubSystem( SDL_INIT_EVENTTHREAD ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    _sdl_initialized_base = btrue;
}

//--------------------------------------------------------------------------------------------
void cartman_create_mesh( cartman_mpd_t * pmesh )
{
    int mapx, mapy, fan;
    int x, y;

    cartman_mpd_create_info_t mpd_info;

    if ( NULL == pmesh ) pmesh = &mesh;

    printf( "Mesh file not found, so creating a new one...\n" );

    printf( "Number of tiles in X direction ( 32-512 ):  " );
    scanf( "%d", &( mpd_info.tiles_x ) );

    printf( "Number of tiles in Y direction ( 32-512 ):  " );
    scanf( "%d", &( mpd_info.tiles_y ) );

    cartman_mpd_create( pmesh, mpd_info.tiles_x, mpd_info.tiles_y );

    fan = 0;
    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        y = mapy * TILE_ISIZE;
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            x = mapx * TILE_ISIZE;
            if ( !cartman_mpd_add_fan( pmesh, fan, x, y ) )
            {
                printf( "NOT ENOUGH VERTICES!!!\n\n" );
                exit( -1 );
            }

            fan++;
        }
    }

    fix_mesh( pmesh );
}

//--------------------------------------------------------------------------------------------
void cartman_save_mesh( const char * modname, cartman_mpd_t * pmesh )
{
    STRING newloadname;

    if ( NULL == pmesh ) pmesh = &mesh;

    numwritten = 0;
    numattempt = 0;

    sprintf( newloadname, "%s" SLASH_STR "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "plan.bmp", egoboo_path, modname );

    make_planmap( pmesh );
    if ( bmphitemap )
    {
        SDL_SaveBMP( bmphitemap, newloadname );
    }

    //  make_newloadname(modname, SLASH_STR "gamedat" SLASH_STR "level.png", newloadname);
    //  make_hitemap();
    //  if(bmphitemap)
    //  {
    //    make_graypal();
    //    save_pcx(newloadname, bmphitemap);
    //  }

    cartman_mpd_save_vfs( /*modname,*/ pmesh );

    show_name( newloadname, cart_white );
}

//--------------------------------------------------------------------------------------------
void cartman_check_input( const char * modulename, cartman_mpd_t * pmesh )
{
    if ( NULL == pmesh ) pmesh = &mesh;

    debugx = -1;
    debugy = -1;

    check_input();

    cartman_check_mouse( modulename, pmesh );
    cartman_check_keys( modulename, pmesh );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t config_download( egoboo_config_t * pcfg )
{
    return setup_download( pcfg );
}

//--------------------------------------------------------------------------------------------
bool_t config_upload( egoboo_config_t * pcfg )
{
    return setup_upload( pcfg );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
cart_mouse_data_t * cart_mouse_data_ctor( cart_mouse_data_t * ptr )
{
    if ( NULL == ptr ) return NULL;

    memset( ptr, 0, sizeof( *ptr ) );

    ptr->win_id   = -1;
    ptr->win_mode = ( Uint16 )( ~0 );
    ptr->xfan     = -1;
    ptr->yfan     = -1;

    ptr->onfan = -1;
    ptr->fx = MPDFX_SHA;

    ptr->rect_drag = -1;
    ptr->rect_done = -1;

    return ptr;
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_mesh_set_tile( Uint16 tiletoset )
{
    mesh_set_tile( mdata.pmesh, tiletoset, mdata.upper, mdata.presser, mdata.tx );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_flatten_mesh()
{
    flatten_mesh( mdata.pmesh, mdata.ypos );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_clear_mesh()
{
    clear_mesh( mdata.pmesh,  mdata.upper, mdata.presser, mdata.tx, mdata.type );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_three_e_mesh()
{
    three_e_mesh( mdata.pmesh, mdata.upper, mdata.tx );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_mesh_replace_tile( bool_t tx_only, bool_t at_floor_level )
{
    mesh_replace_tile( mdata.pmesh, mdata.xfan, mdata.yfan, mdata.onfan, mdata.tx, mdata.upper, mdata.fx, mdata.type, mdata.presser, tx_only, at_floor_level );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_mesh_set_fx()
{
    mesh_set_fx( mdata.pmesh, mdata.onfan, mdata.fx );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_toggle_fx( int fxmask )
{
    mdata.fx ^= fxmask;
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_rect_select()
{
    select_add_rect( mdata.pmesh, mdata.rect_x0, mdata.rect_y0, mdata.rect_x1, mdata.rect_y1, mdata.win_mode );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_rect_unselect()
{
    select_add_rect( mdata.pmesh, mdata.rect_x0, mdata.rect_y0, mdata.rect_x1, mdata.rect_y1, mdata.win_mode );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_mesh_replace_fx()
{
    Uint8  type;
    Uint16 tx_bits;

    if ( mdata.onfan < 0 || mdata.onfan >= MPD_TILE_MAX ) return;

    type = mdata.pmesh->fan[mdata.onfan].type;
    if ( type >= MPD_FAN_TYPE_MAX ) return;

    tx_bits = mdata.pmesh->fan[mdata.onfan].tx_bits;
    if ( TILE_IS_FANOFF( tx_bits ) ) return;

    if ( type >= ( MPD_FAN_TYPE_MAX >> 1 ) )
    {
        mesh_replace_fx( mdata.pmesh, tx_bits, 0xC0, mdata.fx );
    }
    else
    {
        mesh_replace_fx( mdata.pmesh, tx_bits, 0xF0, mdata.fx );
    }
}
