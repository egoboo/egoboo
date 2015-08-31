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
///       Object.

#pragma once

#include "game/egoboo_typedef.h"
#include "game/graphic.h"
#include "game/graphic_mad.h"
#include "egolib/Script/script.h"
#include "egolib/Graphics/MD2Model.hpp"
#include "egolib/Logic/Team.hpp"
#include "game/physics.h"
#include "game/egoboo.h"
#include "game/Entities/_Include.hpp"
#include "game/CharacterMatrix.h"
#include "game/ObjectAnimation.h"

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

struct Billboard;
struct mesh_wall_data_t;

//--------------------------------------------------------------------------------------------
// internal structs
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

#define TURNSPD             0.01f                 ///< Cutoff for turning or same direction
#define SPINRATE            200                   ///< How fast spinners spin
#define WATCHMIN            0.01f                 ///< Tolerance for TURNMODE_WATCH

#define PITDEPTH            -60                     ///< Depth to kill character
#define HURTDAMAGE           256                    ///< Minimum damage for hurt animation

//Dismounting
#define DISMOUNTZVEL        16
#define DISMOUNTZVELFLY     4
#define PHYS_DISMOUNT_TIME  (TICKS_PER_SEC*0.05f)          ///< time delay for full object-object interaction (approximately 0.05 second)

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
#define BORETIME            (Random::next<uint16_t>(255, 255 + 511)) ///< IfBored timer
#define CAREFULTIME         50                            ///< Friendly fire timer
#define SIZETIME            100                           ///< Time it takes to resize a character

//--------------------------------------------------------------------------------------------

Object *   chr_update_hide( Object * pchr );
egolib_rv chr_update_collision_size( Object * pchr, bool update_matrix );

const oglx_texture_t* chr_get_txtexture_icon_ref( const CHR_REF item );

egolib_rv chr_set_frame( const CHR_REF character, int action, int frame_along, int lip );

bool chr_update_safe_raw( Object * pchr );
bool chr_update_safe( Object * pchr, bool force );
bool chr_get_safe( Object * pchr);

void chr_set_floor_level( Object * pchr, const float level );

std::string chr_get_dir_name( const CHR_REF ichr );
bool chr_get_skill( Object * pchr, IDSZ whichskill );


// counters for debugging wall collisions
extern int chr_stoppedby_tests;
extern int chr_pressure_tests;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Function prototypes
void update_all_characters();

/// @details This function drops all keys ( [KEYA] to [KEYZ] ) that are in a character's
///    inventory ( Not hands ).
void  drop_keys(const CHR_REF character);

// save character functions
bool  export_one_character_quest_vfs( const char *szSaveName, const CHR_REF character );
bool  export_one_character_name_vfs( const char *szSaveName, const CHR_REF character );

void character_swipe( const CHR_REF cnt, slot_t slot );

bool chr_calc_grip_cv( Object * pmount, int grip_offset, oct_bb_t * grip_cv_ptr, const bool shift_origin );

CHR_REF chr_get_lowest_attachment( const CHR_REF ichr, bool non_item );

void drop_money( const CHR_REF character, int money );
void spawn_poof( const CHR_REF character, const PRO_REF profile );
void spawn_defense_ping( Object *pchr, const CHR_REF attacker );

void    switch_team( const CHR_REF character, const TEAM_REF team );
egolib_rv attach_character_to_mount( const CHR_REF character, const CHR_REF mount, grip_offset_t grip_off );


bool  drop_all_items( const CHR_REF character );

void chr_init_size( Object * pchr, const std::shared_ptr<ObjectProfile> &profile);

//--------------------------------------------------------------------------------------------
// generic helper functions
std::shared_ptr<Billboard> chr_make_text_billboard(const CHR_REF ichr, const char * txt, const Ego::Math::Colour4f& text_color, const Ego::Math::Colour4f& tint, int lifetime_secs, const BIT_FIELD opt_bits);


//--------------------------------------------------------------------------------------------
// PREVIOUSLY INLINE FUNCTIONS
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Object accessor functions
TEAM_REF chr_get_iteam( const CHR_REF ichr );
TEAM_REF chr_get_iteam_base( const CHR_REF ichr );

Team         *chr_get_pteam( const CHR_REF ichr );
Team         *chr_get_pteam_base( const CHR_REF ichr );
chr_instance_t *chr_get_pinstance( const CHR_REF ichr );

IDSZ chr_get_idsz( const CHR_REF ichr, int type );

bool chr_has_idsz( const CHR_REF ichr, IDSZ idsz );
bool chr_is_type_idsz( const CHR_REF ichr, IDSZ idsz );
bool chr_has_vulnie( const CHR_REF item, const PRO_REF weapon_profile );

