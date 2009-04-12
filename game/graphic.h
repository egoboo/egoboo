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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXLIGHTLEVEL                   16          // Number of premade light intensities
#define MAXSPEKLEVEL                    16          // Number of premade specularities
#define MAXLIGHTROTATION                256         // Number of premade light maps

#define DONTFLASH                       255         //
#define SEEKURSEAND                     31          // Blacking flash

// Special Textures
typedef enum e_tx_type
{
    TX_PARTICLE = 0,
    TX_TILE_0,
    TX_TILE_1,
    TX_TILE_2,
    TX_TILE_3,
    TX_WATER_TOP,
    TX_WATER_LOW,
    TX_PHONG,
    TX_LAST
} TX_TYPE;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern Uint16           dolist[MAXCHR];             // List of which characters to draw
extern Uint16           numdolist;                  // How many in the list

// JF - Added so that the video mode might be determined outside of the graphics code
extern SDL_Surface    *displaySurface;

extern Uint16          meshlasttexture;             // Last texture used

extern int             numrenderlistall;                               // Number to render, total
extern int             numrenderlistref;                               // ..., reflective
extern int             numrenderlistsha;                               // ..., shadow
extern Uint32          renderlistall[MAXMESHRENDER];                   // List of which to render, total
extern Uint32          renderlistref[MAXMESHRENDER];                   // ..., reflective
extern Uint32          renderlistsha[MAXMESHRENDER];                   // ..., shadow

extern Uint8           lightdirectionlookup[65536];                        // For lighting characters
extern Uint8           lighttable[MAXLIGHTLEVEL][MAXLIGHTROTATION][MD2LIGHTINDICES];
extern float           indextoenvirox[MD2LIGHTINDICES];                    // Environment map
extern float           lighttoenviroy[256];                                // Environment map
extern Uint32          lighttospek[MAXSPEKLEVEL][256];                     //

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Function prototypes
void draw_blip( float sizeFactor, Uint8 color, int x, int y );
int get_free_message();
void create_szfpstext( int frames );
void figure_out_what_to_draw();
void order_dolist();
void add_to_dolist( Uint16 cnt );

void make_lighttospek();
void make_lighttable( float lx, float ly, float lz, float ambi );
void make_lightdirectionlookup();
void make_renderlist();
void make_dolist();
void make_vrtstart();
void make_fanstart();

void init_all_icons();
void init_all_titleimages();
void init_bars();
void init_blip();
void init_map();
void init_all_textures();
void init_all_models();

void release_all_icons();
void release_all_titleimages();
void release_bars();
void release_blip();
void release_map();
void release_all_textures();
void release_all_models();

void   load_graphics();
bool_t load_blip_bitmap();
void   load_bars(  const char* szBitmap );
void   load_map(  const char* szModule );
bool_t load_all_global_icons();

float light_for_normal( int rotation, int normal, float lx, float ly, float lz, float ambi );
void  make_lighttable( float lx, float ly, float lz, float ambi );


void render_water();
void draw_scene_sadreflection();
void draw_scene_zreflection();

void draw_one_icon( int icontype, int x, int y, Uint8 sparkle );
void draw_one_font( int fonttype, int x, int y );
void draw_map( int x, int y );
int  draw_one_bar( int bartype, int x, int y, int ticks, int maxticks );
void draw_string(  const char *szText, int x, int y );
int  length_of_word(  const char *szText );
int  draw_wrap_string(  const char *szText, int x, int y, int maxx );
int  draw_status( Uint16 character, int x, int y );
void draw_text();
void flip_pages();
void draw_scene();
void draw_main();

void render_prt();
void render_shadow( Uint16 character );
void render_bad_shadow( Uint16 character );
void render_refprt();
void render_fan( Uint32 fan );
void render_water_fan( Uint32 fan, Uint8 layer );
void render_enviromad( Uint16 character, Uint8 trans );
void render_texmad( Uint16 character, Uint8 trans );
void render_mad( Uint16 character, Uint8 trans );
void render_refmad( int tnc, Uint8 trans );

void light_characters();
void light_particles();
void set_fan_light( int fanx, int fany, Uint16 particle );
void do_dynalight();

void do_cursor();

void sdlinit( int argc, char **argv );
int glinit( int argc, char **argv );

void   check_screenshot();
bool_t dump_screenshot();

void make_enviro();
float light_for_normal( int rotation, int normal, float lx, float ly, float lz, float ambi );
