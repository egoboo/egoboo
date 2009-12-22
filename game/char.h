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

/// @file char.h

#include "egoboo_typedef.h"
#include "cap_file.h"
#include "graphic_mad.h"

#include "sound.h"
#include "script.h"
#include "md2.h"
#include "graphic.h"
#include "physics.h"

#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_mad;
struct s_eve;
struct s_pip;
struct s_object_profile;
struct s_billboard_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Attack directions
#define ATK_FRONT  0x0000
#define ATK_RIGHT  0x4000
#define ATK_BEHIND 0x8000
#define ATK_LEFT   0xC000

#define MAP_TURN_OFFSET 0x8000

#define MAXXP           200000                        ///< Maximum experience
#define MAXMONEY        9999                        ///< Maximum money
#define SHOP_IDENTIFY   200                         ///< Maximum value for identifying shop items

#define MAX_CAP    MAX_PROFILE

#define INFINITE_WEIGHT          (( Uint32 )0xFFFFFFFF)
#define MAX_WEIGHT               (( Uint32 )0xFFFFFFFE)

/// The possible methods for characters to determine what direction they are facing
typedef enum e_turn_modes
{
    TURNMODE_VELOCITY = 0,                       ///< Character gets rotation from velocity (normal)
    TURNMODE_WATCH,                              ///< For watch towers, look towards waypoint
    TURNMODE_SPIN,                               ///< For spinning objects
    TURNMODE_WATCHTARGET,                        ///< For combat intensive AI
    TURNMODE_COUNT
} TURN_MODE;

#define MANARETURNSHIFT     22                      ///< ChrList.lst[ichr].manareturn/MANARETURNSHIFT = mana regen per second

#define TURNSPD             0.01f                 ///< Cutoff for turning or same direction
#define SPINRATE            200                   ///< How fast spinners spin
#define WATCHMIN            0.01f                 ///< Tolerance for TURNMODE_WATCH

/// The vertex offsets for the various grips
enum e_grip_offset
{
    GRIP_ORIGIN    =               0,                ///< Spawn attachments at the center
    GRIP_LAST      =               1,                ///< Spawn particles at the last vertex
    GRIP_LEFT      = ( 1 * GRIP_VERTS ),             ///< Left weapon grip starts  4 from last
    GRIP_RIGHT     = ( 2 * GRIP_VERTS ),             ///< Right weapon grip starts 8 from last

    // aliases
    GRIP_INVENTORY =               GRIP_ORIGIN,
    GRIP_ONLY      =               GRIP_LEFT
};
typedef enum e_grip_offset grip_offset_t;

grip_offset_t slot_to_grip_offset( slot_t slot );
slot_t        grip_offset_to_slot( grip_offset_t grip );

#define PITDEPTH            -60                     ///< Depth to kill character

#define NOSKINOVERRIDE      -1          ///< For import

#define HURTDAMAGE           256                     ///< Minimum damage for hurt animation

//Knockbacks
#define REEL                7600.0f     ///< Dampen for melee knock back
#define REELBASE            0.35f

//Water
#define WATERJUMP           12
#define RIPPLETOLERANCE     60          ///< For deep water
#define SPLASHTOLERANCE     10
#define RIPPLEAND           15          ///< How often ripples spawn

/// Stats
#define LOWSTAT             256                     ///< Worst...
#define PERFECTSTAT         (60*256)                ///< Maximum stat without magic effects
#define PERFECTBIG          (100*256)               ///< Perfect life or mana...
#define HIGHSTAT            (100*256)               ///< Absolute max adding enchantments as well

/// Throwing
#define THROWFIX            30.0f                    ///< To correct thrown velocities
#define MINTHROWVELOCITY    15.0f
#define MAXTHROWVELOCITY    75.0f

/// Inventory
#define MAXNUMINPACK        6                       ///< Max number of items to carry in pack
#define PACKDELAY           25                      ///< Time before inventory rotate again
#define GRABDELAY           25                      ///< Time before grab again

/// Z velocity
#define FLYDAMPEN           0.001f                    ///< Levelling rate for flyers
#define JUMPDELAY           20                      ///< Time between jumps
#define JUMPINFINITE        255                     ///< Flying character
#define SLIDETOLERANCE      10                      ///< Stick to ground better
#define PLATADD             -10                     ///< Height add...
#define PLATASCEND          0.10f                     ///< Ascension rate
#define PLATKEEP            0.90f                     ///< Retention rate
#define MOUNTTOLERANCE      (2 * PLATTOLERANCE)
#define STOPBOUNCING        0.1f // 1.0f                ///< To make objects stop bouncing
#define DROPZVEL            7
#define DROPXYVEL           8

//Timer resets
#define DAMAGETILETIME      32                            ///< Invincibility time
#define DAMAGETIME          16                            ///< Invincibility time
#define DEFENDTIME          16                            ///< Invincibility time
#define BORETIME            generate_randmask( 120, 255 ) ///< IfBored timer
#define CAREFULTIME         50                            ///< Friendly fire timer
#define SIZETIME            50                            ///< Time it takes to resize a character

/// Bits used to control options for the chr_get_name() function
enum e_chr_name_bits
{
    CHRNAME_NONE     = 0,               ///< no options
    CHRNAME_ARTICLE  = ( 1 << 0 ),      ///< use an article (a, an, the)
    CHRNAME_DEFINITE = ( 1 << 1 ),      ///< if set, choose "the" else "a" or "an"
    CHRNAME_CAPITAL  = ( 1 << 2 )       ///< capitalize the name
};

//------------------------------------
/// Team variables
//------------------------------------
enum e_team_types
{
    TEAM_EVIL            = ( 'E' - 'A' ),        ///< Evil team
    TEAM_GOOD            = ( 'G' - 'A' ),        ///< Good team
    TEAM_NULL            = ( 'N' - 'A' ),        ///< Null or Neutral team
    TEAM_ZIPPY           = ( 'Z' - 'A' ),        ///< Zippy Team?
    TEAM_DAMAGE,                                 ///< For damage tiles
    TEAM_MAX
};

#define NOLEADER            0xFFFF                   ///< If the team has no leader...

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// The description of a single team
struct s_team
{
    bool_t  hatesteam[TEAM_MAX];     ///< Don't damage allies...
    Uint16  morale;                 ///< Number of characters on team
    Uint16  leader;                 ///< The leader of the team
    Uint16  sissy;                  ///< Whoever called for help last
};
typedef struct s_team team_t;

//--------------------------------------------------------------------------------------------
/// Everything that is necessary to compute the character's interaction with the environment
struct s_chr_environment
{
    // floor stuff
    Uint8  twist;

    float  floor_level;           ///< Height of tile
    float  level;                 ///< Height of a tile or a platform
    float  fly_level;             ///< Height of tile, platform, or water, whever is highest.

    float  zlerp;
    bool_t grounded;              ///< standing on something?

    // friction stuff
    bool_t is_slipping;
    bool_t is_slippy,    is_watery;
    float  air_friction, ice_friction;
    float  fluid_friction_xy, fluid_friction_z;
    float  traction, friction_xy;

    // misc states
    bool_t   inwater;
    float    new_vx, new_vy;
    fvec3_t   acc;
};
typedef struct s_chr_environment chr_environment_t;

//--------------------------------------------------------------------------------------------
/// The definition of the character object
/// This "inherits" for ego_object_base_t
struct s_chr
{
    ego_object_base_t obj_base;

    // character state
    ai_state_t     ai;              ///< ai data
    latch_t        latch;

    // character stats
    STRING         Name;
    Uint8          gender;          ///< Gender

    Uint8          lifecolor;       ///< Bar color
    Sint16         life;            ///< Basic character stats
    Sint16         lifemax;         ///<  All 8.8 fixed point
    Uint16         lifeheal;
    Sint16         lifereturn;      ///< Regeneration/poison

    Uint8          manacolor;       ///< Bar color
    Sint16         mana;            ///< Mana stuff
    Sint16         manamax;
    Sint16         manaflow;
    Sint16         manareturn;

    Sint16         strength;        ///< Strength
    Sint16         wisdom;          ///< Wisdom
    Sint16         intelligence;    ///< Intelligence
    Sint16         dexterity;       ///< Dexterity

    Uint32         experience;                    ///< Experience
    Uint8          experiencelevel;               ///< Experience Level

    // what the character is holding
    Uint8          pack_ispacked;    ///< Is it in the inventory?
    Uint8          pack_waspacked;   ///< Temporary thing...
    Uint16         pack_next;        ///< Link to the next item
    Uint8          pack_count;       ///< How many
    Sint16         money;            ///< Money
    Uint8          ammomax;          ///< Ammo stuff
    Uint16         ammo;
    Uint16         holdingwhich[SLOT_COUNT]; ///< !=MAX_CHR if character is holding something
    Uint16         inventory[INVEN_COUNT];   ///< !=MAX_CHR if character is storing something

    // team stuff
    Uint8          team;            ///< Character's team
    Uint8          baseteam;        ///< Character's starting team

    // enchant data
    Uint16         firstenchant;                  ///< Linked list for enchants
    Uint16         undoenchant;                   ///< Last enchantment spawned

    float          fat;                           ///< Character's size
    float          fat_goto;                      ///< Character's size goto
    Sint16         fat_goto_time;                  ///< Time left in size change

    // jump stuff
    float          jump_power;                    ///< Jump power
    Uint8          jumptime;                      ///< Delay until next jump
    Uint8          jumpnumber;                    ///< Number of jumps remaining
    Uint8          jumpnumberreset;               ///< Number of jumps total, 255=Flying
    Uint8          jumpready;                     ///< For standing on a platform character

    // attachments
    Uint16         attachedto;                    ///< !=MAX_CHR if character is a held weapon
    slot_t         inwhich_slot;                  ///< SLOT_LEFT or SLOT_RIGHT

    // platform stuff
    Uint8          platform;                      ///< Can it be stood on
    Uint16         holdingweight;                 ///< For weighted buttons
    Uint16         onwhichplatform;               ///< Am I on a platform?

    // combat stuff
    Uint8          damagetargettype;              ///< Type of damage for AI DamageTarget
    Uint8          reaffirmdamagetype;            ///< For relighting torches
    Uint8          damagemodifier[DAMAGE_COUNT];  ///< Resistances and inversion
    Uint8          defense;                       ///< Base defense rating
    Uint16         damageboost;                   ///< Add to swipe damage
    Uint16         damagethreshold;               ///< Damage below this number is ignored

    // sound stuff
    Sint8          sound_index[SOUND_COUNT];       ///< a map for soundX.wav to sound types
    int            loopedsound_channel;           ///< Which sound channel it is looping on, -1 is none.

    // missle handling
    Uint8          missiletreatment;              ///< For deflection, etc.
    Uint8          missilecost;                   ///< Mana cost for each one
    Uint16         missilehandler;                ///< Who pays the bill for each one...

    // "variable" properties
    bool_t         is_hidden;
    Uint8          alive;                         ///< Is it alive?
    Uint8          waskilled;                     ///< Fix for network
    bool_t         isplayer;                      ///< btrue = player
    bool_t         islocalplayer;                 ///< btrue = local player
    Uint8          invictus;                      ///< Totally invincible?
    Uint8          iskursed;                      ///< Can't be dropped?
    Uint8          nameknown;                     ///< Is the name known?
    Uint8          ammoknown;                     ///< Is the ammo known?
    Uint8          hitready;                      ///< Was it just dropped?
    Uint8          isequipped;                    ///< For boots and rings and stuff

    // "constant" properties
    Uint8          isitem;                        ///< Is it grabbable?
    bool_t         cangrabmoney;                  ///< Picks up coins?
    Uint8          openstuff;                     ///< Can it open chests/doors?
    Uint8          stickybutt;                    ///< Rests on floor
    bool_t         isshopitem;                    ///< Spawned in a shop?
    Uint8          ismount;                       ///< Can you ride it?
    bool_t         canbecrushed;                  ///< Crush in a door?
    bool_t         canchannel;                    ///< Can it convert life to mana?
    Sint16         manacost;                      ///< Mana cost to use

    // misc timers
    Sint16         grogtime;                      ///< Grog timer
    Sint16         dazetime;                      ///< Daze timer
    Sint16         boretime;                      ///< Boredom timer
    Uint8          carefultime;                   ///< "You hurt me!" timer
    Uint16         reloadtime;                    ///< Time before another shot
    Uint8          damagetime;                    ///< Invincibility timer

    // graphica info
    Uint8          flashand;        ///< 1,3,7,15,31 = Flash, 255 = Don't
    Uint8          transferblend;   ///< Give transparency to weapons?
    bool_t         icon;            ///< Show the icon?
    Uint8          sparkle;         ///< Sparkle color or 0 for off
    Uint8          StatusList_on;          ///< Display stats?
    Uint16         uoffvel;         ///< Moving texture speed
    Uint16         voffvel;
    Uint32         shadowsize;      ///< Size of shadow
    Uint32         shadowsizesave;  ///< Without size modifiers
    Uint16         ibillboard;      ///< The attached billboard

    // model info
    bool_t         is_overlay;                    ///< Is this an overlay? Track aitarget...
    Uint16         skin;                          ///< Character's skin
    Uint8          iprofile;                      ///< Character's profile
    Uint8          basemodel;                     ///< The true form
    Uint8          basealpha;
    Uint8          baselight;
    chr_instance_t inst;                          ///< the render data

    // Skills
    int           shieldproficiency;            ///< Can it use shields?
    int           canjoust;
    int           canuseadvancedweapons;
    int           see_invisible_level;
    int           canseekurse;
    int           canusedivine;
    int           canusearcane;
    int           canusetech;
    int           candisarm;
    int           canbackstab;
    int           canusepoison;
    int           canread;
    int           hascodeofconduct;
    int           darkvision_level;
    int           darkvision_level_base;

    /// collision info

    /// @note - to make it easier for things to "hit" one another (like a damage particle from
    ///        a torch hitting a grub bug), Aaron sometimes made the bumper size much different
    ///        than the shape of the actual object.
    ///        The old bumper data that is read from the data.txt file will be kept in
    ///        the struct "bump". A new bumper that actually matches the size of the object will
    ///        be kept in the struct "collision"
    bumper_t     bump;
    bumper_t     bump_save;

    bumper_t     bump_1;       ///< the loosest collision volume that mimics the current bump
    oct_bb_t     chr_prt_cv;   ///< a looser collision volume for chr-prt interactions
    oct_bb_t     chr_chr_cv;   ///< the tightest collision volume for chr-chr interactions

    Uint8          stoppedby;                     ///< Collision mask

    // character location data
    fvec3_t        pos_stt;                       ///< Starting position
    fvec3_t        pos;                           ///< Character's position
    fvec3_t        vel;                           ///< Character's velocity

    Uint16         turn_z;                        ///< Character's z-rotation 0 to 0xFFFF
    Uint16         map_turn_y;
    Uint16         map_turn_x;

    fvec3_t        pos_old;                       ///< Character's last position

    bool_t         safe_valid;
    fvec3_t        pos_safe;                      ///< Character's last safe position

    fvec3_t        vel_old;                       ///< Character's last velocity

    Uint16         turn_old_z;

    Uint32         onwhichfan;                    ///< Where the char is
    Uint32         onwhichblock;                  ///< The character's collision block
    Uint16         bumplist_next;                 ///< Next character on fanblock

    // movement properties
    Uint8          waterwalk;                     ///< Always above watersurfacelevel?
    TURN_MODE      turnmode;                      ///< Turning mode
    Uint8          sneakspd;                      ///< Sneaking if above this speed
    Uint8          walkspd;                       ///< Walking if above this speed
    Uint8          runspd;                        ///< Running if above this speed
    float          maxaccel;                      ///< Maximum acceleration
    Uint8          flyheight;                     ///< Height to stabilize at

    // data for doing the physics in bump_all_objects()
    phys_data_t       phys;
    chr_environment_t enviro;

    // a timer BB added in to make mounts and dismounts not so unpredictable
    int                dismount_timer;

};

typedef struct s_chr chr_t;

//--------------------------------------------------------------------------------------------
// list definitions
//--------------------------------------------------------------------------------------------

extern team_t TeamList[TEAM_MAX];

extern cap_t CapList[MAX_CAP];

#define VALID_CAP_RANGE( ICAP ) ( ((ICAP) >= 0) && ((ICAP) < MAX_CAP) )
#define LOADED_CAP( ICAP )       ( VALID_CAP_RANGE( ICAP ) && CapList[ICAP].loaded )

DEFINE_LIST_EXTERN( chr_t, ChrList, MAX_CHR );

#define VALID_CHR_RANGE( ICHR ) ( ((ICHR) >= 0) && ((ICHR) < MAX_CHR) )
#define ALLOCATED_CHR( ICHR )   ( VALID_CHR_RANGE( ICHR ) && ALLOCATED_PBASE ( &(ChrList.lst[ICHR].obj_base) ) )
#define ACTIVE_CHR( ICHR )      ( VALID_CHR_RANGE( ICHR ) && ACTIVE_PBASE    ( &(ChrList.lst[ICHR].obj_base) ) )
#define WAITING_CHR( ICHR )     ( VALID_CHR_RANGE( ICHR ) && WAITING_PBASE   ( &(ChrList.lst[ICHR].obj_base) ) )
#define TERMINATED_CHR( ICHR )  ( VALID_CHR_RANGE( ICHR ) && TERMINATED_PBASE( &(ChrList.lst[ICHR].obj_base) ) )

#define ACTIVE_PCHR( PCHR )     ( (NULL != (PCHR)) && VALID_CHR_RANGE( GET_INDEX_POBJ( PCHR, MAX_CHR) ) && ACTIVE_PBASE( POBJ_GET_PBASE( (PCHR) ) ) )
#define ALLOCATED_PCHR( PCHR )  ( (NULL != (PCHR)) && VALID_CHR_RANGE( GET_INDEX_POBJ( PCHR, MAX_CHR) ) && ALLOCATED_PBASE( POBJ_GET_PBASE( (PCHR) ) ) )

#define GET_INDEX_PCHR( PCHR )  GET_INDEX_POBJ( PCHR, MAX_CHR )

#define CHR_BEGIN_LOOP(IT, PCHR) {int IT##internal; for(IT##internal=0;IT##internal<ChrList.used_count;IT##internal++) { int IT; chr_t * PCHR = NULL; IT = ChrList.used_ref[IT##internal]; if(!ACTIVE_CHR(IT)) continue; PCHR = ChrList.lst + IT;
#define CHR_END_LOOP() }}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Function prototypes
void drop_money( Uint16 character, Uint16 money );
void call_for_help( Uint16 character );
void give_experience( Uint16 character, int amount, Uint8 xptype, bool_t override_invictus );
void give_team_experience( Uint8 team, int amount, Uint8 xptype );
int  damage_character( Uint16 character, Uint16 direction,
                       IPair damage, Uint8 damagetype, Uint8 team,
                       Uint16 attacker, Uint16 effects, bool_t ignore_invictus );
void kill_character( Uint16 character, Uint16 killer, bool_t ignore_invictus );
bool_t heal_character( Uint16 character, Uint16 healer, int amount, bool_t ignore_invictus );
void spawn_poof( Uint16 character, Uint16 profile );
void reset_character_alpha( Uint16 character );
void reset_character_accel( Uint16 character );
bool_t detach_character_from_mount( Uint16 character, Uint8 ignorekurse, Uint8 doshop );

void flash_character_height( Uint16 character, Uint8 valuelow, Sint16 low,
                             Uint8 valuehigh, Sint16 high );
void flash_character( Uint16 character, Uint8 value );

void free_one_character_in_game( Uint16 character );
//void make_one_weapon_matrix( Uint16 iweap, Uint16 iholder, bool_t do_phys  );
void make_all_character_matrices( bool_t do_physics );
void free_inventory_in_game( Uint16 character );

void keep_weapons_with_holders();
void make_one_character_matrix( Uint16 cnt );

void update_all_characters( void );
void move_all_characters( void );
void cleanup_all_characters( void );

void do_level_up( Uint16 character );
bool_t setup_xp_table( Uint16 character );

void free_all_chraracters();

Uint32 __chrhitawall( chr_t * pchr, float nrm[] );

int chr_count_free();

Uint16 spawn_one_character( fvec3_t   pos, Uint16 profile, Uint8 team, Uint8 skin, Uint16 facing, const char *name, Uint16 override );
void respawn_character( Uint16 character );
Uint16 change_armor( Uint16 character, Uint16 skin );
void change_character( Uint16 cnt, Uint16 profile, Uint8 skin, Uint8 leavewhich );
void change_character_full( Uint16 ichr, Uint16 profile, Uint8 skin, Uint8 leavewhich );
bool_t cost_mana( Uint16 character, int amount, Uint16 killer );
void switch_team( Uint16 character, Uint8 team );
void issue_clean( Uint16 character );
int  restock_ammo( Uint16 character, IDSZ idsz );
void attach_character_to_mount( Uint16 character, Uint16 mount, grip_offset_t grip_off );
bool_t inventory_add_item( Uint16 item, Uint16 character );
Uint16 inventory_get_item( Uint16 character, grip_offset_t grip_off, bool_t ignorekurse );
void drop_keys( Uint16 character );
bool_t drop_all_items( Uint16 character );
bool_t character_grab_stuff( Uint16 chara, grip_offset_t grip, bool_t people );

bool_t export_one_character_name( const char *szSaveName, Uint16 character );
bool_t export_one_character_profile( const char *szSaveName, Uint16 character );
bool_t export_one_character_skin( const char *szSaveName, Uint16 character );
int    load_one_character_profile( const char *szLoadName, int slot_override, bool_t required );

void character_swipe( Uint16 cnt, slot_t slot );

int check_skills( Uint16 who, IDSZ whichskill );

bool_t looped_stop_object_sounds( Uint16 character );

bool_t is_invictus_direction( Uint16 direction, Uint16 character, Uint16 effects );

void   init_slot_idsz();

bool_t ai_add_order( ai_state_t * pai, Uint32 value, Uint16 counter );

struct s_billboard_data * chr_make_text_billboard( Uint16 ichr, const char * txt, SDL_Color color, int lifetime_secs );
const char * chr_get_name( Uint16 ichr, Uint32 bits );
const char * chr_get_dir_name( Uint16 ichr );

void   ChrList_update_used();

//---------------------------------------------------------------------------------------------
/// helper functions

void init_all_cap();
void release_all_cap();
bool_t release_one_cap( Uint16 icap );

bool_t cap_is_type_idsz( Uint16 icap, IDSZ test_idsz );

Uint16 chr_get_ipro( Uint16 ichr );
Uint16 chr_get_icap( Uint16 ichr );
Uint16 chr_get_imad( Uint16 ichr );
Uint16 chr_get_ieve( Uint16 ichr );
Uint16 chr_get_ipip( Uint16 ichr, Uint16 ipip );
Uint16 chr_get_iteam( Uint16 ichr );
Uint16 chr_get_iteam_base( Uint16 ichr );

struct s_object_profile * chr_get_ppro( Uint16 ichr );
struct s_cap * chr_get_pcap( Uint16 ichr );
struct s_mad * chr_get_pmad( Uint16 ichr );
struct s_eve * chr_get_peve( Uint16 ichr );
struct s_pip * chr_get_ppip( Uint16 ichr, Uint16 ipip );

Mix_Chunk      * chr_get_chunk_ptr( chr_t * pchr, int index );
Mix_Chunk      * chr_get_chunk( Uint16 ichr, int index );
team_t         * chr_get_pteam( Uint16 ichr );
team_t         * chr_get_pteam_base( Uint16 ichr );
ai_state_t     * chr_get_pai( Uint16 ichr );
chr_instance_t * chr_get_pinstance( Uint16 ichr );

Uint16   team_get_ileader( Uint16 iteam );
chr_t  * team_get_pleader( Uint16 iteam );

bool_t team_hates_team( Uint16 ipredator, Uint16 iprey );

IDSZ chr_get_idsz( Uint16 ichr, Uint16 type );

void chr_update_size( chr_t * pchr );
void chr_init_size( chr_t * pchr, cap_t * pcap );
void chr_set_size( chr_t * pchr, float size );
void chr_set_width( chr_t * pchr, float width );
void chr_set_shadow( chr_t * pchr, float width );
void chr_set_height( chr_t * pchr, float height );
void chr_set_fat( chr_t * pchr, float fat );

bool_t chr_has_idsz( Uint16 ichr, IDSZ idsz );
bool_t chr_is_type_idsz( Uint16 ichr, IDSZ idsz );
bool_t chr_has_vulnie( Uint16 item, Uint16 weapon_profile );

Uint32 chr_get_icon_ref( Uint16 item );

const char * describe_value( float value, float maxval, int * rank_ptr );
const char* describe_damage( float value, float maxval, int * rank_ptr );

void reset_teams();

egoboo_rv chr_update_matrix( chr_t * pchr, bool_t update_size );
bool_t chr_teleport( Uint16 ichr, float x, float y, float z, Uint16 turn_z );

bool_t chr_request_terminate( Uint16 ichr );

chr_t * chr_update_hide( chr_t * pchr );

bool_t ai_state_set_changed( ai_state_t * pai );

bool_t chr_matrix_valid( chr_t * pchr );

bool_t chr_getMatUp( chr_t *pchr, fvec3_t   * pvec );
bool_t chr_getMatRight( chr_t *pchr, fvec3_t   * pvec );
bool_t chr_getMatForward( chr_t *pchr, fvec3_t   * pvec );
bool_t chr_getMatTranslate( chr_t *pchr, fvec3_t   * pvec );

egoboo_rv chr_update_collision_size( chr_t * pchr, bool_t update_matrix );

Uint16 chr_has_inventory_idsz( Uint16 ichr, IDSZ idsz, bool_t equipped, Uint16 * pack_last );
Uint16 chr_holding_idsz( Uint16 ichr, IDSZ idsz );
Uint16 chr_has_item_idsz( Uint16 ichr, IDSZ idsz, bool_t equipped, Uint16 * pack_last );

bool_t apply_reflection_matrix( chr_instance_t * pinst, float floor_level );

bool_t chr_can_see_object( Uint16 ichr, Uint16 iobj );
int    chr_get_price( Uint16 ichr );

void chr_set_floor_level( chr_t * pchr, float level );
void chr_set_redshift( chr_t * pchr, int rs );
void chr_set_grnshift( chr_t * pchr, int gs );
void chr_set_blushift( chr_t * pchr, int bs );
void chr_set_sheen( chr_t * pchr, int sheen );
void chr_set_alpha( chr_t * pchr, int alpha );
void chr_set_light( chr_t * pchr, int light );

void chr_instance_get_tint( chr_instance_t * pinst, GLfloat * tint, Uint32 bits );
void chr_instance_free_one( chr_instance_t * pinst );

Uint16 chr_get_lowest_attachment( Uint16 ichr, bool_t non_item );

void character_system_init();

bool_t chr_get_mass_pair( chr_t * pchr_a, chr_t * pchr_b, float * wta, float * wtb );

bool_t chr_can_mount( Uint16 ichr_a, Uint16 ichr_b );

Uint32 chr_get_framefx( chr_t * pchr );

void      chr_set_frame( Uint16 character, Uint16 action, int frame, Uint16 lip );

egoboo_rv chr_set_action( chr_t * pchr, int action, bool_t action_ready, bool_t override_action );
egoboo_rv chr_start_anim( chr_t * pchr, int action, bool_t action_ready, bool_t override_action );
egoboo_rv chr_set_anim( chr_t * pchr, int action, int frame, bool_t action_ready, bool_t override_action );
egoboo_rv chr_increment_action( chr_t * pchr );
egoboo_rv chr_increment_frame( chr_t * pchr );
egoboo_rv chr_play_action( chr_t * pchr, int action, bool_t action_ready );
