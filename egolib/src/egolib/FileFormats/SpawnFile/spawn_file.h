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

/// @file egolib/FileFormats/SpawnFile/spawn_file.h
/// @details loading the environment definitions for a module

#pragma once

#include "egolib/typedef.h"
#include "egolib/_math.h"
#include "egolib/Math/Standard.hpp"

// Forward declaration.
class SpawnFileReaderImpl;

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
public:
	/**
	 * @brief
	 *	Construct this spawn file info with safe values.
	 */
    spawn_file_info_t();

    bool       do_spawn;
    std::string spawn_comment;

    std::string spawn_name;
#if 0
    std::string *pname;
#endif
    int        slot;
	Vector3f   pos;
    int        passage;
    int        content;
    int        money;
    int        level;
    int        skin;
    bool       stat;
    REF_T      team;
    Facing     facing;
    REF_T      attach;
};


struct SpawnFileReader
{
private:
    std::unique_ptr<SpawnFileReaderImpl> impl;
public:
    SpawnFileReader();
    ~SpawnFileReader();
    std::vector<spawn_file_info_t> read(const std::string& pathname);
};
