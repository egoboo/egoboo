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

/// @file egolib/FileFormats/quest_file.h
/// @brief read/write/modify the <tt>quest.txt</tt> file

#pragma once

#include "egolib/typedef.h"
#include "egolib/IDSZ.hpp"
#include "egolib/FileFormats/configfile.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Quest system values
    enum e_quest_values
    {
        QUEST_BEATEN         = -2,
        QUEST_NONE           = -1,

        QUEST_MINVAL         = QUEST_BEATEN,
        QUEST_MAXVAL         = 0x7FFFFFFF  // maximum positive signed integer
    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Public functions
egolib_rv quest_log_download_vfs(std::unordered_map<IDSZ, int> & quest_log, const char* player_directory);
egolib_rv quest_log_upload_vfs(const std::unordered_map<IDSZ, int> &quest_log, const char *player_directory);
int       quest_log_set_level(std::unordered_map<IDSZ, int> & quest_log, IDSZ idsz, int level);
int       quest_log_adjust_level(std::unordered_map<IDSZ, int> & quest_log, IDSZ idsz, int adjustment);
int       quest_log_get_level(std::unordered_map<IDSZ, int> & quest_log, IDSZ idsz);
egolib_rv quest_log_add(std::unordered_map<IDSZ, int> & quest_log, IDSZ idsz, int level);

std::shared_ptr<ConfigFile> quest_file_open(const char *player_directory);
egolib_rv quest_file_export(std::shared_ptr<ConfigFile> file);
