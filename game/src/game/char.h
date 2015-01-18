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

/// @file game/char.h
/// @note You will routinely include "char.h" only in headers (*.h) files where you need to declare an
///       object of team_t or chr_t. In *.inl files or *.c/*.cpp files you will routinely include "char.inl", instead.

#pragma once

#include "game/egoboo_typedef.h"
#include "game/egoboo_object.h"
#include "game/graphic.h"
#include "game/graphic_mad.h"
#include "game/script.h"
#include "game/physics.h"
#include "game/egoboo.h"

#include "game/graphics/MD2Model.hpp"
#include "game/profiles/Profile.hpp"
#include "game/enchant.h"
#include "game/particle.h"

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

struct mad_t;
struct eve_t;
struct s_pip;
struct s_object_profile;
struct billboard_data_t;
struct mesh_wall_data_t;

struct s_prt;
class ObjectProfile;

//--------------------------------------------------------------------------------------------
// internal structs
//--------------------------------------------------------------------------------------------
struct team_t;
struct chr_environment_t;
struct chr_spawn_data_t;
struct chr_t;


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Attack directions
#define ATK_FRONT  0x0000
#define ATK_RIGHT  0x4000
#define ATK_BEHIND 0x8000
#define ATK_LEFT   0xC000

#define MAP_TURN_OFFSET 0x8000

#define MAXXP           200000                      ///< Maximum experience
#define MAXMONEY        9999                        ///< Maximum money
#define SHOP_IDENTIFY   200                         ///< Maximum value for identifying shop items

#define CHR_INFINITE_WEIGHT          (( Uint32 )0xFFFFFFFF)
#define CHR_MAX_WEIGHT               (( Uint32 )0xFFFFFFFE)

#define GRABSIZE            90.0f //135.0f             ///< Grab tolerance
#define SEEINVISIBLE        128                        ///< Cutoff for invisible characters

#define RAISE               12                  ///< Helps correct z level

#define MANARETURNSHIFT     44                    ///< mana_return/MANARETURNSHIFT = mana regen per second

#define TURNSPD             0.01f                 ///< Cutoff for turning or same direction
#define SPINRATE            200                   ///< How fast spinners spin
#define WATCHMIN            0.01f                 ///< Tolerance for TURNMODE_WATCH

#define PITDEPTH            -60                     ///< Depth to kill character
#define HURTDAMAGE           256                    ///< Minimum damage for hurt animation

//Dismounting
#define DISMOUNTZVEL        16
#define DISMOUNTZVELFLY     4
#define PHYS_DISMOUNT_TIME  (TICKS_PER_SEC*0.05f)          ///< time delay for full object-object interaction (approximately 0.05 second)

//Knockbacks
#define REEL                7600.0f     ///< Dampen for melee knock back
#define REELBASE            0.35f

//Water
#define RIPPLETOLERANCE     60          ///< For deep water
#define SPLASHTOLERANCE     10
#define RIPPLEAND           15          ///< How often ripples spawn

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
#define WATERJUMP           25                        ///< How good we jump in water
#define JUMPINFINITE        255                     ///< Flying character
#define SLIDETOLERANCE      10                      ///< Stick to ground better
#define PLATADD             -10                     ///< Height add...
#define PLATASCEND          0.10f                     ///< Ascension rate
#define PLATKEEP            0.90f                     ///< Retention rate
#define MOUNTTOLERANCE      (PLATTOLERANCE)
#define STOPBOUNCING        2.00f                     ///< To make objects stop bouncing
#define DROPZVEL            7
#define DROPXYVEL           12

//Timer resets
#define DAMAGETILETIME      32                            ///< Invincibility time
#define DAMAGETIME          32                            ///< Invincibility time
#define DEFENDTIME          24                            ///< Invincibility time
#define BORETIME            ((Uint16)generate_randmask( 255, 511 )) ///< IfBored timer
#define CAREFULTIME         50                            ///< Friendly fire timer
#define SIZETIME            100                           ///< Time it takes to resize a character

// team constants
#define TEAM_NOLEADER       0xFFFF                        ///< If the team has no leader...

#define PACK_BEGIN_LOOP(INVENTORY, PITEM, IT) { int IT##_internal; for(IT##_internal=0;IT##_internal<MAXNUMINPACK;IT##_internal++) { CHR_REF IT; chr_t * PITEM = NULL; IT = (CHR_REF)INVENTORY[IT##_internal]; if(!ACTIVE_CHR (IT)) continue; PITEM = ChrList_get_ptr( IT );
#define PACK_END_LOOP() } }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The possible methods for characters to determine what direction they are facing
enum turn_mode_t
{
    TURNMODE_VELOCITY = 0,                       ///< Character gets rotation from velocity (normal)
    TURNMODE_WATCH,                              ///< For watch towers, look towards waypoint
    TURNMODE_SPIN,                               ///< For spinning objects
    TURNMODE_WATCHTARGET,                        ///< For combat intensive AI
    TURNMODE_COUNT
};

/// The vertex offsets for the various grips
enum grip_offset_t
{
    GRIP_ORIGIN    =               0,                ///< Spawn attachments at the center
    GRIP_LAST      =               1,                ///< Spawn particles at the last vertex
    GRIP_LEFT      = ( 1 * GRIP_VERTS ),             ///< Left weapon grip starts  4 from last
    GRIP_RIGHT     = ( 2 * GRIP_VERTS ),             ///< Right weapon grip starts 8 from last

    // aliases
    GRIP_INVENTORY =               GRIP_ORIGIN,
    GRIP_ONLY      =               GRIP_LEFT
};

/// Bits used to control options for the chr_get_name() function
enum e_chr_name_bits
{
    CHRNAME_NONE     = 0,               ///< no options
    CHRNAME_ARTICLE  = ( 1 << 0 ),      ///< use an article (a, an, the)
    CHRNAME_DEFINITE = ( 1 << 1 ),      ///< if set, choose "the" else "a" or "an"
    CHRNAME_CAPITAL  = ( 1 << 2 )       ///< capitalize the name
};

enum e_chr_movement_idx
{
    CHR_MOVEMENT_STOP  = 0,
    CHR_MOVEMENT_SNEAK,
    CHR_MOVEMENT_WALK,
    CHR_MOVEMENT_RUN,
    CHR_MOVEMENT_COUNT
};

enum e_chr_movement_bits
{
    CHR_MOVEMENT_NONE  = 0,
    CHR_MOVEMENT_BITS_STOP  = 1 << CHR_MOVEMENT_STOP,
    CHR_MOVEMENT_BITS_SNEAK = 1 << CHR_MOVEMENT_SNEAK,
    CHR_MOVEMENT_BITS_WALK  = 1 << CHR_MOVEMENT_WALK,
    CHR_MOVEMENT_BITS_RUN   = 1 << CHR_MOVEMENT_RUN
};

enum e_team_types
{
    TEAM_EVIL            = ( 'E' - 'A' ),        ///< Evil team
    TEAM_GOOD            = ( 'G' - 'A' ),        ///< Good team
    TEAM_NULL            = ( 'N' - 'A' ),        ///< Null or Neutral team
    TEAM_ZIPPY           = ( 'Z' - 'A' ),        ///< Zippy Team?
    TEAM_DAMAGE,                                 ///< For damage tiles
    TEAM_MAX
};

//--------------------------------------------------------------------------------------------

/// The description of a single team
struct team_t
{
    bool hatesteam[TEAM_MAX];    ///< Don't damage allies...
    Uint16   morale;                 ///< Number of characters on team
    CHR_REF  leader;                 ///< The leader of the team
    CHR_REF  sissy;                  ///< Whoever called for help last
};

//--------------------------------------------------------------------------------------------

/// Everything that is necessary to compute the character's interaction with the environment
struct chr_environment_t
{
    // floor stuff
    Uint8   grid_twist;           ///< The twist parameter of the current grid (what angle it it at)
    float   grid_level;           ///< Height relative to the current grid
    float   grid_lerp;

    float   water_level;           ///< Height relative to the current water level
    float   water_lerp;

    float  floor_level;           ///< Height of tile
    float  level;                 ///< Height of a tile or a platform
    float  fly_level;             ///< Height of tile, platform, or water, whever is highest.

    float  zlerp;

    fvec3_t floor_speed;

    // friction stuff
    bool is_slipping;
    bool is_slippy,    is_watery;
    float  air_friction, ice_friction;
    float  fluid_friction_hrz, fluid_friction_vrt;
    float  traction, friction_hrz;

    // misc states
    bool inwater;
    bool grounded;              ///< standing on something?

    // various motion parameters
    fvec3_t  new_v;
    fvec3_t  acc;
    fvec3_t  vel;
};

//--------------------------------------------------------------------------------------------

/// the data used to define the spawning of a character
struct chr_spawn_data_t
{
    fvec3_t     pos;
    PRO_REF     profile;
    TEAM_REF    team;
    int         skin;
    FACING_T    facing;
    STRING      name;
    CHR_REF     override;
};

//--------------------------------------------------------------------------------------------

/// The definition of the character object.
/// @extends Ego::Entity
struct chr_t
{
    Ego::Entity obj_base;            ///< The "inheritance" from Ego::Entity.

    chr_spawn_data_t  spawn_data;

    // character state
    ai_state_t     ai;              ///< ai data
    latch_t        latch;

    // character stats
    STRING         Name;            ///< My name
    Uint8          gender;          ///< Gender

    Uint8          life_color;      ///< Bar color
	SFP8_T         life;            ///< (signed 8.8 fixed point)
	UFP8_T         life_max;        ///< (unsigned 8.8 fixed point) @inv life_max >= life
    SFP8_T         life_return;     ///< Regeneration/poison - (8.8 fixed point)

    Uint8          mana_color;      ///< Bar color
	SFP8_T         mana;            ///< (signed 8.8 fixed point)
	UFP8_T         mana_max;        ///< (unsigned 8.8 fixed point) @inv mana_max >= mana
    SFP8_T         mana_return;     ///< (8.8 fixed point)

	SFP8_T         mana_flow;       ///< (8.8 fixed point)

    SFP8_T         strength;        ///< Strength     - (8.8 fixed point)
    SFP8_T         wisdom;          ///< Wisdom       - (8.8 fixed point)
    SFP8_T         intelligence;    ///< Intelligence - (8.8 fixed point)
    SFP8_T         dexterity;       ///< Dexterity    - (8.8 fixed point)

    Uint32         experience;      ///< Experience
    Uint8          experiencelevel; ///< Experience Level

    Sint16         money;            ///< Money
    Uint16         ammomax;          ///< Ammo stuff
    Uint16         ammo;

    // equipment and inventory
    CHR_REF        holdingwhich[SLOT_COUNT]; ///< != INVALID_CHR_REF if character is holding something
    CHR_REF        equipment[INVEN_COUNT];   ///< != INVALID_CHR_REF if character has equipped something
    CHR_REF        inventory[MAXNUMINPACK];  ///< != INVALID_CHR_REF if character has something in the inventory

    // team stuff
    TEAM_REF       team;            ///< Character's team
    TEAM_REF       team_base;        ///< Character's starting team

    // enchant data
    ENC_REF        firstenchant;                  ///< Linked list for enchants
    ENC_REF        undoenchant;                   ///< Last enchantment spawned

    float          fat_stt;                       ///< Character's initial size
    float          fat;                           ///< Character's size
    float          fat_goto;                      ///< Character's size goto
    Sint16         fat_goto_time;                 ///< Time left in size change

    // jump stuff
    float          jump_power;                    ///< Jump power
    Uint8          jump_timer;                      ///< Delay until next jump
    Uint8          jumpnumber;                    ///< Number of jumps remaining
    Uint8          jumpnumberreset;               ///< Number of jumps total, 255=Flying
    Uint8          jumpready;                     ///< For standing on a platform character

    // attachments
    CHR_REF        attachedto;                    ///< != INVALID_CHR_REF if character is a held weapon
    slot_t         inwhich_slot;                  ///< SLOT_LEFT or SLOT_RIGHT
    CHR_REF        inwhich_inventory;             ///< != INVALID_CHR_REF if character is inside an inventory

    // platform stuff
    bool         platform;                      ///< Can it be stood on
    bool         canuseplatforms;               ///< Can use platforms?
    int            holdingweight;                 ///< For weighted buttons
    float          targetplatform_level;          ///< What is the height of the target platform?
    CHR_REF        targetplatform_ref;            ///< Am I trying to attach to a platform?
    CHR_REF        onwhichplatform_ref;           ///< Am I on a platform?
    Uint32         onwhichplatform_update;        ///< When was the last platform attachment made?

    // combat stuff
    Uint8          damagetarget_damagetype;       ///< Type of damage for AI DamageTarget
    Uint8          reaffirm_damagetype;           ///< For relighting torches
    Uint8          damage_modifier[DAMAGE_COUNT]; ///< Damage inversion
    float          damage_resistance[DAMAGE_COUNT]; ///< Damage Resistances
    Uint8          defense;                       ///< Base defense rating
    SFP8_T         damage_boost;                  ///< Add to swipe damage (8.8 fixed point)
    SFP8_T         damage_threshold;              ///< Damage below this number is ignored (8.8 fixed point)

    // missle handling
    Uint8          missiletreatment;              ///< For deflection, etc.
    Uint8          missilecost;                   ///< Mana cost for each one
    CHR_REF        missilehandler;                ///< Who pays the bill for each one...

    // "variable" properties
    bool         is_hidden;
    bool         alive;                         ///< Is it alive?
    bool         waskilled;                     ///< Fix for network
    PLA_REF        is_which_player;               ///< true = player
    bool         islocalplayer;                 ///< true = local player
    bool         invictus;                      ///< Totally invincible?
    bool         iskursed;                      ///< Can't be dropped?
    bool         nameknown;                     ///< Is the name known?
    bool         ammoknown;                     ///< Is the ammo known?
    bool         hitready;                      ///< Was it just dropped?
    bool         isequipped;                    ///< For boots and rings and stuff

    // "constant" properties
    bool         isitem;                        ///< Is it grabbable?
    bool         cangrabmoney;                  ///< Picks up coins?
    bool         openstuff;                     ///< Can it open chests/doors?
    bool         stickybutt;                    ///< Rests on floor
    bool         isshopitem;                    ///< Spawned in a shop?
    bool         ismount;                       ///< Can you ride it?
    bool         canbecrushed;                  ///< Crush in a door?
    bool         canchannel;                    ///< Can it convert life to mana?

    // misc timers
    Sint16         grog_timer;                    ///< Grog timer
    Sint16         daze_timer;                    ///< Daze timer
    Sint16         bore_timer;                    ///< Boredom timer
    Uint8          careful_timer;                 ///< "You hurt me!" timer
    Uint16         reload_timer;                  ///< Time before another shot
    Uint8          damage_timer;                  ///< Invincibility timer

    // graphica info
    Uint8          flashand;        ///< 1,3,7,15,31 = Flash, 255 = Don't
    bool         transferblend;   ///< Give transparency to weapons?
    bool         draw_icon;       ///< Show the icon?
    Uint8          sparkle;         ///< Sparkle color or 0 for off
    bool         show_stats;      ///< Display stats?
    SFP8_T         uoffvel;         ///< Moving texture speed (8.8 fixed point)
    SFP8_T         voffvel;          ///< Moving texture speed (8.8 fixed point)
    float          shadow_size_stt;  ///< Initial shadow size
    Uint32         shadow_size;      ///< Size of shadow
    Uint32         shadow_size_save; ///< Without size modifiers
    BBOARD_REF     ibillboard;       ///< The attached billboard

    // model info
    bool         is_overlay;                    ///< Is this an overlay? Track aitarget...
    SKIN_T         skin;                          ///< Character's skin
    PRO_REF        profile_ref;                      ///< Character's profile
    PRO_REF        basemodel_ref;                     ///< The true form
    Uint8          alpha_base;
    Uint8          light_base;
    chr_instance_t inst;                          ///< the render data

    // Skills
    int           darkvision_level;
    int           see_kurse_level;
    int           see_invisible_level;
    IDSZ_node_t   skills[MAX_IDSZ_MAP_SIZE];

    // collision info

    /// @note - to make it easier for things to "hit" one another (like a damage particle from
    ///        a torch hitting a grub bug), Aaron sometimes made the bumper size much different
    ///        than the shape of the actual object.
    ///        The old bumper data that is read from the data.txt file will be kept in
    ///        the struct "bump". A new bumper that actually matches the size of the object will
    ///        be kept in the struct "collision"
    bumper_t     bump_stt;
    bumper_t     bump;
    bumper_t     bump_save;

    bumper_t     bump_1;       ///< the loosest collision volume that mimics the current bump
    oct_bb_t     chr_max_cv;   ///< a looser collision volume for chr-prt interactions
    oct_bb_t     chr_min_cv;   ///< the tightest collision volume for chr-chr interactions

    oct_bb_t     slot_cv[SLOT_COUNT];  ///< the cv's for the object's slots

    Uint8        stoppedby;                     ///< Collision mask

    // character location data
    fvec3_t        pos_stt;                       ///< Starting position
    fvec3_t        pos;                           ///< Character's position
    fvec3_t        vel;                           ///< Character's velocity
    orientation_t  ori;                           ///< Character's orientation

    fvec3_t        pos_old;                       ///< Character's last position
    fvec3_t        vel_old;                       ///< Character's last velocity
    orientation_t  ori_old;                       ///< Character's last orientation

    Uint32         onwhichgrid;                   ///< Where the char is
    Uint32         onwhichblock;                  ///< The character's collision block
    CHR_REF        bumplist_next;                 ///< Next character on fanblock

    // movement properties
    bool         waterwalk;                     ///< Always above watersurfacelevel?
    turn_mode_t  turnmode;                      ///< Turning mode

    BIT_FIELD      movement_bits;                 ///< What movement modes are allowed?
    float          anim_speed_sneak;              ///< Movement rate of the sneak animation
    float          anim_speed_walk;               ///< Walking if above this speed
    float          anim_speed_run;                ///< Running if above this speed
    float          maxaccel;                      ///< The actual maximum acelleration
    float          maxaccel_reset;                ///< The current maxaccel_reset
    Uint8          flyheight;                     ///< Height to stabilize at

    // data for doing the physics in bump_all_objects()
    phys_data_t       phys;
    chr_environment_t enviro;

    int               dismount_timer;                ///< a timer BB added in to make mounts and dismounts not so unpredictable
    CHR_REF           dismount_object;               ///< the object that you were dismounting from

    bool         safe_valid;                    ///< is the last "safe" position valid?
    fvec3_t        safe_pos;                      ///< the last "safe" position
    Uint32         safe_time;                     ///< the last "safe" time
    Uint32         safe_grid;                     ///< the last "safe" grid

    breadcrumb_list_t crumbs;                     ///< a list of previous valid positions that the object has passed through

	static chr_t * ctor(chr_t * pchr);
};

chr_t *chr_dtor(chr_t * pchr);
bool  chr_request_terminate( chr_t * pchr );

bool    chr_matrix_valid( const chr_t * pchr );
egolib_rv chr_update_matrix( chr_t * pchr, bool update_size );

chr_t *   chr_update_hide( chr_t * pchr );
egolib_rv chr_update_collision_size( chr_t * pchr, bool update_matrix );
bool    chr_can_see_dark( const chr_t * pchr, const chr_t * pobj );
bool    chr_can_see_invis( const chr_t * pchr, const chr_t * pobj );
int       chr_get_price( const CHR_REF ichr );

bool     chr_heal_mad( chr_t * pchr );
MAD_REF  chr_get_imad( const CHR_REF ichr );
mad_t   *chr_get_pmad( const CHR_REF ichr );
TX_REF   chr_get_txtexture_icon_ref( const CHR_REF item );

bool chr_can_mount( const CHR_REF ichr_a, const CHR_REF ichr_b );

bool chr_is_over_water( chr_t *pchr );

Uint32 chr_get_framefx( chr_t * pchr );

egolib_rv chr_set_frame( const CHR_REF character, int action, int frame_along, int lip );

egolib_rv chr_set_action( chr_t * pchr, int action, bool action_ready, bool override_action );
egolib_rv chr_start_anim( chr_t * pchr, int action, bool action_ready, bool override_action );
egolib_rv chr_set_anim( chr_t * pchr, int action, int frame, bool action_ready, bool override_action );
egolib_rv chr_increment_action( chr_t * pchr );
egolib_rv chr_increment_frame( chr_t * pchr );
egolib_rv chr_play_action( chr_t * pchr, int action, bool action_ready );
bool chr_update_breadcrumb_raw( chr_t * pchr );
bool chr_update_breadcrumb( chr_t * pchr, bool force );
bool chr_update_safe_raw( chr_t * pchr );
bool chr_update_safe( chr_t * pchr, bool force );
bool chr_get_safe( chr_t * pchr, fvec3_base_t pos );

bool chr_set_pos(chr_t *self, const fvec3_t& position);
bool chr_set_pos(chr_t *self, const fvec3_base_t position); ///< @todo Remove this.

bool chr_set_maxaccel( chr_t * pchr, float new_val );
bool character_is_attacking( chr_t * pchr );

void chr_set_floor_level( chr_t * pchr, const float level );
void chr_set_redshift( chr_t * pchr, const int rs );
void chr_set_grnshift( chr_t * pchr, const int gs );
void chr_set_blushift( chr_t * pchr, const int bs );
void chr_set_sheen( chr_t * pchr, const int sheen );
void chr_set_alpha( chr_t * pchr, const int alpha );
void chr_set_light( chr_t * pchr, const int light );

void chr_set_fat(chr_t *chr, const float fat);
void chr_set_height(chr_t *chr, const float height);
void chr_set_width(chr_t *chr, const float width);
void chr_set_size(chr_t *chr, const float size);
void chr_set_shadow(chr_t *chr, const float width);

/// @details Make sure the value it calculated relative to a valid matrix.
bool chr_getMatUp(chr_t *self, fvec3_t& up);

/// @details Make sure the value it calculated relative to a valid matrix.
bool chr_getMatRight(chr_t *self, fvec3_t& right);

/// @details Make sure the value it calculated relative to a valid matrix.
bool chr_getMatForward(chr_t *self, fvec3_t& forward);

/// @details Make sure the value it calculated relative to a valid matrix.
bool chr_getMatTranslate(chr_t *self, fvec3_t& translate);

const char * chr_get_name( const CHR_REF ichr, const BIT_FIELD bits, char * buffer, size_t buffer_size );
const char * chr_get_dir_name( const CHR_REF ichr );
int chr_get_skill( chr_t * pchr, IDSZ whichskill );

void reset_character_alpha( const CHR_REF character );
void reset_character_accel( const CHR_REF character );

// this function is needed because the "hidden" state of an ai is determined by
// whether  ai.state == cap.hidestate
chr_t * chr_set_ai_state( chr_t * pchr, int state );

//--------------------------------------------------------------------------------------------
// list definitions
//--------------------------------------------------------------------------------------------

extern Stack<team_t, TEAM_MAX> TeamStack;

#define VALID_TEAM_RANGE( ITEAM ) ( ((ITEAM) >= 0) && ((ITEAM) < TEAM_MAX) )


#define IS_ATTACHED_CHR_RAW(ICHR) ( (DEFINED_CHR(ChrList.lst[ICHR].attachedto) || DEFINED_CHR(ChrList.lst[ICHR].inwhich_inventory)) )
#define IS_ATTACHED_CHR(ICHR) LAMBDA( !DEFINED_CHR(ICHR), false, IS_ATTACHED_CHR_RAW(ICHR) )

// counters for debugging wall collisions
extern int chr_stoppedby_tests;
extern int chr_pressure_tests;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Function prototypes

// character_system functions
void character_system_begin();
void character_system_end();

void update_all_character_matrices();

void reset_teams();
void update_all_characters();
void move_all_characters();
void cleanup_all_characters();
void bump_all_characters_update_counters();
void free_all_chraracters();
void free_one_character_in_game( const CHR_REF character );

void keep_weapons_with_holders();

void make_one_character_matrix( const CHR_REF cnt );
void move_one_character_get_environment( chr_t * pchr );

BIT_FIELD chr_hit_wall( chr_t * pchr, const float test_pos[], float nrm[], float * pressure, mesh_wall_data_t * pdata );
BIT_FIELD chr_test_wall( chr_t * pchr, const float test_pos[], mesh_wall_data_t * pdata );

/**
 * @brief
 *	Spawn a character.
 * @return
 *	the index of the character on success success, #MAX_CHR on failure
 */
CHR_REF spawn_one_character( const fvec3_t& pos, const PRO_REF profile, const TEAM_REF team, const int skin, const FACING_T facing, const char *name, const CHR_REF override );
void    respawn_character( const CHR_REF character );

// inventory functions
bool inventory_remove_item( const CHR_REF ichr, const size_t inventory_slot, const bool ignorekurse );
bool inventory_add_item( const CHR_REF ichr, const CHR_REF item, Uint8 inventory_slot, const bool ignorekurse );
bool inventory_swap_item( const CHR_REF ichr, Uint8 inventory_slot, const slot_t grip_off, const bool ignorekurse );

// save character functions
bool  export_one_character_quest_vfs( const char *szSaveName, const CHR_REF character );
bool  export_one_character_name_vfs( const char *szSaveName, const CHR_REF character );

void character_swipe( const CHR_REF cnt, slot_t slot );

bool chr_teleport( const CHR_REF ichr, float x, float y, float z, FACING_T facing_z );

CHR_REF chr_has_inventory_idsz( const CHR_REF ichr, IDSZ idsz, bool equipped );
CHR_REF chr_holding_idsz( const CHR_REF ichr, IDSZ idsz );
CHR_REF chr_has_item_idsz( const CHR_REF ichr, IDSZ idsz, bool equipped );

bool chr_copy_enviro( chr_t * chr_psrc, chr_t * chr_pdst );

bool chr_calc_grip_cv( chr_t * pmount, int grip_offset, oct_bb_t * grip_cv_ptr, fvec3_base_t grip_origin_vec, fvec3_base_t grip_up_vec, const bool shift_origin );

// character state machine functions
chr_t * chr_run_config( chr_t * pchr );
chr_t * chr_config_construct( chr_t * pchr, int max_iterations );
chr_t * chr_config_initialize( chr_t * pchr, int max_iterations );
chr_t * chr_config_activate( chr_t * pchr, int max_iterations );
chr_t * chr_config_deinitialize( chr_t * pchr, int max_iterations );
chr_t * chr_config_deconstruct( chr_t * pchr, int max_iterations );

bool  chr_can_see_object( const chr_t * pchr, const chr_t * pobj );
CHR_REF chr_get_lowest_attachment( const CHR_REF ichr, bool non_item );

void drop_money( const CHR_REF character, int money );
void call_for_help( const CHR_REF character );
void give_experience( const CHR_REF character, int amount, XPType xptype, bool override_invictus );
void give_team_experience( const TEAM_REF team, int amount, XPType xptype );
int  damage_character( const CHR_REF character, const FACING_T direction,
                       const IPair damage, const Uint8 damagetype, const TEAM_REF team,
                       const CHR_REF attacker, const BIT_FIELD effects, const bool ignore_invictus );
void kill_character( const CHR_REF character, const CHR_REF killer, bool ignore_invictus );
/**
 * @brief
 *	This function gives some purelife points to the target, ignoring any resistances and so forth.
 * @param character
 *	the character
 * @param healer
 *	the healer
 * @param amount
 *	the amount to heal the character
 */
bool heal_character( const CHR_REF character, const CHR_REF healer, UFP8_T amount, bool ignore_invictus );
void spawn_poof( const CHR_REF character, const PRO_REF profile );
void spawn_defense_ping( chr_t *pchr, const CHR_REF attacker );

bool detach_character_from_mount( const CHR_REF character, Uint8 ignorekurse, Uint8 doshop );

egolib_rv flash_character_height( const CHR_REF character, Uint8 valuelow, Sint16 low, Uint8 valuehigh, Sint16 high );

void free_inventory_in_game( const CHR_REF character );
void do_level_up( const CHR_REF character );
bool setup_xp_table( const CHR_REF character );

int     change_armor( const CHR_REF character, const SKIN_T skin );
void    change_character( const CHR_REF cnt, const PRO_REF profile, const int skin, const Uint8 leavewhich );
void    change_character_full( const CHR_REF ichr, const PRO_REF profile, const int skin, const Uint8 leavewhich );
bool  cost_mana( const CHR_REF character, int amount, const CHR_REF killer );
void    switch_team( const CHR_REF character, const TEAM_REF team );
void    issue_clean( const CHR_REF character );
int     restock_ammo( const CHR_REF character, IDSZ idsz );
egolib_rv attach_character_to_mount( const CHR_REF character, const CHR_REF mount, grip_offset_t grip_off );

void  drop_keys( const CHR_REF character );
bool  drop_all_items( const CHR_REF character );
bool  character_grab_stuff( const CHR_REF chara, grip_offset_t grip, bool people );

//--------------------------------------------------------------------------------------------
// generic helper functions

bool is_invictus_direction( FACING_T direction, const CHR_REF character, BIT_FIELD effects );
void init_slot_idsz();

grip_offset_t slot_to_grip_offset( slot_t slot );
slot_t        grip_offset_to_slot( grip_offset_t grip );

const char * describe_value( float value, float maxval, int * rank_ptr );
const char* describe_damage( float value, float maxval, int * rank_ptr );
const char* describe_wounds( float max, float current );

billboard_data_t * chr_make_text_billboard( const CHR_REF ichr, const char * txt, const SDL_Color text_color, const GLXvector4f tint, int lifetime_secs, const BIT_FIELD opt_bits );


//--------------------------------------------------------------------------------------------
// PREVIOUSLY INLINE FUNCTIONS
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// team_t accessor functions
CHR_REF team_get_ileader( const TEAM_REF iteam );
chr_t  *team_get_pleader( const TEAM_REF iteam );

bool team_hates_team( const TEAM_REF ipredator_team, const TEAM_REF iprey_team );

//--------------------------------------------------------------------------------------------
// chr_t accessor functions
PRO_REF  chr_get_ipro( const CHR_REF ichr );
CAP_REF  chr_get_icap( const CHR_REF ichr );
TEAM_REF chr_get_iteam( const CHR_REF ichr );
TEAM_REF chr_get_iteam_base( const CHR_REF ichr );

ObjectProfile *chr_get_ppro( const CHR_REF ichr );

team_t         *chr_get_pteam( const CHR_REF ichr );
team_t         *chr_get_pteam_base( const CHR_REF ichr );
ai_state_t     *chr_get_pai( const CHR_REF ichr );
chr_instance_t *chr_get_pinstance( const CHR_REF ichr );

IDSZ chr_get_idsz( const CHR_REF ichr, int type );

void chr_update_size( chr_t * pchr );


bool chr_has_idsz( const CHR_REF ichr, IDSZ idsz );
bool chr_is_type_idsz( const CHR_REF ichr, IDSZ idsz );
bool chr_has_vulnie( const CHR_REF item, const PRO_REF weapon_profile );

const fvec3_t& chr_get_pos_v_const(const chr_t *pchr);
bool chr_get_pos(const chr_t *self, fvec3_t& position);
bool chr_get_pos(const chr_t *self, fvec3_base_t position); ///< @todo Remove this.
