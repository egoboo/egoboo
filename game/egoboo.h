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

/* Egoboo - egoboo.h
 * Disgusting, hairy, way too monolithic header file for the whole darn
 * project.  In severe need of cleaning up.  Venture here with extreme
 * caution, and bring one of those canaries with you to make sure you
 * don't run out of oxygen.
 */

/* Typedefs for various platforms */
#include "egoboo_typedef.h"
#include "egoboo_math.h"  /* vector and matrix math */
#include "egoboo_config.h"

#include "proto.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#include <SDL.h>
#include <SDL_opengl.h>

// The following magic allows this include to work in multiple files
#ifdef DECLARE_GLOBALS
#    define EXTERN
#    define EQ(x) = x;
#else
#    define EXTERN extern
#    define EQ(x)
#endif

#define VERSION "2.7.3"                         // Version of the game

// My lil' random number table
#define MAXRAND 4096
#define RANDIE randie[randindex];  randindex = (randindex+1)&(MAXRAND-1)

EXTERN Uint16  randindex EQ( 0 );
EXTERN Uint16  randie[MAXRAND];

enum e_game_difficulty
{
    GAME_EASY  = 0,
    GAME_NORMAL,
    GAME_HARD
};

EXTERN Uint8     difficulty EQ( GAME_NORMAL );    // What is the current game difficulty
EXTERN bool_t    gamepaused EQ( bfalse );    // Is the game paused?
EXTERN bool_t    pausekeyready EQ( btrue );  // Ready to pause game?
EXTERN bool_t    overrideslots EQ( bfalse );     //Override existing slots?
EXTERN bool_t    screenshotkeyready EQ( btrue );    // Ready to take screenshot?

#define EXPKEEP 0.85f                                // Experience to keep when respawning

#define MAXINVENTORY        7
#define MAXIMPORTPERPLAYER  (MAXINVENTORY + 2)
#define MAXIMPORT           (4*MAXIMPORTPERPLAYER)          // Number of subdirs in IMPORT directory

#define NOSPARKLE           255
#define ANYTIME             0xFF          // Code for respawnvalid...

#define SIZETIME            50                      // Time it takes to resize a character

#define NOSKINOVERRIDE      -1                      // For import

#define DISMOUNTZVEL        16
#define DISMOUNTZVELFLY     4

#define EDGE                128                     // Camera bounds/edge of the map

#define NOHIDE              127                     // Don't hide

// Stats
#define MANARETURNSHIFT     22
#define LOWSTAT             256                     // Worst...
#define PERFECTSTAT         (60*256)                // Maximum stat without magic effects
#define PERFECTBIG          (100*256)               // Perfect life or mana...
#define HIGHSTAT            (100*256)                // Absolute max adding enchantments as well

EXTERN int wraptolerance  EQ( 80 );        // Status bar

#define DAMFX_NONE           0                       // Damage effects
#define DAMFX_ARMO           1                       // Armor piercing
#define DAMFX_BLOC           2                       // Cannot be blocked by shield
#define DAMFX_ARRO           4                       // Only hurts the one it's attached to
#define DAMFX_TURN           8                       // Turn to attached direction
#define DAMFX_TIME           16

#define HURTDAMAGE           256                     // Minimum damage for hurt animation

#define ULTRABLUDY         2                       // This makes any damage draw blud

#define SPELLBOOK           127                     // The spellbook model

// Geneder stuff
#define GENFEMALE           0                       // Gender
#define GENMALE             1
#define GENOTHER            2
#define GENRANDOM           3

#define MAXSTAT             16                      // Maximum status displays

// Messaging stuff
#define MAXMESSAGE          6                       // Number of messages
#define MAXTOTALMESSAGE     4096
#define MESSAGESIZE         80
#define MESSAGEBUFFERSIZE   (MAXTOTALMESSAGE*40)
EXTERN Uint16 messagetime   EQ(200);                     // Time to keep the message alive
#define TABAND              31                      // Tab size

// Model tags
#define MADFXINVICTUS       1                       // I Invincible
#define MADFXACTLEFT        2                       // AL Activate left item
#define MADFXACTRIGHT       4                       // AR Activate right item
#define MADFXGRABLEFT       8                       // GL GO Grab left/Grab only item
#define MADFXGRABRIGHT      16                      // GR Grab right item
#define MADFXDROPLEFT       32                      // DL DO Drop left/Drop only item
#define MADFXDROPRIGHT      64                      // DR Drop right item
#define MADFXSTOP           128                     // S Stop movement
#define MADFXFOOTFALL       256                     // F Footfall sound
#define MADFXCHARLEFT       512                     // CL Grab left/Grab only character
#define MADFXCHARRIGHT      1024                    // CR Grab right character
#define MADFXPOOF           2048                    // P Poof

// Animation walking
#define LIPDA               0                       // For smooth transitions 'tween
#define LIPWA               1                       //   walking rates
#define LIPWB               2
#define LIPWC               3

#define NOACTION            0xffff                     // Action not valid for this character
#define MAXACTION           76                         // Number of action types
EXTERN char                 cActionName[MAXACTION][2]; // Two letter name code

#define GRABSIZE            90.0f                      // Grab tolerance
#define SEEINVISIBLE        128                        // Cutoff for invisible characters


enum e_damage_type
{
    DAMAGE_SLASH = 0,
    DAMAGE_CRUSH,
    DAMAGE_POKE,
    DAMAGE_HOLY,                             // (Most invert Holy damage )
    DAMAGE_EVIL,
    DAMAGE_FIRE,
    DAMAGE_ICE,
    DAMAGE_ZAP,
    DAMAGE_COUNT                             // Damage types
};
#define DAMAGENULL          255

#define DAMAGEMANA          16                      // 000x0000 Deals damage to mana
#define DAMAGECHARGE        8                       // 0000x000 Converts damage to mana
#define DAMAGEINVERT        4                       // 00000x00 Makes damage heal
#define DAMAGESHIFT         3                       // 000000xx Resistance ( 1 is common )

#define DAMAGETILETIME      32                      // Invincibility time
#define DAMAGETIME          16                      // Invincibility time
#define DEFENDTIME          16                      // Invincibility time
#define DROPXYVEL           8
#define DROPZVEL            7
#define JUMPATTACKVEL       -2
#define WATERJUMP           12

#define TURNMODEVELOCITY    0                       // Character gets rotation from velocity
#define TURNMODEWATCH       1                       // For watch towers
#define TURNMODESPIN        2                       // For spinning objects
#define TURNMODEWATCHTARGET 3                       // For combat intensive AI

#define MAXLEVEL            6                       // Basic Levels 0-5

enum e_idsz_type
{
    IDSZ_PARENT = 0,                             // Parent index
    IDSZ_TYPE,                                   // Self index
    IDSZ_SKILL,                                  // Skill index
    IDSZ_SPECIAL,                                // Special index
    IDSZ_HATE,                                   // Hate index
    IDSZ_VULNERABILITY,                          // Vulnerability index
    IDSZ_COUNT                                   // ID strings per character
};

// XP stuff
enum e_xp_type
{
    XP_FINDSECRET = 0,                          // Finding a secret
    XP_WINQUEST,                                // Beating a module or a subquest
    XP_USEDUNKOWN,                              // Used an unknown item
    XP_KILLENEMY,                               // Killed an enemy
    XP_KILLSLEEPY,                              // Killed a sleeping enemy
    XP_KILLHATED,                               // Killed a hated enemy
    XP_TEAMKILL,                                // Team has killed an enemy
    XP_TALKGOOD,                                // Talk good, er...  I mean well
    XP_COUNT                                    // Number of ways to get experience
};
#define XPDIRECT            255                     // No modification

#define LATCHBUTTON_LEFT      ( 1 << 0 )                      // Character button presses
#define LATCHBUTTON_RIGHT     ( 1 << 1 )
#define LATCHBUTTON_JUMP      ( 1 << 2 )
#define LATCHBUTTON_ALTLEFT   ( 1 << 3 )                      // ( Alts are for grab/drop )
#define LATCHBUTTON_ALTRIGHT  ( 1 << 4 )
#define LATCHBUTTON_PACKLEFT  ( 1 << 5 )                     // ( Packs are for inventory cycle )
#define LATCHBUTTON_PACKRIGHT ( 1 << 6 )
#define LATCHBUTTON_RESPAWN   ( 1 << 7 )

// Z velocity stuff
#define JUMPDELAY           20                      // Time between jumps

#define MAPID 0x4470614d                        // The string... MapD

#define RAISE 12 //25                               // Helps correct z level
#define SHADOWRAISE 5
#define DAMAGERAISE 25

#define MAXWATERLAYER 2                             // Maximum water layers
#define MAXWATERFRAME 512                           // Maximum number of wave frames
#define WATERFRAMEAND (MAXWATERFRAME-1)
#define WATERPOINTS 4                               // Points in a water fan
#define WATERMODE 4                                 // Ummm...  For making it work, yeah...

#define TURNSPD                         0.01f         // Cutoff for turning or same direction

#define MD2START                        0x32504449  // MD2 files start with these four bytes
#define MD2MAXLOADSIZE                  (512*1024)  // Don't load any models bigger than 512k
#define MD2LIGHTINDICES                 163//162    // MD2's store vertices as x,y,z,normal
#define EQUALLIGHTINDEX                 162         // I added an extra index to do the
// spikey mace...

#define MAXTEXTURE                      768         // Max number of textures
#define MAXMODEL                        256         // Max number of models
#define MAXCHR                          512         // Max number of characters

//Object positions
#define MAXSLOT                          2
#define SLOT_LEFT                        0
#define SLOT_RIGHT                       1

#define GRIP_VERTS                       4
#define GRIP_RIGHT                   (2 * GRIP_VERTS)  // Right weapon grip starts 8 from last
#define GRIP_LEFT                    (1 * GRIP_VERTS)  // Left weapon grip starts 4 from last
#define GRIP_ONLY                        GRIP_LEFT     // Only weapon grip starts 4 from last
#define GRIP_LAST                        1             // Spawn particles at the last vertex
#define GRIP_ORIGIN                      0             // Spawn attachments at the center
#define GRIP_INVENTORY                   0

#define CHOPPERMODEL                    32

#define MAXMESHFAN                      (512*512)   // Terrain mesh size
#define MAXMESHTILEY                    1024        // Max tiles in y direction
#define MAXMESHBLOCKY                   (( MAXMESHTILEY >> 2 )+1)  // max blocks in the y direction
#define BYTESFOREACHVERTEX              14            // 14 bytes each
#define MAXMESHVERTICES                 16            // Fansquare vertices
#define MAXMESHTYPE                     64            // Number of fansquare command types
#define MAXMESHCOMMAND                  4             // Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES           32            // Fansquare command list size
#define MAXMESHCOMMANDSIZE              32            // Max trigs in each command
#define MAXTILETYPE                     256           // Max number of tile images
#define MAXMESHRENDER                   1024          // Max number of tiles to draw
#define FANOFF                          0xffff        // Don't draw the fansquare if tile = this

#define MESHFX_REF                       0           // 0 This tile is drawn 1st
#define MESHFX_SHA                       1           // 0 This tile is drawn 2nd
#define MESHFX_DRAWREF                   2           // 1 Draw reflection of characters
#define MESHFX_ANIM                      4           // 2 Animated tile ( 4 frame )
#define MESHFX_WATER                     8           // 3 Render water above surface ( Water details are set per module )
#define MESHFX_WALL                      16          // 4 Wall ( Passable by ghosts, particles )
#define MESHFX_IMPASS                    32          // 5 Impassable
#define MESHFX_DAMAGE                    64          // 6 Damage
#define MESHFX_SLIPPY                    128         // 7 Ice or normal

// Physics
#define SLOPE                           800         // Slope increments for terrain normals
#define SLIDE                           0.04f         // Acceleration for steep hills
#define SLIDEFIX                        0.08f         // To make almost flat surfaces flat

#define STOPBOUNCING                    0.1f //1.0f         // To make objects stop bouncing
#define STOPBOUNCINGPART                5.0f         // To make particles stop bouncing

/* SDL_GetTicks() always returns milli seconds */
#define TICKS_PER_SEC                   1000
EXTERN Sint32 framelimit                EQ(30);
#define UPDATES_PER_SEC                 50
#define UPDATE_SKIP                     ((float)TICKS_PER_SEC/(float)UPDATES_PER_SEC)
#define ONESECOND                       TICKS_PER_SEC

#define TRANSCOLOR                      0           // Transparent color

// Debug option
EXTERN bool_t  gGrabMouse EQ( btrue );
EXTERN bool_t  gHideMouse EQ( bfalse );
EXTERN bool_t  gDevMode EQ( bfalse );
// Debug option

EXTERN int     animtileupdateand  EQ( 7 );                        // New tile every 7 frames
EXTERN Uint16  animtileframeand  EQ( 3 );              // Only 4 frames
EXTERN Uint16  animtilebaseand  EQ( 0xfffc );
EXTERN Uint16  biganimtileframeand  EQ( 7 );           // For big tiles
EXTERN Uint16  biganimtilebaseand  EQ( 0xfff8 );
EXTERN Uint16  animtileframeadd  EQ( 0 );              // Current frame

#define NORTH 16384                                 // Character facings
#define SOUTH 49152
#define EAST 32768
#define WEST 0
#define RANDOM rand() % 65535
#define FRONT 0                                     // Attack directions
#define BEHIND 32768
#define LEFT 49152
#define RIGHT 16384

#define MAXXP 200000                               // Maximum experience
#define MAXMONEY 9999                               // Maximum money

//------------------------------------
// Character defines
//------------------------------------
#define MAXSKIN   4

EXTERN Sint16                  damagetileparttype;
EXTERN short                   damagetilepartand;
EXTERN short                   damagetilesound;
EXTERN short                   damagetilesoundtime;
EXTERN Uint16                  damagetilemindistance;

#define TILESOUNDTIME 16
#define TILEREAFFIRMAND  3

// Display
EXTERN Uint8                   timeron  EQ( bfalse );          // Game timer displayed?
EXTERN Uint32                  timervalue  EQ( 0 );           // Timer time ( 50ths of a second )
EXTERN char                    szfpstext[]  EQ( "000 FPS" );
EXTERN Uint8                   fpson  EQ( btrue );             // FPS displayed?

// Timers
EXTERN Sint32          clock_stt;                   // GetTickCount at start
EXTERN Sint32          clock_all  EQ( 0 );             // The total number of ticks so far
EXTERN Sint32          clock_lst  EQ( 0 );             // The last total of ticks so far
EXTERN Sint32          clock_wld  EQ( 0 );             // The sync clock
EXTERN Sint32          clock_fps  EQ( 0 );             // The number of ticks this second
EXTERN Uint32          frame_wld  EQ( 0 );             // The number of frames that should have been drawn
EXTERN Uint32          frame_all  EQ( 0 );             // The total number of frames drawn so far
EXTERN Uint32          frame_fps  EQ( 0 );             // The number of frames drawn this second
EXTERN Uint32          clock_stat  EQ( 0 );            // For stat regeneration
EXTERN Uint32          pitclock  EQ( 0 );             // For pit kills
EXTERN Uint32          outofsync  EQ( 0 );
EXTERN Uint8           parseerror  EQ( bfalse );

//Pitty stuff
EXTERN bool_t          pitskill  EQ( bfalse );          // Do they kill?
EXTERN bool_t          pitsfall  EQ( bfalse );          // Do they teleport?
EXTERN Uint32          pitx;
EXTERN Uint32          pity;
EXTERN Uint32          pitz;

EXTERN bool_t                    fullscreen EQ( bfalse );        // Start in fullscreen?
EXTERN bool_t                    clearson  EQ( btrue );             // Do we clear every time?
EXTERN bool_t                    gameactive  EQ( bfalse );       // Stay in game or quit to windows?
EXTERN bool_t                    moduleactive  EQ( bfalse );     // Is the control loop still going?
EXTERN bool_t                    gamemenuactive EQ( bfalse );
EXTERN bool_t                    soundon  EQ( btrue );              // Is the sound alive?
EXTERN bool_t                    staton  EQ( btrue );               // Draw the status bars?
EXTERN bool_t                    phongon  EQ( btrue );              // Do phong overlay?
EXTERN bool_t                    networkon  EQ( btrue );            // Try to connect?
EXTERN bool_t                    serviceon  EQ( bfalse );        // Do I need to free the interface?
EXTERN bool_t                    twolayerwateron  EQ( btrue );      // Two layer water?
EXTERN bool_t                    menuactive  EQ( bfalse );       // Menu running?
EXTERN bool_t                    hostactive  EQ( bfalse );       // Hosting?
EXTERN bool_t                    readytostart;               // Ready to hit the Start Game button?
EXTERN bool_t                    waitingforplayers;          // Has everyone talked to the host?
EXTERN bool_t                    respawnvalid;               // Can players respawn with Spacebar?
EXTERN bool_t                    respawnanytime;             // True if it's a small level...
EXTERN bool_t                    importvalid;                // Can it import?
EXTERN bool_t                    exportvalid;                // Can it export?
EXTERN bool_t                    rtscontrol;                 // Play as a real-time stragedy? BAD REMOVE
EXTERN bool_t                    backgroundvalid;            // Allow large background?
EXTERN bool_t                    overlayvalid;               // Allow large overlay?
EXTERN bool_t                    local_noplayers;             // Are there any local players?
EXTERN bool_t                    usefaredge;                 // Far edge maps? (Outdoor)
EXTERN bool_t                    beatmodule;                 // Show Module Ended text?
EXTERN Uint8                     autoturncamera;             // Type of camera control...
EXTERN Uint8                     importamount;               // Number of imports for this module
EXTERN Uint8                     playeramount;
EXTERN Uint32                    seed  EQ( 0 );              // The module seed
EXTERN char                      pickedmodule[64];           // The module load name
EXTERN int                       pickedindex;                // The module index number
EXTERN int                       playersready;               // Number of players ready to start
EXTERN int                       playersloaded;

//Respawning
EXTERN bool_t                   local_allpladead;            // Has everyone died?
EXTERN Uint16                   revivetimer EQ(0);

// Imports
EXTERN int                     numimport;                     // Number of imports from this machine
EXTERN Uint32                  local_control[16];             // Input bits for each imported player
EXTERN short                   local_slot[16];                // For local imports

// Setup values
EXTERN int                     maxmessage  EQ( MAXMESSAGE );
EXTERN int                     scrd  EQ( 32 );                 // Screen bit depth
EXTERN int                     scrz  EQ( 16 );                // Screen z-buffer depth ( 8 unsupported )
EXTERN int                     scrx  EQ( 320 );               // Screen X size
EXTERN int                     scry  EQ( 200 );               // Screen Y size
EXTERN Uint8                   reffadeor  EQ( 0 );            // 255 = Don't fade reflections
EXTERN Uint8                   messageon  EQ( btrue );         // Messages?
EXTERN bool_t                  overlayon  EQ( bfalse );        // Draw overlay?
EXTERN bool_t                  perspective  EQ( bfalse );      // Perspective correct textures?
EXTERN bool_t                  dither  EQ( bfalse );           // Dithering?
EXTERN GLuint                  shading  EQ( GL_SMOOTH );           // Gourad shading?
EXTERN Uint8                   antialiasing  EQ( bfalse );     // Antialiasing?
EXTERN bool_t                  refon  EQ( bfalse );            // Reflections?
EXTERN bool_t                  shaon  EQ( bfalse );            // Shadows?
EXTERN Uint8                   texturefilter  EQ( 1 );     // Texture filtering?
EXTERN bool_t                  wateron  EQ( btrue );           // Water overlays?
EXTERN bool_t                  shasprite  EQ( bfalse );        // Shadow sprites?
EXTERN bool_t                  prtreflect  EQ( bfalse );         // Reflect particles?
EXTERN  bool_t                 use_sdl_image EQ(btrue);    // Allow advanced SDL_Image functions?

// EWWWW. GLOBALS ARE EVIL.

// KEYBOARD
EXTERN bool_t console_mode EQ( bfalse );                   // Input text from keyboard?
EXTERN bool_t console_done EQ( bfalse );                   // Input text from keyboard finished?

// Weather and water gfx
EXTERN int                     weatherplayer;
EXTERN int                     weatheroverwater EQ( bfalse );       // Only spawn over water?
EXTERN int                     weathertimereset EQ( 10 );          // Rate at which weather particles spawn
EXTERN int                     weathertime EQ( 0 );                // 0 is no weather

EXTERN int                     numwaterlayer EQ( 0 );              // Number of layers
EXTERN float                   watersurfacelevel EQ( 0 );          // Surface level for water striders
EXTERN float                   waterdouselevel EQ( 0 );            // Surface level for torches
EXTERN Uint8                   waterlight EQ( 0 );                 // Is it light ( default is alpha )
EXTERN Uint8                   waterspekstart EQ( 128 );           // Specular begins at which light value
EXTERN Uint8                   waterspeklevel EQ( 128 );           // General specular amount (0-255)
EXTERN Uint8                   wateriswater  EQ( btrue );          // Is it water?  ( Or lava... )
EXTERN Uint8                   waterlightlevel[MAXWATERLAYER]; // General light amount (0-63)
EXTERN Uint8                   waterlightadd[MAXWATERLAYER];   // Ambient light amount (0-63)
EXTERN float                   waterlayerz[MAXWATERLAYER];     // Base height of water
EXTERN Uint8                   waterlayeralpha[MAXWATERLAYER]; // Transparency
EXTERN float                   waterlayeramp[MAXWATERLAYER];   // Amplitude of waves
EXTERN float                   waterlayeru[MAXWATERLAYER];     // Coordinates of texture
EXTERN float                   waterlayerv[MAXWATERLAYER];
EXTERN float                   waterlayeruadd[MAXWATERLAYER];  // Texture movement
EXTERN float                   waterlayervadd[MAXWATERLAYER];
EXTERN float                   waterlayerzadd[MAXWATERLAYER][MAXWATERFRAME][WATERMODE][WATERPOINTS];
EXTERN Uint8                   waterlayercolor[MAXWATERLAYER][MAXWATERFRAME][WATERMODE][WATERPOINTS];
EXTERN Uint16                  waterlayerframe[MAXWATERLAYER]; // Frame
EXTERN Uint16                  waterlayerframeadd[MAXWATERLAYER];      // Speed
EXTERN float                   waterlayerdistx[MAXWATERLAYER];         // For distant backgrounds
EXTERN float                   waterlayerdisty[MAXWATERLAYER];
EXTERN Uint32                  waterspek[256];             // Specular highlights
EXTERN float                   foregroundrepeat  EQ( 1 );
EXTERN float                   backgroundrepeat  EQ( 1 );

// Fog stuff
EXTERN bool_t                  fogallowed  EQ( btrue );
EXTERN bool_t                  fogon  EQ( bfalse );            // Do ground fog?
EXTERN float                   fogbottom  EQ( 0.0f );
EXTERN float                   fogtop  EQ( 100 );
EXTERN float                   fogdistance  EQ( 100 );
EXTERN Uint8                   fogred  EQ( 255 );             //  Fog collour
EXTERN Uint8                   foggrn  EQ( 255 );
EXTERN Uint8                   fogblu  EQ( 255 );
EXTERN Uint8                   fogaffectswater;

//Texture filtering
typedef enum e_tx_filters
{
    TX_UNFILTERED,
    TX_LINEAR,
    TX_MIPMAP,
    TX_BILINEAR,
    TX_TRILINEAR_1,
    TX_TRILINEAR_2,
    TX_ANISOTROPIC
} TX_FILTERS;

// Input player control
EXTERN int                     nullicon  EQ( 0 );
EXTERN int                     keybicon  EQ( 0 );
EXTERN int                     mousicon  EQ( 0 );
EXTERN int                     joyaicon  EQ( 0 );
EXTERN int                     joybicon  EQ( 0 );

EXTERN Uint8                   cLoadBuffer[MD2MAXLOADSIZE];// Where to put an MD2

#define INVISIBLE           20                      // The character can't be detected

EXTERN bool_t                    local_seeinvisible   EQ( bfalse );
EXTERN bool_t                    local_seekurse       EQ( bfalse );
EXTERN Uint16                    local_senseenemies   EQ( MAXCHR );
EXTERN IDSZ                      local_senseenemiesID;
EXTERN bool_t                    local_listening      EQ( bfalse );  // Players with listen skill?

//------------------------------------
// Model stuff
//------------------------------------
#define MAXPRTPIPPEROBJECT       13                                      // Max part*.txt per object
#define MAXCOMMAND                      512         // Max number of commands
#define MAXCOMMANDSIZE                  128          // Max number of points in a command
#define MAXCOMMANDENTRIES               512         // Max entries in a command list ( trigs )

EXTERN int             globalicon_count;                              // Number of icons

struct s_mad
{
    bool_t  used;                          // Model slot
    STRING  name;                          // Model name

    // templates
    Uint16  ai;                            // AI for each model
    Uint16  prtpip[MAXPRTPIPPEROBJECT];    // Local particles

    Uint16  skins;                         // Number of skins
    Uint16  skinstart;                     // Starting skin of model

    Uint16  frames;                        // Number of frames
    Uint16  framestart;                    // Starting frame of model

    Uint16  msgstart;                      // The first message

    Uint16  vertices;                      // Number of vertices
    Uint16  transvertices;                 // Number to transform

    Uint16  commands;                      // Number of commands
    GLenum  commandtype[MAXCOMMAND];       // Fan or strip
    Uint16  commandsize[MAXCOMMAND];       // Entries used by command
    Uint16  commandvrt[MAXCOMMANDENTRIES]; // Which vertex
    float   commandu[MAXCOMMANDENTRIES];   // Texture position
    float   commandv[MAXCOMMANDENTRIES];

    Uint16  frameliptowalkframe[4][16];    // For walk animations

    Uint8   actionvalid[MAXACTION];        // bfalse if not valid
    Uint16  actionstart[MAXACTION];        // First frame of anim
    Uint16  actionend[MAXACTION];          // One past last frame
};
typedef struct s_mad mad_t;

EXTERN mad_t   MadList[MAXMODEL];

EXTERN Uint16  skintoicon[MAXTEXTURE];                  // Skin to icon

EXTERN Uint16  bookicon_count    EQ(0);
EXTERN Uint16  bookicon[MAXSKIN];                      // The first book icon

EXTERN const char *globalparsename  EQ( NULL ); // The SCRIPT.TXT filename

// phisics info
EXTERN float           hillslide  EQ( 1.00f );
EXTERN float           slippyfriction  EQ( 1.00f );                            // Friction
EXTERN float           airfriction  EQ( 0.91f );                               // 0.9868 is approximately real world air friction
EXTERN float           waterfriction  EQ( 0.80f );
EXTERN float           noslipfriction  EQ( 0.91f );
EXTERN float           platstick  EQ( 0.040f );
EXTERN float           gravity  EQ( -1.0f );                                   // Gravitational accel

EXTERN int             damagetileamount  EQ( 256 );                           // Amount of damage
EXTERN Uint8           damagetiletype  EQ( DAMAGE_FIRE );                      // Type of damage

EXTERN char            cFrameName[16];                                     // MD2 Frame Name

//BAD!!
EXTERN Uint16          glouseangle;
//BAD!!

// Bump List
struct s_bumplist
{
    Uint16  chr;                     // For character collisions
    Uint16  chrnum;                  // Number on the block
    Uint16  prt;                     // For particle collisions
    Uint16  prtnum;                  // Number on the block
};
typedef struct s_bumplist bumplist_t;

EXTERN bumplist_t bumplist[MAXMESHFAN/16];

//Mesh
EXTERN Uint32 maplrtwist[256];            // For surface normal of mesh
EXTERN Uint32 mapudtwist[256];
EXTERN float  vellrtwist[256];            // For sliding down steep hills
EXTERN float  veludtwist[256];
EXTERN Uint8  flattwist[256];

#define MESH_MAXTOTALVERTRICES 1024*100

struct s_mesh_info
{
    Uint8           exploremode;                      // Explore mode?

    size_t          vertcount;                         // For malloc

    int             tiles_x;                          // Size in tiles
    int             tiles_y;
    Uint32          tiles_count;                      // Number of tiles

    int             blocks_x;                         // Size in blocks
    int             blocks_y;
    Uint32          blocks_count;                     // Number of blocks (collision areas)

    float           edge_x;                           // Limits
    float           edge_y;
};
typedef struct s_mesh_info mesh_info_t;

struct s_tile_info
{
    Uint8   type;                              // Tile type
    Uint16  img;                               // Get texture from this
    Uint8   fx;                                 // Special effects flags
    Uint8   twist;
    Uint32  vrtstart;                           // Which vertex to start at

    bool_t  inrenderlist;
};
typedef struct s_tile_info tile_info_t;

struct s_mesh_mem
{
    size_t          vertcount;                                      // For malloc

    tile_info_t *   tile_list;                               // Command type

    Uint32*         blockstart;
    Uint32*         tilestart;                         // Which fan to start a row with

    float*          vrt_x;                                 // Vertex position
    float*          vrt_y;
    float*          vrt_z;                                 // Vertex elevation
    Uint8*          vrt_a;                                 // Vertex base light
    Uint8*          vrt_l;                                 // Vertex light
};
typedef struct s_mesh_mem mesh_mem_t;

struct s_mesh
{
    mesh_info_t info;
    mesh_mem_t  mem;

    float       tileoff_u[MAXTILETYPE];                          // Tile texture offset
    float       tileoff_v[MAXTILETYPE];
};
typedef struct s_mesh mesh_t;

EXTERN mesh_t mesh;

mesh_t * mesh_new( mesh_t * pmesh );
mesh_t * mesh_renew( mesh_t * pmesh );
mesh_t * mesh_delete( mesh_t * pmesh );
mesh_t * mesh_create( mesh_t * pmesh, int tiles_x, int tiles_y );

struct s_tile_definition
{
    Uint8           numvertices;                // Number of vertices
    float           u[MAXMESHVERTICES];         // Vertex texture posi
    float           v[MAXMESHVERTICES];

    Uint8           command_count;                        // Number of commands
    Uint8           command_entries[MAXMESHCOMMAND];      // Entries in each command
    Uint16          command_verts[MAXMESHCOMMANDENTRIES]; // Fansquare vertex list
};
typedef struct s_tile_definition tile_definition_t;

tile_definition_t tile_dict[MAXMESHTYPE];

#define INVALID_BLOCK ((Uint32)(~0))
#define INVALID_TILE  ((Uint32)(~0))

EXTERN Uint8 asciitofont[256];                                   // Conversion table

// Display messages
EXTERN Uint16          msgtimechange EQ( 0 );
EXTERN Uint16          msgstart EQ( 0 );                                       // The message queue
EXTERN Sint16          msgtime[MAXMESSAGE];
EXTERN char            msgtextdisplay[MAXMESSAGE][MESSAGESIZE];            // The displayed text

// Message files
EXTERN Uint16          msgtotal EQ( 0 );                                       // The number of messages
EXTERN Uint32          msgtotalindex EQ( 0 );                                  // Where to put letter
EXTERN Uint32          msgindex[MAXTOTALMESSAGE];                          // Where it is
EXTERN char            msgtext[MESSAGEBUFFERSIZE];                         // The text buffer

//End text
#define MAXENDTEXT 1024
EXTERN char   endtext[MAXENDTEXT];     // The end-module text
EXTERN int    endtextwrite;

// This stuff is for actions
#define ACTIONDA            0
#define ACTIONDB            1
#define ACTIONDC            2
#define ACTIONDD            3
#define ACTIONUA            4
#define ACTIONUB            5
#define ACTIONUC            6
#define ACTIONUD            7
#define ACTIONTA            8
#define ACTIONTB            9
#define ACTIONTC            10
#define ACTIONTD            11
#define ACTIONCA            12
#define ACTIONCB            13
#define ACTIONCC            14
#define ACTIONCD            15
#define ACTIONSA            16
#define ACTIONSB            17
#define ACTIONSC            18
#define ACTIONSD            19
#define ACTIONBA            20
#define ACTIONBB            21
#define ACTIONBC            22
#define ACTIONBD            23
#define ACTIONLA            24
#define ACTIONLB            25
#define ACTIONLC            26
#define ACTIONLD            27
#define ACTIONXA            28
#define ACTIONXB            29
#define ACTIONXC            30
#define ACTIONXD            31
#define ACTIONFA            32
#define ACTIONFB            33
#define ACTIONFC            34
#define ACTIONFD            35
#define ACTIONPA            36
#define ACTIONPB            37
#define ACTIONPC            38
#define ACTIONPD            39
#define ACTIONEA            40
#define ACTIONEB            41
#define ACTIONRA            42
#define ACTIONZA            43
#define ACTIONZB            44
#define ACTIONZC            45
#define ACTIONZD            46
#define ACTIONWA            47
#define ACTIONWB            48
#define ACTIONWC            49
#define ACTIONWD            50
#define ACTIONJA            51
#define ACTIONJB            52
#define ACTIONJC            53
#define ACTIONHA            54
#define ACTIONHB            55
#define ACTIONHC            56
#define ACTIONHD            57
#define ACTIONKA            58
#define ACTIONKB            59
#define ACTIONKC            60
#define ACTIONKD            61
#define ACTIONMA            62
#define ACTIONMB            63
#define ACTIONMC            64
#define ACTIONMD            65
#define ACTIONME            66
#define ACTIONMF            67
#define ACTIONMG            68
#define ACTIONMH            69
#define ACTIONMI            70
#define ACTIONMJ            71
#define ACTIONMK            72
#define ACTIONML            73
#define ACTIONMM            74
#define ACTIONMN            75

#define CLOSETOLERANCE      2                       // For closing doors

// Status displays
EXTERN int    numstat  EQ( 0 );
EXTERN Uint16 statlist[MAXSTAT];
EXTERN int    statdelay  EQ( 25 );

enum e_order
{
    ORDER_NONE  = 0,
    ORDER_ATTACK,
    ORDER_ASSIST,
    ORDER_STAND,
    ORDER_TERRAIN,
    ORDER_COUNT
};

//Quest system
#define QUEST_BEATEN         -1
#define QUEST_NONE             -2

#define  _EGOBOO_H_
