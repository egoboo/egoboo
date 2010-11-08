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

#include "IDSZ_map.h"
#include "quest.h"
#include "log.h"

#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"
#include "egoboo_math.inl"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
bool_t quest_add_idsz_vfs( const char *player_directory, const IDSZ idsz )
{
    /// @details ZF@> This function writes a IDSZ (With quest level 0) into a player quest.txt
    //      file, returns btrue if it succeeded.

    vfs_FILE *filewrite;
    STRING newloadname;

    // Only add quest IDSZ if it doesnt have it already
    if ( idsz == IDSZ_NONE || quest_check_vfs( player_directory, idsz, btrue ) != QUEST_NONE ) return bfalse;

	// Figure out the file path
	snprintf( newloadname, SDL_arraysize( newloadname ), "import%s/quest.txt", vfs_mount_info_strip_path( player_directory ) );

    // Try to open the file in read and append mode
	filewrite = vfs_openAppend( newloadname );

    // Create the file if it does not exist
    if ( NULL == filewrite )
    {
        log_debug( "Creating new quest file (%s)\n", newloadname );
		        
		filewrite = vfs_openWrite( newloadname );
        if ( !filewrite )
        {
            log_warning( "Cannot create quest file! (%s)\n", newloadname );
            return bfalse;
        }

        vfs_printf( filewrite, "// This file keeps order of all the quests for this player\n" );
        vfs_printf( filewrite, "// The number after the IDSZ shows the quest level. -1 means it is completed." );
    }

    vfs_printf( filewrite, "\n:[%4s] 0", undo_idsz( idsz ) );
    vfs_close( filewrite );

    return btrue;
}

//--------------------------------------------------------------------------------------------
int quest_modify_idsz_vfs( const char *player_directory, const IDSZ idsz, const int adjustment )
{
    /// @details ZF@> This function increases or decreases a IDSZ quest level by the amount determined in
    ///     adjustment. It then returns the current quest level it now has.
    ///     It returns QUEST_NONE if failed and if the adjustment is QUEST_BEATEN, the quest is permanently flagged as beaten...

    vfs_FILE *filewrite, *fileread;
    STRING newloadname, copybuffer;
    IDSZ newidsz;
    char ctmp;
	const char* path;
    int retval = QUEST_NONE, questlevel;

	// Cannot modify a quest if they don't have it or if it is beaten
	retval = quest_check_vfs( player_directory, idsz, btrue );
    if ( retval == QUEST_NONE || retval == QUEST_BEATEN )  return QUEST_NONE;

    // modify the CData.quest_file
    // create a "tmp_*" copy of the file
	path = vfs_mount_info_strip_path( player_directory );
    snprintf( newloadname, SDL_arraysize( newloadname ), "import%s/quest.txt", path );
    snprintf( copybuffer, SDL_arraysize( copybuffer ), "import%s/tmp_quest.txt", path );
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

    // Now check each expansion until we find correct IDSZ
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
				if( adjustment == QUEST_BEATEN )	questlevel = QUEST_BEATEN;							//beaten
                else								questlevel = MAX( 0, questlevel + adjustment );		//normal adjustment
                retval = questlevel;
            }

            vfs_printf( filewrite, "\n:[%s] %i", undo_idsz( newidsz ), questlevel );
        }
    }

    // clean it up
    vfs_close( fileread );
    vfs_close( filewrite );
    vfs_delete_file( copybuffer );

    return retval;
}

//--------------------------------------------------------------------------------------------
int quest_check_vfs( const char *player_directory, const IDSZ idsz, bool_t import_chr )
{
    /// @details ZF@> This function checks if the specified player has the IDSZ in his or her quest.txt
    /// and returns the quest level of that specific quest (Or QUEST_NONE if it is not found)

    vfs_FILE *fileread;
    STRING newloadname;
    int result = QUEST_NONE;
	const char* prefix;

	// Always return "true" for [NONE] IDSZ checks
    if ( idsz == IDSZ_NONE ) return 0;

	if( import_chr ) prefix = "import";
	else			 prefix = "players";

	// Figure out the file path
	snprintf( newloadname, SDL_arraysize( newloadname ), "%s%s/quest.txt", prefix, vfs_mount_info_strip_path( player_directory ) );
    fileread = vfs_openRead( newloadname );

	log_debug( "----quest_check_vfs(\"%s\",[%s]) - update == %d\n", newloadname, undo_idsz( idsz ), update_wld );

    if ( NULL == fileread ) return result;

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

