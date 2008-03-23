/* Egoboo - egoboo.h
 * Disgusting, hairy, way too monolithic header file for the whole darn
 * project.  In severe need of cleaning up.  Venture here with extreme
 * caution, and bring one of those canaries with you to make sure you
 * don't run out of oxygen.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _EGOBOO_H_
#define _EGOBOO_H_

/* Typedefs for various platforms */
#include "egobootypedef.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#include <SDL_opengl.h>

#include "proto.h"
#include "gltexture.h" /* OpenGL texture loader */
#include "mathstuff.h" /* vector and matrix math */
#include "configfile.h"
#include "Md2.h"

typedef struct glvertex_t
{
  GLVector pos;
  GLVector col;
  Uint32 color; // should replace r,g,b,a and be called by glColor4ubv
  GLfloat s, t; // u and v in D3D I guess
  vect3 nrm;
  vect3 up;
  vect3 rt;
} GLVertex;

// The following magic allows this include to work in multiple files
#ifdef DECLARE_GLOBALS
#define EXTERN
#define EQ(x) =x;
#else
#define EXTERN extern
#define EQ(x)
#endif

EXTERN const char VERSION[] EQ( "2.7.x" );   // Version of the game
EXTERN bool_t     game_single_frame EQ( bfalse );  //Is the game paused?
EXTERN bool_t     game_do_frame EQ( bfalse );  //Is the game paused?
EXTERN bool_t     gamepaused EQ( bfalse );  //Is the game paused?
EXTERN bool_t     startpause EQ( btrue );   //Pause button avalible?

#define NETREFRESH          1000                    // Every second
#define NONETWORK           numservice              //
#define MAXMODULE           100                     // Number of modules
#define TITLESIZE           128                     // Size of a title image
#define MAXSEQUENCE         256                     // Number of tracks in sequence
#define MAXIMPORT           (32*9)                  // Number of subdirs in IMPORT directory

#define NOSPARKLE           255

typedef enum respawn_mode_e
{
  RESPAWN_NONE = 0,
  RESPAWN_NORMAL,
  RESPAWN_ANYTIME
} RESPAWN_MODE;

#define DELAY_RESIZE            50                      // Time it takes to resize a character
#define WATCHMIN            .01                     //

#define PRTLEVELFIX         20                      // Fix for shooting over cliffs
#define DELAY_PACK           25                      // Time before inventory rotate again
#define DELAY_GRAB           25                      // Time before grab again
#define NOSKINOVERRIDE      -1                      // For import

#define PITDEPTH            -30                     // Depth to kill character
#define PITNOSOUND          -256                    // Stop sound at bottom of pits...

EXTERN Uint16  endtextindex[8];

#define EXPKEEP 0.85                                // Experience to keep when respawning

#define DISMOUNTZVEL        16
#define DISMOUNTZVELFLY     4

#define EDGE                128                     // Camera bounds
#define FARTRACK            1200                    // For outside modules...
#define EDGETRACK           800                     // Camtrack bounds

#define MAXNUMINPACK        6                       // Max number of items to carry in pack

#define NOHIDE              127                     // Don't hide

#define THROWFIX            30.0                    // To correct thrown velocities
#define MINTHROWVELOCITY    15.0                    //
#define MAXTHROWVELOCITY    45.0                    //


#define LOWSTAT             INT_TO_FP8(  1)      // Worst...
#define PERFECTSTAT         INT_TO_FP8( 75)      // Perfect...
#define HIGHSTAT            INT_TO_FP8( 99)      // Absolute MAX strength...
#define PERFECTBIG          INT_TO_FP8(127)      // Perfect life or mana...
#define MINDAMAGE           INT_TO_FP8(  1)      // Minimum damage for hurt animation
#define MSGDISTANCE         2000                 // Range for SendMessageNear

typedef enum damage_effects_bits_e
{
  DAMFX_NONE           = 0,                       // Damage effects
  DAMFX_ARMO           = 1 << 0,                  // Armor piercing
  DAMFX_BLOC           = 1 << 1,                  // Cannot be blocked by shield
  DAMFX_ARRO           = 1 << 2,                  // Only hurts the one it's attached to
  DAMFX_TURN           = 1 << 3,                  // Turn to attached direction
  DAMFX_TIME           = 1 << 4                   //
} DAMFX_BITS;

typedef enum blud_level_e
{
  BLUD_NONE = 0,
  BLUD_NORMAL,
  BLUD_ULTRA                         // This makes any damage draw blud
} BLUD_LEVEL;

#define SPELLBOOK           127                     // The spellbook model TODO: change this badly thing

typedef enum gender_e
{
  GEN_FEMALE = 0,
  GEN_MALE,
  GEN_OTHER,
  GEN_RANDOM,
} GENDER;

#define RETURNAND           63                      // Return mana every so often
#define MANARETURNSHIFT     4                       //
#define DAMAGE_HURT         INT_TO_FP8(1)           // 1 point of damage == hurt

#define MAXPASS             256                     // Maximum number of passages ( mul 32 )
#define MAXSTAT             16                      // Maximum status displays
#define MAXLOCO             3                       // Maximum number of local players

#define JOYBUTTON           8                       // Maximum number of joystick buttons
#define MOUSEBUTTON         4
#define MAXMESSAGE          6                       // Number of messages
#define MAXTOTALMESSAGE     1024                    //
#define MESSAGESIZE         80                      //
#define MESSAGEBUFFERSIZE   (MAXTOTALMESSAGE*40)
#define DELAY_MESSAGE         200                     // Time to keep the message alive
#define TABAND              31                      // Tab size

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

#define GRABSIZE            90.0                    // Grab tolerance

typedef enum lip_transition_e
{
  LIPT_DA = 0,                                  // For smooth transitions 'tween
  LIPT_WA,                                      //   walking rates
  LIPT_WB,                                      //
  LIPT_WC,                                      //
  LIPT_COUNT
} LIPT;

#define NULLICON            0                       // Empty hand image

#define MAXPRTPIP           1024                    // Particle templates

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

#define RIPPLEAND           15                      // How often ripples spawn
#define RIPPLETOLERANCE     60                      // For deep water
#define SPLASHTOLERANCE     10                      //
#define CLOSETOLERANCE      2                       // For closing doors

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

#define DELAY_DAMAGETILE      32                      // Invincibility time
#define DELAY_DAMAGE          16                      // Invincibility time
#define DELAY_DEFEND          16                      // Invincibility time
#define DROPXYVEL           8                       //
#define DROPZVEL            7                       //
#define JUMPATTACKVEL       -2                      //
#define WATERJUMP           12                      //

#define MAXSTOR             8                       // Storage data
#define STORAND             7                       //

#define MAXWAY              8                       // Waypoints
#define WAYTHRESH           128                     // Threshold for reaching waypoint
#define MAXLINESIZE         1024                    //
#define MAXAI               129                     //
#define MAXCODE             1024                    // Number of lines in AICODES.TXT
#define MAXCODENAMESIZE     64                      //
#define AISMAXCOMPILESIZE   (MAXAI*MAXCODE)         // For parsing AI scripts

#define MAXCAPNAMESIZE      32                      // Character class names
#define MAXLEVEL            6                       // Levels 0-5

typedef enum idsz_index_e
{
  IDSZ_PARENT = 0,                             // Parent index
  IDSZ_TYPE,                                   // Self index
  IDSZ_SKILL,                                  // Skill index
  IDSZ_SPECIAL,                                // Special index
  IDSZ_HATE,                                   // Hate index
  IDSZ_VULNERABILITY,                          // Vulnerability index
  IDSZ_COUNT                                   // ID strings per character
} IDSZ_INDEX;

#define IDSZ_NONE            MAKE_IDSZ("NONE")       // [NONE]

typedef enum experience_e
{
  XP_FINDSECRET = 0,                           // Finding a secret
  XP_WINQUEST,                                 // Beating a module or a subquest
  XP_USEDUNKOWN,                               // Used an unknown item
  XP_KILLENEMY,                                // Killed an enemy
  XP_KILLSLEEPY,                               // Killed a sleeping enemy
  XP_KILLHATED,                                // Killed a hated enemy
  XP_TEAMKILL,                                 // Team has killed an enemy
  XP_TALKGOOD,                                 // Talk good, er...  I mean well
  XP_COUNT,                                    // Number of ways to get experience
  XP_DIRECT     = 255                          // No modification
} EXPERIENCE;


// This stuff is for actions
typedef enum action_e
{
  ACTION_ST = 0,
  ACTION_DA = ACTION_ST,// :DA Dance ( Standing still )
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
  ACTION_INVALID        = 0xffff                // Action not valid for this character
} ACTION;

EXTERN char    cActionName[MAXACTION][2];                  // Two letter name code

typedef enum turnmode_e
{
  TURNMODE_NONE     = 0,
  TURNMODE_VELOCITY,                      // Character gets rotation from velocity
  TURNMODE_WATCH,                             // For watch towers
  TURNMODE_SPIN,                              // For spinning objects
  TURNMODE_WATCHTARGET                        // For combat intensive AI
} TURNMODE;

#define SPINRATE            200                     // How fast spinners spin
#define FLYDAMPEN           .001                    // Levelling rate for flyers

typedef enum latch_button_e
{
  LATCHBUTTON_NONE      =      0,
  LATCHBUTTON_LEFT      = 1 << 0,                    // Character button presses
  LATCHBUTTON_RIGHT     = 1 << 1,                    //
  LATCHBUTTON_JUMP      = 1 << 2,                    //
  LATCHBUTTON_ALTLEFT   = 1 << 3,                    // ( Alts are for grab/drop )
  LATCHBUTTON_ALTRIGHT  = 1 << 4,                    //
  LATCHBUTTON_PACKLEFT  = 1 << 5,                    // ( Packs are for inventory cycle )
  LATCHBUTTON_PACKRIGHT = 1 << 6,                    //
  LATCHBUTTON_RESPAWN   = 1 << 7                     //
} LATCHBUTTON_BITS;

#define JUMPINFINITE        255                     // Flying character
#define DELAY_JUMP           20                      // Time between jumps
#define JUMPTOLERANCE       20                      // Distance above ground to be jumping
#define SLIDETOLERANCE      10                      // Stick to ground better
#define PLATTOLERANCE       50                      // Platform tolerance...
#define PLATADD             -10                     // Height add...
#define PLATASCEND          .10                     // Ascension rate
#define PLATKEEP            (1.0f-PLATASCEND)       // Retention rate

#define MAPID 0x4470614d                      // The string... MapD

#define RAISE 5 //25                               // Helps correct z level
#define SHADOWRAISE 5                               //
#define DAMAGERAISE 25                              //

#define MAXWATERLAYER 2                             // Maximum water layers
#define MAXWATERFRAME 512                           // Maximum number of wave frames
#define WATERFRAMEAND (MAXWATERFRAME-1)             //
#define WATERPOINTS 4                               // Points in a water fan
#define WATERMODE 4                                 // Ummm...  For making it work, yeah...

#define DONTFLASH 255                               //

#define FOV                             40          // Field of view

#define NUMFONTX                        16          // Number of fonts in the bitmap
#define NUMFONTY                        6           //
#define NUMFONT                         (NUMFONTX*NUMFONTY)
#define FONTADD                         4           // Gap between letters
#define NUMBAR                          6           // Number of status bars
#define TABX                            32          // Size of little name tag on the bar
#define BARX                            112         // Size of bar
#define BARY                            16          //
#define NUMTICK                         10          // Number of ticks per row
#define TICKX                           8           // X size of each tick
#define MAXTICK                         (NUMTICK*5) // Max number of ticks to draw

#define TURNSPD                         .01         // Cutoff for turning or same direction
#define CAMKEYTURN                      5           // Keyboard camera rotation
#define CAMJOYTURN                      5           // Joystick camera rotation


// Multi cam
#define MINZOOM                         100         // Camera distance
#define MAXZOOM                         600         //
#define MINZADD                         100         // Camera height
#define MAXZADD                         3000
//#define MINUPDOWN                       (.18*PI)//(.24*PI)    // Camera updown angle
//#define MAXUPDOWN                       (.18*PI)


#define MD2START                        0x32504449  // MD2 files start with these four bytes
#define MD2MAXLOADSIZE                  (512*1024)  // Don't load any models bigger than 512k
#define EQUALLIGHTINDEX                 162         // I added an extra index to do the spikey mace...
#define MD2LIGHTINDICES                 163         // MD2's store vertices as x,y,z,normal

#define MAXTEXTURE                      512         // Max number of textures
#define MAXVERTICES                    1024         // Max number of points in a model
#define MAXCOMMAND                      256         // Max number of commands
#define MAXCOMMANDSIZE                  64          // Max number of points in a command
#define MAXCOMMANDENTRIES               512         // Max entries in a command list ( trigs )
#define MAXMODEL                        256         // Max number of models
#define MAXEVE                          MAXMODEL    // One enchant type per model
#define MAXENCHANT                      128         // Number of enchantments
#define MAXFRAME                        (128*32)    // Max number of frames in all models
#define MAXCHR                          350         // Max number of characters
#define MAXLIGHTLEVEL                   16          // Number of premade light intensities
#define MAXSPEKLEVEL                    16          // Number of premade specularities
#define MAXLIGHTROTATION                256         // Number of premade light maps
#define MAXPRT                          512         // Max number of particles
#define MAXDYNA                         8           // Number of dynamic lights
#define MAXDYNADIST                     2700        // Leeway for offscreen lights

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

EXTERN SLOT _slot;

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

SLOT grip_to_slot( GRIP g );
GRIP slot_to_grip( SLOT s );
Uint16 slot_to_latch( Uint16 object, SLOT s );
Uint16 slot_to_offset( SLOT s );

#define CHOPPERMODEL                    32          //
#define MAXSECTION                      4           // T-wi-n-k...  Most of 4 sections
#define MAXCHOP                         (MAXMODEL*CHOPPERMODEL)
#define CHOPSIZE                        8
#define CHOPDATACHUNK                   (MAXCHOP*CHOPSIZE)



typedef enum particle_type
{
  PRTTYPE_LIGHT = 0,                         // Magic effect particle
  PRTTYPE_SOLID,                             // Sprite particle
  PRTTYPE_ALPHA,                             // Smoke particle
} PRTTYPE;

/* SDL_GetTicks() always returns milli seconds */
#define TICKS_PER_SEC                   1000.0f


#define TARGETFPS                       30.0f
#define FRAMESKIP                       (TICKS_PER_SEC/TARGETFPS)    // 1000 tics per sec / 50 fps = 20 ticks per frame

#define EMULATEUPS                      50.0f
#define TARGETUPS                       30.0f
#define UPDATESCALE                     (EMULATEUPS/(stabilized_ups_sum/stabilized_ups_weight))
#define UPDATESKIP                      (TICKS_PER_SEC/TARGETUPS)    // 1000 tics per sec / 50 fps = 20 ticks per frame
#define ONESECOND                       (TICKS_PER_SEC/UPDATESKIP)    // 1000 tics per sec / 20 ticks per frame = 50 fps

#define STOPBOUNCING                    0.5f         // To make objects stop bouncing
#define STOPBOUNCINGPART                1.0f         // To make particles stop bouncing

#define TRANSCOLOR                      0           // Transparent color
#define DELAY_BORE                        (rand()&255)+120
#define DELAY_CAREFUL                     50
#define REEL                            7600.0      // Dampen for melee knock back
#define REELBASE                        .35         //


EXTERN int    animtileupdateand  EQ( 7 );                        // New tile every 7 frames
EXTERN Uint16 animtileframeand  EQ( 3 );              // Only 4 frames
EXTERN Uint16 animtilebaseand  EQ( 0xfffc );          //
EXTERN Uint16 animtilebigframeand  EQ( 7 );           // For big tiles
EXTERN Uint16 animtilebigbaseand  EQ( 0xfff8 );       //
EXTERN float  animtileframefloat  EQ( 0 );              // Current frame
EXTERN Uint16 animtileframeadd  EQ( 0 );              // Current frame

EXTERN Uint16 bookicon  EQ( 0 );                      // The first book icon

#define NORTH 16384                                 // Character facings
#define SOUTH 49152                                 //
#define EAST 32768                                  //
#define WEST 0                                      //

#define FRONT 0                                     // Attack directions
#define BEHIND 32768                                //
#define LEFT 49152                                  //
#define RIGHT 16384                                 //


#define MAXXP    ((1<<30)-1)                               // Maximum experience (Sint32 32)
#define MAXMONEY 9999                                      // Maximum money


typedef enum team_e
{
  TEAM_EVIL            = 'E' -'A',                      // E
  TEAM_GOOD            = 'G' -'A',                      // G
  TEAM_NULL            = 'N' -'A',                      // N
  TEAM_ZIPPY           = 'Z' -'A',
  TEAM_DAMAGE,                                          // For damage tiles
  TEAM_COUNT                                              // Teams A-Z, +1 more for damage tiles
} TEAM;

EXTERN bool_t  teamhatesteam[TEAM_COUNT][TEAM_COUNT];     // Don't damage allies...
EXTERN Uint16  teammorale[TEAM_COUNT];                 // Number of characters on team
EXTERN CHR_REF teamleader[TEAM_COUNT];                 // The leader of the team
EXTERN CHR_REF teamsissy[TEAM_COUNT];                  // Whoever called for help last

#define VALID_TEAM(XX) ( ((XX)>=0) && ((XX)<TEAM_COUNT) )

CHR_REF team_get_sissy( TEAM_REF iteam );
CHR_REF team_get_leader( TEAM_REF iteam );

EXTERN short  damagetileparttype;
EXTERN short  damagetilepartand;
EXTERN Sint8  damagetilesound;
EXTERN int    damagetileamount  EQ( 256 );                           // Amount of damage
EXTERN DAMAGE damagetiletype  EQ( DAMAGE_FIRE );                      // Type of damage

#define DELAY_TILESOUND 16
#define TILEREAFFIRMAND  3


//Minimap stuff
#define MAXBLIP     32     //Max number of blips displayed on the map
EXTERN Uint16          numblip  EQ( 0 );
EXTERN Uint16          blipx[MAXBLIP];
EXTERN Uint16          blipy[MAXBLIP];
EXTERN Uint8           blipc[MAXBLIP];
EXTERN Uint8           mapon  EQ( bfalse );
EXTERN Uint8           youarehereon  EQ( bfalse );


EXTERN Uint8           timeron     EQ( bfalse );      // Game timer displayed?
EXTERN Uint32          timervalue  EQ( 0 );           // Timer time ( 50ths of a second )


EXTERN Sint32          ups_clock             EQ( 0 );             // The number of ticks this second
EXTERN Uint32          ups_loops             EQ( 0 );             // The number of frames drawn this second
EXTERN float           stabilized_ups        EQ( TARGETUPS );
EXTERN float           stabilized_ups_sum    EQ( TARGETUPS );
EXTERN float           stabilized_ups_weight EQ( 1 );

EXTERN Sint32          fps_clock             EQ( 0 );             // The number of ticks this second
EXTERN Uint32          fps_loops             EQ( 0 );             // The number of frames drawn this second
EXTERN float           stabilized_fps        EQ( TARGETFPS );
EXTERN float           stabilized_fps_sum    EQ( TARGETFPS );
EXTERN float           stabilized_fps_weight EQ( 1 );

EXTERN Sint32          sttclock;                       // GetTickCount at start
EXTERN Sint32          allclock   EQ( 0 );             // The total number of ticks so far
EXTERN Sint32          lstclock   EQ( 0 );             // The last total of ticks so far
EXTERN Sint32          wldclock   EQ( 0 );             // The sync clock
EXTERN Uint32          wldframe   EQ( 0 );             // The number of frames that should have been drawn
EXTERN Uint32          allframe   EQ( 0 );             // The total number of frames drawn so far
EXTERN Uint8           statclock  EQ( 0 );            // For stat regeneration
EXTERN float           pitclock   EQ( 0 );             // For pit kills
EXTERN Uint8           pitskill   EQ( bfalse );          // Do they kill?

EXTERN Uint8           outofsync  EQ( 0 );    //Is this only for RTS? Can it be removed then?
EXTERN Uint8           parseerror EQ( 0 );    //Do we have an script error?



EXTERN bool_t                    gameActive  EQ( bfalse );       // Stay in game or quit to windows?
EXTERN bool_t                    ingameMenuActive EQ( bfalse );  // Is the in-game menu active?
EXTERN bool_t                    moduleActive  EQ( bfalse );     // Is the control loop still going?

EXTERN bool_t                    mouseon  EQ( btrue );              // Is the mouse alive?
EXTERN bool_t                    keyon  EQ( btrue );                // Is the keyboard alive?
EXTERN bool_t                    joyaon  EQ( bfalse );               // Is the holy joystick alive?
EXTERN bool_t                    joybon  EQ( bfalse );               // Is the other joystick alive?
EXTERN bool_t                    serviceon  EQ( bfalse );        // Do I need to free the interface?
//EXTERN bool_t                    menuaneeded  EQ(bfalse);         // Give them MENUA?
//EXTERN bool_t                    menuactive  EQ(bfalse);         // Menu running?
EXTERN bool_t                    hostactive  EQ( bfalse );       // Hosting?
EXTERN bool_t                    readytostart;               // Ready to hit the Start Game button?
EXTERN bool_t                    waitingforplayers;          // Has everyone talked to the host?
EXTERN bool_t                    somelocalpladead;            // Has someone died?
EXTERN bool_t                    alllocalpladead;            // Has everyone died?
EXTERN bool_t                    respawnvalid;               // Can players respawn with Spacebar?
EXTERN bool_t                    respawnanytime;             // True if it's a small level...
EXTERN bool_t                    importvalid;                // Can it import?
EXTERN bool_t                    exportvalid;                // Can it export?
EXTERN bool_t                    netmessagemode;             // Input text from keyboard?
EXTERN bool_t                    nolocalplayers;             // Are there any local players?
EXTERN bool_t                    beatmodule;                 // Show Module Ended text?

#define DELAY_TURN 16
EXTERN Uint8                   netmessagedelay;            // For slowing down input
EXTERN int                     netmessagewrite;            // The cursor position
EXTERN int                     netmessagewritemin;         // The starting cursor position
EXTERN char                    netmessage[MESSAGESIZE];    // The input message
EXTERN int                     importamount;               // Number of imports for this module
EXTERN int                     playeramount;               //
EXTERN Uint32                  seed  EQ( 0 );              // The module seed
EXTERN char                    pickedmodule[64];           // The module load name
EXTERN int                     pickedindex;                // The module index number
EXTERN int                     playersready;               // Number of players ready to start
EXTERN int                     playersloaded;              //

// JF - Added so that the video mode might be determined outside of the graphics code
extern SDL_Surface *displaySurface;

//Networking
EXTERN int                     localmachine  EQ( 0 );         // 0 is host, 1 is 1st remote, 2 is 2nd...
EXTERN int                     numimport;                  // Number of imports from this machine
EXTERN Uint8                   localcontrol[16];           // For local imports
EXTERN short                   localslot[16];              // For local imports

//Setup values
EXTERN float                   mousesense  EQ( 2 );           // Sensitivity threshold
EXTERN float                   mousesustain EQ( .9 );         // Falloff rate for old movement
EXTERN float                   mousecover EQ( .1 );           // For falloff
EXTERN int                     mousedx  EQ( 0 );               // Mouse X movement counter
EXTERN int                     mousedy  EQ( 0 );               // Mouse Y movement counter
EXTERN int                     mousex  EQ( 0 );               // Mouse X movement counter
EXTERN int                     mousey  EQ( 0 );               // Mouse Y movement counter
EXTERN Sint32                  mousez  EQ( 0 );               // Mouse wheel movement counter
EXTERN int                     cursorx  EQ( 0 );              // Cursor position
EXTERN int                     cursory  EQ( 0 );              //
EXTERN float                   mouselatcholdx  EQ( 0 );       // For sustain
EXTERN float                   mouselatcholdy  EQ( 0 );       //
EXTERN Uint8                   mousebutton[MOUSEBUTTON];             // Mouse button states
EXTERN bool_t                    pressed EQ( 0 );                //
EXTERN bool_t                    clicked EQ( 0 );                //
EXTERN bool_t         pending_click EQ( 0 );
// EWWWW. GLOBALS ARE EVIL.

//Input Control
//PORT: Use sdlkeybuffer instead.
//EXTERN char                    keybuffer[256];             // Keyboard key states
//EXTERN char                    keypress[256];              // Keyboard new hits
EXTERN float                   joyax  EQ( 0 );                // Joystick A
EXTERN float                   joyay  EQ( 0 );                //
EXTERN Uint8           joyabutton[JOYBUTTON];      //
EXTERN float                   joybx  EQ( 0 );                // Joystick B
EXTERN float                   joyby  EQ( 0 );                //
EXTERN Uint8           joybbutton[JOYBUTTON];      //
EXTERN Uint8           msb, jab, jbb;              // Button masks


//Weather and water gfx
EXTERN Uint8     watershift  EQ( 3 );
EXTERN bool_t    weatheroverwater EQ( bfalse );       // Only spawn over water?
EXTERN int       weathertimereset EQ( 10 );          // Rate at which weather particles spawn
EXTERN float     weathertime EQ( 0 );                // 0 is no weather
EXTERN int       weatherplayer;
EXTERN int       numwaterlayer EQ( 0 );              // Number of layers
EXTERN float     watersurfacelevel EQ( 0 );          // Surface level for water striders
EXTERN float     waterdouselevel EQ( 0 );            // Surface level for torches
EXTERN bool_t    waterlight EQ( 0 );                 // Is it light ( default is alpha )
EXTERN Uint8     waterspekstart EQ( 128 );           // Specular begins at which light value
EXTERN Uint8     waterspeklevel_fp8 EQ( 128 );           // General specular amount (0-255)
EXTERN bool_t    wateriswater  EQ( btrue );          // Is it water?  ( Or lava... )
EXTERN Uint8     waterlightlevel_fp8[MAXWATERLAYER]; // General light amount (0-63)
EXTERN Uint8     waterlightadd_fp8[MAXWATERLAYER];   // Ambient light amount (0-63)
EXTERN float     waterlayerz[MAXWATERLAYER];     // Base height of water
EXTERN Uint8     waterlayeralpha_fp8[MAXWATERLAYER]; // Transparency
EXTERN float     waterlayeramp[MAXWATERLAYER];   // Amplitude of waves
EXTERN float     waterlayeru[MAXWATERLAYER];     // Coordinates of texture
EXTERN float     waterlayerv[MAXWATERLAYER];     //
EXTERN float     waterlayeruadd[MAXWATERLAYER];  // Texture movement
EXTERN float     waterlayervadd[MAXWATERLAYER];  //
EXTERN float     waterlayerzadd[MAXWATERLAYER][MAXWATERFRAME][WATERMODE][WATERPOINTS];
EXTERN Uint8     waterlayercolor[MAXWATERLAYER][MAXWATERFRAME][WATERMODE][WATERPOINTS];
EXTERN Uint16    waterlayerframe[MAXWATERLAYER]; // Frame
EXTERN Uint16    waterlayerframeadd[MAXWATERLAYER];      // Speed
EXTERN float     waterlayerdistx[MAXWATERLAYER];         // For distant backgrounds
EXTERN float     waterlayerdisty[MAXWATERLAYER];         //
EXTERN Uint32    waterspek[256];             // Specular highlights
EXTERN float     foregroundrepeat  EQ( 1 );     //
EXTERN float     backgroundrepeat  EQ( 1 );     //

// Global lighting stuff
EXTERN float                   lightspek EQ( 0 );
extern vect3                   lightspekdir;
extern vect3                   lightspekcol;
EXTERN float                   lightambi EQ( 0 );
extern vect3                   lightambicol;


//Fog stuff
EXTERN bool_t          fogon  EQ( bfalse );            // Do ground fog?
EXTERN float                   fogbottom  EQ( 0.0 );          //
EXTERN float                   fogtop  EQ( 100 );             //
EXTERN float                   fogdistance  EQ( 100 );        //
EXTERN Uint8           fogred  EQ( 255 );             //  Fog collour
EXTERN Uint8           foggrn  EQ( 255 );             //
EXTERN Uint8           fogblu  EQ( 255 );             //
EXTERN bool_t          fogaffectswater;


//Camera control stuff

EXTERN int                     camswing  EQ( 0 );             // Camera swingin'
EXTERN int                     camswingrate  EQ( 0 );         //
EXTERN float                   camswingamp  EQ( 0 );          //
extern vect3                   campos;                        // Camera position
EXTERN float                   camzoom  EQ( 500 );           // Distance from the trackee
EXTERN vect3                   camtrackvel;                  // Change in trackee position
EXTERN vect3                   camcenterpos;                 // Move character to side before tracking
EXTERN vect3                   camtrackpos;                  // Trackee position
EXTERN float                   camtracklevel;              //
EXTERN float                   camzadd  EQ( 800 );            // Camera height above terrain
EXTERN float                   camzaddgoto  EQ( 800 );        // Desired z position
EXTERN float                   camzgoto  EQ( 800 );           //
EXTERN float                   camturn_lr  EQ( 8192 );    // Camera rotations
EXTERN float                   camturn_lr_one  EQ( -1.0 / 8.0 );
EXTERN float                   camturnadd  EQ( 0 );           // Turning rate
EXTERN float                   camsustain  EQ( .80 );         // Turning rate falloff
EXTERN float                   camroll  EQ( 0 );              //

EXTERN float                   cornerx[4];                 // Render area corners
EXTERN float                   cornery[4];                 //
EXTERN int                     cornerlistlowtohighy[4];    // Ordered list
EXTERN int                     cornerlowx;                 // Render area extremes
EXTERN int                     cornerhighx;                //
EXTERN int                     cornerlowy;                 //
EXTERN int                     cornerhighy;                //

/*OpenGL Textures*/
EXTERN  GLTexture       TxTexture[MAXTEXTURE];        /* All textures */
EXTERN  GLTexture       TxIcon[MAXTEXTURE+1];       /* icons */
EXTERN  GLTexture       TxTitleImage[MAXMODULE];      /* title images */
EXTERN  GLTexture       TxTrim;
EXTERN  GLTexture       TxTrimX;                                        /* trim */
EXTERN  GLTexture       TxTrimY;                                        /* trim */
EXTERN  GLTexture       TxFont;                                         /* font */
EXTERN  GLTexture       TxBars;                                         /* status bars */
EXTERN  GLTexture       TxBlip;                                         /* you are here texture */
EXTERN  GLTexture       TxMap;

//Particle Texture Types
typedef enum part_type
{
  PART_NORMAL,
  PART_SMOOTH,
  PART_FAST
} PART_TYPE;

//Texture filtering
typedef enum tx_filters_e
{
  TX_UNFILTERED,
  TX_LINEAR,
  TX_MIPMAP,
  TX_BILINEAR,
  TX_TRILINEAR_1,
  TX_TRILINEAR_2,
  TX_ANISOTROPIC
} TX_FILTERS;

//Anisotropic filtering - yay! :P
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
EXTERN float maxAnisotropy;          // Max anisotropic filterings (Between 1.00 and 16.00)
EXTERN int   userAnisotropy;           // Requested anisotropic level
EXTERN int   log2Anisotropy;                  // Max levels of anisotropy

EXTERN bool_t      video_mode_chaged EQ( bfalse );
EXTERN SDL_Rect ** video_mode_list EQ( NULL );

EXTERN GLMatrix mView;           // View Matrix
EXTERN GLMatrix mProjection;     // Projection Matrix
EXTERN GLMatrix mProjectionBig;  // Larger projection matrix for frustum culling


//Input player control
EXTERN int                     numloadplayer  EQ( 0 );
#define MAXLOADPLAYER     100
EXTERN char                    loadplayername[MAXLOADPLAYER][MAXCAPNAMESIZE];
EXTERN char                    loadplayerdir[MAXLOADPLAYER][16];
EXTERN int                     nullicon  EQ( 0 );
EXTERN int                     keybicon  EQ( 0 );
EXTERN int                     mousicon  EQ( 0 );
EXTERN int                     joyaicon  EQ( 0 );
EXTERN int                     joybicon  EQ( 0 );
EXTERN int                     keybplayer  EQ( 0 );
EXTERN int                     mousplayer  EQ( 0 );
EXTERN int                     joyaplayer  EQ( 0 );
EXTERN int                     joybplayer  EQ( 0 );

//Interface stuff
#define TRIMX 640
#define TRIMY 480

EXTERN IRect                    iconrect;                   // The 32x32 icon rectangle
EXTERN IRect                    trimrect;                   // The menu trim rectangle

EXTERN int                       fontoffset;                 // Line up fonts from top of screen
EXTERN SDL_Rect              fontrect[NUMFONT];          // The font rectangles
EXTERN Uint8                fontxspacing[NUMFONT];      // The spacing stuff
EXTERN Uint8                fontyspacing;               //

EXTERN IRect                    tabrect[NUMBAR];            // The tab rectangles

EXTERN IRect                    barrect[NUMBAR];            // The bar rectangles

EXTERN IRect                    bliprect[NUMBAR];           // The blip rectangles
EXTERN Uint16                    blipwidth;
EXTERN Uint16                    blipheight;

EXTERN float                     mapscale EQ( 1.0 );
EXTERN IRect                    maprect;                    // The map rectangle

#define SPARKLESIZE 28
#define SPARKLEADD 2
#define MAPSIZE 96

//Lightning effects
EXTERN int                     numdynalight;               // Number of dynamic lights
EXTERN int                     dynadistancetobeat;         // The number to beat
EXTERN int                     dynadistance[MAXDYNA];      // The distances
EXTERN vect3                   dynalightlist[MAXDYNA];    // Light position
EXTERN float                   dynalightlevel[MAXDYNA];    // Light level
EXTERN float                   dynalightfalloff[MAXDYNA];  // Light falloff
EXTERN Uint8               lightdirectionlookup[UINT16_SIZE];// For lighting characters
EXTERN float                   spek_global[MAXLIGHTROTATION][MD2LIGHTINDICES];
EXTERN float                   spek_local[MAXLIGHTROTATION][MD2LIGHTINDICES];
EXTERN Uint8               cLoadBuffer[MD2MAXLOADSIZE];// Where to put an MD2

EXTERN Uint32       maptwist_lr[256];            // For surface normal of mesh
EXTERN Uint32        maptwist_ud[256];            //
EXTERN vect3           mapnrm[256];                // For sliding down steep hills
EXTERN bool_t          maptwistflat[256];             //

typedef enum order_t
{
  MESSAGE_MOVE    = 0,
  MESSAGE_ATTACK,
  MESSAGE_ASSIST,
  MESSAGE_STAND,
  MESSAGE_TERRAIN,
  MESSAGE_ENTERPASSAGE
} ORDER;

typedef enum color_e
{
  COLOR_WHITE = 0,
  COLOR_RED,
  COLOR_YELLOW,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_PURPLE
} COLOR;

typedef enum move_t
{
  MOVE_MELEE = 300,
  MOVE_RANGED = -600,
  MOVE_DISTANCE = -175,
  MOVE_RETREAT = 900,
  MOVE_CHARGE = 1500,
  MOVE_FOLLOW = 0
} MOVE;

typedef enum search_bits_e
{
  SEARCH_DEAD      = 1 << 0,   
  SEARCH_ENEMIES   = 1 << 1,
  SEARCH_FRIENDS   = 1 << 2,
  SEARCH_ITEMS     = 1 << 3,  
  SEARCH_INVERT    = 1 << 4
} SEARCH_BITS;


typedef enum missle_handling_e
{
  MIS_NORMAL  = 0,                  //Treat missiles normally
  MIS_DEFLECT,                      //Deflect incoming missiles
  MIS_REFLECT                       //Reflect them back!
} MISSLE_TYPE;

//Character stuff
#define VALID_MDL(XX) ( ((XX)>=0) && ((XX)<MAXMODEL) )
#define VALIDATE_MDL(XX) ( VALID_MDL(XX) ? (XX) : MAXMODEL )

#define VALID_CHR(XX)   ( ((XX)>=0) && ((XX)<MAXCHR) && chron[XX] )
#define VALIDATE_CHR(XX) ( VALID_CHR(XX) ? (XX) : MAXCHR )

bool_t chr_in_pack( CHR_REF character );
bool_t chr_attached( CHR_REF character );
bool_t chr_has_inventory( CHR_REF character );
bool_t chr_is_invisible( CHR_REF character );
bool_t chr_using_slot( CHR_REF character, SLOT slot );

CHR_REF chr_get_nextinpack( CHR_REF ichr );
CHR_REF chr_get_onwhichplatform( CHR_REF ichr );
CHR_REF chr_get_inwhichpack( CHR_REF ichr );
CHR_REF chr_get_attachedto( CHR_REF ichr );
CHR_REF chr_get_bumpnext( CHR_REF ichr );
CHR_REF chr_get_holdingwhich( CHR_REF ichr, SLOT slot );

CHR_REF chr_get_aitarget( CHR_REF ichr );
CHR_REF chr_get_aiowner( CHR_REF ichr );
CHR_REF chr_get_aichild( CHR_REF ichr );
CHR_REF chr_get_aiattacklast( CHR_REF ichr );
CHR_REF chr_get_aibumplast( CHR_REF ichr );
CHR_REF chr_get_aihitlast( CHR_REF ichr );

CHR_REF prt_get_owner( PRT_REF iprt );
CHR_REF prt_get_target( PRT_REF iprt );
CHR_REF prt_get_attachedtochr( PRT_REF iprt );

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

typedef struct lighting_data_t
{
  vect4   emission, diffuse, specular;
  GLfloat shininess[1];
} LData;

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

EXTERN int             numfreechr EQ( -1 );         // For allocation
EXTERN Uint16          freechrlist[MAXCHR];        //
EXTERN Uint16          chrcollisionlevel EQ(2);
EXTERN bool_t          chron[MAXCHR];              // Does it exist?
EXTERN char            chrname[MAXCHR][MAXCAPNAMESIZE];  // Character name
EXTERN bool_t          chrgopoof[MAXCHR];          // is poof requested?
EXTERN bool_t          chrfreeme[MAXCHR];          // is free_one_character() requested?
EXTERN GLMatrix        chrmatrix[MAXCHR];   // Character's matrix
EXTERN bool_t          chrmatrixvalid[MAXCHR];     // Did we make one yet?
EXTERN Uint16          chrmodel[MAXCHR];           // Character's model
EXTERN Uint16          chrbasemodel[MAXCHR];       // The true form
EXTERN VData_Blended   chrvdata[MAXCHR];           // pre-processed per-vertex lighting data
EXTERN LData           chrldata[MAXCHR];           // pre-processed matrial parameters
EXTERN bool_t          chralive[MAXCHR];           // Is it alive?
EXTERN bool_t          chrinwhichpack[MAXCHR];     // Is it in whose inventory?
EXTERN Uint16          chrnextinpack[MAXCHR];      // Link to the next item
EXTERN Uint8           chrnuminpack[MAXCHR];       // How many
EXTERN bool_t          chropenstuff[MAXCHR];       // Can it open chests/doors?
EXTERN Uint8           chrlifecolor[MAXCHR];       // Bar color
EXTERN Uint8           chrsparkle[MAXCHR];         // Sparkle color or 0 for off
EXTERN Sint16          chrlife_fp8[MAXCHR];            // Basic character stats
EXTERN Sint16          chrlifemax_fp8[MAXCHR];         //   All 8.8 fixed point
EXTERN Uint16          chrlifeheal[MAXCHR];        //
EXTERN Uint8           chrmanacolor[MAXCHR];       // Bar color
EXTERN Uint8           chrammomax[MAXCHR];         // Ammo stuff
EXTERN Uint8           chrammo[MAXCHR];            //
EXTERN GENDER          chrgender[MAXCHR];          // Gender
EXTERN Sint16          chrmana_fp8[MAXCHR];            // Mana stuff
EXTERN Sint16          chrmanamax_fp8[MAXCHR];         //
EXTERN Sint16          chrmanaflow_fp8[MAXCHR];        //
EXTERN Sint16          chrmanareturn_fp8[MAXCHR];      //
EXTERN Sint16          chrstrength_fp8[MAXCHR];        // Strength
EXTERN Sint16          chrwisdom_fp8[MAXCHR];          // Wisdom
EXTERN Sint16          chrintelligence_fp8[MAXCHR];    // Intelligence
EXTERN Sint16          chrdexterity_fp8[MAXCHR];       // Dexterity
EXTERN bool_t          chrstickybutt[MAXCHR];      // Rests on floor
EXTERN bool_t          chrenviro[MAXCHR];          // Environment map?
EXTERN bool_t          chrinwater[MAXCHR];         //
EXTERN Uint32          chralert[MAXCHR];           // Alerts for AI script
EXTERN float           chrflyheight[MAXCHR];       // Height to stabilize at
EXTERN TEAM            chrteam[MAXCHR];            // Character's team
EXTERN TEAM            chrbaseteam[MAXCHR];        // Character's starting team
EXTERN bool_t          chrstaton[MAXCHR];          // Display stats?
EXTERN vect3           chrstt[MAXCHR];             // Starting position
EXTERN vect3           chrpos[MAXCHR];             // Character's position
EXTERN vect3           chrtrgvel[MAXCHR];          // Character's target velocity
EXTERN vect3           chrvel[MAXCHR];             // Character's velocity
EXTERN vect3           chrpos_old[MAXCHR];          // Character's last position
EXTERN Uint16          chrturn_lr_old[MAXCHR];         //
EXTERN vect3           chraccum_acc[MAXCHR];         //
EXTERN vect3           chraccum_vel[MAXCHR];         //
EXTERN vect3           chraccum_pos[MAXCHR];         //
EXTERN float           chrlatchx[MAXCHR];          // Character latches
EXTERN float           chrlatchy[MAXCHR];          //
EXTERN Uint8           chrlatchbutton[MAXCHR];     // Button latches
EXTERN float           chrreloadtime[MAXCHR];      // Time before another shot
EXTERN float           chrmaxaccel[MAXCHR];        // Maximum acceleration
EXTERN float           chrscale[MAXCHR];           // Character's size (useful)
EXTERN float           chrfat[MAXCHR];             // Character's size (legible)
EXTERN float           chrsizegoto[MAXCHR];        // Character's size goto ( legible )
EXTERN float           chrsizegototime[MAXCHR];    // Time left in siez change
EXTERN float           chrdampen[MAXCHR];          // Bounciness
EXTERN float           chrbumpstrength[MAXMODEL];  // ghost-like interaction with objects?
EXTERN float           chrlevel[MAXCHR];           // Height under character
EXTERN bool_t          chrlevelvalid[MAXCHR];      // Has height been stored?
EXTERN float           chrjump[MAXCHR];            // Jump power
EXTERN float           chrjumptime[MAXCHR];        // Delay until next jump
EXTERN float           chrjumpnumber[MAXCHR];      // Number of jumps remaining
EXTERN Uint8           chrjumpnumberreset[MAXCHR]; // Number of jumps total, 255=Flying
EXTERN bool_t          chrjumpready[MAXCHR];       // For standing on a platform character
EXTERN Uint32          chronwhichfan[MAXCHR];      // Where the char is
EXTERN bool_t          chrindolist[MAXCHR];        // Has it been added yet?
EXTERN Uint16          chruoffset_fp8[MAXCHR];         // For moving textures
EXTERN Uint16          chrvoffset_fp8[MAXCHR];         //
EXTERN Uint16          chruoffvel[MAXCHR];         // Moving texture speed
EXTERN Uint16          chrvoffvel[MAXCHR];         //
EXTERN Uint16          chrturn_lr[MAXCHR];   // Character's rotation 0 to 65535
EXTERN Uint16          chrmapturn_lr[MAXCHR];       //
EXTERN Uint16          chrmapturn_ud[MAXCHR];       //
EXTERN Uint16          chrtexture[MAXCHR];         // Character's skin
EXTERN bool_t          chractionready[MAXCHR];     // Ready to play a new one
EXTERN ACTION          chraction[MAXCHR];          // Character's action
EXTERN bool_t          chrkeepaction[MAXCHR];      // Keep the action playing
EXTERN bool_t          chrloopaction[MAXCHR];      // Loop it too
EXTERN ACTION          chrnextaction[MAXCHR];      // Character's action to play next
EXTERN Uint16          chrframe[MAXCHR];           // Character's frame
EXTERN Uint16          chrframelast[MAXCHR];       // Character's last frame
EXTERN float           chrflip[MAXCHR];
EXTERN Uint8           chrlip_fp8[MAXCHR];             // Character's frame in betweening
EXTERN Uint8           chrvrtar_fp8[MAXCHR][MAXVERTICES];// Lighting hack ( Ooze )
EXTERN Uint8           chrvrtag_fp8[MAXCHR][MAXVERTICES];// Lighting hack ( Ooze )
EXTERN Uint8           chrvrtab_fp8[MAXCHR][MAXVERTICES];// Lighting hack ( Ooze )
EXTERN Uint16          chrholdingwhich[MAXCHR][SLOT_COUNT]; // !=MAXCHR if character is holding something
EXTERN Uint16          chrattachedto[MAXCHR];      // !=MAXCHR if character is a held weapon
EXTERN Uint16          chrattachedgrip[MAXCHR][GRIP_SIZE];   // Vertices which describe the weapon grip
EXTERN Uint8           chralpha_fp8[MAXCHR];           // 255 = Solid, 0 = Invisible
EXTERN Uint8           chrlight_fp8[MAXCHR];           // 1 = Light, 0 = Normal
EXTERN Uint8           chrflashand[MAXCHR];        // 1,3,7,15,31 = Flash, 255 = Don't
EXTERN Uint8           chrlightambir_fp8[MAXCHR];      // 0-255, terrain light
EXTERN Uint8           chrlightambig_fp8[MAXCHR];      // 0-255, terrain light
EXTERN Uint8           chrlightambib_fp8[MAXCHR];      // 0-255, terrain light
EXTERN Uint8           chrlightspekr_fp8[MAXCHR];      // 0-255, terrain light
EXTERN Uint8           chrlightspekg_fp8[MAXCHR];      // 0-255, terrain light
EXTERN Uint8           chrlightspekb_fp8[MAXCHR];      // 0-255, terrain light
EXTERN Uint16          chrlightturn_lrr[MAXCHR];  // Character's light rotation 0 to 65535
EXTERN Uint16          chrlightturn_lrg[MAXCHR];  // Character's light rotation 0 to 65535
EXTERN Uint16          chrlightturn_lrb[MAXCHR];  // Character's light rotation 0 to 65535
EXTERN Uint8           chrsheen_fp8[MAXCHR];           // 0-15, how shiny it is
EXTERN bool_t          chrtransferblend[MAXCHR];   // Give transparency to weapons?
EXTERN bool_t          chrisitem[MAXCHR];          // Is it grabbable?
EXTERN bool_t          chrismount[MAXCHR];         // Can you ride it?
EXTERN Uint8           chrredshift[MAXCHR];        // Color channel shifting
EXTERN Uint8           chrgrnshift[MAXCHR];        //
EXTERN Uint8           chrblushift[MAXCHR];        //
EXTERN BData           chrbmpdata[MAXCHR];           // character bump size data
EXTERN BData           chrbmpdata_save[MAXCHR];
EXTERN Uint16          chrbumpnext[MAXCHR];        // Next character on fanblock
EXTERN float           chrbumpdampen[MAXCHR];      // Character bump mass
EXTERN Uint16          chrdirectionlast[MAXCHR];   // Direction of last attack/healing
EXTERN Uint8           chrdamagetypelast[MAXCHR];  // Last damage type
EXTERN bool_t          chrisplatform[MAXCHR];        // Can it be stood on
EXTERN Uint8           chrturnmode[MAXCHR];        // Turning mode
EXTERN Uint8           chrsneakspd[MAXCHR];        // Sneaking if above this speed
EXTERN Uint8           chrwalkspd[MAXCHR];         // Walking if above this speed
EXTERN Uint8           chrrunspd[MAXCHR];          // Running if above this speed
EXTERN DAMAGE          chrdamagetargettype[MAXCHR];// Type of damage for AI DamageTarget
EXTERN DAMAGE          chrreaffirmdamagetype[MAXCHR]; // For relighting torches
EXTERN Uint8           chrdamagemodifier_fp8[MAXCHR][MAXDAMAGETYPE];  // Resistances and inversion
EXTERN float           chrdamagetime[MAXCHR];      // Invincibility timer
EXTERN Uint8           chrdefense_fp8[MAXCHR];         // Base defense rating
EXTERN float           chrweight[MAXCHR];          // Weight ( for pressure plates )
EXTERN Uint8           chrpassage[MAXCHR];         // The passage associated with this character
EXTERN Uint32          chrmessage[MAXCHR];           // The last order given the character
EXTERN Uint8           chrmessagedata[MAXCHR];         // The rank of the character on the order chain
EXTERN Uint16          chronwhichplatform[MAXCHR]; // What are we standing on?
EXTERN Uint16          chrholdingweight[MAXCHR];   // For weighted buttons
EXTERN Sint32          chrmoney[MAXCHR];           // Money
EXTERN Sint16          chrlifereturn[MAXCHR];      // Regeneration/poison
EXTERN Sint16          chrmanacost[MAXCHR];        // Mana cost to use
EXTERN Uint32          chrstoppedby[MAXCHR];       // Collision mask
EXTERN int             chrexperience[MAXCHR];      // Experience
EXTERN int             chrexperiencelevel[MAXCHR]; // Experience Level
EXTERN Sint16          chrgrogtime[MAXCHR];        // Grog timer
EXTERN Sint16          chrdazetime[MAXCHR];        // Daze timer
EXTERN bool_t          chrnameknown[MAXCHR];       // Is the name known?
EXTERN bool_t          chrammoknown[MAXCHR];       // Is the ammo known?
EXTERN bool_t          chrhitready[MAXCHR];        // Was it just dropped?
EXTERN float           chrboretime[MAXCHR];        // Boredom timer
EXTERN float           chrcarefultime[MAXCHR];     // "You hurt me!" timer
EXTERN bool_t          chrcanbecrushed[MAXCHR];    // Crush in a door?
EXTERN SLOT            chrinwhichslot[MAXCHR];     // SLOT_LEFT or SLOT_RIGHT or SLOT_SADDLE
EXTERN bool_t          chrisequipped[MAXCHR];      // For boots and rings and stuff
EXTERN Uint16          chrfirstenchant[MAXCHR];    // Linked list for enchants
EXTERN Uint16          chrundoenchant[MAXCHR];     // Last enchantment spawned
EXTERN MISSLE_TYPE     chrmissiletreatment[MAXCHR];// For deflection, etc.
EXTERN Uint8           chrmissilecost[MAXCHR];     // Mana cost for each one
EXTERN Uint16          chrmissilehandler[MAXCHR];  // Who pays the bill for each one...
EXTERN Uint16          chrdamageboost[MAXCHR];     // Add to swipe damage
EXTERN bool_t          chroverlay[MAXCHR];         // Is this an overlay?  Track aitarget...
EXTERN vect3           chrpancakepos[MAXCHR];
EXTERN vect3           chrpancakevel[MAXCHR];


EXTERN Uint16          chraitype[MAXCHR];          // The AI script to run
EXTERN bool_t          chricon[MAXCHR];            // Show the icon?
EXTERN bool_t          chrcangrabmoney[MAXCHR];    // Picks up coins?
EXTERN bool_t          chrisplayer[MAXCHR];        // btrue = player
EXTERN bool_t          chrislocalplayer[MAXCHR];   // btrue = local player
EXTERN int             chraistate[MAXCHR];         // Short term memory for AI
EXTERN int             chraicontent[MAXCHR];       // More short term memory
EXTERN float           chraitime[MAXCHR];          // AI Timer
EXTERN Uint8           chraigoto[MAXCHR];          // Which waypoint
EXTERN Uint8           chraigotoadd[MAXCHR];       // Where to stick next
EXTERN float           chraigotox[MAXCHR][MAXWAY]; // Waypoint
EXTERN float           chraigotoy[MAXCHR][MAXWAY]; // Waypoint
EXTERN int             chraix[MAXCHR][MAXSTOR];    // Temporary values...  SetXY
EXTERN int             chraiy[MAXCHR][MAXSTOR];    //
EXTERN bool_t          chraimorphed[MAXCHR];       //Some various other stuff

EXTERN CHR_REF         chraitarget[MAXCHR];        // Who the AI is after
EXTERN CHR_REF         chraiowner[MAXCHR];         // The character's owner
EXTERN CHR_REF         chraichild[MAXCHR];         // The character's child
EXTERN CHR_REF         chraibumplast[MAXCHR];        // Last character it was bumped by
EXTERN CHR_REF         chraiattacklast[MAXCHR];      // Last character it was attacked by
EXTERN CHR_REF         chraihitlast[MAXCHR];         // Last character it hit

EXTERN Sint8           chrloopingchannel[MAXCHR];    // Channel number of the loop so
EXTERN float           chrloopingvolume[MAXCHR];     // Sound volume of the channel	

// [BEGIN] Character states that are like skill expansions
EXTERN bool_t     chrinvictus[MAXCHR];          // Totally invincible?
EXTERN bool_t     chrwaterwalk[MAXCHR];         // Always above watersurfacelevel?
EXTERN bool_t     chriskursed[MAXCHR];          // Can't be dropped?  Could this also mean damage debuff? Spell fizzle rate? etc.
EXTERN bool_t     chrcanseeinvisible[MAXCHR];   //
EXTERN bool_t     chrcanchannel[MAXCHR];        //
// [END] Character states that are like skill expansions

// [BEGIN] Skill Expansions
EXTERN bool_t     chrcanseekurse[MAXCHR];             // Can it see kurses?
EXTERN bool_t     chrcanusearcane[MAXCHR];            // Can use [WMAG] spells?
EXTERN bool_t     chrcanjoust[MAXCHR];                // Can it use a lance to joust?
EXTERN bool_t     chrcanusetech[MAXCHR];              // Can it use [TECH]items?
EXTERN bool_t     chrcanusedivine[MAXCHR];            // Can it use [HMAG] runes?
EXTERN bool_t     chrcandisarm[MAXCHR];               // Disarm and find traps [DISA]
EXTERN bool_t     chrcanbackstab[MAXCHR];             // Backstab and murder [STAB]
EXTERN bool_t     chrcanuseadvancedweapons[MAXCHR];   // Advanced weapons usage [AWEP]
EXTERN bool_t     chrcanusepoison[MAXCHR];            // Use poison without err [POIS]
EXTERN bool_t     chrcanread[MAXCHR];     // Can read books and scrolls
// [END]  Skill Expansions

void calc_cap_experience( Uint16 object );
int calc_chr_experience( Uint16 object, float level );
float calc_chr_level( Uint16 object );

#define SEEKURSEAND         31                      // Blacking flash
#define SEEINVISIBLE        128                     // Cutoff for invisible characters
#define INVISIBLE           20                      // The character can't be detected
EXTERN bool_t                    localseeinvisible;
EXTERN bool_t                    localseekurse;


//------------------------------------
//Enchantment variables
//------------------------------------

typedef enum eve_set_e
{
  SETDAMAGETYPE           = 0,
  SETNUMBEROFJUMPS,
  SETLIFEBARCOLOR,
  SETMANABARCOLOR,
  SETSLASHMODIFIER,       //Damage modifiers
  SETCRUSHMODIFIER,
  SETPOKEMODIFIER,
  SETHOLYMODIFIER,
  SETEVILMODIFIER,
  SETFIREMODIFIER,
  SETICEMODIFIER,
  SETZAPMODIFIER,
  SETFLASHINGAND,
  SETLIGHTBLEND,
  SETALPHABLEND,
  SETSHEEN,                //Shinyness
  SETFLYTOHEIGHT,
  SETWALKONWATER,
  SETCANSEEINVISIBLE,
  SETMISSILETREATMENT,
  SETCOSTFOREACHMISSILE,
  SETMORPH,                //Morph character?
  SETCHANNEL,               //Can channel life as mana?
  EVE_SET_COUNT
} EVE_SET;

typedef enum eve_add_e
{
  ADDJUMPPOWER = 0,
  ADDBUMPDAMPEN,
  ADDBOUNCINESS,
  ADDDAMAGE,
  ADDSIZE,
  ADDACCEL,
  ADDRED,             //Red shift
  ADDGRN,             //Green shift
  ADDBLU,             //Blue shift
  ADDDEFENSE,         //Defence adjustments
  ADDMANA,
  ADDLIFE,
  ADDSTRENGTH,
  ADDWISDOM,
  ADDINTELLIGENCE,
  ADDDEXTERITY,
  EVE_ADD_COUNT      // Number of adds
} EVE_ADD;

EXTERN Uint16    numfreeenchant;             // For allocating new ones
EXTERN Uint16    freeenchant[MAXENCHANT];    //

EXTERN bool_t          evevalid[MAXEVE];                       // Enchant.txt loaded?
EXTERN bool_t          eveoverride[MAXEVE];                    // Override other enchants?
EXTERN bool_t          everemoveoverridden[MAXEVE];            // Remove other enchants?
EXTERN bool_t          evesetyesno[MAXEVE][EVE_SET_COUNT];    // Set this value?
EXTERN Uint8           evesetvalue[MAXEVE][EVE_SET_COUNT];    // Value to use
EXTERN Sint8           eveaddvalue[MAXEVE][EVE_ADD_COUNT];    // The values to add
EXTERN bool_t          everetarget[MAXEVE];                    // Pick a weapon?
EXTERN bool_t          evekillonend[MAXEVE];                   // Kill the target on end?
EXTERN bool_t          evepoofonend[MAXEVE];                   // Spawn a poof on end?
EXTERN bool_t          eveendifcantpay[MAXEVE];                // End on out of mana
EXTERN bool_t          evestayifnoowner[MAXEVE];               // Stay if owner has died?
EXTERN Sint16          evetime[MAXEVE];                        // Time in seconds
EXTERN Sint8           eveendmessage[MAXEVE];                  // Message for end -1 for none
EXTERN Sint16          eveownermana[MAXEVE];                   // Boost values
EXTERN Sint16          eveownerlife[MAXEVE];                   //
EXTERN Sint16          evetargetmana[MAXEVE];                  //
EXTERN Sint16          evetargetlife[MAXEVE];                  //
EXTERN DAMAGE          evedontdamagetype[MAXEVE];              // Don't work if ...
EXTERN DAMAGE          eveonlydamagetype[MAXEVE];              // Only work if ...
EXTERN IDSZ            everemovedbyidsz[MAXEVE];               // By particle or [NONE]
EXTERN Uint16          evecontspawntime[MAXEVE];               // Spawn timer
EXTERN Uint8           evecontspawnamount[MAXEVE];             // Spawn amount
EXTERN Uint16          evecontspawnfacingadd[MAXEVE];          // Spawn in circle
EXTERN Uint16          evecontspawnpip[MAXEVE];                // Spawn type ( local )
EXTERN Sint8           eveendsound[MAXEVE];                    // Sound on end (-1 for none)
EXTERN Uint16          evefrequency[MAXEVE];                   // Sound frequency
EXTERN Uint16          eveoverlay[MAXEVE];                     // Spawn an overlay?
EXTERN bool_t          evecanseekurse[MAXEVE];                 // Allow target to see kurses?

EXTERN Uint8           encon[MAXENCHANT];                      // Enchantment on
EXTERN Uint16          enceve[MAXENCHANT];                     // The type
EXTERN Uint16          enctarget[MAXENCHANT];                  // Who it enchants
EXTERN Uint16          encnextenchant[MAXENCHANT];             // Next in the list
EXTERN Uint16          encowner[MAXENCHANT];                   // Who cast the enchant
EXTERN Uint16          encspawner[MAXENCHANT];                 // The spellbook character
EXTERN Uint16          encoverlay[MAXENCHANT];                 // The overlay character
EXTERN Sint16          encownermana[MAXENCHANT];               // Boost values
EXTERN Sint16          encownerlife[MAXENCHANT];               //
EXTERN Sint16          enctargetmana[MAXENCHANT];              //
EXTERN Sint16          enctargetlife[MAXENCHANT];              //
EXTERN bool_t          encsetyesno[MAXENCHANT][EVE_SET_COUNT]; // Was it set?
EXTERN Uint8           encsetsave[MAXENCHANT][EVE_SET_COUNT];  // The value to restore
EXTERN Sint16          encaddsave[MAXENCHANT][EVE_ADD_COUNT];  // The value to take away
EXTERN Sint16          enctime[MAXENCHANT];                    // Time before end
EXTERN float           encspawntime[MAXENCHANT];               // Time before spawn

typedef enum disenchant_mode_e
{
  LEAVE_ALL   = 0,
  LEAVE_FIRST,
  LEAVE_NONE,
} DISENCHANT_MODE;

//------------------------------------
//Particle variables
//------------------------------------
#define SPAWN_NOCHARACTER        255                 // For particles that spawn characters...

EXTERN float           textureoffset[256];         // For moving textures
EXTERN Uint16          dolist[MAXCHR];             // List of which characters to draw
EXTERN Uint16          numdolist;                  // How many in the list

EXTERN int             numfreeprt EQ( 0 );                         // For allocation
EXTERN PRT_REF         freeprtlist[MAXPRT];                        //
EXTERN bool_t          prton[MAXPRT];                              // Does it exist?
EXTERN Uint16          prtpip[MAXPRT];                             // The part template
EXTERN Uint16          prtmodel[MAXPRT];                           // Pip spawn model
EXTERN CHR_REF         prtattachedtochr[MAXPRT];             // For torch flame
EXTERN Uint16          prtvertoffset[MAXPRT];                      // The vertex it's on (counting backward from max vertex)
EXTERN PRTTYPE         prttype[MAXPRT];                            // Transparency mode, 0-2
EXTERN Uint8           prtalpha_fp8[MAXPRT];
EXTERN Uint16          prtfacing[MAXPRT];                          // Direction of the part
EXTERN Uint8           prtteam[MAXPRT];                            // Team
EXTERN vect3           prtpos[MAXPRT];                            // Position
EXTERN vect3           prtvel[MAXPRT];                            // Velocity
EXTERN float           prtlevel[MAXPRT];                           // Height of tile
EXTERN vect3           prtpos_old[MAXPRT];                            // Position
EXTERN vect3           prtaccum_acc[MAXPRT];         //
EXTERN vect3           prtaccum_vel[MAXPRT];         //
EXTERN vect3           prtaccum_pos[MAXPRT];         //
EXTERN Uint8           prtspawncharacterstate[MAXPRT];             //
EXTERN Uint16          prtrotate[MAXPRT];                          // Rotation direction
EXTERN Sint16          prtrotateadd[MAXPRT];                       // Rotation rate
EXTERN Uint32          prtonwhichfan[MAXPRT];                      // Where the part is
EXTERN Uint16          prtsize_fp8[MAXPRT];                        // Size of particle
EXTERN Sint16          prtsizeadd_fp8[MAXPRT];                         // Change in size
EXTERN bool_t          prtinview[MAXPRT];                          // Render this one?
EXTERN Uint32          prtimage_fp8[MAXPRT];                       // Which image
EXTERN Uint32          prtimageadd_fp8[MAXPRT];                        // Animation rate
EXTERN Uint32          prtimagemax_fp8[MAXPRT];                        // End of image loop
EXTERN Uint32          prtimagestt_fp8[MAXPRT];                    // Start of image loop
EXTERN Uint8           prtlightr_fp8[MAXPRT];                           // Light level
EXTERN Uint8           prtlightg_fp8[MAXPRT];                           // Light level
EXTERN Uint8           prtlightb_fp8[MAXPRT];                           // Light level
EXTERN float           prttime[MAXPRT];                            // Duration of particle
EXTERN bool_t          prtgopoof[MAXPRT];                          // Are we gone?
EXTERN float           prtspawntime[MAXPRT];                       // Time until spawn
EXTERN Uint8           prtbumpsize[MAXPRT];                        // Size of bumpers
EXTERN Uint8           prtbumpsizebig[MAXPRT];                     //
EXTERN Uint8           prtbumpheight[MAXPRT];                      // Bounding box height
EXTERN float           prtbumpstrength[MAXPRT];                    // The amount of interaction
EXTERN float           prtweight[MAXPRT];                          // The mass of the particle
EXTERN PRT_REF         prtbumpnext[MAXPRT];                        // Next particle on fanblock
EXTERN BData           prtbmpdata[MAXPRT];                         // particle bump size data
EXTERN PAIR            prtdamage[MAXPRT];                          // For strength
EXTERN DAMAGE          prtdamagetype[MAXPRT];                      // Damage type
EXTERN CHR_REF         prtowner[MAXPRT];                           // The character that is attacking
EXTERN float           prtdynalightfalloff[MAXPRT];                // Dyna light...
EXTERN float           prtdynalightlevel[MAXPRT];                  //
EXTERN bool_t          prtdynalighton[MAXPRT];                     // Dynamic light?
EXTERN CHR_REF         prttarget[MAXPRT];                          // Who it's chasing
EXTERN Uint16          particletexture;                            // All in one bitmap
EXTERN Uint16          prttexw;
EXTERN Uint16          prttexh;
EXTERN float           prttexwscale EQ( 1.0f );
EXTERN float           prttexhscale EQ( 1.0f );

#define VALID_PRT(XX) ( ((XX)>=0) && ((XX)<MAXPRT) && prton[XX] )
#define VALIDATE_PRT(XX) ( VALID_PRT(XX) ? (XX) : MAXPRT )

PRT_REF prt_get_bumpnext( PRT_REF iprt );
CHR_REF prt_get_owner( PRT_REF iprt );
CHR_REF prt_get_target( PRT_REF iprt );


#define CALCULATE_PRT_U0(CNT)  (((.05f+(CNT&15))/16.0f)*prttexwscale)
#define CALCULATE_PRT_U1(CNT)  (((.95f+(CNT&15))/16.0f)*prttexwscale)
#define CALCULATE_PRT_V0(CNT)  (((.05f+(CNT/16))/16.0f) * ((float)prttexw/(float)prttexh)*prttexhscale)
#define CALCULATE_PRT_V1(CNT)  (((.95f+(CNT/16))/16.0f) * ((float)prttexw/(float)prttexh)*prttexhscale)

//------------------------------------
//Module variables
//------------------------------------
#define RANKSIZE 8
#define SUMMARYLINES 8
#define SUMMARYSIZE  80
EXTERN int             globalnummodule;                            // Number of modules
EXTERN char            modrank[MAXMODULE][RANKSIZE];               // Number of stars
EXTERN char            modlongname[MAXMODULE][MAXCAPNAMESIZE];     // Module names
EXTERN char            modsummaryval;
EXTERN char            modloadname[MAXMODULE][MAXCAPNAMESIZE];     // Module load names
EXTERN Uint8           modimportamount[MAXMODULE];                 // # of import characters
EXTERN bool_t          modallowexport[MAXMODULE];                  // Export characters?
EXTERN Uint8           modminplayers[MAXMODULE];                   // Number of players
EXTERN Uint8           modmaxplayers[MAXMODULE];                   //
EXTERN bool_t          modmonstersonly[MAXMODULE];                 // Only allow monsters
EXTERN bool_t          modrespawnvalid[MAXMODULE];                 // Allow respawn
EXTERN int             numlines;                                   // Lines in summary
EXTERN char            modsummary[SUMMARYLINES][SUMMARYSIZE];      // Quest description


//------------------------------------
//Model stuff
//------------------------------------

#define MAXFRAMESPERANIM 16

EXTERN int             globalnumicon;                              // Number of icons
EXTERN Uint16          madloadframe;                               // Where to load next
EXTERN bool_t          madused[MAXMODEL];                          // Model slot
EXTERN STRING          madname[MAXMODEL];                          // Model name
EXTERN MD2_Model *     mad_md2[MAXMODEL];                          // Md2 model pointer
EXTERN Uint16          madskintoicon[MAXTEXTURE];                  // Skin to icon
EXTERN Uint16          madskins[MAXMODEL];                         // Number of skins
EXTERN Uint16          madskinstart[MAXMODEL];                     // Starting skin of model
EXTERN Uint16          madmsgstart[MAXMODEL];                      // The first message
EXTERN Uint16          madvertices[MAXMODEL];                      // Number of vertices
EXTERN Uint16          madtransvertices[MAXMODEL];                 // Number to transform
EXTERN Uint8  *        madframelip[MAXMODEL];                      // 0-15, How far into action is each frame
EXTERN Uint16 *        madframefx[MAXMODEL];                       // Invincibility, Spawning
EXTERN Uint16          madframeliptowalkframe[MAXMODEL][LIPT_COUNT][MAXFRAMESPERANIM];    // For walk animations
EXTERN Uint16          madai[MAXMODEL];                            // AI for each model
EXTERN bool_t          madactionvalid[MAXMODEL][MAXACTION];        // bfalse if not valid
EXTERN Uint16          madactionstart[MAXMODEL][MAXACTION];        // First frame of anim
EXTERN Uint16          madactionend[MAXMODEL][MAXACTION];          // One past last frame
EXTERN Uint16          madprtpip[MAXMODEL][PRTPIP_PEROBJECT_COUNT];    // Local particles


// Character profiles

#define CAP_INHERIT_IDSZ(model,idsz) (capidsz[model][IDSZ_PARENT] == (IDSZ)(idsz) || capidsz[model][IDSZ_TYPE] == (IDSZ)(idsz))
#define CAP_INHERIT_IDSZ_RANGE(model,idszmin,idszmax) ( (capidsz[model][IDSZ_PARENT] >= (IDSZ)(idszmin && capidsz[model][IDSZ_PARENT] <= (IDSZ)(idszmax))) || (capidsz[model][IDSZ_TYPE] >= (IDSZ)(idszmin && capidsz[model][IDSZ_TYPE] <= (IDSZ)(idszmax))) )

#define MAXSKIN 4
EXTERN int           importobject;
EXTERN short         capimportslot[MAXMODEL];
EXTERN char          capclassname[MAXMODEL][MAXCAPNAMESIZE];     // Class name
EXTERN Sint8         capskinoverride[MAXMODEL];                  // -1 or 0-3.. For import
EXTERN Uint8         capleveloverride[MAXMODEL];                 // 0 for normal
EXTERN int           capstateoverride[MAXMODEL];                 // 0 for normal
EXTERN int           capcontentoverride[MAXMODEL];               // 0 for normal
EXTERN bool_t        capskindressy[MAXMODEL];                    // Dressy
EXTERN float         capstrengthdampen[MAXMODEL];                // Strength damage factor
EXTERN Uint8         capstoppedby[MAXMODEL];                     // Collision Mask
EXTERN bool_t        capuniformlit[MAXMODEL];                    // Bad lighting?
EXTERN Uint8         caplifecolor[MAXMODEL];                     // Bar colors
EXTERN Uint8         capmanacolor[MAXMODEL];                     //
EXTERN Uint8         capammomax[MAXMODEL];                       // Ammo stuff
EXTERN Uint8         capammo[MAXMODEL];                          //
EXTERN GENDER        capgender[MAXMODEL];                        // Gender
EXTERN PAIR          caplife_fp8[MAXMODEL];                      // Life
EXTERN PAIR          caplifeperlevel_fp8[MAXMODEL];              //
EXTERN Sint16        caplifereturn_fp8[MAXMODEL];                //
EXTERN Sint16        capmoney[MAXMODEL];                         // Money
EXTERN Uint16        caplifeheal_fp8[MAXMODEL];                  //
EXTERN PAIR          capmana_fp8[MAXMODEL];                      // Mana
EXTERN Sint16        capmanacost_fp8[MAXMODEL];                  //
EXTERN PAIR          capmanaperlevel_fp8[MAXMODEL];              //
EXTERN PAIR          capmanareturn_fp8[MAXMODEL];                //
EXTERN PAIR          capmanareturnperlevel_fp8[MAXMODEL];        //
EXTERN PAIR          capmanaflow_fp8[MAXMODEL];                  //
EXTERN PAIR          capmanaflowperlevel_fp8[MAXMODEL];          //
EXTERN PAIR          capstrength_fp8[MAXMODEL];                  // Strength
EXTERN PAIR          capstrengthperlevel_fp8[MAXMODEL];          //
EXTERN PAIR          capwisdom_fp8[MAXMODEL];                    // Wisdom
EXTERN PAIR          capwisdomperlevel_fp8[MAXMODEL];            //
EXTERN PAIR          capintelligence_fp8[MAXMODEL];              // Intlligence
EXTERN PAIR          capintelligenceperlevel_fp8[MAXMODEL];      //
EXTERN PAIR          capdexterity_fp8[MAXMODEL];                 // Dexterity
EXTERN PAIR          capdexterityperlevel_fp8[MAXMODEL];         //
EXTERN float         capsize[MAXMODEL];                          // Scale of model
EXTERN float         capsizeperlevel[MAXMODEL];                  // Scale increases
EXTERN float         capdampen[MAXMODEL];                        // Bounciness
EXTERN float         capbumpstrength[MAXMODEL];                  // ghostlike interaction with objects?
EXTERN Uint8         capshadowsize[MAXMODEL];                    // Shadow size
EXTERN float         capbumpsize[MAXMODEL];                      // Bounding octagon
EXTERN float         capbumpsizebig[MAXMODEL];                   // For octagonal bumpers
EXTERN float         capbumpheight[MAXMODEL];                    //
EXTERN float         capbumpdampen[MAXMODEL];                    // Mass
EXTERN float         capweight[MAXMODEL];                        // Weight
EXTERN float         capjump[MAXMODEL];                          // Jump power
EXTERN Uint8         capjumpnumber[MAXMODEL];                    // Number of jumps ( Ninja )
EXTERN Uint8         capsneakspd[MAXMODEL];                      // Sneak threshold
EXTERN Uint8         capwalkspd[MAXMODEL];                       // Walk threshold
EXTERN Uint8         caprunspd[MAXMODEL];                        // Run threshold
EXTERN Uint8         capflyheight[MAXMODEL];                     // Fly height
EXTERN Uint8         capflashand[MAXMODEL];                      // Flashing rate
EXTERN Uint8         capalpha_fp8[MAXMODEL];                     // Transparency
EXTERN Uint8         caplight_fp8[MAXMODEL];                     // Light blending
EXTERN bool_t        captransferblend[MAXMODEL];                 // Transfer blending to rider/weapons
EXTERN Uint8         capsheen_fp8[MAXMODEL];                     // How shiny it is ( 0-15 )
EXTERN bool_t        capenviro[MAXMODEL];                        // Phong map this baby?
EXTERN Uint16        capuoffvel[MAXMODEL];                       // Texture movement rates
EXTERN Uint16        capvoffvel[MAXMODEL];                       //
EXTERN bool_t        capstickybutt[MAXMODEL];                    // Stick to the ground?
EXTERN Uint16        capiframefacing[MAXMODEL];                  // Invincibility frame
EXTERN Uint16        capiframeangle[MAXMODEL];                   //
EXTERN Uint16        capnframefacing[MAXMODEL];                  // Normal frame
EXTERN Uint16        capnframeangle[MAXMODEL];                   //
EXTERN int           capexperienceforlevel[MAXMODEL][MAXLEVEL];  // Experience needed for next level
EXTERN float         capexperienceconst[MAXMODEL];
EXTERN float         capexperiencecoeff[MAXMODEL];
EXTERN PAIR          capexperience[MAXMODEL];                    // Starting experience
EXTERN int           capexperienceworth[MAXMODEL];               // Amount given to killer/user
EXTERN float         capexperienceexchange[MAXMODEL];            // Adds to worth
EXTERN float         capexperiencerate[MAXMODEL][XP_COUNT];
EXTERN IDSZ          capidsz[MAXMODEL][IDSZ_COUNT];                 // ID strings
EXTERN bool_t        capisitem[MAXMODEL];                        // Is it an item?
EXTERN bool_t        capismount[MAXMODEL];                       // Can you ride it?
EXTERN bool_t        capisstackable[MAXMODEL];                   // Is it arrowlike?
EXTERN bool_t        capnameknown[MAXMODEL];                     // Is the class name known?
EXTERN bool_t        capusageknown[MAXMODEL];                    // Is its usage known
EXTERN bool_t        capcancarrytonextmodule[MAXMODEL];          // Take it with you?
EXTERN bool_t        capneedskillidtouse[MAXMODEL];              // Check IDSZ first?
EXTERN bool_t        capisplatform[MAXMODEL];                      // Can be stood on?
EXTERN bool_t        capcanuseplatforms[MAXMODEL];               // Can use platforms?
EXTERN bool_t        capcangrabmoney[MAXMODEL];                  // Collect money?
EXTERN bool_t        capcanopenstuff[MAXMODEL];                  // Open chests/doors?
EXTERN bool_t        capicon[MAXMODEL];                          // Draw icon
EXTERN bool_t        capforceshadow[MAXMODEL];                   // Draw a shadow?
EXTERN bool_t        capripple[MAXMODEL];                        // Spawn ripples?
EXTERN DAMAGE        capdamagetargettype[MAXMODEL];              // For AI DamageTarget
EXTERN ACTION        capweaponaction[MAXMODEL];                  // Animation needed to swing
EXTERN bool_t        capslotvalid[MAXMODEL][SLOT_COUNT];         // Left/Right hands valid
EXTERN bool_t        capattackattached[MAXMODEL];                //
EXTERN Sint8         capattackprttype[MAXMODEL];                 //
EXTERN Uint8         capattachedprtamount[MAXMODEL];             // Sticky particles
EXTERN DAMAGE        capattachedprtreaffirmdamagetype[MAXMODEL]; // Relight that torch...
EXTERN Uint16        capattachedprttype[MAXMODEL];               //
EXTERN Uint8         capgopoofprtamount[MAXMODEL];               // Poof effect
EXTERN Sint16        capgopoofprtfacingadd[MAXMODEL];            //
EXTERN Uint16        capgopoofprttype[MAXMODEL];                 //
EXTERN Uint8         capbludlevel[MAXMODEL];                     // Blud ( yuck )
EXTERN Uint16        capbludprttype[MAXMODEL];                   //
EXTERN Sint8         capfootfallsound[MAXMODEL];                 // Footfall sound, -1
EXTERN Sint8         capjumpsound[MAXMODEL];                     // Jump sound, -1
EXTERN Uint8         capkursechance[MAXMODEL];                   // Chance of being kursed
EXTERN Sint8         caphidestate[MAXCHR];                       // Don't draw when...

EXTERN Uint8         capdefense_fp8[MAXMODEL][MAXSKIN];                // Defense for each skin
EXTERN char          capskinname[MAXMODEL][MAXSKIN][MAXCAPNAMESIZE];   // Skin name
EXTERN Uint16        capskincost[MAXMODEL][MAXSKIN];                   // Store prices
EXTERN Uint8         capdamagemodifier_fp8[MAXMODEL][MAXDAMAGETYPE][MAXSKIN];
EXTERN float         capmaxaccel[MAXMODEL][MAXSKIN];                   // Acceleration for each skin

// [BEGIN] Character template parameters that are like Skill Expansions
EXTERN bool_t        capistoobig[MAXMODEL];                      // Can't be put in pack
EXTERN bool_t        capreflect[MAXMODEL];                       // Draw the reflection
EXTERN bool_t        capalwaysdraw[MAXMODEL];                    // Always render
EXTERN bool_t        capisranged[MAXMODEL];                      // Flag for ranged weapon
EXTERN bool_t        capisequipment[MAXCHR];                     // Behave in silly ways
EXTERN bool_t        capridercanattack[MAXMODEL];                // Rider attack?
EXTERN bool_t        capcanbedazed[MAXMODEL];                    // Can it be dazed?
EXTERN bool_t        capcanbegrogged[MAXMODEL];                  // Can it be grogged?
EXTERN bool_t        capresistbumpspawn[MAXMODEL];               // Don't catch fire
EXTERN bool_t        capwaterwalk[MAXMODEL];                     // Walk on water?
EXTERN bool_t        capinvictus[MAXMODEL];                      // Is it invincible?
EXTERN bool_t        capcanseeinvisible[MAXMODEL];               // Can it see invisible?
// [END] Character template parameters that are like Skill Expansions


// [BEGIN] Skill Expansions
EXTERN bool_t     capcanseekurse[MAXMODEL];                   // Can it see kurses?
EXTERN bool_t     capcanusearcane[MAXMODEL];      // Can use [WMAG] spells?
EXTERN bool_t     capcanjoust[MAXMODEL];       // Can it use a lance to joust?
EXTERN bool_t     capcanusetech[MAXMODEL];      // Can it use [TECH]items?
EXTERN bool_t     capcanusedivine[MAXMODEL];      // Can it use [HMAG] runes?
EXTERN bool_t     capcandisarm[MAXMODEL];         // Disarm and find traps [DISA]
EXTERN bool_t     capcanbackstab[MAXCHR];         // Backstab and murder [STAB]
EXTERN bool_t     capcanuseadvancedweapons[MAXCHR];   // Advanced weapons usage [AWEP]
EXTERN bool_t     capcanusepoison[MAXCHR];         // Use poison without err [POIS]
EXTERN bool_t     capcanread[MAXCHR];   // Can read books and scrolls [READ]
// [END] Skill Expansions


// Particle template
typedef enum dynalight_mode_e
{
  DYNA_OFF = 0,
  DYNA_ON,
  DYNA_LOCAL,
} DYNA_MODE;

#define DYNAFANS  12
#define MAXFALLOFF 1400

EXTERN int             numpip  EQ( 0 );
EXTERN Uint8           pipforce[MAXPRTPIP];                        // Force spawn?
EXTERN STRING          pipfname[MAXPRTPIP];
EXTERN STRING          pipcomment[MAXPRTPIP];
EXTERN PRTTYPE         piptype[MAXPRTPIP];                         // Transparency mode
EXTERN Uint8           pipnumframes[MAXPRTPIP];                    // Number of frames
EXTERN Uint16          pipimagebase[MAXPRTPIP];                    // Starting image
EXTERN PAIR            pipimageadd[MAXPRTPIP];                     // Frame rate
EXTERN Uint16          piptime[MAXPRTPIP];                         // Time until end
EXTERN PAIR            piprotate[MAXPRTPIP];                       // Rotation
EXTERN Uint16          piprotateadd[MAXPRTPIP];                    // Rotation
EXTERN Uint16          pipsizebase_fp8[MAXPRTPIP];                 // Size
EXTERN Sint16          pipsizeadd[MAXPRTPIP];                      // Size rate
EXTERN float           pipspdlimit[MAXPRTPIP];                     // Speed limit
EXTERN float           pipdampen[MAXPRTPIP];                       // Bounciness
EXTERN Sint8           pipbumpmoney[MAXPRTPIP];                    // Value of particle
EXTERN Uint8           pipbumpsize[MAXPRTPIP];                     // Bounding box size
EXTERN Uint8           pipbumpheight[MAXPRTPIP];                   // Bounding box height
EXTERN float           pipbumpstrength[MAXPRTPIP];
EXTERN bool_t          pipendwater[MAXPRTPIP];                     // End if underwater
EXTERN bool_t          pipendbump[MAXPRTPIP];                      // End if bumped
EXTERN bool_t          pipendground[MAXPRTPIP];                    // End if on ground
EXTERN bool_t          pipendwall[MAXPRTPIP];                      // End if hit a wall
EXTERN bool_t          pipendlastframe[MAXPRTPIP];                 // End on last frame
EXTERN PAIR            pipdamage_fp8[MAXPRTPIP];                   // Damage
EXTERN DAMAGE          pipdamagetype[MAXPRTPIP];                   // Damage type
EXTERN PAIR            pipfacing[MAXPRTPIP];                       // Facing
EXTERN Uint16          pipfacingadd[MAXPRTPIP];                    // Facing rotation
EXTERN PAIR            pipxyspacing[MAXPRTPIP];                    // Spacing
EXTERN PAIR            pipzspacing[MAXPRTPIP];                     // Altitude
EXTERN PAIR            pipxyvel[MAXPRTPIP];                    // Shot velocity
EXTERN PAIR            pipzvel[MAXPRTPIP];                     // Up velocity
EXTERN Uint16          pipcontspawntime[MAXPRTPIP];                // Spawn timer
EXTERN Uint8           pipcontspawnamount[MAXPRTPIP];              // Spawn amount
EXTERN Uint16          pipcontspawnfacingadd[MAXPRTPIP];           // Spawn in circle
EXTERN Uint16          pipcontspawnpip[MAXPRTPIP];                 // Spawn type ( local )
EXTERN Uint8           pipendspawnamount[MAXPRTPIP];               // Spawn amount
EXTERN Uint16          pipendspawnfacingadd[MAXPRTPIP];            // Spawn in circle
EXTERN Uint16          pipendspawnpip[MAXPRTPIP];                  // Spawn type ( local )
EXTERN Uint8           pipbumpspawnamount[MAXPRTPIP];              // Spawn amount
EXTERN Uint16          pipbumpspawnpip[MAXPRTPIP];                 // Spawn type ( global )
EXTERN DYNA_MODE       pipdynalightmode[MAXPRTPIP];                // Dynamic light on?
EXTERN float           pipdynalevel[MAXPRTPIP];                    // Intensity
EXTERN Uint16          pipdynafalloff[MAXPRTPIP];                  // Falloff
EXTERN Uint16          pipdazetime[MAXPRTPIP];                     // Daze
EXTERN Uint16          pipgrogtime[MAXPRTPIP];                     // Drunkeness
EXTERN Sint8           pipsoundspawn[MAXPRTPIP];                   // Beginning sound
EXTERN Sint8           pipsoundend[MAXPRTPIP];                     // Ending sound
EXTERN Sint8           pipsoundfloor[MAXPRTPIP];                   // Floor sound
EXTERN Sint8           pipsoundwall[MAXPRTPIP];                    // Ricochet sound
EXTERN bool_t          pipfriendlyfire[MAXPRTPIP];                 // Friendly fire
EXTERN bool_t          piprotatetoface[MAXPRTPIP];                 // Arrows/Missiles
EXTERN bool_t          pipcausepancake[MAXPRTPIP];                 // Cause pancake?
EXTERN bool_t          pipcauseknockback[MAXPRTPIP];               // Cause knockback?
EXTERN bool_t          pipnewtargetonspawn[MAXPRTPIP];             // Get new target?
EXTERN bool_t          piphoming[MAXPRTPIP];                       // Homing?
EXTERN Uint16          piptargetangle[MAXPRTPIP];                  // To find target
EXTERN float           piphomingaccel[MAXPRTPIP];                  // Acceleration rate
EXTERN float           piphomingfriction[MAXPRTPIP];               // Deceleration rate
EXTERN float           pipdynalightleveladd[MAXPRTPIP];            // Dyna light changes
EXTERN float           pipdynalightfalloffadd[MAXPRTPIP];          //
EXTERN bool_t          piptargetcaster[MAXPRTPIP];                 // Target caster?
EXTERN bool_t          pipspawnenchant[MAXPRTPIP];                 // Spawn enchant?
EXTERN bool_t          pipneedtarget[MAXPRTPIP];                   // Need a target?
EXTERN bool_t          piponlydamagefriendly[MAXPRTPIP];           // Only friends?
EXTERN bool_t          piphateonly[MAXPRTPIP];      // Only enemies? !!BAD NOT DONE!!
EXTERN bool_t          pipstartontarget[MAXPRTPIP];                // Start on target?
EXTERN int             pipzaimspd[MAXPRTPIP];                      // [ZSPD] For Z aiming
EXTERN Uint16          pipdamfx[MAXPRTPIP];                        // Damage effects
EXTERN bool_t          pipallowpush[MAXPRTPIP];                    //Allow particle to push characters around
EXTERN bool_t          pipintdamagebonus[MAXPRTPIP];               //Add intelligence as damage bonus
EXTERN bool_t          pipwisdamagebonus[MAXPRTPIP];               //Add wisdom as damage bonus
EXTERN float           pipmanadrain[MAXPRTPIP];                      //Reduce target mana by this amount
EXTERN float           piplifedrain[MAXPRTPIP];                      //Reduce target mana by this amount
EXTERN bool_t          piprotatewithattached[MAXPRTPIP];           // do attached particles rotate with the object?

// The ID number for host searches
// {A0F72DE8-2C17-11d3-B7FE-444553540000}
/* PORT
DEFINE_GUID(NETWORKID, 0xa0f72de8, 0x2c17, 0x11d3, 0xb7, 0xfe, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
*/
#define MAXSERVICE 16
#define NETNAMESIZE 16
#define MAXSESSION 16
#define MAXNETPLAYER 8
EXTERN Uint32      randsave;         //Used in network timer
EXTERN int                     networkservice;
EXTERN int                     numservice  EQ( 0 );                           // How many we found
EXTERN char                    netservicename[MAXSERVICE][NETNAMESIZE];    // Names of services
EXTERN int                     numsession  EQ( 0 );                           // How many we found
EXTERN char                    netsessionname[MAXSESSION][NETNAMESIZE];    // Names of sessions
EXTERN int                     numplayer  EQ( 0 );                            // How many we found
EXTERN char                    netplayername[MAXNETPLAYER][NETNAMESIZE];   // Names of machines

EXTERN char *globalname  EQ( NULL );   // For debuggin' fgoto_colon
EXTERN char *globalparsename  EQ( NULL );  // The SCRIPT.TXT filename
EXTERN FILE *globalparseerr  EQ( NULL );  // For debuggin' scripted AI
EXTERN FILE *globalnetworkerr  EQ( NULL );  // For debuggin' network


EXTERN float           indextoenvirox[MD2LIGHTINDICES];                    // Environment map
EXTERN float           lighttoenviroy[256];                                // Environment map
EXTERN Uint32          lighttospek[MAXSPEKLEVEL][256];                     //


EXTERN float           hillslide  EQ( 1.00 );                                 //
EXTERN float           slippyfriction  EQ( 1.00 );   //1.05 for Chevron          // Friction
EXTERN float           airfriction  EQ( .95 );                                //
EXTERN float           waterfriction  EQ( .85 );                              //
EXTERN float           noslipfriction  EQ( 0.95 );                            //
EXTERN float           platstick  EQ( .040 );                                 //
EXTERN float           gravity  EQ(( float ) - 1.0 );                         // Gravitational accel


EXTERN char            cFrameName[16];                                     // MD2 Frame Name

//AI Targeting
EXTERN bool_t  search_initialize;
EXTERN CHR_REF search_besttarget;                                      // For find_target
EXTERN Uint16  search_bestangle;                                       //
EXTERN Uint16  search_useangle;                                        //
EXTERN int     search_bestdistance;
EXTERN CHR_REF search_nearest;
EXTERN float   search_distance;




EXTERN Uint8   asciitofont[256];                                   // Conversion table


// Display messages
EXTERN Uint16  msgtimechange EQ( 0 );                                  //
EXTERN Uint16  msgstart EQ( 0 );                                       // The message queue
EXTERN Sint16    msgtime[MAXMESSAGE];                                //
EXTERN char            msgtextdisplay[MAXMESSAGE][MESSAGESIZE];            // The displayed text


// Message files
EXTERN Uint16  msgtotal EQ( 0 );                                       // The number of messages
EXTERN Uint32  msgtotalindex EQ( 0 );                                  // Where to put letter
EXTERN Uint32  msgindex[MAXTOTALMESSAGE];						       // Where it is
EXTERN char    msgtext[MESSAGEBUFFERSIZE];							   // The text buffer


// My lil' random number table
#define MAXRAND 4096
EXTERN Uint16 randie[MAXRAND];
EXTERN Uint16 randindex;
#define RANDIE randie[randindex]; randindex++; randindex %= MAXRAND;


#define MAXENDTEXT 1024
EXTERN char generictext[80];         // Use for whatever purpose
EXTERN char endtext[MAXENDTEXT];     // The end-module text
EXTERN int endtextwrite;


// This is for random naming
EXTERN Uint16          numchop  EQ( 0 );              // The number of name parts
EXTERN Uint32            chopwrite  EQ( 0 );            // The data pointer
EXTERN char                    chopdata[CHOPDATACHUNK];    // The name parts
EXTERN Uint16          chopstart[MAXCHOP];         // The first character of each part
EXTERN Uint16          capsectionsize[MAXMODEL][MAXSECTION];   // Number of choices, 0
EXTERN Uint16          capsectionstart[MAXMODEL][MAXSECTION];  //
EXTERN char                    namingnames[MAXCAPNAMESIZE];// The name returned by the function

// These are for the AI script loading/parsing routines
extern int                     iNumAis;
#define HAS_SOME_BITS(XX,YY) (0 != ((XX)&(YY)))
#define HAS_ALL_BITS(XX,YY)  ((YY) == ((XX)&(YY)))
#define HAS_NO_BITS(XX,YY)   (0 == ((XX)&(YY)))
#define MISSING_BITS(XX,YY)  (HAS_SOME_BITS(XX,YY) && !HAS_ALL_BITS(XX,YY))

typedef enum alert_bits_e
{
  ALERT_NONE                       =       0,
  ALERT_SPAWNED                    = 1 <<  0,
  ALERT_HITVULNERABLE              = 1 <<  1,
  ALERT_ATWAYPOINT                 = 1 <<  2,
  ALERT_ATLASTWAYPOINT             = 1 <<  3,
  ALERT_ATTACKED                   = 1 <<  4,
  ALERT_BUMPED                     = 1 <<  5,
  ALERT_SIGNALED                    = 1 <<  6,
  ALERT_CALLEDFORHELP              = 1 <<  7,
  ALERT_KILLED                     = 1 <<  8,
  ALERT_TARGETKILLED               = 1 <<  9,
  ALERT_DROPPED                    = 1 << 10,
  ALERT_GRABBED                    = 1 << 11,
  ALERT_REAFFIRMED                 = 1 << 12,
  ALERT_LEADERKILLED               = 1 << 13,
  ALERT_USED                       = 1 << 14,
  ALERT_CLEANEDUP                  = 1 << 15,
  ALERT_SCOREDAHIT                 = 1 << 16,
  ALERT_HEALED                     = 1 << 17,
  ALERT_DISAFFIRMED                = 1 << 18,
  ALERT_CHANGED                    = 1 << 19,
  ALERT_INWATER                    = 1 << 20,
  ALERT_BORED                      = 1 << 21,
  ALERT_TOOMUCHBAGGAGE             = 1 << 22,
  ALERT_GROGGED                    = 1 << 23,
  ALERT_DAZED                      = 1 << 24,
  ALERT_HITGROUND                  = 1 << 25,
  ALERT_NOTDROPPED                 = 1 << 26,
  ALERT_BLOCKED                    = 1 << 27,
  ALERT_THROWN                     = 1 << 28,
  ALERT_CRUSHED                    = 1 << 29,
  ALERT_NOTPUTAWAY                 = 1 << 30,
  ALERT_TAKENOUT                   = 1 << 31
} ALERT_BITS;

typedef enum script_opcode_e
{
  F_IfSpawned = 0, // 0                     // Scripted AI functions (v0.10)
  F_IfTimeOut, // 1
  F_IfAtWaypoint, // 2
  F_IfAtLastWaypoint, // 3
  F_IfAttacked, // 4
  F_IfBumped, // 5
  F_IfSignaled, // 6
  F_IfCalledForHelp, // 7
  F_SetContent, // 8
  F_IfKilled, // 9
  F_IfTargetKilled, // 10
  F_ClearWaypoints, // 11
  F_AddWaypoint, // 12
  F_FindPath, // 13
  F_Compass, // 14
  F_GetTargetArmorPrice, // 15
  F_SetTime, // 16
  F_GetContent, // 17
  F_JoinTargetTeam, // 18
  F_SetTargetToNearbyEnemy, // 19
  F_SetTargetToTargetLeftHand, // 20
  F_SetTargetToTargetRightHand, // 21
  F_SetTargetToWhoeverAttacked, // 22
  F_SetTargetToWhoeverBumped, // 23
  F_SetTargetToWhoeverCalledForHelp, // 24
  F_SetTargetToOldTarget, // 25
  F_SetTurnModeToVelocity, // 26
  F_SetTurnModeToWatch, // 27
  F_SetTurnModeToSpin, // 28
  F_SetBumpHeight, // 29
  F_IfTargetHasID, // 30
  F_IfTargetHasItemID, // 31
  F_IfTargetHoldingItemID, // 32
  F_IfTargetHasSkillID, // 33
  F_Else, // 34
  F_Run, // 35
  F_Walk, // 36
  F_Sneak, // 37
  F_DoAction, // 38
  F_KeepAction, // 39
  F_SignalTeam, // 40
  F_DropWeapons, // 41
  F_TargetDoAction, // 42
  F_OpenPassage, // 43
  F_ClosePassage, // 44
  F_IfPassageOpen, // 45
  F_GoPoof, // 46
  F_CostTargetItemID, // 47
  F_DoActionOverride, // 48
  F_IfHealed, // 49
  F_DisplayMessage, // 50
  F_CallForHelp, // 51
  F_AddIDSZ, // 52
  F_End, // 53
  F_SetState, // 54                         // Scripted AI functions (v0.20)
  F_GetState, // 55
  F_IfStateIs, // 56
  F_IfTargetCanOpenStuff, // 57             // Scripted AI functions (v0.30)
  F_IfGrabbed, // 58
  F_IfDropped, // 59
  F_SetTargetToWhoeverIsHolding, // 60
  F_DamageTarget, // 61
  F_IfXIsLessThanY, // 62
  F_SetWeatherTime, // 63                   // Scripted AI functions (v0.40)
  F_GetBumpHeight, // 64
  F_IfReaffirmed, // 65
  F_UnkeepAction, // 66
  F_IfTargetIsOnOtherTeam, // 67
  F_IfTargetIsOnHatedTeam, // 68         // Scripted AI functions (v0.50)
  F_PressLatchButton, // 69
  F_SetTargetToTargetOfLeader, // 70
  F_IfLeaderKilled, // 71
  F_BecomeLeader, // 72
  F_ChangeTargetArmor, // 73               // Scripted AI functions (v0.60)
  F_GiveMoneyToTarget, // 74
  F_DropKeys, // 75
  F_IfLeaderIsAlive, // 76
  F_IfTargetIsOldTarget, // 77
  F_SetTargetToLeader, // 78
  F_SpawnCharacter, // 79
  F_RespawnCharacter, // 80
  F_ChangeTile, // 81
  F_IfUsed, // 82
  F_DropMoney, // 83
  F_SetOldTarget, // 84
  F_DetachFromHolder, // 85
  F_IfTargetHasVulnerabilityID, // 86
  F_CleanUp, // 87
  F_IfCleanedUp, // 88
  F_IfSitting, // 89
  F_IfTargetIsHurt, // 90
  F_IfTargetIsAPlayer, // 91
  F_PlaySound, // 92
  F_SpawnParticle, // 93
  F_IfTargetIsAlive, // 94
  F_Stop, // 95
  F_DisaffirmCharacter, // 96
  F_ReaffirmCharacter, // 97
  F_IfTargetIsSelf, // 98
  F_IfTargetIsMale, // 99
  F_IfTargetIsFemale, // 100
  F_SetTargetToSelf, // 101           // Scripted AI functions (v0.70)
  F_SetTargetToRider, // 102
  F_GetAttackTurn, // 103
  F_GetDamageType, // 104
  F_BecomeSpell, // 105
  F_BecomeSpellbook, // 106
  F_IfScoredAHit, // 107
  F_IfDisaffirmed, // 108
  F_DecodeOrder, // 109
  F_SetTargetToWhoeverWasHit, // 110
  F_SetTargetToWideEnemy, // 111
  F_IfChanged, // 112
  F_IfInWater, // 113
  F_IfBored, // 114
  F_IfTooMuchBaggage, // 115
  F_IfGrogged, // 116
  F_IfDazed, // 117
  F_IfTargetHasSpecialID, // 118
  F_PressTargetLatchButton, // 119
  F_IfInvisible, // 120
  F_IfArmorIs, // 121
  F_GetTargetGrogTime, // 122
  F_GetTargetDazeTime, // 123
  F_SetDamageType, // 124
  F_SetWaterLevel, // 125
  F_EnchantTarget, // 126
  F_EnchantChild, // 127
  F_TeleportTarget, // 128
  F_GiveExperienceToTarget, // 129
  F_IncreaseAmmo, // 130
  F_UnkurseTarget, // 131
  F_GiveExperienceToTargetTeam, // 132
  F_IfUnarmed, // 133
  F_RestockTargetAmmoIDAll, // 134
  F_RestockTargetAmmoIDFirst, // 135
  F_FlashTarget, // 136
  F_SetRedShift, // 137
  F_SetGreenShift, // 138
  F_SetBlueShift, // 139
  F_SetLight, // 140
  F_SetAlpha, // 141
  F_IfHitFromBehind, // 142
  F_IfHitFromFront, // 143
  F_IfHitFromLeft, // 144
  F_IfHitFromRight, // 145
  F_IfTargetIsOnSameTeam, // 146
  F_KillTarget, // 147
  F_UndoEnchant, // 148
  F_GetWaterLevel, // 149
  F_CostTargetMana, // 150
  F_IfTargetHasAnyID, // 151
  F_SetBumpSize, // 152
  F_IfNotDropped, // 153
  F_IfYIsLessThanX, // 154
  F_SetFlyHeight, // 155
  F_IfBlocked, // 156
  F_IfTargetIsDefending, // 157
  F_IfTargetIsAttacking, // 158
  F_IfStateIs0, // 159
  F_IfStateIs1, // 160
  F_IfStateIs2, // 161
  F_IfStateIs3, // 162
  F_IfStateIs4, // 163
  F_IfStateIs5, // 164
  F_IfStateIs6, // 165
  F_IfStateIs7, // 166
  F_IfContentIs, // 167
  F_SetTurnModeToWatchTarget, // 168
  F_IfStateIsNot, // 169
  F_IfXIsEqualToY, // 170
  F_DisplayDebugMessage, // 171
  F_BlackTarget, // 172                       // Scripted AI functions (v0.80)
  F_DisplayMessageNear, // 173
  F_IfHitGround, // 174
  F_IfNameIsKnown, // 175
  F_IfUsageIsKnown, // 176
  F_IfHoldingItemID, // 177
  F_IfHoldingRangedWeapon, // 178
  F_IfHoldingMeleeWeapon, // 179
  F_IfHoldingShield, // 180
  F_IfKursed, // 181
  F_IfTargetIsKursed, // 182
  F_IfTargetIsDressedUp, // 183
  F_IfOverWater, // 184
  F_IfThrown, // 185
  F_MakeNameKnown, // 186
  F_MakeUsageKnown, // 187
  F_StopTargetMovement, // 188
  F_SetXY, // 189
  F_GetXY, // 190
  F_AddXY, // 191
  F_MakeAmmoKnown, // 192
  F_SpawnAttachedParticle, // 193
  F_SpawnExactParticle, // 194
  F_AccelerateTarget, // 195
  F_IfDistanceIsMoreThanTurn, // 196
  F_IfCrushed, // 197
  F_MakeCrushValid, // 198
  F_SetTargetToLowestTarget, // 199
  F_IfNotPutAway, // 200
  F_IfTakenOut, // 201
  F_IfAmmoOut, // 202
  F_PlaySoundLooped, // 203
  F_StopSoundLoop, // 204
  F_HealSelf, // 205
  F_Equip, // 206
  F_IfTargetHasItemIDEquipped, // 207
  F_SetOwnerToTarget, // 208
  F_SetTargetToOwner, // 209
  F_SetFrame, // 210
  F_BreakPassage, // 211
  F_SetReloadTime, // 212
  F_SetTargetToWideBlahID, // 213
  F_PoofTarget, // 214
  F_ChildDoActionOverride, // 215
  F_SpawnPoof, // 216
  F_SetSpeedPercent, // 217
  F_SetChildState, // 218
  F_SpawnAttachedSizedParticle, // 219
  F_ChangeArmor, // 220
  F_ShowTimer, // 221
  F_IfFacingTarget, // 222
  F_PlaySoundVolume, // 223
  F_SpawnAttachedFacedParticle, // 224
  F_IfStateIsOdd, // 225
  F_SetTargetToDistantEnemy, // 226
  F_Teleport, // 227
  F_GiveStrengthToTarget, // 228
  F_GiveWisdomToTarget, // 229
  F_GiveIntelligenceToTarget, // 230
  F_GiveDexterityToTarget, // 231
  F_GiveLifeToTarget, // 232
  F_GiveManaToTarget, // 233
  F_ShowMap, // 234
  F_ShowYouAreHere, // 235
  F_ShowBlipXY, // 236
  F_HealTarget, // 237
  F_PumpTarget, // 238
  F_CostAmmo, // 239
  F_MakeSimilarNamesKnown, // 240
  F_SpawnAttachedHolderParticle, // 241
  F_SetTargetReloadTime, // 242
  F_SetFogLevel, // 243
  F_GetFogLevel, // 244
  F_SetFogTAD, // 245
  F_SetFogBottomLevel, // 246
  F_GetFogBottomLevel, // 247
  F_CorrectActionForHand, // 248
  F_IfTargetIsMounted, // 249
  F_SparkleIcon, // 250
  F_UnsparkleIcon, // 251
  F_GetTileXY, // 252
  F_SetTileXY, // 253
  F_SetShadowSize, // 254
  F_SignalTarget, // 255
  F_SetTargetToWhoeverIsInPassage, // 256
  F_IfCharacterWasABook, // 257
  F_SetEnchantBoostValues, // 258              : Scripted AI functions (v0.90)
  F_SpawnCharacterXYZ, // 259
  F_SpawnExactCharacterXYZ, // 260
  F_ChangeTargetClass, // 261
  F_PlayFullSound, // 262
  F_SpawnExactChaseParticle, // 263
  F_EncodeOrder, // 264
  F_SignalSpecialID, // 265
  F_UnkurseTargetInventory, // 266
  F_IfTargetIsSneaking, // 267
  F_DropItems, // 268
  F_RespawnTarget, // 269
  F_TargetDoActionSetFrame, // 270
  F_IfTargetCanSeeInvisible, // 271
  F_SetTargetToNearestBlahID, // 272
  F_SetTargetToNearestEnemy, // 273
  F_SetTargetToNearestFriend, // 274
  F_SetTargetToNearestLifeform, // 275
  F_FlashPassage, // 276
  F_FindTileInPassage, // 277
  F_IfHeldInLeftSaddle, // 278
  F_NotAnItem, // 279
  F_SetChildAmmo, // 280
  F_IfHitVulnerable, // 281
  F_IfTargetIsFlying, // 282
  F_IdentifyTarget, // 283
  F_BeatModule, // 284
  F_EndModule, // 285
  F_DisableExport, // 286
  F_EnableExport, // 287
  F_GetTargetState, // 288
  F_SetSpeech, // 289
  F_SetMoveSpeech, // 290
  F_SetSecondMoveSpeech, // 291
  F_SetAttackSpeech, // 292
  F_SetAssistSpeech, // 293
  F_SetTerrainSpeech, // 294
  F_SetSelectSpeech, // 295
  F_ClearEndText, // 296
  F_AddEndText, // 297
  F_PlayMusic, // 298
  F_SetMusicPassage, // 299
  F_MakeCrushInvalid, // 300
  F_StopMusic, // 301
  F_FlashVariable, // 302
  F_AccelerateUp, // 303
  F_FlashVariableHeight, // 304
  F_SetDamageTime, // 305
  F_IfStateIs8, // 306
  F_IfStateIs9, // 307
  F_IfStateIs10, // 308
  F_IfStateIs11, // 309
  F_IfStateIs12, // 310
  F_IfStateIs13, // 311
  F_IfStateIs14, // 312
  F_IfStateIs15, // 313
  F_IfTargetIsAMount, // 314
  F_IfTargetIsAPlatform, // 315
  F_AddStat, // 316
  F_DisenchantTarget, // 317
  F_DisenchantAll, // 318
  F_SetVolumeNearestTeammate, // 319
  F_AddShopPassage, // 320
  F_TargetPayForArmor, // 321
  F_JoinEvilTeam, // 322
  F_JoinNullTeam, // 323
  F_JoinGoodTeam, // 324
  F_PitsKill, // 325
  F_SetTargetToPassageID, // 326
  F_MakeNameUnknown, // 327
  F_SpawnExactParticleEndSpawn, // 328
  F_SpawnPoofSpeedSpacingDamage, // 329
  F_GiveExperienceToGoodTeam,           // 330
  F_DoNothing,                          // 331 : Scripted AI functions (v0.95)
  F_DazeTarget,                         // 332
  F_GrogTarget,                         // 333
  F_IfEquipped,                         //
  F_DropTargetMoney,                    //
  F_GetTargetContent,                   //
  F_DropTargetKeys,                     //
  F_JoinTeam,                           //
  F_TargetJoinTeam,                     //
  F_ClearMusicPassage,                  //
  F_AddQuest,                           // Scripted AI functions (v1.00)
  F_BeatQuest,                          //
  F_IfTargetHasQuest,                   //
  F_IfTargetHasNotFullMana,
  F_IfJumping,
  F_IfOperatorIsLinux,
  F_IfTargetIsOwner                     // Scripted AI functions (v1.05)
} OPCODE;

typedef enum script_operation_e
{
  OP_ADD = 0,     // +
  OP_SUB,         // -
  OP_AND,         // &
  OP_SHR,         // >
  OP_SHL,         // <
  OP_MUL,         // *
  OP_DIV,         // /
  OP_MOD          // %
} OPERATION;

typedef enum script_variable_e
{
  VAR_TMP_X = 0,
  VAR_TMP_Y,
  VAR_TMP_DISTANCE,
  VAR_TMP_TURN,
  VAR_TMP_ARGUMENT,
  VAR_RAND,
  VAR_SELF_X,
  VAR_SELF_Y,
  VAR_SELF_TURN,
  VAR_SELF_COUNTER,
  VAR_SELF_ORDER,
  VAR_SELF_MORALE,
  VAR_SELF_LIFE,
  VAR_TARGET_X,
  VAR_TARGET_Y,
  VAR_TARGET_DISTANCE,
  VAR_TARGET_TURN,
  VAR_LEADER_X,
  VAR_LEADER_Y,
  VAR_LEADER_DISTANCE,
  VAR_LEADER_TURN,
  VAR_GOTO_X,
  VAR_GOTO_Y,
  VAR_GOTO_DISTANCE,
  VAR_TARGET_TURNTO,
  VAR_PASSAGE,
  VAR_WEIGHT,
  VAR_SELF_ALTITUDE,
  VAR_SELF_ID,
  VAR_SELF_HATEID,
  VAR_SELF_MANA,
  VAR_TARGET_STR,
  VAR_TARGET_WIS,
  VAR_TARGET_INT,
  VAR_TARGET_DEX,
  VAR_TARGET_LIFE,
  VAR_TARGET_MANA,
  VAR_TARGET_LEVEL,
  VAR_TARGET_SPEEDX,
  VAR_TARGET_SPEEDY,
  VAR_TARGET_SPEEDZ,
  VAR_SELF_SPAWNX,
  VAR_SELF_SPAWNY,
  VAR_SELF_STATE,
  VAR_SELF_STR,
  VAR_SELF_WIS,
  VAR_SELF_INT,
  VAR_SELF_DEX,
  VAR_SELF_MANAFLOW,
  VAR_TARGET_MANAFLOW,
  VAR_SELF_ATTACHED,
  VAR_SWINGTURN,
  VAR_XYDISTANCE,
  VAR_SELF_Z,
  VAR_TARGET_ALTITUDE,
  VAR_TARGET_Z,
  VAR_SELF_INDEX,
  VAR_OWNER_X,
  VAR_OWNER_Y,
  VAR_OWNER_TURN,
  VAR_OWNER_DISTANCE,
  VAR_OWNER_TURNTO,
  VAR_XYTURNTO,
  VAR_SELF_MONEY,
  VAR_SELF_ACCEL,
  VAR_TARGET_EXP,
  VAR_SELF_AMMO,
  VAR_TARGET_AMMO,
  VAR_TARGET_MONEY,
  VAR_TARGET_TURNAWAY,
  VAR_SELF_LEVEL,
  VAR_SPAWN_DISTANCE
} VARIABLE;

typedef struct script_global_values_t
{
  Uint16 oldtarget;
  Sint32 tmpx;
  Sint32 tmpy;
  Uint32 tmpturn;
  Sint32 tmpdistance;
  Sint32 tmpargument;
  Uint32 lastindent;
  Sint32 operationsum;
} SCRIPT_GLOBAL_VALUES;

extern SCRIPT_GLOBAL_VALUES scr_globals;



// For damage/stat pair reads/writes
EXTERN int pairbase, pairrand;
EXTERN float pairfrom, pairto;


//Passages
EXTERN Uint32 numpassage;             // Number of passages in the module
EXTERN int passtlx[MAXPASS];       // Passage positions
EXTERN int passtly[MAXPASS];
EXTERN int passbrx[MAXPASS];
EXTERN int passbry[MAXPASS];
EXTERN int passagemusic[MAXPASS];  //Music track appointed to the specific passage
EXTERN Uint32 passmask[MAXPASS];
EXTERN bool_t passopen[MAXPASS];   // Is the passage open?
EXTERN Uint16 passowner[MAXPASS];  // Who controls the passage?

// For shops
EXTERN Uint16 numshoppassage;
EXTERN Uint16 shoppassage[MAXPASS];  // The passage number
EXTERN Uint16 shopowner[MAXPASS];    // Who gets the gold?
#define NOOWNER 65535


// Status displays
EXTERN int numstat  EQ( 0 );
EXTERN Uint16 statlist[MAXSTAT];
EXTERN int statdelay  EQ( 0 );
EXTERN Uint32 particletrans  EQ( 0x80 );
EXTERN Uint32 antialiastrans_fp8  EQ( 0xC0 );


//Network Stuff
//#define CHARVEL 5.0


#define MAXPLAYER   (1<<3)                          // 2 to a power...  2^3

EXTERN int               numpla;                                 // Number of players
EXTERN bool_t            plavalid[MAXPLAYER];                    // Player used?
EXTERN CHR_REF           plachr[MAXPLAYER];                    // Which character?
EXTERN float             plalatchx[MAXPLAYER];                   // Local latches
EXTERN float             plalatchy[MAXPLAYER];                   //
EXTERN Uint8             plalatchbutton[MAXPLAYER];              //
EXTERN Uint8             pladevice[MAXPLAYER];                   // Input device

CHR_REF pla_get_character( PLA_REF iplayer );

#define VALID_PLA(XX) ( ((XX)>=0) && ((XX)<MAXPLAYER) && plavalid[XX] )

EXTERN int               numlocalpla;                            //
EXTERN int               numfile;                                // For network copy
EXTERN int               numfilesent;                            // For network copy
EXTERN int               numfileexpected;                        // For network copy
EXTERN int               numplayerrespond;                       //



//Sound using SDL_Mixer
EXTERN bool_t         mixeron EQ( bfalse );           //Is the SDL_Mixer loaded?
#define MAXWAVE         16                            // Up to 16 waves per model
#define VOLMIN          -4000                         // Minumum Volume level
#define VOLUMERATIO     7                             // Volume ratio
EXTERN Mix_Chunk  *capwavelist[MAXMODEL][MAXWAVE];    //sounds in a object
EXTERN Mix_Chunk  *globalwave[MAXWAVE];               //All sounds loaded into memory
EXTERN Mix_Chunk  *wave;                             //Used for playing one selected sound file
EXTERN int         channel;                           //Which channel the current sound is using

#define INVALID_SOUND   (-1)
#define INVALID_CHANNEL (-1)

#define FIX_SOUND(XX) ((((XX)<0) || ((XX)>=MAXWAVE)) ? INVALID_SOUND : (XX))

typedef enum global_sound_t
{
  GSOUND_COINGET = 0,              // 0 - Pick up coin
  GSOUND_DEFEND,                   // 1 - Defend clank
  GSOUND_WEATHER,                  // 2 - Weather Effect
  GSOUND_SPLASH,                   // 3 - Hit Water tile (Splash)
  GSOUND_COINFALL,                 // 4 - Coin falls on ground
  GSOUND_COUNT = MAXWAVE
};

//Music using SDL_Mixer
#define MAXPLAYLISTLENGHT 25      //Max number of different tracks loaded into memory
EXTERN bool_t     musicinmemory EQ( bfalse );  //Is the music loaded in memory?
EXTERN Mix_Music *instrumenttosound[MAXPLAYLISTLENGHT]; //This is a specific music file loaded into memory
EXTERN int        songplaying EQ( -1 );  //Current song that is playing



typedef enum input_type_e
{
  INPUT_MOUS = 0,
  INPUT_KEYB,
  INPUT_JOYA,
  INPUT_JOYB,
  INPUT_COUNT
} INPUT_TYPE;

typedef enum input_bits_e
{
  INBITS_NONE  =               0,                         //
  INBITS_MOUS  = 1 << INPUT_MOUS,                         // Input devices
  INBITS_KEYB  = 1 << INPUT_KEYB,                         //
  INBITS_JOYA  = 1 << INPUT_JOYA,                         //
  INBITS_JOYB  = 1 << INPUT_JOYB                          //
} INPUT_BITS;

//Key/Control input defenitions
#define MAXTAG              128                     // Number of tags in scancode.txt
#define TAGSIZE             32                      // Size of each tag
EXTERN int numscantag;
EXTERN char tagname[MAXTAG][TAGSIZE];               // Scancode names
EXTERN Uint32 tagvalue[MAXTAG];                     // Scancode values

typedef enum control_type_e
{
  CONTROL_JUMP         = 0,
  CONTROL_LEFT_USE,
  CONTROL_LEFT_GET,
  CONTROL_LEFT_PACK,
  CONTROL_RIGHT_USE,
  CONTROL_RIGHT_GET,
  CONTROL_RIGHT_PACK,
  CONTROL_CAMERA,
  CONTROL_LAST
} CONTROL;

typedef enum control_list_e
{
  KEY_FIRST = 0,
  KEY_JUMP = KEY_FIRST,
  KEY_LEFT_USE,
  KEY_LEFT_GET,
  KEY_LEFT_PACK,
  KEY_RIGHT_USE,
  KEY_RIGHT_GET,
  KEY_RIGHT_PACK,
  KEY_MESSAGE,
  KEY_CAMERA_LEFT,
  KEY_CAMERA_RIGHT,
  KEY_CAMERA_IN,
  KEY_CAMERA_OUT,
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_LAST,

  MOS_FIRST,
  MOS_JUMP = MOS_FIRST,
  MOS_LEFT_USE,
  MOS_LEFT_GET,
  MOS_LEFT_PACK,
  MOS_RIGHT_USE,
  MOS_RIGHT_GET,
  MOS_RIGHT_PACK,
  MOS_CAMERA,
  MOS_LAST,

  JOA_FIRST,
  JOA_JUMP = JOA_FIRST,
  JOA_LEFT_USE,
  JOA_LEFT_GET,
  JOA_LEFT_PACK,
  JOA_RIGHT_USE,
  JOA_RIGHT_GET,
  JOA_RIGHT_PACK,
  JOA_CAMERA,
  JOA_LAST,

  JOB_FIRST,
  JOB_JUMP = JOB_FIRST,
  JOB_LEFT_USE,
  JOB_LEFT_GET,
  JOB_LEFT_PACK,
  JOB_RIGHT_USE,
  JOB_RIGHT_GET,
  JOB_RIGHT_PACK,
  JOB_CAMERA,
  JOB_LAST
} CONTROL_LIST;

EXTERN Uint32 controlvalue[INPUT_COUNT][KEY_LAST];             // The scancode or mask
EXTERN bool_t controliskey[INPUT_COUNT][KEY_LAST];             // Is it a key?



// SDL specific declarations
EXTERN SDL_Joystick *sdljoya EQ( NULL );
EXTERN SDL_Joystick *sdljoyb EQ( NULL );
EXTERN Uint8 *sdlkeybuffer;
#define SDLKEYDOWN(k) (NULL!=sdlkeybuffer && 0!=sdlkeybuffer[k])



typedef struct configurable_data_t
{
  STRING basicdat_dir;
  STRING gamedat_dir;
  STRING mnu_dir;
  STRING globalparticles_dir;
  STRING modules_dir;
  STRING music_dir;
  STRING objects_dir;
  STRING import_dir;
  STRING players_dir;

  STRING nullicon_bitmap;
  STRING keybicon_bitmap;
  STRING mousicon_bitmap;
  STRING joyaicon_bitmap;
  STRING joybicon_bitmap;

  STRING tile0_bitmap;
  STRING tile1_bitmap;
  STRING tile2_bitmap;
  STRING tile3_bitmap;
  STRING watertop_bitmap;
  STRING waterlow_bitmap;
  STRING phong_bitmap;
  STRING plan_bitmap;
  STRING blip_bitmap;
  STRING font_bitmap;
  STRING icon_bitmap;
  STRING bars_bitmap;
  STRING particle_bitmap;
  STRING title_bitmap;

  STRING menu_main_bitmap;
  STRING menu_advent_bitmap;
  STRING menu_sleepy_bitmap;
  STRING menu_gnome_bitmap;


  STRING slotused_file;
  STRING passage_file;
  STRING aicodes_file;
  STRING actions_file;
  STRING alliance_file;
  STRING fans_file;
  STRING fontdef_file;
  STRING mnu_file;
  STRING money1_file;
  STRING money5_file;
  STRING money25_file;
  STRING money100_file;
  STRING weather4_file;
  STRING weather5_file;
  STRING script_file;
  STRING ripple_file;
  STRING scancode_file;
  STRING playlist_file;
  STRING spawn_file;
  STRING wawalite_file;
  STRING defend_file;
  STRING splash_file;
  STRING mesh_file;
  STRING setup_file;
  STRING log_file;
  STRING controls_file;
  STRING data_file;
  STRING copy_file;
  STRING enchant_file;
  STRING message_file;
  STRING naming_file;
  STRING modules_file;
  STRING skin_file;
  STRING credits_file;
  STRING quest_file;

  int    uifont_points;
  int    uifont_points2;
  STRING uifont_ttf;

  //Global Sounds
  STRING coinget_sound;
  STRING defend_sound;
  STRING coinfall_sound;
  STRING lvlup_sound;

  //------------------------------------
  //Setup Variables
  //------------------------------------
  bool_t zreflect;                   // Reflection z buffering?
  int    maxtotalmeshvertices;       // of vertices
  bool_t fullscreen;                 // Start in CData.fullscreen?
  int    scrd;                       // Screen bit depth
  int    scrx;                       // Screen X size
  int    scry;                       // Screen Y size
  int    scrz;                       // Screen z-buffer depth ( 8 unsupported )
  int    maxmessage;                 //
  bool_t messageon;                  // Messages?
  int    wraptolerance;              // Status bar
  bool_t  staton;                    // Draw the status bars?
  bool_t  render_overlay;            // Draw overlay?
  bool_t  render_background;         // Do we render the water as a background?
  GLenum  perspective;               // Perspective correct textures?
  bool_t  dither;                    // Dithering?
  Uint8   reffadeor;                 // 255 = Don't fade reflections
  GLenum  shading;                   // Gourad CData.shading?
  bool_t  antialiasing;              // Antialiasing?
  bool_t  refon;                     // Reflections?
  bool_t  shaon;                     // Shadows?
  int     texturefilter;             // Texture filtering?
  bool_t  wateron;                   // Water overlays?
  bool_t  shasprite;                 // Shadow sprites?
  bool_t  phongon;                   // Do phong overlay? (Outdated?)
  bool_t  twolayerwateron;           // Two layer water?
  bool_t  overlayvalid;              // Allow large overlay?
  bool_t  backgroundvalid;           // Allow large background?
  bool_t  fogallowed;                // Draw fog? (Not implemented!)
  int     particletype;              // Particle Effects image
  bool_t  vsync;				             // Wait for vertical sync?
  bool_t  gfxacceleration;		       // Force OpenGL graphics acceleration?

  bool_t  soundvalid;        // Allow playing of sound?
  bool_t  musicvalid;        // Allow music and loops?
  int     musicvolume;       // The sound volume of music
  int     soundvolume;       // Volume of sounds played
  int     maxsoundchannel;   // Max number of sounds playing at the same time
  int     buffersize;        // Buffer size set in setup.txt

  Uint8 autoturncamera;           // Type of camera control...

  bool_t  networkon;              // Try to connect?
  int     lag;                    // Lag tolerance
  STRING  nethostname;            // Name for hosting session
  STRING  netmessagename;         // Name for messages
  Uint8 fpson;                    // FPS displayed?

  // Debug options
  SDL_GrabMode GrabMouse;
  bool_t HideMouse;
  bool_t DevMode;
  // Debug options

} CONFIG_DATA;

EXTERN bool_t          usefaredge;                     // Far edge maps? (Outdoor)
EXTERN float		   doturntime;                     // Time for smooth turn

EXTERN STRING      CStringTmp1, CStringTmp2;
EXTERN CONFIG_DATA CData_default, CData;

//Mesh stuff
#include "mesh.h"

extern Uint32  numfanblock;                                    // Number of collision areas
extern Uint16  bumplistchr[MAXMESHFAN/16];                     // For character collisions
extern Uint16  bumplistchrnum[MAXMESHFAN/16];                  // Number on the block
extern Uint16  bumplistprt[MAXMESHFAN/16];                     // For particle collisions
extern Uint16  bumplistprtnum[MAXMESHFAN/16];                  // Number on the block

extern int     numrenderlist;                                  // Number to render, total
extern int     numrenderlist_shine;                            // ..., reflective
extern int     numrenderlist_reflc;                            // ..., has reflection
extern int     numrenderlist_norm;                             // ..., no reflect, no reflection
extern int     numrenderlist_watr;                             // ..., water
extern Uint32  renderlist[MAXMESHRENDER];                      // List of which to render, total
extern Uint32  renderlist_shine[MAXMESHRENDER];                // ..., reflective
extern Uint32  renderlist_reflc[MAXMESHRENDER];                // ..., has reflection
extern Uint32  renderlist_norm[MAXMESHRENDER];                 // ..., no reflect, no reflection
extern Uint32  renderlist_watr[MAXMESHRENDER];                 // ..., water


#endif
