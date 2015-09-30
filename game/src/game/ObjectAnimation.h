#pragma once

#include "game/Entities/_Include.hpp"

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


egolib_rv chr_set_action( Object * pchr, int action, bool action_ready, bool override_action );
egolib_rv chr_start_anim( Object * pchr, int action, bool action_ready, bool override_action );
egolib_rv chr_set_anim( Object * pchr, int action, int frame, bool action_ready, bool override_action );
egolib_rv chr_increment_action( Object * pchr );
egolib_rv chr_increment_frame( Object * pchr );
egolib_rv chr_play_action( Object * pchr, int action, bool action_ready );
bool chr_calc_grip_cv( Object * pmount, int grip_offset, oct_bb_t * grip_cv_ptr, const bool shift_origin );

void move_one_character_do_animation( Object * pchr );
