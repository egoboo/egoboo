#pragma once

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

/// @file
/// @brief System-dependent global parameters.
///   @todo  move more of the typical config stuff to this file.
///   @todo  add in linux and mac stuff.
///   @todo  some of this stuff is compiler dependent, rather than system dependent.

#include "egoboo_endian.h"
#include "egoboo_platform.h"

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// compliation flags

// object pre-allocations
#define MAX_CHR            512             // Maximum number of characters
#define MAX_ENC            200             // Maximum number of enchantments
#define TOTAL_MAX_PRT     2048             // Maximum number of particles
#define TOTAL_MAX_DYNA      64             // Maximum number of dynamic lights

#define MAX_TEXTURE        (MAX_CHR * 4)     // Maximum number of textures
#define MAX_ICON           (MAX_TEXTURE + 4) // Maximum number of icons

// profile pre-allocations
#define MAX_PROFILE        256        // Maximum number of object profiles
#define MAX_AI             129        // Maximum number of scripts

// per-object pre-allocations
#define MAX_WAVE             30        // Maximum number of *.wav/*.ogg per object
#define MAX_PIP_PER_PROFILE  13        // Maximum number of part*.txt per object
#define MAX_PIP             (MAX_PROFILE * MAX_PIP_PER_PROFILE)

// Some macro switches
#define USE_DEBUG (defined(_DEBUG) || !defined(NDEBUG))

#undef  OLD_CAMERA_MODE       // Use the old camera style
#undef  USE_LUA_CONSOLE       // LUA support for the console
#undef  ENABLE_BODY_GRAB      // Enable the grabbing of bodies?
#undef  TEST_NAN_RESULT       // Test the result of certain math operations?
#undef  DRAW_XP_BARS          // Draws XP bars in the status displays (buggy)

#undef  RENDER_HMAP           // render the mesh's heightmap?
#undef  DEBUG_MESH_NORMALS    // render the mesh normals
#define LOG_TO_CONSOLE
#undef DEBUG_PROFILE

#define EGOBOO_CONFIG
