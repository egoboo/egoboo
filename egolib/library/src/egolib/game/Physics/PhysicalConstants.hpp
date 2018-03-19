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

/// @file egolib/game/PhysicalConstants.cpp
/// @brief invariants & defaults of the game's physics system

#pragma once

namespace Ego::Physics
{

/// @brief The default gravity    amount and direction.
/// @todo Should be a vector.
static constexpr float DEFAULT_GRAVITY = -1.0f;

/// @brief The default friction of   air.
static constexpr float DEFAULT_AIR_FRICTION = 0.9868f;

/// @brief The default friction of water.
static constexpr float DEFAULT_WATER_FRICTION = 0.80f;

/// @brief The default friction on slippy ground.
///        i.e. tiles for which the MAPFX_SLIPPY bit is
///        set.
static constexpr float DEFAULT_SLIPPY_GROUND_FRICTION = 1.00f;

/// @brief The default friction on normal ground.
///        i.e. tiles for which the MAPFX_SLIPPY bit is
///        NOT
///        set.
static constexpr float DEFAULT_GROUND_FRICTION = 0.91f;

static constexpr float DEFAULT_ICE_FRICTION = 0.8f;

static constexpr float STOP_BOUNCING = 8.00f;        ///< To make objects stop bouncing

static constexpr float MOUNTTOLERANCE = 20;          ///< Threshold for mounting objects

static constexpr uint32_t CHR_INFINITE_WEIGHT = std::numeric_limits<uint32_t>::max();
static constexpr uint32_t CHR_MAX_WEIGHT = CHR_INFINITE_WEIGHT - 1;


struct Environment
{

    /**
     * @brief
     *  Extra downhill force.
     * @default
     *  1.0f
     * @todo
     *  Short description of (reasonable) limits and effect.
     */
    float hillslide;

    /**
     * @brief
     *  Friction on tiles that are marked with MAPFX_SLIPPY.
     * @default
     *  1.0f
     * @todo
     *  Short description of (reasonable) limits and effect.
     */
    float slippyfriction;

    /**
     * @brief
     *  Air friction.
     * @default
     *  0.9868f
     * @remark
     *  0.9868f is approximately real world air friction.
     * @todo
     *  Short description of (reasonable) limits and effect.
     */
    float airfriction;

    /**
     * @brief
     *  Ice friction.
     * @default
     *  0.9738f (square of air friction)
     * @todo
     *  Short description of (reasonable) limits and effect.
     */
    float icefriction;

    /**
     * @brief
     *  Water friction.  
     * @default
     *  0.8f
     * @todo     
     *  Short description of (reasonable) limits and effect.
     */
    float waterfriction;

    /**
     * @brief
     *  Friction on tiles that are not marked with MAPFX_SLIPPY.
     * @default
     *  0.91f
     * @todo
     *  Short description of (reasonable) limits and effect.
     */
    float noslipfriction;

    /**
     * @brief
     *  Gravitational force.
     * @default
     *  -1.0f
     * @todo
     *  Short description of (reasonable) limits and effect.
     */
    float gravity;

    /**
     * @brief
     *  The game's windspeed.
     * @default
     *  <tt>(0,0,0)</tt>
     * @todo
     *  Short description of (reasonable) limits and effect.
     */
    Vector3f windspeed;

    /**
     * @brief
     *  The game's waterspeed.
     * @default
     *  <tt>(0,0,0)</tt>
     * @todo
     *  Short description of (reasonable) limits and effect.
     */
    Vector3f waterspeed;

    /**
     * @brief
     *  Construct this environment with its default values.
     */
    Environment() :
        hillslide(DEFAULT_SLIPPY_GROUND_FRICTION),
        slippyfriction(DEFAULT_SLIPPY_GROUND_FRICTION),
        airfriction(DEFAULT_AIR_FRICTION),
        icefriction(DEFAULT_ICE_FRICTION),
        waterfriction(DEFAULT_WATER_FRICTION),
        noslipfriction(DEFAULT_GROUND_FRICTION),
        gravity(DEFAULT_GRAVITY),
        windspeed(0.0f, 0.0f, 0.0f),
        waterspeed(0.0f, 0.0f, 0.0f)
    {
        //ctor
    }

};

extern Environment g_environment;

} //namespace Ego::Physics
