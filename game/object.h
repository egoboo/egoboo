#pragma once

#include "egoboo_types.h"
#include "egoboo_math.h"

#define MAXPROFILE 1024

typedef struct collision_volume_t
{
  int   level;
  float x_min, x_max;
  float y_min, y_max;
  float z_min, z_max;
  float xy_min, xy_max;
  float yx_min, yx_max;
} CVolume;

typedef CVolume CVolume_Tree[8];

typedef struct bump_data_t
{
  Uint8 shadow;      // Size of shadow
  Uint8 size;        // Size of bumpers
  Uint8 sizebig;     // For octagonal bumpers
  Uint8 height;      // Distance from head to toe

  bool_t calc_is_platform;
  bool_t calc_is_mount;

  float  calc_size;
  float  calc_size_big;
  float  calc_height;

  CVolume        cv;
  CVolume_Tree * cv_tree;
  vect3   mids_hi, mids_lo;
} BData;


typedef enum team_e
{
  TEAM_EVIL            = 'E' -'A',                      // E
  TEAM_GOOD            = 'G' -'A',                      // G
  TEAM_NULL            = 'N' -'A',                      // N
  TEAM_ZIPPY           = 'Z' -'A',
  TEAM_DAMAGE,                                          // For damage tiles
  TEAM_COUNT                                              // Teams A-Z, +1 more for damage tiles
} TEAM;

typedef struct team_t
{
  bool_t  hatesteam[TEAM_COUNT];     // Don't damage allies...
  Uint16  morale;                 // Number of characters on team
  CHR_REF leader;                 // The leader of the team
  CHR_REF sissy;                  // Whoever called for help last
} TEAM_INFO;

extern TEAM_INFO TeamList[TEAM_COUNT];



#define VALID_TEAM(XX) ( ((XX)>=0) && ((XX)<TEAM_COUNT) )

CHR_REF team_get_sissy( TEAM_REF iteam );
CHR_REF team_get_leader( TEAM_REF iteam );



typedef struct vertex_data_blended_t
{
  Uint32  frame0;
  Uint32  frame1;
  Uint32  vrtmin;
  Uint32  vrtmax;
  float   lerp;
  bool_t  needs_lighting;

  // Storage for blended vertices
  vect3 *Vertices;
  vect3 *Normals;
  vect4 *Colors;
  vect2 *Texture;
  float *Ambient;      // Lighting hack ( Ooze )
} VData_Blended;

void VData_Blended_construct(VData_Blended * v);
void VData_Blended_destruct(VData_Blended * v);
VData_Blended * VData_Blended_new();
void VData_Blended_delete(VData_Blended * v);
void VData_Blended_Allocate(VData_Blended * v, size_t verts);
void VData_Blended_Deallocate(VData_Blended * v);




typedef enum damage_e
{
  DAMAGE_SLASH   = 0,                          //
  DAMAGE_CRUSH,                                //
  DAMAGE_POKE,                                 //
  DAMAGE_HOLY,                                 // (Most invert Holy damage )
  DAMAGE_EVIL,                                 //
  DAMAGE_FIRE,                                 //
  DAMAGE_ICE,                                  //
  DAMAGE_ZAP,                                  //
  MAXDAMAGETYPE,                              // Damage types
  DAMAGE_NULL     = 255,                       //
} DAMAGE;

#define DAMAGE_SHIFT         3                       // 000000xx Resistance ( 1 is common )
#define DAMAGE_INVERT        4                       // 00000x00 Makes damage heal
#define DAMAGE_CHARGE        8                       // 0000x000 Converts damage to mana
#define DAMAGE_MANA         16                       // 000x0000 Makes damage deal to mana TODO


typedef enum slot_e
{
  SLOT_LEFT,
  SLOT_RIGHT,
  SLOT_SADDLE,          // keep a slot open for a possible "saddle" for future use
  SLOT_COUNT,

  // other values
  SLOT_INVENTORY,        // this is a virtual "slot" that really means the inventory
  SLOT_NONE,

  // aliases
  SLOT_BEGIN = SLOT_LEFT,
} SLOT;

extern SLOT _slot;

#define GRIP_SIZE                        4
#define GRIP_VERTICES                    (2*GRIP_SIZE)   // Each model has 8 grip vertices
typedef enum grip_e
{
  GRIP_ORIGIN   = 0,                        // Grip at mount's origin
  GRIP_LAST     = 1,                        // Grip at mount's last vertex
  GRIP_RIGHT    = (( SLOT_RIGHT + 1 ) * GRIP_SIZE ),  // Grip at mount's right hand
  GRIP_LEFT     = (( SLOT_LEFT + 1 ) * GRIP_SIZE ),  // Grip at mount's left hand

  // other values
  GRIP_NONE,

  // Aliases
  GRIP_SADDLE    = GRIP_LEFT,    // Grip at mount's "saddle" (== left hand for now)
  GRIP_INVENTORY = GRIP_ORIGIN   // "Grip" in the object's inventory
} GRIP;

typedef struct tile_damage_t
{
  short  parttype;
  short  partand;
  Sint8  sound;
  int    amount; //  EQ( 256 );                           // Amount of damage
  DAMAGE type; //  EQ( DAMAGE_FIRE );                      // Type of damage
} TILE_DAMAGE;

extern TILE_DAMAGE GTile_Dam;

#define DELAY_TILESOUND 16
#define TILEREAFFIRMAND  3

SLOT grip_to_slot( GRIP g );
GRIP slot_to_grip( SLOT s );
Uint16 slot_to_latch( Uint16 object, SLOT s );
Uint16 slot_to_offset( SLOT s );


void flash_character_height( CHR_REF character, Uint8 valuelow, Sint16 low,
                             Uint8 valuehigh, Sint16 high );
void flash_character( CHR_REF character, Uint8 value );

void attach_particle_to_character( PRT_REF particle, CHR_REF character, Uint16 vertoffset );


void export_one_character( CHR_REF character, Uint16 owner, int number );
void free_all_characters();
void setup_particles();


void spawn_bump_particles( CHR_REF character, PRT_REF particle );



void disaffirm_attached_particles( CHR_REF character );
Uint16 number_of_attached_particles( CHR_REF character );
void reaffirm_attached_particles( CHR_REF character );

void switch_team( CHR_REF character, TEAM team );
int restock_ammo( CHR_REF character, IDSZ idsz );
void issue_clean( CHR_REF character );

