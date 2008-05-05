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
#define	_EGOBOO_H_

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
#include "gltexture.h"	/* OpenGL texture loader */
#include "mathstuff.h"	/* vector and matrix math */
#include "configfile.h"
#include "Md2.h"

typedef struct {
	GLfloat x,y,z,w;
	GLfloat r,g,b,a;
	Uint32	color; // should replace r,g,b,a and be called by glColor4ubv
	GLfloat s,t;	// u and v in D3D I guess
} GLVERTEX;

// The following magic allows this include to work in multiple files
#ifdef DECLARE_GLOBALS
#define EXTERN
#define EQ(x) =x;
#else
#define EXTERN extern
#define EQ(x)
#endif

EXTERN char					VERSION[] EQ("2.6.0");  // Version of the game
#define PI                  3.1415929               //
EXTERN bool_t				gamepaused EQ(bfalse);	//Is the game paused?

#define NETREFRESH          1000                    // Every second
#define NONETWORK           numservice              //
#define MAXMODULE           100                     // Number of modules
#define TITLESIZE           128                     // Size of a title image
#define MAXSEQUENCE         256                     // Number of tracks in sequence
#define MAXIMPORT           (36*8)                  // Number of subdirs in IMPORT directory

#define NOSPARKLE           255
#define ANYTIME             255                     // Code for respawnvalid...

#define SIZETIME            50                      // Time it takes to resize a character
#define WATCHMIN            .01                     //

#define PRTLEVELFIX         20                      // Fix for shooting over cliffs
#define PACKDELAY           25                      // Time before inventory rotate again
#define GRABDELAY           25                      // Time before grab again
#define NOSKINOVERRIDE      -1                      // For import

#define PITDEPTH            -30                     // Depth to kill character
#define PITNOSOUND          -256                    // Stop sound at bottom of pits...

EXTERN unsigned short		endtextindex[8];

/*
#define MAXSPEECH           6
#define SPEECHMOVE          0
#define SPEECHMOVEALT       1
#define SPEECHATTACK        2
#define SPEECHASSIST        3
#define SPEECHTERRAIN       4
#define SPEECHSELECT        5
*/

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


#define LOWSTAT             256                     // Worst...
#define PERFECTSTAT         (33*256)                // Perfect...
#define PERFECTBIG          (127*256)               // Perfect life or mana...
#define HIGHSTAT            (99*256)                // Absolute max strength...
#define MINDAMAGE           256                     // Minimum damage for hurt animation
#define MSGDISTANCE         2000                    // Range for SendMessageNear
EXTERN int wraptolerance  EQ(80);					// Status bar

#define DAMFXNONE           0                       // Damage effects
#define DAMFXARMO           1                       // Armor piercing
#define DAMFXBLOC           2                       // Cannot be blocked by shield
#define DAMFXARRO           4                       // Only hurts the one it's attached to
#define DAMFXTURN           8                       // Turn to attached direction
#define DAMFXTIME           16                      //


#define ULTRABLOODY         2                       // This makes any damage draw blood
#define SPELLBOOK           127                     // The spellbook model TODO: change this badly thing

#define GENFEMALE           0                       // Gender
#define GENMALE             1                       //
#define GENOTHER            2                       //
#define GENRANDOM           3                       //

#define RETURNAND           63                      // Return mana every so often
#define MANARETURNSHIFT     4                       //
#define HURTDAMAGE          (1*256)                 //

#define MAXPASS             256                     // Maximum number of passages ( mul 32 )
#define MAXSTAT             16                      // Maximum status displays
#define MAXLOCO             3                       // Maximum number of local players

#define JOYBUTTON           8                       // Maximum number of joystick buttons
#define MAXMESSAGE          6                       // Number of messages
#define MAXTOTALMESSAGE     1024                    //
#define MESSAGESIZE         80                      //
#define MESSAGEBUFFERSIZE   (MAXTOTALMESSAGE*40)
#define MESSAGETIME         200                     // Time to keep the message alive
#define TABAND              31                      // Tab size

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


#define GRABSIZE            90.0                    // Grab tolerance

#define LIPDA               0                       // For smooth transitions 'tween
#define LIPWA               1                       //   walking rates
#define LIPWB               2                       //
#define LIPWC               3                       //

#define NULLICON            0                       // Empty hand image

#define MAXPRTPIP           1024                    // Particle templates
#define MAXPRTPIPPEROBJECT  8                       //
#define COIN1               0                       // Coins are the first particles loaded
#define COIN5               1                       //
#define COIN25              2                       //
#define COIN100             3                       //
#define WEATHER4            4                       // Weather particles
#define WEATHER5            5                       // Weather particle finish
#define SPLASH              6                       // Water effects are next
#define RIPPLE              7                       //
#define DEFEND              8                       // Defend particle
#define RIPPLEAND           15                      // How often ripples spawn
#define RIPPLETOLERANCE     60                      // For deep water
#define SPLASHTOLERANCE     10                      //
#define CLOSETOLERANCE      2                       // For closing doors


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

#define MAXSTOR             8                       // Storage data
#define STORAND             7                       //

#define MAXWAY              8                       // Waypoints
#define WAYTHRESH           128                     // Threshold for reaching waypoint
#define AISMAXCOMPILESIZE   (128*4096/4)            // For parsing AI scripts
#define MAXLINESIZE         1024                    //
#define MAXAI               129                     //
#define MAXCODE             1024                    // Number of lines in AICODES.TXT
#define MAXCODENAMESIZE     64                      //

#define MAXCAPNAMESIZE      32                      // Character class names
#define MAXLEVEL            6                       // Levels 0-5
#define MAXEXPERIENCETYPE   8                       // Number of ways to get experience
#define MAXIDSZ             6                       // ID strings per character
#define IDSZNONE            0                       // [NONE]
#define IDSZPARENT          0                       // Parent index
#define IDSZTYPE            1                       // Self index
#define IDSZSKILL           2                       // Skill index
#define IDSZSPECIAL         3                       // Special index
#define IDSZHATE            4                       // Hate index
#define IDSZVULNERABILITY   5                       // Vulnerability index


#define XPFINDSECRET        0                       // Finding a secret
#define XPWINQUEST          1                       // Beating a module or a subquest
#define XPUSEDUNKOWN        2                       // Used an unknown item
#define XPKILLENEMY         3                       // Killed an enemy
#define XPKILLSLEEPY        4                       // Killed a sleeping enemy
#define XPKILLHATED         5                       // Killed a hated enemy 
#define XPTEAMKILL          6                       // Team has killed an enemy
#define XPTALKGOOD          7                       // Talk good, er...  I mean well
#define XPDIRECT            255                     // No modification


#define MAXTEAM             27                      // Teams A-Z, +1 more for damage tiles
#define DAMAGETEAM          26                      // For damage tiles
#define EVILTEAM            4                       // E
#define GOODTEAM            6                       // G
#define NULLTEAM            13                      // N


#define NOACTION            0xffff                  // Action not valid for this character
#define MAXACTION           76                      // Number of action types
EXTERN char    cActionName[MAXACTION][2];                  // Two letter name code

#define TURNMODEVELOCITY    0                       // Character gets rotation from velocity
#define TURNMODEWATCH       1                       // For watch towers
#define TURNMODESPIN        2                       // For spinning objects
#define TURNMODEWATCHTARGET 3                       // For combat intensive AI
#define SPINRATE            200                     // How fast spinners spin
#define FLYDAMPEN           .001                    // Levelling rate for flyers

#define LATCHBUTTONLEFT      1                      // Character button presses
#define LATCHBUTTONRIGHT     2                      //
#define LATCHBUTTONJUMP      4                      //
#define LATCHBUTTONALTLEFT   8                      // ( Alts are for grab/drop )
#define LATCHBUTTONALTRIGHT  16                     //
#define LATCHBUTTONPACKLEFT  32                     // ( Packs are for inventory cycle )
#define LATCHBUTTONPACKRIGHT 64                     //
#define LATCHBUTTONRESPAWN   128                    //

#define JUMPINFINITE        255                     // Flying character
#define JUMPDELAY           20                      // Time between jumps
#define JUMPTOLERANCE       20                      // Distance above ground to be jumping
#define SLIDETOLERANCE      10                      // Stick to ground better
#define PLATTOLERANCE       50 //5 //10             // Platform tolerance...
#define PLATADD             -10                     // Height add...
#define PLATASCEND          .10                     // Ascension rate
#define PLATKEEP            .90                     // Retention rate

#define MAPID 0x4470614d	                    	// The string... MapD

#define RAISE 12 //25                               // Helps correct z level
#define SHADOWRAISE 5                               //
#define DAMAGERAISE 25                              //

#define MAXWATERLAYER 2                             // Maximum water layers
#define MAXWATERFRAME 512                           // Maximum number of wave frames
#define WATERFRAMEAND (MAXWATERFRAME-1)             //
#define WATERPOINTS 4                               // Points in a water fan
#define WATERMODE 4                                 // Ummm...  For making it work, yeah...

#define DONTFLASH 255                               //

#define FOV                             60          // Field of view
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
#define MAXTICK                         (NUMTICK*5) // Max number of ticks to draw

#define TURNSPD                         .01         // Cutoff for turning or same direction
#define CAMKEYTURN                      10          // Keyboard camera rotation
#define CAMJOYTURN                      .01         // Joystick camera rotation


// Multi cam
#define MINZOOM                         500         // Camera distance
#define MAXZOOM                         600         //
#define MINZADD                         800         // Camera height
#define MAXZADD                         1500  //1000        //
#define MINUPDOWN                       (.24*PI)    // Camera updown angle
#define MAXUPDOWN                       (.18*PI)//(.15*PI) // (.18*PI)


#define MD2START                        0x32504449  // MD2 files start with these four bytes
#define MD2MAXLOADSIZE                  (512*1024)  // Don't load any models bigger than 512k
#define MD2LIGHTINDICES                 163//162    // MD2's store vertices as x,y,z,normal
#define EQUALLIGHTINDEX                 162         // I added an extra index to do the
                                                    // spikey mace...

#define MAXTEXTURE                      512         // Max number of textures
#define MAXVERTICES                     512    //128     // Max number of points in a model
#define MAXCOMMAND                      256         // Max number of commands
#define MAXCOMMANDSIZE                  64          // Max number of points in a command
#define MAXCOMMANDENTRIES               512//256         // Max entries in a command list ( trigs )
#define MAXMODEL                        256         // Max number of models
#define MAXEVE                          MAXMODEL    // One enchant type per model
#define MAXEVESETVALUE                  24          // Number of sets
#define MAXEVEADDVALUE                  16          // Number of adds
#define MAXENCHANT                      128         // Number of enchantments
#define MAXFRAME                        (128*32)    // Max number of frames in all models
#define MAXCHR                          350         // Max number of characters
//#define MAXCHRBIT                       (256>>5)    // Bitwise compression
#define MAXLIGHTLEVEL                   16          // Number of premade light intensities
#define MAXSPEKLEVEL                    16          // Number of premade specularities
#define MAXLIGHTROTATION                256         // Number of premade light maps
#define MAXPRT                          512         // Max number of particles
#define MAXPARTICLEIMAGE                256         // Number of particle images ( frames )
#define MAXDYNA                         8           // Number of dynamic lights
#define MAXDYNADIST                     2700        // Leeway for offscreen lights
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


#define SLOPE                           800         // Slope increments for terrain normals
#define SLIDE                           .04         // Acceleration for steep hills
#define SLIDEFIX                        .08         // To make almost flat surfaces flat

#define PRTLIGHTSPRITE                  0           // Magic effect particle
#define PRTSOLIDSPRITE                  1           // Sprite particle
#define PRTALPHASPRITE                  2           // Smoke particle

#ifndef CLOCK_PER_SEC
#define CLOCKS_PER_SEC          1000        // Windows ticks 1000 times per second
#endif

#define FRAMESKIP                       CLOCKS_PER_SEC/50 //20          // 1000/20 = 50 game updates per second
#define ONESECOND                       (CLOCKS_PER_SEC/FRAMESKIP)
#define STOPBOUNCING                    0.1 //1.0         // To make objects stop bouncing
#define STOPBOUNCINGPART                5.0         // To make particles stop bouncing

#define TRANSCOLOR                      0           // Transparent color
#define BORETIME                        (rand()&255)+120
#define CAREFULTIME                     50
#define REEL                            7600.0      // Dampen for melee knock back
#define REELBASE                        .35         //

/* PORT
GUID FAR* enum_id;                                  // Ben's Voodoo search
int enum_nonnull EQ(0);                                 //
char enum_desc[100];                                //
*/

// Debug option
EXTERN bool_t	gGrabMouse EQ(btrue);
EXTERN bool_t gHideMouse EQ(bfalse);
// Debug option



EXTERN int animtileupdateand  EQ(7);                          // New tile every 7 frames
EXTERN unsigned short animtileframeand  EQ(3);                // Only 4 frames
EXTERN unsigned short animtilebaseand  EQ(0xfffc);            //
EXTERN unsigned short biganimtileframeand  EQ(7);             // For big tiles
EXTERN unsigned short biganimtilebaseand  EQ(0xfff8);         //
EXTERN unsigned short animtileframeadd  EQ(0);                // Current frame

EXTERN unsigned short bookicon  EQ(0);                        // The first book icon

#define NORTH 16384                                 // Character facings
#define SOUTH 49152                                 //
#define EAST 32768                                  //
#define WEST 0                                      //
#define FRONT 0                                     // Attack directions
#define BEHIND 32768                                //
#define LEFT 49152                                  //
#define RIGHT 16384                                 //


#define MAXXP 9999                                  // Maximum experience
#define MAXMONEY 9999                               // Maximum money
#define NOLEADER 65535                              // If the team has no leader...
EXTERN unsigned char  teamhatesteam[MAXTEAM][MAXTEAM];     // Don't damage allies...
EXTERN unsigned short teammorale[MAXTEAM];                 // Number of characters on team
EXTERN unsigned short teamleader[MAXTEAM];                 // The leader of the team
EXTERN unsigned short teamsissy[MAXTEAM];                  // Whoever called for help last
EXTERN short damagetileparttype;
EXTERN short damagetilepartand;
EXTERN short damagetilesound;
EXTERN short damagetilesoundtime;
EXTERN int   damagetilemindistance;
#define TILESOUNDTIME 16
#define TILEREAFFIRMAND  3


//Minimap stuff
#define MAXBLIP 32
EXTERN unsigned short          numblip  EQ(0);
EXTERN unsigned short          blipx[MAXBLIP];
EXTERN unsigned short          blipy[MAXBLIP];
EXTERN unsigned char           blipc[MAXBLIP];
EXTERN unsigned char           mapon  EQ(bfalse);
EXTERN unsigned char           youarehereon  EQ(bfalse);


EXTERN unsigned char           timeron  EQ(bfalse);            // Game timer displayed?
EXTERN unsigned int            timervalue  EQ(0);             // Timer time ( 50ths of a second )
EXTERN char                    szfpstext[]  EQ("000 FPS");
EXTERN unsigned char           fpson  EQ(btrue);               // FPS displayed?
EXTERN signed int              sttclock;                   // GetTickCount at start
EXTERN signed int              allclock  EQ(0);               // The total number of ticks so far
EXTERN signed int              lstclock  EQ(0);               // The last total of ticks so far
EXTERN signed int              wldclock  EQ(0);               // The sync clock
EXTERN signed int              fpsclock  EQ(0);               // The number of ticks this second
EXTERN unsigned int            wldframe  EQ(0);               // The number of frames that should have been drawn
EXTERN unsigned int            allframe  EQ(0);               // The total number of frames drawn so far
EXTERN unsigned int            fpsframe  EQ(0);               // The number of frames drawn this second
EXTERN unsigned char           statclock  EQ(0);              // For stat regeneration
EXTERN unsigned char           pitclock  EQ(0);               // For pit kills
EXTERN unsigned char           pitskill  EQ(bfalse);            // Do they kill?
EXTERN unsigned char outofsync  EQ(0);
EXTERN unsigned char parseerror  EQ(0);



EXTERN bool_t                    fullscreen EQ(bfalse);          // Start in fullscreen?
//EXTERN bool_t                    videoacceleratoron;            // Set to 1 if deviceguid == 0
EXTERN bool_t                    clearson  EQ(1);               // Do we clear every time?
EXTERN bool_t                    gameactive  EQ(bfalse);         // Stay in game or quit to windows?
EXTERN bool_t                    moduleactive  EQ(bfalse);       // Is the control loop still going?
EXTERN bool_t                    soundon  EQ(1);                // Is the sound alive?
EXTERN bool_t                    mouseon  EQ(1);                // Is the mouse alive?
EXTERN bool_t                    keyon  EQ(1);                  // Is the keyboard alive?
EXTERN bool_t                    joyaon  EQ(0);                 // Is the holy joystick alive?
EXTERN bool_t                    joybon  EQ(0);                 // Is the other joystick alive?
EXTERN bool_t                    staton  EQ(1);                 // Draw the status bars?
EXTERN bool_t                    phongon  EQ(1);                // Do phong overlay?
EXTERN bool_t                    networkon  EQ(1);              // Try to connect?
EXTERN bool_t                    serviceon  EQ(bfalse);          // Do I need to free the interface?
EXTERN bool_t                    twolayerwateron  EQ(1);        // Two layer water?
EXTERN bool_t                    menuaneeded  EQ(bfalse);         // Give them MENUA?
EXTERN bool_t                    menuactive  EQ(bfalse);         // Menu running?
EXTERN bool_t                    hostactive  EQ(bfalse);         // Hosting?
EXTERN bool_t                    readytostart;               // Ready to hit the Start Game button?
EXTERN bool_t                    waitingforplayers;          // Has everyone talked to the host?
EXTERN bool_t                    alllocalpladead;            // Has everyone died?
EXTERN bool_t                    respawnvalid;               // Can players respawn with Spacebar?
EXTERN bool_t                    respawnanytime;             // True if it's a small level...
EXTERN bool_t                    importvalid;                // Can it import?
EXTERN bool_t                    exportvalid;                // Can it export?
EXTERN bool_t                    rtscontrol;                 // Play as a real-time stragedy? BAD REMOVE
EXTERN bool_t                    allselect;                  // Select entire team at start?
EXTERN bool_t                    netmessagemode;             // Input text from keyboard?
EXTERN bool_t                    backgroundvalid;            // Allow large background?
EXTERN bool_t                    overlayvalid;               // Allow large overlay?
EXTERN bool_t                    nolocalplayers;             // Are there any local players?
EXTERN bool_t                    usefaredge;                 // Far edge maps? (Outdoor)
EXTERN bool_t                    beatmodule;                 // Show Module Ended text?
EXTERN unsigned char           autoturncamera;             // Type of camera control...
EXTERN unsigned char           doturntime;                 // Time for smooth turn
#define TURNTIME 16
EXTERN unsigned char           netmessagedelay;            // For slowing down input
EXTERN int                     netmessagewrite;            // The cursor position
EXTERN int                     netmessagewritemin;         // The starting cursor position
EXTERN char                    netmessage[MESSAGESIZE];    // The input message
EXTERN int                     importamount;               // Number of imports for this module
EXTERN int                     playeramount;               //
EXTERN unsigned int            seed  EQ(0);                   // The module seed
EXTERN char                    pickedmodule[64];           // The module load name
EXTERN int                     pickedindex;                // The module index number
EXTERN int                     playersready;               // Number of players ready to start
EXTERN int                     playersloaded;              //

// JF - Added so that the video mode might be determined outside of the graphics code
extern SDL_Surface *displaySurface;

//Networking
EXTERN int                     localmachine  EQ(0);           // 0 is host, 1 is 1st remote, 2 is 2nd...
EXTERN int                     numimport;                  // Number of imports from this machine
EXTERN unsigned char           localcontrol[16];           // For local imports
EXTERN short                   localslot[16];              // For local imports

//Setup values
EXTERN int                     maxmessage  EQ(MAXMESSAGE);    //
EXTERN int                     scrd  EQ(8);                   // Screen bit depth
EXTERN int                     scrz  EQ(16);                  // Screen z-buffer depth ( 8 unsupported )
EXTERN int                     scrx  EQ(320);                 // Screen X size
EXTERN int                     scry  EQ(200);                 // Screen Y size
EXTERN unsigned char           reffadeor  EQ(0);              // 255 = Don't fade reflections
EXTERN unsigned char           messageon  EQ(btrue);           // Messages?
EXTERN bool_t                  overlayon  EQ(bfalse);          //Draw overlay?
EXTERN bool_t                  perspective  EQ(bfalse);        // Perspective correct textures?
EXTERN bool_t                  dither  EQ(bfalse);             // Dithering?
EXTERN bool_t                  shading  EQ(bfalse);             //Gourad shading?
EXTERN bool_t                  antialiasing  EQ(bfalse);       //Antialiasing?
EXTERN bool_t                  refon  EQ(bfalse);              // Reflections?
EXTERN bool_t                  shaon  EQ(bfalse);              // Shadows?
EXTERN int		               texturefilter  EQ(1);       //Texture filtering?
EXTERN bool_t                  wateron  EQ(btrue);             // Water overlays?
EXTERN bool_t                  shasprite  EQ(bfalse);          // Shadow sprites?
EXTERN bool_t                  zreflect  EQ(bfalse);           // Reflection z buffering?
EXTERN float                   mousesense  EQ(6);             // Sensitivity threshold
EXTERN float                   mousesustain EQ(.50);           // Falloff rate for old movement
EXTERN float                   mousecover EQ(.50);             // For falloff
EXTERN Sint32                    mousex  EQ(0);                 // Mouse X movement counter
EXTERN Sint32                    mousey  EQ(0);                 // Mouse Y movement counter
EXTERN Sint32                    mousez  EQ(0);                 // Mouse wheel movement counter
EXTERN int                     cursorx  EQ(0);                // Cursor position
EXTERN int                     cursory  EQ(0);                //
EXTERN float                   mouselatcholdx  EQ(0);         // For sustain
EXTERN float                   mouselatcholdy  EQ(0);         //
EXTERN unsigned char           mousebutton[4];             // Mouse button states
EXTERN bool_t                    pressed EQ(0);                  //
EXTERN bool_t                    clicked EQ(0);                  //
EXTERN bool_t		       pending_click EQ(0);	       
// EWWWW. GLOBALS ARE EVIL.

//Input Control
//PORT: Use sdlkeybuffer instead.
//EXTERN char                    keybuffer[256];             // Keyboard key states
//EXTERN char                    keypress[256];              // Keyboard new hits
EXTERN int                     joyax  EQ(0);                  // Joystick A
EXTERN int                     joyay  EQ(0);                  //
EXTERN unsigned char           joyabutton[JOYBUTTON];      //
EXTERN int                     joybx  EQ(0);                  // Joystick B
EXTERN int                     joyby  EQ(0);                  //
EXTERN unsigned char           joybbutton[JOYBUTTON];      //
EXTERN unsigned char           msb, jab, jbb;              // Button masks
#define	KEYDOWN(k) (sdlkeybuffer[k])						// Helper for gettin' em


//Weather and water gfx
EXTERN unsigned char		   watershift  EQ(3);					
EXTERN int                     weatheroverwater EQ(bfalse);         // Only spawn over water?
EXTERN int                     weathertimereset EQ(10);            // Rate at which weather particles spawn
EXTERN int                     weathertime EQ(0);                  // 0 is no weather
EXTERN int                     weatherplayer;
EXTERN int                     numwaterlayer EQ(0);                // Number of layers
EXTERN float                   watersurfacelevel EQ(0);            // Surface level for water striders
EXTERN float                   waterdouselevel EQ(0);              // Surface level for torches
EXTERN unsigned char           waterlight EQ(0);                   // Is it light ( default is alpha )
EXTERN unsigned char           waterspekstart EQ(128);             // Specular begins at which light value
EXTERN unsigned char           waterspeklevel EQ(128);             // General specular amount (0-255)
EXTERN unsigned char           wateriswater  EQ(btrue);            // Is it water?  ( Or lava... )
EXTERN unsigned char           waterlightlevel[MAXWATERLAYER]; // General light amount (0-63)
EXTERN unsigned char           waterlightadd[MAXWATERLAYER];   // Ambient light amount (0-63)
EXTERN float                   waterlayerz[MAXWATERLAYER];     // Base height of water
EXTERN unsigned char           waterlayeralpha[MAXWATERLAYER]; // Transparency
EXTERN float                   waterlayeramp[MAXWATERLAYER];   // Amplitude of waves
EXTERN float                   waterlayeru[MAXWATERLAYER];     // Coordinates of texture
EXTERN float                   waterlayerv[MAXWATERLAYER];     //
EXTERN float                   waterlayeruadd[MAXWATERLAYER];  // Texture movement
EXTERN float                   waterlayervadd[MAXWATERLAYER];  //
EXTERN float                   waterlayerzadd[MAXWATERLAYER][MAXWATERFRAME][WATERMODE][WATERPOINTS];
EXTERN unsigned char           waterlayercolor[MAXWATERLAYER][MAXWATERFRAME][WATERMODE][WATERPOINTS];
EXTERN unsigned short          waterlayerframe[MAXWATERLAYER]; // Frame
EXTERN unsigned short          waterlayerframeadd[MAXWATERLAYER];      // Speed
EXTERN float                   waterlayerdistx[MAXWATERLAYER];         // For distant backgrounds
EXTERN float                   waterlayerdisty[MAXWATERLAYER];         //
EXTERN Uint32                  waterspek[256];             // Specular highlights
EXTERN float                   foregroundrepeat  EQ(1);       //
EXTERN float                   backgroundrepeat  EQ(1);       //

//Fog stuff
EXTERN unsigned char           fogallowed  EQ(btrue);          //
EXTERN unsigned char           fogon  EQ(bfalse);              // Do ground fog?
EXTERN float                   fogbottom  EQ(0.0);            //
EXTERN float                   fogtop  EQ(100);               //
EXTERN float                   fogdistance  EQ(100);          //
EXTERN unsigned char           fogred  EQ(255);               //  Fog collour
EXTERN unsigned char           foggrn  EQ(255);               //
EXTERN unsigned char           fogblu  EQ(255);               //
EXTERN unsigned char           fogaffectswater;

//Camera control stuff
EXTERN int                     camswing  EQ(0);               // Camera swingin'
EXTERN int                     camswingrate  EQ(0);           //
EXTERN float                   camswingamp  EQ(0);            //
EXTERN float                   camx  EQ(0);                   // Camera position
EXTERN float                   camy  EQ(1500);                //
EXTERN float                   camz  EQ(750);                 // 500-1000
EXTERN float                   camzoom  EQ(1000);             // Distance from the trackee
EXTERN float                   camtrackxvel;               // Change in trackee position
EXTERN float                   camtrackyvel;               //
EXTERN float                   camtrackzvel;               //
EXTERN float                   camcenterx;                 // Move character to side before tracking 
EXTERN float                   camcentery;                 //
EXTERN float                   camtrackx;                  // Trackee position
EXTERN float                   camtracky;                  //
EXTERN float                   camtrackz;                  //
EXTERN float                   camtracklevel;              //
EXTERN float                   camzadd  EQ(800);              // Camera height above terrain
EXTERN float                   camzaddgoto  EQ(800);          // Desired z position
EXTERN float                   camzgoto  EQ(800);             //
EXTERN float                   camturnleftright  EQ((float) (-PI/4));       // Camera rotations
EXTERN float                   camturnleftrightone  EQ((float) (-PI/4)/(2*PI));
EXTERN unsigned short          camturnleftrightshort  EQ(0);
EXTERN float                   camturnadd  EQ(0);             // Turning rate
EXTERN float                   camsustain  EQ(.80);           // Turning rate falloff
EXTERN float                   camturnupdown  EQ((float) (PI/4));
EXTERN float                   camroll  EQ(0);                //
EXTERN float                   cornerx[4];                 // Render area corners
EXTERN float                   cornery[4];                 //
EXTERN int                     cornerlistlowtohighy[4];    // Ordered list
EXTERN int                     cornerlowx;                 // Render area extremes
EXTERN int                     cornerhighx;                //
EXTERN int                     cornerlowy;                 //
EXTERN int                     cornerhighy;                //
EXTERN int                     fontoffset;                 // Line up fonts from top of screen

/*OpenGL Textures*/
EXTERN	GLTexture		TxIcon[MAXTEXTURE+1];						//OpenGL icon surfaces
EXTERN	GLTexture		TxTitleImage[MAXMODULE];					//OpenGL title image surfaces
//EXTERN	GLTexture	TxTrimX;									//OpenGL trim surface
//EXTERN	GLTexture	TxTrimY;									//OpenGL trim surface
EXTERN	GLTexture		TxTrim;
EXTERN	GLTexture		TxFont;										//OpenGL font surface
EXTERN	GLTexture		TxBars;										//OpenGL status bar surface
EXTERN	GLTexture		TxBlip;										//OpenGL you are here surface
EXTERN	GLTexture		TxMap;										//OpenGL map surface
EXTERN	GLTexture		TxTrim;	
EXTERN  GLTexture       TxIcon[MAXTEXTURE+1];							/* icons */
EXTERN  GLTexture       TxTitleImage[MAXMODULE];						/* title images */
EXTERN  GLTexture       TxTrimX;                                        /* trim */
EXTERN  GLTexture       TxTrimY;                                        /* trim */
EXTERN  GLTexture       TxFont;                                         /* font */
EXTERN  GLTexture       TxBars;                                         /* status bars */
EXTERN  GLTexture       TxBlip;                                         /* you are here texture */
EXTERN  GLTexture       TxMap;
EXTERN  GLTexture       txTexture[MAXTEXTURE];							 /* All textures */



//Anisotropic filtering - yay! :P
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
EXTERN float maxAnisotropy;										//Max anisotropic filterings (Between 1.00 and 2.00)

EXTERN GLMATRIX mWorld;											// World Matrix
EXTERN GLMATRIX mView;											// View Matrix
EXTERN GLMATRIX mViewSave;										//View Matrix initial state
EXTERN GLMATRIX mProjection;									//Projection Matrix


//Input player control
EXTERN int                     numloadplayer  EQ(0);
#define MAXLOADPLAYER   100
EXTERN char                    loadplayername[MAXLOADPLAYER][MAXCAPNAMESIZE];
EXTERN char                    loadplayerdir[MAXLOADPLAYER][16];
EXTERN int                     nullicon  EQ(0);
EXTERN int                     keybicon  EQ(0);
EXTERN int                     mousicon  EQ(0);
EXTERN int                     joyaicon  EQ(0);
EXTERN int                     joybicon  EQ(0);
EXTERN int                     keybplayer  EQ(0);
EXTERN int                     mousplayer  EQ(0);
EXTERN int                     joyaplayer  EQ(0);
EXTERN int                     joybplayer  EQ(0);

//Interface stuff
#define TRIMX 640
#define TRIMY 480
EXTERN rect_t                    iconrect;                   // The 32x32 icon rectangle
EXTERN rect_t                    trimrect;                   // The menu trim rectangle
EXTERN SDL_Rect					 fontrect[NUMFONT];          // The font rectangles
EXTERN unsigned char			 fontxspacing[NUMFONT];      // The spacing stuff
EXTERN unsigned char			 fontyspacing;               //
EXTERN rect_t                    tabrect[NUMBAR];            // The tab rectangles
EXTERN rect_t                    barrect[NUMBAR];            // The bar rectangles
EXTERN rect_t                    bliprect[NUMBAR];           // The blip rectangles
EXTERN rect_t                    maprect;                    // The map rectangle
#define SPARKLESIZE 28
#define SPARKLEADD 2
#define MAPSIZE 64
#define BLIPSIZE 3

//Lightning effects
EXTERN int                     numdynalight;               // Number of dynamic lights
EXTERN int                     dynadistancetobeat;         // The number to beat
EXTERN int                     dynadistance[MAXDYNA];      // The distances
EXTERN float                   dynalightlistx[MAXDYNA];    // Light position 
EXTERN float                   dynalightlisty[MAXDYNA];    //
EXTERN float                   dynalightlevel[MAXDYNA];    // Light level
EXTERN float                   dynalightfalloff[MAXDYNA];  // Light falloff
EXTERN unsigned char           lightdirectionlookup[65536];// For lighting characters
EXTERN unsigned char           lighttable[MAXLIGHTLEVEL][MAXLIGHTROTATION][MD2LIGHTINDICES];
EXTERN unsigned char           cLoadBuffer[MD2MAXLOADSIZE];// Where to put an MD2
EXTERN float                   turntosin[16384];           // Convert chrturn>>2...  to sine


EXTERN unsigned int            maplrtwist[256];            // For surface normal of mesh
EXTERN unsigned int            mapudtwist[256];            //
EXTERN float                   vellrtwist[256];            // For sliding down steep hills
EXTERN float                   veludtwist[256];            //
EXTERN unsigned char           flattwist[256];             //

//Camera stuff
#define TRACKXAREALOW     100
#define TRACKXAREAHIGH    180
#define TRACKYAREAMINLOW  320
#define TRACKYAREAMAXLOW  460
#define TRACKYAREAMINHIGH 460
#define TRACKYAREAMAXHIGH 600


//Character stuff
EXTERN int                     numfreechr EQ(0);               // For allocation
EXTERN unsigned short          freechrlist[MAXCHR];        //          
EXTERN GLMATRIX                chrmatrix[MAXCHR];			// Character's matrix
EXTERN char                    chrmatrixvalid[MAXCHR];     // Did we make one yet?
EXTERN char                    chrname[MAXCHR][MAXCAPNAMESIZE];  // Character name
EXTERN unsigned char           chron[MAXCHR];              // Does it exist?
EXTERN unsigned char           chronold[MAXCHR];           // Network fix
EXTERN unsigned char           chralive[MAXCHR];           // Is it alive?
EXTERN unsigned char           chrwaskilled[MAXCHR];       // Fix for network
EXTERN unsigned char           chrinpack[MAXCHR];          // Is it in the inventory?
EXTERN unsigned char           chrwasinpack[MAXCHR];       // Temporary thing...
EXTERN unsigned short          chrnextinpack[MAXCHR];      // Link to the next item
EXTERN unsigned char           chrnuminpack[MAXCHR];       // How many
EXTERN unsigned char           chropenstuff[MAXCHR];       // Can it open chests/doors?
EXTERN unsigned char           chrlifecolor[MAXCHR];       // Bar color
EXTERN unsigned char           chrsparkle[MAXCHR];         // Sparkle color or 0 for off
EXTERN signed short            chrlife[MAXCHR];            // Basic character stats
EXTERN signed short            chrlifemax[MAXCHR];         //   All 8.8 fixed point
EXTERN unsigned short          chrlifeheal[MAXCHR];        //
EXTERN unsigned char           chrmanacolor[MAXCHR];       // Bar color
EXTERN unsigned char           chrammomax[MAXCHR];         // Ammo stuff
EXTERN unsigned char           chrammo[MAXCHR];            //
EXTERN unsigned char           chrgender[MAXCHR];          // Gender
EXTERN signed short            chrmana[MAXCHR];            // Mana stuff
EXTERN signed short            chrmanamax[MAXCHR];         //
EXTERN signed short            chrmanaflow[MAXCHR];        //
EXTERN signed short            chrmanareturn[MAXCHR];      //
EXTERN signed short            chrstrength[MAXCHR];        // Strength
EXTERN signed short            chrwisdom[MAXCHR];          // Wisdom
EXTERN signed short            chrintelligence[MAXCHR];    // Intelligence
EXTERN signed short            chrdexterity[MAXCHR];       // Dexterity
EXTERN unsigned char           chraitype[MAXCHR];          // The AI script to run
EXTERN bool_t                    chricon[MAXCHR];            // Show the icon?
EXTERN bool_t                    chrcangrabmoney[MAXCHR];    // Picks up coins?
EXTERN bool_t                    chrisplayer[MAXCHR];        // btrue = player
EXTERN bool_t                    chrislocalplayer[MAXCHR];   // btrue = local player
EXTERN unsigned short          chraitarget[MAXCHR];        // Who the AI is after
EXTERN unsigned short          chraiowner[MAXCHR];         // The character's owner
EXTERN unsigned short          chraichild[MAXCHR];         // The character's child
EXTERN int                     chraistate[MAXCHR];         // Short term memory for AI
EXTERN int                     chraicontent[MAXCHR];       // More short term memory
EXTERN unsigned short          chraitime[MAXCHR];          // AI Timer
EXTERN unsigned char           chraigoto[MAXCHR];          // Which waypoint
EXTERN unsigned char           chraigotoadd[MAXCHR];       // Where to stick next
EXTERN float                   chraigotox[MAXCHR][MAXWAY]; // Waypoint
EXTERN float                   chraigotoy[MAXCHR][MAXWAY]; // Waypoint
EXTERN int                     chraix[MAXCHR][MAXSTOR];    // Temporary values...  SetXY
EXTERN int                     chraiy[MAXCHR][MAXSTOR];    //
EXTERN unsigned char           chrstickybutt[MAXCHR];      // Rests on floor
EXTERN unsigned char           chrenviro[MAXCHR];          // Environment map?
EXTERN float                   chroldx[MAXCHR];            // Character's last position
EXTERN float                   chroldy[MAXCHR];            //
EXTERN float                   chroldz[MAXCHR];            //
EXTERN unsigned char           chrinwater[MAXCHR];         //
EXTERN unsigned short          chroldturn[MAXCHR];         //
EXTERN unsigned int            chralert[MAXCHR];           // Alerts for AI script
EXTERN unsigned char           chrflyheight[MAXCHR];       // Height to stabilize at
EXTERN unsigned char           chrteam[MAXCHR];            // Character's team
EXTERN unsigned char           chrbaseteam[MAXCHR];        // Character's starting team
EXTERN unsigned char           chrstaton[MAXCHR];          // Display stats?
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
EXTERN unsigned char           chrlatchbutton[MAXCHR];     // Button latches
EXTERN unsigned char           chrreloadtime[MAXCHR];      // Time before another shot
EXTERN float                   chrmaxaccel[MAXCHR];        // Maximum acceleration
EXTERN float                   chrscale[MAXCHR];           // Character's size (useful)
EXTERN float                   chrfat[MAXCHR];             // Character's size (legible)
EXTERN float                   chrsizegoto[MAXCHR];        // Character's size goto ( legible )
EXTERN unsigned char           chrsizegototime[MAXCHR];    // Time left in siez change
EXTERN float                   chrdampen[MAXCHR];          // Bounciness
EXTERN float                   chrlevel[MAXCHR];           // Height of tile
EXTERN float                   chrjump[MAXCHR];            // Jump power
EXTERN unsigned char           chrjumptime[MAXCHR];        // Delay until next jump
EXTERN unsigned char           chrjumpnumber[MAXCHR];      // Number of jumps remaining
EXTERN unsigned char           chrjumpnumberreset[MAXCHR]; // Number of jumps total, 255=Flying
EXTERN unsigned char           chrjumpready[MAXCHR];       // For standing on a platform character
EXTERN unsigned int            chronwhichfan[MAXCHR];      // Where the char is
EXTERN unsigned char           chrindolist[MAXCHR];        // Has it been added yet?
EXTERN unsigned short          chruoffset[MAXCHR];         // For moving textures
EXTERN unsigned short          chrvoffset[MAXCHR];         //
EXTERN unsigned short          chruoffvel[MAXCHR];         // Moving texture speed
EXTERN unsigned short          chrvoffvel[MAXCHR];         //
EXTERN unsigned short          chrturnleftright[MAXCHR];   // Character's rotation 0 to 65535
EXTERN unsigned short          chrlightturnleftright[MAXCHR];// Character's light rotation 0 to 65535
EXTERN unsigned short          chrturnmaplr[MAXCHR];       //
EXTERN unsigned short          chrturnmapud[MAXCHR];       //
EXTERN unsigned short          chrtexture[MAXCHR];         // Character's skin
EXTERN unsigned char           chrmodel[MAXCHR];           // Character's model
EXTERN unsigned char           chrbasemodel[MAXCHR];       // The true form
EXTERN unsigned char           chractionready[MAXCHR];     // Ready to play a new one
EXTERN unsigned char           chraction[MAXCHR];          // Character's action
EXTERN unsigned char           chrkeepaction[MAXCHR];      // Keep the action playing
EXTERN unsigned char           chrloopaction[MAXCHR];      // Loop it too
EXTERN unsigned char           chrnextaction[MAXCHR];      // Character's action to play next
EXTERN unsigned short          chrframe[MAXCHR];           // Character's frame
EXTERN unsigned short          chrlastframe[MAXCHR];       // Character's last frame
EXTERN unsigned char           chrlip[MAXCHR];             // Character's frame in betweening
EXTERN unsigned char           chrvrta[MAXCHR][MAXVERTICES];// Lighting hack ( Ooze )
EXTERN unsigned short          chrholdingwhich[MAXCHR][2]; // !=MAXCHR if character is holding something
EXTERN unsigned short          chrattachedto[MAXCHR];      // !=MAXCHR if character is a held weapon 
EXTERN unsigned short          chrweapongrip[MAXCHR][4];   // Vertices which describe the weapon grip
EXTERN unsigned char           chralpha[MAXCHR];           // 255 = Solid, 0 = Invisible
EXTERN unsigned char           chrlight[MAXCHR];           // 1 = Light, 0 = Normal
EXTERN unsigned char           chrflashand[MAXCHR];        // 1,3,7,15,31 = Flash, 255 = Don't
EXTERN unsigned char           chrlightlevel[MAXCHR];      // 0-255, terrain light
EXTERN unsigned char           chrsheen[MAXCHR];           // 0-15, how shiny it is
EXTERN unsigned char           chrtransferblend[MAXCHR];   // Give transparency to weapons?
EXTERN unsigned char           chrisitem[MAXCHR];          // Is it grabbable?
EXTERN unsigned char           chrinvictus[MAXCHR];        // Totally invincible?
EXTERN unsigned char           chrismount[MAXCHR];         // Can you ride it?
EXTERN unsigned char           chrredshift[MAXCHR];        // Color channel shifting
EXTERN unsigned char           chrgrnshift[MAXCHR];        //
EXTERN unsigned char           chrblushift[MAXCHR];        //
EXTERN unsigned char           chrshadowsize[MAXCHR];      // Size of shadow
EXTERN unsigned char           chrbumpsize[MAXCHR];        // Size of bumpers
EXTERN unsigned char           chrbumpsizebig[MAXCHR];     // For octagonal bumpers
EXTERN unsigned char           chrbumpheight[MAXCHR];      // Distance from head to toe
EXTERN unsigned char           chrshadowsizesave[MAXCHR];  // Without size modifiers
EXTERN unsigned char           chrbumpsizesave[MAXCHR];    //
EXTERN unsigned char           chrbumpsizebigsave[MAXCHR]; //
EXTERN unsigned char           chrbumpheightsave[MAXCHR];  //
EXTERN unsigned short          chrbumpnext[MAXCHR];        // Next character on fanblock
EXTERN unsigned short          chrbumplast[MAXCHR];        // Last character it was bumped by
EXTERN float                   chrbumpdampen[MAXCHR];      // Character bump mass
EXTERN unsigned short          chrattacklast[MAXCHR];      // Last character it was attacked by
EXTERN unsigned short          chrhitlast[MAXCHR];         // Last character it hit
EXTERN unsigned short          chrdirectionlast[MAXCHR];   // Direction of last attack/healing
EXTERN unsigned char           chrdamagetypelast[MAXCHR];  // Last damage type
EXTERN unsigned char           chrplatform[MAXCHR];        // Can it be stood on
EXTERN unsigned char           chrwaterwalk[MAXCHR];       // Always above watersurfacelevel?
EXTERN unsigned char           chrturnmode[MAXCHR];        // Turning mode
EXTERN unsigned char           chrsneakspd[MAXCHR];        // Sneaking if above this speed
EXTERN unsigned char           chrwalkspd[MAXCHR];         // Walking if above this speed
EXTERN unsigned char           chrrunspd[MAXCHR];          // Running if above this speed
EXTERN unsigned char           chrdamagetargettype[MAXCHR];// Type of damage for AI DamageTarget
EXTERN unsigned char           chrreaffirmdamagetype[MAXCHR]; // For relighting torches
EXTERN unsigned char           chrdamagemodifier[MAXCHR][MAXDAMAGETYPE];  // Resistances and inversion
EXTERN unsigned char           chrdamagetime[MAXCHR];      // Invincibility timer
EXTERN unsigned char           chrdefense[MAXCHR];         // Base defense rating
EXTERN unsigned short          chrweight[MAXCHR];          // Weight ( for pressure plates )
EXTERN unsigned char           chrpassage[MAXCHR];         // The passage associated with this character
EXTERN unsigned int            chrorder[MAXCHR];           // The last order given the character
EXTERN unsigned char           chrcounter[MAXCHR];         // The rank of the character on the order chain
EXTERN unsigned short          chrholdingweight[MAXCHR];   // For weighted buttons
EXTERN signed short            chrmoney[MAXCHR];           // Money
EXTERN signed short            chrlifereturn[MAXCHR];      // Regeneration/poison
EXTERN signed short            chrmanacost[MAXCHR];        // Mana cost to use
EXTERN unsigned char           chrstoppedby[MAXCHR];       // Collision mask
EXTERN unsigned short          chrexperience[MAXCHR];      // Experience
EXTERN unsigned char           chrexperiencelevel[MAXCHR]; // Experience Level
EXTERN signed short            chrgrogtime[MAXCHR];        // Grog timer
EXTERN signed short            chrdazetime[MAXCHR];        // Daze timer
EXTERN unsigned char           chriskursed[MAXCHR];        // Can't be dropped?
EXTERN unsigned char           chrnameknown[MAXCHR];       // Is the name known?
EXTERN unsigned char           chrammoknown[MAXCHR];       // Is the ammo known?
EXTERN unsigned char           chrhitready[MAXCHR];        // Was it just dropped?
EXTERN signed short            chrboretime[MAXCHR];        // Boredom timer
EXTERN unsigned char           chrcarefultime[MAXCHR];     // "You hurt me!" timer
EXTERN unsigned char           chrcanbecrushed[MAXCHR];    // Crush in a door?
EXTERN unsigned char           chrinwhichhand[MAXCHR];     // GRIPLEFT or GRIPRIGHT
EXTERN unsigned char           chrisequipped[MAXCHR];      // For boots and rings and stuff
EXTERN unsigned char           chrfirstenchant[MAXCHR];    // Linked list for enchants
EXTERN unsigned char           chrundoenchant[MAXCHR];     // Last enchantment spawned
EXTERN bool_t                    chrcanseeinvisible[MAXCHR]; //
EXTERN bool_t                    chrcanseekurse[MAXCHR];     //
EXTERN bool_t                    chrcanchannel[MAXCHR];      //
EXTERN bool_t                    chroverlay[MAXCHR];         // Is this an overlay?  Track aitarget...
EXTERN unsigned char           chrmissiletreatment[MAXCHR];// For deflection, etc.
EXTERN unsigned char           chrmissilecost[MAXCHR];     // Mana cost for each one
EXTERN unsigned short          chrmissilehandler[MAXCHR];  // Who pays the bill for each one...
EXTERN unsigned short          chrdamageboost[MAXCHR];     // Add to swipe damage


#define SEEKURSEAND         31                      // Blacking flash
#define SEEINVISIBLE        128                     // Cutoff for invisible characters
#define INVISIBLE           20                      // The character can't be detected
EXTERN bool_t                    localseeinvisible;
EXTERN bool_t                    localseekurse;


//------------------------------------
//Enchantment variables
//------------------------------------
EXTERN unsigned short			 numfreeenchant;             // For allocating new ones
EXTERN unsigned short			 freeenchant[MAXENCHANT];    //

EXTERN bool_t                    evevalid[MAXEVE];                       // Enchant.txt loaded?
EXTERN bool_t                    eveoverride[MAXEVE];                    // Override other enchants?
EXTERN bool_t                    everemoveoverridden[MAXEVE];            // Remove other enchants?
EXTERN bool_t                    evesetyesno[MAXEVE][MAXEVESETVALUE];    // Set this value?
EXTERN unsigned char             evesetvalue[MAXEVE][MAXEVESETVALUE];    // Value to use
EXTERN signed char               eveaddvalue[MAXEVE][MAXEVEADDVALUE];    // The values to add
EXTERN bool_t                    everetarget[MAXEVE];                    // Pick a weapon?
EXTERN bool_t                    evekillonend[MAXEVE];                   // Kill the target on end?
EXTERN bool_t                    evepoofonend[MAXEVE];                   // Spawn a poof on end?
EXTERN bool_t                    eveendifcantpay[MAXEVE];                // End on out of mana
EXTERN bool_t                    evestayifnoowner[MAXEVE];               // Stay if owner has died?
EXTERN signed short            evetime[MAXEVE];                        // Time in seconds
EXTERN signed char             eveendmessage[MAXEVE];                  // Message for end -1 for none
EXTERN signed short            eveownermana[MAXEVE];                   // Boost values
EXTERN signed short            eveownerlife[MAXEVE];                   //
EXTERN signed short            evetargetmana[MAXEVE];                  //
EXTERN signed short            evetargetlife[MAXEVE];                  //
EXTERN unsigned char           evedontdamagetype[MAXEVE];              // Don't work if ... 
EXTERN unsigned char           eveonlydamagetype[MAXEVE];              // Only work if ...
EXTERN unsigned int            everemovedbyidsz[MAXEVE];               // By particle or [NONE]
EXTERN unsigned short          evecontspawntime[MAXEVE];               // Spawn timer
EXTERN unsigned char           evecontspawnamount[MAXEVE];             // Spawn amount
EXTERN unsigned short          evecontspawnfacingadd[MAXEVE];          // Spawn in circle
EXTERN unsigned short          evecontspawnpip[MAXEVE];                // Spawn type ( local )
EXTERN unsigned short	       evewaveindex[MAXEVE];                   // Sound on end (-1 for none)
EXTERN unsigned short          evefrequency[MAXEVE];                   // Sound frequency
EXTERN unsigned char           eveoverlay[MAXEVE];                     // Spawn an overlay?

EXTERN unsigned char           encon[MAXENCHANT];                      // Enchantment on
EXTERN unsigned char           enceve[MAXENCHANT];                     // The type
EXTERN unsigned short          enctarget[MAXENCHANT];                  // Who it enchants
EXTERN unsigned short          encnextenchant[MAXENCHANT];             // Next in the list
EXTERN unsigned short          encowner[MAXENCHANT];                   // Who cast the enchant
EXTERN unsigned short          encspawner[MAXENCHANT];                 // The spellbook character
EXTERN unsigned short          encoverlay[MAXENCHANT];                 // The overlay character
EXTERN signed short            encownermana[MAXENCHANT];               // Boost values
EXTERN signed short            encownerlife[MAXENCHANT];               //
EXTERN signed short            enctargetmana[MAXENCHANT];              //
EXTERN signed short            enctargetlife[MAXENCHANT];              //
EXTERN bool_t                  encsetyesno[MAXENCHANT][MAXEVESETVALUE];// Was it set?
EXTERN unsigned char           encsetsave[MAXENCHANT][MAXEVESETVALUE]; // The value to restore
EXTERN signed short            encaddsave[MAXENCHANT][MAXEVEADDVALUE]; // The value to take away
EXTERN signed short            enctime[MAXENCHANT];                    // Time before end
EXTERN unsigned short          encspawntime[MAXENCHANT];               // Time before spawn

#define MISNORMAL               0									//Treat missiles normally
#define MISDEFLECT              1									//Deflect incoming missiles
#define MISREFLECT              2									//Reflect them back!

#define LEAVEALL                0
#define LEAVEFIRST              1
#define LEAVENONE               2

#define SETDAMAGETYPE           0
#define SETNUMBEROFJUMPS        1
#define SETLIFEBARCOLOR         2
#define SETMANABARCOLOR         3
#define SETSLASHMODIFIER        4			//Damage modifiers
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
#define SETSHEEN                15			//Shinyness
#define SETFLYTOHEIGHT          16
#define SETWALKONWATER          17
#define SETCANSEEINVISIBLE      18
#define SETMISSILETREATMENT     19
#define SETCOSTFOREACHMISSILE   20
#define SETMORPH                21			//Morph character?
#define SETCHANNEL              22			//Can channel life as mana?

#define ADDJUMPPOWER            0
#define ADDBUMPDAMPEN           1
#define ADDBOUNCINESS           2
#define ADDDAMAGE               3
#define ADDSIZE                 4
#define ADDACCEL                5
#define ADDRED                  6			//Red shift
#define ADDGRN                  7			//Green shift
#define ADDBLU                  8			//Blue shift
#define ADDDEFENSE              9			//Defence adjustments
#define ADDMANA                 10	
#define ADDLIFE                 11
#define ADDSTRENGTH             12
#define ADDWISDOM               13
#define ADDINTELLIGENCE         14
#define ADDDEXTERITY            15


//------------------------------------
//Particle variables
//------------------------------------
#define SPAWNNOCHARACTER        255                 // For particles that spawn characters...

EXTERN float                   textureoffset[256];         // For moving textures
EXTERN unsigned short          dolist[MAXCHR];             // List of which characters to draw
EXTERN unsigned short          numdolist;                  // How many in the list

EXTERN int                     numfreeprt EQ(0);                           // For allocation
EXTERN unsigned short          freeprtlist[MAXPRT];                        //
EXTERN unsigned char           prton[MAXPRT];                              // Does it exist?
EXTERN unsigned short          prtpip[MAXPRT];                             // The part template
EXTERN unsigned short          prtmodel[MAXPRT];                           // Pip spawn model
EXTERN unsigned short          prtattachedtocharacter[MAXPRT];             // For torch flame
EXTERN unsigned short          prtgrip[MAXPRT];                            // The vertex it's on
EXTERN unsigned char           prttype[MAXPRT];                            // Transparency mode, 0-2
EXTERN unsigned short          prtfacing[MAXPRT];                          // Direction of the part
EXTERN unsigned char           prtteam[MAXPRT];                            // Team
EXTERN float                   prtxpos[MAXPRT];                            // Position
EXTERN float                   prtypos[MAXPRT];                            //
EXTERN float                   prtzpos[MAXPRT];                            //
EXTERN float                   prtxvel[MAXPRT];                            // Velocity
EXTERN float                   prtyvel[MAXPRT];                            //
EXTERN float                   prtzvel[MAXPRT];                            //
EXTERN float                   prtlevel[MAXPRT];                           // Height of tile
EXTERN unsigned char           prtspawncharacterstate[MAXPRT];             //
EXTERN unsigned short          prtrotate[MAXPRT];                          // Rotation direction
EXTERN signed short            prtrotateadd[MAXPRT];                       // Rotation rate
EXTERN unsigned int            prtonwhichfan[MAXPRT];                      // Where the part is
EXTERN unsigned short          prtsize[MAXPRT];                            // Size of particle>>8
EXTERN signed short            prtsizeadd[MAXPRT];                         // Change in size
EXTERN unsigned char           prtinview[MAXPRT];                          // Render this one?
EXTERN unsigned short          prtimage[MAXPRT];                           // Which image ( >> 8 )
EXTERN unsigned short          prtimageadd[MAXPRT];                        // Animation rate
EXTERN unsigned short          prtimagemax[MAXPRT];                        // End of image loop
EXTERN unsigned short          prtimagestt[MAXPRT];                        // Start of image loop
EXTERN unsigned char           prtlight[MAXPRT];                           // Light level
EXTERN unsigned short          prttime[MAXPRT];                            // Duration of particle
EXTERN unsigned short          prtspawntime[MAXPRT];                       // Time until spawn
EXTERN unsigned char           prtbumpsize[MAXPRT];                        // Size of bumpers
EXTERN unsigned char           prtbumpsizebig[MAXPRT];                     //
EXTERN unsigned char           prtbumpheight[MAXPRT];                      // Bounding box height
EXTERN unsigned short          prtbumpnext[MAXPRT];                        // Next particle on fanblock
EXTERN unsigned short          prtdamagebase[MAXPRT];                      // For strength
EXTERN unsigned short          prtdamagerand[MAXPRT];                      // For fixes...
EXTERN unsigned char           prtdamagetype[MAXPRT];                      // Damage type
EXTERN unsigned short          prtchr[MAXPRT];                             // The character that is attacking
EXTERN float                   prtdynalightfalloff[MAXPRT];                // Dyna light...
EXTERN float                   prtdynalightlevel[MAXPRT];                  //
EXTERN float                   particleimageu[MAXPARTICLEIMAGE][2];        // Texture coordinates
EXTERN float                   particleimagev[MAXPARTICLEIMAGE][2];        //
EXTERN unsigned short          particletexture;                            // All in one bitmap
EXTERN unsigned char           prtdynalighton[MAXPRT];                     // Dynamic light?
EXTERN unsigned short          prttarget[MAXPRT];                          // Who it's chasing


//------------------------------------
//Module variables
//------------------------------------
#define RANKSIZE 8
#define SUMMARYLINES 8
#define SUMMARYSIZE  80
EXTERN int                     globalnummodule;                            // Number of modules
EXTERN char                    modrank[MAXMODULE][RANKSIZE];               // Number of stars
EXTERN char                    modlongname[MAXMODULE][MAXCAPNAMESIZE];     // Module names
EXTERN char                    modloadname[MAXMODULE][MAXCAPNAMESIZE];     // Module load names
EXTERN unsigned char           modimportamount[MAXMODULE];                 // # of import characters
EXTERN unsigned char           modallowexport[MAXMODULE];                  // Export characters?
EXTERN unsigned char           modminplayers[MAXMODULE];                   // Number of players
EXTERN unsigned char           modmaxplayers[MAXMODULE];                   //
EXTERN unsigned char           modmonstersonly[MAXMODULE];                 // Only allow monsters
EXTERN unsigned char           modrtscontrol[MAXMODULE];                   // Real Time Stragedy?
EXTERN unsigned char           modrespawnvalid[MAXMODULE];                 // Allow respawn
EXTERN int                     numlines;                                   // Lines in summary
EXTERN char                    modsummary[SUMMARYLINES][SUMMARYSIZE];      // Quest description


//------------------------------------
//Model stuff
//------------------------------------
// TEMPORARY: Needs to be moved out of egoboo.h eventually
extern struct Md2Model *md2_models[MAXMODEL];							   // Md2 models

EXTERN int                     globalnumicon;                              // Number of icons
EXTERN unsigned short          madloadframe;                               // Where to load next
EXTERN unsigned char           madused[MAXMODEL];                          // Model slot
EXTERN char                    madname[MAXMODEL][128];                     // Model name
EXTERN unsigned short          madskintoicon[MAXTEXTURE];                  // Skin to icon
EXTERN unsigned short          madskins[MAXMODEL];                         // Number of skins
EXTERN unsigned short          madskinstart[MAXMODEL];                     // Starting skin of model
EXTERN unsigned short          madframes[MAXMODEL];                        // Number of frames
EXTERN unsigned short          madframestart[MAXMODEL];                    // Starting frame of model
EXTERN unsigned short          madmsgstart[MAXMODEL];                      // The first message
EXTERN unsigned short          madvertices[MAXMODEL];                      // Number of vertices
EXTERN unsigned short          madtransvertices[MAXMODEL];                 // Number to transform
EXTERN unsigned short          madcommands[MAXMODEL];                      // Number of commands
EXTERN float                   madscale[MAXMODEL];                         // Multiply by value
EXTERN GLenum				   madcommandtype[MAXMODEL][MAXCOMMAND];       // Fan or strip
EXTERN unsigned char           madcommandsize[MAXMODEL][MAXCOMMAND];       // Entries used by command
EXTERN unsigned short          madcommandvrt[MAXMODEL][MAXCOMMANDENTRIES]; // Which vertex
EXTERN float                   madcommandu[MAXMODEL][MAXCOMMANDENTRIES];   // Texture position
EXTERN float                   madcommandv[MAXMODEL][MAXCOMMANDENTRIES];   //
EXTERN signed short            madvrtx[MAXFRAME][MAXVERTICES];             // Vertex position
EXTERN signed short            madvrty[MAXFRAME][MAXVERTICES];             //
EXTERN signed short            madvrtz[MAXFRAME][MAXVERTICES];             //
EXTERN unsigned char           madvrta[MAXFRAME][MAXVERTICES];             // Light index of vertex
EXTERN unsigned char           madframelip[MAXFRAME];                      // 0-15, How far into action is each frame
EXTERN unsigned short          madframefx[MAXFRAME];                       // Invincibility, Spawning
EXTERN unsigned short          madframeliptowalkframe[MAXMODEL][4][16];    // For walk animations
EXTERN unsigned short          madai[MAXMODEL];                            // AI for each model
EXTERN unsigned char           madactionvalid[MAXMODEL][MAXACTION];        // bfalse if not valid
EXTERN unsigned short          madactionstart[MAXMODEL][MAXACTION];        // First frame of anim
EXTERN unsigned short          madactionend[MAXMODEL][MAXACTION];          // One past last frame
EXTERN unsigned short          madprtpip[MAXMODEL][MAXPRTPIPPEROBJECT];    // Local particles


// Character profiles
EXTERN int                   importobject;
EXTERN short                 capimportslot[MAXMODEL];
EXTERN char                  capclassname[MAXMODEL][MAXCAPNAMESIZE];     // Class name
EXTERN char                  capskinname[MAXMODEL][4][MAXCAPNAMESIZE];   // Skin name
EXTERN signed char           capskinoverride[MAXMODEL];                  // -1 or 0-3.. For import
EXTERN unsigned char         capleveloverride[MAXMODEL];                 // 0 for normal
EXTERN int                   capstateoverride[MAXMODEL];                 // 0 for normal
EXTERN int                   capcontentoverride[MAXMODEL];               // 0 for normal
EXTERN unsigned short        capskincost[MAXMODEL][4];                   // Store prices
EXTERN unsigned char         capskindressy[MAXMODEL];                    // Dressy
EXTERN float                 capstrengthdampen[MAXMODEL];                // Strength damage factor
EXTERN unsigned char         capstoppedby[MAXMODEL];                     // Collision Mask
EXTERN unsigned char         capuniformlit[MAXMODEL];                    // Bad lighting?
EXTERN unsigned char         caplifecolor[MAXMODEL];                     // Bar colors
EXTERN unsigned char         capmanacolor[MAXMODEL];                     //
EXTERN unsigned char         capammomax[MAXMODEL];                       // Ammo stuff
EXTERN unsigned char         capammo[MAXMODEL];                          //
EXTERN unsigned char         capgender[MAXMODEL];                        // Gender
EXTERN unsigned short        caplifebase[MAXMODEL];                      // Life
EXTERN unsigned short        capliferand[MAXMODEL];                      //
EXTERN unsigned short        caplifeperlevelbase[MAXMODEL];              //
EXTERN unsigned short        caplifeperlevelrand[MAXMODEL];              //
EXTERN signed short          caplifereturn[MAXMODEL];                    //
EXTERN signed short          capmoney[MAXMODEL];                         // Money
EXTERN unsigned short        caplifeheal[MAXMODEL];                      //
EXTERN unsigned short        capmanabase[MAXMODEL];                      // Mana
EXTERN unsigned short        capmanarand[MAXMODEL];                      //
EXTERN signed short          capmanacost[MAXMODEL];                      //
EXTERN unsigned short        capmanaperlevelbase[MAXMODEL];              //
EXTERN unsigned short        capmanaperlevelrand[MAXMODEL];              //
EXTERN unsigned short        capmanareturnbase[MAXMODEL];                //
EXTERN unsigned short        capmanareturnrand[MAXMODEL];                //
EXTERN unsigned short        capmanareturnperlevelbase[MAXMODEL];        //
EXTERN unsigned short        capmanareturnperlevelrand[MAXMODEL];        //
EXTERN unsigned short        capmanaflowbase[MAXMODEL];                  //
EXTERN unsigned short        capmanaflowrand[MAXMODEL];                  //
EXTERN unsigned short        capmanaflowperlevelbase[MAXMODEL];          //
EXTERN unsigned short        capmanaflowperlevelrand[MAXMODEL];          //
EXTERN unsigned short        capstrengthbase[MAXMODEL];                  // Strength
EXTERN unsigned short        capstrengthrand[MAXMODEL];                  //
EXTERN unsigned short        capstrengthperlevelbase[MAXMODEL];          //
EXTERN unsigned short        capstrengthperlevelrand[MAXMODEL];          //
EXTERN unsigned short        capwisdombase[MAXMODEL];                    // Wisdom
EXTERN unsigned short        capwisdomrand[MAXMODEL];                    //
EXTERN unsigned short        capwisdomperlevelbase[MAXMODEL];            //
EXTERN unsigned short        capwisdomperlevelrand[MAXMODEL];            //
EXTERN unsigned short        capintelligencebase[MAXMODEL];              // Intlligence
EXTERN unsigned short        capintelligencerand[MAXMODEL];              //
EXTERN unsigned short        capintelligenceperlevelbase[MAXMODEL];      //
EXTERN unsigned short        capintelligenceperlevelrand[MAXMODEL];      //
EXTERN unsigned short        capdexteritybase[MAXMODEL];                 // Dexterity
EXTERN unsigned short        capdexterityrand[MAXMODEL];                 //
EXTERN unsigned short        capdexterityperlevelbase[MAXMODEL];         //
EXTERN unsigned short        capdexterityperlevelrand[MAXMODEL];         //
EXTERN float                 capsize[MAXMODEL];                          // Scale of model
EXTERN float                 capsizeperlevel[MAXMODEL];                  // Scale increases
EXTERN float                 capdampen[MAXMODEL];                        // Bounciness
EXTERN unsigned char         capshadowsize[MAXMODEL];                    // Shadow size
EXTERN unsigned char         capbumpsize[MAXMODEL];                      // Bounding octagon
EXTERN unsigned char         capbumpsizebig[MAXMODEL];                   // For octagonal bumpers
EXTERN unsigned char         capbumpheight[MAXMODEL];                    //
EXTERN float                 capbumpdampen[MAXMODEL];                    // Mass
EXTERN unsigned char         capweight[MAXMODEL];                        // Weight
EXTERN float                 capjump[MAXMODEL];                          // Jump power
EXTERN unsigned char         capjumpnumber[MAXMODEL];                    // Number of jumps ( Ninja )
EXTERN unsigned char         capsneakspd[MAXMODEL];                      // Sneak threshold
EXTERN unsigned char         capwalkspd[MAXMODEL];                       // Walk threshold
EXTERN unsigned char         caprunspd[MAXMODEL];                        // Run threshold
EXTERN unsigned char         capflyheight[MAXMODEL];                     // Fly height
EXTERN unsigned char         capflashand[MAXMODEL];                      // Flashing rate
EXTERN unsigned char         capalpha[MAXMODEL];                         // Transparency
EXTERN unsigned char         caplight[MAXMODEL];                         // Light blending
EXTERN unsigned char         captransferblend[MAXMODEL];                 // Transfer blending to rider/weapons
EXTERN unsigned char         capsheen[MAXMODEL];                         // How shiny it is ( 0-15 )
EXTERN unsigned char         capenviro[MAXMODEL];                        // Phong map this baby?
EXTERN unsigned short        capuoffvel[MAXMODEL];                       // Texture movement rates
EXTERN unsigned short        capvoffvel[MAXMODEL];                       //
EXTERN unsigned char         capstickybutt[MAXMODEL];                    // Stick to the ground?
EXTERN unsigned short        capiframefacing[MAXMODEL];                  // Invincibility frame
EXTERN unsigned short        capiframeangle[MAXMODEL];                   //
EXTERN unsigned short        capnframefacing[MAXMODEL];                  // Normal frame
EXTERN unsigned short        capnframeangle[MAXMODEL];                   //
EXTERN unsigned char         capresistbumpspawn[MAXMODEL];               // Don't catch fire
EXTERN unsigned char         capdefense[MAXMODEL][4];                    // Defense for each skin
EXTERN unsigned char         capdamagemodifier[MAXMODEL][MAXDAMAGETYPE][4];
EXTERN float                 capmaxaccel[MAXMODEL][4];                   // Acceleration for each skin
EXTERN unsigned short        capexperienceforlevel[MAXMODEL][MAXLEVEL];  // Experience needed for next level
EXTERN unsigned short        capexperiencebase[MAXMODEL];                // Starting experience
EXTERN unsigned short        capexperiencerand[MAXMODEL];                //
EXTERN unsigned short        capexperienceworth[MAXMODEL];               // Amount given to killer/user
EXTERN float                 capexperienceexchange[MAXMODEL];            // Adds to worth
EXTERN float                 capexperiencerate[MAXMODEL][MAXEXPERIENCETYPE];
EXTERN unsigned int          capidsz[MAXMODEL][MAXIDSZ];                 // ID strings
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
EXTERN unsigned char         capdamagetargettype[MAXMODEL];              // For AI DamageTarget
EXTERN unsigned char         capweaponaction[MAXMODEL];                  // Animation needed to swing
EXTERN unsigned char         capgripvalid[MAXMODEL][2];                  // Left/Right hands valid
EXTERN unsigned char         capattackattached[MAXMODEL];                //
EXTERN signed char           capattackprttype[MAXMODEL];                 //
EXTERN unsigned char         capattachedprtamount[MAXMODEL];             // Sticky particles
EXTERN unsigned char         capattachedprtreaffirmdamagetype[MAXMODEL]; // Relight that torch...
EXTERN unsigned char         capattachedprttype[MAXMODEL];               //
EXTERN unsigned char         capgopoofprtamount[MAXMODEL];               // Poof effect
EXTERN signed short          capgopoofprtfacingadd[MAXMODEL];            //
EXTERN unsigned char         capgopoofprttype[MAXMODEL];                 //
EXTERN unsigned char         capbloodvalid[MAXMODEL];                    // Blood ( yuck )
EXTERN unsigned char         capbloodprttype[MAXMODEL];                  //
EXTERN signed char           capwavefootfall[MAXMODEL];                  // Footfall sound, -1
EXTERN signed char           capwavejump[MAXMODEL];                      // Jump sound, -1
EXTERN unsigned char         capridercanattack[MAXMODEL];                // Rider attack?
EXTERN bool_t                capcanbedazed[MAXMODEL];                    // Can it be dazed?
EXTERN bool_t                capcanbegrogged[MAXMODEL];                  // Can it be grogged?
EXTERN bool_t                capcanseeinvisible[MAXMODEL];               // Can it see invisible?
EXTERN bool_t                capcanseekurse[MAXMODEL];                   // Can it see kurses?
EXTERN unsigned char         capkursechance[MAXMODEL];                   // Chance of being kursed
EXTERN unsigned char         capistoobig[MAXMODEL];                      // Can't be put in pack
EXTERN unsigned char         capreflect[MAXMODEL];                       // Draw the reflection
EXTERN unsigned char         capalwaysdraw[MAXMODEL];                    // Always render
EXTERN unsigned char         capisranged[MAXMODEL];                      // Flag for ranged weapon
EXTERN signed char           caphidestate[MAXCHR];                       // Don't draw when...
EXTERN unsigned char         capisequipment[MAXCHR];                     // Behave in silly ways


// Particle template
#define DYNAOFF   0
#define DYNAON    1
#define DYNALOCAL 2
#define DYNAFANS  12
#define MAXFALLOFF 1400

EXTERN int                     numpip  EQ(0);
EXTERN unsigned char           pipforce[MAXPRTPIP];                        // Force spawn?
EXTERN unsigned char           piptype[MAXPRTPIP];                         // Transparency mode
EXTERN unsigned char           pipnumframes[MAXPRTPIP];                    // Number of frames
EXTERN unsigned char           pipimagebase[MAXPRTPIP];                    // Starting image
EXTERN unsigned short          pipimageadd[MAXPRTPIP];                     // Frame rate
EXTERN unsigned short          pipimageaddrand[MAXPRTPIP];                 // Frame rate randomness
EXTERN unsigned short          piptime[MAXPRTPIP];                         // Time until end
EXTERN unsigned short          piprotatebase[MAXPRTPIP];                   // Rotation
EXTERN unsigned short          piprotaterand[MAXPRTPIP];                   // Rotation
EXTERN signed short            piprotateadd[MAXPRTPIP];                    // Rotation rate
EXTERN unsigned short          pipsizebase[MAXPRTPIP];                     // Size
EXTERN signed short            pipsizeadd[MAXPRTPIP];                      // Size rate
EXTERN float                   pipspdlimit[MAXPRTPIP];                     // Speed limit
EXTERN float                   pipdampen[MAXPRTPIP];                       // Bounciness
EXTERN signed char             pipbumpmoney[MAXPRTPIP];                    // Value of particle
EXTERN unsigned char           pipbumpsize[MAXPRTPIP];                     // Bounding box size
EXTERN unsigned char           pipbumpheight[MAXPRTPIP];                   // Bounding box height
EXTERN unsigned char           pipendwater[MAXPRTPIP];                     // End if underwater
EXTERN unsigned char           pipendbump[MAXPRTPIP];                      // End if bumped
EXTERN unsigned char           pipendground[MAXPRTPIP];                    // End if on ground
EXTERN unsigned char           pipendwall[MAXPRTPIP];                      // End if hit a wall
EXTERN unsigned char           pipendlastframe[MAXPRTPIP];                 // End on last frame
EXTERN unsigned short          pipdamagebase[MAXPRTPIP];                   // Damage
EXTERN unsigned short          pipdamagerand[MAXPRTPIP];                   // Damage
EXTERN unsigned char           pipdamagetype[MAXPRTPIP];                   // Damage type
EXTERN signed short            pipfacingbase[MAXPRTPIP];                   // Facing
EXTERN unsigned short          pipfacingadd[MAXPRTPIP];                    // Facing
EXTERN unsigned short          pipfacingrand[MAXPRTPIP];                   // Facing
EXTERN signed short            pipxyspacingbase[MAXPRTPIP];                // Spacing
EXTERN unsigned short          pipxyspacingrand[MAXPRTPIP];                // Spacing
EXTERN signed short            pipzspacingbase[MAXPRTPIP];                 // Altitude
EXTERN unsigned short          pipzspacingrand[MAXPRTPIP];                 // Altitude
EXTERN signed char             pipxyvelbase[MAXPRTPIP];                    // Shot velocity
EXTERN unsigned char           pipxyvelrand[MAXPRTPIP];                    // Shot velocity
EXTERN signed char             pipzvelbase[MAXPRTPIP];                     // Up velocity
EXTERN unsigned char           pipzvelrand[MAXPRTPIP];                     // Up velocity
EXTERN unsigned short          pipcontspawntime[MAXPRTPIP];                // Spawn timer
EXTERN unsigned char           pipcontspawnamount[MAXPRTPIP];              // Spawn amount
EXTERN unsigned short          pipcontspawnfacingadd[MAXPRTPIP];           // Spawn in circle
EXTERN unsigned short          pipcontspawnpip[MAXPRTPIP];                 // Spawn type ( local )
EXTERN unsigned char           pipendspawnamount[MAXPRTPIP];               // Spawn amount
EXTERN unsigned short          pipendspawnfacingadd[MAXPRTPIP];            // Spawn in circle
EXTERN unsigned char           pipendspawnpip[MAXPRTPIP];                  // Spawn type ( local )
EXTERN unsigned char           pipbumpspawnamount[MAXPRTPIP];              // Spawn amount
EXTERN unsigned char           pipbumpspawnpip[MAXPRTPIP];                 // Spawn type ( global )
EXTERN unsigned char           pipdynalightmode[MAXPRTPIP];                // Dynamic light on?
EXTERN float                   pipdynalevel[MAXPRTPIP];                    // Intensity
EXTERN unsigned short          pipdynafalloff[MAXPRTPIP];                  // Falloff
EXTERN unsigned short          pipdazetime[MAXPRTPIP];                     // Daze
EXTERN unsigned short          pipgrogtime[MAXPRTPIP];                     // Drunkeness
EXTERN signed char             pipsoundspawn[MAXPRTPIP];                   // Beginning sound
EXTERN signed char             pipsoundend[MAXPRTPIP];                     // Ending sound
EXTERN signed char             pipsoundfloor[MAXPRTPIP];                   // Floor sound
EXTERN signed char             pipsoundwall[MAXPRTPIP];                    // Ricochet sound
EXTERN unsigned char           pipfriendlyfire[MAXPRTPIP];                 // Friendly fire
EXTERN unsigned char           piprotatetoface[MAXPRTPIP];                 // Arrows/Missiles
EXTERN unsigned char           pipnewtargetonspawn[MAXPRTPIP];             // Get new target?
EXTERN unsigned char           piphoming[MAXPRTPIP];                       // Homing?
EXTERN unsigned short          piptargetangle[MAXPRTPIP];                  // To find target
EXTERN float                   piphomingaccel[MAXPRTPIP];                  // Acceleration rate
EXTERN float                   piphomingfriction[MAXPRTPIP];               // Deceleration rate
EXTERN float                   pipdynalightleveladd[MAXPRTPIP];            // Dyna light changes
EXTERN float                   pipdynalightfalloffadd[MAXPRTPIP];          //
EXTERN unsigned char           piptargetcaster[MAXPRTPIP];                 // Target caster?
EXTERN unsigned char           pipspawnenchant[MAXPRTPIP];                 // Spawn enchant?
EXTERN unsigned char           pipneedtarget[MAXPRTPIP];                   // Need a target?
EXTERN unsigned char           piponlydamagefriendly[MAXPRTPIP];           // Only friends?
EXTERN unsigned char           pipstartontarget[MAXPRTPIP];                // Start on target?
EXTERN int                     pipzaimspd[MAXPRTPIP];                      // [ZSPD] For Z aiming
EXTERN unsigned short          pipdamfx[MAXPRTPIP];                        // Damage effects
EXTERN bool_t                  pipallowpush[MAXPRTPIP];                    //Allow particle to push characters around
EXTERN bool_t                  pipintdamagebonus[MAXPRTPIP];               //Add intelligence as damage bonus
EXTERN bool_t                  pipwisdamagebonus[MAXPRTPIP];               //Add wisdom as damage bonus


// The ID number for host searches
// {A0F72DE8-2C17-11d3-B7FE-444553540000}
/* PORT
DEFINE_GUID(NETWORKID, 0xa0f72de8, 0x2c17, 0x11d3, 0xb7, 0xfe, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
*/
#define MAXSERVICE 16
#define NETNAMESIZE 16
#define MAXSESSION 16
#define MAXNETPLAYER 8
EXTERN unsigned int			   randsave;									//Used in network timer
EXTERN int                     networkservice;
EXTERN char                    nethostname[64];                            // Name for hosting session
EXTERN char                    netmessagename[64];                         // Name for messages
EXTERN int                     numservice  EQ(0);                             // How many we found
EXTERN char                    netservicename[MAXSERVICE][NETNAMESIZE];    // Names of services
EXTERN int                     numsession  EQ(0);                             // How many we found
EXTERN char                    netsessionname[MAXSESSION][NETNAMESIZE];    // Names of sessions
EXTERN int                     numplayer  EQ(0);                              // How many we found
EXTERN char                    netplayername[MAXNETPLAYER][NETNAMESIZE];   // Names of machines
/* PORT
LPVOID                  netlpconnectionbuffer[MAXSERVICE];          // Location of service info
LPGUID                  netlpsessionguid[MAXSESSION];               // GUID for joining
DPID                    netplayerid[MAXNETPLAYER];                  // Player ID
DPID                    selfid;                                     // Player ID
*/


EXTERN char *globalname  EQ(NULL);  // For debuggin' goto_colon
EXTERN char *globalparsename  EQ(NULL); // The SCRIPT.TXT filename
EXTERN FILE *globalparseerr  EQ(NULL); // For debuggin' scripted AI
EXTERN FILE *globalnetworkerr  EQ(NULL); // For debuggin' network


EXTERN float           indextoenvirox[MD2LIGHTINDICES];                    // Environment map
EXTERN float           lighttoenviroy[256];                                // Environment map
EXTERN Uint32          lighttospek[MAXSPEKLEVEL][256];                     //


EXTERN float           hillslide  EQ(1.00);                                   //
EXTERN float           slippyfriction  EQ(1.00);  //1.05 for Chevron          // Friction
EXTERN float           airfriction  EQ(.95);                                  //
EXTERN float           waterfriction  EQ(.85);                                //
EXTERN float           noslipfriction  EQ(0.95);                              //
EXTERN float           platstick  EQ(.040);                                   //
EXTERN float           gravity  EQ((float) -1.0);                             // Gravitational accel
EXTERN int             damagetileamount  EQ(256);                             // Amount of damage
EXTERN unsigned char   damagetiletype  EQ(DAMAGEFIRE);                        // Type of damage


EXTERN char            cFrameName[16];                                     // MD2 Frame Name


EXTERN unsigned short  globesttarget;                                      // For find_target
EXTERN unsigned short  globestangle;                                       //
EXTERN unsigned short  glouseangle;                                        //
EXTERN int             globestdistance;


EXTERN unsigned int    numfanblock  EQ(0);                                    // Number of collision areas
EXTERN unsigned short  meshbumplistchr[MAXMESHFAN/16];                     // For character collisions
EXTERN unsigned short  meshbumplistchrnum[MAXMESHFAN/16];                  // Number on the block
EXTERN unsigned short  meshbumplistprt[MAXMESHFAN/16];                     // For particle collisions
EXTERN unsigned short  meshbumplistprtnum[MAXMESHFAN/16];                  // Number on the block
EXTERN unsigned char   meshexploremode  EQ(bfalse);                            // Explore mode?
EXTERN int             maxtotalmeshvertices  EQ(256*256*6);                   // of vertices
EXTERN int             meshsizex;                                          // Size in fansquares
EXTERN int             meshsizey;                                          //
EXTERN float           meshedgex;                                          // Limits !!!BAD!!!
EXTERN float           meshedgey;                                          //
EXTERN unsigned short  meshlasttexture;                                    // Last texture used
EXTERN unsigned char   meshtype[MAXMESHFAN];                               // Command type
EXTERN unsigned char   meshfx[MAXMESHFAN];                                 // Special effects flags
EXTERN unsigned char   meshtwist[MAXMESHFAN];                              //
EXTERN unsigned char   meshinrenderlist[MAXMESHFAN];                       // 
EXTERN unsigned short  meshtile[MAXMESHFAN];                               // Get texture from this
EXTERN unsigned int    meshvrtstart[MAXMESHFAN];                           // Which vertex to start at
EXTERN unsigned int    meshblockstart[(MAXMESHSIZEY/4)+1];
EXTERN unsigned int    meshfanstart[MAXMESHSIZEY];                         // Which fan to start a row with
//float           meshvrtx[MAXTOTALMESHVERTICES];                     // Vertex position
//float           meshvrty[MAXTOTALMESHVERTICES];                     //
//float           meshvrtz[MAXTOTALMESHVERTICES];                     // Vertex elevation
//unsigned char   meshvrta[MAXTOTALMESHVERTICES];                     // Vertex starting light
//unsigned char   meshvrtl[MAXTOTALMESHVERTICES];                     // Vertex light
EXTERN float *floatmemory;                                                 // For malloc
EXTERN float*          meshvrtx;                                           // Vertex position
EXTERN float*          meshvrty;                                           //
EXTERN float*          meshvrtz;                                           // Vertex elevation
EXTERN unsigned char*  meshvrta;                                           // Vertex base light
EXTERN unsigned char*  meshvrtl;                                           // Vertex light
EXTERN unsigned char   meshcommands[MAXMESHTYPE];                          // Number of commands
EXTERN unsigned char   meshcommandsize[MAXMESHTYPE][MAXMESHCOMMAND];       // Entries in each command
EXTERN unsigned short  meshcommandvrt[MAXMESHTYPE][MAXMESHCOMMANDENTRIES]; // Fansquare vertex list
EXTERN unsigned char   meshcommandnumvertices[MAXMESHTYPE];                // Number of vertices
EXTERN float           meshcommandu[MAXMESHTYPE][MAXMESHVERTICES];         // Vertex texture posi
EXTERN float           meshcommandv[MAXMESHTYPE][MAXMESHVERTICES];         //
EXTERN float           meshtileoffu[MAXTILETYPE];                          // Tile texture offset
EXTERN float           meshtileoffv[MAXTILETYPE];                          //
EXTERN int             nummeshrenderlist;                                  // Number to render, total
EXTERN int             nummeshrenderlistref;                               // ..., reflective
EXTERN int             nummeshrenderlistsha;                               // ..., shadow
EXTERN unsigned int    meshrenderlist[MAXMESHRENDER];                      // List of which to render, total
EXTERN unsigned int    meshrenderlistref[MAXMESHRENDER];                   // ..., reflective
EXTERN unsigned int    meshrenderlistsha[MAXMESHRENDER];                   // ..., shadow


EXTERN unsigned char   asciitofont[256];                                   // Conversion table


// Display messages
EXTERN unsigned short  msgtimechange EQ(0);                                    //
EXTERN unsigned short  msgstart EQ(0);                                         // The message queue
EXTERN signed short    msgtime[MAXMESSAGE];                                //
EXTERN char            msgtextdisplay[MAXMESSAGE][MESSAGESIZE];            // The displayed text
// Message files
EXTERN unsigned short  msgtotal EQ(0);                                         // The number of messages
EXTERN unsigned int    msgtotalindex EQ(0);                                    // Where to put letter
EXTERN unsigned int    msgindex[MAXTOTALMESSAGE];                          // Where it is
EXTERN char            msgtext[MESSAGEBUFFERSIZE];                         // The text buffer



// My lil' random number table
#define MAXRAND 4096
EXTERN unsigned short randie[MAXRAND];
EXTERN unsigned short randindex;
#define RANDIE randie[randindex];  randindex=(randindex+1)&(MAXRAND-1)


#define MAXENDTEXT 1024
EXTERN char generictext[80];         // Use for whatever purpose
EXTERN char endtext[MAXENDTEXT];     // The end-module text
EXTERN int endtextwrite;


// This is id's normal table for computing light values
#ifdef DECLARE_GLOBALS
float md2normals[MD2LIGHTINDICES][3] =
{{-0.5257,0.0000,0.8507},{-0.4429,0.2389,0.8642},{-0.2952,0.0000,0.9554},
 {-0.3090,0.5000,0.8090},{-0.1625,0.2629,0.9511},{0.0000,0.0000,1.0000},
 {0.0000,0.8507,0.5257},{-0.1476,0.7166,0.6817},{0.1476,0.7166,0.6817},
 {0.0000,0.5257,0.8507},{0.3090,0.5000,0.8090},{0.5257,0.0000,0.8507},
 {0.2952,0.0000,0.9554},{0.4429,0.2389,0.8642},{0.1625,0.2629,0.9511},
 {-0.6817,0.1476,0.7166},{-0.8090,0.3090,0.5000},{-0.5878,0.4253,0.6882},
 {-0.8507,0.5257,0.0000},{-0.8642,0.4429,0.2389},{-0.7166,0.6817,0.1476},
 {-0.6882,0.5878,0.4253},{-0.5000,0.8090,0.3090},{-0.2389,0.8642,0.4429},
 {-0.4253,0.6882,0.5878},{-0.7166,0.6817,-0.1476},{-0.5000,0.8090,-0.3090},
 {-0.5257,0.8507,0.0000},{0.0000,0.8507,-0.5257},{-0.2389,0.8642,-0.4429},
 {0.0000,0.9554,-0.2952},{-0.2629,0.9511,-0.1625},{0.0000,1.0000,0.0000},
 {0.0000,0.9554,0.2952},{-0.2629,0.9511,0.1625},{0.2389,0.8642,0.4429},
 {0.2629,0.9511,0.1625},{0.5000,0.8090,0.3090},{0.2389,0.8642,-0.4429},
 {0.2629,0.9511,-0.1625},{0.5000,0.8090,-0.3090},{0.8507,0.5257,0.0000},
 {0.7166,0.6817,0.1476},{0.7166,0.6817,-0.1476},{0.5257,0.8507,0.0000},
 {0.4253,0.6882,0.5878},{0.8642,0.4429,0.2389},{0.6882,0.5878,0.4253},
 {0.8090,0.3090,0.5000},{0.6817,0.1476,0.7166},{0.5878,0.4253,0.6882},
 {0.9554,0.2952,0.0000},{1.0000,0.0000,0.0000},{0.9511,0.1625,0.2629},
 {0.8507,-0.5257,0.0000},{0.9554,-0.2952,0.0000},{0.8642,-0.4429,0.2389},
 {0.9511,-0.1625,0.2629},{0.8090,-0.3090,0.5000},{0.6817,-0.1476,0.7166},
 {0.8507,0.0000,0.5257},{0.8642,0.4429,-0.2389},{0.8090,0.3090,-0.5000},
 {0.9511,0.1625,-0.2629},{0.5257,0.0000,-0.8507},{0.6817,0.1476,-0.7166},
 {0.6817,-0.1476,-0.7166},{0.8507,0.0000,-0.5257},{0.8090,-0.3090,-0.5000},
 {0.8642,-0.4429,-0.2389},{0.9511,-0.1625,-0.2629},{0.1476,0.7166,-0.6817},
 {0.3090,0.5000,-0.8090},{0.4253,0.6882,-0.5878},{0.4429,0.2389,-0.8642},
 {0.5878,0.4253,-0.6882},{0.6882,0.5878,-0.4253},{-0.1476,0.7166,-0.6817},
 {-0.3090,0.5000,-0.8090},{0.0000,0.5257,-0.8507},{-0.5257,0.0000,-0.8507},
 {-0.4429,0.2389,-0.8642},{-0.2952,0.0000,-0.9554},{-0.1625,0.2629,-0.9511},
 {0.0000,0.0000,-1.0000},{0.2952,0.0000,-0.9554},{0.1625,0.2629,-0.9511},
 {-0.4429,-0.2389,-0.8642},{-0.3090,-0.5000,-0.8090},{-0.1625,-0.2629,-0.9511},
 {0.0000,-0.8507,-0.5257},{-0.1476,-0.7166,-0.6817},{0.1476,-0.7166,-0.6817},
 {0.0000,-0.5257,-0.8507},{0.3090,-0.5000,-0.8090},{0.4429,-0.2389,-0.8642},
 {0.1625,-0.2629,-0.9511},{0.2389,-0.8642,-0.4429},{0.5000,-0.8090,-0.3090},
 {0.4253,-0.6882,-0.5878},{0.7166,-0.6817,-0.1476},{0.6882,-0.5878,-0.4253},
 {0.5878,-0.4253,-0.6882},{0.0000,-0.9554,-0.2952},{0.0000,-1.0000,0.0000},
 {0.2629,-0.9511,-0.1625},{0.0000,-0.8507,0.5257},{0.0000,-0.9554,0.2952},
 {0.2389,-0.8642,0.4429},{0.2629,-0.9511,0.1625},{0.5000,-0.8090,0.3090},
 {0.7166,-0.6817,0.1476},{0.5257,-0.8507,0.0000},{-0.2389,-0.8642,-0.4429},
 {-0.5000,-0.8090,-0.3090},{-0.2629,-0.9511,-0.1625},{-0.8507,-0.5257,0.0000},
 {-0.7166,-0.6817,-0.1476},{-0.7166,-0.6817,0.1476},{-0.5257,-0.8507,0.0000},
 {-0.5000,-0.8090,0.3090},{-0.2389,-0.8642,0.4429},{-0.2629,-0.9511,0.1625},
 {-0.8642,-0.4429,0.2389},{-0.8090,-0.3090,0.5000},{-0.6882,-0.5878,0.4253},
 {-0.6817,-0.1476,0.7166},{-0.4429,-0.2389,0.8642},{-0.5878,-0.4253,0.6882},
 {-0.3090,-0.5000,0.8090},{-0.1476,-0.7166,0.6817},{-0.4253,-0.6882,0.5878},
 {-0.1625,-0.2629,0.9511},{0.4429,-0.2389,0.8642},{0.1625,-0.2629,0.9511},
 {0.3090,-0.5000,0.8090},{0.1476,-0.7166,0.6817},{0.0000,-0.5257,0.8507},
 {0.4253,-0.6882,0.5878},{0.5878,-0.4253,0.6882},{0.6882,-0.5878,0.4253},
 {-0.9554,0.2952,0.0000},{-0.9511,0.1625,0.2629},{-1.0000,0.0000,0.0000},
 {-0.8507,0.0000,0.5257},{-0.9554,-0.2952,0.0000},{-0.9511,-0.1625,0.2629},
 {-0.8642,0.4429,-0.2389},{-0.9511,0.1625,-0.2629},{-0.8090,0.3090,-0.5000},
 {-0.8642,-0.4429,-0.2389},{-0.9511,-0.1625,-0.2629},{-0.8090,-0.3090,-0.5000},
 {-0.6817,0.1476,-0.7166},{-0.6817,-0.1476,-0.7166},{-0.8507,0.0000,-0.5257},
 {-0.6882,0.5878,-0.4253},{-0.5878,0.4253,-0.6882},{-0.4253,0.6882,-0.5878},
 {-0.4253,-0.6882,-0.5878},{-0.5878,-0.4253,-0.6882},{-0.6882,-0.5878,-0.4253},
 {0,0,0}  // Spikey mace
};
#else
EXTERN float md2normals[MD2LIGHTINDICES][3];
#endif

// This is for random naming
EXTERN unsigned short          numchop  EQ(0);                // The number of name parts
EXTERN unsigned short          chopwrite  EQ(0);              // The data pointer
EXTERN char                    chopdata[CHOPDATACHUNK];    // The name parts
EXTERN unsigned short          chopstart[MAXCHOP];         // The first character of each part
EXTERN unsigned short          capsectionsize[MAXMODEL][MAXSECTION];   // Number of choices, 0
EXTERN unsigned short          capsectionstart[MAXMODEL][MAXSECTION];  //
EXTERN char                    namingnames[MAXCAPNAMESIZE];// The name returned by the function



// These are for the AI script loading/parsing routines
extern int                     iNumAis;

#define ALERTIFSPAWNED                      1           // 0
#define ALERTIFHITVULNERABLE                2           // 1
#define ALERTIFATWAYPOINT                   4           // 2
#define ALERTIFATLASTWAYPOINT               8           // 3
#define ALERTIFATTACKED                     16          // 4
#define ALERTIFBUMPED                       32          // 5
#define ALERTIFORDERED                      64          // 6
#define ALERTIFCALLEDFORHELP                128         // 7
#define ALERTIFKILLED                       256         // 8
#define ALERTIFTARGETKILLED                 512         // 9
#define ALERTIFDROPPED                      1024        // 10
#define ALERTIFGRABBED                      2048        // 11
#define ALERTIFREAFFIRMED                   4096        // 12
#define ALERTIFLEADERKILLED                 8192        // 13
#define ALERTIFUSED                         16384       // 14
#define ALERTIFCLEANEDUP                    32768       // 15
#define ALERTIFSCOREDAHIT                   65536       // 16
#define ALERTIFHEALED                       131072      // 17
#define ALERTIFDISAFFIRMED                  262144      // 18
#define ALERTIFCHANGED                      524288      // 19
#define ALERTIFINWATER                      1048576     // 20
#define ALERTIFBORED                        2097152     // 21
#define ALERTIFTOOMUCHBAGGAGE               4194304     // 22
#define ALERTIFGROGGED                      8388608     // 23
#define ALERTIFDAZED                        16777216    // 24
#define ALERTIFHITGROUND                    33554432    // 25
#define ALERTIFNOTDROPPED                   67108864    // 26
#define ALERTIFBLOCKED                      134217728   // 27
#define ALERTIFTHROWN                       268435456   // 28
#define ALERTIFCRUSHED                      536870912   // 29
#define ALERTIFNOTPUTAWAY                   1073741824  // 30
#define ALERTIFTAKENOUT                     2147483648  // 31

#define FIFSPAWNED                          0   // Scripted AI functions (v0.10)
#define FIFTIMEOUT                          1   //
#define FIFATWAYPOINT                       2   //
#define FIFATLASTWAYPOINT                   3   //
#define FIFATTACKED                         4   //
#define FIFBUMPED                           5   //
#define FIFORDERED                          6   //
#define FIFCALLEDFORHELP                    7   //
#define FSETCONTENT                         8   //
#define FIFKILLED                           9   //
#define FIFTARGETKILLED                     10  //
#define FCLEARWAYPOINTS                     11  //
#define FADDWAYPOINT                        12  //
#define FFINDPATH                           13  //
#define FCOMPASS                            14  //
#define FGETTARGETARMORPRICE                15  //
#define FSETTIME                            16  //
#define FGETCONTENT                         17  //
#define FJOINTARGETTEAM                     18  //
#define FSETTARGETTONEARBYENEMY             19  //
#define FSETTARGETTOTARGETLEFTHAND          20  //
#define FSETTARGETTOTARGETRIGHTHAND         21  //
#define FSETTARGETTOWHOEVERATTACKED         22  //
#define FSETTARGETTOWHOEVERBUMPED           23  //
#define FSETTARGETTOWHOEVERCALLEDFORHELP    24  //
#define FSETTARGETTOOLDTARGET               25  //
#define FSETTURNMODETOVELOCITY              26  //
#define FSETTURNMODETOWATCH                 27  //
#define FSETTURNMODETOSPIN                  28  //
#define FSETBUMPHEIGHT                      29  //
#define FIFTARGETHASID                      30  //
#define FIFTARGETHASITEMID                  31  //
#define FIFTARGETHOLDINGITEMID              32  //
#define FIFTARGETHASSKILLID                 33  //
#define FELSE                               34  //
#define FRUN                                35  //
#define FWALK                               36  //
#define FSNEAK                              37  //
#define FDOACTION                           38  //
#define FKEEPACTION                         39  //
#define FISSUEORDER                         40  //
#define FDROPWEAPONS                        41  //
#define FTARGETDOACTION                     42  //
#define FOPENPASSAGE                        43  //
#define FCLOSEPASSAGE                       44  //
#define FIFPASSAGEOPEN                      45  //
#define FGOPOOF                             46  //
#define FCOSTTARGETITEMID                   47  //
#define FDOACTIONOVERRIDE                   48  //
#define FIFHEALED                           49  //
#define FSENDMESSAGE                        50  //
#define FCALLFORHELP                        51  //
#define FADDIDSZ                            52  //
#define FEND                                53  //
#define FSETSTATE                           54  // Scripted AI functions (v0.20)
#define FGETSTATE                           55  //
#define FIFSTATEIS                          56  //
#define FIFTARGETCANOPENSTUFF               57  // Scripted AI functions (v0.30)
#define FIFGRABBED                          58  //
#define FIFDROPPED                          59  //
#define FSETTARGETTOWHOEVERISHOLDING        60  //
#define FDAMAGETARGET                       61  //
#define FIFXISLESSTHANY                     62  //
#define FSETWEATHERTIME                     63  // Scripted AI functions (v0.40)
#define FGETBUMPHEIGHT                      64  //
#define FIFREAFFIRMED                       65  //
#define FUNKEEPACTION                       66  //
#define FIFTARGETISONOTHERTEAM              67  //
#define FIFTARGETISONHATEDTEAM              68  // Scripted AI functions (v0.50)
#define FPRESSLATCHBUTTON                   69  //
#define FSETTARGETTOTARGETOFLEADER          70  //
#define FIFLEADERKILLED                     71  //
#define FBECOMELEADER                       72  //
#define FCHANGETARGETARMOR                  73  // Scripted AI functions (v0.60)
#define FGIVEMONEYTOTARGET                  74  //
#define FDROPKEYS                           75  //
#define FIFLEADERISALIVE                    76  //
#define FIFTARGETISOLDTARGET                77  //
#define FSETTARGETTOLEADER                  78  //
#define FSPAWNCHARACTER                     79  //
#define FRESPAWNCHARACTER                   80  //
#define FCHANGETILE                         81  //
#define FIFUSED                             82  //
#define FDROPMONEY                          83  //
#define FSETOLDTARGET                       84  //
#define FDETACHFROMHOLDER                   85  //
#define FIFTARGETHASVULNERABILITYID         86  //
#define FCLEANUP                            87  //
#define FIFCLEANEDUP                        88  //
#define FIFSITTING                          89  //
#define FIFTARGETISHURT                     90  //
#define FIFTARGETISAPLAYER                  91  //
#define FPLAYSOUND                          92  //
#define FSPAWNPARTICLE                      93  //
#define FIFTARGETISALIVE                    94  //
#define FSTOP                               95  //
#define FDISAFFIRMCHARACTER                 96  //
#define FREAFFIRMCHARACTER                  97  //
#define FIFTARGETISSELF                     98  //
#define FIFTARGETISMALE                     99  //
#define FIFTARGETISFEMALE                   100 //
#define FSETTARGETTOSELF                    101 // Scripted AI functions (v0.70)
#define FSETTARGETTORIDER                   102 //
#define FGETATTACKTURN                      103 //
#define FGETDAMAGETYPE                      104 //
#define FBECOMESPELL                        105 //
#define FBECOMESPELLBOOK                    106 //
#define FIFSCOREDAHIT                       107 //
#define FIFDISAFFIRMED                      108 //
#define FTRANSLATEORDER                     109 //
#define FSETTARGETTOWHOEVERWASHIT           110 //
#define FSETTARGETTOWIDEENEMY               111 //
#define FIFCHANGED                          112 //
#define FIFINWATER                          113 //
#define FIFBORED                            114 //
#define FIFTOOMUCHBAGGAGE                   115 //
#define FIFGROGGED                          116 //
#define FIFDAZED                            117 //
#define FIFTARGETHASSPECIALID               118 //
#define FPRESSTARGETLATCHBUTTON             119 //
#define FIFINVISIBLE                        120 //
#define FIFARMORIS                          121 //
#define FGETTARGETGROGTIME                  122 //
#define FGETTARGETDAZETIME                  123 //
#define FSETDAMAGETYPE                      124 //
#define FSETWATERLEVEL                      125 //
#define FENCHANTTARGET                      126 //
#define FENCHANTCHILD                       127 //
#define FTELEPORTTARGET                     128 //
#define FGIVEEXPERIENCETOTARGET             129 //
#define FINCREASEAMMO                       130 //
#define FUNKURSETARGET                      131 //
#define FGIVEEXPERIENCETOTARGETTEAM         132 //
#define FIFUNARMED                          133 //
#define FRESTOCKTARGETAMMOIDALL             134 //
#define FRESTOCKTARGETAMMOIDFIRST           135 //
#define FFLASHTARGET                        136 //
#define FSETREDSHIFT                        137 //
#define FSETGREENSHIFT                      138 //
#define FSETBLUESHIFT                       139 //
#define FSETLIGHT                           140 //
#define FSETALPHA                           141 //
#define FIFHITFROMBEHIND                    142 //
#define FIFHITFROMFRONT                     143 //
#define FIFHITFROMLEFT                      144 //
#define FIFHITFROMRIGHT                     145 //
#define FIFTARGETISONSAMETEAM               146 //
#define FKILLTARGET                         147 //
#define FUNDOENCHANT                        148 //
#define FGETWATERLEVEL                      149 //
#define FCOSTTARGETMANA                     150 //
#define FIFTARGETHASANYID                   151 //
#define FSETBUMPSIZE                        152 //
#define FIFNOTDROPPED                       153 //
#define FIFYISLESSTHANX                     154 //
#define FSETFLYHEIGHT                       155 //
#define FIFBLOCKED                          156 //
#define FIFTARGETISDEFENDING                157 //
#define FIFTARGETISATTACKING                158 //
#define FIFSTATEIS0                         159 //
#define FIFSTATEIS1                         160 //
#define FIFSTATEIS2                         161 //
#define FIFSTATEIS3                         162 //
#define FIFSTATEIS4                         163 //
#define FIFSTATEIS5                         164 //
#define FIFSTATEIS6                         165 //
#define FIFSTATEIS7                         166 //
#define FIFCONTENTIS                        167 //
#define FSETTURNMODETOWATCHTARGET           168 //
#define FIFSTATEISNOT                       169 //
#define FIFXISEQUALTOY                      170 //
#define FDEBUGMESSAGE                       171 //
#define FBLACKTARGET                        172 // Scripted AI functions (v0.80)
#define FSENDMESSAGENEAR                    173 //
#define FIFHITGROUND                        174 //
#define FIFNAMEISKNOWN                      175 //
#define FIFUSAGEISKNOWN                     176 //
#define FIFHOLDINGITEMID                    177 //
#define FIFHOLDINGRANGEDWEAPON              178 //
#define FIFHOLDINGMELEEWEAPON               179 //
#define FIFHOLDINGSHIELD                    180 //
#define FIFKURSED                           181 //
#define FIFTARGETISKURSED                   182 //
#define FIFTARGETISDRESSEDUP                183 //
#define FIFOVERWATER                        184 //
#define FIFTHROWN                           185 //
#define FMAKENAMEKNOWN                      186 //
#define FMAKEUSAGEKNOWN                     187 //
#define FSTOPTARGETMOVEMENT                 188 //
#define FSETXY                              189 //
#define FGETXY                              190 //
#define FADDXY                              191 //
#define FMAKEAMMOKNOWN                      192 //
#define FSPAWNATTACHEDPARTICLE              193 //
#define FSPAWNEXACTPARTICLE                 194 //
#define FACCELERATETARGET                   195 //
#define FIFDISTANCEISMORETHANTURN           196 //
#define FIFCRUSHED                          197 //
#define FMAKECRUSHVALID                     198 //
#define FSETTARGETTOLOWESTTARGET            199 //
#define FIFNOTPUTAWAY                       200 //
#define FIFTAKENOUT                         201 //
#define FIFAMMOOUT                          202 //
#define FPLAYSOUNDLOOPED                    203 //
#define FSTOPSOUND                          204 //
#define FHEALSELF                           205 //
#define FEQUIP                              206 //
#define FIFTARGETHASITEMIDEQUIPPED          207 //
#define FSETOWNERTOTARGET                   208 //
#define FSETTARGETTOOWNER                   209 //
#define FSETFRAME                           210 //
#define FBREAKPASSAGE                       211 //
#define FSETRELOADTIME                      212 //
#define FSETTARGETTOWIDEBLAHID              213 //
#define FPOOFTARGET                         214 //
#define FCHILDDOACTIONOVERRIDE              215 //
#define FSPAWNPOOF                          216 //
#define FSETSPEEDPERCENT                    217 //
#define FSETCHILDSTATE                      218 //
#define FSPAWNATTACHEDSIZEDPARTICLE         219 //
#define FCHANGEARMOR                        220 //
#define FSHOWTIMER                          221 //
#define FIFFACINGTARGET                     222 //
#define FPLAYSOUNDVOLUME                    223 //
#define FSPAWNATTACHEDFACEDPARTICLE         224 //
#define FIFSTATEISODD                       225 //
#define FSETTARGETTODISTANTENEMY            226 //
#define FTELEPORT                           227 //
#define FGIVESTRENGTHTOTARGET               228 //
#define FGIVEWISDOMTOTARGET                 229 //
#define FGIVEINTELLIGENCETOTARGET           230 //
#define FGIVEDEXTERITYTOTARGET              231 //
#define FGIVELIFETOTARGET                   232 //
#define FGIVEMANATOTARGET                   233 //
#define FSHOWMAP                            234 //
#define FSHOWYOUAREHERE                     235 //
#define FSHOWBLIPXY                         236 //
#define FHEALTARGET                         237 //
#define FPUMPTARGET                         238 //
#define FCOSTAMMO                           239 //
#define FMAKESIMILARNAMESKNOWN              240 //
#define FSPAWNATTACHEDHOLDERPARTICLE        241 //
#define FSETTARGETRELOADTIME                242 //
#define FSETFOGLEVEL                        243 //
#define FGETFOGLEVEL                        244 //
#define FSETFOGTAD                          245 //
#define FSETFOGBOTTOMLEVEL                  246 //
#define FGETFOGBOTTOMLEVEL                  247 //
#define FCORRECTACTIONFORHAND               248 //
#define FIFTARGETISMOUNTED                  249 //
#define FSPARKLEICON                        250 //
#define FUNSPARKLEICON                      251 //
#define FGETTILEXY                          252 //
#define FSETTILEXY                          253 //
#define FSETSHADOWSIZE                      254 //
#define FORDERTARGET                        255 //
#define FSETTARGETTOWHOEVERISINPASSAGE      256 //
#define FIFCHARACTERWASABOOK                257 //
#define FSETENCHANTBOOSTVALUES              258 // Scripted AI functions (v0.90)
#define FSPAWNCHARACTERXYZ                  259 //
#define FSPAWNEXACTCHARACTERXYZ             260 //
#define FCHANGETARGETCLASS                  261 //
#define FPLAYFULLSOUND                      262 //
#define FSPAWNEXACTCHASEPARTICLE            263 //
#define FCREATEORDER                        264 //
#define FORDERSPECIALID                     265 //
#define FUNKURSETARGETINVENTORY             266 //
#define FIFTARGETISSNEAKING                 267 //
#define FDROPITEMS                          268 //
#define FRESPAWNTARGET                      269 //
#define FTARGETDOACTIONSETFRAME             270 //
#define FIFTARGETCANSEEINVISIBLE            271 //
#define FSETTARGETTONEARESTBLAHID           272 //
#define FSETTARGETTONEARESTENEMY            273 //
#define FSETTARGETTONEARESTFRIEND           274 //
#define FSETTARGETTONEARESTLIFEFORM         275 //
#define FFLASHPASSAGE                       276 //
#define FFINDTILEINPASSAGE                  277 //
#define FIFHELDINLEFTHAND                   278 //
#define FNOTANITEM                          279 //
#define FSETCHILDAMMO                       280 //
#define FIFHITVULNERABLE                    281 //
#define FIFTARGETISFLYING                   282 //
#define FIDENTIFYTARGET                     283 //
#define FBEATMODULE                         284 //
#define FENDMODULE                          285 //
#define FDISABLEEXPORT                      286 //
#define FENABLEEXPORT                       287 //
#define FGETTARGETSTATE                     288 //
#define FIFEQUIPPED                         289 //Redone in v 0.95
#define FDROPTARGETMONEY                    290 //
#define FGETTARGETCONTENT	                291 //
#define FDROPTARGETKEYS                     292 //
#define FJOINTEAM		                    293 //
#define FTARGETJOINTEAM	                    294 //
#define FCLEARMUSICPASSAGE                  295 //Below is original code again
#define FCLEARENDMESSAGE                    296 //
#define FADDENDMESSAGE                      297 //
#define FPLAYMUSIC                          298 //
#define FSETMUSICPASSAGE                    299 //
#define FMAKECRUSHINVALID                   300 //
#define FSTOPMUSIC                          301 //
#define FFLASHVARIABLE                      302 //
#define FACCELERATEUP                       303 //
#define FFLASHVARIABLEHEIGHT                304 //
#define FSETDAMAGETIME                      305 //
#define FIFSTATEIS8                         306 //
#define FIFSTATEIS9                         307 //
#define FIFSTATEIS10                        308 //
#define FIFSTATEIS11                        309 //
#define FIFSTATEIS12                        310 //
#define FIFSTATEIS13                        311 //
#define FIFSTATEIS14                        312 //
#define FIFSTATEIS15                        313 //
#define FIFTARGETISAMOUNT                   314 //
#define FIFTARGETISAPLATFORM                315 //
#define FADDSTAT                            316 //
#define FDISENCHANTTARGET                   317 //
#define FDISENCHANTALL                      318 //
#define FSETVOLUMENEARESTTEAMMATE           319 //
#define FADDSHOPPASSAGE                     320 //
#define FTARGETPAYFORARMOR                  321 //
#define FJOINEVILTEAM                       322 //
#define FJOINNULLTEAM                       323 //
#define FJOINGOODTEAM                       324 //
#define FPITSKILL                           325 //
#define FSETTARGETTOPASSAGEID               326 //
#define FMAKENAMEUNKNOWN                    327 //
#define FSPAWNEXACTPARTICLEENDSPAWN         328 //
#define FSPAWNPOOFSPEEDSPACINGDAMAGE        329 //
#define FGIVEEXPERIENCETOGOODTEAM           330 //
#define FDONOTHING                          331 // Scripted AI functions (v0.95)
#define FGROGTARGET                         332 //
#define FDAZETARGET                         333 //

#define OPADD 0								// +
#define OPSUB 1								// -
#define OPAND 2								// &
#define OPSHR 3								// >
#define OPSHL 4								// <
#define OPMUL 5								// *
#define OPDIV 6								// /
#define OPMOD 7								// %

#define VARTMPX             0
#define VARTMPY             1
#define VARTMPDISTANCE      2
#define VARTMPTURN          3
#define VARTMPARGUMENT      4
#define VARRAND             5
#define VARSELFX            6
#define VARSELFY            7
#define VARSELFTURN         8
#define VARSELFCOUNTER      9
#define VARSELFORDER        10
#define VARSELFMORALE       11
#define VARSELFLIFE         12
#define VARTARGETX          13
#define VARTARGETY          14
#define VARTARGETDISTANCE   15
#define VARTARGETTURN       16
#define VARLEADERX          17
#define VARLEADERY          18
#define VARLEADERDISTANCE   19
#define VARLEADERTURN       20
#define VARGOTOX            21
#define VARGOTOY            22
#define VARGOTODISTANCE     23
#define VARTARGETTURNTO     24
#define VARPASSAGE          25
#define VARWEIGHT           26
#define VARSELFALTITUDE     27
#define VARSELFID           28
#define VARSELFHATEID       29
#define VARSELFMANA         30
#define VARTARGETSTR        31
#define VARTARGETWIS        32
#define VARTARGETINT        33
#define VARTARGETDEX        34
#define VARTARGETLIFE       35
#define VARTARGETMANA       36
#define VARTARGETLEVEL      37
#define VARTARGETSPEEDX     38
#define VARTARGETSPEEDY     39
#define VARTARGETSPEEDZ     40
#define VARSELFSPAWNX       41
#define VARSELFSPAWNY       42
#define VARSELFSTATE        43
#define VARSELFSTR          44
#define VARSELFWIS          45
#define VARSELFINT          46
#define VARSELFDEX          47
#define VARSELFMANAFLOW     48
#define VARTARGETMANAFLOW   49
#define VARSELFATTACHED     50
#define VARSWINGTURN        51
#define VARXYDISTANCE       52
#define VARSELFZ            53
#define VARTARGETALTITUDE   54
#define VARTARGETZ          55
#define VARSELFINDEX        56
#define VAROWNERX           57
#define VAROWNERY           58
#define VAROWNERTURN        59
#define VAROWNERDISTANCE    60
#define VAROWNERTURNTO      61
#define VARXYTURNTO         62
#define VARSELFMONEY        63
#define VARSELFACCEL        64
#define VARTARGETEXP        65
#define VARSELFAMMO         66
#define VARTARGETAMMO       67
#define VARTARGETMONEY      68
#define VARTARGETTURNAWAY   69
#define VARSELFLEVEL	    70

EXTERN unsigned short valueoldtarget EQ(0);
EXTERN int valuetmpx EQ(0);
EXTERN int valuetmpy EQ(0);
EXTERN unsigned valuetmpturn EQ(0);
EXTERN int valuetmpdistance EQ(0);
EXTERN int valuetmpargument EQ(0);
EXTERN unsigned int valuelastindent EQ(0);
EXTERN int valueoperationsum EQ(0);
EXTERN unsigned char valuegopoof;

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


//Passages
EXTERN int numpassage;							//Number of passages in the module
EXTERN int passtlx[MAXPASS];					//Passage positions
EXTERN int passtly[MAXPASS];
EXTERN int passbrx[MAXPASS];
EXTERN int passbry[MAXPASS];
EXTERN int passagemusic[MAXPASS];				//Music track appointed to the specific passage
EXTERN unsigned char passmask[MAXPASS];
EXTERN unsigned char passopen[MAXPASS];			//Is the passage open?

// For shops
EXTERN int numshoppassage;
EXTERN unsigned short shoppassage[MAXPASS];  // The passage number
EXTERN unsigned short shopowner[MAXPASS];    // Who gets the gold?
#define NOOWNER 65535


// Status displays
EXTERN int numstat  EQ(0);
EXTERN unsigned short statlist[MAXSTAT];
EXTERN int statdelay  EQ(0);
EXTERN Uint32 particletrans  EQ(0x80000000);
EXTERN Uint32 antialiastrans  EQ(0xC0000000);


//Network Stuff
#define SHORTLATCH 1024.0
#define CHARVEL 5.0
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
EXTERN unsigned char           plavalid[MAXPLAYER];                    // Player used?
EXTERN unsigned short          plaindex[MAXPLAYER];                    // Which character?
EXTERN float                   plalatchx[MAXPLAYER];                   // Local latches
EXTERN float                   plalatchy[MAXPLAYER];                   //
EXTERN unsigned char           plalatchbutton[MAXPLAYER];              //
EXTERN unsigned int            numplatimes;
EXTERN float                   platimelatchx[MAXPLAYER][MAXLAG];       // Timed latches
EXTERN float                   platimelatchy[MAXPLAYER][MAXLAG];       //
EXTERN unsigned char           platimelatchbutton[MAXPLAYER][MAXLAG];  //
EXTERN unsigned char           pladevice[MAXPLAYER];                   // Input device
EXTERN int                     numpla;                                 // Number of players
EXTERN int                     numlocalpla;                            //
EXTERN int                     lag  EQ(3);                                // Lag tolerance
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
extern unsigned int	nexttimestamp;								// Expected timestamp


//Sound using SDL_Mixer
EXTERN bool_t	        mixeron EQ(bfalse);			//Is the SDL_Mixer loaded?
#define MAXWAVE         16							 // Up to 16 waves per model
#define VOLMIN          -4000						// Minumum Volume level
#define VOLUMERATIO     7							 // Volume ratio
EXTERN Mix_Chunk		*capwaveindex[MAXMODEL][MAXWAVE];    //sounds in a object
EXTERN int              maxsoundchannel;			//Max number of sounds playing at the same time
EXTERN int              buffersize;					//Buffer size set in setup.txt
EXTERN Mix_Chunk		*globalwave[10];			//All sounds loaded into memory
EXTERN Mix_Chunk		*sound;						//Used for playing one selected sound file
EXTERN bool_t			soundvalid;					//Allow playing of sound?
EXTERN int              soundvolume;				//Volume of sounds played
EXTERN int              channel;					//Which channel the current sound is using

//Music using SDL_Mixer
#define MAXPLAYLISTLENGHT 25						//Max number of different tracks loaded into memory
EXTERN bool_t			musicvalid;					// Allow music and loops?
EXTERN int				musicvolume;				//The sound volume of music
EXTERN bool_t			musicinmemory EQ(bfalse);	//Is the music loaded in memory?
EXTERN Mix_Music        *instrumenttosound[MAXPLAYLISTLENGHT];	//This is a specific music file loaded into memory
EXTERN int              songplaying EQ(-1);				//Current song that is playing



//Some various other stuff
EXTERN char valueidsz[5];
EXTERN unsigned char changed;

//Key/Control input defenitions
#define MAXTAG              128                     // Number of tags in scancode.txt
#define TAGSIZE             32                      // Size of each tag
EXTERN int numscantag;
EXTERN char tagname[MAXTAG][TAGSIZE];                      // Scancode names
EXTERN unsigned int tagvalue[MAXTAG];                     // Scancode values
#define MAXCONTROL          64
EXTERN unsigned int controlvalue[MAXCONTROL];             // The scancode or mask
EXTERN unsigned int controliskey[MAXCONTROL];             // Is it a key?
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

//AI Targeting
EXTERN unsigned short globalnearest;
EXTERN float globaldistance;

#define ABS(X)  (((X) > 0) ? (X) : -(X))
#define min(x, y)  (((x) > (y)) ? (y) : (x))
#define max(x, y)  (((x) > (y)) ? (x) : (y))

// SDL specific declarations
EXTERN SDL_Joystick *sdljoya EQ(NULL);
EXTERN SDL_Joystick *sdljoyb EQ(NULL);
EXTERN Uint8 *sdlkeybuffer;
#define SDLKEYDOWN(k) sdlkeybuffer[k]

// OPENGL specific declarations
EXTERN int title_tex[512][512];
#ifdef DECLARE_GLOBALS
//GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};  /* White diffuse light. */
//GLfloat light_position[] = {10.0, 10.0, 30.0, 1.0};  /* nonInfinite light location. */
#else
//extern GLfloat light_diffuse[];
//extern GLfloat light_position[];
#endif
EXTERN int win_id;
EXTERN GLuint texName;

#endif
