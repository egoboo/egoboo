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
#include "egobootypedef.h"

#include "proto.h"

#include "gltexture.h"  /* OpenGL texture loader */
#include "egoboo_math.h"  /* vector and matrix math */
#include "configfile.h"
#include "Md2.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#include <SDL_opengl.h>

//OPENGL VERTEX
typedef struct
{
    GLfloat x, y, z, w;
    GLfloat r, g, b, a;
    Uint32   color; // should replace r,g,b,a and be called by glColor4ubv
    GLfloat s, t; // u and v in D3D I guess
} GLVERTEX;

// The following magic allows this include to work in multiple files
#ifdef DECLARE_GLOBALS
#define EXTERN
#define EQ(x) =x;
#else
#define EXTERN extern
#define EQ(x)
#endif

EXTERN char          VERSION[] EQ( "2.7.0b" );  // Version of the game
EXTERN bool_t        gamepaused EQ( bfalse );  // Is the game paused?
EXTERN bool_t        pausekeyready EQ( btrue );  // Is the game paused?
EXTERN bool_t    overrideslots EQ( bfalse ); //Override existing slots?
EXTERN bool_t    screenshotkeyready EQ( btrue );    // Ready to take screenshot?
#define EXPKEEP 0.85f                                // Experience to keep when respawning

#define MAXMODULE           100                     // Number of modules
#define TITLESIZE           128                     // Size of a title image
#define MAXIMPORT           (36*8)                  // Number of subdirs in IMPORT directory

#define NOSPARKLE           255
#define ANYTIME             0xFF          // Code for respawnvalid...

#define SIZETIME            50                      // Time it takes to resize a character

#define NOSKINOVERRIDE      -1                      // For import

EXTERN Uint16    endtextindex[8];

#define DISMOUNTZVEL        16
#define DISMOUNTZVELFLY     4

#define EDGE                128                     // Camera bounds/edge of the map

#define NOHIDE              127                     // Don't hide

// Stats
#define MANARETURNSHIFT     22                       //
#define LOWSTAT             256                     // Worst...
#define PERFECTSTAT         (75*256)                // Perfect...
#define PERFECTBIG          (100*256)               // Perfect life or mana...
#define HIGHSTAT            (100*256)                // Absolute max strength...

EXTERN int wraptolerance  EQ( 80 );        // Status bar

#define DAMFXNONE           0                       // Damage effects
#define DAMFXARMO           1                       // Armor piercing
#define DAMFXBLOC           2                       // Cannot be blocked by shield
#define DAMFXARRO           4                       // Only hurts the one it's attached to
#define DAMFXTURN           8                       // Turn to attached direction
#define DAMFXTIME           16                      //

#define HURTDAMAGE           256                     // Minimum damage for hurt animation

#define ULTRABLOODY         2                       // This makes any damage draw blood

#define SPELLBOOK           127                     // The spellbook model

// Geneder stuff
#define GENFEMALE           0                       // Gender
#define GENMALE             1                       //
#define GENOTHER            2                       //
#define GENRANDOM           3                       //

#define MAXPASS             256                     // Maximum number of passages ( mul 32 )
#define MAXSTAT             16                      // Maximum status displays
#define MAXLOCO             3                       // Maximum number of local players

#define JOYBUTTON           8                       // Maximum number of joystick buttons

// Messaging stuff
#define MAXMESSAGE          6                       // Number of messages
#define MAXTOTALMESSAGE     2048                    //
#define MESSAGESIZE         80                      //
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
#define LIPWB               2                       //
#define LIPWC               3                       //

#define NOACTION            0xffff                  // Action not valid for this character
#define MAXACTION           76                      // Number of action types
EXTERN char    cActionName[MAXACTION][2];                  // Two letter name code

#define GRABSIZE            90.0f                    // Grab tolerance
#define SEEINVISIBLE        128                     // Cutoff for invisible characters

#define MAXDAMAGETYPE       8                       // Damage types
#define DAMAGENULL          255                     //
#define DAMAGESLASH         0                       //
#define DAMAGECRUSH         1                       //
#define DAMAGEPOKE          2                       //
#define DAMAGEHOLY          3                       // (Most invert Holy damage )
#define DAMAGEEVIL          4                       //
#define DAMAGEFIRE          5                       //
#define DAMAGEICE           6                       //
#define DAMAGEZAP           7                       //
#define DAMAGECHARGE        8                       // 0000x000 Converts damage to mana
#define DAMAGEINVERT        4                       // 00000x00 Makes damage heal
#define DAMAGESHIFT         3                       // 000000xx Resistance ( 1 is common )
#define DAMAGETILETIME      32                      // Invincibility time
#define DAMAGETIME          16                      // Invincibility time
#define DEFENDTIME          16                      // Invincibility time
#define DROPXYVEL           8                       //
#define DROPZVEL            7                       //
#define JUMPATTACKVEL       -2                      //
#define WATERJUMP           12                      //

#define TURNMODEVELOCITY    0                       // Character gets rotation from velocity
#define TURNMODEWATCH       1                       // For watch towers
#define TURNMODESPIN        2                       // For spinning objects
#define TURNMODEWATCHTARGET 3                       // For combat intensive AI

#define MAXCAPNAMESIZE      32                      // Character class names
#define MAXLEVEL            6                       // Basic Levels 0-5
#define MAXEXPERIENCETYPE   8                       // Number of ways to get experience
#define MAXIDSZ             6                       // ID strings per character
#define IDSZNONE            0                       // [NONE]
#define IDSZPARENT          0                       // Parent index
#define IDSZTYPE            1                       // Self index
#define IDSZSKILL           2                       // Skill index
#define IDSZSPECIAL         3                       // Special index
#define IDSZHATE            4                       // Hate index
#define IDSZVULNERABILITY   5                       // Vulnerability index

// XP stuff
#define XPFINDSECRET        0                       // Finding a secret
#define XPWINQUEST          1                       // Beating a module or a subquest
#define XPUSEDUNKOWN        2                       // Used an unknown item
#define XPKILLENEMY         3                       // Killed an enemy
#define XPKILLSLEEPY        4                       // Killed a sleeping enemy
#define XPKILLHATED         5                       // Killed a hated enemy
#define XPTEAMKILL          6                       // Team has killed an enemy
#define XPTALKGOOD          7                       // Talk good, er...  I mean well
#define XPDIRECT            255                     // No modification

// Teams
#define MAXTEAM             27                      // Teams A-Z, +1 more for damage tiles
#define DAMAGETEAM          26                      // For damage tiles
#define EVILTEAM            4                       // E
#define GOODTEAM            6                       // G
#define NULLTEAM            13                      // N

#define LATCHBUTTONLEFT      1                      // Character button presses
#define LATCHBUTTONRIGHT     2                      //
#define LATCHBUTTONJUMP      4                      //
#define LATCHBUTTONALTLEFT   8                      // ( Alts are for grab/drop )
#define LATCHBUTTONALTRIGHT  16                     //
#define LATCHBUTTONPACKLEFT  32                     // ( Packs are for inventory cycle )
#define LATCHBUTTONPACKRIGHT 64                     //
#define LATCHBUTTONRESPAWN   128                    //

// Z velocity stuff
#define JUMPDELAY           20                      // Time between jumps

#define MAPID 0x4470614d                        // The string... MapD

#define RAISE 12 //25                               // Helps correct z level
#define SHADOWRAISE 5                               //
#define DAMAGERAISE 25                              //

#define MAXWATERLAYER 2                             // Maximum water layers
#define MAXWATERFRAME 512                           // Maximum number of wave frames
#define WATERFRAMEAND (MAXWATERFRAME-1)             //
#define WATERPOINTS 4                               // Points in a water fan
#define WATERMODE 4                                 // Ummm...  For making it work, yeah...

#define ROTMESHTOPSIDE                  55          // For figuring out what to draw
#define ROTMESHBOTTOMSIDE               65          //
#define ROTMESHUP                       40 //35          //
#define ROTMESHDOWN                     60          //
EXTERN int rotmeshtopside;                                 // The ones that get used
EXTERN int rotmeshbottomside;                              //
EXTERN int rotmeshup;                                      //
EXTERN int rotmeshdown;                                    //

#define NUMFONTX                        16          // Number of fonts in the bitmap
#define NUMFONTY                        6           //
#define NUMFONT                         (NUMFONTX*NUMFONTY)
#define FONTADD                         4           // Gap between letters
#define NUMBAR                          6           // Number of status bars

#define TABX                            32//16      // Size of little name tag on the bar
#define BARX                            112//216         // Size of bar
#define BARY                            16//8           //
#define NUMTICK                         10//50          // Number of ticks per row
#define TICKX                           8//4           // X size of each tick
#define MAXTICK                         (NUMTICK*10) // Max number of ticks to draw

#define TURNSPD                         0.01f         // Cutoff for turning or same direction

#define MD2START                        0x32504449  // MD2 files start with these four bytes
#define MD2MAXLOADSIZE                  (512*1024)  // Don't load any models bigger than 512k
#define MD2LIGHTINDICES                 163//162    // MD2's store vertices as x,y,z,normal
#define EQUALLIGHTINDEX                 162         // I added an extra index to do the
// spikey mace...

#define MAXTEXTURE                      768         // Max number of textures
#define MAXVERTICES                     2048        // Max number of points in a model
#define MAXCOMMAND                      512         // Max number of commands
#define MAXCOMMANDSIZE                  128          // Max number of points in a command
#define MAXCOMMANDENTRIES               512         // Max entries in a command list ( trigs )
#define MAXMODEL                        256         // Max number of models
#define MAXEVE                          MAXMODEL    // One enchant type per model
#define MAXEVESETVALUE                  24          // Number of sets
#define MAXEVEADDVALUE                  16          // Number of adds
#define MAXFRAME                        (128*32)    // Max number of frames in all models
#define MAXCHR                          512         // Max number of characters

//Object positions
#define GRIPVERTICES                    8           // Each model has 8 grip vertices
#define GRIPRIGHT                       8           // Right weapon grip starts 8 from last
#define GRIPLEFT                        4           // Left weapon grip starts 4 from last
#define GRIPONLY                        4           // Only weapon grip starts 4 from last
#define SPAWNORIGIN                     0           // Center for spawn attachments
#define SPAWNLAST                       1           // Position for spawn attachments
#define INVENTORY                       0           //

#define CHOPPERMODEL                    32          //
#define MAXSECTION                      4           // T-wi-n-k...  Most of 4 sections
#define MAXCHOP                         (MAXMODEL*CHOPPERMODEL)
#define CHOPSIZE                        8
#define CHOPDATACHUNK                   (MAXCHOP*CHOPSIZE)

#define MAXMESHFAN                      (512*512)   // Terrain mesh size
#define MAXMESHSIZEY                    1024        // Max fans in y direction
#define BYTESFOREACHVERTEX              14          // 14 bytes each
#define MAXMESHVERTICES                 16          // Fansquare vertices
#define MAXMESHTYPE                     64          // Number of fansquare command types
#define MAXMESHCOMMAND                  4           // Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES           32          // Fansquare command list size
#define MAXMESHCOMMANDSIZE              32          // Max trigs in each command
#define MAXTILETYPE                     256         // Max number of tile images
#define MAXMESHRENDER                   1024        // Max number of tiles to draw
#define FANOFF                          0xffff      // Don't draw the fansquare if tile = this
#define INVALID_TILE                    ((Uint16)(~(Uint16)0))   // Don't draw the fansquare if tile = this
#define OFFEDGE                         0           // Character not on a fan ( maybe )

#define MESHFXREF                       0           // 0 This tile is drawn 1st
#define MESHFXSHA                       1           // 0 This tile is drawn 2nd
#define MESHFXDRAWREF                   2           // 1 Draw reflection of characters
#define MESHFXANIM                      4           // 2 Animated tile ( 4 frame )
#define MESHFXWATER                     8           // 3 Render water above surface ( Water details are set per module )
#define MESHFXWALL                      16          // 4 Wall ( Passable by ghosts, particles )
#define MESHFXIMPASS                    32          // 5 Impassable
#define MESHFXDAMAGE                    64          // 6 Damage
#define MESHFXSLIPPY                    128         // 7 Ice or normal

// Physics
#define SLOPE                           800         // Slope increments for terrain normals
#define SLIDE                           0.04f         // Acceleration for steep hills
#define SLIDEFIX                        0.08f         // To make almost flat surfaces flat

#define STOPBOUNCING                    0.1f //1.0f         // To make objects stop bouncing
#define STOPBOUNCINGPART                5.0f         // To make particles stop bouncing

// Particles
#define PRTLIGHTSPRITE                  0           // Magic effect particle
#define PRTSOLIDSPRITE                  1           // Sprite particle
#define PRTALPHASPRITE                  2           // Smoke particle
#define MAXPARTICLEIMAGE                256         // Number of particle images ( frames )

/* SDL_GetTicks() always returns milli seconds */
#define TICKS_PER_SEC                   1000
EXTERN Sint32 framelimit                EQ(30);
#define UPDATES_PER_SEC                 50
#define UPDATE_SKIP                     ((float)TICKS_PER_SEC/(float)UPDATES_PER_SEC)
#define ONESECOND                       TICKS_PER_SEC

#define TRANSCOLOR                      0           // Transparent color

// Debug option
EXTERN bool_t  gGrabMouse EQ( btrue );
EXTERN bool_t gHideMouse EQ( bfalse );
EXTERN bool_t gDevMode EQ( bfalse );
// Debug option

EXTERN int     animtileupdateand  EQ( 7 );                        // New tile every 7 frames
EXTERN Uint16  animtileframeand  EQ( 3 );              // Only 4 frames
EXTERN Uint16  animtilebaseand  EQ( 0xfffc );          //
EXTERN Uint16  biganimtileframeand  EQ( 7 );           // For big tiles
EXTERN Uint16  biganimtilebaseand  EQ( 0xfff8 );       //
EXTERN Uint16  animtileframeadd  EQ( 0 );              // Current frame

EXTERN Uint16  bookicon  EQ( 3 );                      // The first book icon

#define NORTH 16384                                 // Character facings
#define SOUTH 49152                                 //
#define EAST 32768                                  //
#define WEST 0                                      //
#define FRONT 0                                     // Attack directions
#define BEHIND 32768                                //
#define LEFT 49152                                  //
#define RIGHT 16384                                 //

#define MAXXP 200000                               // Maximum experience
#define MAXMONEY 9999                               // Maximum money
#define NOLEADER 65535                              // If the team has no leader...

EXTERN bool_t                   teamhatesteam[MAXTEAM][MAXTEAM];     // Don't damage allies...
EXTERN Uint16                    teammorale[MAXTEAM];                 // Number of characters on team
EXTERN Uint16                    teamleader[MAXTEAM];                 // The leader of the team
EXTERN Uint16                    teamsissy[MAXTEAM];                  // Whoever called for help last
EXTERN Sint16                   damagetileparttype;
EXTERN short                   damagetilepartand;
EXTERN short                   damagetilesound;
EXTERN short                   damagetilesoundtime;
EXTERN Uint16                  damagetilemindistance;

#define TILESOUNDTIME 16
#define TILEREAFFIRMAND  3

// Minimap stuff
#define MAXBLIP 128
#define NUMBLIP 6             //Blip textures
EXTERN Uint16           numblip  EQ( 0 );
EXTERN Uint16           blipx[MAXBLIP];
EXTERN Uint16           blipy[MAXBLIP];
EXTERN Uint8                   blipc[MAXBLIP];
EXTERN Uint8                   mapon  EQ( bfalse );
EXTERN Uint8                   mapvalid  EQ( bfalse );
EXTERN Uint8                   youarehereon  EQ( bfalse );

// Display
EXTERN Uint8                   timeron  EQ( bfalse );          // Game timer displayed?
EXTERN Uint32                   timervalue  EQ( 0 );           // Timer time ( 50ths of a second )
EXTERN char                    szfpstext[]  EQ( "000 FPS" );
EXTERN Uint8                   fpson  EQ( btrue );             // FPS displayed?

// Timers
EXTERN Sint32              sttclock;                   // GetTickCount at start
EXTERN Sint32              allclock  EQ( 0 );             // The total number of ticks so far
EXTERN Sint32              lstclock  EQ( 0 );             // The last total of ticks so far
EXTERN Sint32              wldclock  EQ( 0 );             // The sync clock
EXTERN Sint32              fpsclock  EQ( 0 );             // The number of ticks this second
EXTERN Uint32                   wldframe  EQ( 0 );             // The number of frames that should have been drawn
EXTERN Uint32                   allframe  EQ( 0 );             // The total number of frames drawn so far
EXTERN Uint32                   fpsframe  EQ( 0 );             // The number of frames drawn this second
EXTERN Uint32                   statclock  EQ( 0 );            // For stat regeneration
EXTERN Uint32                   pitclock  EQ( 0 );             // For pit kills
EXTERN Uint32                   outofsync  EQ( 0 );
EXTERN Uint8           parseerror  EQ( bfalse );

//Pitty stuff
EXTERN bool_t                  pitskill  EQ( bfalse );          // Do they kill?
EXTERN bool_t                  pitsfall  EQ( bfalse );          // Do they teleport?
EXTERN Uint32          pitx;
EXTERN Uint32          pity;
EXTERN Uint32          pitz;

EXTERN bool_t                    fullscreen EQ( bfalse );        // Start in fullscreen?
EXTERN bool_t                    clearson  EQ( btrue );             // Do we clear every time?
EXTERN bool_t                    gameactive  EQ( bfalse );       // Stay in game or quit to windows?
EXTERN bool_t                    moduleactive  EQ( bfalse );     // Is the control loop still going?
EXTERN bool_t                    soundon  EQ( btrue );              // Is the sound alive?
EXTERN bool_t                    mouseon  EQ( btrue );              // Is the mouse alive?
EXTERN bool_t                    keyon  EQ( btrue );                // Is the keyboard alive?
EXTERN bool_t                    joyaon  EQ( 0 );               // Is the holy joystick alive?
EXTERN bool_t                    joybon  EQ( 0 );               // Is the other joystick alive?
EXTERN bool_t                    staton  EQ( btrue );               // Draw the status bars?
EXTERN bool_t                    phongon  EQ( btrue );              // Do phong overlay?
EXTERN bool_t                    networkon  EQ( btrue );            // Try to connect?
EXTERN bool_t                    serviceon  EQ( bfalse );        // Do I need to free the interface?
EXTERN bool_t                    twolayerwateron  EQ( btrue );      // Two layer water?
EXTERN bool_t                    menuactive  EQ( bfalse );       // Menu running?
EXTERN int             selectedPlayer EQ( 0 );           // Which player is currently selected to play
EXTERN bool_t                    hostactive  EQ( bfalse );       // Hosting?
EXTERN bool_t                    readytostart;               // Ready to hit the Start Game button?
EXTERN bool_t                    waitingforplayers;          // Has everyone talked to the host?
EXTERN bool_t                    respawnvalid;               // Can players respawn with Spacebar?
EXTERN bool_t                    respawnanytime;             // True if it's a small level...
EXTERN bool_t                    importvalid;                // Can it import?
EXTERN bool_t                    exportvalid;                // Can it export?
EXTERN bool_t                    rtscontrol;                 // Play as a real-time stragedy? BAD REMOVE
EXTERN bool_t                    netmessagemode;             // Input text from keyboard?
EXTERN bool_t                    backgroundvalid;            // Allow large background?
EXTERN bool_t                    overlayvalid;               // Allow large overlay?
EXTERN bool_t                    nolocalplayers;             // Are there any local players?
EXTERN bool_t                    usefaredge;                 // Far edge maps? (Outdoor)
EXTERN bool_t                    beatmodule;                 // Show Module Ended text?
EXTERN Uint8                   autoturncamera;             // Type of camera control...
EXTERN Uint8                   doturntime;                 // Time for smooth turn
EXTERN Uint8                   netmessagedelay;            // For slowing down input
EXTERN int                     netmessagewrite;            // The cursor position
EXTERN int                     netmessagewritemin;         // The starting cursor position
EXTERN char                    netmessage[MESSAGESIZE];    // The input message
EXTERN Uint8                     importamount;               // Number of imports for this module
EXTERN Uint8                     playeramount;               //
EXTERN Uint32                   seed  EQ( 0 );                 // The module seed
EXTERN char                    pickedmodule[64];           // The module load name
EXTERN int                     pickedindex;                // The module index number
EXTERN int                     playersready;               // Number of players ready to start
EXTERN int                     playersloaded;              //

//Respawning
EXTERN bool_t                    alllocalpladead;            // Has everyone died?
EXTERN Uint16                   revivetimer EQ(0);

// JF - Added so that the video mode might be determined outside of the graphics code
extern SDL_Surface *displaySurface;

// Networking
EXTERN int                     localmachine  EQ( 0 );         // 0 is host, 1 is 1st remote, 2 is 2nd...
EXTERN int                     numimport;                  // Number of imports from this machine
EXTERN Uint8                   localcontrol[16];           // For local imports
EXTERN short                   localslot[16];              // For local imports

// Setup values
EXTERN int                     maxmessage  EQ( MAXMESSAGE );  //
EXTERN int                     scrd  EQ( 8 );                 // Screen bit depth
EXTERN int                     scrz  EQ( 16 );                // Screen z-buffer depth ( 8 unsupported )
EXTERN int                     scrx  EQ( 320 );               // Screen X size
EXTERN int                     scry  EQ( 200 );               // Screen Y size
EXTERN Uint8                   reffadeor  EQ( 0 );            // 255 = Don't fade reflections
EXTERN Uint8                   messageon  EQ( btrue );         // Messages?
EXTERN bool_t                  overlayon  EQ( bfalse );        // Draw overlay?
EXTERN bool_t                  perspective  EQ( bfalse );      // Perspective correct textures?
EXTERN bool_t                  dither  EQ( bfalse );           // Dithering?
EXTERN GLuint                  shading  EQ( GL_SMOOTH );           // Gourad shading?
EXTERN bool_t                  antialiasing  EQ( bfalse );     // Antialiasing?
EXTERN bool_t                  refon  EQ( bfalse );            // Reflections?
EXTERN bool_t                  shaon  EQ( bfalse );            // Shadows?
EXTERN Uint8                   texturefilter  EQ( 1 );     // Texture filtering?
EXTERN bool_t                  wateron  EQ( btrue );           // Water overlays?
EXTERN bool_t                  shasprite  EQ( bfalse );        // Shadow sprites?
EXTERN bool_t                  zreflect  EQ( bfalse );         // Reflection z buffering?
EXTERN float                   mousesense  EQ( 6 );           // Sensitivity threshold
EXTERN float                   mousesustain EQ( 0.50f );         // Falloff rate for old movement
EXTERN float                   mousecover EQ( 0.50f );           // For falloff
EXTERN Sint32                    mousex  EQ( 0 );               // Mouse X movement counter
EXTERN Sint32                    mousey  EQ( 0 );               // Mouse Y movement counter
EXTERN Sint32                    mousez  EQ( 0 );               // Mouse wheel movement counter
EXTERN int                     cursorx  EQ( 0 );              // Cursor position
EXTERN int                     cursory  EQ( 0 );              //
EXTERN float                   mouselatcholdx  EQ( 0 );       // For sustain
EXTERN float                   mouselatcholdy  EQ( 0 );       //
EXTERN Uint8                   mousebutton[4];             // Mouse button states
EXTERN bool_t                    pressed EQ( 0 );                //
EXTERN bool_t                    clicked EQ( 0 );                //
EXTERN bool_t           pending_click EQ( 0 );
// EWWWW. GLOBALS ARE EVIL.

// Input Control
EXTERN int                     joyax  EQ( 0 );                // Joystick A
EXTERN int                     joyay  EQ( 0 );                //
EXTERN Uint8                   joyabutton[JOYBUTTON];      //
EXTERN int                     joybx  EQ( 0 );                // Joystick B
EXTERN int                     joyby  EQ( 0 );                //
EXTERN Uint8                   joybbutton[JOYBUTTON];      //
EXTERN Uint8                   msb, jab, jbb;              // Button masks
#define  KEYDOWN(k) (sdlkeybuffer[k])            // Helper for gettin' em

// Weather and water gfx
EXTERN int                     weatheroverwater EQ( bfalse );       // Only spawn over water?
EXTERN int                     weathertimereset EQ( 10 );          // Rate at which weather particles spawn
EXTERN int                     weathertime EQ( 0 );                // 0 is no weather
EXTERN int                     weatherplayer;
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
EXTERN float                   waterlayerv[MAXWATERLAYER];     //
EXTERN float                   waterlayeruadd[MAXWATERLAYER];  // Texture movement
EXTERN float                   waterlayervadd[MAXWATERLAYER];  //
EXTERN float                   waterlayerzadd[MAXWATERLAYER][MAXWATERFRAME][WATERMODE][WATERPOINTS];
EXTERN Uint8                   waterlayercolor[MAXWATERLAYER][MAXWATERFRAME][WATERMODE][WATERPOINTS];
EXTERN Uint16           waterlayerframe[MAXWATERLAYER]; // Frame
EXTERN Uint16           waterlayerframeadd[MAXWATERLAYER];      // Speed
EXTERN float                   waterlayerdistx[MAXWATERLAYER];         // For distant backgrounds
EXTERN float                   waterlayerdisty[MAXWATERLAYER];         //
EXTERN Uint32                   waterspek[256];             // Specular highlights
EXTERN float                   foregroundrepeat  EQ( 1 );     //
EXTERN float                   backgroundrepeat  EQ( 1 );     //

// Fog stuff
EXTERN bool_t                  fogallowed  EQ( btrue );        //
EXTERN bool_t                  fogon  EQ( bfalse );            // Do ground fog?
EXTERN float                   fogbottom  EQ( 0.0f );          //
EXTERN float                   fogtop  EQ( 100 );             //
EXTERN float                   fogdistance  EQ( 100 );        //
EXTERN Uint8                   fogred  EQ( 255 );             //  Fog collour
EXTERN Uint8                   foggrn  EQ( 255 );             //
EXTERN Uint8                   fogblu  EQ( 255 );             //
EXTERN Uint8                   fogaffectswater;

// Camera control stuff
EXTERN int                     camswing  EQ( 0 );             // Camera swingin'
EXTERN int                     camswingrate  EQ( 0 );         //
EXTERN float                   camswingamp  EQ( 0 );          //
EXTERN float                   camx  EQ( 0 );                 // Camera position
EXTERN float                   camy  EQ( 1500 );              //
EXTERN float                   camz  EQ( 1500 );               // 500-1000
EXTERN float                   camzoom  EQ( 1000 );           // Distance from the trackee
EXTERN float                   camtrackxvel;               // Change in trackee position
EXTERN float                   camtrackyvel;               //
EXTERN float                   camtrackzvel;               //
EXTERN float                   camcenterx;                 // Move character to side before tracking
EXTERN float                   camcentery;                 //
EXTERN float                   camtrackx;                  // Trackee position
EXTERN float                   camtracky;                  //
EXTERN float                   camtrackz;                  //
EXTERN float                   camtracklevel;              //
EXTERN float                   camzadd  EQ( 800 );            // Camera height above terrain
EXTERN float                   camzaddgoto  EQ( 800 );        // Desired z position
EXTERN float                   camzgoto  EQ( 800 );           //
EXTERN float                   camturnleftright  EQ( ( float )( -PI / 4 ) );   // Camera rotations
EXTERN float                   camturnleftrightone  EQ( ( float )( -PI / 4 ) / ( TWO_PI ) );
EXTERN Uint16                  camturnleftrightshort  EQ( 0 );
EXTERN float                   camturnadd  EQ( 0 );           // Turning rate
EXTERN float                   camsustain  EQ( 0.60f );         // Turning rate falloff
EXTERN float                   camturnupdown  EQ( ( float )( PI / 4 ) );
EXTERN float                   camroll  EQ( 0 );              //
EXTERN float                   cornerx[4];                 // Render area corners
EXTERN float                   cornery[4];                 //
EXTERN int                     cornerlistlowtohighy[4];    // Ordered list
EXTERN float                     cornerlowx;                 // Render area extremes
EXTERN float                     cornerhighx;                //
EXTERN float                     cornerlowy;                 //
EXTERN float                     cornerhighy;                //
EXTERN int                     fontoffset;                 // Line up fonts from top of screen

/*OpenGL Textures*/
EXTERN  STRING      TxFormatSupported[50]; // OpenGL icon surfaces
EXTERN  Uint8       maxformattypes EQ(0);
EXTERN  GLTexture       TxIcon[MAXTEXTURE+1];            // OpenGL icon surfaces
EXTERN  GLTexture       TxTitleImage[MAXMODULE];          // OpenGL title image surfaces
EXTERN  GLTexture       TxFont;                    // OpenGL font surface
EXTERN  GLTexture       TxBars;                    // OpenGL status bar surface
EXTERN  GLTexture       TxBlip;                    // OpenGL you are here surface
EXTERN  GLTexture       TxMap;                    // OpenGL map surface
EXTERN  GLTexture       txTexture[MAXTEXTURE];               /* All textures */
EXTERN  bool_t          use_sdl_image EQ(btrue);    //Allow advanced SDL_Image functions?
// Anisotropic filtering - yay! :P
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
EXTERN float maxAnisotropy;                    // Max anisotropic filterings (Between 1.00f and 16.00f)

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

EXTERN glMatrix mWorld;                      // World Matrix
EXTERN glMatrix mView;                      // View Matrix
EXTERN glMatrix mViewSave;                    // View Matrix initial state
EXTERN glMatrix mProjection;                  // Projection Matrix

// Input player control
#define MAXLOADPLAYER   100
EXTERN Uint8                   numloadplayer  EQ( 0 );
EXTERN char                    loadplayername[MAXLOADPLAYER][MAXCAPNAMESIZE];
EXTERN char                    loadplayerdir[MAXLOADPLAYER][16];
EXTERN int                     playericon[MAXLOADPLAYER];
EXTERN int                     nullicon  EQ( 0 );
EXTERN int                     keybicon  EQ( 0 );
EXTERN int                     mousicon  EQ( 0 );
EXTERN int                     joyaicon  EQ( 0 );
EXTERN int                     joybicon  EQ( 0 );
EXTERN int                     keybplayer  EQ( 0 );
EXTERN int                     mousplayer  EQ( 0 );
EXTERN int                     joyaplayer  EQ( 0 );
EXTERN int                     joybplayer  EQ( 0 );

// Interface stuff
EXTERN rect_t                    iconrect;                   // The 32x32 icon rectangle
EXTERN SDL_Rect           fontrect[NUMFONT];          // The font rectangles
EXTERN Uint8       fontxspacing[NUMFONT];      // The spacing stuff
EXTERN Uint8       fontyspacing;               //
EXTERN rect_t                    tabrect[NUMBAR];            // The tab rectangles
EXTERN rect_t                    barrect[NUMBAR];            // The bar rectangles
EXTERN rect_t                    bliprect[NUMBAR];           // The blip rectangles
EXTERN rect_t                    maprect;                    // The map rectangle

#define SPARKLESIZE 28
#define SPARKLEADD 2
#define MAPSIZE 96
#define BLIPSIZE 6

// Lightning effects
#define MAXLIGHTLEVEL                   16          // Number of premade light intensities
#define MAXSPEKLEVEL                    16          // Number of premade specularities
#define MAXLIGHTROTATION                256         // Number of premade light maps
#define MAXDYNADIST                     2700        // Leeway for offscreen lights
#define TOTALMAXDYNA                    64          // Absolute max number of dynamic lights
EXTERN int                     maxlights EQ( 8 ); // Max number of lights to draw
EXTERN int                     numdynalight;               // Number of dynamic lights
EXTERN int                     numdynalight;               // Number of dynamic lights
EXTERN int                     dynadistancetobeat;         // The number to beat
EXTERN int                     dynadistance[TOTALMAXDYNA];      // The distances
EXTERN float                   dynalightlistx[TOTALMAXDYNA];    // Light position
EXTERN float                   dynalightlisty[TOTALMAXDYNA];    //
EXTERN float                   dynalightlevel[TOTALMAXDYNA];    // Light level
EXTERN float                   dynalightfalloff[TOTALMAXDYNA];  // Light falloff
EXTERN Uint8                   lightdirectionlookup[65536];// For lighting characters
EXTERN Uint8                   lighttable[MAXLIGHTLEVEL][MAXLIGHTROTATION][MD2LIGHTINDICES];
EXTERN Uint8                   cLoadBuffer[MD2MAXLOADSIZE];// Where to put an MD2

//Mesh
EXTERN Uint32 maplrtwist[256];            // For surface normal of mesh
EXTERN Uint32 mapudtwist[256];            //
EXTERN float                   vellrtwist[256];            // For sliding down steep hills
EXTERN float                   veludtwist[256];            //
EXTERN Uint8 flattwist[256];             //

// Camera stuff
#define TRACKXAREALOW     100
#define TRACKXAREAHIGH    180
#define TRACKYAREAMINLOW  320
#define TRACKYAREAMAXLOW  460
#define TRACKYAREAMINHIGH 460
#define TRACKYAREAMAXHIGH 600

// Values for manipulating AI
#define MAXWAY              8                       // Waypoints
#define WAYTHRESH           64                      // Threshold for reaching waypoint
#define MAXSTOR             16                      // Storage data (Used in SetXY)
#define STORAND             15                      //

// Character stuff
EXTERN int                     numfreechr EQ( 0 );             // For allocation
EXTERN Uint16           freechrlist[MAXCHR];        //
EXTERN glMatrix                chrmatrix[MAXCHR];      // Character's matrix
EXTERN char                    chrmatrixvalid[MAXCHR];     // Did we make one yet?
EXTERN char                    chrname[MAXCHR][MAXCAPNAMESIZE];  // Character name
EXTERN bool_t          chron[MAXCHR];              // Does it exist?
EXTERN Uint8          chronold[MAXCHR];           // Network fix
EXTERN Uint8          chralive[MAXCHR];           // Is it alive?
EXTERN Uint8          chrwaskilled[MAXCHR];       // Fix for network
EXTERN Uint8          chrinpack[MAXCHR];          // Is it in the inventory?
EXTERN Uint8          chrwasinpack[MAXCHR];       // Temporary thing...
EXTERN Uint16           chrnextinpack[MAXCHR];      // Link to the next item
EXTERN Uint8          chrnuminpack[MAXCHR];       // How many
EXTERN Uint8          chropenstuff[MAXCHR];       // Can it open chests/doors?
EXTERN Uint8          chrlifecolor[MAXCHR];       // Bar color
EXTERN Uint8          chrsparkle[MAXCHR];         // Sparkle color or 0 for off
EXTERN Sint16            chrlife[MAXCHR];            // Basic character stats
EXTERN Sint16            chrlifemax[MAXCHR];         //   All 8.8 fixed point
EXTERN Uint16           chrlifeheal[MAXCHR];        //
EXTERN Uint8          chrmanacolor[MAXCHR];       // Bar color
EXTERN Uint8          chrammomax[MAXCHR];         // Ammo stuff
EXTERN Uint16          chrammo[MAXCHR];            //
EXTERN Uint8          chrgender[MAXCHR];          // Gender
EXTERN Sint16            chrmana[MAXCHR];            // Mana stuff
EXTERN Sint16            chrmanamax[MAXCHR];         //
EXTERN Sint16            chrmanaflow[MAXCHR];        //
EXTERN Sint16            chrmanareturn[MAXCHR];      //
EXTERN Sint16            chrstrength[MAXCHR];        // Strength
EXTERN Sint16            chrwisdom[MAXCHR];          // Wisdom
EXTERN Sint16            chrintelligence[MAXCHR];    // Intelligence
EXTERN Sint16            chrdexterity[MAXCHR];       // Dexterity
EXTERN Uint8             chraitype[MAXCHR];          // The AI script to run
EXTERN bool_t                    chricon[MAXCHR];            // Show the icon?
EXTERN bool_t                    chrcangrabmoney[MAXCHR];    // Picks up coins?
EXTERN bool_t                    chrisplayer[MAXCHR];        // btrue = player
EXTERN bool_t                    chrislocalplayer[MAXCHR];   // btrue = local player
EXTERN Uint16           chrlastitemused[MAXCHR];        // The last item the character used
EXTERN Uint16           chraitarget[MAXCHR];        // Who the AI is after
EXTERN Uint16           chraiowner[MAXCHR];         // The character's owner
EXTERN Uint16           chraichild[MAXCHR];         // The character's child
EXTERN int                     chraistate[MAXCHR];         // Short term memory for AI
EXTERN int                     chraicontent[MAXCHR];       // More short term memory
EXTERN Uint16           chraitime[MAXCHR];          // AI Timer
EXTERN Uint8          chraigoto[MAXCHR];          // Which waypoint
EXTERN Uint8          chraigotoadd[MAXCHR];       // Where to stick next
EXTERN float                   chraigotox[MAXCHR][MAXWAY]; // Waypoint
EXTERN float                   chraigotoy[MAXCHR][MAXWAY]; // Waypoint
EXTERN int                     chraix[MAXCHR][MAXSTOR];    // Temporary values...  SetXY
EXTERN int                     chraiy[MAXCHR][MAXSTOR];    //
EXTERN Uint8          chrstickybutt[MAXCHR];      // Rests on floor
EXTERN Uint8          chrenviro[MAXCHR];          // Environment map?
EXTERN float                   chroldx[MAXCHR];            // Character's last position
EXTERN float                   chroldy[MAXCHR];            //
EXTERN float                   chroldz[MAXCHR];            //
EXTERN Uint8          chrinwater[MAXCHR];         //
EXTERN Uint16           chroldturn[MAXCHR];         //
EXTERN Uint32         chralert[MAXCHR];           // Alerts for AI script
EXTERN Uint8          chrflyheight[MAXCHR];       // Height to stabilize at
EXTERN Uint8          chrteam[MAXCHR];            // Character's team
EXTERN Uint8          chrbaseteam[MAXCHR];        // Character's starting team
EXTERN Uint8          chrstaton[MAXCHR];          // Display stats?
EXTERN float                   chrxstt[MAXCHR];            // Starting position
EXTERN float                   chrystt[MAXCHR];            //
EXTERN float                   chrzstt[MAXCHR];            //
EXTERN float                   chrxpos[MAXCHR];            // Character's position
EXTERN float                   chrypos[MAXCHR];            //
EXTERN float                   chrzpos[MAXCHR];            //
EXTERN float                   chrxvel[MAXCHR];            // Character's velocity
EXTERN float                   chryvel[MAXCHR];            //
EXTERN float                   chrzvel[MAXCHR];            //
EXTERN float                   chrlatchx[MAXCHR];          // Character latches
EXTERN float                   chrlatchy[MAXCHR];          //
EXTERN Uint8          chrlatchbutton[MAXCHR];     // Button latches
EXTERN Uint16          chrreloadtime[MAXCHR];      // Time before another shot
EXTERN float                   chrmaxaccel[MAXCHR];        // Maximum acceleration
EXTERN float                   chrscale[MAXCHR];           // Character's size (useful)
EXTERN float                   chrfat[MAXCHR];             // Character's size (legible)
EXTERN float                   chrsizegoto[MAXCHR];        // Character's size goto ( legible )
EXTERN Uint8          chrsizegototime[MAXCHR];    // Time left in siez change
EXTERN float                   chrdampen[MAXCHR];          // Bounciness
EXTERN float                   chrlevel[MAXCHR];           // Height of tile
EXTERN float                   chrjump[MAXCHR];            // Jump power
EXTERN Uint8          chrjumptime[MAXCHR];        // Delay until next jump
EXTERN Uint8          chrjumpnumber[MAXCHR];      // Number of jumps remaining
EXTERN Uint8          chrjumpnumberreset[MAXCHR]; // Number of jumps total, 255=Flying
EXTERN Uint8          chrjumpready[MAXCHR];       // For standing on a platform character
EXTERN Uint32         chronwhichfan[MAXCHR];      // Where the char is
EXTERN Uint8          chrindolist[MAXCHR];        // Has it been added yet?
EXTERN Uint16           chruoffset[MAXCHR];         // For moving textures
EXTERN Uint16           chrvoffset[MAXCHR];         //
EXTERN Uint16           chruoffvel[MAXCHR];         // Moving texture speed
EXTERN Uint16           chrvoffvel[MAXCHR];         //
EXTERN Uint16           chrturnleftright[MAXCHR];   // Character's rotation 0 to 65535
EXTERN Uint16           chrlightturnleftright[MAXCHR];// Character's light rotation 0 to 65535
EXTERN Uint16           chrturnmaplr[MAXCHR];       //
EXTERN Uint16           chrturnmapud[MAXCHR];       //
EXTERN Uint16           chrtexture[MAXCHR];         // Character's skin
EXTERN Uint8          chrmodel[MAXCHR];           // Character's model
EXTERN Uint8          chrbasemodel[MAXCHR];       // The true form
EXTERN Uint8          chractionready[MAXCHR];     // Ready to play a new one
EXTERN Uint8          chraction[MAXCHR];          // Character's action
EXTERN bool_t          chrkeepaction[MAXCHR];      // Keep the action playing
EXTERN bool_t          chrloopaction[MAXCHR];      // Loop it too
EXTERN Uint8          chrnextaction[MAXCHR];      // Character's action to play next
EXTERN Uint16           chrframe[MAXCHR];           // Character's frame
EXTERN Uint16           chrlastframe[MAXCHR];       // Character's last frame
EXTERN Uint8          chrlip[MAXCHR];             // Character's frame in betweening
EXTERN Uint8          chrvrta[MAXCHR][MAXVERTICES];// Lighting hack ( Ooze )
EXTERN Uint16           chrholdingwhich[MAXCHR][2]; // !=MAXCHR if character is holding something
EXTERN Uint16           chrattachedto[MAXCHR];      // !=MAXCHR if character is a held weapon
EXTERN Uint16           chrweapongrip[MAXCHR][4];   // Vertices which describe the weapon grip
EXTERN Uint8          chralpha[MAXCHR];           // 255 = Solid, 0 = Invisible
EXTERN Uint8          chrbasealpha[MAXCHR];
EXTERN Uint8          chrlight[MAXCHR];           // 1 = Light, 0 = Normal
EXTERN Uint8          chrflashand[MAXCHR];        // 1,3,7,15,31 = Flash, 255 = Don't
EXTERN Uint8          chrlightlevel[MAXCHR];      // 0-255, terrain light
EXTERN Uint8          chrsheen[MAXCHR];           // 0-15, how shiny it is
EXTERN Uint8          chrtransferblend[MAXCHR];   // Give transparency to weapons?
EXTERN Uint8          chrisitem[MAXCHR];          // Is it grabbable?
EXTERN Uint8          chrinvictus[MAXCHR];        // Totally invincible?
EXTERN Uint8          chrismount[MAXCHR];         // Can you ride it?
EXTERN Uint8          chrredshift[MAXCHR];        // Color channel shifting
EXTERN Uint8          chrgrnshift[MAXCHR];        //
EXTERN Uint8          chrblushift[MAXCHR];        //
EXTERN Uint8          chrshadowsize[MAXCHR];      // Size of shadow
EXTERN Uint8          chrbumpsize[MAXCHR];        // Size of bumpers
EXTERN Uint8          chrbumpsizebig[MAXCHR];     // For octagonal bumpers
EXTERN Uint8          chrbumpheight[MAXCHR];      // Distance from head to toe
EXTERN Uint8          chrshadowsizesave[MAXCHR];  // Without size modifiers
EXTERN Uint8          chrbumpsizesave[MAXCHR];    //
EXTERN Uint8          chrbumpsizebigsave[MAXCHR]; //
EXTERN Uint8          chrbumpheightsave[MAXCHR];  //
EXTERN Uint16           chrbumpnext[MAXCHR];        // Next character on fanblock
EXTERN Uint16           chrbumplast[MAXCHR];        // Last character it was bumped by
EXTERN float                   chrbumpdampen[MAXCHR];      // Character bump mass
EXTERN Uint16           chrattacklast[MAXCHR];      // Last character it was attacked by
EXTERN Uint16           chrhitlast[MAXCHR];         // Last character it hit
EXTERN Uint16           chrdirectionlast[MAXCHR];   // Direction of last attack/healing
EXTERN Uint8          chrdamagetypelast[MAXCHR];  // Last damage type
EXTERN Uint8          chrplatform[MAXCHR];        // Can it be stood on
EXTERN Uint8          chrwaterwalk[MAXCHR];       // Always above watersurfacelevel?
EXTERN Uint8          chrturnmode[MAXCHR];        // Turning mode
EXTERN Uint8          chrsneakspd[MAXCHR];        // Sneaking if above this speed
EXTERN Uint8          chrwalkspd[MAXCHR];         // Walking if above this speed
EXTERN Uint8          chrrunspd[MAXCHR];          // Running if above this speed
EXTERN Uint8          chrdamagetargettype[MAXCHR];// Type of damage for AI DamageTarget
EXTERN Uint8          chrreaffirmdamagetype[MAXCHR]; // For relighting torches
EXTERN Uint8          chrdamagemodifier[MAXCHR][MAXDAMAGETYPE];  // Resistances and inversion
EXTERN Uint8          chrdamagetime[MAXCHR];      // Invincibility timer
EXTERN Uint8          chrdefense[MAXCHR];         // Base defense rating
EXTERN Uint16           chrweight[MAXCHR];          // Weight ( for pressure plates )
EXTERN Uint8          chrpassage[MAXCHR];         // The passage associated with this character
EXTERN Uint32         chrorder[MAXCHR];           // The last order given the character
EXTERN Uint16          chrcounter[MAXCHR];         // The rank of the character on the order chain
EXTERN Uint16           chrholdingweight[MAXCHR];   // For weighted buttons
EXTERN Sint16            chrmoney[MAXCHR];           // Money
EXTERN Sint16            chrlifereturn[MAXCHR];      // Regeneration/poison
EXTERN Sint16            chrmanacost[MAXCHR];        // Mana cost to use
EXTERN Uint8          chrstoppedby[MAXCHR];       // Collision mask
EXTERN Uint32         chrexperience[MAXCHR];      // Experience
EXTERN Uint8          chrexperiencelevel[MAXCHR]; // Experience Level
EXTERN Sint16            chrgrogtime[MAXCHR];        // Grog timer
EXTERN Sint16            chrdazetime[MAXCHR];        // Daze timer
EXTERN Uint8          chriskursed[MAXCHR];        // Can't be dropped?
EXTERN Uint8          chrnameknown[MAXCHR];       // Is the name known?
EXTERN Uint8          chrammoknown[MAXCHR];       // Is the ammo known?
EXTERN Uint8          chrhitready[MAXCHR];        // Was it just dropped?
EXTERN Sint16            chrboretime[MAXCHR];        // Boredom timer
EXTERN Uint8          chrcarefultime[MAXCHR];     // "You hurt me!" timer
EXTERN bool_t          chrcanbecrushed[MAXCHR];    // Crush in a door?
EXTERN Uint8          chrinwhichhand[MAXCHR];     // GRIPLEFT or GRIPRIGHT
EXTERN Uint8           chrisequipped[MAXCHR];      // For boots and rings and stuff
EXTERN Uint16           chrfirstenchant[MAXCHR];    // Linked list for enchants
EXTERN Uint16           chrundoenchant[MAXCHR];     // Last enchantment spawned
EXTERN bool_t                    chrcanchannel[MAXCHR];      //
EXTERN bool_t                    chroverlay[MAXCHR];         // Is this an overlay?  Track aitarget...
EXTERN Uint8           chrmissiletreatment[MAXCHR];// For deflection, etc.
EXTERN Uint8           chrmissilecost[MAXCHR];     // Mana cost for each one
EXTERN Uint16           chrmissilehandler[MAXCHR];  // Who pays the bill for each one...
EXTERN Uint16           chrdamageboost[MAXCHR];     // Add to swipe damage
EXTERN bool_t           chrisshopitem[MAXCHR];     // Spawned in a shop?

//Skills
EXTERN Sint8                     chrshieldproficiency[MAXCHR];  // Can it use shields?
EXTERN bool_t                    chrcanjoust[MAXCHR]; //
EXTERN bool_t                    chrcanuseadvancedweapons[MAXCHR]; //
EXTERN bool_t                    chrcanseeinvisible[MAXCHR]; //
EXTERN bool_t                    chrcanseekurse[MAXCHR];     //
EXTERN bool_t          chrcanusedivine[MAXCHR];
EXTERN bool_t          chrcanusearcane[MAXCHR];
EXTERN bool_t          chrcanusetech[MAXCHR];
EXTERN bool_t          chrcandisarm[MAXCHR];
EXTERN bool_t          chrcanbackstab[MAXCHR];
EXTERN bool_t          chrcanusepoison[MAXCHR];
EXTERN bool_t          chrcanread[MAXCHR];

#define INVISIBLE           20                      // The character can't be detected

//AI targeting
#define NEARBY    3*TILESIZE    //3 tiles
#define WIDE    6*TILESIZE    //8 tiles
#define NEAREST   0           //unlimited range
#define TILESIZE    128       //Size of one texture tile in egoboo

EXTERN bool_t                    localseeinvisible;
EXTERN bool_t                    localseekurse;
EXTERN Uint16                    localsenseenemies;
EXTERN IDSZ                      localsenseenemiesID;

//------------------------------------
// Enchantment variables
//------------------------------------
#define MAXENCHANT                      200         // Number of enchantments
EXTERN Uint16       numfreeenchant;             // For allocating new ones
EXTERN Uint16       freeenchant[MAXENCHANT];    //

EXTERN bool_t                    evevalid[MAXEVE];                       // Enchant.txt loaded?
EXTERN bool_t                    eveoverride[MAXEVE];                    // Override other enchants?
EXTERN bool_t                    everemoveoverridden[MAXEVE];            // Remove other enchants?
EXTERN bool_t                    evesetyesno[MAXEVE][MAXEVESETVALUE];    // Set this value?
EXTERN Uint8           evesetvalue[MAXEVE][MAXEVESETVALUE];    // Value to use
EXTERN Sint32               eveaddvalue[MAXEVE][MAXEVEADDVALUE];    // The values to add
EXTERN bool_t                    everetarget[MAXEVE];                    // Pick a weapon?
EXTERN bool_t                    evekillonend[MAXEVE];                   // Kill the target on end?
EXTERN bool_t                    evepoofonend[MAXEVE];                   // Spawn a poof on end?
EXTERN bool_t                    eveendifcantpay[MAXEVE];                // End on out of mana
EXTERN bool_t                    evestayifnoowner[MAXEVE];               // Stay if owner has died?
EXTERN Sint16            evetime[MAXEVE];                        // Time in seconds
EXTERN Sint32             eveendmessage[MAXEVE];                  // Message for end -1 for none
EXTERN Sint16            eveownermana[MAXEVE];                   // Boost values
EXTERN Sint16            eveownerlife[MAXEVE];                   //
EXTERN Sint16            evetargetmana[MAXEVE];                  //
EXTERN Sint16            evetargetlife[MAXEVE];                  //
EXTERN Uint8           evedontdamagetype[MAXEVE];              // Don't work if ...
EXTERN Uint8           eveonlydamagetype[MAXEVE];              // Only work if ...
EXTERN Uint32           everemovedbyidsz[MAXEVE];               // By particle or [NONE]
EXTERN Uint16           evecontspawntime[MAXEVE];               // Spawn timer
EXTERN Uint8           evecontspawnamount[MAXEVE];             // Spawn amount
EXTERN Uint16           evecontspawnfacingadd[MAXEVE];          // Spawn in circle
EXTERN Uint16           evecontspawnpip[MAXEVE];                // Spawn type ( local )
EXTERN Sint16           evewaveindex[MAXEVE];                   // Sound on end (-1 for none)
EXTERN Uint16           evefrequency[MAXEVE];                   // Sound frequency
EXTERN Uint8            eveoverlay[MAXEVE];                     // Spawn an overlay?
EXTERN Uint16           eveseekurse[MAXEVE];                     // Spawn an overlay?

EXTERN bool_t           encon[MAXENCHANT];                      // Enchantment on
EXTERN Uint16           enceve[MAXENCHANT];                     // The type
EXTERN Uint16           enctarget[MAXENCHANT];                  // Who it enchants
EXTERN Uint16           encnextenchant[MAXENCHANT];             // Next in the list
EXTERN Uint16           encowner[MAXENCHANT];                   // Who cast the enchant
EXTERN Uint16           encspawner[MAXENCHANT];                 // The spellbook character
EXTERN Uint16           encoverlay[MAXENCHANT];                 // The overlay character
EXTERN Sint16            encownermana[MAXENCHANT];               // Boost values
EXTERN Sint16            encownerlife[MAXENCHANT];               //
EXTERN Sint16            enctargetmana[MAXENCHANT];              //
EXTERN Sint16            enctargetlife[MAXENCHANT];              //
EXTERN bool_t                  encsetyesno[MAXENCHANT][MAXEVESETVALUE];// Was it set?
EXTERN bool_t           encsetsave[MAXENCHANT][MAXEVESETVALUE]; // The value to restore
EXTERN Sint16            encaddsave[MAXENCHANT][MAXEVEADDVALUE]; // The value to take away
EXTERN Sint16            enctime[MAXENCHANT];                    // Time before end
EXTERN Uint16           encspawntime[MAXENCHANT];               // Time before spawn

#define MISNORMAL               0                  // Treat missiles normally
#define MISDEFLECT              1                  // Deflect incoming missiles
#define MISREFLECT              2                  // Reflect them back!

#define LEAVEALL                0
#define LEAVEFIRST              1
#define LEAVENONE               2

#define SETDAMAGETYPE           0
#define SETNUMBEROFJUMPS        1
#define SETLIFEBARCOLOR         2
#define SETMANABARCOLOR         3
#define SETSLASHMODIFIER        4      // Damage modifiers
#define SETCRUSHMODIFIER        5
#define SETPOKEMODIFIER         6
#define SETHOLYMODIFIER         7
#define SETEVILMODIFIER         8
#define SETFIREMODIFIER         9
#define SETICEMODIFIER          10
#define SETZAPMODIFIER          11
#define SETFLASHINGAND          12
#define SETLIGHTBLEND           13
#define SETALPHABLEND           14
#define SETSHEEN                15      // Shinyness
#define SETFLYTOHEIGHT          16
#define SETWALKONWATER          17
#define SETCANSEEINVISIBLE      18
#define SETMISSILETREATMENT     19
#define SETCOSTFOREACHMISSILE   20
#define SETMORPH                21      // Morph character?
#define SETCHANNEL              22      // Can channel life as mana?

#define ADDJUMPPOWER            0
#define ADDBUMPDAMPEN           1
#define ADDBOUNCINESS           2
#define ADDDAMAGE               3
#define ADDSIZE                 4
#define ADDACCEL                5
#define ADDRED                  6      // Red shift
#define ADDGRN                  7      // Green shift
#define ADDBLU                  8      // Blue shift
#define ADDDEFENSE              9      // Defence adjustments
#define ADDMANA                 10
#define ADDLIFE                 11
#define ADDSTRENGTH             12
#define ADDWISDOM               13
#define ADDINTELLIGENCE         14
#define ADDDEXTERITY            15

EXTERN float                   textureoffset[256];         // For moving textures
EXTERN Uint16           dolist[MAXCHR];             // List of which characters to draw
EXTERN Uint16           numdolist;                  // How many in the list

//------------------------------------
// Particle variables
//------------------------------------
#define SPAWNNOCHARACTER        255                 // For particles that spawn characters...
#define TOTALMAXPRT             2048         // True max number of particles
EXTERN Uint16           maxparticles EQ(512);                  // max number of particles
#define TOTALMAXPRTPIP           1024                    // Particle templates
#define MAXPRTPIPPEROBJECT  13                       // Max part*.txt per object

EXTERN int                     numfreeprt EQ( 0 );                         // For allocation
EXTERN Uint16           freeprtlist[TOTALMAXPRT];                        //
EXTERN Uint8           prton[TOTALMAXPRT];                              // Does it exist?
EXTERN Uint16           prtpip[TOTALMAXPRT];                             // The part template
EXTERN Uint16           prtmodel[TOTALMAXPRT];                           // Pip spawn model
EXTERN Uint16           prtattachedtocharacter[TOTALMAXPRT];             // For torch flame
EXTERN Uint16           prtgrip[TOTALMAXPRT];                            // The vertex it's on
EXTERN Uint8           prttype[TOTALMAXPRT];                            // Transparency mode, 0-2
EXTERN Uint16           prtfacing[TOTALMAXPRT];                          // Direction of the part
EXTERN Uint8           prtteam[TOTALMAXPRT];                            // Team
EXTERN float                   prtxpos[TOTALMAXPRT];                            // Position
EXTERN float                   prtypos[TOTALMAXPRT];                            //
EXTERN float                   prtzpos[TOTALMAXPRT];                            //
EXTERN float                   prtxvel[TOTALMAXPRT];                            // Velocity
EXTERN float                   prtyvel[TOTALMAXPRT];                            //
EXTERN float                   prtzvel[TOTALMAXPRT];                            //
EXTERN float                   prtlevel[TOTALMAXPRT];                           // Height of tile
EXTERN Uint8           prtspawncharacterstate[TOTALMAXPRT];             //
EXTERN Uint16           prtrotate[TOTALMAXPRT];                          // Rotation direction
EXTERN Sint16            prtrotateadd[TOTALMAXPRT];                       // Rotation rate
EXTERN Uint32           prtonwhichfan[TOTALMAXPRT];                      // Where the part is
EXTERN Uint16           prtsize[TOTALMAXPRT];                            // Size of particle>>8
EXTERN Sint16            prtsizeadd[TOTALMAXPRT];                         // Change in size
EXTERN bool_t           prtinview[TOTALMAXPRT];                          // Render this one?
EXTERN Uint16           prtimage[TOTALMAXPRT];                           // Which image ( >> 8 )
EXTERN Uint16           prtimageadd[TOTALMAXPRT];                        // Animation rate
EXTERN Uint16           prtimagemax[TOTALMAXPRT];                        // End of image loop
EXTERN Uint16           prtimagestt[TOTALMAXPRT];                        // Start of image loop
EXTERN Uint8           prtlight[TOTALMAXPRT];                           // Light level
EXTERN Uint16           prttime[TOTALMAXPRT];                            // Duration of particle
EXTERN Uint16           prtspawntime[TOTALMAXPRT];                       // Time until spawn
EXTERN Uint8           prtbumpsize[TOTALMAXPRT];                        // Size of bumpers
EXTERN Uint8           prtbumpsizebig[TOTALMAXPRT];                     //
EXTERN Uint8           prtbumpheight[TOTALMAXPRT];                      // Bounding box height
EXTERN Uint16           prtbumpnext[TOTALMAXPRT];                        // Next particle on fanblock
EXTERN Uint16           prtdamagebase[TOTALMAXPRT];                      // For strength
EXTERN Uint16           prtdamagerand[TOTALMAXPRT];                      // For fixes...
EXTERN Uint8           prtdamagetype[TOTALMAXPRT];                      // Damage type
EXTERN Uint16           prtchr[TOTALMAXPRT];                             // The character that is attacking
EXTERN float                   prtdynalightfalloff[TOTALMAXPRT];                // Dyna light...
EXTERN float                   prtdynalightlevel[TOTALMAXPRT];                  //
EXTERN float                   particleimageu[MAXPARTICLEIMAGE][2];        // Texture coordinates
EXTERN float                   particleimagev[MAXPARTICLEIMAGE][2];        //
EXTERN Uint16           particletexture;                            // All in one bitmap
EXTERN bool_t           prtdynalighton[TOTALMAXPRT];                     // Dynamic light?
EXTERN Uint16           prttarget[TOTALMAXPRT];                          // Who it's chasing

//particle templates
EXTERN int                     numpip  EQ( 0 );
EXTERN bool_t                   pipforce[TOTALMAXPRTPIP];                        // Force spawn?
EXTERN Uint8                   piptype[TOTALMAXPRTPIP];                         // Transparency mode
EXTERN Uint8                   pipnumframes[TOTALMAXPRTPIP];                    // Number of frames
EXTERN Uint8                   pipimagebase[TOTALMAXPRTPIP];                    // Starting image
EXTERN Uint16                  pipimageadd[TOTALMAXPRTPIP];                     // Frame rate
EXTERN Uint16                  pipimageaddrand[TOTALMAXPRTPIP];                 // Frame rate randomness
EXTERN Uint16                  piptime[TOTALMAXPRTPIP];                         // Time until end
EXTERN Uint16                  piprotatebase[TOTALMAXPRTPIP];                   // Rotation
EXTERN Uint16                  piprotaterand[TOTALMAXPRTPIP];                   // Rotation
EXTERN Sint16                  piprotateadd[TOTALMAXPRTPIP];                    // Rotation rate
EXTERN Uint16                  pipsizebase[TOTALMAXPRTPIP];                     // Size
EXTERN Sint16                  pipsizeadd[TOTALMAXPRTPIP];                      // Size rate
EXTERN float                   pipspdlimit[TOTALMAXPRTPIP];                     // Speed limit
EXTERN float                   pipdampen[TOTALMAXPRTPIP];                       // Bounciness
EXTERN Sint8                   pipbumpmoney[TOTALMAXPRTPIP];                    // Value of particle
EXTERN Uint8                   pipbumpsize[TOTALMAXPRTPIP];                     // Bounding box size
EXTERN Uint8                   pipbumpheight[TOTALMAXPRTPIP];                   // Bounding box height
EXTERN bool_t                   pipendwater[TOTALMAXPRTPIP];                     // End if underwater
EXTERN bool_t                   pipendbump[TOTALMAXPRTPIP];                      // End if bumped
EXTERN bool_t                   pipendground[TOTALMAXPRTPIP];                    // End if on ground
EXTERN bool_t                   pipendwall[TOTALMAXPRTPIP];                      // End if hit a wall
EXTERN bool_t                   pipendlastframe[TOTALMAXPRTPIP];                 // End on last frame
EXTERN Uint16                  pipdamagebase[TOTALMAXPRTPIP];                   // Damage
EXTERN Uint16                  pipdamagerand[TOTALMAXPRTPIP];                   // Damage
EXTERN Uint8                   pipdamagetype[TOTALMAXPRTPIP];                   // Damage type
EXTERN Sint16                  pipfacingbase[TOTALMAXPRTPIP];                   // Facing
EXTERN Uint16                  pipfacingadd[TOTALMAXPRTPIP];                    // Facing
EXTERN Uint16                  pipfacingrand[TOTALMAXPRTPIP];                   // Facing
EXTERN Sint16                  pipxyspacingbase[TOTALMAXPRTPIP];                // Spacing
EXTERN Uint16                  pipxyspacingrand[TOTALMAXPRTPIP];                // Spacing
EXTERN Sint16                  pipzspacingbase[TOTALMAXPRTPIP];                 // Altitude
EXTERN Uint16                  pipzspacingrand[TOTALMAXPRTPIP];                 // Altitude
EXTERN Sint8                   pipxyvelbase[TOTALMAXPRTPIP];                    // Shot velocity
EXTERN Uint8                   pipxyvelrand[TOTALMAXPRTPIP];                    // Shot velocity
EXTERN Sint8                   pipzvelbase[TOTALMAXPRTPIP];                     // Up velocity
EXTERN Uint8                   pipzvelrand[TOTALMAXPRTPIP];                     // Up velocity
EXTERN Uint16                  pipcontspawntime[TOTALMAXPRTPIP];                // Spawn timer
EXTERN Uint8                   pipcontspawnamount[TOTALMAXPRTPIP];              // Spawn amount
EXTERN Uint16                  pipcontspawnfacingadd[TOTALMAXPRTPIP];           // Spawn in circle
EXTERN Uint16                  pipcontspawnpip[TOTALMAXPRTPIP];                 // Spawn type ( local )
EXTERN Uint8                   pipendspawnamount[TOTALMAXPRTPIP];               // Spawn amount
EXTERN Uint16                  pipendspawnfacingadd[TOTALMAXPRTPIP];            // Spawn in circle
EXTERN Uint8                   pipendspawnpip[TOTALMAXPRTPIP];                  // Spawn type ( local )
EXTERN Uint8                   pipbumpspawnamount[TOTALMAXPRTPIP];              // Spawn amount
EXTERN Uint8                   pipbumpspawnpip[TOTALMAXPRTPIP];                 // Spawn type ( global )
EXTERN Uint8                   pipdynalightmode[TOTALMAXPRTPIP];                // Dynamic light on?
EXTERN float                   pipdynalevel[TOTALMAXPRTPIP];                    // Intensity
EXTERN Uint16                  pipdynafalloff[TOTALMAXPRTPIP];                  // Falloff
EXTERN Uint16                  pipdazetime[TOTALMAXPRTPIP];                     // Daze
EXTERN Uint16                  pipgrogtime[TOTALMAXPRTPIP];                     // Drunkeness
EXTERN Sint8                   pipsoundspawn[TOTALMAXPRTPIP];                   // Beginning sound
EXTERN Sint8                   pipsoundend[TOTALMAXPRTPIP];                     // Ending sound
EXTERN Sint8                   pipsoundfloor[TOTALMAXPRTPIP];                   // Floor sound
EXTERN Sint8                   pipsoundwall[TOTALMAXPRTPIP];                    // Ricochet sound
EXTERN bool_t                   pipfriendlyfire[TOTALMAXPRTPIP];                 // Friendly fire
EXTERN bool_t                   piphateonly[TOTALMAXPRTPIP];          //Only hit hategroup
EXTERN bool_t                   piprotatetoface[TOTALMAXPRTPIP];                 // Arrows/Missiles
EXTERN bool_t                   pipnewtargetonspawn[TOTALMAXPRTPIP];             // Get new target?
EXTERN bool_t                   piphoming[TOTALMAXPRTPIP];                       // Homing?
EXTERN Uint16                  piptargetangle[TOTALMAXPRTPIP];                  // To find target
EXTERN float                   piphomingaccel[TOTALMAXPRTPIP];                  // Acceleration rate
EXTERN float                   piphomingfriction[TOTALMAXPRTPIP];               // Deceleration rate
EXTERN float                   pipdynalightleveladd[TOTALMAXPRTPIP];            // Dyna light changes
EXTERN float                   pipdynalightfalloffadd[TOTALMAXPRTPIP];          //
EXTERN bool_t                   piptargetcaster[TOTALMAXPRTPIP];                 // Target caster?
EXTERN bool_t                   pipspawnenchant[TOTALMAXPRTPIP];                 // Spawn enchant?
EXTERN bool_t                   pipneedtarget[TOTALMAXPRTPIP];                   // Need a target?
EXTERN bool_t                   piponlydamagefriendly[TOTALMAXPRTPIP];           // Only friends?
EXTERN bool_t                   pipstartontarget[TOTALMAXPRTPIP];                // Start on target?
EXTERN int                     pipzaimspd[TOTALMAXPRTPIP];                      // [ZSPD] For Z aiming
EXTERN Uint16                  pipdamfx[TOTALMAXPRTPIP];                        // Damage effects
EXTERN bool_t                  pipallowpush[TOTALMAXPRTPIP];                    // Allow particle to push characters around
EXTERN bool_t                  pipintdamagebonus[TOTALMAXPRTPIP];               // Add intelligence as damage bonus
EXTERN bool_t                  pipwisdamagebonus[TOTALMAXPRTPIP];               // Add wisdom as damage bonus

#define COIN1               0                       // Coins are the first particles loaded
#define COIN5               1                       //
#define COIN25              2                       //
#define COIN100             3                       //
#define WEATHER4            4                       // Weather particles
#define WEATHER5            5                       // Weather particle finish
#define SPLASH              6                       // Water effects are next
#define RIPPLE              7                       //
#define DEFEND              8                       // Defend particle

//------------------------------------
// Module variables
//------------------------------------
#define RANKSIZE 12
#define SUMMARYLINES 8
#define SUMMARYSIZE  80

EXTERN int                     globalnummodule;                            // Number of modules
EXTERN char                    modrank[MAXMODULE][RANKSIZE];               // Number of stars
EXTERN char                    modlongname[MAXMODULE][MAXCAPNAMESIZE];     // Module names
EXTERN char                    modloadname[MAXMODULE][MAXCAPNAMESIZE];     // Module load names
EXTERN Uint8           modimportamount[MAXMODULE];                 // # of import characters
EXTERN Uint8           modallowexport[MAXMODULE];                  // Export characters?
EXTERN Uint8           modminplayers[MAXMODULE];                   // Number of players
EXTERN Uint8           modmaxplayers[MAXMODULE];                   //
EXTERN bool_t          modmonstersonly[MAXMODULE];                 // Only allow monsters
EXTERN bool_t          modrtscontrol[MAXMODULE];                   // Real Time Stragedy?
EXTERN Uint8           modrespawnvalid[MAXMODULE];                 // Allow respawn
EXTERN int                     numlines;                                   // Lines in summary
EXTERN char                    modsummary[SUMMARYLINES][SUMMARYSIZE];      // Quest description

//------------------------------------
// Model stuff
//------------------------------------
// TEMPORARY: Needs to be moved out of egoboo.h eventually
extern struct Md2Model *md2_models[MAXMODEL];                 // Md2 models

EXTERN int             globalnumicon;                              // Number of icons
EXTERN Uint16          madloadframe;                               // Where to load next
EXTERN bool_t          madused[MAXMODEL];                          // Model slot
EXTERN char            madname[MAXMODEL][128];                     // Model name
EXTERN Uint16          madskintoicon[MAXTEXTURE];                  // Skin to icon
EXTERN Uint16          madskins[MAXMODEL];                         // Number of skins
EXTERN Uint16          madskinstart[MAXMODEL];                     // Starting skin of model
EXTERN Uint16          madframes[MAXMODEL];                        // Number of frames
EXTERN Uint16          madframestart[MAXMODEL];                    // Starting frame of model
EXTERN Uint16          madmsgstart[MAXMODEL];                      // The first message
EXTERN Uint16          madvertices[MAXMODEL];                      // Number of vertices
EXTERN Uint16          madtransvertices[MAXMODEL];                 // Number to transform
EXTERN Uint16          madcommands[MAXMODEL];                      // Number of commands
EXTERN float           madscale[MAXMODEL];                         // Multiply by value
EXTERN GLenum          madcommandtype[MAXMODEL][MAXCOMMAND];       // Fan or strip
EXTERN Uint16          madcommandsize[MAXMODEL][MAXCOMMAND];       // Entries used by command
EXTERN Uint16          madcommandvrt[MAXMODEL][MAXCOMMANDENTRIES]; // Which vertex
EXTERN float           madcommandu[MAXMODEL][MAXCOMMANDENTRIES];   // Texture position
EXTERN float           madcommandv[MAXMODEL][MAXCOMMANDENTRIES];   //
EXTERN Sint16          madvrtx[MAXFRAME][MAXVERTICES];             // Vertex position
EXTERN Sint16          madvrty[MAXFRAME][MAXVERTICES];             //
EXTERN Sint16          madvrtz[MAXFRAME][MAXVERTICES];             //
EXTERN Uint8           madvrta[MAXFRAME][MAXVERTICES];             // Light index of vertex
EXTERN Uint8           madframelip[MAXFRAME];                      // 0-15, How far into action is each frame
EXTERN Uint16          madframefx[MAXFRAME];                       // Invincibility, Spawning
EXTERN Uint16          madframeliptowalkframe[MAXMODEL][4][16];    // For walk animations
EXTERN Uint16          madai[MAXMODEL];                            // AI for each model
EXTERN Uint8           madactionvalid[MAXMODEL][MAXACTION];        // bfalse if not valid
EXTERN Uint16          madactionstart[MAXMODEL][MAXACTION];        // First frame of anim
EXTERN Uint16          madactionend[MAXMODEL][MAXACTION];          // One past last frame
EXTERN Uint16          madprtpip[MAXMODEL][MAXPRTPIPPEROBJECT];    // Local particles

// Character profiles
EXTERN int                   importobject;
EXTERN short                 capimportslot[MAXMODEL];
EXTERN char                  capclassname[MAXMODEL][MAXCAPNAMESIZE];     // Class name
EXTERN char                  capskinname[MAXMODEL][4][MAXCAPNAMESIZE];   // Skin name
EXTERN Sint8           capskinoverride[MAXMODEL];                  // -1 or 0-3.. For import
EXTERN Uint8           capleveloverride[MAXMODEL];                 // 0 for normal
EXTERN int                   capstateoverride[MAXMODEL];                 // 0 for normal
EXTERN int                   capcontentoverride[MAXMODEL];               // 0 for normal
EXTERN Uint16         capskincost[MAXMODEL][4];                   // Store prices
EXTERN Uint8           capskindressy[MAXMODEL];                    // Dressy
EXTERN float                 capstrengthdampen[MAXMODEL];                // Strength damage factor
EXTERN Uint8           capstoppedby[MAXMODEL];                     // Collision Mask
EXTERN bool_t           capuniformlit[MAXMODEL];                    // Bad lighting?
EXTERN Uint8           caplifecolor[MAXMODEL];                     // Bar colors
EXTERN Uint8           capmanacolor[MAXMODEL];                     //
EXTERN Uint8           capammomax[MAXMODEL];                       // Ammo stuff
EXTERN Uint8           capammo[MAXMODEL];                          //
EXTERN Uint8           capgender[MAXMODEL];                        // Gender
EXTERN Uint16         caplifebase[MAXMODEL];                      // Life
EXTERN Uint16         capliferand[MAXMODEL];                      //
EXTERN Uint16         caplifeperlevelbase[MAXMODEL];              //
EXTERN Uint16         caplifeperlevelrand[MAXMODEL];              //
EXTERN Sint16          caplifereturn[MAXMODEL];                    //
EXTERN Sint16          capmoney[MAXMODEL];                         // Money
EXTERN Uint16         caplifeheal[MAXMODEL];                      //
EXTERN Uint16         capmanabase[MAXMODEL];                      // Mana
EXTERN Uint16         capmanarand[MAXMODEL];                      //
EXTERN Sint16          capmanacost[MAXMODEL];                      //
EXTERN Uint16         capmanaperlevelbase[MAXMODEL];              //
EXTERN Uint16         capmanaperlevelrand[MAXMODEL];              //
EXTERN Uint16         capmanareturnbase[MAXMODEL];                //
EXTERN Uint16         capmanareturnrand[MAXMODEL];                //
EXTERN Uint16         capmanareturnperlevelbase[MAXMODEL];        //
EXTERN Uint16         capmanareturnperlevelrand[MAXMODEL];        //
EXTERN Uint16         capmanaflowbase[MAXMODEL];                  //
EXTERN Uint16         capmanaflowrand[MAXMODEL];                  //
EXTERN Uint16         capmanaflowperlevelbase[MAXMODEL];          //
EXTERN Uint16         capmanaflowperlevelrand[MAXMODEL];          //
EXTERN Uint16         capstrengthbase[MAXMODEL];                  // Strength
EXTERN Uint16         capstrengthrand[MAXMODEL];                  //
EXTERN Uint16         capstrengthperlevelbase[MAXMODEL];          //
EXTERN Uint16         capstrengthperlevelrand[MAXMODEL];          //
EXTERN Uint16         capwisdombase[MAXMODEL];                    // Wisdom
EXTERN Uint16         capwisdomrand[MAXMODEL];                    //
EXTERN Uint16         capwisdomperlevelbase[MAXMODEL];            //
EXTERN Uint16         capwisdomperlevelrand[MAXMODEL];            //
EXTERN Uint16         capintelligencebase[MAXMODEL];              // Intlligence
EXTERN Uint16         capintelligencerand[MAXMODEL];              //
EXTERN Uint16         capintelligenceperlevelbase[MAXMODEL];      //
EXTERN Uint16         capintelligenceperlevelrand[MAXMODEL];      //
EXTERN Uint16         capdexteritybase[MAXMODEL];                 // Dexterity
EXTERN Uint16         capdexterityrand[MAXMODEL];                 //
EXTERN Uint16         capdexterityperlevelbase[MAXMODEL];         //
EXTERN Uint16         capdexterityperlevelrand[MAXMODEL];         //
EXTERN float                 capsize[MAXMODEL];                          // Scale of model
EXTERN float                 capsizeperlevel[MAXMODEL];                  // Scale increases
EXTERN float                 capdampen[MAXMODEL];                        // Bounciness
EXTERN Uint8           capshadowsize[MAXMODEL];                    // Shadow size
EXTERN Uint8           capbumpsize[MAXMODEL];                      // Bounding octagon
EXTERN Uint8           capbumpsizebig[MAXMODEL];                   // For octagonal bumpers
EXTERN Uint8           capbumpheight[MAXMODEL];                    //
EXTERN float                 capbumpdampen[MAXMODEL];                    // Mass
EXTERN Uint8           capweight[MAXMODEL];                        // Weight
EXTERN float                 capjump[MAXMODEL];                          // Jump power
EXTERN Uint8           capjumpnumber[MAXMODEL];                    // Number of jumps ( Ninja )
EXTERN Uint8           capsneakspd[MAXMODEL];                      // Sneak threshold
EXTERN Uint8           capwalkspd[MAXMODEL];                       // Walk threshold
EXTERN Uint8           caprunspd[MAXMODEL];                        // Run threshold
EXTERN Uint8           capflyheight[MAXMODEL];                     // Fly height
EXTERN Uint8           capflashand[MAXMODEL];                      // Flashing rate
EXTERN Uint8           capalpha[MAXMODEL];                         // Transparency
EXTERN Uint8           caplight[MAXMODEL];                         // Light blending
EXTERN bool_t           captransferblend[MAXMODEL];                 // Transfer blending to rider/weapons
EXTERN bool_t           capsheen[MAXMODEL];                         // How shiny it is ( 0-15 )
EXTERN bool_t           capenviro[MAXMODEL];                        // Phong map this baby?
EXTERN Uint16         capuoffvel[MAXMODEL];                       // Texture movement rates
EXTERN Uint16         capvoffvel[MAXMODEL];                       //
EXTERN bool_t           capstickybutt[MAXMODEL];                    // Stick to the ground?
EXTERN Uint16         capiframefacing[MAXMODEL];                  // Invincibility frame
EXTERN Uint16         capiframeangle[MAXMODEL];                   //
EXTERN Uint16         capnframefacing[MAXMODEL];                  // Normal frame
EXTERN Uint16         capnframeangle[MAXMODEL];                   //
EXTERN Uint8           capresistbumpspawn[MAXMODEL];               // Don't catch fire
EXTERN Uint8           capdefense[MAXMODEL][4];                    // Defense for each skin
EXTERN Uint8           capdamagemodifier[MAXMODEL][MAXDAMAGETYPE][4];
EXTERN float                 capmaxaccel[MAXMODEL][4];                   // Acceleration for each skin
EXTERN Uint32           capexperienceforlevel[MAXMODEL][MAXLEVEL];  // Experience needed for next level
EXTERN Uint32           capexperiencebase[MAXMODEL];                // Starting experience
EXTERN Uint16         capexperiencerand[MAXMODEL];                //
EXTERN Uint16         capexperienceworth[MAXMODEL];               // Amount given to killer/user
EXTERN float                 capexperienceexchange[MAXMODEL];            // Adds to worth
EXTERN float                 capexperiencerate[MAXMODEL][MAXEXPERIENCETYPE];
EXTERN IDSZ           capidsz[MAXMODEL][MAXIDSZ];                 // ID strings
EXTERN bool_t                capisitem[MAXMODEL];                        // Is it an item?
EXTERN bool_t                capinvictus[MAXMODEL];                      // Is it invincible?
EXTERN bool_t                capismount[MAXMODEL];                       // Can you ride it?
EXTERN bool_t                capisstackable[MAXMODEL];                   // Is it arrowlike?
EXTERN bool_t                capnameknown[MAXMODEL];                     // Is the class name known?
EXTERN bool_t                capusageknown[MAXMODEL];                    // Is its usage known
EXTERN bool_t                capcancarrytonextmodule[MAXMODEL];          // Take it with you?
EXTERN bool_t                capneedskillidtouse[MAXMODEL];              // Check IDSZ first?
EXTERN bool_t                capwaterwalk[MAXMODEL];                     // Walk on water?
EXTERN bool_t                capplatform[MAXMODEL];                      // Can be stood on?
EXTERN bool_t                capcanuseplatforms[MAXMODEL];               // Can use platforms?
EXTERN bool_t                capcangrabmoney[MAXMODEL];                  // Collect money?
EXTERN bool_t                capcanopenstuff[MAXMODEL];                  // Open chests/doors?
EXTERN bool_t                capicon[MAXMODEL];                          // Draw icon
EXTERN bool_t                capforceshadow[MAXMODEL];                   // Draw a shadow?
EXTERN bool_t                capripple[MAXMODEL];                        // Spawn ripples?
EXTERN Uint8           capdamagetargettype[MAXMODEL];              // For AI DamageTarget
EXTERN Uint8           capweaponaction[MAXMODEL];                  // Animation needed to swing
EXTERN bool_t           capgripvalid[MAXMODEL][2];                  // Left/Right hands valid
EXTERN Uint8           capattackattached[MAXMODEL];                //
EXTERN Sint8           capattackprttype[MAXMODEL];                 //
EXTERN Uint8           capattachedprtamount[MAXMODEL];             // Sticky particles
EXTERN Uint8           capattachedprtreaffirmdamagetype[MAXMODEL]; // Relight that torch...
EXTERN Uint16           capattachedprttype[MAXMODEL];               //
EXTERN Uint8           capgopoofprtamount[MAXMODEL];               // Poof effect
EXTERN Sint16          capgopoofprtfacingadd[MAXMODEL];            //
EXTERN Uint16           capgopoofprttype[MAXMODEL];                 //
EXTERN bool_t           capbloodvalid[MAXMODEL];                    // Blood ( yuck )
EXTERN Uint8           capbloodprttype[MAXMODEL];                  //
EXTERN Sint8           capwavefootfall[MAXMODEL];                  // Footfall sound, -1
EXTERN Sint8           capwavejump[MAXMODEL];                      // Jump sound, -1
EXTERN bool_t           capridercanattack[MAXMODEL];                // Rider attack?
EXTERN bool_t                capcanbedazed[MAXMODEL];                    // Can it be dazed?
EXTERN bool_t                capcanbegrogged[MAXMODEL];                  // Can it be grogged?
EXTERN Uint8           capkursechance[MAXMODEL];                   // Chance of being kursed
EXTERN bool_t           capistoobig[MAXMODEL];                      // Can't be put in pack
EXTERN bool_t           capreflect[MAXMODEL];                       // Draw the reflection
EXTERN bool_t           capalwaysdraw[MAXMODEL];                    // Always render
EXTERN bool_t           capisranged[MAXMODEL];                      // Flag for ranged weapon
EXTERN Sint8           caphidestate[MAXCHR];                       // Don't draw when...
EXTERN bool_t           capisequipment[MAXCHR];                     // Behave in silly ways
EXTERN Sint8           capisvaluable[MAXCHR];                     // Force to be valuable

//skill system
EXTERN Sint8                 capshieldproficiency[MAXMODEL];               // Can it use shields?
EXTERN bool_t                capcanjoust[MAXMODEL];            // Can it use advanced weapons?
EXTERN bool_t                capcanuseadvancedweapons[MAXMODEL];         // Can it use advanced weapons?
EXTERN bool_t                capcanseeinvisible[MAXMODEL];               // Can it see invisible?
EXTERN bool_t                capcanseekurse[MAXMODEL];                   // Can it see kurses?
EXTERN bool_t          capcanusedivine[MAXMODEL];
EXTERN bool_t        capcanusearcane[MAXMODEL];
EXTERN bool_t        capcanusetech[MAXMODEL];
EXTERN bool_t        capcandisarm[MAXMODEL];
EXTERN bool_t          capcanbackstab[MAXMODEL];
EXTERN bool_t        capcanusepoison[MAXMODEL];
EXTERN bool_t        capcanread[MAXMODEL];

// Network stuff
#define NETREFRESH          1000                    // Every second
#define NONETWORK           numservice              //
#define MAXSERVICE 16
#define NETNAMESIZE 16
#define MAXSESSION 16
#define MAXNETPLAYER 8

EXTERN Uint32                   randsave;                  // Used in network timer
EXTERN int                     networkservice;
EXTERN char                    nethostname[64];                            // Name for hosting session
EXTERN char                    netmessagename[64];                         // Name for messages
EXTERN int                     numservice  EQ( 0 );                           // How many we found
EXTERN char                    netservicename[MAXSERVICE][NETNAMESIZE];    // Names of services
EXTERN int                     numsession  EQ( 0 );                           // How many we found
EXTERN char                    netsessionname[MAXSESSION][NETNAMESIZE];    // Names of sessions
EXTERN int                     numplayer  EQ( 0 );                            // How many we found
EXTERN char                    netplayername[MAXNETPLAYER][NETNAMESIZE];   // Names of machines

EXTERN char *parse_filename  EQ( NULL );  // For debuggin' goto_colon
EXTERN char *globalparsename  EQ( NULL ); // The SCRIPT.TXT filename
EXTERN FILE *globalnetworkerr  EQ( NULL ); // For debuggin' network

EXTERN float           indextoenvirox[MD2LIGHTINDICES];                    // Environment map
EXTERN float           lighttoenviroy[256];                                // Environment map
EXTERN Uint32           lighttospek[MAXSPEKLEVEL][256];                     //

EXTERN float           hillslide  EQ( 1.00f );                                 //
EXTERN float           slippyfriction  EQ( 1.00f );  //1.05f for Chevron          // Friction
EXTERN float           airfriction  EQ( 0.95f );                                //
EXTERN float           waterfriction  EQ( 0.85f );                              //
EXTERN float           noslipfriction  EQ( 0.95f );                            //
EXTERN float           platstick  EQ( 0.040f );                                 //
EXTERN float           gravity  EQ( ( float ) - 1.0f );                        // Gravitational accel
EXTERN int             damagetileamount  EQ( 256 );                           // Amount of damage
EXTERN Uint8           damagetiletype  EQ( DAMAGEFIRE );                      // Type of damage

EXTERN char            cFrameName[16];                                     // MD2 Frame Name

EXTERN Uint16          globesttarget;                                      // For find_target
EXTERN Uint16          globestangle;                                       //
EXTERN Uint16          glouseangle;                                        //
EXTERN int             globestdistance;

EXTERN Uint32          numfanblock  EQ( 0 );                                  // Number of collision areas
EXTERN Uint16          meshbumplistchr[MAXMESHFAN/16];                     // For character collisions
EXTERN Uint16          meshbumplistchrnum[MAXMESHFAN/16];                  // Number on the block
EXTERN Uint16          meshbumplistprt[MAXMESHFAN/16];                     // For particle collisions
EXTERN Uint16          meshbumplistprtnum[MAXMESHFAN/16];                  // Number on the block
EXTERN Uint8           meshexploremode  EQ( bfalse );                          // Explore mode?
EXTERN int             maxtotalmeshvertices  EQ( 256*256*6 );                 // of vertices
EXTERN int             meshsizex;                                          // Size in fansquares
EXTERN int             meshsizey;                                          //
EXTERN float           meshedgex;                                          // Limits !!!BAD!!!
EXTERN float           meshedgey;                                          //
EXTERN Uint16          meshlasttexture;                                    // Last texture used
EXTERN Uint8           meshtype[MAXMESHFAN];                               // Command type
EXTERN Uint8           meshfx[MAXMESHFAN];                                 // Special effects flags
EXTERN Uint8           meshtwist[MAXMESHFAN];                              //
EXTERN bool_t          meshinrenderlist[MAXMESHFAN];                       //
EXTERN Uint16          meshtile[MAXMESHFAN];                               // Get texture from this
EXTERN Uint32          meshvrtstart[MAXMESHFAN];                           // Which vertex to start at
EXTERN Uint32          meshblockstart[( MAXMESHSIZEY/4 )+1];
EXTERN Uint32          meshfanstart[MAXMESHSIZEY];                         // Which fan to start a row with
EXTERN float*          floatmemory;                                                 // For malloc
EXTERN float*          meshvrtx;                                           // Vertex position
EXTERN float*          meshvrty;                                           //
EXTERN float*          meshvrtz;                                           // Vertex elevation
EXTERN Uint8*          meshvrta;                                           // Vertex base light
EXTERN Uint8*          meshvrtl;                                           // Vertex light
EXTERN Uint8           meshcommands[MAXMESHTYPE];                          // Number of commands
EXTERN Uint8           meshcommandsize[MAXMESHTYPE][MAXMESHCOMMAND];       // Entries in each command
EXTERN Uint16          meshcommandvrt[MAXMESHTYPE][MAXMESHCOMMANDENTRIES]; // Fansquare vertex list
EXTERN Uint8           meshcommandnumvertices[MAXMESHTYPE];                // Number of vertices
EXTERN float           meshcommandu[MAXMESHTYPE][MAXMESHVERTICES];         // Vertex texture posi
EXTERN float           meshcommandv[MAXMESHTYPE][MAXMESHVERTICES];         //
EXTERN float           meshtileoffu[MAXTILETYPE];                          // Tile texture offset
EXTERN float           meshtileoffv[MAXTILETYPE];                          //
EXTERN int             nummeshrenderlist;                                  // Number to render, total
EXTERN int             nummeshrenderlistref;                               // ..., reflective
EXTERN int             nummeshrenderlistsha;                               // ..., shadow
EXTERN Uint32          meshrenderlist[MAXMESHRENDER];                      // List of which to render, total
EXTERN Uint32          meshrenderlistref[MAXMESHRENDER];                   // ..., reflective
EXTERN Uint32          meshrenderlistsha[MAXMESHRENDER];                   // ..., shadow

EXTERN Uint8 asciitofont[256];                                   // Conversion table

// Display messages
EXTERN Uint16          msgtimechange EQ( 0 );                                  //
EXTERN Uint16          msgstart EQ( 0 );                                       // The message queue
EXTERN Sint16          msgtime[MAXMESSAGE];                                //
EXTERN char            msgtextdisplay[MAXMESSAGE][MESSAGESIZE];            // The displayed text
// Message files
EXTERN Uint16          msgtotal EQ( 0 );                                       // The number of messages
EXTERN Uint32          msgtotalindex EQ( 0 );                                  // Where to put letter
EXTERN Uint32          msgindex[MAXTOTALMESSAGE];                          // Where it is
EXTERN char            msgtext[MESSAGEBUFFERSIZE];                         // The text buffer

// My lil' random number table
#define MAXRAND 4096
EXTERN Uint16  randie[MAXRAND];
EXTERN Uint16  randindex;
#define RANDIE randie[randindex];  randindex=(randindex+1)&(MAXRAND-1)

#define MAXENDTEXT 1024
EXTERN char generictext[80];         // Use for whatever purpose
EXTERN char endtext[MAXENDTEXT];     // The end-module text
EXTERN int  endtextwrite;

// This is for random naming
EXTERN Uint16           numchop  EQ( 0 );              // The number of name parts
EXTERN Uint32           chopwrite  EQ( 0 );            // The data pointer
EXTERN char             chopdata[CHOPDATACHUNK];    // The name parts
EXTERN Uint16           chopstart[MAXCHOP];         // The first character of each part
EXTERN Uint16           capsectionsize[MAXMODEL][MAXSECTION];   // Number of choices, 0
EXTERN Uint16           capsectionstart[MAXMODEL][MAXSECTION];  //
EXTERN char             namingnames[MAXCAPNAMESIZE];// The name returned by the function

// These are for FindPath function only
#define   MOVE_MELEE  300
#define   MOVE_RANGED  600
#define   MOVE_DISTANCE 175
#define   MOVE_RETREAT  900
#define   MOVE_CHARGE  111
#define   MOVE_FOLLOW  0

// These are for the AI script loading/parsing routines
extern int                     iNumAis;

// Character AI alerts
#define ALERTIFSPAWNED                      1 << 0          // 0
#define ALERTIFHITVULNERABLE                1 << 1          // 1
#define ALERTIFATWAYPOINT                   1 << 2           // 2
#define ALERTIFATLASTWAYPOINT               1 << 3           // 3
#define ALERTIFATTACKED                     1 << 4          // 4
#define ALERTIFBUMPED                       1 << 5          // 5
#define ALERTIFORDERED                      1 << 6          // 6
#define ALERTIFCALLEDFORHELP                1 << 7         // 7
#define ALERTIFKILLED                       1 << 8         // 8
#define ALERTIFTARGETKILLED                 1 << 9         // 9
#define ALERTIFDROPPED                      1 << 10        // 10
#define ALERTIFGRABBED                      1 << 11        // 11
#define ALERTIFREAFFIRMED                   1 << 12        // 12
#define ALERTIFLEADERKILLED                 1 << 13        // 13
#define ALERTIFUSED                         1 << 14       // 14
#define ALERTIFCLEANEDUP                    1 << 15       // 15
#define ALERTIFSCOREDAHIT                   1 << 16       // 16
#define ALERTIFHEALED                       1 << 17      // 17
#define ALERTIFDISAFFIRMED                  1 << 18      // 18
#define ALERTIFCHANGED                      1 << 19      // 19
#define ALERTIFINWATER                      1 << 20     // 20
#define ALERTIFBORED                        1 << 21     // 21
#define ALERTIFTOOMUCHBAGGAGE               1 << 22     // 22
#define ALERTIFGROGGED                      1 << 23     // 23
#define ALERTIFDAZED                        1 << 24    // 24
#define ALERTIFHITGROUND                    1 << 25    // 25
#define ALERTIFNOTDROPPED                   1 << 26    // 26
#define ALERTIFBLOCKED                      1 << 27   // 27
#define ALERTIFTHROWN                       1 << 28   // 28
#define ALERTIFCRUSHED                      1 << 29   // 29
#define ALERTIFNOTPUTAWAY                   1 << 30  // 30
#define ALERTIFTAKENOUT                     1 << 31 // 31

EXTERN int     valuetmpx EQ( 0 );
EXTERN int     valuetmpy EQ( 0 );
EXTERN Uint16  valuetmpturn EQ( 0 );
EXTERN int     valuetmpdistance EQ( 0 );
EXTERN int     valuetmpargument EQ( 0 );
EXTERN Uint32  valuelastindent EQ( 0 );
EXTERN Uint16  valueoldtarget EQ( 0 );
EXTERN int     valueoperationsum EQ( 0 );
EXTERN bool_t  valuegopoof EQ( bfalse );

enum e_mix_type { MIX_UNKNOWN = 0, MIX_MUS, MIX_SND };
typedef enum e_mix_type mix_type_t;

struct s_mix_ptr
{
    mix_type_t type;

    union
    {
        void      * unk;
        Mix_Music * mus;
        Mix_Chunk * snd;
    } ptr;
};
typedef struct s_mix_ptr mix_ptr_t;

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

// For damage/stat pair reads/writes
EXTERN int pairbase, pairrand;
EXTERN float pairfrom, pairto;

// Passages
EXTERN int numpassage;              // Number of passages in the module
EXTERN int passtlx[MAXPASS];          // Passage positions
EXTERN int passtly[MAXPASS];
EXTERN int passbrx[MAXPASS];
EXTERN int passbry[MAXPASS];
EXTERN int passagemusic[MAXPASS];        // Music track appointed to the specific passage
EXTERN Uint8 passmask[MAXPASS];
EXTERN Uint8 passopen[MAXPASS];      // Is the passage open?
#define CLOSETOLERANCE      2                       // For closing doors

// For shops
EXTERN int numshoppassage;
EXTERN Uint16  shoppassage[MAXPASS];  // The passage number
EXTERN Uint16  shopowner[MAXPASS];    // Who gets the gold?
#define NOOWNER 65535        // Shop has no owner
#define STOLEN  65535        // Someone stole a item
#define BUY  0
#define SELL 1

// Status displays
EXTERN int numstat  EQ( 0 );
EXTERN Uint16  statlist[MAXSTAT];
EXTERN int statdelay  EQ( 25 );

// Particle
#define DYNAFANS  12

// Network Stuff
#define SHORTLATCH 1024.0f
#define CHARVEL 5.0f
#define MAXSENDSIZE 8192
#define COPYSIZE    4096
#define TOTALSIZE   2097152
#define MAXPLAYER   8                               // 2 to a power...  2^3
#define INPUTNONE   0                               //
#define INPUTMOUSE  1                               // Input devices
#define INPUTKEY    2                               //
#define INPUTJOYA   4                               //
#define INPUTJOYB   8                               //
#define MAXLAG      64                              //
#define LAGAND      63                              //
#define STARTTALK   10                              //
EXTERN bool_t plavalid[MAXPLAYER];                    // Player used?
EXTERN Uint16           plaindex[MAXPLAYER];                    // Which character?
EXTERN float                   plalatchx[MAXPLAYER];                   // Local latches
EXTERN float                   plalatchy[MAXPLAYER];                   //
EXTERN Uint8 plalatchbutton[MAXPLAYER];              //
EXTERN Uint32 numplatimes;
EXTERN float                   platimelatchx[MAXPLAYER][MAXLAG];       // Timed latches
EXTERN float                   platimelatchy[MAXPLAYER][MAXLAG];       //
EXTERN Uint8 platimelatchbutton[MAXPLAYER][MAXLAG];  //
EXTERN Uint8 pladevice[MAXPLAYER];                   // Input device
EXTERN int                     numpla;                                 // Number of players
EXTERN int                     numlocalpla;                            //
EXTERN int                     lag  EQ( 3 );                              // Lag tolerance
EXTERN int                     numfile;                                // For network copy
EXTERN int                     numfilesent;                            // For network copy
EXTERN int                     numfileexpected;                        // For network copy
EXTERN int                     numplayerrespond;                       //

#define TO_ANY_TEXT         25935                               // Message headers
#define TO_HOST_MODULEOK    14951                               //
#define TO_HOST_LATCH       33911                               //
#define TO_HOST_RTS         30376                               //
#define TO_HOST_IM_LOADED   40192                               //
#define TO_HOST_FILE        20482                               //
#define TO_HOST_DIR         49230                               //
#define TO_HOST_FILESENT    13131                               //
#define TO_REMOTE_MODULE    56025                               //
#define TO_REMOTE_LATCH     12715                               //
#define TO_REMOTE_FILE      62198                               //
#define TO_REMOTE_DIR       11034                               //
#define TO_REMOTE_RTS        5143                               //
#define TO_REMOTE_START     51390                               //
#define TO_REMOTE_FILESENT  19903                               //
extern Uint32  nexttimestamp;                // Expected timestamp

typedef enum global_sounds
{
    SND_GETCOIN = 0,
    SND_DEFEND,
    SND_WEATHER1,
    SND_WEATHER2,
    SND_COINFALL,
    SND_LEVELUP,
    SND_PITFALL,
    MAXGLOBALSOUNDS
} SND_GLOBAL;

// Sound using SDL_Mixer
EXTERN bool_t          mixeron EQ( bfalse );    // Is the SDL_Mixer loaded?
#define MAXWAVE         30               // Up to 30 wave/ogg per model
#define VOLUMERATIO     7               // Volume ratio
EXTERN mix_ptr_t    capwaveindex[MAXMODEL][MAXWAVE];    // sounds in a object
EXTERN Uint16       maxsoundchannel;      // Max number of sounds playing at the same time
EXTERN Uint16       buffersize;          // Buffer size set in setup.txt
EXTERN mix_ptr_t    globalwave[MAXGLOBALSOUNDS];      // All sounds loaded into memory
EXTERN bool_t       soundvalid;          // Allow playing of sound?
EXTERN Uint8        soundvolume;        // Volume of sounds played
EXTERN Sint16      channel;          // Which channel the current sound is using
EXTERN bool_t      listening EQ( bfalse );  // Playrers with listen skill?

// Music using SDL_Mixer
#define MAXPLAYLISTLENGTH 25            // Max number of different tracks loaded into memory
EXTERN bool_t      musicvalid;          // Allow music and loops?
EXTERN Uint8       musicvolume;        // The sound volume of music
EXTERN bool_t      musicinmemory EQ( bfalse );  // Is the music loaded in memory?
EXTERN mix_ptr_t   musictracksloaded[MAXPLAYLISTLENGTH];  // This is a specific music file loaded into memory
EXTERN Sint8       songplaying EQ( -1 );      // Current song that is playing

// Some various other stuff
EXTERN char valueidsz[5];
EXTERN Uint8 changed;
#define IDSZ_NONE            Make_IDSZ("NONE")       // [NONE]

// Key/Control input defenitions
#define MAXTAG              128                     // Number of tags in scancode.txt
#define TAGSIZE             32                      // Size of each tag
EXTERN int numscantag;
EXTERN char tagname[MAXTAG][TAGSIZE];                      // Scancode names
EXTERN Sint32 tagvalue[MAXTAG];                     // Scancode values
#define MAXCONTROL          64
EXTERN Uint32 controlvalue[MAXCONTROL];             // The scancode or mask
EXTERN Uint32 controliskey[MAXCONTROL];             // Is it a key?
#define KEY_INVALID     255
#define KEY_JUMP            0
#define KEY_LEFT_USE        1
#define KEY_LEFT_GET        2
#define KEY_LEFT_PACK       3
#define KEY_RIGHT_USE       4
#define KEY_RIGHT_GET       5
#define KEY_RIGHT_PACK      6
#define KEY_MESSAGE         7
#define KEY_CAMERA_LEFT     8
#define KEY_CAMERA_RIGHT    9
#define KEY_CAMERA_IN       10
#define KEY_CAMERA_OUT      11
#define KEY_UP              12
#define KEY_DOWN            13
#define KEY_LEFT            14
#define KEY_RIGHT           15
#define MOS_JUMP            16
#define MOS_LEFT_USE        17
#define MOS_LEFT_GET        18
#define MOS_LEFT_PACK       19
#define MOS_RIGHT_USE       20
#define MOS_RIGHT_GET       21
#define MOS_RIGHT_PACK      22
#define MOS_CAMERA          23
#define JOA_JUMP            24
#define JOA_LEFT_USE        25
#define JOA_LEFT_GET        26
#define JOA_LEFT_PACK       27
#define JOA_RIGHT_USE       28
#define JOA_RIGHT_GET       29
#define JOA_RIGHT_PACK      30
#define JOA_CAMERA          31
#define JOB_JUMP            32
#define JOB_LEFT_USE        33
#define JOB_LEFT_GET        34
#define JOB_LEFT_PACK       35
#define JOB_RIGHT_USE       36
#define JOB_RIGHT_GET       37
#define JOB_RIGHT_PACK      38
#define JOB_CAMERA          39

//Quest system
#define QUESTBEATEN         -1
#define NOQUEST             -2

// SDL specific declarations
EXTERN SDL_Joystick *sdljoya EQ( NULL );
EXTERN SDL_Joystick *sdljoyb EQ( NULL );
EXTERN Uint8  *sdlkeybuffer;
#define SDLKEYDOWN(k) sdlkeybuffer[k]

#define  _EGOBOO_H_
