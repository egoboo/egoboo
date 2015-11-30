#pragma once

#include "game/egoboo_typedef.h"

//Function prototypes
void move_one_character_get_environment( Object * pchr );
bool character_grab_stuff( ObjectRef chara, grip_offset_t grip, bool people );
float get_chr_mass(Object * pchr);

egolib_rv chr_update_collision_size( Object * pchr, bool update_matrix );

egolib_rv attach_character_to_mount( ObjectRef riderRef, ObjectRef mountRef, grip_offset_t grip_off );
