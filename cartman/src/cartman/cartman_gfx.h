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

#pragma once

#include "cartman/cartman_typedef.h"
#include "cartman/cartman_math.h"

//--------------------------------------------------------------------------------------------

struct ogl_surface_t
{
    GLint viewport[4];
};

//--------------------------------------------------------------------------------------------

struct camera_t
{
    float x;       // the position of the center of the window
    float y;       //
    float z;

    float w;       // the size of the window
    float h;       //
    float d;       //
};

//--------------------------------------------------------------------------------------------

struct simple_vertex_t
{
    GLfloat x, y, z;
    GLfloat s, t;
    GLfloat r, g, b, a;

    GLfloat l;
};

//--------------------------------------------------------------------------------------------

#define MAXTILE 256

inline Ego::Math::Colour4f make_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (Ego::Math::Colour4f)Ego::Math::Colour4b(r, g, b, a);
}

#define POINT_SIZE(X) ( (X) * 0.5f + 4.0f )
#define MAXPOINTSIZE 16.0f

//--------------------------------------------------------------------------------------------

extern camera_t cam;

struct Resources : id::singleton<Resources> {
    std::shared_ptr<SDL_Surface> bmphitemap;     // Heightmap image
    std::shared_ptr<Ego::Texture> tx_point;      // Vertex image
    std::shared_ptr<Ego::Texture> tx_pointon;    // Vertex image ( select_vertsed )
    std::shared_ptr<Ego::Texture> tx_ref;        // Meshfx images
    std::shared_ptr<Ego::Texture> tx_drawref;    //
    std::shared_ptr<Ego::Texture> tx_anim;       //
    std::shared_ptr<Ego::Texture> tx_water;      //
    std::shared_ptr<Ego::Texture> tx_wall;       //
    std::shared_ptr<Ego::Texture> tx_impass;     //
    std::shared_ptr<Ego::Texture> tx_damage;     //
    std::shared_ptr<Ego::Texture> tx_slippy;     //

    std::array<std::shared_ptr<Ego::Texture>, MAXTILE> tx_smalltile;     // Tiles
    std::array<std::shared_ptr<Ego::Texture>, MAXTILE> tx_bigtile;       //
    std::array<std::shared_ptr<Ego::Texture>, MAXTILE> tx_tinysmalltile; // Plan tiles
    std::array<std::shared_ptr<Ego::Texture>, MAXTILE> tx_tinybigtile;   //
};

extern int     numsmalltile;   //
extern int     numbigtile;     //

extern int      animtileupdateand;                      // New tile every ( (1 << n) - 1 ) frames
extern uint16_t animtileframeand;                       // 1 << n frames
extern uint16_t animtilebaseand;
extern uint16_t biganimtileframeand;                    // 1 << n frames
extern uint16_t biganimtilebaseand;
extern uint16_t animtileframeadd;

extern int16_t  damagetileparttype;
extern short    damagetilepartand;
extern short    damagetilesound;
extern short    damagetilesoundtime;
extern uint16_t damagetilemindistance;
extern int      damagetileamount;                       // Amount of damage
extern uint8_t  damagetiletype;                         // Type of damage

extern std::shared_ptr<Ego::Font> gfx_font_ptr;

//--------------------------------------------------------------------------------------------

// make a bitmap of the mesh
void make_hitemap( cartman_mpd_t * pmesh );
void make_planmap( cartman_mpd_t * pmesh );

// tile rendering routines
/// Draw a fan from top perspective in wireframe mode.
/// Draw vertex selection indicators.
void draw_top_fan(select_lst_t& plst, int fan, float zoom_hrz, float zoom_vrt);
/// Draw a fan from side perspective in wireframe mode.
/// Draw vertex selection indicators.
void draw_side_fan(select_lst_t& plst, int fan, float zoom_hrz, float zoom_vrt);

void draw_schematic(std::shared_ptr<Cartman::Gui::Window> pwin, int fantype, int x, int y);
void draw_top_tile( float x0, float y0, int fan, std::shared_ptr<Ego::Texture> tx_tile, bool draw_tile, cartman_mpd_t * pmesh );
void draw_tile_fx( float x, float y, uint8_t fx, float scale );

// ogl routines
void ogl_draw_sprite_2d( std::shared_ptr<Ego::Texture> img, float x, float y, float width, float height );
void ogl_draw_sprite_3d( std::shared_ptr<Ego::Texture> img, cart_vec_t pos, cart_vec_t vup, cart_vec_t vright, float width, float height );
void ogl_draw_box_xy( float x, float y, float z, float w, float h, Ego::Math::Colour4f& colour );
void ogl_draw_box_xz( float x, float y, float z, float w, float d, Ego::Math::Colour4f& colour );
void ogl_beginFrame();
void ogl_endFrame();

// SDL routines
void draw_sprite( SDL_Surface * dst, SDL_Surface * sprite, int x, int y );
SDL_Surface * cartman_LoadIMG( const std::string& szName );

// camera stuff
void cartman_begin_ortho_camera_hrz(Cartman::Gui::Window& pwin, camera_t *pcam, float zoom_x, float zoom_y);
void cartman_begin_ortho_camera_vrt(Cartman::Gui::Window& pwin, camera_t *pcam, float zoom_x, float zoom_z);
void cartman_end_ortho_camera();

// setup

void load_img();
void get_tiles(SDL_Surface* bmpload);

// misc
std::shared_ptr<Ego::Texture> tiny_tile_at( cartman_mpd_t * pmesh, Index2D index2d );
std::shared_ptr<Ego::Texture> tile_at( cartman_mpd_t * pmesh, int fan );

// initialization
struct GFX : Ego::App<GFX>
{
    GFX();
    ~GFX();
};
