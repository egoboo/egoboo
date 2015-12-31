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
//--------------------------------------------------------------------------------------------
#define MAP_TURN_OFFSET 0x8000

#define MAXMONEY        9999                        ///< Maximum money

#define CHR_INFINITE_WEIGHT          (static_cast<uint32_t>(0xFFFFFFFF))
#define CHR_MAX_WEIGHT               (static_cast<uint32_t>(0xFFFFFFFE))

#define SEEINVISIBLE        128                        ///< Cutoff for invisible characters

#define RAISE               12                  ///< Helps correct z level

#define PITDEPTH            -60                     ///< Depth to kill character

//Dismounting
#define DISMOUNTZVEL        12
#define DISMOUNTZVELFLY     4
#define PHYS_DISMOUNT_TIME  50          ///< time delay for full object-object interaction (approximately 1 second)

/// Inventory
#define PACKDELAY           25                      ///< Time before inventory rotate again

/// Z velocity
#define JUMPDELAY           20                      ///< Time between jumps
#define JUMPINFINITE        255                     ///< Flying character
#define DROPZVEL            7
#define DROPXYVEL           12

//Timer resets
#define DAMAGETIME          32                            ///< Invincibility time
#define BORETIME            (Random::next<uint16_t>(255, 255 + 511)) ///< IfBored timer
