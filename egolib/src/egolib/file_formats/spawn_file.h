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

/// @file egolib/file_formats/spawn_file.h
/// @details loading the environment definitions for a module

#pragma once

#include "egolib/typedef.h"
#include "egolib/fileutil.h"
#include "egolib/vec.h"

/// Where a spawned character can be attached
enum e_attachment_type
{
    ATTACH_NONE       = 0,
    ATTACH_INVENTORY,
    ATTACH_LEFT,
    ATTACH_RIGHT
};

/// The internal representation of a single line in "spawn.txt"
struct spawn_file_info_t
{
    bool       do_spawn;
    STRING     spawn_coment;

    STRING     spawn_name;
    char      *pname;
    int        slot;
    fvec3_t    pos;
    int        passage;
    int        content;
    int        money;
    int        level;
    int        skin;
    bool       stat;
    REF_T      team;
    FACING_T   facing;
    REF_T      attach;
    REF_T      parent;
};
/**
 * @brief
 *  Assign safe values to all fields.
 */
spawn_file_info_t *spawn_file_info_init(spawn_file_info_t *self);
/**
 * @brief
 *  Assign safe values to all fields, keep the parent.
 */
spawn_file_info_t *spawn_file_info_reinit(spawn_file_info_t *self);
bool spawn_file_read(ReadContext& ctxt, spawn_file_info_t *self);

