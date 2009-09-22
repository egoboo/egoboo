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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
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
#    define EQ(x) = x
#else
#    define EXTERN extern
#    define EQ(x)
#endif

#define VERSION "2.7.8"                         // Version of the game

#define MAXINVENTORY        6
#define MAXIMPORTOBJECTS    (MAXINVENTORY + 2)      // left hand + right hand + MAXINVENTORY
#define MAXIMPORTPERPLAYER  (1 + MAXIMPORTOBJECTS)  // player + MAXIMPORTOBJECTS

#define NOSPARKLE           255
#define RESPAWN_ANYTIME     0xFF          // Code for respawnvalid...

#define NOHIDE              127                     // Don't hide

#define SPELLBOOK           127                     // The spellbook model

// Messaging stuff
#define MAX_MESSAGE          8                       // Number of messages
#define MAXTOTALMESSAGE     4096
#define MESSAGESIZE         80
#define MESSAGEBUFFERSIZE   (MAXTOTALMESSAGE*40)
#define TABADD              (1<<5)
#define TABAND              (~(TABADD-1))                      // Tab size

#define GRABSIZE            90.0f                      // Grab tolerance
#define SEEINVISIBLE        128                        // Cutoff for invisible characters

#define RAISE               12                  // Helps correct z level
#define SHADOWRAISE         5
#define DAMAGERAISE         25                  // Tolerance for damage tiles

/* SDL_GetTicks() always returns milli seconds */
#define TICKS_PER_SEC                   1000.0f

#define TARGET_FPS                      30.0f
#define FRAME_SKIP                      (TICKS_PER_SEC/TARGET_FPS)    // 1000 tics per sec / 50 fps = 20 ticks per frame

#define EMULATE_UPS                     50.0f
#define TARGET_UPS                      50.0f
#define UPDATE_SCALE                    (EMULATE_UPS/(stabilized_ups_sum/stabilized_ups_weight))
#define UPDATE_SKIP                     (TICKS_PER_SEC/TARGET_UPS)    // 1000 tics per sec / 50 fps = 20 ticks per frame
#define ONESECOND                       (TICKS_PER_SEC/UPDATE_SKIP)    // 1000 tics per sec / 20 ticks per frame = 50 fps

//------------------------------------
// Character defines
//------------------------------------

//Dismounting
#define DISMOUNTZVEL        16
#define DISMOUNTZVELFLY     4
#define PHYS_DISMOUNT_TIME  (TICKS_PER_SEC*0.5f)          // time delay for full object-object interaction (approximately 0.5 second)

//------------------------------------
// Timers
//------------------------------------

// Display
EXTERN Uint8           timeron     EQ( bfalse );          // Game timer displayed?
EXTERN Uint32          timervalue  EQ( 0 );           // Timer time ( 50ths of a second )

EXTERN bool_t          fpson        EQ(btrue);

// fps stuff
EXTERN float           est_max_fps           EQ( TARGET_FPS );
EXTERN float           est_gfx_time          EQ( 1.0f );
EXTERN Sint32          fps_clock             EQ(0);               ///< The number of ticks this second
EXTERN Uint32          fps_loops             EQ(0);               ///< The number of frames drawn this second
EXTERN float           stabilized_fps        EQ( TARGET_FPS );
EXTERN float           stabilized_fps_sum    EQ( 0 );
EXTERN float           stabilized_fps_weight EQ( 0 );

EXTERN float           est_update_time       EQ( 1.0f / TARGET_UPS );
EXTERN float           est_max_ups           EQ( TARGET_UPS );
EXTERN Sint32          ups_clock             EQ( 0 );             ///< The number of ticks this second
EXTERN Uint32          ups_loops             EQ( 0 );             ///< The number of frames drawn this second
EXTERN float           stabilized_ups        EQ( TARGET_UPS );
EXTERN float           stabilized_ups_sum    EQ( 0 );
EXTERN float           stabilized_ups_weight EQ( 0 );

// Timers
EXTERN Sint32          clock_stt;                   // GetTickCount at start
EXTERN Sint32          clock_all   EQ( 0 );             // The total number of ticks so far
EXTERN Sint32          clock_lst   EQ( 0 );             // The last total of ticks so far
EXTERN Sint32          clock_wld   EQ( 0 );             // The sync clock
EXTERN Sint32          clock_fps   EQ( 0 );             // The number of ticks this second
EXTERN Uint32          update_wld  EQ( 0 );            // The number of times the game has been updated
EXTERN Uint32          frame_all   EQ( 0 );             // The total number of frames drawn so far
EXTERN Uint32          frame_fps   EQ( 0 );             // The number of frames drawn this second
EXTERN Uint32          clock_stat  EQ( 0 );            // For stat regeneration
EXTERN Uint32          clock_pit   EQ( 0 );             // For pit kills
EXTERN Uint32          outofsync   EQ( 0 );

EXTERN bool_t          soundon  EQ( btrue );              // Is the sound alive?

EXTERN bool_t          pickedmodule_ready;              // Is there a new picked module?
EXTERN char            pickedmodule_name[64];           // The module load name
EXTERN int             pickedmodule_index;                // The module index number

// Respawning
EXTERN bool_t                   local_allpladead;            // Has everyone died?
EXTERN Uint16                   revivetimer EQ(0);

// Imports
EXTERN int                     numimport;                     // Number of imports from this machine
EXTERN Uint32                  local_control[16];             // Input bits for each imported player
EXTERN short                   local_slot[16];                // For local imports

// Setup values
EXTERN Uint8                   messageon      EQ( btrue );         // Messages?
EXTERN int                     maxmessage     EQ( MAX_MESSAGE );
EXTERN int                     wraptolerance  EQ( 80 );            // Status bar
EXTERN bool_t                  wateron        EQ( btrue );         // Water overlays?

// EWWWW. GLOBALS ARE EVIL.

// KEYBOARD
EXTERN bool_t console_mode EQ( bfalse );                   // Input text from keyboard?
EXTERN bool_t console_done EQ( bfalse );                   // Input text from keyboard finished?

extern float           light_a, light_d, light_x, light_y, light_z;
EXTERN float           hillslide  EQ( 1.00f );
EXTERN float           slippyfriction  EQ( 1.00f );                            // Friction
EXTERN float           airfriction  EQ( 0.91f );                               // 0.9868 is approximately real world air friction
EXTERN float           waterfriction  EQ( 0.80f );
EXTERN float           noslipfriction  EQ( 0.91f );
EXTERN float           gravity  EQ( -1.0f );                                   // Gravitational accel

#define INVISIBLE           20                      // The character can't be detected

EXTERN bool_t                    local_seeinvisible      EQ( bfalse );
EXTERN bool_t                    local_seekurse          EQ( bfalse );
EXTERN bool_t                    local_listening         EQ( bfalse );  // Players with listen skill?
EXTERN bool_t                    local_noplayers;                    // Are there any local players?

//------------------------------------
// Model stuff
//------------------------------------

EXTERN Uint16  bookicon_count    EQ(0);
EXTERN Uint16  bookicon_ref[MAX_SKIN];                      // The first book icon

EXTERN const char *globalparsename  EQ( NULL ); // The SCRIPT.TXT filename

// phisics info
EXTERN float           platstick  EQ( 0.1f );

enum e_order
{
    ORDER_NONE  = 0,
    ORDER_ATTACK,
    ORDER_ASSIST,
    ORDER_STAND,
    ORDER_TERRAIN,
    ORDER_COUNT
};

#define  _EGOBOO_H_

// list of global objects
// 37    grubbug.obj
// 40    strider.obj
// 41    beartrap.obj
// 44    lamp.obj
// 46    spkmace.obj
// 47    torch.obj
// 48    everburn.obj
// 49    oilflask.obj
// 50    sword.obj
// 51    spear.obj
// 54    light.obj
// 55    pitcobra.obj
// 56    cinquedea.obj
// 57    trident.obj
// 58    enchantarmorscroll.obj
// 59    chest.obj
// 60    enchantweaponscroll.obj
// 61    lbow.obj
// 62    thunderspear.obj
// 63    towershield.obj
// 64    enchant.obj
// 65    fireball.obj
// 66    unseen.obj
// 67    storms.obj
// 68    size.obj
// 69    protect.obj
// 70    icicles.obj
// 71    xbow.obj
// 72    mimic.obj
// 73    missile.obj
// 74    pstiletto.obj
// 75    phanxbow.obj
// 76    bullwolf.obj
// 77    hatchet.obj
// 78    kshield.obj
// 79    ressurection.obj
// 80    medkit.obj
// 81    msword.obj
// 82    sojurn.obj
// 83    strike.obj
// 84    armorchest.obj
// 85    clairvoyance.obj
// 86    lpotion.obj
// 87    mpotion.obj
// 88    confusion.obj
// 89    pike.obj
// 90    quiver.obj
// 91    firering.obj
// 92    scimitar.obj
// 93    keya.obj
// 94    keyc.obj
// 95    blob.obj
// 96    bomb.obj
// 97    cockatrice.obj
// 98    pkatana.obj
// 99    dsamulet.obj
//100    rack.obj
//101    carpet.obj
//108    iceray.obj
//109    shock.obj
//110    haste.obj
//111    battleward.obj
//112    syphonmana.obj
//114    metamorph.obj
//115    meteorswarm.obj
//116    darkritual.obj
//117    disjunction.obj
//118    arcanenova.obj
//119    disintergrate.obj
//120    wonder.obj
//121    hrune.obj
//122    combatbless.obj
//123    ammobox.obj
//124    regeneration.obj
//125    exorcism.obj
//127    book.obj
//128    restoration.obj
//129    holybolt.obj
//130    retribution.obj
//131    scythe.obj
//132    mallet.obj
//133    rebirth.obj
//135    jring.obj
//136    lifering.obj
//137    luckring.obj
//140    truesightscroll.obj
//156    lifedrain.obj
//158    pring.obj
//160    touristguide.obj

// list of objects that spawn other ("named") objects
//
// module               object
//
// abyss2.mod            bossyeti.obj
// abyss2.mod            cage.obj
// abyss2.mod            daemonlord.obj
// abyss2.mod            daemonlordwing.obj
// abyss2.mod            dracolich.obj
// abyss2.mod            goldenchest.obj
// abyss2.mod            seal.obj
// abyss2.mod            thebetrayer.obj
//
// archaeologist.mod    brazier.obj
// archaeologist.mod    pedestal.obj
//
// archmage.mod            archghost.obj
// archmage.mod            pedestal.obj
// archmage.mod            specialchest.obj
//
// benemocave.mod        bossbandit.obj
//
// bishopiacity.mod        angus.obj
// bishopiacity.mod        marcus.obj
// bishopiacity.mod        moduletool.obj
// bishopiacity.mod        outhouse.obj
// bishopiacity.mod        shopkeep.obj
// bishopiacity.mod        shopkeep2.obj
// bishopiacity.mod        touristguide.obj
//
// catacomb1.mod        body.obj
// catacomb1.mod        brazier.obj
// catacomb1.mod        moduletool.obj
// catacomb1.mod        puzzle.obj
// catacomb1.mod        puzzlemod.obj
// catacomb1.mod        sarcophagus.obj
// catacomb1.mod        sporkbutton.obj
//
// catacomb2.mod        body.obj
//
// crypt.mod            rockhead.obj
//
// elf.mod                grave.obj
// elf.mod                rockhead.obj
//
// forgotten.mod        droptrap.obj
//
// healer.mod            rockhead.obj
//
// heist.mod            eyeballguard.obj
// heist.mod            goldenchest.obj
// heist.mod            moduletool.obj
// heist.mod            puzzlemod.obj
//
// paladin.mod            bpopper.obj
// paladin.mod            evilaltar.obj
// paladin.mod            popper.obj
// paladin.mod            rockhead.obj
// paladin.mod            sarcophagus.obj
//
// palice.mod            monsterdrop.obj
// palice.mod            yeti.obj
//
// palsand.mod            bossdrop.obj
// palsand.mod            bossfall.obj
// palshad.mod            popper.obj
// palshad.mod            sacripit.obj
//
// palwater.mod            filldrop.obj
// palwater.mod            grubdrop.obj
// palwater.mod            trapdrop.obj
//
// tourist.mod            sarcophagus.obj
//
// wizard.mod            ritual.obj
//
// worldmap.mod            encounter.obj
//
// zippy.mod            lamb.obj
// zippy.mod            mushroomtrader.obj
// zippy.mod            pettrader.obj
// zippy.mod            shopkeep.obj
//
// zombor.mod            nercomancer.obj
