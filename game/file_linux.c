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

// lin-file.c

#include "file_common.h"
#include "log.h"

#include "egoboo.h"

#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Paths that the game will deal with
static char linux_binaryPath[PATH_MAX]   = EMPTY_CSTR;
static char linux_dataPath[PATH_MAX]     = EMPTY_CSTR;
static char linux_userdataPath[PATH_MAX] = EMPTY_CSTR;
static char linux_configPath[PATH_MAX]   = EMPTY_CSTR;

static glob_t last_find_glob;
static size_t glob_find_index;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// File Routines
void fs_init()
{
    char * username;

    printf( "Initializing filesystem services...\n" );

    username = getenv( "USER" );

    // this is just a skeleton. the USER needs to be replaced by an environment variable
    snprintf( linux_userdataPath, SDL_arraysize(linux_userdataPath), "/home/%s/.egoboo-2.x/", username );

    // this is a read-only directory
    strncpy( linux_configPath, "/etc/egoboo-2.x/",             SDL_arraysize(linux_configPath) );
    strncpy( linux_binaryPath, "/usr/games/egoboo-2.x/",       SDL_arraysize(linux_binaryPath) );
    strncpy( linux_dataPath,   "/usr/share/games/egoboo-2.x/", SDL_arraysize(linux_dataPath)   );

    // the log file cannot be started until there is a user data path to dump the file into
    // so dump this debug info to stdout
    printf( "Game directories are:\n\tBinaries: %s\n\tData: %s\n\tUser Data: %s\n\tConfig Files: %s\n",
              linux_binaryPath, linux_dataPath, linux_userdataPath, linux_configPath );
}

//--------------------------------------------------------------------------------------------
int fs_fileIsDirectory( const char *filename )
{
    struct stat stats;
    if ( !stat( filename, &stats ) )
        return S_ISDIR( stats.st_mode );

    return 0;
}

//--------------------------------------------------------------------------------------------
int fs_createDirectory( const char *dirname )
{
    // ZZ> This function makes a new directory
    return mkdir( dirname, 0755 );
}

//--------------------------------------------------------------------------------------------
int fs_removeDirectory( const char *dirname )
{
    // ZZ> This function removes a directory
    return rmdir( dirname );
}

//--------------------------------------------------------------------------------------------
void fs_deleteFile( const char *filename )
{
    // ZZ> This function deletes a file
    unlink( filename );
}

//--------------------------------------------------------------------------------------------
bool_t fs_copyFile( const char *source, const char *dest )
{
    // ZZ> This function copies a file on the local machine

    FILE *sourcef;
    FILE *destf;
    char buf[4096] = EMPTY_CSTR;
    int bytes_read;

    sourcef = fopen( source, "rb" );
    if ( !sourcef )
        return bfalse;

    destf = fopen( source, "wb" );
    if ( !destf )
    {
        fclose( sourcef );
        return bfalse;
    }

    while ( ( bytes_read = fread( buf, 1, sizeof( buf ), sourcef ) ) )
        fwrite( buf, 1, bytes_read, destf );

	//Finish it up
    fclose( sourcef );
    fclose( destf );
	return btrue;
}

//--------------------------------------------------------------------------------------------
// Read the first directory entry
const char *fs_findFirstFile( const char *searchDir, const char *searchExtension )
{
    char pattern[PATH_MAX] = EMPTY_CSTR;
    char *last_slash;

    if ( searchExtension )
        snprintf( pattern, PATH_MAX, "%s" SLASH_STR "*.%s", searchDir, searchExtension );
    else
        snprintf( pattern, PATH_MAX, "%s" SLASH_STR "*", searchDir );

    last_find_glob.gl_offs = 0;
    glob( pattern, GLOB_NOSORT, NULL, &last_find_glob );
    if ( !last_find_glob.gl_pathc )
        return NULL;

    glob_find_index = 0;
    last_slash = strrchr( last_find_glob.gl_pathv[glob_find_index], '/' );
    if ( last_slash )
        return last_slash + 1;

    return NULL; /* should never happen */
}

//--------------------------------------------------------------------------------------------
// Read the next directory entry (NULL if done)
const char *fs_findNextFile( void )
{
    char *last_slash;

    ++glob_find_index;
    if ( glob_find_index >= last_find_glob.gl_pathc )
        return NULL;

    last_slash = strrchr( last_find_glob.gl_pathv[glob_find_index], '/' );
    if ( last_slash )
        return last_slash + 1;

    return NULL; /* should never happen */
}

//--------------------------------------------------------------------------------------------
// Close anything left open
void fs_findClose()
{
    globfree( &last_find_glob );
}

//--------------------------------------------------------------------------------------------
const char *fs_getBinaryDirectory()
{
    return linux_binaryPath;
}

//--------------------------------------------------------------------------------------------
const char *fs_getDataDirectory()
{
    return linux_dataPath;
}

//--------------------------------------------------------------------------------------------
const char *fs_getUserDirectory()
{
    return linux_userdataPath;
}

//--------------------------------------------------------------------------------------------
const char *fs_getConfigDirectory()
{
    return linux_configPath;
}
