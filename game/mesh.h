#pragma once

#include "egoboo_types.inl"
#include "egoboo_math.h"

#define MAPID                           0x4470614d     // The string 'MapD'

#define MAXMESHFAN                      (512*512)      // Terrain mesh size
#define MAXMESHSIZEY                    1024           // Max fans in y direction
#define BYTESFOREACHVERTEX              (3*sizeof(float) + 6*sizeof(Uint8))
#define MAXMESHVERTICES                 16             // Fansquare vertices
#define MAXMESHTYPE                     64             // Number of fansquare command types
#define MAXMESHCOMMAND                  4              // Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES           32             // Fansquare command list size
#define MAXMESHCOMMANDSIZE              32             // Max trigs in each command
#define MAXTILETYPE                     (64*4)         // Max number of tile images
#define MAXMESHRENDER                   (1024*8)       // Max number of tiles to draw
#define INVALID_TILE                    (~(Uint16)0)   // Don't draw the fansquare if tile = this
#define INVALID_FAN                     (~(Uint32)0)   // Character not on a fan ( maybe )

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

typedef enum mpd_effect_bits_e
{
  MPDFX_REF                     =      0,         // 0 This tile is drawn 1st
  MPDFX_NOREFLECT               = 1 << 0,         // 0 This tile IS reflected in the floors
  MPDFX_SHINY                   = 1 << 1,         // 1 Draw reflection of characters
  MPDFX_ANIM                    = 1 << 2,         // 2 Animated tile ( 4 frame )
  MPDFX_WATER                   = 1 << 3,         // 3 Render water above surface ( Water details are set per module )
  MPDFX_WALL                    = 1 << 4,         // 4 Wall ( Passable by ghosts, particles )
  MPDFX_IMPASS                  = 1 << 5,         // 5 Impassable
  MPDFX_DAMAGE                  = 1 << 6,         // 6 Damage
  MPDFX_SLIPPY                  = 1 << 7          // 7 Ice or normal
} MPDFX_BITS;

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

extern Uint32  Mesh_Block_X[( MAXMESHSIZEY/4 ) +1];
extern Uint32  Mesh_Fan_X[MAXMESHSIZEY];                         // Which fan to start a row with

typedef struct mesh_info_t
{
  bool_t  exploremode;                            // Explore mode?
  int     size_x;                                          // Size in fansquares
  int     size_y;                                          //
  float   edge_x;                                          // Limits !!!BAD!!!
  float   edge_y;                                          //
  Uint16  last_texture;                                    // Last texture used
} MESH_INFO;

extern MESH_INFO mesh;


typedef struct mesh_fan_t
{
  Uint8   type;                               // Command type
  Uint8   fx;                                 // Special effects flags
  Uint8   twist;                              //
  bool_t  inrenderlist;                       //
  Uint16  tile;                               // Get texture from this
  Uint32  vrt_start;                          // Which vertex to start at
  AA_BBOX bbox;                               // what is the minimum extent of the fan
} MESH_FAN;

extern MESH_FAN Mesh_Fan[MAXMESHFAN];


typedef struct mesh_memory_t
{
  float*  base;                                                 // For malloc
  float*  vrt_x;                                           // Vertex position
  float*  vrt_y;                                           //
  float*  vrt_z;                                           // Vertex elevation
  Uint8*  vrt_ar_fp8;                                           // Vertex base light
  Uint8*  vrt_ag_fp8;                                           // Vertex base light
  Uint8*  vrt_ab_fp8;                                           // Vertex base light
  Uint8*  vrt_lr_fp8;                                           // Vertex light
  Uint8*  vrt_lg_fp8;                                           // Vertex light
  Uint8*  vrt_lb_fp8;                                           // Vertex light
} MESH_MEMORY;

extern MESH_MEMORY Mesh_Mem;

typedef struct mesh_command_t
{
  Uint8   cmd_count;                  // Number of commands
  Uint8   cmd_size[MAXMESHCOMMAND];   // Entries in each command
  Uint16  vrt[MAXMESHCOMMANDENTRIES]; // Fansquare vertex list

  Uint8   vrt_count;                  // Number of vertices
  Uint8   ref[MAXMESHVERTICES];       // Lighting references
  vect2   tx[MAXMESHVERTICES];
} MESH_COMMAND;

MESH_COMMAND Mesh_Cmd[MAXMESHTYPE];

typedef struct mesh_tile_t
{
  vect2   off;                          // Tile texture offset
} MESH_TILE;

extern MESH_TILE Mesh_Tile[MAXTILETYPE];

typedef struct bumplist_node_t
{
  Uint32 ref;
  Uint32 next; 
} BUMPLIST_NODE;

typedef struct bumplist_t
{
  bool_t   valid;
  Uint32   num_blocks;              // Number of collision areas

  Uint16 * chr;                     // For character collisions
  Uint16 * num_chr;                 // Number on the block
  BUMPLIST_NODE * chr_list;         //

  Uint16 * prt;                     // For particle collisions
  Uint16 * num_prt;                 // Number on the block
  BUMPLIST_NODE * prt_list;         //
} BUMPLIST;

BUMPLIST * bumplist_new(BUMPLIST * b);
void       bumplist_delete(BUMPLIST * b);
BUMPLIST * bumplist_renew(BUMPLIST * b);
bool_t     bumplist_allocate(BUMPLIST * b, int size);

bool_t     bumplist_insert_chr(BUMPLIST * b, Uint32 block, CHR_REF chr_ref);
bool_t     bumplist_insert_prt(BUMPLIST * b, Uint32 block, PRT_REF prt_ref);
CHR_REF    bumplist_get_next_chr(BUMPLIST * b, CHR_REF ichr );
PRT_REF    bumplist_get_next_prt(BUMPLIST * b, PRT_REF iprt );

CHR_REF    bumplist_get_chr_head(BUMPLIST * b, Uint32 block);
PRT_REF    bumplist_get_prt_head(BUMPLIST * b, Uint32 block);
bool_t     bumplist_clear( BUMPLIST * b );

extern BUMPLIST bumplist;

bool_t get_mesh_memory();
void free_mesh_memory();
bool_t load_mesh_fans();
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

Uint32 mesh_hitawall( vect3 pos, float size_x, float size_y, Uint32 collision_bits, vect3 * nrm );

Uint32 mesh_test_bits( int fan, Uint32 bits );
bool_t mesh_has_some_bits( int fan, Uint32 bits );
bool_t mesh_has_no_bits( int fan, Uint32 bits );
bool_t mesh_has_all_bits( int fan, Uint32 bits );

Uint8 mesh_get_twist( int fan );