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

/// @file quest.h
/// @brief read/write/modify the quest.txt file

#include "../typedef.h"

#include "configfile.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// @note BB@> use this forward declaration of the "struct s_IDSZ_node" instead of including
/// "IDSZ_map.h" to remove possible circular dependencies
    struct s_IDSZ_node;

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
    egolib_rv quest_log_download_vfs( struct s_IDSZ_node quest_log[], size_t quest_log_len, const char* player_directory );
    egolib_rv quest_log_upload_vfs( struct s_IDSZ_node quest_log[], size_t quest_log_len, const char *player_directory );
    int       quest_log_set_level( struct s_IDSZ_node quest_log[], size_t quest_log_len, IDSZ idsz, int level );
    int       quest_log_adjust_level( struct s_IDSZ_node quest_log[], size_t quest_log_len, IDSZ idsz, int adjustment );
    int       quest_log_get_level( struct s_IDSZ_node quest_log[], size_t quest_log_len, IDSZ idsz );
    egolib_rv quest_log_add( struct s_IDSZ_node quest_log[], size_t quest_log_len, IDSZ idsz, int level );

    ConfigFilePtr_t quest_file_open( const char *player_directory );
    egolib_rv       quest_file_export( ConfigFilePtr_t pfile );
    egolib_rv       quest_file_close( ConfigFilePtr_t * ppfile, bool_t do_export );
    egolib_rv       quest_file_set_level( ConfigFilePtr_t ppfile, IDSZ idsz, int level );
    egolib_rv       quest_file_adjust_level( ConfigFilePtr_t ppfile, IDSZ idsz, int adjustment );
    egolib_rv       quest_file_get_level( ConfigFilePtr_t ppfile, IDSZ idsz );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _file_formats_quest_fille_h
