#pragma once

#include "object.h"

#include "egoboo_math.h"
#include "egoboo.h"

#define MAXMODEL                        MAXPROFILE  // Max number of models
//#define MD2START                        0x32504449  // MD2 files start with these four bytes
#define MD2MAXLOADSIZE                  (512*1024)  // Don't load any models bigger than 512k
#define EQUALLIGHTINDEX                 162         // I added an extra index to do the spikey mace...
#define MD2LIGHTINDICES                 163         // MD2's store vertices as x,y,z,normal

extern float           spek_global[MAXLIGHTROTATION][MD2LIGHTINDICES];
extern float           spek_local[MAXLIGHTROTATION][MD2LIGHTINDICES];
extern float           indextoenvirox[MD2LIGHTINDICES];                    // Environment map
extern float           lighttoenviroy[256];                                // Environment map
extern Uint32          lighttospek[MAXSPEKLEVEL][256];                     //

// This stuff is for actions
typedef enum action_e
{
  ACTION_DA = 0,        // :DA Dance ( Standing still )
  ACTION_DB,            // :DB Dance ( Bored )
  ACTION_DC,            // :DC Dance ( Bored )
  ACTION_DD,            // :DD Dance ( Bored )
  ACTION_UA,            // :UA Unarmed
  ACTION_UB,            // :UB Unarmed
  ACTION_UC,            // :UC Unarmed
  ACTION_UD,            // :UD Unarmed
  ACTION_TA,            // :TA Thrust
  ACTION_TB,            // :TB Thrust
  ACTION_TC,            // :TC Thrust
  ACTION_TD,            // :TD Thrust
  ACTION_CA,            // :CA Crush
  ACTION_CB,            // :CB Crush
  ACTION_CC,            // :CC Crush
  ACTION_CD,            // :CD Crush
  ACTION_SA,            // :SA Slash
  ACTION_SB,            // :SB Slash
  ACTION_SC,            // :SC Slash
  ACTION_SD,            // :SD Slash
  ACTION_BA,            // :BA Bash
  ACTION_BB,            // :BB Bash
  ACTION_BC,            // :BC Bash
  ACTION_BD,            // :BD Bash
  ACTION_LA,            // :LA Longbow
  ACTION_LB,            // :LB Longbow
  ACTION_LC,            // :LC Longbow
  ACTION_LD,            // :LD Longbow
  ACTION_XA,            // :XA Crossbow
  ACTION_XB,            // :XB Crossbow
  ACTION_XC,            // :XC Crossbow
  ACTION_XD,            // :XD Crossbow
  ACTION_FA,            // :FA Flinged
  ACTION_FB,            // :FB Flinged
  ACTION_FC,            // :FC Flinged
  ACTION_FD,            // :FD Flinged
  ACTION_PA,            // :PA Parry
  ACTION_PB,            // :PB Parry
  ACTION_PC,            // :PC Parry
  ACTION_PD,            // :PD Parry
  ACTION_EA,            // :EA Evade
  ACTION_EB,            // :EB Evade
  ACTION_RA,            // :RA Roll
  ACTION_ZA,            // :ZA Zap Magic
  ACTION_ZB,            // :ZB Zap Magic
  ACTION_ZC,            // :ZC Zap Magic
  ACTION_ZD,            // :ZD Zap Magic
  ACTION_WA,            // :WA Sneak
  ACTION_WB,            // :WB Walk
  ACTION_WC,            // :WC Run
  ACTION_WD,            // :WD Push
  ACTION_JA,            // :JA Jump
  ACTION_JB,            // :JB Jump ( Falling ) ( Drop left )
  ACTION_JC,            // :JC Jump ( Falling ) ( Drop right )
  ACTION_HA,            // :HA Hit ( Taking damage )
  ACTION_HB,            // :HB Hit ( Taking damage )
  ACTION_HC,            // :HC Hit ( Taking damage )
  ACTION_HD,            // :HD Hit ( Taking damage )
  ACTION_KA,            // :KA Killed
  ACTION_KB,            // :KB Killed
  ACTION_KC,            // :KC Killed
  ACTION_KD,            // :KD Killed
  ACTION_MA,            // :MA Drop Item Left
  ACTION_MB,            // :MB Drop Item Right
  ACTION_MC,            // :MC Cheer
  ACTION_MD,            // :MD Show Off
  ACTION_ME,            // :ME Grab Item Left
  ACTION_MF,            // :MF Grab Item Right
  ACTION_MG,            // :MG Open Chest
  ACTION_MH,            // :MH Sit ( Not implemented )
  ACTION_MI,            // :MI Ride
  ACTION_MJ,            // :MJ Activated ( For items )
  ACTION_MK,            // :MK Snoozing
  ACTION_ML,            // :ML Unlock
  ACTION_MM,            // :MM Held Left
  ACTION_MN,            // :MN Held Right
  MAXACTION,            //                      // Number of action types

  ACTION_ST = ACTION_DA,

  ACTION_INVALID        = 0xffff                // Action not valid for this character
} ACTION;

extern char cActionName[MAXACTION][2];                  // Two letter name code

typedef enum mad_effects_bits_e
{
  MADFX_INVICTUS       = 1 <<  0,                    // I  Invincible
  MADFX_ACTLEFT        = 1 <<  1,                    // AL Activate left item
  MADFX_ACTRIGHT       = 1 <<  2,                    // AR Activate right item
  MADFX_GRABLEFT       = 1 <<  3,                    // GL GO Grab left/Grab only item
  MADFX_GRABRIGHT      = 1 <<  4,                    // GR Grab right item
  MADFX_DROPLEFT       = 1 <<  5,                    // DL DO Drop left/Drop only item
  MADFX_DROPRIGHT      = 1 <<  6,                    // DR Drop right item
  MADFX_STOP           = 1 <<  7,                    // S  Stop movement
  MADFX_FOOTFALL       = 1 <<  8,                    // F  Footfall sound
  MADFX_CHARLEFT       = 1 <<  9,                    // CL Grab left/Grab only character
  MADFX_CHARRIGHT      = 1 << 10,                    // CR Grab right character
  MADFX_POOF           = 1 << 11                     // P  Poof
} MADFX_BITS;

typedef enum lip_transition_e
{
  LIPT_DA = 0,                                  // For smooth transitions 'tween
  LIPT_WA,                                      //   walking rates
  LIPT_WB,                                      //
  LIPT_WC,                                      //
  LIPT_COUNT
} LIPT;

typedef enum prtpip_t
{
  PRTPIP_COIN_001 = 0,                  // Coins are the first particles loaded
  PRTPIP_COIN_005,                      //
  PRTPIP_COIN_025,                      //
  PRTPIP_COIN_100,                      //
  PRTPIP_WEATHER_1,                     // Weather particles
  PRTPIP_WEATHER_2,                     // Weather particle finish
  PRTPIP_SPLASH,                        // Water effects are next
  PRTPIP_RIPPLE,                        //
  PRTPIP_DEFEND,                        // Defend particle
  PRTPIP_PEROBJECT_COUNT                //
} PRTPIP;

//------------------------------------
//Model stuff
//------------------------------------

#define MAXFRAMESPERANIM 16

extern int             globalnumicon;                              // Number of icons
extern Uint16          madloadframe;                               // Where to load next

extern Uint16 skintoicon[MAXTEXTURE];                  // Skin to icon

typedef struct bbox_list_t
{
  int       count;
  AA_BBOX * list;
} BBOX_LIST;

BBOX_LIST * bbox_list_new(BBOX_LIST * lst);
BBOX_LIST * bbox_list_delete(BBOX_LIST * lst);
BBOX_LIST * bbox_list_renew(BBOX_LIST * lst);
BBOX_LIST * bbox_list_alloc(BBOX_LIST * lst, int count);
BBOX_LIST * bbox_list_realloc(BBOX_LIST * lst, int count);

typedef struct bbox_array_t
{
  int         count;
  BBOX_LIST * list;
} BBOX_ARY;

BBOX_ARY * bbox_ary_new(BBOX_ARY * ary);
BBOX_ARY * bbox_ary_delete(BBOX_ARY * ary);
BBOX_ARY * bbox_ary_renew(BBOX_ARY * ary);
BBOX_ARY * bbox_ary_alloc(BBOX_ARY * ary, int count);


typedef struct mad_t
{
  bool_t          used;                          // Model slot
  STRING          name;                          // Model name
  MD2_Model *     md2_ptr;                          // Md2 model pointer
  Uint16          skins;                         // Number of skins
  Uint16          skinstart;                     // Starting skin of model
  Uint16          msg_start;                      // The first message
  Uint16          vertices;                      // Number of vertices
  Uint16          transvertices;                 // Number to transform
  Uint8  *        framelip;                      // 0-15, How far into action is each frame
  Uint16 *        framefx;                       // Invincibility, Spawning
  Uint16          frameliptowalkframe[LIPT_COUNT][MAXFRAMESPERANIM];    // For walk animations
  Uint16          ai;                            // AI for each model
  bool_t          actionvalid[MAXACTION];        // bfalse if not valid
  Uint16          actionstart[MAXACTION];        // First frame of anim
  Uint16          actionend[MAXACTION];          // One past last frame
  Uint16          prtpip[PRTPIP_PEROBJECT_COUNT];    // Local particles

  int             bbox_frames;
  BBOX_ARY *      bbox_arrays;
} MAD;

extern MAD MadList[MAXMODEL];

#define VALID_MDL_RANGE(XX) ( ((XX)>=0) && ((XX)<MAXMODEL) )
#define VALID_MDL(XX)       ( VALID_MDL_RANGE(XX) )
#define VALIDATE_MDL(XX)    ( VALID_MDL(XX) ? (XX) : MAXMODEL )

ACTION action_number(char * szName);
Uint16 action_frame();
bool_t test_frame_name( char letter );
void   action_copy_correct( Uint16 object, ACTION actiona, ACTION actionb );
void   get_walk_frame( Uint16 object, LIPT lip_trans, ACTION action );
Uint16 get_framefx( char * szName );
void   make_framelip( Uint16 object, ACTION action );
void   get_actions( Uint16 object );
void make_mad_equally_lit( Uint16 model );

bool_t mad_generate_bbox_tree(int max_level, MAD * pmad);

MAD *  mad_new(MAD * pmad);
MAD *  mad_delete(MAD * pmad);
MAD *  mad_renew(MAD * pmad);
Uint16 load_one_mad( char* szLoadname, Uint16 modelindex );
void free_one_mad( Uint16 imdl );

bool_t mad_display_bbox_tree(int level, matrix_4x4 matrix, MAD * pmad, int frame1, int frame2);
