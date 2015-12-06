#pragma once

#include "game/egoboo_typedef.h"

//Function prototypes
void move_one_character_get_environment( Object * pchr );

egolib_rv chr_update_collision_size( Object * pchr, bool update_matrix );
