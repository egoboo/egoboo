#pragma once

#include "egoboo_types.h"



#define MAXMESHFAN                      (512*512)      // Terrain mesh size
#define MAXMESHSIZEY                    1024           // Max fans in y direction
#define BYTESFOREACHVERTEX              (3*sizeof(float) + 6*sizeof(int))
#define MAXMESHVERTICES                 16             // Fansquare vertices
#define MAXMESHTYPE                     64             // Number of fansquare command types
#define MAXMESHCOMMAND                  4              // Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES           32             // Fansquare command list size
#define MAXMESHCOMMANDSIZE              32             // Max trigs in each command
#define MAXTILETYPE                     256            // Max number of tile images
#define MAXMESHRENDER                   1024*8           // Max number of tiles to draw
#define INVALID_TILE                    ((Uint16)(-1)) // Don't draw the fansquare if tile = this
#define INVALID_FAN                     ((Uint32)(-1)) // Character not on a fan ( maybe )

#define FAN_BITS 7
#define MESH_FAN_TO_INT(XX)    ( (XX) << FAN_BITS )
#define MESH_INT_TO_FAN(XX)    ( (XX) >> FAN_BITS )
#define MESH_FAN_TO_FLOAT(XX)  ( (float)(XX) * (float)(1<<FAN_BITS) )
#define MESH_FLOAT_TO_FAN(XX)  ( (XX) / (float)(1<<FAN_BITS))

#define BLOCK_BITS 9
#define MESH_BLOCK_TO_INT(XX)    ( (XX) << BLOCK_BITS )
#define MESH_INT_TO_BLOCK(XX)    ( (XX) >> BLOCK_BITS )
#define MESH_BLOCK_TO_FLOAT(XX)  ( (float)(XX) * (float)(1<<BLOCK_BITS) )
#define MESH_FLOAT_TO_BLOCK(XX)  ( (XX) / (float)(1<<BLOCK_BITS))

typedef enum mesh_effects_e
{
  MESHFX_REF                     =      0,         // 0 This tile is drawn 1st
  MESHFX_NOREFLECT               = 1 << 0,         // 0 This tile IS reflected in the floors
  MESHFX_SHINY                   = 1 << 1,         // 1 Draw reflection of characters
  MESHFX_ANIM                    = 1 << 2,         // 2 Animated tile ( 4 frame )
  MESHFX_WATER                   = 1 << 3,         // 3 Render water above surface ( Water details are set per module )
  MESHFX_WALL                    = 1 << 4,         // 4 Wall ( Passable by ghosts, particles )
  MESHFX_IMPASS                  = 1 << 5,         // 5 Impassable
  MESHFX_DAMAGE                  = 1 << 6,         // 6 Damage
  MESHFX_SLIPPY                  = 1 << 7          // 7 Ice or normal
} MESHFX_BITS;

#define SLOPE                           800     // Slope increments for terrain normals
#define SLIDE                           .04         // Acceleration for steep hills
#define SLIDEFIX                        .08         // To make almost flat surfaces flat

typedef enum fan_type_t
{
  GROUND_1,  //  0  Two Faced Ground...
  GROUND_2,   //  1  Two Faced Ground...
  GROUND_3,   //  2  Four Faced Ground...
  GROUND_4,   //  3  Eight Faced Ground...
  PILLAR_1,   //  4  Ten Face Pillar...
  PILLAR_2,   //  5  Eighteen Faced Pillar...
  BLANK_1,    //  6  Blank...
  BLANK_2,    //  7  Blank...
  WALL_WE,    //  8  Six Faced Wall (WE)...
  WALL_NS,    //  9  Six Faced Wall (NS)...
  BLANK_3,    //  10  Blank...
  BLANK_4,    //  11  Blank...
  WALL_W,     //  12  Eight Faced Wall (W)...
  WALL_N,     //  13  Eight Faced Wall (N)...
  WALL_E,     //  14  Eight Faced Wall (E)...
  WALL_S,     //  15  Eight Faced Wall (S)...
  CORNER_WS,  //  16  Ten Faced Wall (WS)...
  CORNER_NW,  //  17  Ten Faced Wall (NW)...
  CORNER_NE,  //  18  Ten Faced Wall (NE)...
  CORNER_ES,  //  19  Ten Faced Wall (ES)...
  ALCOVE_WSE, //  20  Twelve Faced Wall (WSE)...
  ALCOVE_NWS, //  21  Twelve Faced Wall (NWS)...
  ALCOVE_ENW, //  22  Twelve Faced Wall (ENW)...
  ALCOVE_SEN, //  23  Twelve Faced Wall (SEN)...
  STAIR_WE,   //  24  Twelve Faced Stair (WE)...
  STAIR_NS    //  25  Twelve Faced Stair (NS)...
} FAN_TYPE;

extern bool_t  meshexploremode;                            // Explore mode?


extern int     meshsizex;                                          // Size in fansquares
extern int     meshsizey;                                          //
extern float   meshedgex;                                          // Limits !!!BAD!!!
extern float   meshedgey;                                          //
extern Uint16  meshlasttexture;                                    // Last texture used
extern Uint8   meshtype[MAXMESHFAN];                               // Command type
extern Uint8   meshfx[MAXMESHFAN];                                 // Special effects flags
extern Uint8   meshtwist[MAXMESHFAN];                              //
extern bool_t  meshinrenderlist[MAXMESHFAN];                       //
extern Uint16  meshtile[MAXMESHFAN];                               // Get texture from this
extern Uint32  meshvrtstart[MAXMESHFAN];                           // Which vertex to start at
extern vect3   meshvrtmins[MAXMESHFAN];                            // what is the minimum extent of the fan
extern vect3   meshvrtmaxs[MAXMESHFAN];                            // what is the maximum extent of the tile
extern Uint32  meshblockstart[( MAXMESHSIZEY/4 ) +1];
extern Uint32  meshfanstart[MAXMESHSIZEY];                         // Which fan to start a row with
extern float*  floatmemory;                                                 // For malloc
extern float*  meshvrtx;                                           // Vertex position
extern float*  meshvrty;                                           //
extern float*  meshvrtz;                                           // Vertex elevation
extern Uint8*  meshvrtar_fp8;                                           // Vertex base light
extern Uint8*  meshvrtag_fp8;                                           // Vertex base light
extern Uint8*  meshvrtab_fp8;                                           // Vertex base light
extern Uint8*  meshvrtlr_fp8;                                           // Vertex light
extern Uint8*  meshvrtlg_fp8;                                           // Vertex light
extern Uint8*  meshvrtlb_fp8;                                           // Vertex light
extern Uint8   meshcommands[MAXMESHTYPE];                          // Number of commands
extern Uint8   meshcommandsize[MAXMESHTYPE][MAXMESHCOMMAND];       // Entries in each command
extern Uint16  meshcommandvrt[MAXMESHTYPE][MAXMESHCOMMANDENTRIES]; // Fansquare vertex list
extern Uint8   meshcommandnumvertices[MAXMESHTYPE];                // Number of vertices
extern Uint8   meshcommandref[MAXMESHTYPE][MAXMESHVERTICES];       // Lighting references
extern float   meshcommandu[MAXMESHTYPE][MAXMESHVERTICES];         // Vertex texture posi
extern float   meshcommandv[MAXMESHTYPE][MAXMESHVERTICES];         //
extern float   meshtileoffu[MAXTILETYPE];                          // Tile texture offset
extern float   meshtileoffv[MAXTILETYPE];                          //


bool_t get_mesh_memory();
void free_mesh_memory();
void load_mesh_fans();
void make_fanstart();
void make_twist();
bool_t load_mesh( char *modname );
Uint32 mesh_get_fan( vect3 pos );
Uint32 mesh_get_block( vect3 pos );

bool_t mesh_calc_normal_fan( int fan, vect3 * pnrm, vect3 * ppos );
bool_t mesh_calc_normal_pos( int fan, vect3 pos, vect3 * pnrm );
bool_t mesh_calc_normal( vect3 pos, vect3 * pnrm );

void mesh_set_colora( int fan_x, int fan_y, int color );

bool_t mesh_clear_fan_bits( int fan_x, int fan_y, Uint32 bits );
bool_t mesh_add_fan_bits( int fan_x, int fan_y, Uint32 bits );
bool_t mesh_set_fan_bits( int fan_x, int fan_y, Uint32 bits );

int mesh_bump_tile( int fan_x, int fan_y );
Uint32 mesh_get_tile( int fan_x, int fan_y );
bool_t mesh_set_tile( int fan_x, int fan_y, Uint32 become );

Uint32 mesh_convert_fan( int fan_x, int fan_y );
Uint32 mesh_convert_block( int block_x, int block_y );

float mesh_fraction_x( float x );
float mesh_fraction_y( float y );

bool_t mesh_in_renderlist( int fan );
void mesh_remove_renderlist( int fan );
void mesh_add_renderlist( int fan );

float mesh_clip_x( float x );
float mesh_clip_y( float y );
int mesh_clip_fan_x( int fan_x );
int mesh_clip_fan_y( int fan_y );
int mesh_clip_block_x( int block_x );
int mesh_clip_block_y( int block_y );

bool_t mesh_check( float x, float y );

Uint32 mesh_hitawall( vect3 pos, float size_x, float size_y, Uint32 collision_bits );

Uint32 mesh_test_bits( int fan, Uint32 bits );
bool_t mesh_has_some_bits( int fan, Uint32 bits );
bool_t mesh_has_no_bits( int fan, Uint32 bits );
bool_t mesh_has_all_bits( int fan, Uint32 bits );

Uint8 mesh_get_twist( int fan );