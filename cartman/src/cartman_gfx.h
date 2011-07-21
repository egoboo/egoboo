#pragma once

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

#include <egolib.h>

#include "cartman_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_window;
struct s_cartman_mpd;
struct s_Font;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_camera;
typedef struct s_camera camera_t;

struct s_simple_vertex;
typedef struct s_simple_vertex simple_vertex_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_ogl_surface
{
    GLint viewport[4];
};

//--------------------------------------------------------------------------------------------

struct s_camera
{
    float x;       // the position of the center of the window
    float y;       //
    float z;

    float w;       // the size of the window
    float h;       //
    float d;       //
};

//--------------------------------------------------------------------------------------------
struct s_simple_vertex
{
    GLfloat x, y, z;
    GLfloat s, t;
    GLfloat r, g, b, a;

    GLfloat l;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXTILE 256             //

#define OGL_MAKE_COLOR_3(COL, BB,GG,RR) { COL[0] = RR / 32.0f; COL[1] = GG / 32.0f; COL[2] = BB / 32.0f; }
#define OGL_MAKE_COLOR_4(COL, AA,BB,GG,RR) { COL[0] = RR / 32.0f; COL[1] = GG / 32.0f; COL[2] = BB / 32.0f; COL[3] = AA / 32.0f; }

#define MAKE_BGR(BMP,BB,GG,RR)     SDL_MapRGBA(BMP->format, (RR)<<3, (GG)<<3, (BB)<<3, 0xFF)
#define MAKE_ABGR(BMP,AA,BB,GG,RR) SDL_MapRGBA(BMP->format, (RR)<<3, (GG)<<3, (BB)<<3, AA)

#define POINT_SIZE(X) ( (X) * 0.5f + 4.0f )
#define MAXPOINTSIZE 16.0f

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern camera_t cam;

extern SDL_Surface * theSurface;
extern SDL_Surface * bmphitemap;        // Heightmap image

extern oglx_texture_t     tx_point;      // Vertex image
extern oglx_texture_t     tx_pointon;    // Vertex image ( select_vertsed )
extern oglx_texture_t     tx_ref;        // Meshfx images
extern oglx_texture_t     tx_drawref;    //
extern oglx_texture_t     tx_anim;       //
extern oglx_texture_t     tx_water;      //
extern oglx_texture_t     tx_wall;       //
extern oglx_texture_t     tx_impass;     //
extern oglx_texture_t     tx_damage;     //
extern oglx_texture_t     tx_slippy;     //

extern oglx_texture_t     tx_smalltile[MAXTILE]; // Tiles
extern oglx_texture_t     tx_bigtile[MAXTILE];   //
extern oglx_texture_t     tx_tinysmalltile[MAXTILE]; // Plan tiles
extern oglx_texture_t     tx_tinybigtile[MAXTILE];   //

extern int     numsmalltile;   //
extern int     numbigtile;     //

extern SDLX_video_parameters_t sdl_vparam;
extern oglx_video_parameters_t ogl_vparam;

extern int    animtileupdateand;                      // New tile every ( (1 << n) - 1 ) frames
extern Uint16 animtileframeand;                       // 1 << n frames
extern Uint16 animtilebaseand;
extern Uint16 biganimtileframeand;                    // 1 << n frames
extern Uint16 biganimtilebaseand;
extern Uint16 animtileframeadd;

extern Sint16 damagetileparttype;
extern short  damagetilepartand;
extern short  damagetilesound;
extern short  damagetilesoundtime;
extern Uint16 damagetilemindistance;
extern int    damagetileamount;                           // Amount of damage
extern Uint8  damagetiletype;                      // Type of damage

extern int GFX_WIDTH;
extern int GFX_HEIGHT;

extern const SDL_Color cart_white;
extern const SDL_Color cart_black;

extern struct s_Font * gfx_font_ptr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// misc
SDL_Color MAKE_SDLCOLOR( Uint8 BB, Uint8 RR, Uint8 GG );

// make a bitmap of the mesh
void make_hitemap( struct s_cartman_mpd * pmesh );
void make_planmap( struct s_cartman_mpd * pmesh );

// tile rendering routines
void draw_top_fan( struct s_window * pwin, int fan, float zoom_hrz );
void draw_side_fan( struct s_window * pwin, int fan, float zoom_hrz, float zoom_vrt );
void draw_schematic( struct s_window * pwin, int fantype, int x, int y );
void draw_top_tile( float x0, float y0, int fan, oglx_texture_t * tx_tile, bool_t draw_tile, struct s_cartman_mpd * pmesh );
void draw_tile_fx( float x, float y, Uint8 fx, float scale );

// ogl routines
void ogl_draw_sprite_2d( oglx_texture_t * img, float x, float y, float width, float height );
void ogl_draw_sprite_3d( oglx_texture_t * img, cart_vec_t pos, cart_vec_t vup, cart_vec_t vright, float width, float height );
void ogl_draw_box( float x, float y, float w, float h, float color[] );
void ogl_beginFrame();
void ogl_endFrame();

// SDL routines
void draw_sprite( SDL_Surface * dst, SDL_Surface * sprite, int x, int y );
int cartman_BlitScreen( SDL_Surface * bmp, SDL_Rect * prect );
SDL_Surface * cartman_CreateSurface( int w, int h );
int cartman_BlitSurface( SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect );
SDL_Surface * cartman_LoadIMG( const char * szName );

// camera stuff
void cartman_begin_ortho_camera_hrz( struct s_window * pwin, struct s_camera * pcam, float zoom_x, float zoom_y );
void cartman_begin_ortho_camera_vrt( struct s_window * pwin, struct s_camera * pcam, float zoom_x, float zoom_z );
void cartman_end_ortho_camera( );

// setup
void create_imgcursor( void );
void load_img( void );
void get_tiles( SDL_Surface* bmpload );

// misc
oglx_texture_t * tiny_tile_at( struct s_cartman_mpd * pmesh, int x, int y );
oglx_texture_t * tile_at( struct s_cartman_mpd * pmesh, int fan );

// initialization
void gfx_system_begin();
void gfx_system_end();
