#pragma once

#include "game/Entities/_Include.hpp"

//Function prototypes
void move_one_character_get_environment( Object * pchr );
void move_all_characters();
bool character_grab_stuff( const CHR_REF chara, grip_offset_t grip, bool people );
bool detach_character_from_platform( Object * pchr );
float get_chr_mass(Object * pchr);

egolib_rv chr_update_collision_size( Object * pchr, bool update_matrix );