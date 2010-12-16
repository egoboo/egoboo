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

/// @file quest.c
/// @brief Handles functions that modify quest.txt files and the players quest log
/// @details ZF@> This could be done more optimal with a proper HashMap allowing O(1) speed instead of O(n)
///              I think we should also implement a similar system for skill IDSZ.

#include "IDSZ_map.h"
#include "quest.h"
#include "log.h"

#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"
#include "egoboo.h"

#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ConfigFilePtr_t quest_file_open( const char *player_directory )
{
    STRING          newloadname = EMPTY_CSTR;
    ConfigFilePtr_t retval      = NULL;

    if ( !VALID_CSTR( player_directory ) ) return NULL;

    // Figure out the file path
    snprintf( newloadname, SDL_arraysize( newloadname ), "%s/quest.txt", player_directory );

    retval = LoadConfigFile( newloadname, bfalse );
    if ( NULL == retval )
    {
        retval = ConfigFile_create();
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv quest_file_export( ConfigFilePtr_t pfile )
{
    egoboo_rv         rv      = rv_error;
    ConfigFile_retval save_rv = ConfigFile_succeed;

    if ( NULL == pfile ) return rv_error;

    save_rv = SaveConfigFile( pfile );

    rv = ( ConfigFile_succeed == save_rv ) ? rv_success : rv_fail;

    return rv;
}

//--------------------------------------------------------------------------------------------
egoboo_rv quest_file_close( ConfigFilePtr_t * ppfile, bool_t export )
{
    egoboo_rv export_rv = rv_success;

    if ( NULL == ppfile || NULL == *ppfile ) return rv_error;

    if ( export )
    {
        export_rv = quest_file_export( *ppfile );

        if ( rv_error == export_rv )
        {
            log_warning( "quest_file_close() - error writing quest.txt\n" );
        }
        else if ( rv_fail == export_rv )
        {
            log_warning( "quest_file_close() - could not export quest.txt\n" );
        }
    }

    if ( ConfigFile_succeed != ConfigFile_destroy( ppfile ) )
    {
        log_warning( "quest_file_close() - could not successfully close quest.txt\n" );
    }

    return ( NULL == *ppfile ) && ( rv_success == export_rv ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egoboo_rv quest_log_download_vfs( IDSZ_node_t quest_log[], size_t quest_log_len, const char* player_directory )
{
    /// @details ZF@> Reads a quest.txt for a player and turns it into a data structure
    ///               we can use. If the file isn't found, the quest log will be initialized as empty.

    egoboo_rv retval = rv_success;
    vfs_FILE *fileread;
    STRING newloadname;

    if ( quest_log == NULL ) return rv_error;

    // blank out the existing map
    idsz_map_init( quest_log, quest_log_len );

    // Figure out the file path
    snprintf( newloadname, SDL_arraysize( newloadname ), "%s/quest.txt", player_directory );

    // try to open the file
    fileread = vfs_openRead( newloadname );
    if ( NULL == fileread ) return rv_success;

    // Load each IDSZ
    retval = rv_success;
    while ( goto_colon( NULL, fileread, btrue ) )
    {
        egoboo_rv rv;

        IDSZ idsz = fget_idsz( fileread );
        int  level = fget_int( fileread );

        // Try to add a single quest to the map
        rv = idsz_map_add( quest_log, quest_log_len, idsz, level );

        // Stop here if it failed
        if ( rv_error == rv )
        {
            log_warning( "quest_log_download_vfs() - Encountered an error while trying to add a quest. (%s)\n", newloadname );
            retval = rv;
            break;
        }
        else if ( rv_fail == rv )
        {
            log_warning( "quest_log_download_vfs() - Unable to load all quests. (%s)\n", newloadname );
            retval = rv;
            break;
        }
    }

    // Close up after we are done with it
    vfs_close( fileread );

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv quest_log_upload_vfs( IDSZ_node_t quest_log[], size_t quest_log_len, const char *player_directory )
{
    /// @details ZF@> This exports quest_log data into a quest.txt file
    vfs_FILE *filewrite;
    int iterator;
    IDSZ_node_t *pquest;

    if ( quest_log == NULL ) return rv_error;

    // Write a new quest file with all the quests
    filewrite = vfs_openWrite( player_directory );
    if ( NULL == filewrite )
    {
        log_warning( "Cannot create quest file! (%s)\n", player_directory );
        return rv_fail;
    }

    vfs_printf( filewrite, "// This file keeps order of all the quests for this player\n" );
    vfs_printf( filewrite, "// The number after the IDSZ shows the quest level. %i means it is completed.", QUEST_BEATEN );

    // Iterate through every element in the IDSZ map
    iterator = 0;
    pquest = idsz_map_iterate( quest_log, quest_log_len, &iterator );
    while ( pquest != NULL )
    {
        // Write every single quest to the quest log
        vfs_printf( filewrite, "\n:[%4s] %i", undo_idsz( pquest->id ), pquest->level );

        // Get the next element
        pquest = idsz_map_iterate( quest_log, quest_log_len, &iterator );
    }

    // Clean up and return
    vfs_close( filewrite );
    return rv_success;
}

//--------------------------------------------------------------------------------------------
int quest_set_level( IDSZ_node_t quest_log[], size_t quest_log_len, IDSZ idsz, int level )
{
    ///@details    ZF@> This function will set the quest level for the specified quest
    ///            and return the new quest_level. It will return QUEST_NONE if the quest was
    ///            not found.

    IDSZ_node_t *pquest = NULL;

    // find the quest
    pquest = idsz_map_get( quest_log, quest_log_len, idsz );
    if ( pquest == NULL ) return QUEST_NONE;

    // make a copy of the quest's level
    pquest->level = level;

    return level;
}

//--------------------------------------------------------------------------------------------
int quest_adjust_level( IDSZ_node_t quest_log[], size_t quest_log_len, IDSZ idsz, int adjustment )
{
    ///@details    ZF@> This function will modify the quest level for the specified quest with adjustment
    ///            and return the new quest_level total. It will return QUEST_NONE if the quest was
    ///            not found or if it was already beaten.

    int          src_level = QUEST_NONE;
    int          dst_level = QUEST_NONE;
    IDSZ_node_t *pquest    = NULL;

    // find the quest
    pquest = idsz_map_get( quest_log, quest_log_len, idsz );
    if ( pquest == NULL ) return QUEST_NONE;

    // make a copy of the quest's level
    src_level = pquest->level;

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
        else                             dst_level = MAX( 0, src_level + adjustment );

        // set the quest level
        pquest->level = dst_level;
    }

    return dst_level;
}

//--------------------------------------------------------------------------------------------
int quest_get_level( IDSZ_node_t quest_log[], size_t quest_log_len, IDSZ idsz )
{
    ///@details ZF@> Returns the quest level for the specified quest IDSZ.
    ///                 It will return QUEST_NONE if the quest was not found or if the quest was beaten.

    IDSZ_node_t *pquest;
    pquest = idsz_map_get( quest_log, quest_log_len, idsz );
    if ( pquest == NULL ) return QUEST_NONE;
    return pquest->level;
}

//--------------------------------------------------------------------------------------------
egoboo_rv quest_add( IDSZ_node_t quest_log[], size_t quest_log_len, IDSZ idsz, int level )
{
    ///@details ZF@> This adds a new quest to the quest log. If the quest is already in there, the higher quest
    ///                 level of either the old and new one will be kept.

    return idsz_map_add( quest_log, quest_log_len, idsz, level );
}
