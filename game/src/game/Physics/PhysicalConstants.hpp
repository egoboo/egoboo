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

#pragma once

namespace Ego
{
namespace Physics
{

/// @brief The default gravity    amount and direction.
/// @todo Should be a vector.
static constexpr float gravity = -1.0f;

/// @brief The default windspeed  amount and direction.
static constexpr fvec3_windspeed = Vector3f::ADDITIVE_NEUTRAL; 

/// @brief The default waterspeed amount and direction.
static constexpr fvec3_waterspeed = Vector3f::ADDITIVE_NEUTRAL;

/// @brief The default friction of   air.
static constexpr float airfriction = 0.91f;

/// @brief The default friction of water.
static constexpr float waterfriction = 0.80f;

/// @brief The default friction on slippy ground.
///        i.e. tiles for which the MAPFX_SLIPPY bit is
///        set.
static constexpr float slippyGroundFriction = 1.00f;

/// @brief The default friction on normal ground.
///        i.e. tiles for which the MAPFX_SLIPPY bit is
///        NOT
///        set.
static constexpr float normalGroundFriction = 0.91f;

} //namespace Physics
} //namespace Ego
