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
#include "gltexture.h"
#include "module.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define MAPSIZE 96

#define TABX                            32//16      // Size of little name tag on the bar
#define BARX                            112//216         // Size of bar
#define BARY                            16//8           //
#define NUMTICK                         10//50          // Number of ticks per row
#define TICKX                           8//4           // X size of each tick
#define MAXTICK                         (NUMTICK*10) // Max number of ticks to draw

#define NUMFONTX                        16          // Number of fonts in the bitmap
#define NUMFONTY                        6           //
#define NUMFONT                         (NUMFONTX*NUMFONTY)
#define FONTADD                         4           // Gap between letters
#define NUMBAR                          6           // Number of status bars


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

extern glMatrix mWorld;                       // World Matrix
extern glMatrix mView;                        // View Matrix
extern glMatrix mViewSave;                    // View Matrix initial state
extern glMatrix mProjection;                  // Projection Matrix

/*OpenGL Textures*/
extern  STRING          TxFormatSupported[50]; // OpenGL icon surfaces
extern  Uint8           maxformattypes;

extern  GLTexture       TxIcon[MAXTEXTURE+1];       // OpenGL icon surfaces
extern  GLTexture       TxTitleImage[MAXMODULE];    // OpenGL title image surfaces
extern  GLTexture       TxFont;                     // OpenGL font surface
extern  GLTexture       TxBars;                     // OpenGL status bar surface
extern  GLTexture       TxBlip;                     // OpenGL you are here surface
extern  GLTexture       TxMap;                      // OpenGL map surface
extern  GLTexture       txTexture[MAXTEXTURE];      // All textures

// Minimap stuff
#define MAXBLIP 128
#define NUMBLIP 6             //Blip textures
EXTERN Uint16                  numblip  EQ( 0 );
EXTERN Uint16                  blipx[MAXBLIP];
EXTERN Uint16                  blipy[MAXBLIP];
EXTERN Uint8                   blipc[MAXBLIP];
EXTERN Uint8                   mapon  EQ( bfalse );
EXTERN Uint8                   mapvalid  EQ( bfalse );
EXTERN Uint8                   youarehereon  EQ( bfalse );

// JF - Added so that the video mode might be determined outside of the graphics code
extern SDL_Surface    *displaySurface;

extern bool_t          meshnotexture;
extern Uint16          meshlasttexture;             // Last texture used

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//OPENGL VERTEX
typedef struct
{
    GLfloat x, y, z, w;
    GLfloat r, g, b, a;
    Uint32   color; // should replace r,g,b,a and be called by glColor4ubv
    GLfloat s, t; // u and v in D3D I guess
} GLVERTEX;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_renderlist
{
    int     all_count;                               // Number to render, total
    int     ref_count;                               // ..., is reflected in the floor
    int     sha_count;                               // ..., is not reflected in the floor
    int     drf_count;                               // ..., draws character reflections
    int     ndr_count;                               // ..., draws no character reflections

    Uint32  all[MAXMESHRENDER];                      // List of which to render, total

    Uint32  ref[MAXMESHRENDER];                      // ..., is reflected in the floor
    Uint32  sha[MAXMESHRENDER];                      // ..., is not reflected in the floor

    Uint32  drf[MAXMESHRENDER];                      // ..., draws character reflections
    Uint32  ndr[MAXMESHRENDER];                      // ..., draws no character reflections

};
typedef struct s_renderlist renderlist_t;

extern renderlist_t renderlist;

extern Uint8           lightdirectionlookup[65536];                        // For lighting characters
extern Uint8           lighttable[MAXLIGHTLEVEL][MAXLIGHTROTATION][MD2LIGHTINDICES];
extern float           indextoenvirox[MD2LIGHTINDICES];                    // Environment map
extern float           lighttoenviroy[256];                                // Environment map
extern Uint32          lighttospek[MAXSPEKLEVEL][256];                     //

// camera optimization

#define ROTMESHTOPSIDE                  55          // For figuring out what to draw
#define ROTMESHBOTTOMSIDE               65          //
#define ROTMESHUP                       40 //35          //
#define ROTMESHDOWN                     60          //

extern int rotmeshtopside;                                 // The ones that get used
extern int rotmeshbottomside;                              //
extern int rotmeshup;                                      //
extern int rotmeshdown;                                    //

// Lightning effects

#define MAXDYNADIST                     2700        // Leeway for offscreen lights
#define TOTALMAXDYNA                    64          // Absolute max number of dynamic lights

struct s_dynalight
{
    int   distance;      // The distances
    float x;             // Light position
    float y;             //
    float level;         // Light intensity
    float falloff;       // Light radius
};

typedef struct s_dynalight dynalight_t;


extern int         dyna_distancetobeat;           // The number to beat
extern int         dyna_list_max;                 // Max number of lights to draw
extern int         dyna_list_count;               // Number of dynamic lights
extern dynalight_t dyna_list[TOTALMAXDYNA];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Function prototypes
void draw_blip( float sizeFactor, Uint8 color, int x, int y );
int  get_free_message();
void create_szfpstext( int frames );
void figure_out_what_to_draw();
void order_dolist();
void add_to_dolist( Uint16 cnt );

void make_lighttospek();
void make_lighttable( float lx, float ly, float lz, float ambi );
void make_lightdirectionlookup();
void make_renderlist();
void make_dolist();

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
void draw_scene_zreflection();
void animate_tiles();
void move_water();

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

bool_t dump_screenshot();

void make_enviro();
float light_for_normal( int rotation, int normal, float lx, float ly, float lz, float ambi );

bool_t load_one_icon(  const char *szLoadName );

void make_textureoffset();
void tile_dictionary_load();

void load_basic_textures(  const char *modname );

void clear_messages();

void font_load(  const char* szBitmap,  const char* szSpacing );

void make_water();
void read_wawalite(  const char *modname );

int  load_one_title_image( int titleimage,  const char *szLoadName );

void font_init();