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

#include "egoboo_typedef.h"

#include "sound.h"
#include "script.h"
#include "Md2.h"

#include "egoboo.h"

#define SPINRATE            200                     // How fast spinners spin
#define WATCHMIN            0.01f                     //
#define PITDEPTH            -30                     // Depth to kill character
#define VOLMIN              -4000            // Minumum Volume level

#define BORETIME            (rand()&255)+120
#define CAREFULTIME         50

#define REEL                7600.0f      // Dampen for melee knock back
#define REELBASE            0.35f         //

#define RIPPLEAND           15                      // How often ripples spawn

#define RIPPLETOLERANCE     60                      // For deep water
#define SPLASHTOLERANCE     10                      //

// Throwing
#define THROWFIX            30.0f                    // To correct thrown velocities
#define MINTHROWVELOCITY    15.0f                    //
#define MAXTHROWVELOCITY    45.0f                    //

// Inventory
#define MAXNUMINPACK        6                       // Max number of items to carry in pack
#define PACKDELAY           25                      // Time before inventory rotate again
#define GRABDELAY           25                      // Time before grab again

// Z velocity
#define FLYDAMPEN           0.001f                    // Levelling rate for flyers
#define JUMPINFINITE        255                     // Flying character
#define SLIDETOLERANCE      10                      // Stick to ground better
#define PLATTOLERANCE       50                     // Platform tolerance...
#define PLATADD             -10                     // Height add...
#define PLATASCEND          0.10f                     // Ascension rate
#define PLATKEEP            0.90f                     // Retention rate

//------------------------------------
// Team variables
//------------------------------------

#define MAXTEAM             27                      // Teams A-Z, +1 more for damage tiles
#define DAMAGETEAM          26                      // For damage tiles
#define EVILTEAM            4                       // E
#define GOODTEAM            6                       // G
#define NULLTEAM            13                      // N
#define NOLEADER            0xFFFF                  // If the team has no leader...

struct s_team
{
    bool_t  hatesteam[MAXTEAM];     // Don't damage allies...
    Uint16  morale;                 // Number of characters on team
    Uint16  leader;                 // The leader of the team
    Uint16  sissy;                  // Whoever called for help last
};
typedef struct s_team team_t;

extern team_t TeamList[MAXTEAM];

//------------------------------------
// Character template
//------------------------------------
#define MAXCAPNAMESIZE      32                      // Character class names
#define MAXSECTION                      4           // T-wi-n-k...  Most of 4 sections

struct s_cap
{
    short        importslot;

    // naming
    char         classname[MAXCAPNAMESIZE];     // Class name
    Uint16       chop_sectionsize[MAXSECTION];   // Number of choices, 0
    Uint16       chop_sectionstart[MAXSECTION];  //

    // skins
    char         skinname[MAXSKIN][MAXCAPNAMESIZE];   // Skin name
    Uint16       skincost[MAXSKIN];                   // Store prices
    float        maxaccel[MAXSKIN];                   // Acceleration for each skin
    Uint8        skindressy;                         // Dressy

    // overrides
    Sint8        skinoverride;                  // -1 or 0-3.. For import
    Uint8        leveloverride;                 // 0 for normal
    int          stateoverride;                 // 0 for normal
    int          contentoverride;               // 0 for normal

    IDSZ         idsz[IDSZ_COUNT];                 // ID strings

    float        strengthdampen;                // Strength damage factor
    Uint8        stoppedby;                     // Collision Mask

    // inventory
    Uint8        ammomax;                       // Ammo stuff
    Uint8        ammo;                          //
    Sint16       money;                         // Money

    // characer stats
    Uint8        gender;                        // Gender
    Uint16       lifebase;                      // Life
    Uint16       liferand;                      //
    Uint16       lifeperlevelbase;              //
    Uint16       lifeperlevelrand;              //
    Sint16       lifereturn;                    //
    Uint16       lifeheal;                      //
    Uint16       manabase;                      // Mana
    Uint16       manarand;                      //
    Sint16       manacost;                      //
    Uint16       manaperlevelbase;              //
    Uint16       manaperlevelrand;              //
    Uint16       manareturnbase;                //
    Uint16       manareturnrand;                //
    Uint16       manareturnperlevelbase;        //
    Uint16       manareturnperlevelrand;        //
    Uint16       manaflowbase;                  //
    Uint16       manaflowrand;                  //
    Uint16       manaflowperlevelbase;          //
    Uint16       manaflowperlevelrand;          //
    Uint16       strengthbase;                  // Strength
    Uint16       strengthrand;                  //
    Uint16       strengthperlevelbase;          //
    Uint16       strengthperlevelrand;          //
    Uint16       wisdombase;                    // Wisdom
    Uint16       wisdomrand;                    //
    Uint16       wisdomperlevelbase;            //
    Uint16       wisdomperlevelrand;            //
    Uint16       intelligencebase;              // Intlligence
    Uint16       intelligencerand;              //
    Uint16       intelligenceperlevelbase;      //
    Uint16       intelligenceperlevelrand;      //
    Uint16       dexteritybase;                 // Dexterity
    Uint16       dexterityrand;                 //
    Uint16       dexterityperlevelbase;         //
    Uint16       dexterityperlevelrand;         //

    // physics
    Uint8        weight;                        // Weight
    float        dampen;                        // Bounciness
    float        bumpdampen;                    // Mass

    float        size;                          // Scale of model
    float        sizeperlevel;                  // Scale increases
    Uint8        shadowsize;                    // Shadow size
    Uint8        bumpsize;                      // Bounding octagon
    Uint8        bumpsizebig;                   // For octagonal bumpers
    Uint8        bumpheight;                    //

    // movement
    float        jump;                          // Jump power
    Uint8        jumpnumber;                    // Number of jumps ( Ninja )
    Uint8        sneakspd;                      // Sneak threshold
    Uint8        walkspd;                       // Walk threshold
    Uint8        runspd;                        // Run threshold
    Uint8        flyheight;                     // Fly height

    // graphics
    Uint8        flashand;                      // Flashing rate
    Uint8        alpha;                         // Transparency
    Uint8        light;                         // Light blending
    bool_t       transferblend;                 // Transfer blending to rider/weapons
    bool_t       sheen;                         // How shiny it is ( 0-15 )
    bool_t       enviro;                        // Phong map this baby?
    Uint16       uoffvel;                       // Texture movement rates
    Uint16       voffvel;                       //
    bool_t       uniformlit;                    // Bad lighting?
    Uint8        lifecolor;                     // Bar colors
    Uint8        manacolor;                     //

    // random stuff
    bool_t       stickybutt;                    // Stick to the ground?

    Uint16       iframefacing;                  // Invincibility frame
    Uint16       iframeangle;                   //
    Uint16       nframefacing;                  // Normal frame
    Uint16       nframeangle;                   //

    // defense
    Uint8        resistbumpspawn;               // Don't catch fire
    Uint8        defense[MAXSKIN];                    // Defense for each skin
    Uint8        damagemodifier[DAMAGE_COUNT][MAXSKIN];

    // xp
    Uint32       experienceforlevel[MAXLEVEL];  // Experience needed for next level
    Uint32       experiencebase;                // Starting experience
    Uint16       experiencerand;                //
    Uint16       experienceworth;               // Amount given to killer/user
    float        experienceexchange;            // Adds to worth
    float        experiencerate[XP_COUNT];

    // sound
    Sint8        soundindex[SOUND_COUNT];       // a map for soundX.wav to sound types
    Mix_Chunk *  wavelist[MAXWAVE];             // sounds in a object

    // flags
    bool_t       isitem;                        // Is it an item?
    bool_t       invictus;                      // Is it invincible?
    bool_t       ismount;                       // Can you ride it?
    bool_t       isstackable;                   // Is it arrowlike?
    bool_t       nameknown;                     // Is the class name known?
    bool_t       usageknown;                    // Is its usage known
    bool_t       cancarrytonextmodule;          // Take it with you?
    bool_t       needskillidtouse;              // Check IDSZ first?
    bool_t       waterwalk;                     // Walk on water?
    bool_t       platform;                      // Can be stood on?
    bool_t       canuseplatforms;               // Can use platforms?
    bool_t       cangrabmoney;                  // Collect money?
    bool_t       canopenstuff;                  // Open chests/doors?
    bool_t       icon;                          // Draw icon
    bool_t       forceshadow;                   // Draw a shadow?
    bool_t       ripple;                        // Spawn ripples?
    Uint8        damagetargettype;              // For AI DamageTarget
    Uint8        weaponaction;                  // Animation needed to swing
    bool_t       gripvalid[MAXSLOT];            // Left/Right hands valid
    Uint8        attackattached;                //
    Sint8        attackprttype;                 //
    Uint8        attachedprtamount;             // Sticky particles
    Uint8        attachedprtreaffirmdamagetype; // Relight that torch...
    Uint16       attachedprttype;               //
    Uint8        gopoofprtamount;               // Poof effect
    Sint16       gopoofprtfacingadd;            //
    Uint16       gopoofprttype;                 //
    bool_t       bludvalid;                    // Blud ( yuck )
    Uint8        bludprttype;                  //
    bool_t       ridercanattack;                // Rider attack?
    bool_t       canbedazed;                    // Can it be dazed?
    bool_t       canbegrogged;                  // Can it be grogged?
    Uint8        kursechance;                   // Chance of being kursed
    bool_t       istoobig;                      // Can't be put in pack
    bool_t       reflect;                       // Draw the reflection
    bool_t       alwaysdraw;                    // Always render
    bool_t       isranged;                      // Flag for ranged weapon
    Sint8        hidestate;                       // Don't draw when...
    bool_t       isequipment;                     // Behave in silly ways
    Sint8        isvaluable;                      // Force to be valuable

    //skill system
    Sint8        shieldproficiency;               // Can it use shields?
    bool_t       canjoust;                        // Can it use advanced weapons?
    bool_t       canuseadvancedweapons;           // Can it use advanced weapons?
    bool_t       canseeinvisible;                 // Can it see invisible?
    bool_t       canseekurse;                     // Can it see kurses?
    bool_t       canusedivine;
    bool_t       canusearcane;
    bool_t       canusetech;
    bool_t       candisarm;
    bool_t       canbackstab;
    bool_t       canusepoison;
    bool_t       canread;
};

typedef struct s_cap cap_t;

extern int   importobject;
extern cap_t CapList[MAXMODEL];

//------------------------------------
// Character variables
//------------------------------------
struct s_chr
{
    glMatrix       matrix;          // Character's matrix
    char           matrixvalid;     // Did we make one yet?
    char           name[MAXCAPNAMESIZE];  // Character name
    bool_t         on;              // Does it exist?
    Uint8          onold;           // Network fix
    Uint8          alive;           // Is it alive?
    Uint8          waskilled;       // Fix for network
    Uint8          inpack;          // Is it in the inventory?
    Uint8          wasinpack;       // Temporary thing...
    Uint16         nextinpack;    // Link to the next item
    Uint8          numinpack;       // How many
    Uint8          openstuff;       // Can it open chests/doors?
    Uint8          lifecolor;       // Bar color
    Sint16         life;            // Basic character stats
    Sint16         lifemax;         //   All 8.8 fixed point
    Uint16         lifeheal;        //
    Uint8          manacolor;       // Bar color
    Uint8          ammomax;         // Ammo stuff
    Uint16         ammo;            //
    Uint8          gender;          // Gender
    Sint16         mana;            // Mana stuff
    Sint16         manamax;         //
    Sint16         manaflow;        //
    Sint16         manareturn;      //
    Sint16         strength;        // Strength
    Sint16         wisdom;          // Wisdom
    Sint16         intelligence;    // Intelligence
    Sint16         dexterity;       // Dexterity
    bool_t         icon;            // Show the icon?
    Uint8          sparkle;         // Sparkle color or 0 for off
    bool_t         cangrabmoney;    // Picks up coins?
    bool_t         isplayer;        // btrue = player
    bool_t         islocalplayer;   // btrue = local player
    ai_state_t     ai;
    Uint8          stickybutt;      // Rests on floor
    Uint8          enviro;          // Environment map?
    float          oldx;            // Character's last position
    float          oldy;            //
    float          oldz;            //
    Uint8          inwater;         //
    Uint16         oldturn;         //
    Uint8          flyheight;       // Height to stabilize at
    Uint8          team;            // Character's team
    Uint8          baseteam;        // Character's starting team
    Uint8          staton;          // Display stats?
    float          xstt;            // Starting position
    float          ystt;            //
    float          zstt;            //
    float          xpos;            // Character's position
    float          ypos;            //
    float          zpos;            //
    float          xvel;            // Character's velocity
    float          yvel;            //
    float          zvel;            //
    float          latchx;          // Character latches
    float          latchy;          //
    Uint32         latchbutton;     // Button latches
    Uint16         reloadtime;      // Time before another shot
    float          maxaccel;        // Maximum acceleration
    float          fat;             // Character's size
    float          sizegoto;        // Character's size goto
    Uint8          sizegototime;    // Time left in size change
    float          dampen;          // Bounciness
    float          level;           // Height of tile
    float          jump;            // Jump power
    Uint8          jumptime;        // Delay until next jump
    Uint8          jumpnumber;      // Number of jumps remaining
    Uint8          jumpnumberreset; // Number of jumps total, 255=Flying
    Uint8          jumpready;       // For standing on a platform character
    Uint32         onwhichfan;      // Where the char is
    Uint32         onwhichblock;    // The character's collision block
    Uint8          indolist;        // Has it been added yet?
    Uint16         uoffset;         // For moving textures
    Uint16         voffset;         //
    Uint16         uoffvel;         // Moving texture speed
    Uint16         voffvel;         //
    Uint16         turnleftright;   // Character's rotation 0 to 0xFFFF
    Uint16         lightturnleftright;// Character's light rotation 0 to 0xFFFF
    Uint16         turnmaplr;       //
    Uint16         turnmapud;       //
    Uint16         skin;            // Character's skin
    Uint16         texture;         // The texture id of the character's skin
    Uint8          model;           // Character's model
    Uint8          basemodel;       // The true form
    Uint8          actionready;     // Ready to play a new one
    Uint8          action;          // Character's action
    bool_t         keepaction;      // Keep the action playing
    bool_t         loopaction;      // Loop it too
    Uint8          nextaction;      // Character's action to play next
    Uint16         frame;           // Character's frame
    Uint16         lastframe;       // Character's last frame
    Uint8          lip;             // Character's frame in betweening
    Uint8          vrta[MAXVERTICES];// Lighting hack ( Ooze )
    Uint16         holdingwhich[MAXSLOT]; // !=MAXCHR if character is holding something
    Uint16         attachedto;      // !=MAXCHR if character is a held weapon
    Uint16         weapongrip[GRIP_VERTS];   // Vertices which describe the weapon grip
    Uint8          alpha;           // 255 = Solid, 0 = Invisible
    Uint8          basealpha;
    Uint8          light;           // 1 = Light, 0 = Normal
    Uint8          flashand;        // 1,3,7,15,31 = Flash, 255 = Don't
    Uint8          lightlevel_amb;  // 0-255, terrain light
    Uint8          lightlevel_dir;  // 0-255, terrain light
    Uint8          sheen;           // 0-15, how shiny it is
    Uint8          transferblend;   // Give transparency to weapons?
    Uint8          isitem;          // Is it grabbable?
    Uint8          invictus;        // Totally invincible?
    Uint8          ismount;         // Can you ride it?
    Uint8          redshift;        // Color channel shifting
    Uint8          grnshift;        //
    Uint8          blushift;        //
    Uint8          shadowsize;      // Size of shadow
    Uint8          bumpsize;        // Size of bumpers
    Uint8          bumpsizebig;     // For octagonal bumpers
    Uint8          bumpheight;      // Distance from head to toe
    Uint8          shadowsizesave;  // Without size modifiers
    Uint8          bumpsizesave;    //
    Uint8          bumpsizebigsave; //
    Uint8          bumpheightsave;  //
    Uint16         bumpnext;        // Next character on fanblock
    float          bumpdampen;      // Character bump mass
    Uint8          platform;        // Can it be stood on
    Uint8          waterwalk;       // Always above watersurfacelevel?
    Uint8          turnmode;        // Turning mode
    Uint8          sneakspd;        // Sneaking if above this speed
    Uint8          walkspd;         // Walking if above this speed
    Uint8          runspd;          // Running if above this speed
    Uint8          damagetargettype;// Type of damage for AI DamageTarget
    Uint8          reaffirmdamagetype; // For relighting torches
    Uint8          damagemodifier[DAMAGE_COUNT];  // Resistances and inversion
    Uint8          damagetime;      // Invincibility timer
    Uint8          defense;         // Base defense rating
    Uint16         weight;          // Weight ( for pressure plates )
    Uint16         holdingweight;   // For weighted buttons
    Sint16         money;           // Money
    Sint16         lifereturn;      // Regeneration/poison
    Sint16         manacost;        // Mana cost to use
    Uint8          stoppedby;       // Collision mask
    Uint32         experience;      // Experience
    Uint8          experiencelevel; // Experience Level
    Sint16         grogtime;        // Grog timer
    Sint16         dazetime;        // Daze timer
    Uint8          iskursed;        // Can't be dropped?
    Uint8          nameknown;       // Is the name known?
    Uint8          ammoknown;       // Is the ammo known?
    Uint8          hitready;        // Was it just dropped?
    Sint16         boretime;        // Boredom timer
    Uint8          carefultime;     // "You hurt me!" timer
    bool_t         canbecrushed;    // Crush in a door?
    Uint8          inwhichhand;     // SLOT_LEFT or SLOT_RIGHT
    Uint8          isequipped;      // For boots and rings and stuff
    Uint16         firstenchant;    // Linked list for enchants
    Uint16         undoenchant;     // Last enchantment spawned
    bool_t         canchannel;      //
    bool_t         overlay;         // Is this an overlay?  Track aitarget...
    Uint8          missiletreatment;// For deflection, etc.
    Uint8          missilecost;     // Mana cost for each one
    Uint16         missilehandler;  // Who pays the bill for each one...
    Uint16         damageboost;     // Add to swipe damage
    bool_t         isshopitem;     // Spawned in a shop?
    Sint8          soundindex[SOUND_COUNT];       // a map for soundX.wav to sound types

    //Skills
    Sint8           shieldproficiency;  // Can it use shields?
    bool_t          canjoust; //
    bool_t          canuseadvancedweapons; //
    bool_t          canseeinvisible; //
    bool_t          canseekurse;     //
    bool_t          canusedivine;
    bool_t          canusearcane;
    bool_t          canusetech;
    bool_t          candisarm;
    bool_t          canbackstab;
    bool_t          canusepoison;
    bool_t          canread;

    // Accumulators for doing the physics in bump_characters(). should prevent you from being bumped into a wall
    float          phys_pos_x;
    float          phys_pos_y;
    float          phys_pos_z;
    float          phys_vel_x;
    float          phys_vel_y;
    float          phys_vel_z;
};

typedef struct s_chr chr_t;

extern chr_t ChrList[MAXCHR];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// This is for random naming

#define MAXCHOP                         (MAXMODEL*CHOPPERMODEL)
#define CHOPSIZE                        8
#define CHOPDATACHUNK                   (MAXCHOP*CHOPSIZE)

struct s_chop_data
{
    Uint16  count;                  // The number of name parts

    Uint32  carat;                  // The data pointer
    char    buffer[CHOPDATACHUNK];  // The name parts
    Uint16  start[MAXCHOP];         // The first character of each part
};
typedef struct s_chop_data chop_data_t;

extern chop_data_t chop;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//Function prototypes
char *  undo_idsz( IDSZ idsz );
void drop_money( Uint16 character, Uint16 money );
void call_for_help( Uint16 character );
void give_experience( Uint16 character, int amount, Uint8 xptype, bool_t override_invictus );
void give_team_experience( Uint8 team, int amount, Uint8 xptype );
void damage_character( Uint16 character, Uint16 direction,
                       int damagebase, int damagerand, Uint8 damagetype, Uint8 team,
                       Uint16 attacker, Uint16 effects, bool_t ignoreinvincible );
void kill_character( Uint16 character, Uint16 killer );
void spawn_poof( Uint16 character, Uint16 profile );
void reset_character_alpha( Uint16 character );
void reset_character_accel( Uint16 character );
void detach_character_from_mount( Uint16 character, Uint8 ignorekurse,
                                  Uint8 doshop );

void flash_character_height( Uint16 character, Uint8 valuelow, Sint16 low,
                             Uint8 valuehigh, Sint16 high );
void flash_character( Uint16 character, Uint8 value );

void free_one_character( Uint16 character );
void make_one_weapon_matrix( Uint16 cnt );
void make_character_matrices();
int get_free_character();
void free_inventory( Uint16 character );

void keep_weapons_with_holders();
void make_one_character_matrix( Uint16 cnt );
void resize_characters();
void move_characters( void );
void do_level_up( Uint16 character );

void free_all_characters();

Uint8 __chrhitawall( Uint16 character );

int chr_count_free();

char * chop_create( Uint16 profile );

int spawn_one_character( float x, float y, float z, Uint16 profile, Uint8 team,
                         Uint8 skin, Uint16 facing, const char *name, int override );

void respawn_character( Uint16 character );
Uint16 change_armor( Uint16 character, Uint16 skin );
void change_character( Uint16 cnt, Uint16 profile, Uint8 skin,
                       Uint8 leavewhich );
Uint8 cost_mana( Uint16 character, int amount, Uint16 killer );
void switch_team( Uint16 character, Uint8 team );
void issue_clean( Uint16 character );
int  restock_ammo( Uint16 character, IDSZ idsz );
void attach_character_to_mount( Uint16 character, Uint16 mount, Uint16 grip );
void pack_add_item( Uint16 item, Uint16 character );
Uint16 pack_get_item( Uint16 character, Uint16 grip, Uint8 ignorekurse );
void drop_keys( Uint16 character );
void drop_all_items( Uint16 character );
bool_t character_grab_stuff( Uint16 chara, int grip, Uint8 people );

void chr_play_action( Uint16 character, Uint16 action, Uint8 actionready );
void chr_set_frame( Uint16 character, int frame, Uint16 lip );

void export_one_character_name( const char *szSaveName, Uint16 character );
void export_one_character_profile( const char *szSaveName, Uint16 character );
void export_one_character_skin( const char *szSaveName, Uint16 character );
int load_one_character_profile( const char *szLoadName );

void chop_load( Uint16 profile, const char *szLoadname );
void character_swipe( Uint16 cnt, Uint8 slot );

int check_skills( Uint16 who, IDSZ whichskill );

//---------------------------------------------------------------------------------------------
// Quest system
bool_t add_quest_idsz( const char *whichplayer, IDSZ idsz );
Sint16 modify_quest_idsz( const char *whichplayer, IDSZ idsz, Sint16 adjustment );
Sint16 check_player_quest( const char *whichplayer, IDSZ idsz );
