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

/// @file egolib/FileFormats/SpawnFile/spawn_file.c
/// @brief Implementation of a scanner for Egoboo's spawn.txt file
/// @details

#include "egolib/FileFormats/SpawnFile/spawn_file.h"
#include "egolib/FileFormats/SpawnFile/SpawnFileReaderImpl.hpp"
#include "egolib/Logic/Team.hpp"


spawn_file_info_t::spawn_file_info_t() :
    do_spawn(false),
    spawn_comment(),
    spawn_name(),
    slot(-1),
    pos(0.0f, 0.0f, 0.0f),
    passage(-1),
    content(0),
    money(0),
    level(0),
    skin(0),
    stat(false),
    team(Team::TEAM_NULL),
    facing(FACE_NORTH),
    attach(ATTACH_NONE)
{
    //ctor
}

std::vector<spawn_file_info_t> SpawnFileReader::read(const std::string& pathname)
{
    return impl->read(pathname);
}

SpawnFileReader::SpawnFileReader() :
    impl(std::make_unique<SpawnFileReaderImpl>())
{}

SpawnFileReader::~SpawnFileReader()
{}
