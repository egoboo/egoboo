//********************************************************************************************
//*
//*    This file is part of Cartman.
//*
//*    Cartman is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Cartman is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Cartman.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "egolib/egolib.h"

#include "cartman/cartman_gfx.h"
#include "cartman/cartman.h"
#include "cartman/cartman_map.h"
#include "cartman/cartman_gui.h"
#include "cartman/cartman_functions.h"
#include "cartman/cartman_select.h"
#include "cartman/cartman_math.h"
#include "cartman/SDL_Pixel.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static SDL_bool  _sdl_initialized_graphics = SDL_FALSE;
static GLboolean _ogl_initialized = GL_FALSE;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

const SDL_Color cart_white = { 0xFF, 0xFF, 0xFF, 0xFF };
const SDL_Color cart_black = { 0x00, 0x00, 0x00, 0xFF };

std::shared_ptr<Ego::Font> gfx_font_ptr = NULL;

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
static void gfx_system_init_SDL_graphics();
static int  gfx_init_ogl();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void gfx_system_begin()
{
    // set the graphics state
    gfx_system_init_SDL_graphics();
    ImageManager::initialize();
    Ego::Renderer::initialize();
    TextureManager::initialize();
    gfx_init_ogl();

    theSurface = SDL_GetVideoSurface();

    Ego::FontManager::initialize();
    gfx_font_ptr = Ego::FontManager::loadFont("editor/pc8x8.fon", 12);
}

//--------------------------------------------------------------------------------------------
void gfx_system_end()
{
    gfx_font_ptr.reset();
    Ego::FontManager::uninitialize();
    TextureManager::uninitialize();
    Ego::Renderer::uninitialize();
    ImageManager::uninitialize();
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
oglx_texture_t * tiny_tile_at( cartman_mpd_t * pmesh, int mapx, int mapy )
{
    Uint16 tx_bits, basetile;
    Uint8 fantype, fx;

    cartman_mpd_tile_t * pfan;

    tx_bits = 0;
    fantype = 0;
    fx = 0;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( mapx < 0 || mapx >= pmesh->info.tiles_x || mapy < 0 || mapy >= pmesh->info.tiles_y )
    {
        return NULL;
    }

    pfan = pmesh->get_pfan(mapx, mapy);
    if ( NULL == pfan ) return NULL;

    if ( TILE_IS_FANOFF( pfan->tx_bits ) )
    {
        return NULL;
    }

    tx_bits = pfan->tx_bits;
    fantype = pfan->type;
    fx      = pfan->fx;

    if ( HAS_BITS( fx, MAPFX_ANIM ) )
    {
        animtileframeadd = ( timclock >> 3 ) & 3;
        if ( fantype >= tile_dict.offset )
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

    if ( fantype >= tile_dict.offset )
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
    int    img;
    Uint16 img_base;
    Uint8  type, fx;

    cartman_mpd_tile_t   * pfan   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    pfan = CART_MPD_FAN_PTR( pmesh, fan );
    if ( NULL == pfan ) return NULL;

    if ( TILE_IS_FANOFF( pfan->tx_bits ) )
    {
        return NULL;
    }

    img = TILE_GET_LOWER_BITS( pfan->tx_bits );
    type = pfan->type;
    fx = pfan->fx;

    if ( HAS_BITS( fx, MAPFX_ANIM ) )
    {
        animtileframeadd = ( timclock >> 3 ) & 3;
        if ( type >= tile_dict.offset )
        {
            // Big tiles
            img_base = img & biganimtilebaseand;// Animation set
            img += ( animtileframeadd << 1 );   // Animated img
            img = ( img & biganimtileframeand ) + img_base;
        }
        else
        {
            // Small tiles
            img_base = img & animtilebaseand;  // Animation set
            img += animtileframeadd;           // Animated img
            img = ( img & animtileframeand ) + img_base;
        }
    }

    // remove any of the upper bit information
    img &= 0xFF;

    if ( type >= tile_dict.offset )
    {
        return tx_bigtile + img;
    }
    else
    {
        return tx_smalltile + img;
    }
}

//--------------------------------------------------------------------------------------------
void make_hitemap( cartman_mpd_t * pmesh )
{
    if ( NULL == pmesh ) pmesh = &mesh;

    if ( bmphitemap ) SDL_FreeSurface( bmphitemap );

    bmphitemap = cartman_CreateSurface( pmesh->info.tiles_x << 2, pmesh->info.tiles_y << 2 );
    if ( NULL == bmphitemap ) return;

    for (int pixy = 0, y = 16; pixy < ( pmesh->info.tiles_y << 2 ); pixy++, y += 32 )
    {
        for (int pixx = 0, x = 16; pixx < ( pmesh->info.tiles_x << 2 ); pixx++, x += 32 )
        {
            int level = ( pmesh->get_level(x, y) * 255 ) / pmesh->info.edgez;  // level is 0 to 255
            if ( level > 252 ) level = 252;

            cartman_mpd_tile_t *pfan = pmesh->get_pfan(pixx >> 2, pixy >> 2);
            if ( NULL == pfan ) continue;

            if ( HAS_BITS( pfan->fx, MAPFX_WALL ) ) level = 253;  // Wall
            if ( HAS_BITS( pfan->fx, MAPFX_IMPASS ) ) level = 254;   // Impass
            if ( HAS_BITS( pfan->fx, MAPFX_WALL ) && HAS_BITS( pfan->fx, MAPFX_IMPASS ) ) level = 255;   // Both

            SDL_PutPixel( bmphitemap, pixx, pixy, level );
        }
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
                SDL_Rect dst = {static_cast<Sint16>(putx), static_cast<Sint16>(puty), TINYXY, TINYXY};
                cartman_BlitSurface(tx_tile->source, nullptr, bmphitemap, &dst);
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
void draw_top_fan( select_lst_t * plst, int fan, float zoom_hrz, float zoom_vrt )
{
    // ZZ> This function draws the line drawing preview of the tile type...
    //     A wireframe tile from a vertex connection window

    static Uint32 faketoreal[MAP_FAN_VERTICES_MAX];

    int cnt, stt, end, vert;
    float color[4];
    float size;

    cart_vec_t vup    = { 0, 1, 0};
    cart_vec_t vright = { -1, 0, 0};
    cart_vec_t vpos;

    // aliases
    cartman_mpd_t  * pmesh  = NULL;
    tile_definition_t    * pdef   = NULL;
    const cartman_mpd_tile_t   * pfan   = NULL;
    tile_line_data_t     * plines = NULL;

    plst = select_lst_synch_mesh( plst, &mesh );
    if ( NULL == plst ) return;

    if ( NULL == plst->pmesh ) return;
    pmesh = plst->pmesh;

    pfan = CART_MPD_FAN_PTR( pmesh, fan );
    if ( NULL == pfan ) return;

    pdef   = TILE_DICT_PTR( tile_dict, pfan->type );
    if ( NULL == pdef ) return;

    plines = tile_dict_lines + pfan->type;

    if ( 0 == pdef->numvertices || pdef->numvertices > MAP_FAN_VERTICES_MAX ) return;

    OGL_MAKE_COLOR_4( color, 32, 16, 16, 31 );
    if ( pfan->type >= tile_dict.offset )
    {
        OGL_MAKE_COLOR_4( color, 32, 31, 16, 16 );
    }

    for ( cnt = 0, vert = pfan->vrtstart;
          cnt < pdef->numvertices && CHAINEND != vert;
          cnt++, vert = pmesh->vrt2[vert].next)
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

                glVertex3f(pmesh->vrt2[stt].x, pmesh->vrt2[stt].y, pmesh->vrt2[stt].z);
                glVertex3f(pmesh->vrt2[end].x, pmesh->vrt2[end].y, pmesh->vrt2[end].z);
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

        size = MAXPOINTSIZE * pmesh->vrt2[vert].z / (float)pmesh->info.edgez;
        if ( size < 0.0f ) size = 0.0f;
        if ( size > MAXPOINTSIZE ) size = MAXPOINTSIZE;

        point_size = 4.0f * POINT_SIZE( size ) / zoom_hrz;

        if ( point_size > 0 )
        {
            int select_rv;
            oglx_texture_t * tx_tmp;

            select_rv = select_lst_find( plst, vert );
            if ( select_rv < 0 )
            {
                tx_tmp = &tx_point;
            }
            else
            {
                tx_tmp = &tx_pointon;
            }

            vpos[kX] = pmesh->vrt2[vert].x;
            vpos[kY] = pmesh->vrt2[vert].y;
            vpos[kZ] = pmesh->vrt2[vert].z;

            ogl_draw_sprite_3d( tx_tmp, vpos, vup, vright, point_size, point_size );
        }
    }
}

//--------------------------------------------------------------------------------------------
void draw_side_fan( select_lst_t * plst, int fan, float zoom_hrz, float zoom_vrt )
{
    // ZZ> This function draws the line drawing preview of the tile type...
    //     A wireframe tile from a vertex connection window ( Side view )

    static Uint32 faketoreal[MAP_FAN_VERTICES_MAX];

    cart_vec_t vup    = { 0, 0, 1};
    cart_vec_t vright = { 1, 0, 0};
    cart_vec_t vpos;

    int cnt, stt, end, vert;
    float color[4];
    float size;
    float point_size;

    // aliases
    cartman_mpd_t        * pmesh  = NULL;
    tile_definition_t    * pdef   = NULL;
    cartman_mpd_tile_t   * pfan   = NULL;
    tile_line_data_t     * plines = NULL;

    plst = select_lst_synch_mesh( plst, &mesh );
    if ( NULL == plst ) return;

    if ( NULL == plst->pmesh ) return;
    pmesh = plst->pmesh;

    pfan = CART_MPD_FAN_PTR( pmesh, fan );
    if ( NULL == pfan ) return;

    pdef = TILE_DICT_PTR( tile_dict, pfan->type );
    if ( NULL == pdef ) return;

    plines = tile_dict_lines + pfan->type;

    OGL_MAKE_COLOR_4( color, 32, 16, 16, 31 );
    if ( pfan->type >= tile_dict.offset )
    {
        OGL_MAKE_COLOR_4( color, 32, 31, 16, 16 );
    }

    for ( cnt = 0, vert = pfan->vrtstart;
          cnt < pdef->numvertices && CHAINEND != vert;
          cnt++, vert = pmesh->vrt2[vert].next )
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

                glVertex3f(pmesh->vrt2[stt].x, pmesh->vrt2[stt].y, pmesh->vrt2[stt].z);
                glVertex3f(pmesh->vrt2[end].x, pmesh->vrt2[end].y, pmesh->vrt2[end].z);
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
        int select_rv;
        oglx_texture_t * tx_tmp = NULL;

        vert = faketoreal[cnt];

        select_rv = select_lst_find( plst, vert );
        if ( select_rv < 0 )
        {
            tx_tmp = &tx_point;
        }
        else
        {
            tx_tmp = &tx_pointon;
        }

        vpos[kX] = pmesh->vrt2[vert].x;
        vpos[kY] = pmesh->vrt2[vert].y;
        vpos[kZ] = pmesh->vrt2[vert].z;

        ogl_draw_sprite_3d( tx_tmp, vpos, vup, vright, point_size, point_size );
    }
}

//--------------------------------------------------------------------------------------------
void draw_schematic(std::shared_ptr<Cartman_Window> pwin, int fantype, int x, int y)
{
    // ZZ> This function draws the line drawing preview of the tile type...
    //     The wireframe on the left side of the theSurface.

    int cnt, stt, end;
    float color[4];

    // aliases
    tile_line_data_t     * plines = NULL;
    tile_definition_t    * pdef   = NULL;

    pdef   = TILE_DICT_PTR( tile_dict, fantype );
    if ( NULL == pdef ) return;

    plines = tile_dict_lines + fantype;

    OGL_MAKE_COLOR_4( color, 32, 16, 16, 31 );
    if ( fantype >= tile_dict.offset )
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

                glVertex2f( GRID_TO_POS( pdef->grid_ix[stt] ) + x, GRID_TO_POS( pdef->grid_iy[stt] ) + y );
                glVertex2f( GRID_TO_POS( pdef->grid_ix[end] ) + x, GRID_TO_POS( pdef->grid_iy[end] ) + y );
            }
        }
        glEnd();
    }
    glPopAttrib();
}

//--------------------------------------------------------------------------------------------
void draw_top_tile( float x0, float y0, int fan, oglx_texture_t * tx_tile, bool draw_tile, cartman_mpd_t * pmesh )
{
    static simple_vertex_t loc_vrt[4];

    const float dst = 1.0f / 64.0f;

    int cnt;
    Uint32 ivrt;
    float min_s, min_t, max_s, max_t;

    // aliases
    cartman_mpd_tile_t   * pfan   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    pfan = CART_MPD_FAN_PTR( pmesh, fan );
    if ( NULL == pfan ) return;

    // don't draw FANOFF
    if ( TILE_IS_FANOFF( pfan->tx_bits ) ) return;

    // don't draw if there is no texture
    if ( NULL == tx_tile ) return;
    oglx_texture_t::bind( tx_tile );

    min_s = dst;
    min_t = dst;

    max_s = -dst + (float)oglx_texture_t::getSourceWidth(tx_tile) / (float)oglx_texture_t::getWidth(tx_tile);
    max_t = -dst + (float)oglx_texture_t::getSourceHeight(tx_tile) / (float)oglx_texture_t::getHeight(tx_tile);

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
        loc_vrt[0].l = pmesh->vrt2[ivrt].a / 255.0f;
        ivrt = pmesh->vrt2[ivrt].next;

        // Top Right
        loc_vrt[1].x = x0 + TILE_FSIZE;
        loc_vrt[1].y = y0;
        loc_vrt[1].z = 0;
        loc_vrt[1].l = pmesh->vrt2[ivrt].a / 255.0f;
        ivrt = pmesh->vrt2[ivrt].next;

        // Bottom Right
        loc_vrt[2].x = x0 + TILE_FSIZE;
        loc_vrt[2].y = y0 + TILE_FSIZE;
        loc_vrt[2].z = 0;
        loc_vrt[2].l = pmesh->vrt2[ivrt].a / 255.0f;
        ivrt = pmesh->vrt2[ivrt].next;

        // Bottom Left
        loc_vrt[3].x = x0;
        loc_vrt[3].y = y0 + TILE_FSIZE;
        loc_vrt[3].z = 0;
        loc_vrt[3].l = pmesh->vrt2[ivrt].a / 255.0f;
    }
    else
    {
        // draw the tile using the actual values of the coordinates

        int cnt;

        ivrt = pfan->vrtstart;
        for ( cnt = 0;
              cnt < 4 && CHAINEND != ivrt;
              cnt++, ivrt = pmesh->vrt2[ivrt].next)
        {
            loc_vrt[cnt].x = pmesh->vrt2[ivrt].x;
            loc_vrt[cnt].y = pmesh->vrt2[ivrt].y;
            loc_vrt[cnt].z = pmesh->vrt2[ivrt].z;
            loc_vrt[cnt].l = pmesh->vrt2[ivrt].a / 255.0f;
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
    if ( HAS_BITS( fx, MAPFX_WATER ) )
    {
        x1 = x;
        y1 = y;
        w1 = oglx_texture_t::getSourceWidth(&tx_water) * scale;
        h1 = oglx_texture_t::getSourceHeight(&tx_water) * scale;

        ogl_draw_sprite_2d( &tx_water, x1, y1, w1, h1 );
    }

    // "reflectable tile" is upper left
    if ( !HAS_BITS( fx, MAPFX_SHA ) )
    {
        x1 = x;
        y1 = y;
        w1 = oglx_texture_t::getSourceWidth(&tx_ref) * scale;
        h1 = oglx_texture_t::getSourceHeight(&tx_ref) * scale;

        ogl_draw_sprite_2d( &tx_ref, x1, y1, w1, h1 );
    }

    // "reflects characters" is upper right
    if ( HAS_BITS( fx, MAPFX_DRAWREF ) )
    {
        x1 = x + foff_0;
        y1 = y;

        w1 = oglx_texture_t::getSourceWidth(&tx_drawref) * scale;
        h1 = oglx_texture_t::getSourceHeight(&tx_drawref) * scale;

        ogl_draw_sprite_2d( &tx_drawref, x1, y1, w1, h1 );
    }

    // "animated tile" is lower left
    if ( HAS_BITS( fx, MAPFX_ANIM ) )
    {
        x1 = x;
        y1 = y + foff_0;

        w1 = oglx_texture_t::getSourceWidth(&tx_anim) * scale;
        h1 = oglx_texture_t::getSourceHeight(&tx_anim) * scale;

        ogl_draw_sprite_2d( &tx_anim, x1, y1, w1, h1 );
    }

    // the following are all in the lower left quad
    x1 = x + foff_0;
    y1 = y + foff_0;

    if ( HAS_BITS( fx, MAPFX_WALL ) )
    {
        float x2 = x1;
        float y2 = y1;

        w1 = oglx_texture_t::getSourceWidth(&tx_wall) * scale;
        h1 = oglx_texture_t::getSourceHeight(&tx_wall) * scale;

        ogl_draw_sprite_2d( &tx_wall, x2, y2, w1, h1 );
    }

    if ( HAS_BITS( fx, MAPFX_IMPASS ) )
    {
        float x2 = x1 + foff_1;
        float y2 = y1;

        w1 = oglx_texture_t::getSourceWidth(&tx_impass) * scale;
        h1 = oglx_texture_t::getSourceHeight(&tx_impass) * scale;

        ogl_draw_sprite_2d( &tx_impass, x2, y2, w1, h1 );
    }

    if ( HAS_BITS( fx, MAPFX_DAMAGE ) )
    {
        float x2 = x1;
        float y2 = y1 + foff_1;

        w1 = oglx_texture_t::getSourceWidth(&tx_damage) * scale;
        h1 = oglx_texture_t::getSourceHeight(&tx_damage) * scale;

        ogl_draw_sprite_2d( &tx_damage, x2, y2, w1, h1 );
    }

    if ( HAS_BITS( fx, MAPFX_SLIPPY ) )
    {
        float x2 = x1 + foff_1;
        float y2 = y1 + foff_1;

        w1 = oglx_texture_t::getSourceWidth(&tx_slippy) * scale;
        h1 = oglx_texture_t::getSourceHeight(&tx_slippy) * scale;

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
            w = oglx_texture_t::getWidth( img );
            h = oglx_texture_t::getHeight( img );
        }

        min_s = dst;
        min_t = dst;

        max_s = -dst + (float)oglx_texture_t::getSourceWidth(img) / (float)oglx_texture_t::getWidth(img);
        max_t = -dst + (float)oglx_texture_t::getSourceHeight(img) / (float)oglx_texture_t::getHeight(img);
    }
    else
    {
        min_s = dst;
        min_t = dst;

        max_s = 1.0f - dst;
        max_t = 1.0f - dst;
    }

    // Draw the image
    oglx_texture_t::bind( img );

    Ego::Renderer::get().setColour(Ego::Math::Colour4f(1.0f, 1.0f, 1.0f, 1.0f));

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
            w = oglx_texture_t::getWidth( img );
            h = oglx_texture_t::getHeight( img );
        }

        min_s = dst;
        min_t = dst;

        max_s = -dst + (float)oglx_texture_t::getSourceWidth(img) / (float)oglx_texture_t::getWidth(img);
        max_t = -dst + (float)oglx_texture_t::getSourceHeight(img) / (float)oglx_texture_t::getHeight(img);
    }
    else
    {
        min_s = dst;
        min_t = dst;

        max_s = 1.0f - dst;
        max_t = 1.0f - dst;
    }

    // Draw the image
    oglx_texture_t::bind( img );

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
void ogl_draw_box_xy( float x, float y, float z, float w, float h, float color[] )
{
    glPushAttrib( GL_ENABLE_BIT );
    {
        glDisable( GL_TEXTURE_2D );

        glColor4fv( color );

        glBegin( GL_QUADS );
        {
            glVertex3f( x,     y,     z );
            glVertex3f( x,     y + h, z );
            glVertex3f( x + w, y + h, z );
            glVertex3f( x + w, y,     z );
        }
        glEnd();
    }
    glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void ogl_draw_box_xz( float x, float y, float z, float w, float d, float color[] )
{
    glPushAttrib( GL_ENABLE_BIT );
    {
        glDisable( GL_TEXTURE_2D );

        glColor4fv( color );

        glBegin( GL_QUADS );
        {
            glVertex3f( x,     y, z );
            glVertex3f( x,     y, z + d );
            glVertex3f( x + w, y, z + d );
            glVertex3f( x + w, y, z );
        }
        glEnd();
    }
    glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void ogl_beginFrame()
{
    glPushAttrib( GL_ENABLE_BIT );
	Ego::Renderer::get().setDepthTestEnabled(false);
    glDisable( GL_CULL_FACE );
    glEnable( GL_TEXTURE_2D );

    Ego::Renderer::get().setBlendingEnabled(true);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    Ego::Renderer::get().setViewportRectangle(0, 0, theSurface->w, theSurface->h);

    // Set up an ortho projection for the gui to use.  Controls are free to modify this
    // later, but most of them will need this, so it's done by default at the beginning
    // of a frame
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    fmat_4x4_t projection = fmat_4x4_t::ortho(0, theSurface->w, theSurface->h, 0, -1, 1);
    Ego::Renderer::get().loadMatrix(projection);

    glMatrixMode( GL_MODELVIEW );
    Ego::Renderer::get().loadMatrix(fmat_4x4_t::identity());
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
SDL_Surface *cartman_LoadIMG(const char *name)
{
    // Load the image.
    SDL_Surface *originalImage = IMG_Load_RW(vfs_openRWopsRead(name), 1);
    if (!originalImage)
    {
        log_error("unable to load image `%s` - reason `%s`\n", name, IMG_GetError());
        return nullptr;
    }

    // Expand the screen format to support alpha.
    SDL_PixelFormat expandedPixelFormat;
    memcpy(&expandedPixelFormat, theSurface->format, sizeof(SDL_PixelFormat));   // make a copy of the format
    SDLX_ExpandFormat(&expandedPixelFormat);

    // Convert the image to the same pixel format as the expanded screen format.
    SDL_Surface *convertedImage = SDL_ConvertSurface(originalImage, &expandedPixelFormat, SDL_SWSURFACE);
    SDL_FreeSurface(originalImage);
    if (!convertedImage)
    {
        log_error("unable to convert image `%s`\n", name);
    }

    return convertedImage;
}

//--------------------------------------------------------------------------------------------
void cartman_begin_ortho_camera_hrz(std::shared_ptr<Cartman_Window> pwin, camera_t * pcam, float zoom_x, float zoom_y)
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
void cartman_begin_ortho_camera_vrt(std::shared_ptr<Cartman_Window> pwin, camera_t * pcam, float zoom_x, float zoom_z)
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
void cartman_end_ortho_camera()
{
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();

    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
}

//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
void load_img()
{
    if ( INVALID_GL_ID == oglx_texture_t::load( &tx_point, "editor/point.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "editor/point.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_t::load( &tx_pointon, "editor/pointon.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "editor/pointon.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_t::load( &tx_ref, "editor/ref.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "editor/ref.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_t::load( &tx_drawref, "editor/drawref.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "editor/drawref.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_t::load( &tx_anim, "editor/anim.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "editor/anim.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_t::load( &tx_water, "editor/water.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "editor/water.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_t::load( &tx_wall, "editor/slit.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "editor/slit.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_t::load( &tx_impass, "editor/impass.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "editor/impass.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_t::load( &tx_damage, "editor/damage.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "editor/damage.png" );
    }

    if ( INVALID_GL_ID == oglx_texture_t::load( &tx_slippy, "editor/slippy.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "editor/slippy.png" );
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

    for ( y = 0, y1 = 0; y < sz_y && y1 < 256; y += step_y, y1 += SMALLXY )
    {
        for ( x = 0, x1 = 0; x < sz_x && x1 < 256; x += step_x, x1 += SMALLXY )
        {
            SDL_Rect src1 = { static_cast<Sint16>(x), static_cast<Sint16>(y), static_cast<Uint16>( step_x - 1 ), static_cast<Uint16>( step_y - 1 ) };

            oglx_texture_t::ctor( tx_smalltile + numsmalltile );

            image = cartman_CreateSurface( SMALLXY, SMALLXY );
            SDL_FillRect( image, NULL, MAKE_BGR( image, 0, 0, 0 ) );
            SDL_SoftStretch( bmpload, &src1, image, NULL );

            oglx_texture_t::load( tx_smalltile + numsmalltile, image, INVALID_KEY );

            numsmalltile++;
        }
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

    for ( y = 0, y1 = 0; y < sz_y; y += step_y, y1 += SMALLXY )
    {
        for ( x = 0, x1 = 0; x < sz_x; x += step_x, x1 += SMALLXY )
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

            oglx_texture_t::ctor( tx_bigtile + numbigtile );

            image = cartman_CreateSurface( SMALLXY, SMALLXY );
            SDL_FillRect( image, NULL, MAKE_BGR( image, 0, 0, 0 ) );

            SDL_SoftStretch( bmpload, &src1, image, NULL );

            oglx_texture_t::load( tx_bigtile + numbigtile, image, INVALID_KEY );

            numbigtile++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void get_tiles( SDL_Surface* bmpload )
{
    get_small_tiles( bmpload );
    get_big_tiles( bmpload );
}

//--------------------------------------------------------------------------------------------
SDL_Surface *cartman_CreateSurface(int w, int h)
{
    if ( NULL == theSurface ) return NULL;

    // Expand the screen format to support alpha.
    // a) Copy the format of the main surface.
    SDL_PixelFormat format;
    memcpy(&format, theSurface->format, sizeof(SDL_PixelFormat));
    // b) Expand the format.
    SDLX_ExpandFormat(&format);

    return SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, format.BitsPerPixel, format.Rmask, format.Gmask, format.Bmask, format.Amask);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void gfx_system_init_SDL_graphics()
{
    if (_sdl_initialized_graphics) return;

    cartman_init_SDL_base();

    log_info("Intializing SDL Video ... ");
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
        log_message(" failure!\n");
        log_warning("SDL error == \"%s\"\n", SDL_GetError());
    }
    else
    {
        log_message(" success!\n");
    }

#if !defined(__APPLE__)
    {
        // Setup the cute windows manager icon, don't do this on Mac.
        const std::string fileName = "icon.bmp";
        auto pathName = "mp_data/" + fileName;
        SDL_Surface *theSurface = IMG_Load_RW(vfs_openRWopsRead(pathName.c_str()), 1);
        if (!theSurface)
        {
            log_warning("unable to load icon `%s` - reason: %s\n", pathName.c_str(), SDL_GetError());
        }
        else
        {
            SDL_WM_SetIcon(theSurface, nullptr);
        }
    }
#endif

    // Set the window name
    SDL_WM_SetCaption(NAME " " VERSION_STR, NAME);

#if defined(__unix__)

    // GLX doesn't differentiate between 24 and 32 bpp, asking for 32 bpp
    // will cause SDL_SetVideoMode to fail with:
    // "Unable to set video mode: Couldn't find matching GLX visual"
    if (32 == egoboo_config_t::get().graphic_colorBuffer_bitDepth.getValue())
        egoboo_config_t::get().graphic_colorBuffer_bitDepth.setValue(24);
    if (32 == egoboo_config_t::get().graphic_depthBuffer_bitDepth.getValue())
        egoboo_config_t::get().graphic_depthBuffer_bitDepth.setValue(24);

#endif

    // The flags to pass to SDL_SetVideoMode.
    SDLX_video_parameters_t::download(&sdl_vparam, &egoboo_config_t::get());

    sdl_vparam.flags.opengl = SDL_TRUE;
    sdl_vparam.flags.double_buf = SDL_TRUE;
    sdl_vparam.gl_att.accelerated_visual = GL_TRUE;
    sdl_vparam.gl_att.accum[0] = 8;
    sdl_vparam.gl_att.accum[1] = 8;
    sdl_vparam.gl_att.accum[2] = 8;
    sdl_vparam.gl_att.accum[3] = 8;

    oglx_video_parameters_t::download(&ogl_vparam, &egoboo_config_t::get());

    log_info("Opening SDL Video Mode...\n");

    // actually set the video mode
    if (NULL == SDL_GL_set_mode(NULL, &sdl_vparam, &ogl_vparam, _sdl_initialized_graphics))
    {
        log_message("Failed!\n");
        log_error("I can't get SDL to set any video mode: %s\n", SDL_GetError());
    }
    else
    {
        GFX_WIDTH = (float)GFX_HEIGHT / (float)sdl_vparam.verticalResolution * (float)sdl_vparam.horizontalResolution;
        log_message("Success!\n");
    }

    _sdl_initialized_graphics = SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
int gfx_init_ogl()
{
    using namespace Ego;
    using namespace Ego::Math;
    gfx_system_init_SDL_graphics();

    auto& renderer = Renderer::get();
    // Set clear colour.
    renderer.getColourBuffer().setClearValue(Colour4f(0, 0, 0, 0)); // Set black/transparent background.
    
    // Set clear depth.
    renderer.getDepthBuffer().setClearValue(1.0f);

    // Enable writing to the depth buffer.
    renderer.setDepthWriteEnabled(true);

    // Enable depth testing: Incoming fragment's depth value must be less.
    renderer.setDepthTestEnabled(true);
    renderer.setDepthFunction(CompareFunction::Less);

    // Disable blending.
    renderer.setBlendingEnabled(false);

    // do not display the completely transparent portion
    renderer.setAlphaTestEnabled(true);
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
    renderer.getAccumulationBuffer().setClearValue(Colour4f(0.0f, 0.0f, 0.0f, 1.0f));
    renderer.getAccumulationBuffer().clear();

    // Load the current graphical settings
    // gfx_system_load_assets();

    _ogl_initialized = true;

    return _ogl_initialized && _sdl_initialized_graphics;
}
