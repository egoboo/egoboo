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

/// @file  game/PhysicalConstants.cpp
/// @brief invariants & defaults of the game's physics system

#include "game/PhysicalConstants.hpp"

const float   PhysicalConstants::gravity              = -1.0f;

const fvec3_t PhysicalConstants::windspeed            = fvec3_t::ADDITIVE_NEUTRAL;

const fvec3_t PhysicalConstants::waterspeed           = fvec3_t::ADDITIVE_NEUTRAL;

const float   PhysicalConstants::airfriction          = 0.91f;

const float   PhysicalConstants::waterfriction        = 0.80f;

const float   PhysicalConstants::slippyGroundFriction = 1.00f;

const float   PhysicalConstants::normalGroundFriction = 0.91f;