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
/// @brief Handles functions that modify quest.txt files
/// @details

#include "quest.h"

#include "log.h"

#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"
#include "egoboo_math.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
bool_t quest_add_idsz( const char *player_directory, const IDSZ idsz )
{
    /// @details ZF@> This function writes a IDSZ (With quest level 0) into a player quest.txt
    //      file, returns btrue if it succeeded.

    vfs_FILE *filewrite;
    STRING newloadname;

    // Only add quest IDSZ if it doesnt have it already
    if ( idsz == IDSZ_NONE || quest_check( player_directory, idsz ) >= QUEST_BEATEN ) return bfalse;

    // Try to open the file in read and append mode
    snprintf( newloadname, SDL_arraysize( newloadname ), "%s/quest.txt", player_directory );
    filewrite = vfs_openAppend( newloadname );

    // Create the file if it does not exist
    if ( !filewrite )
    {
        filewrite = vfs_openWrite( newloadname );
        if ( !filewrite )
        {
            log_warning( "Cannot create quest file! (%s)\n", newloadname );
            return bfalse;
        }

        vfs_printf( filewrite, "// This file keeps order of all the quests for the player (%s)\n", player_directory );
        vfs_printf( filewrite, "// The number after the IDSZ shows the quest level. -1 means it is completed." );
    }

    vfs_printf( filewrite, "\n:[%4s] 0", undo_idsz( idsz ) );
    vfs_close( filewrite );

    return btrue;
}

//--------------------------------------------------------------------------------------------
int quest_modify_idsz( const char *player_directory, const IDSZ idsz, const int adjustment )
{
    /// @details ZF@> This function increases or decreases a IDSZ quest level by the amount determined in
    ///     adjustment. It then returns the current quest level it now has.
    ///     It returns QUEST_NONE if failed and if the adjustment is 0, the quest is marked as beaten...

    vfs_FILE *filewrite, *fileread;
    STRING newloadname, copybuffer;
    IDSZ newidsz;
    char ctmp;
    int newquestlevel = QUEST_NONE, questlevel;

    // Now check each expansion until we find correct IDSZ
    if ( adjustment == 0 || quest_check( player_directory, idsz ) <= QUEST_BEATEN )  return QUEST_NONE;

    // modify the CData.quest_file
    // create a "tmp_*" copy of the file
    snprintf( newloadname, SDL_arraysize( newloadname ), "%s/quest.txt", player_directory );
    snprintf( copybuffer, SDL_arraysize( copybuffer ), "%s/tmp_quest.txt", player_directory );
    vfs_copyFile( newloadname, copybuffer );

    // open the tmp file for reading and overwrite the original file
    fileread  = vfs_openRead( copybuffer );
    filewrite = vfs_openWrite( newloadname );

    // Something went wrong
    if ( !fileread || !filewrite )
    {
        log_warning( "Could not modify quest IDSZ (%s).\n", newloadname );
        return QUEST_NONE;
    }

    // read the tmp file line-by line
    while ( !vfs_eof( fileread ) )
    {
        ctmp = vfs_getc( fileread );
        vfs_ungetc( ctmp, fileread );
        if ( '/' == ctmp )
        {
            // copy comments exactly
            fcopy_line( fileread, filewrite );
        }
        else if ( goto_colon( NULL, fileread, btrue ) )
        {
            // scan the line for quest info
            newidsz = fget_idsz( fileread );
            questlevel = fget_int( fileread );

            // modify it
            if ( newidsz == idsz )
            {
                newquestlevel = questlevel = ABS( questlevel + adjustment );      // Don't get negative
            }

            vfs_printf( filewrite, "\n:[%s] %i", undo_idsz( newidsz ), questlevel );
        }
    }

    // clean it up
    vfs_close( fileread );
    vfs_close( filewrite );
    vfs_delete_file( copybuffer );

    return newquestlevel;
}

//--------------------------------------------------------------------------------------------
int quest_check( const char *player_directory, const IDSZ idsz )
{
    /// @details ZF@> This function checks if the specified player has the IDSZ in his or her quest.txt
    /// and returns the quest level of that specific quest (Or QUEST_NONE if it is not found, QUEST_BEATEN if it is finished)

    vfs_FILE *fileread;
    STRING newloadname;
    int result = QUEST_NONE;

    snprintf( newloadname, SDL_arraysize( newloadname ), "%s/quest.txt", player_directory );
    fileread = vfs_openRead( newloadname );

    printf( "----quest_check(\"%s\",[%s]) - update == %d\n", player_directory, undo_idsz( idsz ), update_wld );

    if ( NULL == fileread ) return result;

    // Always return "true" for [NONE] IDSZ checks
    if ( idsz == IDSZ_NONE ) result = QUEST_BEATEN;

    // Check each expansion
    while ( goto_colon( NULL, fileread, btrue ) )
    {
        if ( fget_idsz( fileread ) == idsz )
        {
            result = fget_int( fileread );  // Read value behind colon and IDSZ
            break;
        }
    }

    vfs_close( fileread );

    return result;
}

