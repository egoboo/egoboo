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
///       object of team_t or GameObject. In *.inl files or *.c/*.cpp files you will routinely include "char.inl", instead.

#pragma once

#include "game/egoboo_typedef.h"
#include "game/graphic.h"
#include "game/graphic_mad.h"
#include "game/script.h"
#include "game/physics.h"
#include "game/egoboo.h"

#include "game/graphics/MD2Model.hpp"
#include "game/profiles/Profile.hpp"
#include "game/entities/GameObject.hpp"
#include "game/enchant.h"
#include "game/particle.h"

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

struct mad_t;
struct eve_t;
struct s_pip;
struct billboard_data_t;
struct mesh_wall_data_t;

struct prt_t;

//--------------------------------------------------------------------------------------------
// internal structs
//--------------------------------------------------------------------------------------------
struct team_t;
struct chr_environment_t;
struct chr_spawn_data_t;


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

#define CHR_INFINITE_WEIGHT          (static_cast<uint32_t>(0xFFFFFFFF))
#define CHR_MAX_WEIGHT               (static_cast<uint32_t>(0xFFFFFFFE))

#define GRABSIZE            90.0f                   ///< Grab tolerance
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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


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

bool    chr_matrix_valid( const GameObject * pchr );
egolib_rv chr_update_matrix( GameObject * pchr, bool update_size );

GameObject *   chr_update_hide( GameObject * pchr );
egolib_rv chr_update_collision_size( GameObject * pchr, bool update_matrix );
bool    chr_can_see_dark( const GameObject * pchr, const GameObject * pobj );
bool    chr_can_see_invis( const GameObject * pchr, const GameObject * pobj );
int       chr_get_price( const CHR_REF ichr );

bool     chr_heal_mad( GameObject * pchr );
MAD_REF  chr_get_imad( const CHR_REF ichr );
mad_t   *chr_get_pmad( const CHR_REF ichr );
TX_REF   chr_get_txtexture_icon_ref( const CHR_REF item );

Uint32 chr_get_framefx( GameObject * pchr );

egolib_rv chr_set_frame( const CHR_REF character, int action, int frame_along, int lip );

egolib_rv chr_set_action( GameObject * pchr, int action, bool action_ready, bool override_action );
egolib_rv chr_start_anim( GameObject * pchr, int action, bool action_ready, bool override_action );
egolib_rv chr_set_anim( GameObject * pchr, int action, int frame, bool action_ready, bool override_action );
egolib_rv chr_increment_action( GameObject * pchr );
egolib_rv chr_increment_frame( GameObject * pchr );
egolib_rv chr_play_action( GameObject * pchr, int action, bool action_ready );
bool chr_update_breadcrumb_raw( GameObject * pchr );
bool chr_update_breadcrumb( GameObject * pchr, bool force );
bool chr_update_safe_raw( GameObject * pchr );
bool chr_update_safe( GameObject * pchr, bool force );
bool chr_get_safe( GameObject * pchr);

bool chr_set_maxaccel( GameObject * pchr, float new_val );

void chr_set_floor_level( GameObject * pchr, const float level );
void chr_set_redshift( GameObject * pchr, const int rs );
void chr_set_grnshift( GameObject * pchr, const int gs );
void chr_set_blushift( GameObject * pchr, const int bs );

void chr_set_fat(GameObject *chr, const float fat);
void chr_set_height(GameObject *chr, const float height);
void chr_set_width(GameObject *chr, const float width);
void chr_set_size(GameObject *chr, const float size);
void chr_set_shadow(GameObject *chr, const float width);

/// @details Make sure the value it calculated relative to a valid matrix.
bool chr_getMatUp(GameObject *self, fvec3_t& up);

/// @details Make sure the value it calculated relative to a valid matrix.
bool chr_getMatRight(GameObject *self, fvec3_t& right);

/// @details Make sure the value it calculated relative to a valid matrix.
bool chr_getMatForward(GameObject *self, fvec3_t& forward);

/// @details Make sure the value it calculated relative to a valid matrix.
bool chr_getMatTranslate(GameObject *self, fvec3_t& translate);

const char * chr_get_dir_name( const CHR_REF ichr );
int chr_get_skill( GameObject * pchr, IDSZ whichskill );

bool update_chr_darkvision( const CHR_REF character );

void reset_character_alpha( const CHR_REF character );
void reset_character_accel( const CHR_REF character );

// this function is needed because the "hidden" state of an ai is determined by
// whether  ai.state == cap.hidestate
GameObject * chr_set_ai_state( GameObject * pchr, int state );

//--------------------------------------------------------------------------------------------
// list definitions
//--------------------------------------------------------------------------------------------

extern Stack<team_t, TEAM_MAX> TeamStack;

#define VALID_TEAM_RANGE( ITEAM ) ( ((ITEAM) >= 0) && ((ITEAM) < TEAM_MAX) )


#define IS_ATTACHED_CHR_RAW(ICHR) ( (_gameObjects.exists(_gameObjects.get(ICHR)->attachedto) || _gameObjects.exists(_gameObjects.get(ICHR)->inwhich_inventory)) )
#define IS_ATTACHED_CHR(ICHR) LAMBDA( !_gameObjects.exists(ICHR), false, IS_ATTACHED_CHR_RAW(ICHR) )

// counters for debugging wall collisions
extern int chr_stoppedby_tests;
extern int chr_pressure_tests;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Function prototypes

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
void move_one_character_get_environment( GameObject * pchr );

BIT_FIELD chr_hit_wall( GameObject * pchr, const float test_pos[], float nrm[], float * pressure, mesh_wall_data_t * pdata );
BIT_FIELD GameObjectest_wall( GameObject * pchr, const float test_pos[], mesh_wall_data_t * pdata );

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

CHR_REF chr_has_inventory_idsz( const CHR_REF ichr, IDSZ idsz, bool equipped );
CHR_REF chr_holding_idsz( const CHR_REF ichr, IDSZ idsz );
CHR_REF chr_has_item_idsz( const CHR_REF ichr, IDSZ idsz, bool equipped );

bool chr_copy_enviro( GameObject * chr_psrc, GameObject * chr_pdst );

bool chr_calc_grip_cv( GameObject * pmount, int grip_offset, oct_bb_t * grip_cv_ptr, const bool shift_origin );

// character state machine functions
GameObject * chr_config_do_init( GameObject * pchr );

bool  chr_can_see_object( const GameObject * pchr, const GameObject * pobj );
CHR_REF chr_get_lowest_attachment( const CHR_REF ichr, bool non_item );

void drop_money( const CHR_REF character, int money );
void call_for_help( const CHR_REF character );
void give_experience( const CHR_REF character, int amount, XPType xptype, bool override_invictus );
void give_team_experience( const TEAM_REF team, int amount, XPType xptype );
void kill_character( const CHR_REF character, const CHR_REF killer, bool ignore_invictus );
void spawn_poof( const CHR_REF character, const PRO_REF profile );
void spawn_defense_ping( GameObject *pchr, const CHR_REF attacker );

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
GameObject  *team_get_pleader( const TEAM_REF iteam );

bool team_hates_team( const TEAM_REF ipredator_team, const TEAM_REF iprey_team );

//--------------------------------------------------------------------------------------------
// GameObject accessor functions
PRO_REF  chr_get_ipro( const CHR_REF ichr );
TEAM_REF chr_get_iteam( const CHR_REF ichr );
TEAM_REF chr_get_iteam_base( const CHR_REF ichr );

ObjectProfile *chr_get_ppro( const CHR_REF ichr );

team_t         *chr_get_pteam( const CHR_REF ichr );
team_t         *chr_get_pteam_base( const CHR_REF ichr );
ai_state_t     *chr_get_pai( const CHR_REF ichr );
chr_instance_t *chr_get_pinstance( const CHR_REF ichr );

IDSZ chr_get_idsz( const CHR_REF ichr, int type );

void chr_update_size( GameObject * pchr );


bool chr_has_idsz( const CHR_REF ichr, IDSZ idsz );
bool chr_is_type_idsz( const CHR_REF ichr, IDSZ idsz );
bool chr_has_vulnie( const CHR_REF item, const PRO_REF weapon_profile );

