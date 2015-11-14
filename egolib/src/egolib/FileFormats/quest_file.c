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

/// @file egolib/FileFormats/quest_file.c
/// @brief Handles functions that modify quest.txt files and the players quest log

#include "egolib/FileFormats/quest_file.h"

#include "egolib/log.h"

#include "egolib/fileutil.h"
#include "egolib/vfs.h"
#include "egolib/strutil.h"

#include "egolib/_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
std::shared_ptr<ConfigFile> quest_file_open(const char *player_directory)
{
    if (!player_directory || !strlen(player_directory))
    {
        return nullptr;
    }
    // Figure out the file path
    std::string newLoadName = std::string(player_directory) + "/quest.txt";
    std::shared_ptr<ConfigFile> configFile = ConfigFileParser().parse(newLoadName);
    if (!configFile)
    {
        configFile = std::make_shared<ConfigFile>(newLoadName);
    }
    return configFile;
}

//--------------------------------------------------------------------------------------------
egolib_rv quest_file_export(std::shared_ptr<ConfigFile> file)
{
    if (!file)
    {
        return rv_error;
    }
    if (!ConfigFileUnParser().unparse(file))
    {
		Log::get().warn("%s:%d: unable to export quest file `%s`\n", __FILE__, __LINE__, file->getFileName().c_str());
        return rv_fail;
    }
    return rv_success;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egolib_rv quest_log_download_vfs(std::unordered_map<IDSZ, int> &quest_log, const char* player_directory)
{
    /// @author ZF
    /// @details Reads a quest.txt for a player and turns it into a data structure
    ///               we can use. If the file isn't found, the quest log will be initialized as empty.

    // blank out the existing map
    quest_log.clear();

    // Figure out the file path
    std::string newLoadName = std::string(player_directory) + "/quest.txt";

    // Try to open a context
    ReadContext ctxt(newLoadName);
    if (!ctxt.ensureOpen()) return rv_error;

    // Load each IDSZ
    while (ctxt.skipToColon(true))
    {
        IDSZ idsz = ctxt.readIDSZ();
        int  level = ctxt.readInt();

        // Try to add a single quest to the map
        quest_log[idsz] = level;
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv quest_log_upload_vfs(const std::unordered_map<IDSZ, int> &quest_log, const char *player_directory )
{
    /// @author ZF
    /// @details This exports quest_log data into a quest.txt file
    vfs_FILE *filewrite;

    // Write a new quest file with all the quests
    filewrite = vfs_openWrite( player_directory );
    if ( NULL == filewrite )
    {
		Log::get().warn( "Cannot create quest file! (%s)\n", player_directory );
        return rv_fail;
    }

    vfs_printf( filewrite, "// This file keeps order of all the quests for this player\n" );
    vfs_printf( filewrite, "// The number after the IDSZ shows the quest level. %i means it is completed.", QUEST_BEATEN );

    // Iterate through every element in the IDSZ map
    for(const auto& node : quest_log)
    {
        // Write every single quest to the quest log
        vfs_printf( filewrite, "\n:[%4s] %i", undo_idsz(node.first), node.second);
    }

    // Clean up and return
    vfs_close( filewrite );
    return rv_success;
}

//--------------------------------------------------------------------------------------------
int quest_log_set_level(std::unordered_map<IDSZ, int> &quest_log, IDSZ idsz, int level )
{
    /// @author ZF
    /// @details This function will set the quest level for the specified quest
    ///          and return the new quest_level. It will return QUEST_NONE if the quest was
    ///          not found.

    // find the quest
    if(quest_log.find(idsz) == quest_log.end()) return QUEST_NONE;

    quest_log[idsz] = level;

    return level;
}

//--------------------------------------------------------------------------------------------
int quest_log_adjust_level( std::unordered_map<IDSZ, int> &quest_log, IDSZ idsz, int adjustment )
{
    /// @author ZF
    /// @details This function will modify the quest level for the specified quest with adjustment
    ///          and return the new quest_level total. It will return QUEST_NONE if the quest was
    ///          not found or if it was already beaten.

    int          src_level = QUEST_NONE;
    int          dst_level = QUEST_NONE;

    // find the quest
    if(quest_log.find(idsz) == quest_log.end()) return QUEST_NONE;

    // make a copy of the quest's level
    src_level = quest_log[idsz];

    // figure out what the dst_level is
    if ( QUEST_BEATEN == src_level )
    {
        // Don't modify quests that are already beaten
        dst_level = src_level;
    }
    else
    {
        // if the quest "doesn't exist" make the src_level 0
        if ( QUEST_NONE   == src_level ) src_level = 0;

        // Modify the quest level for that specific quest
        if ( adjustment == QUEST_MAXVAL ) dst_level = QUEST_BEATEN;
        else                             dst_level = std::max( 0, src_level + adjustment );

        // set the quest level
        quest_log[idsz] = dst_level;
    }

    return dst_level;
}

//--------------------------------------------------------------------------------------------
int quest_log_get_level( std::unordered_map<IDSZ, int> &quest_log, IDSZ idsz )
{
    /// @author ZF
    /// @details Returns the quest level for the specified quest IDSZ.
    ///          It will return QUEST_NONE if the quest was not found or if the quest was beaten.

    const auto &result = quest_log.find(idsz);

    if(result == quest_log.end()) return QUEST_NONE;

    return (*result).second;
}

//--------------------------------------------------------------------------------------------
egolib_rv quest_log_add( std::unordered_map<IDSZ, int> &quest_log, IDSZ idsz, int level )
{
    /// @author ZF
    /// @details This adds a new quest to the quest log. If the quest is already in there, the higher quest
    ///          level of either the old and new one will be kept.

    quest_log[idsz] = std::max(quest_log[idsz], level);
    return rv_success;
}
