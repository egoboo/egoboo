#include "cartman_gfx.h"

#include "cartman.h"
#include "cartman_mpd.h"
#include "cartman_gui.h"
#include "cartman_functions.h"
#include "cartman_math.inl"

#include "SDL_Pixel.h"

#include <egolib.h>

#include <SDL_image.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static SDL_bool  _sdl_initialized_graphics = SDL_FALSE;
static GLboolean _ogl_initialized = GL_FALSE;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

const SDL_Color cart_white = { 0xFF, 0xFF, 0xFF, 0xFF };
const SDL_Color cart_black = { 0x00, 0x00, 0x00, 0xFF };

Font * gfx_font_ptr = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

camera_t cam;

Sint16          damagetileparttype;
short           damagetilepartand;
short           damagetilesound;
short           damagetilesoundtime;
Uint16          damagetilemindistance;
int             damagetileamount = 256;                           // Amount of damage
Uint8           damagetiletype  = DAMAGE_FIRE;                      // Type of damage

int    animtileupdateand   = 7;
Uint16 animtileframeand    = 3;
Uint16 animtilebaseand     = ( Uint16 )( ~3 );
Uint16 biganimtileframeand = 7;
Uint16 biganimtilebaseand  = ( Uint16 )( ~7 );
Uint16 animtileframeadd    = 0;

SDLX_video_parameters_t sdl_vparam;
oglx_video_parameters_t ogl_vparam;

SDL_Surface * theSurface = NULL;
SDL_Surface * bmphitemap = NULL;        // Heightmap image

oglx_texture_t     tx_point;      // Vertex image
oglx_texture_t     tx_pointon;    // Vertex image ( select_vertsed )
oglx_texture_t     tx_ref;        // Meshfx images
oglx_texture_t     tx_drawref;    //
oglx_texture_t     tx_anim;       //
oglx_texture_t     tx_water;      //
oglx_texture_t     tx_wall;       //
oglx_texture_t     tx_impass;     //
oglx_texture_t     tx_damage;     //
oglx_texture_t     tx_slippy;     //

oglx_texture_t     tx_smalltile[MAXTILE]; // Tiles
oglx_texture_t     tx_bigtile[MAXTILE];   //
oglx_texture_t     tx_tinysmalltile[MAXTILE]; // Plan tiles
oglx_texture_t     tx_tinybigtile[MAXTILE];   //

int     numsmalltile = 0;   //
int     numbigtile = 0;     //

int GFX_WIDTH  = 800;
int GFX_HEIGHT = 600;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void get_small_tiles( SDL_Surface* bmpload );
static void get_big_tiles( SDL_Surface* bmpload );
static void gfx_init_SDL_graphics();
static int  gfx_init_ogl();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void gfx_system_begin()
{
    // set the graphics state
    gfx_init_SDL_graphics();
    gfx_init_ogl();

    theSurface = SDL_GetVideoSurface();

    gfx_font_ptr = fnt_loadFont( "data" SLASH_STR "pc8x8.fon", 12 );
}

//--------------------------------------------------------------------------------------------
void gfx_system_end()
{
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

SDL_Color MAKE_SDLCOLOR( Uint8 BB, Uint8 RR, Uint8 GG )
{
    SDL_Color tmp;

    tmp.r = RR << 3;
    tmp.g = GG << 3;
    tmp.b = BB << 3;

    return tmp;
}

//--------------------------------------------------------------------------------------------
oglx_texture_t * tiny_tile_at( cartman_mpd_t * pmesh, int x, int y )
{
    Uint16 tx_bits, basetile;
    Uint8 fantype, fx;
    int fan;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( x < 0 || x >= pmesh->info.tiles_x || y < 0 || y >= pmesh->info.tiles_y )
    {
        return NULL;
    }

    tx_bits = 0;
    fantype = 0;
    fx = 0;
    fan = cartman_mpd_get_fan( pmesh, x, y );
    if ( fan >= 0 && fan < MPD_TILE_MAX )
    {
        if ( TILE_IS_FANOFF( pmesh->fan[fan].tx_bits ) )
        {
            return NULL;
        }
        tx_bits = pmesh->fan[fan].tx_bits;
        fantype = pmesh->fan[fan].type;
        fx      = pmesh->fan[fan].fx;
    }

    if ( HAS_BITS( fx, MPDFX_ANIM ) )
    {
        animtileframeadd = ( timclock >> 3 ) & 3;
        if ( fantype >= ( MPD_FAN_TYPE_MAX >> 1 ) )
        {
            // Big tiles
            basetile = tx_bits & biganimtilebaseand;// Animation set
            tx_bits += ( animtileframeadd << 1 );   // Animated tx_bits
            tx_bits = ( tx_bits & biganimtileframeand ) + basetile;
        }
        else
        {
            // Small tiles
            basetile = tx_bits & animtilebaseand;// Animation set
            tx_bits += animtileframeadd;       // Animated tx_bits
            tx_bits = ( tx_bits & animtileframeand ) + basetile;
        }
    }

    // remove any of the upper bit information
    tx_bits &= 0xFF;

    if ( fantype >= ( MPD_FAN_TYPE_MAX >> 1 ) )
    {
        return tx_bigtile + tx_bits;
    }
    else
    {
        return tx_smalltile + tx_bits;
    }
}

//--------------------------------------------------------------------------------------------
oglx_texture_t *tile_at( cartman_mpd_t * pmesh, int fan )
{
    Uint16 tx_bits, basetile;
    Uint8 fantype, fx;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( fan == -1 || TILE_IS_FANOFF( pmesh->fan[fan].tx_bits ) )
    {
        return NULL;
    }

    tx_bits = pmesh->fan[fan].tx_bits;
    fantype = pmesh->fan[fan].type;
    fx = pmesh->fan[fan].fx;
    if ( HAS_BITS( fx, MPDFX_ANIM ) )
    {
        animtileframeadd = ( timclock >> 3 ) & 3;
        if ( fantype >= ( MPD_FAN_TYPE_MAX >> 1 ) )
        {
            // Big tiles
            basetile = tx_bits & biganimtilebaseand;// Animation set
            tx_bits += ( animtileframeadd << 1 );   // Animated tx_bits
            tx_bits = ( tx_bits & biganimtileframeand ) + basetile;
        }
        else
        {
            // Small tiles
            basetile = tx_bits & animtilebaseand;  // Animation set
            tx_bits += animtileframeadd;           // Animated tx_bits
            tx_bits = ( tx_bits & animtileframeand ) + basetile;
        }
    }

    // remove any of the upper bit information
    tx_bits &= 0xFF;

    if ( fantype >= ( MPD_FAN_TYPE_MAX >> 1 ) )
    {
        return tx_bigtile + tx_bits;
    }
    else
    {
        return tx_smalltile + tx_bits;
    }
}

//--------------------------------------------------------------------------------------------
void make_hitemap( cartman_mpd_t * pmesh )
{
    int x, y, pixx, pixy, level, fan;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( bmphitemap ) SDL_FreeSurface( bmphitemap );

    bmphitemap = cartman_CreateSurface( pmesh->info.tiles_x << 2, pmesh->info.tiles_y << 2 );
    if ( NULL == bmphitemap ) return;

    y = 16;
    pixy = 0;
    while ( pixy < ( pmesh->info.tiles_y << 2 ) )
    {
        x = 16;
        pixx = 0;
        while ( pixx < ( pmesh->info.tiles_x << 2 ) )
        {
            level = ( cartman_mpd_get_level( pmesh, x, y ) * 255 / pmesh->info.edgez );  // level is 0 to 255
            if ( level > 252 ) level = 252;

            fan = cartman_mpd_get_fan( pmesh,  pixx >> 2, pixy );
            if ( fan >= 0 && fan < MPD_TILE_MAX )
            {
                if ( HAS_BITS( pmesh->fan[fan].fx, MPDFX_WALL ) ) level = 253;  // Wall
                if ( HAS_BITS( pmesh->fan[fan].fx, MPDFX_IMPASS ) ) level = 254;   // Impass
                if ( HAS_BITS( pmesh->fan[fan].fx, MPDFX_WALL ) &&
                     HAS_BITS( pmesh->fan[fan].fx, MPDFX_IMPASS ) ) level = 255;   // Both
            }

            SDL_PutPixel( bmphitemap, pixx, pixy, level );
            x += 32;
            pixx++;
        }
        y += 32;
        pixy++;
    }
}

//--------------------------------------------------------------------------------------------
void make_planmap( cartman_mpd_t * pmesh )
{
    int x, y, putx, puty;
    //SDL_Surface* bmptemp;

    //bmptemp = cartman_CreateSurface(64, 64);
    //if(NULL != bmptemp)  return;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( NULL == bmphitemap ) SDL_FreeSurface( bmphitemap );
    bmphitemap = cartman_CreateSurface( pmesh->info.tiles_x * TINYXY, pmesh->info.tiles_y * TINYXY );
    if ( NULL == bmphitemap ) return;

    SDL_FillRect( bmphitemap, NULL, MAKE_BGR( bmphitemap, 0, 0, 0 ) );

    puty = 0;
    for ( y = 0; y < pmesh->info.tiles_y; y++ )
    {
        putx = 0;
        for ( x = 0; x < pmesh->info.tiles_x; x++ )
        {
            oglx_texture_t * tx_tile;
            tx_tile = tiny_tile_at( pmesh, x, y );

            if ( NULL != tx_tile )
            {
                SDL_Rect dst = {putx, puty, TINYXY, TINYXY};
                cartman_BlitSurface( tx_tile->surface, NULL, bmphitemap, &dst );
            }
            putx += TINYXY;
        }
        puty += TINYXY;
    }

    //SDL_SoftStretch(bmphitemap, NULL, bmptemp, NULL);
    //SDL_FreeSurface(bmphitemap);

    //bmphitemap = bmptemp;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void draw_top_fan( window_t * pwin, int fan, float zoom_hrz )
{
    // ZZ> This function draws the line drawing preview of the tile type...
    //     A wireframe tile from a vertex connection window

    static Uint32 faketoreal[MPD_FAN_VERTICES_MAX];

    int cnt, stt, end, vert;
    float color[4];
    float size;

    cart_vec_t vup    = { 0, 1, 0};
    cart_vec_t vright = { -1, 0, 0};
    cart_vec_t vpos;

    // aliases
    tile_definition_t    * pdef   = NULL;
    cartman_mpd_tile_t   * pfan   = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;
    tile_line_data_t     * plines = NULL;

    if ( NULL == pwin->pmesh ) pwin->pmesh = &mesh;
    vlst = pwin->pmesh->vrt;

    if ( fan < 0 || fan >= MPD_TILE_MAX ) return;
    pfan = pwin->pmesh->fan + fan;

    if ( pfan->type >= MPD_FAN_TYPE_MAX )
    {
        return;
    }
    pdef = tile_dict + pfan->type;
    plines = tile_dict_lines + pfan->type;

    if ( 0 == pdef->numvertices || pdef->numvertices > MPD_FAN_VERTICES_MAX ) return;

    OGL_MAKE_COLOR_4( color, 32, 16, 16, 31 );
    if ( pfan->type >= MPD_FAN_TYPE_MAX / 2 )
    {
        OGL_MAKE_COLOR_4( color, 32, 31, 16, 16 );
    }

    for ( cnt = 0, vert = pfan->vrtstart;
          cnt < pdef->numvertices && CHAINEND != vert;
          cnt++, vert = vlst[vert].next )
    {
        faketoreal[cnt] = vert;
    }

    glPushAttrib( GL_TEXTURE_BIT | GL_ENABLE_BIT );
    {
        glColor4fv( color );
        glDisable( GL_TEXTURE_2D );

        glBegin( GL_LINES );
        {
            for ( cnt = 0; cnt < plines->count; cnt++ )
            {
                stt = faketoreal[plines->start[cnt]];
                end = faketoreal[plines->end[cnt]];

                glVertex3f( vlst[stt].x, vlst[stt].y, vlst[stt].z );
                glVertex3f( vlst[end].x, vlst[end].y, vlst[end].z );
            }
        }
        glEnd();
    }
    glPopAttrib();

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    for ( cnt = 0; cnt < pdef->numvertices; cnt++ )
    {
        int point_size;

        vert = faketoreal[cnt];

        size = MAXPOINTSIZE * vlst[vert].z / ( float ) pwin->pmesh->info.edgez;
        if ( size < 0.0f ) size = 0.0f;
        if ( size > MAXPOINTSIZE ) size = MAXPOINTSIZE;

        point_size = 4.0f * POINT_SIZE( size ) / zoom_hrz;

        if ( point_size > 0 )
        {
            oglx_texture_t * tx_tmp;

            if ( select_has_vert( vert ) )
            {
                tx_tmp = &tx_pointon;
            }
            else
            {
                tx_tmp = &tx_point;
            }

            vpos[kX] = vlst[vert].x;
            vpos[kY] = vlst[vert].y;
            vpos[kZ] = vlst[vert].z;

            ogl_draw_sprite_3d( tx_tmp, vpos, vup, vright, point_size, point_size );
        }
    }
}

//--------------------------------------------------------------------------------------------
void draw_side_fan( window_t * pwin, int fan, float zoom_hrz, float zoom_vrt )
{
    // ZZ> This function draws the line drawing preview of the tile type...
    //     A wireframe tile from a vertex connection window ( Side view )

    static Uint32 faketoreal[MPD_FAN_VERTICES_MAX];

    cart_vec_t vup    = { 0, 0, 1};
    cart_vec_t vright = { 1, 0, 0};
    cart_vec_t vpos;

    int cnt, stt, end, vert;
    float color[4];
    float size;
    float point_size;

    // aliases
    tile_definition_t    * pdef   = NULL;
    cartman_mpd_tile_t   * pfan   = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;
    tile_line_data_t     * plines = NULL;

    if ( NULL == pwin->pmesh ) pwin->pmesh = &mesh;
    vlst = pwin->pmesh->vrt;

    if ( fan < 0 || fan >= MPD_TILE_MAX ) return;
    pfan = pwin->pmesh->fan + fan;

    if ( pfan->type >= MPD_FAN_TYPE_MAX ) return;
    pdef = tile_dict + pfan->type;
    plines = tile_dict_lines + pfan->type;

    OGL_MAKE_COLOR_4( color, 32, 16, 16, 31 );
    if ( pfan->type >= MPD_FAN_TYPE_MAX / 2 )
    {
        OGL_MAKE_COLOR_4( color, 32, 31, 16, 16 );
    }

    for ( cnt = 0, vert = pfan->vrtstart;
          cnt < pdef->numvertices && CHAINEND != vert;
          cnt++, vert = vlst[vert].next )
    {
        faketoreal[cnt] = vert;
    }

    glPushAttrib( GL_TEXTURE_BIT | GL_ENABLE_BIT );
    {
        glColor4fv( color );
        glDisable( GL_TEXTURE_2D );

        glBegin( GL_LINES );
        {
            for ( cnt = 0; cnt < plines->count; cnt++ )
            {
                stt = faketoreal[plines->start[cnt]];
                end = faketoreal[plines->end[cnt]];

                glVertex3f( vlst[stt].x, vlst[stt].y, vlst[stt].z );
                glVertex3f( vlst[end].x, vlst[end].y, vlst[end].z );
            }
        }
        glEnd();
    }
    glPopAttrib();

    size = 7;
    point_size = 4.0f * POINT_SIZE( size ) / zoom_hrz;

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    for ( cnt = 0; cnt < pdef->numvertices; cnt++ )
    {
        oglx_texture_t * tx_tmp = NULL;

        vert = faketoreal[cnt];

        if ( select_has_vert( vert ) )
        {
            tx_tmp = &tx_pointon;
        }
        else
        {
            tx_tmp = &tx_point;
        }

        vpos[kX] = vlst[vert].x;
        vpos[kY] = vlst[vert].y;
        vpos[kZ] = vlst[vert].z;

        ogl_draw_sprite_3d( tx_tmp, vpos, vup, vright, point_size, point_size );
    }
}

//--------------------------------------------------------------------------------------------
void draw_schematic( window_t * pwin, int fantype, int x, int y )
{
    // ZZ> This function draws the line drawing preview of the tile type...
    //     The wireframe on the left side of the theSurface.

    int cnt, stt, end;
    float color[4];

    // aliases
    tile_line_data_t     * plines = NULL;
    tile_definition_t    * pdef   = NULL;

    if ( fantype < 0 || fantype >= MPD_FAN_TYPE_MAX ) return;
    plines = tile_dict_lines + fantype;
    pdef   = tile_dict + fantype;

    OGL_MAKE_COLOR_4( color, 32, 16, 16, 31 );
    if ( fantype >= MPD_FAN_TYPE_MAX / 2 )
    {
        OGL_MAKE_COLOR_4( color, 32, 31, 16, 16 );
    };

    glPushAttrib( GL_TEXTURE_BIT | GL_ENABLE_BIT );
    {
        glColor4fv( color );
        glDisable( GL_TEXTURE_2D );

        glBegin( GL_LINES );
        {
            for ( cnt = 0; cnt < plines->count; cnt++ )
            {
                stt = plines->start[cnt];
                end = plines->end[cnt];

                glVertex2f( pdef->u[stt] * TILE_FSIZE + x, pdef->v[stt] * TILE_FSIZE + y );
                glVertex2f( pdef->u[end] * TILE_FSIZE + x, pdef->v[end] * TILE_FSIZE + y );
            }
        }
        glEnd();
    }
    glPopAttrib();
}

//--------------------------------------------------------------------------------------------
void draw_top_tile( float x0, float y0, int fan, oglx_texture_t * tx_tile, bool_t draw_tile, cartman_mpd_t * pmesh )
{
    static simple_vertex_t loc_vrt[4];

    const float dst = 1.0f / 64.0f;

    int cnt;
    Uint32 ivrt;
    float min_s, min_t, max_s, max_t;

    // aliases
    cartman_mpd_tile_t   * pfan   = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;
    vlst = pmesh->vrt;

    if ( fan < 0 || fan >= MPD_TILE_MAX ) return;
    pfan = pmesh->fan + fan;

    // don't draw FANOFF
    if ( TILE_IS_FANOFF( pfan->tx_bits ) ) return;

    // don't draw if there is no texture
    if ( NULL == tx_tile ) return;
    oglx_texture_Bind( tx_tile );

    min_s = dst;
    min_t = dst;

    max_s = -dst + ( float ) oglx_texture_GetImageWidth( tx_tile )  / ( float ) oglx_texture_GetTextureWidth( tx_tile );
    max_t = -dst + ( float ) oglx_texture_GetImageHeight( tx_tile )  / ( float ) oglx_texture_GetTextureHeight( tx_tile );

    // set the texture coordinates
    loc_vrt[0].s = min_s;
    loc_vrt[0].t = min_t;

    loc_vrt[1].s = max_s;
    loc_vrt[1].t = min_t;

    loc_vrt[2].s = max_s;
    loc_vrt[2].t = max_t;

    loc_vrt[3].s = min_s;
    loc_vrt[3].t = max_t;

    // set the tile corners
    if ( draw_tile )
    {
        // draw the tile on a 31x31 grix, using the values of x0,y0

        ivrt = pfan->vrtstart;

        // Top Left
        loc_vrt[0].x = x0;
        loc_vrt[0].y = y0;
        loc_vrt[0].z = 0;
        loc_vrt[0].l = vlst[ivrt].a / 255.0f;
        ivrt = vlst[ivrt].next;

        // Top Right
        loc_vrt[1].x = x0 + TILE_FSIZE;
        loc_vrt[1].y = y0;
        loc_vrt[1].z = 0;
        loc_vrt[1].l = vlst[ivrt].a / 255.0f;
        ivrt = vlst[ivrt].next;

        // Bottom Right
        loc_vrt[2].x = x0 + TILE_FSIZE;
        loc_vrt[2].y = y0 + TILE_FSIZE;
        loc_vrt[2].z = 0;
        loc_vrt[2].l = vlst[ivrt].a / 255.0f;
        ivrt = vlst[ivrt].next;

        // Bottom Left
        loc_vrt[3].x = x0;
        loc_vrt[3].y = y0 + TILE_FSIZE;
        loc_vrt[3].z = 0;
        loc_vrt[3].l = vlst[ivrt].a / 255.0f;
    }
    else
    {
        // draw the tile using the actual values of the coordinates

        int cnt;

        ivrt = pfan->vrtstart;
        for ( cnt = 0;
              cnt < 4 && CHAINEND != ivrt;
              cnt++, ivrt = vlst[ivrt].next )
        {
            loc_vrt[cnt].x = vlst[ivrt].x;
            loc_vrt[cnt].y = vlst[ivrt].y;
            loc_vrt[cnt].z = vlst[ivrt].z;
            loc_vrt[cnt].l = vlst[ivrt].a / 255.0f;
        }
    }

    // Draw A Quad
    glBegin( GL_QUADS );
    {
        // initialize the color. remove any transparency!
        glColor4f( 1.0f,  1.0f,  1.0f, 1.0f );

        for ( cnt = 0; cnt < 4; cnt++ )
        {
            glColor3f( loc_vrt[cnt].l,  loc_vrt[cnt].l,  loc_vrt[cnt].l );
            glTexCoord2f( loc_vrt[cnt].s, loc_vrt[cnt].t );
            glVertex3f( loc_vrt[cnt].x, loc_vrt[cnt].y, loc_vrt[cnt].z );
        };
    }
    glEnd();
}

//--------------------------------------------------------------------------------------------
void draw_tile_fx( float x, float y, Uint8 fx, float scale )
{
    const int ioff_0 = TILE_ISIZE >> 3;
    const int ioff_1 = TILE_ISIZE >> 4;

    const float foff_0 = ioff_0 * scale;
    const float foff_1 = ioff_1 * scale;

    float x1, y1;
    float w1, h1;

    // water is whole tile
    if ( HAS_BITS( fx, MPDFX_WATER ) )
    {
        x1 = x;
        y1 = y;
        w1 = tx_water.imgW * scale;
        h1 = tx_water.imgH * scale;

        ogl_draw_sprite_2d( &tx_water, x1, y1, w1, h1 );
    }

    // "reflectable tile" is upper left
    if ( !HAS_BITS( fx, MPDFX_SHA ) )
    {
        x1 = x;
        y1 = y;
        w1 = tx_ref.imgW * scale;
        h1 = tx_ref.imgH * scale;

        ogl_draw_sprite_2d( &tx_ref, x1, y1, w1, h1 );
    }

    // "reflects characters" is upper right
    if ( HAS_BITS( fx, MPDFX_DRAWREF ) )
    {
        x1 = x + foff_0;
        y1 = y;

        w1 = tx_drawref.imgW * scale;
        h1 = tx_drawref.imgH * scale;

        ogl_draw_sprite_2d( &tx_drawref, x1, y1, w1, h1 );
    }

    // "animated tile" is lower left
    if ( HAS_BITS( fx, MPDFX_ANIM ) )
    {
        x1 = x;
        y1 = y + foff_0;

        w1 = tx_anim.imgW * scale;
        h1 = tx_anim.imgH * scale;

        ogl_draw_sprite_2d( &tx_anim, x1, y1, w1, h1 );
    }

    // the following are all in the lower left quad
    x1 = x + foff_0;
    y1 = y + foff_0;

    if ( HAS_BITS( fx, MPDFX_WALL ) )
    {
        float x2 = x1;
        float y2 = y1;

        w1 = tx_wall.imgW * scale;
        h1 = tx_wall.imgH * scale;

        ogl_draw_sprite_2d( &tx_wall, x2, y2, w1, h1 );
    }

    if ( HAS_BITS( fx, MPDFX_IMPASS ) )
    {
        float x2 = x1 + foff_1;
        float y2 = y1;

        w1 = tx_impass.imgW * scale;
        h1 = tx_impass.imgH * scale;

        ogl_draw_sprite_2d( &tx_impass, x2, y2, w1, h1 );
    }

    if ( HAS_BITS( fx, MPDFX_DAMAGE ) )
    {
        float x2 = x1;
        float y2 = y1 + foff_1;

        w1 = tx_damage.imgW * scale;
        h1 = tx_damage.imgH * scale;

        ogl_draw_sprite_2d( &tx_damage, x2, y2, w1, h1 );
    }

    if ( HAS_BITS( fx, MPDFX_SLIPPY ) )
    {
        float x2 = x1 + foff_1;
        float y2 = y1 + foff_1;

        w1 = tx_slippy.imgW * scale;
        h1 = tx_slippy.imgH * scale;

        ogl_draw_sprite_2d( &tx_slippy, x2, y2, w1, h1 );
    }

}

//--------------------------------------------------------------------------------------------
void ogl_draw_sprite_2d( oglx_texture_t * img, float x, float y, float width, float height )
{
    float w, h;
    float min_s, max_s, min_t, max_t;

    const float dst = 1.0f / 64.0f;

    w = width;
    h = height;

    if ( NULL != img )
    {
        if ( width == 0 || height == 0 )
        {
            w = oglx_texture_GetTextureWidth( img );
            h = oglx_texture_GetTextureHeight( img );
        }

        min_s = dst;
        min_t = dst;

        max_s = -dst + ( float ) oglx_texture_GetImageWidth( img )  / ( float ) oglx_texture_GetTextureWidth( img );
        max_t = -dst + ( float ) oglx_texture_GetImageHeight( img )  / ( float ) oglx_texture_GetTextureHeight( img );
    }
    else
    {
        min_s = dst;
        min_t = dst;

        max_s = 1.0f - dst;
        max_t = 1.0f - dst;
    }

    // Draw the image
    oglx_texture_Bind( img );

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    glBegin( GL_TRIANGLE_STRIP );
    {
        glTexCoord2f( min_s, min_t );  glVertex2f( x,     y );
        glTexCoord2f( max_s, min_t );  glVertex2f( x + w, y );
        glTexCoord2f( min_s, max_t );  glVertex2f( x,     y + h );
        glTexCoord2f( max_s, max_t );  glVertex2f( x + w, y + h );
    }
    glEnd();
}

//--------------------------------------------------------------------------------------------
void ogl_draw_sprite_3d( oglx_texture_t * img, cart_vec_t pos, cart_vec_t vup, cart_vec_t vright, float width, float height )
{
    float w, h;
    float min_s, max_s, min_t, max_t;
    cart_vec_t bboard[4];

    const float dst = 1.0f / 64.0f;

    w = width;
    h = height;

    if ( NULL != img )
    {
        if ( width == 0 || height == 0 )
        {
            w = oglx_texture_GetTextureWidth( img );
            h = oglx_texture_GetTextureHeight( img );
        }

        min_s = dst;
        min_t = dst;

        max_s = -dst + ( float ) oglx_texture_GetImageWidth( img )  / ( float ) oglx_texture_GetTextureWidth( img );
        max_t = -dst + ( float ) oglx_texture_GetImageHeight( img )  / ( float ) oglx_texture_GetTextureHeight( img );
    }
    else
    {
        min_s = dst;
        min_t = dst;

        max_s = 1.0f - dst;
        max_t = 1.0f - dst;
    }

    // Draw the image
    oglx_texture_Bind( img );

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    bboard[0][kX] = pos[kX] - w / 2 * vright[kX] + h / 2 * vup[kX];
    bboard[0][kY] = pos[kY] - w / 2 * vright[kY] + h / 2 * vup[kY];
    bboard[0][kZ] = pos[kZ] - w / 2 * vright[kZ] + h / 2 * vup[kZ];

    bboard[1][kX] = pos[kX] + w / 2 * vright[kX] + h / 2 * vup[kX];
    bboard[1][kY] = pos[kY] + w / 2 * vright[kY] + h / 2 * vup[kY];
    bboard[1][kZ] = pos[kZ] + w / 2 * vright[kZ] + h / 2 * vup[kZ];

    bboard[2][kX] = pos[kX] - w / 2 * vright[kX] - h / 2 * vup[kX];
    bboard[2][kY] = pos[kY] - w / 2 * vright[kY] - h / 2 * vup[kY];
    bboard[2][kZ] = pos[kZ] - w / 2 * vright[kZ] - h / 2 * vup[kZ];

    bboard[3][kX] = pos[kX] + w / 2 * vright[kX] - h / 2 * vup[kX];
    bboard[3][kY] = pos[kY] + w / 2 * vright[kY] - h / 2 * vup[kY];
    bboard[3][kZ] = pos[kZ] + w / 2 * vright[kZ] - h / 2 * vup[kZ];

    glBegin( GL_TRIANGLE_STRIP );
    {
        glTexCoord2f( min_s, min_t );  glVertex3fv( bboard[0] );
        glTexCoord2f( max_s, min_t );  glVertex3fv( bboard[1] );
        glTexCoord2f( min_s, max_t );  glVertex3fv( bboard[2] );
        glTexCoord2f( max_s, max_t );  glVertex3fv( bboard[3] );
    }
    glEnd();
}

//--------------------------------------------------------------------------------------------
void ogl_draw_box( float x, float y, float w, float h, float color[] )
{
    glPushAttrib( GL_ENABLE_BIT );
    {
        glDisable( GL_TEXTURE_2D );

        glBegin( GL_QUADS );
        {
            glColor4fv( color );

            glVertex2f( x,     y );
            glVertex2f( x,     y + h );
            glVertex2f( x + w, y + h );
            glVertex2f( x + w, y );
        }
        glEnd();
    }
    glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void ogl_beginFrame()
{
    glPushAttrib( GL_ENABLE_BIT );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );
    glEnable( GL_TEXTURE_2D );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glViewport( 0, 0, theSurface->w, theSurface->h );

    // Set up an ortho projection for the gui to use.  Controls are free to modify this
    // later, but most of them will need this, so it's done by default at the beginning
    // of a frame
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    glOrtho( 0, theSurface->w, theSurface->h, 0, -1, 1 );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
}

//--------------------------------------------------------------------------------------------
void ogl_endFrame()
{
    // Restore the OpenGL matrices to what they were
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    // Re-enable any states disabled by gui_beginFrame
    glPopAttrib();
}

//--------------------------------------------------------------------------------------------
void draw_sprite( SDL_Surface * dst, SDL_Surface * sprite, int x, int y )
{
    SDL_Rect rdst;

    if ( NULL == dst || NULL == sprite ) return;

    rdst.x = x;
    rdst.y = y;
    rdst.w = sprite->w;
    rdst.h = sprite->h;

    cartman_BlitSurface( sprite, NULL, dst, &rdst );
}

//--------------------------------------------------------------------------------------------
int cartman_BlitScreen( SDL_Surface * bmp, SDL_Rect * prect )
{
    return cartman_BlitSurface( bmp, NULL, theSurface, prect );
}

//--------------------------------------------------------------------------------------------
int cartman_BlitSurface( SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect )
{
    // clip the source and destination rectangles

    int retval = -1;
    SDL_Rect rsrc, rdst;

    if ( NULL == src || HAS_BITS(( size_t )src->map, 0x80000000 ) ) return 0;

    if ( NULL == srcrect && NULL == dstrect )
    {
        retval = SDL_BlitSurface( src, NULL, dst, NULL );
        if ( retval >= 0 )
        {
            SDL_UpdateRect( dst, 0, 0, 0, 0 );
        }
    }
    else if ( NULL == srcrect )
    {
        SDL_RectIntersect( &( dst->clip_rect ), dstrect, &rdst );
        retval = SDL_BlitSurface( src, NULL, dst, &rdst );
        if ( retval >= 0 )
        {
            SDL_UpdateRect( dst, rdst.x, rdst.y, rdst.w, rdst.h );
        }
    }
    else if ( NULL == dstrect )
    {
        SDL_RectIntersect( &( src->clip_rect ), srcrect, &rsrc );

        retval = SDL_BlitSurface( src, &rsrc, dst, NULL );
        if ( retval >= 0 )
        {
            SDL_UpdateRect( dst, 0, 0, 0, 0 );
        }
    }
    else
    {
        SDL_RectIntersect( &( src->clip_rect ), srcrect, &rsrc );
        SDL_RectIntersect( &( dst->clip_rect ), dstrect, &rdst );

        retval = SDL_BlitSurface( src, &rsrc, dst, &rdst );
        if ( retval >= 0 )
        {
            SDL_UpdateRect( dst, rdst.x, rdst.y, rdst.w, rdst.h );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
SDL_Surface * cartman_LoadIMG( const char * szName )
{
    SDL_PixelFormat tmpformat;
    SDL_Surface * bmptemp, * bmpconvert;

    // load the bitmap
    bmptemp = IMG_Load( szName );

    // expand the screen format to support alpha
    memcpy( &tmpformat, theSurface->format, sizeof( SDL_PixelFormat ) );   // make a copy of the format
    SDLX_ExpandFormat( &tmpformat );

    // convert it to the same pixel format as the screen surface
    bmpconvert = SDL_ConvertSurface( bmptemp, &tmpformat, SDL_SWSURFACE );
    SDL_FreeSurface( bmptemp );

    return bmpconvert;
}

//--------------------------------------------------------------------------------------------
void cartman_begin_ortho_camera_hrz( window_t * pwin, camera_t * pcam, float zoom_x, float zoom_y )
{
    float w, h, d;
    float aspect;
    float left, right, bottom, top, front, back;

    w = ( float )DEFAULT_RESOLUTION * TILE_FSIZE * (( float )pwin->surfacex / ( float )DEFAULT_WINDOW_W ) / zoom_x;
    h = ( float )DEFAULT_RESOLUTION * TILE_FSIZE * (( float )pwin->surfacey / ( float )DEFAULT_WINDOW_H ) / zoom_y;
    d = DEFAULT_Z_SIZE;

    pcam->w = w;
    pcam->h = h;
    pcam->d = d;

    left   = - w / 2;
    right  =   w / 2;
    bottom = - h / 2;
    top    =   h / 2;
    front  = -d;
    back   =  d;

    aspect = ( float ) w / h;
    if ( aspect < 1.0f )
    {
        // window taller than wide
        bottom /= aspect;
        top /= aspect;
    }
    else
    {
        left *= aspect;
        right *= aspect;
    }

    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    glOrtho( left, right, bottom, top, front, back );

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    glScalef( -1.0f, 1.0f, 1.0f );
    gluLookAt( pcam->x, pcam->y, back, pcam->x, pcam->y, front, 0.0f, -1.0f, 0.0f );
}

//--------------------------------------------------------------------------------------------
void cartman_begin_ortho_camera_vrt( window_t * pwin, camera_t * pcam, float zoom_x, float zoom_z )
{
    float w, h, d;
    float aspect;
    float left, right, bottom, top, back, front;

    w = pwin->surfacex * ( float )DEFAULT_RESOLUTION * TILE_FSIZE / ( float )DEFAULT_WINDOW_W / zoom_x;
    h = w;
    d = pwin->surfacey * ( float )DEFAULT_RESOLUTION * TILE_FSIZE / ( float )DEFAULT_WINDOW_H / zoom_z;

    pcam->w = w;
    pcam->h = h;
    pcam->d = d;

    left   = - w / 2;
    right  =   w / 2;
    bottom = - d / 2;
    top    =   d / 2;
    front  =   0;
    back   =   h;

    aspect = ( float ) w / ( float ) d;
    if ( aspect < 1.0f )
    {
        // window taller than wide
        bottom /= aspect;
        top /= aspect;
    }
    else
    {
        left *= aspect;
        right *= aspect;
    }

    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    glOrtho( left, right, bottom, top, front, back );

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    glScalef( 1.0f, 1.0f, 1.0f );
    gluLookAt( pcam->x, pcam->y, pcam->z, pcam->x, pcam->y + back, pcam->z, 0.0f, 0.0f, 1.0f );
}

//--------------------------------------------------------------------------------------------
void cartman_end_ortho_camera( )
{
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();

    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
}

//--------------------------------------------------------------------------------------------
void create_imgcursor( void )
{
    int x, y;
    Uint32 col, loc, clr;
    SDL_Rect rtmp;

    bmpcursor = cartman_CreateSurface( 8, 8 );
    col = MAKE_BGR( bmpcursor, 31, 31, 31 );    // White color
    loc = MAKE_BGR( bmpcursor, 3, 3, 3 );           // Gray color
    clr = MAKE_ABGR( bmpcursor, 0, 0, 0, 8 );

    // Simple triangle
    rtmp.x = 0;
    rtmp.y = 0;
    rtmp.w = 8;
    rtmp.h = 1;
    SDL_FillRect( bmpcursor, &rtmp, loc );

    for ( y = 0; y < 8; y++ )
    {
        for ( x = 0; x < 8; x++ )
        {
            if ( x + y < 8 ) SDL_PutPixel( bmpcursor, x, y, col );
            else SDL_PutPixel( bmpcursor, x, y, clr );
        }
    }
}

//--------------------------------------------------------------------------------------------
void load_img( void )
{
    if ( INVALID_GL_ID == oglx_texture_Load( &tx_point, "data" SLASH_STR "point.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "point.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_Load( &tx_pointon, "data" SLASH_STR "pointon.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "pointon.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_Load( &tx_ref, "data" SLASH_STR "ref.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "ref.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_Load( &tx_drawref, "data" SLASH_STR "drawref.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "drawref.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_Load( &tx_anim, "data" SLASH_STR "anim.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "anim.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_Load( &tx_water, "data" SLASH_STR "water.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "water.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_Load( &tx_wall, "data" SLASH_STR "slit.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "slit.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_Load( &tx_impass, "data" SLASH_STR "impass.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "impass.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_Load( &tx_damage, "data" SLASH_STR "damage.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "damage.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_Load( &tx_slippy, "data" SLASH_STR "slippy.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "slippy.png" );
    }
}

//--------------------------------------------------------------------------------------------
void get_small_tiles( SDL_Surface* bmpload )
{
    SDL_Surface * image;

    int x, y, x1, y1;
    int sz_x = bmpload->w;
    int sz_y = bmpload->h;
    int step_x = sz_x >> 3;
    int step_y = sz_y >> 3;

    if ( step_x == 0 ) step_x = 1;
    if ( step_y == 0 ) step_y = 1;

    y1 = 0;
    y = 0;
    while ( y < sz_y && y1 < 256 )
    {
        x1 = 0;
        x = 0;
        while ( x < sz_x && x1 < 256 )
        {
            SDL_Rect src1 = { x, y, ( step_x - 1 ), ( step_y - 1 ) };

            oglx_texture_ctor( tx_smalltile + numsmalltile );

            image = cartman_CreateSurface( SMALLXY, SMALLXY );
            SDL_FillRect( image, NULL, MAKE_BGR( image, 0, 0, 0 ) );
            SDL_SoftStretch( bmpload, &src1, image, NULL );

            oglx_texture_Convert( tx_smalltile + numsmalltile, image, INVALID_KEY );

            numsmalltile++;
            x += step_x;
            x1 += 32;
        }
        y += step_y;
        y1 += 32;
    }
}

//--------------------------------------------------------------------------------------------
void get_big_tiles( SDL_Surface* bmpload )
{
    SDL_Surface * image;

    int x, y, x1, y1;
    int sz_x = bmpload->w;
    int sz_y = bmpload->h;
    int step_x = sz_x >> 3;
    int step_y = sz_y >> 3;

    if ( step_x == 0 ) step_x = 1;
    if ( step_y == 0 ) step_y = 1;

    y1 = 0;
    y = 0;
    while ( y < sz_y )
    {
        x1 = 0;
        x = 0;
        while ( x < sz_x )
        {
            int wid, hgt;

            SDL_Rect src1;

            wid = ( 2 * step_x - 1 );
            if ( x + wid > bmpload->w ) wid = bmpload->w - x;

            hgt = ( 2 * step_y - 1 );
            if ( y + hgt > bmpload->h ) hgt = bmpload->h - y;

            src1.x = x;
            src1.y = y;
            src1.w = wid;
            src1.h = hgt;

            oglx_texture_ctor( tx_bigtile + numbigtile );

            image = cartman_CreateSurface( SMALLXY, SMALLXY );
            SDL_FillRect( image, NULL, MAKE_BGR( image, 0, 0, 0 ) );

            SDL_SoftStretch( bmpload, &src1, image, NULL );

            oglx_texture_Convert( tx_bigtile + numbigtile, image, INVALID_KEY );

            numbigtile++;
            x += step_x;
            x1 += 32;
        }
        y += step_y;
        y1 += 32;
    }
}

//--------------------------------------------------------------------------------------------
void get_tiles( SDL_Surface* bmpload )
{
    get_small_tiles( bmpload );
    get_big_tiles( bmpload );
}

//--------------------------------------------------------------------------------------------
SDL_Surface * cartman_CreateSurface( int w, int h )
{
    SDL_PixelFormat   tmpformat;

    if ( NULL == theSurface ) return NULL;

    // expand the screen format to support alpha
    memcpy( &tmpformat, theSurface->format, sizeof( SDL_PixelFormat ) );   // make a copy of the format
    SDLX_ExpandFormat( &tmpformat );

    return SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, tmpformat.BitsPerPixel, tmpformat.Rmask, tmpformat.Gmask, tmpformat.Bmask, tmpformat.Amask );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void gfx_init_SDL_graphics()
{
    if ( _sdl_initialized_graphics ) return;

    cartman_init_SDL_base();

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
    SDL_WM_SetCaption( NAME " " VERSION_STR, NAME );

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
int gfx_init_ogl()
{
    gfx_init_SDL_graphics();

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
    // backface culling
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
    // load_graphics();

    _ogl_initialized = btrue;

    return _ogl_initialized && _sdl_initialized_graphics;
}
