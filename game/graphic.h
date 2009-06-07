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

#include "ogl_texture.h"
#include "module.h"
#include "mpd.h"

#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_camera;
struct s_egoboo_config;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define DOLIST_SIZE (MAX_CHR + TOTAL_MAX_PRT)

#define MAPSIZE 96

#define TABX                            32//16      // Size of little name tag on the bar
#define BARX                            112//216         // Size of bar
#define BARY                            16//8
#define NUMTICK                         10//50          // Number of ticks per row
#define TICKX                           8//4           // X size of each tick
#define MAXTICK                         (NUMTICK*10) // Max number of ticks to draw

#define NUMFONTX                        16          // Number of fonts in the bitmap
#define NUMFONTY                        6
#define NUMFONT                         (NUMFONTX*NUMFONTY)
#define FONTADD                         4           // Gap between letters
#define NUMBAR                          6           // Number of status bars

#define MAXLIGHTLEVEL                   16          // Number of premade light intensities
#define MAXSPEKLEVEL                    16          // Number of premade specularities
#define MAXLIGHTROTATION                256         // Number of premade light maps

#define DONTFLASH                       255
#define SEEKURSEAND                     31          // Blacking flash

#define TRANSCOLOR                      0           // Transparent color

// Special Textures
typedef enum e_tx_type
{
    TX_PARTICLE_TRANS = 0,
    TX_PARTICLE_LIGHT,
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
struct s_do_list_data
{
    float  dist;
    Uint16 chr;
};
typedef struct s_do_list_data do_list_data_t;

//--------------------------------------------------------------------------------------------
struct s_obj_registry_entity
{
    Uint16 ichr, iprt;
    float  dist;
};
typedef struct s_obj_registry_entity obj_registry_entity_t;

int obj_registry_entity_cmp( const void * pleft, const void * pright );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//OPENGL VERTEX

enum { XX = 0, YY, ZZ, WW };
enum { RR = 0, GG, BB, AA };
enum { SS = 0, TT };

typedef struct
{
    GLfloat pos[4];
    GLfloat nrm[3];
    GLfloat env[2];

    GLfloat tex[2];
    GLfloat col_dir[4];
    GLuint  color_dir;   // the vertex-dependent, directional lighting

    GLfloat col[4];      // the total vertex-dependent lighting (ambient + directional)
} GLvertex;

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

//--------------------------------------------------------------------------------------------
extern float           light_a, light_x, light_y, light_z;
extern Uint8           lightdirectionlookup[65536];                        // For lighting characters
extern float           lighttable_local[MAXLIGHTROTATION][MD2LIGHTINDICES];
extern float           lighttable_global[MAXLIGHTROTATION][MD2LIGHTINDICES];
extern float           indextoenvirox[MD2LIGHTINDICES];                    // Environment map
extern float           lighttoenviroy[256];                                // Environment map
extern Uint32          lighttospek[MAXSPEKLEVEL][256];

//--------------------------------------------------------------------------------------------
// camera optimization

#define ROTMESHTOPSIDE                  55          // For figuring out what to draw
#define ROTMESHBOTTOMSIDE               65
#define ROTMESHUP                       40
#define ROTMESHDOWN                     60

extern int rotmeshtopside;                                 // The ones that get used
extern int rotmeshbottomside;
extern int rotmeshup;
extern int rotmeshdown;

//--------------------------------------------------------------------------------------------
// Lightning effects

#define MAXDYNADIST                     2700        // Leeway for offscreen lights
#define TOTAL_MAX_DYNA                    64          // Absolute max number of dynamic lights

struct s_dynalight
{
    float distance;      // The distances
    float x;             // Light position
    float y;
    float z;
    float level;         // Light intensity
    float falloff;       // Light radius
};

typedef struct s_dynalight dynalight_t;

extern float       dyna_distancetobeat;           // The number to beat
extern int         dyna_list_max;                 // Max number of lights to draw
extern int         dyna_list_count;               // Number of dynamic lights
extern dynalight_t dyna_list[TOTAL_MAX_DYNA];

//--------------------------------------------------------------------------------------------
// encapsulation of all graphics options
struct s_gfx_config
{
    GLuint shading;
    bool_t refon;
    Uint8  reffadeor;
    bool_t antialiasing;
    int    multisamples;
    bool_t dither;
    bool_t perspective;
    bool_t phongon;
    bool_t shaon;
    bool_t shasprite;

    bool_t clearson;          // Do we clear every time?
    bool_t draw_background;   // Do we draw the background image?
    bool_t draw_overlay;      // Draw overlay?
    bool_t draw_water_0;      // Do we draw water layer 1 (TX_WATER_LOW)
    bool_t draw_water_1;      // Do we draw water layer 2 (TX_WATER_TOP)
};
typedef struct s_gfx_config gfx_config_t;

extern gfx_config_t gfx;

bool_t gfx_config_init ( gfx_config_t * pgfx );
bool_t gfx_config_synch( gfx_config_t * pgfx, struct s_egoboo_config * pcfg );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern obj_registry_entity_t dolist[DOLIST_SIZE];             // List of which characters to draw
extern Uint16                dolist_count;                  // How many in the list

/*OpenGL Textures*/
extern  GLXtexture       TxIcon[MAX_ICON];       // OpenGL icon surfaces
extern  GLXtexture       TxFont;                     // OpenGL font surface
extern  GLXtexture       TxBars;                     // OpenGL status bar surface
extern  GLXtexture       TxBlip;                     // OpenGL you are here surface
extern  GLXtexture       TxMap;                      // OpenGL map surface
extern  GLXtexture       TxTexture[MAX_TEXTURE];      // All textures

extern  Uint32          TxTitleImage_count;
extern  GLXtexture       TxTitleImage[MAXMODULE];    // OpenGL title image surfaces

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
extern bool_t          meshnotexture;
extern Uint16          meshlasttexture;             // Last texture used

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Function prototypes
void draw_blip( float sizeFactor, Uint8 color, int x, int y );
int  get_free_message();
void create_szfpstext( int frames );

void make_lighttospek();
void make_lighttable( float lx, float ly, float lz, float ambi );
void make_lightdirectionlookup();
void make_renderlist( struct s_camera * pcam );

void   dolist_sort( struct s_camera * pcam );
void   dolist_make();
bool_t dolist_add_chr( Uint16 cnt );
bool_t dolist_add_prt( Uint16 cnt );

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
void   load_bars( const char* szBitmap );
void   load_map( const char* szModule );
bool_t load_all_global_icons();

void  make_lighttable( float lx, float ly, float lz, float ambi );

void render_water();
void draw_scene_zreflection( struct s_camera * pcam );
void animate_tiles();
void move_water();

void draw_one_icon( int icontype, int x, int y, Uint8 sparkle );
void draw_one_font( int fonttype, int x, int y );
void draw_map( int x, int y );
int  draw_one_bar( int bartype, int x, int y, int ticks, int maxticks );
void draw_string( const char *szText, int x, int y );
int  length_of_word( const char *szText );
int  draw_wrap_string( const char *szText, int x, int y, int maxx );
int  draw_status( Uint16 character, int x, int y );
void draw_text();
void request_flip_pages();
void do_flip_pages();
void draw_scene( struct s_camera * pcam );
void draw_main();

void render_prt( struct s_camera * pcam );
void render_shadow( Uint16 character );
void render_bad_shadow( Uint16 character );
void render_prt_ref( struct s_camera * pcam );
void render_fan( Uint32 fan );
void render_hmap_fan( Uint32 fan );
void render_water_fan( Uint32 fan, Uint8 layer );
void render_one_mad_enviro( Uint16 character, Uint8 trans );
void render_one_mad_tex( Uint16 character, Uint8 trans );
void render_one_mad( Uint16 character, Uint8 trans );
void render_one_mad_ref( int tnc, Uint8 trans );

void light_characters();
void light_particles();
void set_fan_light( int fanx, int fany, Uint16 particle );
void do_mpd_lighting();

void do_cursor();

int sdl_init();
int ogl_init();

bool_t dump_screenshot();

void make_enviro();

bool_t load_one_icon( const char *szLoadName );

void tile_dictionary_load(tile_definition_t dict[], size_t dict_size);

void load_basic_textures( const char *modname );

void clear_messages();

void font_load( const char* szBitmap, const char* szSpacing );

void make_water();
void read_wawalite( const char *modname );

Uint32 load_one_title_image( const char *szLoadName );

void font_init();

void update_all_prt_instance( struct s_camera * pcam );
bool_t render_one_prt_solid( Uint16 iprt );
bool_t render_one_prt_trans( Uint16 iprt );
bool_t render_one_prt_ref( Uint16 iprt );

void Begin3DMode( struct s_camera * pcam );
void End3DMode();