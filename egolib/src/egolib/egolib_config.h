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

/// @file  egolib/egolib_config.h
/// @brief compilation time switches

#pragma once

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// compliation flags

#undef  USE_LUA_CONSOLE       ///< LUA support for the console

/// How much script debugging.
///    0 -- debugging off ( requires defined(_DEBUG) )
/// >= 1 -- Log the amount of script time that every object uses (requires defined(_DEBUG) and DEBUG_PROFILE)
/// >= 2 -- Log the amount of time that every single script command uses (requires defined(_DEBUG) and DEBUG_PROFILE)
/// >= 3 -- decompile every script (requires defined(_DEBUG))
#define DEBUG_SCRIPT_LEVEL 0

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/**
* @defgroup compile-time Compile-time settings
* @brief Settings to choose at compile-time
*/

/**
* @brief
*  Maximum number of characters.
* @ingroup
*  compile-time
*/
#define MAX_CHR 512

/**
* @brief
*  Maximum number of enchants.
* @ingroup
*  compile-time
*/
#define MAX_ENC 200

/**
* @brief
*  Maximum numberof EVEs.
*/
#define MAX_EVE 256

/**
* @brief
*  Maximum number of particles.
* @ingroup
*  compile-time
*/
#define MAX_PRT 2048

/**
* @brief
*  Maximum number of <tt>part*.text</tt> files per object.
* @ingroup
*  compile-time
*/
#define MAX_PIP_PER_PROFILE 13

/**
* @brief
*  Maximum number of textures of all characters.
* @ingroup
*  compile-time
*/
#define MAX_TEXTURE (MAX_CHR * 4)

/**
* @brief
*  Maximum number if icons of all characters.
* @ingroup
*  compile-time
*/
#define MAX_ICON (MAX_TEXTURE + 4)

/**
* @brief
*  The maximum number of textures.
* @ingroup
*  compile-time
*/
#define TX_COUNT (2*(MAX_TEXTURE + MAX_ICON))

/**
* @brief
*  Maximum number of audio files per object.
* @ingroup
*  compile-time
*/
#define MAX_WAVE 30

/**
* @brief
*  Maximum number of PIPs.
* @ingroup
*  compile-time
*/
#define MAX_PIP (256 * MAX_PIP_PER_PROFILE)

/**
* @brief
*  Maximum number of dynamic lights.
* @ingroup
*  compile-time
*/
#define TOTAL_MAX_DYNA 64

/**
* @brief
*  Maximum number of MADs.
* @ingroup compile-time
*/
#define MAX_MAD 256

/**
* @brief
*  Maximum number of billboards.
* @ingroup
*  compile-time
*/
#define MAX_BBOARD (2 * MAX_CHR)

// Some macro switches
#undef  OLD_CAMERA_MODE       ///< Use the old camera style
#undef  ENABLE_BODY_GRAB      ///< Enable the grabbing of bodies?
#undef  TEST_NAN_RESULT       ///< Test the result of certain math operations

#undef  RENDER_HMAP           ///< render the mesh's heightmap?
#undef  DEBUG_MESH_NORMALS    ///< render the mesh normals

#undef  DEBUG_BSP             ///< Print debugging info about the BSP/octree state

#undef  DEBUG_RENDERLIST      ///< Print debugging info for the currently rendered mesh

#undef  DEBUG_PROFILE         ///< Switch the profiling functions on and off
#undef  DEBUG_PROFILE_DISPLAY ///< Display the results for the performance profiling
#undef  DEBUG_PROFILE_RENDER  ///< Display the results for the performance profiling of the generric rendering
#undef  DEBUG_PROFILE_MESH    ///< Display the results for the performance profiling of the mesh rendering sub-system
#undef  DEBUG_PROFILE_INIT    ///< Display the results for the performance profiling of the rendering initialization

#undef  DEBUG_OBJECT_SPAWN    ///< Log debug info for every object spawned

#undef  DEBUG_PRT_LIST        ///< Track every single deletion from the PrtList to make sure the same element is not deleted twice. Prevents corruption of the PrtList.free_lst
#undef  DEBUG_ENC_LIST        ///< Track every single deletion from the EncList to make sure the same element is not deleted twice. Prevents corruption of the EncList.free_lst
#undef  DEBUG_CHR_LIST        ///< Track every single deletion from the ChrList to make sure the same element is not deleted twice. Prevents corruption of the ChrList.free_lst

#define CLIP_LIGHT_FANS       ///< is the light_fans() function going to be throttled?
#undef  CLIP_ALL_LIGHT_FANS   ///< a switch for selecting how the fans will be updated

#undef  DEBUG_WAYPOINTS       ///< display error messages when adding waypoints. It will also prevent "unsafe" waypoint being added to the waypoint list.

#undef  DEBUG_ASTAR           ///< Debug AStar pathfinding

#define DRAW_CHR_BBOX         ///< display selected character bounding boxes
#define DRAW_PRT_BBOX         ///< display selected particle bounding boxes
#define DRAW_LISTS            ///< display any lines or points that have been added to various lists

#define MAD_CULL_RIGHT        ///< helps to define which faces are clipped when rendering character models
#define MAP_CULL_RIGHT        ///< helps to define which faces are clipped when rendering the mesh

#undef EGOBOO_THROTTLED       ///< are the inner loops of the game throttled?

// do the includes last so that the compile switches are always set
#include "egolib/platform.h"
#include "egolib/endian.h"