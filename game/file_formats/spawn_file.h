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

/// @file file_formats/spawn_file.h
/// @details loading the environment definitions for a module

#include "egoboo_typedef.h"
#include "egoboo_vfs.h"
#include "egoboo_math.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Where a spawned character can be attached
    enum e_attachment_type
    {
        ATTACH_NONE       = 0,
        ATTACH_INVENTORY,
        ATTACH_LEFT,
        ATTACH_RIGHT
    };

#define FACE_RANDOM  generate_randmask(0, 0xFFFF)

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The internal representation of a single line in "spawn.txt"
    struct s_spawn_file_info
    {
        bool_t     do_spawn;
        STRING     spawn_coment;

        STRING     spawn_name;
        char      *pname;
        int        slot;
        fvec3_t    pos;
        int        passage, content, money, level, skin;
        bool_t     stat;
        TEAM_REF   team;
        FACING_T   facing;
        CHR_REF    attach;
        CHR_REF    parent;
    };
    typedef struct s_spawn_file_info spawn_file_info_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
    spawn_file_info_t * spawn_file_info_init( spawn_file_info_t *pinfo );
    spawn_file_info_t * spawn_file_info_reinit( spawn_file_info_t *pinfo );

    bool_t spawn_file_scan( vfs_FILE * fileread, spawn_file_info_t *pinfo );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _spawn_file_h