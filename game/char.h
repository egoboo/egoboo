#pragma once

#include "object.h"
#include "sound.h"
#include "mesh.h"
#include "Mad.h"
#include "input.h"

#define MAXWAY              8                       // Waypoints
#define WAYTHRESH           128                     // Threshold for reaching waypoint

#define SPINRATE            200                     // How fast spinners spin
#define FLYDAMPEN           .001                    // Levelling rate for flyers

#define JUMPINFINITE        255                     // Flying character
#define DELAY_JUMP           20                      // Time between jumps
#define JUMPTOLERANCE       20                      // Distance above ground to be jumping
#define SLIDETOLERANCE      10                      // Stick to ground better
#define PLATTOLERANCE       50                      // Platform tolerance...
#define PLATADD             -10                     // Height add...
#define PLATASCEND          .10                     // Ascension rate
#define PLATKEEP            (1.0f-PLATASCEND)       // Retention rate

#define DONTFLASH 255                               //

#define MAXVERTICES                    1024         // Max number of points in a model

#define DELAY_BORE                        (rand()&255)+120
#define DELAY_CAREFUL                     50
#define REEL                            7600.0      // Dampen for melee knock back
#define REELBASE                        .35         //

#define WATCHMIN            .01                     //
#define DELAY_PACK           25                      // Time before inventory rotate again
#define DELAY_GRAB           25                      // Time before grab again
#define NOSKINOVERRIDE      -1                      // For import
#define PITDEPTH            -30                     // Depth to kill character
#define DISMOUNTZVEL        16
#define DISMOUNTZVELFLY     4

#define THROWFIX            30.0                    // To correct thrown velocities
#define MINTHROWVELOCITY    15.0                    //
#define MAXTHROWVELOCITY    45.0                    //

#define RETURNAND           63                      // Return mana every so often
#define MANARETURNSHIFT     4                       //

#define RIPPLEAND           15                      // How often ripples spawn
#define RIPPLETOLERANCE     60                      // For deep water
#define SPLASHTOLERANCE     10                      //
#define CLOSETOLERANCE      2                       // For closing doors

#define DROPXYVEL           8                       //
#define DROPZVEL            7                       //
#define JUMPATTACKVEL       -2                      //
#define WATERJUMP           12                      //


#define MAXCAP              MAXPROFILE
#define MAXCHR              350         // Max number of characters
#define MAXSKIN             4
#define MAXCAPNAMESIZE      32                      // Character class names
#define BASELEVELS            6                       // Levels 0-5

#define MAXXP    ((1<<30)-1)                               // Maximum experience (Sint32 32)
#define MAXMONEY 9999                                      // Maximum money

#define SEEKURSEAND         31                      // Blacking flash
#define SEEINVISIBLE        128                     // Cutoff for invisible characters
#define INVISIBLE           20                      // The character can't be detected


typedef enum gender_e
{
  GEN_FEMALE = 0,
  GEN_MALE,
  GEN_OTHER,
  GEN_RANDOM,
} GENDER;

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

typedef enum alert_bits_e
{
  ALERT_NONE                       =       0,
  ALERT_SPAWNED                    = 1 <<  0,
  ALERT_HITVULNERABLE              = 1 <<  1,
  ALERT_ATWAYPOINT                 = 1 <<  2,
  ALERT_ATLASTWAYPOINT             = 1 <<  3,
  ALERT_ATTACKED                   = 1 <<  4,
  ALERT_BUMPED                     = 1 <<  5,
  ALERT_SIGNALED                   = 1 <<  6,
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

typedef enum turnmode_e
{
  TURNMODE_NONE     = 0,
  TURNMODE_VELOCITY,                      // Character gets rotation from velocity
  TURNMODE_WATCH,                             // For watch towers
  TURNMODE_SPIN,                              // For spinning objects
  TURNMODE_WATCHTARGET                        // For combat intensive AI
} TURNMODE;

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

#define MAXNUMINPACK        6                       // Max number of items to carry in pack
#define LOWSTAT             INT_TO_FP8(  1)      // Worst...
#define PERFECTSTAT         INT_TO_FP8( 75)      // Perfect...
#define HIGHSTAT            INT_TO_FP8( 99)      // Absolute MAX strength...
#define PERFECTBIG          INT_TO_FP8(127)      // Perfect life or mana...
#define DAMAGE_MIN          INT_TO_FP8(  1)      // Minimum damage for hurt animation
#define DAMAGE_HURT         INT_TO_FP8(  1)      // 1 point of damage == hurt


extern Uint16          numdolist;                  // How many in the list
extern Uint16          dolist[MAXCHR];             // List of which characters to draw

// This is for random naming
#define CHOPPERMODEL                    32          //
#define MAXSECTION                      4           // T-wi-n-k...  Most of 4 sections
#define MAXCHOP                         (MAXPROFILE*CHOPPERMODEL)
#define CHOPSIZE                        8
#define CHOPDATACHUNK                   (MAXCHOP*CHOPSIZE)

EXTERN Uint16          numchop  EQ( 0 );              // The number of name parts
EXTERN Uint32          chopwrite  EQ( 0 );            // The data pointer
EXTERN char            chopdata[CHOPDATACHUNK];    // The name parts
EXTERN Uint16          chopstart[MAXCHOP];         // The first character of each part
EXTERN char            namingnames[MAXCAPNAMESIZE];// The name returned by the function

// Character profiles
typedef struct import_info_t
{
  bool_t valid;                // Can it import?
  int    object;
  int    player;
  int    amount;
  Sint16 slot_lst[MAXPROFILE];
} IMPORT_INFO;

bool_t import_info_clear(IMPORT_INFO * ii);
bool_t import_info_add(IMPORT_INFO * ii, int obj);

extern IMPORT_INFO import;

typedef struct skin_t
{
  Uint8         defense_fp8;                // Defense for each skin
  char          name[MAXCAPNAMESIZE];   // Skin name
  Uint16        cost;                   // Store prices
  Uint8         damagemodifier_fp8[MAXDAMAGETYPE];
  float         maxaccel;                   // Acceleration for each skin
} SKIN;

typedef struct cap_t
{
  bool_t        used;
  char          classname[MAXCAPNAMESIZE];     // Class name
  Sint8         skinoverride;                  // -1 or 0-3.. For import
  Uint8         leveloverride;                 // 0 for normal
  int           stateoverride;                 // 0 for normal
  int           contentoverride;               // 0 for normal
  bool_t        skindressy;                    // Dressy
  float         strengthdampen;                // Strength damage factor
  Uint8         stoppedby;                     // Collision Mask
  bool_t        uniformlit;                    // Bad lighting?
  Uint8         lifecolor;                     // Bar colors
  Uint8         manacolor;                     //
  Uint8         ammomax;                       // Ammo stuff
  Uint8         ammo;                          //
  GENDER        gender;                        // Gender
  PAIR          life_fp8;                      // Life
  PAIR          lifeperlevel_fp8;              //
  Sint16        lifereturn_fp8;                //
  Sint16        money;                         // Money
  Uint16        lifeheal_fp8;                  //
  PAIR          mana_fp8;                      // Mana
  Sint16        manacost_fp8;                  //
  PAIR          manaperlevel_fp8;              //
  PAIR          manareturn_fp8;                //
  PAIR          manareturnperlevel_fp8;        //
  PAIR          manaflow_fp8;                  //
  PAIR          manaflowperlevel_fp8;          //
  PAIR          strength_fp8;                  // Strength
  PAIR          strengthperlevel_fp8;          //
  PAIR          wisdom_fp8;                    // Wisdom
  PAIR          wisdomperlevel_fp8;            //
  PAIR          intelligence_fp8;              // Intlligence
  PAIR          intelligenceperlevel_fp8;      //
  PAIR          dexterity_fp8;                 // Dexterity
  PAIR          dexterityperlevel_fp8;         //
  float         size;                          // Scale of model
  float         sizeperlevel;                  // Scale increases
  float         dampen;                        // Bounciness
  float         bumpstrength;                  // ghostlike interaction with objects?
  Uint8         shadowsize;                    // Shadow size
  float         bumpsize;                      // Bounding octagon
  float         bumpsizebig;                   // For octagonal bumpers
  float         bumpheight;                    //
  float         bumpdampen;                    // Mass
  float         weight;                        // Weight
  float         jump;                          // Jump power
  Uint8         jumpnumber;                    // Number of jumps ( Ninja )
  Uint8         spd_sneak;                      // Sneak threshold
  Uint8         spd_walk;                       // Walk threshold
  Uint8         spd_run;                        // Run threshold
  Uint8         flyheight;                     // Fly height
  Uint8         flashand;                      // Flashing rate
  Uint8         alpha_fp8;                     // Transparency
  Uint8         light_fp8;                     // Light blending
  bool_t        transferblend;                 // Transfer blending to rider/weapons
  Uint8         sheen_fp8;                     // How shiny it is ( 0-15 )
  bool_t        enviro;                        // Phong map this baby?
  Uint16        uoffvel;                       // Texture movement rates
  Uint16        voffvel;                       //
  bool_t        stickybutt;                    // Stick to the ground?
  Uint16        iframefacing;                  // Invincibility frame
  Uint16        iframeangle;                   //
  Uint16        nframefacing;                  // Normal frame
  Uint16        nframeangle;                   //
  int           experienceforlevel[BASELEVELS];  // Experience needed for next level
  float         experienceconst;
  float         experiencecoeff;
  PAIR          experience;                    // Starting experience
  int           experienceworth;               // Amount given to killer/user
  float         experienceexchange;            // Adds to worth
  float         experiencerate[XP_COUNT];
  IDSZ          idsz[IDSZ_COUNT];                 // ID strings
  bool_t        isitem;                        // Is it an item?
  bool_t        ismount;                       // Can you ride it?
  bool_t        isstackable;                   // Is it arrowlike?
  bool_t        nameknown;                     // Is the class name known?
  bool_t        usageknown;                    // Is its usage known
  bool_t        cancarrytonextmodule;          // Take it with you?
  bool_t        needskillidtouse;              // Check IDSZ first?
  bool_t        isplatform;                      // Can be stood on?
  bool_t        canuseplatforms;               // Can use platforms?
  bool_t        cangrabmoney;                  // Collect money?
  bool_t        canopenstuff;                  // Open chests/doors?
  bool_t        icon;                          // Draw icon
  bool_t        forceshadow;                   // Draw a shadow?
  bool_t        ripple;                        // Spawn ripples?
  DAMAGE        damagetargettype;              // For AI DamageTarget
  ACTION        weaponaction;                  // Animation needed to swing
  bool_t        slotvalid[SLOT_COUNT];         // Left/Right hands valid
  bool_t        attackattached;                //
  Sint8         attackprttype;                 //
  Uint8         attachedprtamount;             // Sticky particles
  DAMAGE        attachedprtreaffirmdamagetype; // Relight that torch...
  Uint16        attachedprttype;               //
  Uint8         gopoofprtamount;               // Poof effect
  Sint16        gopoofprtfacingadd;            //
  Uint16        gopoofprttype;                 //
  Uint8         bludlevel;                     // Blud ( yuck )
  Uint16        bludprttype;                   //
  Sint8         footfallsound;                 // Footfall sound, -1
  Sint8         jumpsound;                     // Jump sound, -1
  Uint8         kursechance;                   // Chance of being kursed
  Sint8         hidestate;                       // Don't draw when...

  SKIN          skin[MAXSKIN];
  Mix_Chunk *   wavelist[MAXWAVE];                    //sounds in a object

  // [BEGIN] Character template parameters that are like Skill Expansions
  bool_t        istoobig;                      // Can't be put in pack
  bool_t        reflect;                       // Draw the reflection
  bool_t        alwaysdraw;                    // Always render
  bool_t        isranged;                      // Flag for ranged weapon
  bool_t        isequipment;                   // Behave in silly ways
  bool_t        ridercanattack;                // Rider attack?
  bool_t        canbedazed;                    // Can it be dazed?
  bool_t        canbegrogged;                  // Can it be grogged?
  bool_t        resistbumpspawn;               // Don't catch fire
  bool_t        waterwalk;                     // Walk on water?
  bool_t        invictus;                      // Is it invincible?
  bool_t        canseeinvisible;               // Can it see invisible?
  // [END] Character template parameters that are like Skill Expansions


  // [BEGIN] Skill Expansions
  bool_t     canseekurse;             // Can it see kurses?
  bool_t     canusearcane;            // Can use [WMAG] spells?
  bool_t     canjoust;                // Can it use a lance to joust?
  bool_t     canusetech;              // Can it use [TECH]items?
  bool_t     canusedivine;            // Can it use [HMAG] runes?
  bool_t     candisarm;               // Disarm and find traps [DISA]
  bool_t     canbackstab;             // Backstab and murder [STAB]
  bool_t     canuseadvancedweapons;   // Advanced weapons usage [AWEP]
  bool_t     canusepoison;            // Use poison without err [POIS]
  bool_t     canread;                 // Can read books and scrolls [READ]
  // [END] Skill Expansions

  Uint16     sectionsize[MAXSECTION];   // Number of choices, 0
  Uint16     sectionstart[MAXSECTION];  //

} CAP;

extern CAP CapList[MAXCAP];

#define CAP_INHERIT_IDSZ(MODEL,ID) (CapList[MODEL].idsz[IDSZ_PARENT] == (IDSZ)(ID) || CapList[MODEL].idsz[IDSZ_TYPE] == (IDSZ)(ID))
#define CAP_INHERIT_IDSZ_RANGE(MODEL,IDMIN,IDMAX) ((CapList[MODEL].idsz[IDSZ_PARENT] >= (IDSZ)(IDMIN) && CapList[MODEL].idsz[IDSZ_PARENT] <= (IDSZ)(IDMAX)) || (CapList[MODEL].idsz[IDSZ_TYPE] >= (IDSZ)(IDMIN) && CapList[MODEL].idsz[IDSZ_TYPE] <= (IDSZ)(IDMAX)) )

typedef struct lighting_data_t
{
  vect4   emission, diffuse, specular;
  GLfloat shininess[1];
} LData;


// Character data
extern int             numfreechr;         // For allocation
extern Uint16          freechrlist[MAXCHR];        //
extern Uint16          chrcollisionlevel;

typedef struct action_info_t
{
  bool_t  keep;      // Keep the action playing?
  bool_t  loop;      // Loop it too?
  bool_t  ready;     // Ready to play a new one?
  ACTION  now;       // Character's current action
  ACTION  next;      // Character's next    action
} ACTION_INFO;

ACTION_INFO * action_info_new( ACTION_INFO * a);

typedef struct animation_info_t
{
  Uint16          next;       // Character's frame
  Uint16          last;       // Character's last frame
  float           flip;
  Uint8           lip_fp8;    // Character's low-res frame in betweening
} ANIM_INFO;

ANIM_INFO * anim_info_new( ANIM_INFO * a );

typedef vect2 WAYPOINT;

typedef struct waypoint_list_t
{
  int      head, tail;
  WAYPOINT pos[MAXWAY];
} WP_LIST;

WP_LIST * wp_list_new(WP_LIST * w, vect3 * pos);
bool_t    wp_list_advance(WP_LIST * wl);
bool_t    wp_list_add(WP_LIST * wl, float x, float y);

INLINE float wp_list_x( WP_LIST * wl ) { return wl->pos[wl->tail].x; };
INLINE float wp_list_y( WP_LIST * wl ) { return wl->pos[wl->tail].y; };


typedef struct ai_state_t
{
  Uint16          type;          // The AI script to run
  int             state;         // Short term memory for AI
  int             content;       // More short term memory
  float           time;          // AI Timer
  WP_LIST         wp;
  int             x[MAXSTOR];    // Temporary values...  SetXY
  int             y[MAXSTOR];    //
  bool_t          morphed;       //Some various other stuff
  CHR_REF         target;        // Who the AI is after
  CHR_REF         owner;         // The character's owner
  CHR_REF         child;         // The character's child
  CHR_REF         bumplast;        // Last character it was bumped by
  CHR_REF         attacklast;      // Last character it was attacked by
  CHR_REF         hitlast;         // Last character it hit

  Uint16          directionlast;   // Direction of last attack/healing
  Uint8           damagetypelast;  // Last damage type
  Uint8           turnmode;        // Turning mode
  Uint32          alert;           // Alerts for AI script
  vect3           trgvel;          // target's velocity
  LATCH           latch;           // latches
} AI_STATE;

AI_STATE * ai_state_new(AI_STATE * a, Uint16 ichr);
AI_STATE * ai_state_renew(AI_STATE * a, Uint16 ichr);


typedef struct chr_terrain_light_t
{
  vect3_ui08      ambi_fp8; // 0-255, terrain light
  vect3_ui08      spek_fp8; // 0-255, terrain light
  vect3_ui16      turn_lr;  // Character's light rotation 0 to 65535
} CHR_TLIGHT;

typedef struct chr_t
{
  bool_t          on;              // Does it exist?
  char            name[MAXCAPNAMESIZE];  // Character name
  bool_t          gopoof;          // is poof requested?
  bool_t          freeme;          // is free_one_character() requested?


  matrix_4x4      matrix;          // Character's matrix
  bool_t          matrixvalid;     // Did we make one yet?

  Uint16          model;           // Character's model
  Uint16          basemodel;       // The true form

  bool_t          alive;           // Is it alive?

  bool_t          inwhichpack;     // Is it in whose inventory?
  Uint16          nextinpack;      // Link to the next item
  Uint8           numinpack;       // How many

  bool_t          openstuff;       // Can it open chests/doors?

  // stats
  Sint16          life_fp8;            // 
  Sint16          lifemax_fp8;         //
  Uint16          lifeheal;            //
  Sint16          lifereturn;          // Regeneration/poison
  Sint16          mana_fp8;            // Mana stuff
  Sint16          manamax_fp8;         //
  Sint16          manaflow_fp8;        //
  Sint16          manareturn_fp8;      //
  Sint16          strength_fp8;        // Strength
  Sint16          wisdom_fp8;          // Wisdom
  Sint16          intelligence_fp8;    // Intelligence
  Sint16          dexterity_fp8;       // Dexterity
  int             experience;          // Experience
  int             experiencelevel;     // Experience Level
  Sint32          money;               // Money
  Uint8           ammomax;             // Ammo stuff
  Uint8           ammo;                //
  GENDER          gender;              // Gender

  // stat graphic info
  Uint8           sparkle;         // Sparkle color or 0 for off
  Uint8           lifecolor;       // Bar color
  Uint8           manacolor;       // Bar color
  bool_t          staton;          // Display stats?
  bool_t          icon;            // Show the icon?

  // position info
  vect3           stt;             // Starting position
  vect3           pos;             // Character's position
  Uint16          turn_lr;   // Character's rotation 0 to 65535
  vect3           vel;             // Character's velocity
  vect3           pos_old;         // Character's last position
  Uint16          turn_lr_old;     //
  Uint16          mapturn_lr;       //
  Uint16          mapturn_ud;       //

  // physics info
  vect3           accum_acc;       //
  vect3           accum_vel;       //
  vect3           accum_pos;       //
  float           flyheight;       // Height to stabilize at
  bool_t          stickybutt;      // Rests on floor
  bool_t          inwater;         //
  float           dampen;          // Bounciness
  float           bumpstrength;    // ghost-like interaction with objects?
  float           level;           // Height under character
  bool_t          levelvalid;      // Has height been stored?

  AI_STATE        aistate;           // ai-specific into


  float           jump;            // Jump power
  float           jumptime;        // Delay until next jump
  float           jumpnumber;      // Number of jumps remaining
  Uint8           jumpnumberreset; // Number of jumps total, 255=Flying
  bool_t          jumpready;       // For standing on a platform character

  Uint32          onwhichfan;      // Where the char is
  bool_t          indolist;        // Has it been added yet?

  Uint16          uoffset_fp8;     // For moving textures
  Uint16          voffset_fp8;     //
  Uint16          uoffvel;         // Moving texture speed
  Uint16          voffvel;         //

  ACTION_INFO     action;
  ANIM_INFO       anim;

  TEAM            team;            // Character's team
  TEAM            baseteam;        // Character's starting team

  // lighting info
  vect3_ui08      vrta_fp8[MAXVERTICES];  // Lighting hack ( Ooze )
  CHR_TLIGHT      tlight;                 // terrain lighting info
  VData_Blended   vdata;                  // pre-processed per-vertex lighting data
  LData           ldata;                  // pre-processed matrial parameters
  Uint8           alpha_fp8;                 // 255 = Solid, 0 = Invisible
  Uint8           light_fp8;                 // 1 = Light, 0 = Normal
  Uint8           flashand;                  // 1,3,7,15,31 = Flash, 255 = Don't
  Uint8           sheen_fp8;           // 0-15, how shiny it is
  bool_t          transferblend;       // Give transparency to weapons?
  Uint8           redshift;            // Color channel shifting
  Uint8           grnshift;            //
  Uint8           blushift;            //
  bool_t          enviro;              // Environment map?

  Uint16          holdingwhich[SLOT_COUNT];  // !=MAXCHR if character is holding something
  Uint16          attachedto;                // !=MAXCHR if character is a held weapon
  Uint16          attachedgrip[GRIP_SIZE];   // Vertices which describe the weapon grip


  // bumber info
  BData           bmpdata;           // character bump size data
  BData           bmpdata_save;
  float           bumpdampen;      // Character bump mass

  Uint8           spd_sneak;        // Sneaking if above this speed
  Uint8           spd_walk;         // Walking if above this speed
  Uint8           spd_run;          // Running if above this speed

  DAMAGE          damagetargettype;   // Type of damage for AI DamageTarget
  DAMAGE          reaffirmdamagetype; // For relighting torches
  float           damagetime;         // Invincibility timer

  Uint16          skin_ref;           // which skin
  SKIN            skin;               // skin data

  float           weight;          // Weight ( for pressure plates )

  Uint8           passage;         // The passage associated with this character

  Uint32          message;           // The last order given the character
  Uint8           messagedata;       // The rank of the character on the order chain

  bool_t          isplatform;        // Can it be stood on
  Uint16          onwhichplatform; // What are we standing on?
  Uint16          holdingweight;   // For weighted buttons
  SLOT            inwhichslot;     // SLOT_LEFT or SLOT_RIGHT or SLOT_SADDLE
  bool_t          isequipped;      // For boots and rings and stuff


  Sint16          manacost;        // Mana cost to use
  Uint32          stoppedby;       // Collision mask

  // timers
  Sint16          grogtime;        // Grog timer
  Sint16          dazetime;        // Daze timer
  float           boretime;        // Boredom timer
  float           carefultime;     // "You hurt me!" timer
  float           reloadtime;      // Time before another shot

  bool_t          isitem;          // Is it grabbable?
  bool_t          ismount;         // Can you ride it?
  bool_t          nameknown;       // Is the name known?
  bool_t          ammoknown;       // Is the ammo known?
  bool_t          hitready;        // Was it just dropped?
  bool_t          canbecrushed;    // Crush in a door?
  bool_t          cangrabmoney;    // Picks up coins?
  bool_t          isplayer;        // btrue = player
  bool_t          islocalplayer;   // btrue = local player

  // enchant info
  Uint16          firstenchant;    // Linked list for enchants
  Uint16          undoenchant;     // Last enchantment spawned

  // missle info
  MISSLE_TYPE     missiletreatment;// For deflection, etc.
  Uint8           missilecost;     // Mana cost for each one
  Uint16          missilehandler;  // Who pays the bill for each one...

  Uint16          damageboost;     // Add to swipe damage
  bool_t          overlay;         // Is this an overlay?  Track aitarget...


  // matrix mods
  float           scale;           // Character's size (useful)
  float           fat;             // Character's size (legible)
  float           sizegoto;        // Character's size goto ( legible )
  float           sizegototime;    // Time left in siez change
  vect3           pancakepos;
  vect3           pancakevel;

  Sint8           loopingchannel;    // Channel number of the loop so
  float           loopingvolume;     // Sound volume of the channel

  // [BEGIN] Character states that are like skill expansions
  bool_t     invictus;          // Totally invincible?
  bool_t     waterwalk;         // Always above GWater.surfacelevel?
  bool_t     iskursed;          // Can't be dropped?  Could this also mean damage debuff? Spell fizzle rate? etc.
  bool_t     canseeinvisible;   //
  bool_t     canchannel;        //
  // [END] Character states that are like skill expansions

  // [BEGIN] Skill Expansions
  bool_t     canseekurse;             // Can it see kurses?
  bool_t     canusearcane;            // Can use [WMAG] spells?
  bool_t     canjoust;                // Can it use a lance to joust?
  bool_t     canusetech;              // Can it use [TECH]items?
  bool_t     canusedivine;            // Can it use [HMAG] runes?
  bool_t     candisarm;               // Disarm and find traps [DISA]
  bool_t     canbackstab;             // Backstab and murder [STAB]
  bool_t     canuseadvancedweapons;   // Advanced weapons usage [AWEP]
  bool_t     canusepoison;            // Use poison without err [POIS]
  bool_t     canread;     // Can read books and scrolls
  // [END]  Skill Expansions
} CHR;

extern CHR ChrList[MAXCHR];

void calc_cap_experience( Uint16 object );
int calc_chr_experience( Uint16 object, float level );
float calc_chr_level( Uint16 object );

bool_t make_one_character_matrix( CHR_REF cnt );
void free_one_character( CHR_REF character );
void free_inventory( CHR_REF character );
bool_t make_one_weapon_matrix( Uint16 cnt );
void make_character_matrices();
int get_free_character();
Uint32 __chrhitawall( CHR_REF character, vect3 * norm );
void play_action( CHR_REF character, ACTION action, bool_t ready );
void set_frame( CHR_REF character, Uint16 frame, Uint8 lip );
bool_t detach_character_from_mount( CHR_REF character, bool_t ignorekurse, bool_t doshop );
void drop_money( CHR_REF character, Uint16 money );
void call_for_help( CHR_REF character );
void give_experience( CHR_REF character, int amount, EXPERIENCE xptype );

void damage_character( CHR_REF character, Uint16 direction,
                       PAIR * ppair, DAMAGE damagetype, TEAM team,
                       Uint16 attacker, Uint16 effects );
void kill_character( CHR_REF character, Uint16 killer );
void spawn_poof( CHR_REF character, Uint16 profile );


void tilt_characters_to_terrain();
CHR_REF spawn_one_character( vect3 pos, int profile, TEAM team,
                            Uint8 skin, Uint16 facing, char *name, Uint16 override );

void respawn_character( CHR_REF character );
Uint16 change_armor( CHR_REF character, Uint16 skin );
void change_character( Uint16 cnt, Uint16 profile, Uint8 skin,
                       Uint8 leavewhich );
bool_t cost_mana( CHR_REF character, int amount, Uint16 killer );
bool_t attach_character_to_mount( CHR_REF character, Uint16 mount, SLOT slot );
CHR_REF stack_in_pack( CHR_REF item, CHR_REF character );
bool_t add_item_to_character_pack( Uint16 item, CHR_REF character );
Uint16 get_item_from_character_pack( CHR_REF character, SLOT slot, bool_t ignorekurse );
void drop_keys( CHR_REF character );
void drop_all_items( CHR_REF character );
bool_t character_grab_stuff( CHR_REF chara, SLOT slot, bool_t people );
void character_swipe( Uint16 cnt, SLOT slot );
void despawn_characters();
void move_characters( float dUpdate );

#define VALID_CHR_RANGE(XX) (((XX)>=0) && ((XX)<MAXCHR))
#define VALID_CHR(XX)    ( VALID_CHR_RANGE(XX) && ChrList[XX].on )
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
CHR_REF chr_get_holdingwhich( CHR_REF ichr, SLOT slot );

CHR_REF chr_get_aitarget( CHR_REF ichr );
CHR_REF chr_get_aiowner( CHR_REF ichr );
CHR_REF chr_get_aichild( CHR_REF ichr );
CHR_REF chr_get_aiattacklast( CHR_REF ichr );
CHR_REF chr_get_aibumplast( CHR_REF ichr );
CHR_REF chr_get_aihitlast( CHR_REF ichr );

Uint16 object_generate_index( char *szLoadName );
Uint16 load_one_cap( char * szModpath, char *szObjectname, Uint16 icap );

bool_t chr_bdata_reinit(CHR_REF ichr, BData * pbd);

int get_skin( char * szModpath, char * szObjectname );