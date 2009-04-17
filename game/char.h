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
                         Uint8 skin, Uint16 facing,  const char *name, int override );

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

void export_one_character_name(  const char *szSaveName, Uint16 character );
void export_one_character_profile(  const char *szSaveName, Uint16 character );
void export_one_character_skin(  const char *szSaveName, Uint16 character );
int load_one_character_profile(  const char *szLoadName );

void chop_load( Uint16 profile,  const char *szLoadname );
void character_swipe( Uint16 cnt, Uint8 slot );

int check_skills( Uint16 who, IDSZ whichskill );

//---------------------------------------------------------------------------------------------
// Quest system
bool_t add_quest_idsz(  const char *whichplayer, IDSZ idsz );
Sint16 modify_quest_idsz(  const char *whichplayer, IDSZ idsz, Sint16 adjustment );
Sint16 check_player_quest(  const char *whichplayer, IDSZ idsz );
