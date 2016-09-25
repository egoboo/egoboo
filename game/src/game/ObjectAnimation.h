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

