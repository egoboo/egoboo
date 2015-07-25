#pragma once

#include "game/Entities/_Include.hpp"

uint32_t chr_get_framefx( Object * pchr );

egolib_rv chr_set_action( Object * pchr, int action, bool action_ready, bool override_action );
egolib_rv chr_start_anim( Object * pchr, int action, bool action_ready, bool override_action );
egolib_rv chr_set_anim( Object * pchr, int action, int frame, bool action_ready, bool override_action );
egolib_rv chr_increment_action( Object * pchr );
egolib_rv chr_increment_frame( Object * pchr );
egolib_rv chr_play_action( Object * pchr, int action, bool action_ready );

void move_one_character_do_animation( Object * pchr );
