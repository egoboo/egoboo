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

/// @file egoboo_config.h
/// @brief System-dependent global parameters.
///   @todo  some of this stuff is compiler dependent, rather than system dependent.

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// compliation flags

// object pre-allocations
#define MAX_CHR             512             ///< Maximum number of characters
#define MAX_ENC             200             ///< Maximum number of enchantments
#define MAX_PRT             2048             ///< Maximum number of particles
#define TOTAL_MAX_DYNA      64             ///< Maximum number of dynamic lights

#define MAX_TEXTURE        (MAX_CHR * 4)     ///< Maximum number of textures
#define MAX_ICON           (MAX_TEXTURE + 4) ///< Maximum number of icons

#define MAX_SKIN             4               ///< The maxumum number of skins per model. This must remain hard coded at 4 for the moment.

/// profile pre-allocations
#define MAX_PROFILE        256          ///< Maximum number of object profiles
#define MAX_AI             MAX_PROFILE  ///< Maximum number of scripts

/// per-object pre-allocations
#define MAX_WAVE             30        ///< Maximum number of *.wav/*.ogg per object
#define MAX_PIP_PER_PROFILE  13        ///< Maximum number of part*.txt per object
#define MAX_PIP             (MAX_PROFILE * MAX_PIP_PER_PROFILE)

// Some macro switches
#undef  OLD_CAMERA_MODE       ///< Use the old camera style
#undef  ENABLE_BODY_GRAB      ///< Enable the grabbing of bodies?
#undef  TEST_NAN_RESULT       ///< Test the result of certain math operations

#undef  USE_LUA_CONSOLE       ///< LUA support for the console

#undef  RENDER_HMAP           ///< render the mesh's heightmap?
#undef  DEBUG_MESH_NORMALS    ///< render the mesh normals
#define LOG_TO_CONSOLE        ///< dump all log info to file and to the console. Only useful if your compiler generates console for program output. Otherwise the results will end up in a file called stdout.txt

#define  DEBUG_BSP             ///< Print debugging info about the BSP/octree state

#define  DEBUG_RENDERLIST      ///< Print debugging info for the currently rendered mesh

#undef  DEBUG_PROFILE         ///< Switch the profiling functions on and off
#undef  DEBUG_PROFILE_DISPLAY ///< Display the results for the performance profiling
#undef  DEBUG_PROFILE_RENDER  ///< Display the results for the performance profiling of the generric rendering
#undef  DEBUG_PROFILE_MESH    ///< Display the results for the performance profiling of the mesh rendering sub-system
#undef  DEBUG_PROFILE_INIT    ///< Display the results for the performance profiling of the rendering initialization

#undef  DEBUG_OBJECT_SPAWN    ///< Log debug info for every object spawned

#undef   DEBUG_PRT_LIST      ///< Track every single deletion from the PrtList to make sure the same element is not deleted twice. Prevents corruption of the PrtList.free_lst
#undef   DEBUG_ENC_LIST      ///< Track every single deletion from the EncList to make sure the same element is not deleted twice. Prevents corruption of the EncList.free_lst
#undef   DEBUG_CHR_LIST      ///< Track every single deletion from the ChrList to make sure the same element is not deleted twice. Prevents corruption of the ChrList.free_lst

#define CLIP_LIGHT_FANS      ///< is the light_fans() function going to be throttled?
#undef  CLIP_ALL_LIGHT_FANS  ///< a switch for selecting how the fans will be updated

#undef  DEBUG_WAYPOINTS      ///< display error messages when adding waypoints. It will also prevent "unsafe" waypoint being added to the waypoint list.

/// How much script debugging.
///    0 -- debugging off ( requires defined(_DEBUG) )
/// >= 1 -- Log the amount of script time that every object uses (requires defined(_DEBUG) and DEBUG_PROFILE)
/// >= 2 -- Log the amount of time that every single script command uses (requires defined(_DEBUG) and DEBUG_PROFILE)
/// >= 3 -- decompile every script (requires defined(_DEBUG))
#define DEBUG_SCRIPT_LEVEL 0

//#undef DRAW_CHR_BBOX        ///< display selected character bounding boxes
#define DRAW_PRT_BBOX        ///< display selected particle bounding boxes

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// do the includes last so that the compile switches are always set
#include "egoboo_platform.h"
#include "egoboo_endian.h"

#define EGOBOO_CONFIG_H
