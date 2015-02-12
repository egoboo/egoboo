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

/// @file  game/egoboo_config.h
/// @brief Compile switches.

#pragma once

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// compliation flags

// object pre-allocations
#define MAX_CHR             512            ///< Maximum number of characters
#define MAX_ENC             200            ///< Maximum number of enchantments
#define MAX_PRT             2048           ///< Maximum number of particles
#define TOTAL_MAX_DYNA      64             ///< Maximum number of dynamic lights

#define MAX_TEXTURE         (MAX_CHR * 4)     ///< Maximum number of textures
#define MAX_ICON            (MAX_TEXTURE + 4) ///< Maximum number of icons

/// per-object pre-allocations
#define MAX_WAVE            30        ///< Maximum number of *.wav/*.ogg per object
#define MAX_PIP_PER_PROFILE 13        ///< Maximum number of part*.txt per object
#define MAX_PIP             (256 * MAX_PIP_PER_PROFILE)

// special values
#if 0
#define INVALID_CHR_IDX     MAX_CHR
#define INVALID_ENC_IDX     MAX_ENC
#define INVALID_PRT_IDX     MAX_PRT
#define INVALID_PRO_IDX     0xFFFF
#define INVALID_PIP_IDX     MAX_PIP
#endif

#define INVALID_CHR_REF     (( CHR_REF ) MAX_CHR)
#define INVALID_ENC_REF     (( ENC_REF ) MAX_ENC)
#define INVALID_PRT_REF     (( PRT_REF ) MAX_PRT)
#define INVALID_PRO_REF     (( PRO_REF ) 0xFFFF)
#define INVALID_PIP_REF     (( PIP_REF ) MAX_PIP)

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

#undef   DEBUG_PRT_LIST       ///< Track every single deletion from the PrtList to make sure the same element is not deleted twice. Prevents corruption of the PrtList.free_lst
#undef   DEBUG_ENC_LIST       ///< Track every single deletion from the EncList to make sure the same element is not deleted twice. Prevents corruption of the EncList.free_lst
#undef   DEBUG_CHR_LIST       ///< Track every single deletion from the ChrList to make sure the same element is not deleted twice. Prevents corruption of the ChrList.free_lst

#define CLIP_LIGHT_FANS       ///< is the light_fans() function going to be throttled?
#undef  CLIP_ALL_LIGHT_FANS   ///< a switch for selecting how the fans will be updated

#undef  DEBUG_WAYPOINTS       ///< display error messages when adding waypoints. It will also prevent "unsafe" waypoint being added to the waypoint list.

#undef  DEBUG_ASTAR           ///< Debug AStar pathfinding

#undef  DRAW_CHR_BBOX         ///< display selected character bounding boxes
#undef  DRAW_PRT_BBOX         ///< display selected particle bounding boxes
#define DRAW_LISTS            ///< display any lines or points that have been added to various lists

#define MAD_CULL_RIGHT        ///< helps to define which faces are clipped when rendering character models
#define MAP_CULL_RIGHT        ///< helps to define which faces are clipped when rendering the mesh

#undef EGOBOO_THROTTLED       ///< are the inner loops of the game throttled?

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// do the includes last so that the compile switches are always set

#include "egolib/egolib.h"