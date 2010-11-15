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

/// @file platform/file_amiga.c
/// @brief Implementation of the Amiga system-dependent filesystem functions
/// @details

#include "file_common.h"
#include "log.h"

#include "egoboo_strutil.h"
#include "egoboo.h"

#include <unistd.h>
#include <pwd.h>
#include <grp.h>
//#include <glob.h>
#include "glob.h"
#include <sys/types.h>
#include <sys/stat.h>
//#include <sys/dir.h>
#include "egoboo_mem.h"

#define PATH_MAX 256
#define PREFIX "PROGDIR:"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern void sys_fs_init();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Paths that the game will deal with
static char amiga_binaryPath[PATH_MAX]   = EMPTY_CSTR;
static char amiga_dataPath[PATH_MAX]     = EMPTY_CSTR;
static char amiga_userdataPath[PATH_MAX] = EMPTY_CSTR;
static char amiga_configPath[PATH_MAX]   = EMPTY_CSTR;

struct s_amiga_find_context
{
    glob_t last_find;
    size_t find_index;
};

typedef struct s_amiga_find_context amiga_find_context_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// File Routines
void sys_fs_init()
{
    char * userhome;

    printf( "Initializing filesystem services...\n" );

    // grab the user's home directory
    userhome = "PROGDIR:";
    snprintf( amiga_userdataPath, SDL_arraysize( amiga_userdataPath ), "%s.egoboo-2.x", userhome );

#if defined(_NIX_PREFIX) && defined(PREFIX)
    // the access to these directories is completely unknown
    // The default setting from the Makefile is to set PREFIX = "/usr",
    // so that the program will compile and install just like any other
    // .rpm or .deb package.

    strncpy( amiga_configPath, PREFIX ,         SDL_arraysize( amiga_configPath ) );
    strncpy( amiga_binaryPath, PREFIX ,                  SDL_arraysize( amiga_binaryPath ) );
    strncpy( amiga_dataPath,   PREFIX , SDL_arraysize( amiga_dataPath ) );
#else
    // these are read-only directories
    strncpy( amiga_configPath, "",         SDL_arraysize( amiga_configPath ) );
    strncpy( amiga_binaryPath, "",                  SDL_arraysize( amiga_binaryPath ) );
    strncpy( amiga_dataPath,   "", SDL_arraysize( amiga_dataPath ) );
#endif

    // the log file cannot be started until there is a user data path to dump the file into
    // so dump this debug info to stdout
    printf( "Game directories are:\n\tBinaries: %s\n\tData: %s\n\tUser Data: %s\n\tConfig Files: %s\n",
            amiga_binaryPath, amiga_dataPath, amiga_userdataPath, amiga_configPath );

    if ( !fs_fileIsDirectory( amiga_userdataPath ) )
    {
        fs_createDirectory( amiga_userdataPath );
    }
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
    /// @details ZZ@> This function makes a new directory
    return mkdir( dirname, 0755 );
}

//--------------------------------------------------------------------------------------------
int fs_removeDirectory( const char *dirname )
{
    /// @details ZZ@> This function removes a directory
    return rmdir( dirname );
}

//--------------------------------------------------------------------------------------------
void fs_deleteFile( const char *filename )
{
    /// @details ZZ@> This function deletes a file
    unlink( filename );
}

//--------------------------------------------------------------------------------------------
bool_t fs_copyFile( const char *source, const char *dest )
{
    /// @details ZZ@> This function copies a file on the local machine

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

    while (( bytes_read = fread( buf, 1, sizeof( buf ), sourcef ) ) )
        fwrite( buf, 1, bytes_read, destf );

    //Finish it up
    fclose( sourcef );
    fclose( destf );
    return btrue;
}

//--------------------------------------------------------------------------------------------
// Read the first directory entry
const char *fs_findFirstFile( const char *searchDir, const char *searchExtension, fs_find_context_t * fs_search )
{

    char pattern[PATH_MAX] = EMPTY_CSTR;
    char *last_slash;
    amiga_find_context_t * pcnt;

    if ( INVALID_CSTR( searchDir ) || NULL == fs_search ) return NULL;

    pcnt = EGOBOO_NEW( amiga_find_context_t );
    fs_search->type = amiga_find;
    fs_search->ptr.l = pcnt;

    if ( searchExtension )
        snprintf( pattern, PATH_MAX, "%s" SLASH_STR "*.%s", searchDir, searchExtension );
    else
        snprintf( pattern, PATH_MAX, "%s" SLASH_STR "*", searchDir );

    pcnt->last_find.gl_offs = 0;
    glob( pattern, GLOB_NOSORT, NULL, &pcnt->last_find );
    if ( !pcnt->last_find.gl_pathc )
        return NULL;

    pcnt->find_index = 0;
    last_slash = strrchr( pcnt->last_find.gl_pathv[pcnt->find_index], '/' );
    if ( last_slash )
        return last_slash + 1;

    return NULL; /* should never happen */
}

//--------------------------------------------------------------------------------------------
// Read the next directory entry (NULL if done)
const char *fs_findNextFile( fs_find_context_t * fs_search )
{

    char *last_slash;
    amiga_find_context_t * pcnt;

    if ( NULL == fs_search || fs_search->type != amiga_find ) return NULL;

    pcnt = fs_search->ptr.l;
    if ( NULL == pcnt )  return NULL;

    ++pcnt->find_index;
    if ( pcnt->find_index >= pcnt->last_find.gl_pathc )
        return NULL;

    last_slash = strrchr( pcnt->last_find.gl_pathv[pcnt->find_index], '/' );
    if ( last_slash )
        return last_slash + 1;

    return NULL; /* should never happen */
}

//--------------------------------------------------------------------------------------------
// Close anything left open
void fs_findClose( fs_find_context_t * fs_search )
{

    amiga_find_context_t * pcnt;

    if ( NULL == fs_search || fs_search->type != amiga_find ) return;

    pcnt = fs_search->ptr.l;
    if ( NULL == pcnt )  return;

    globfree( &( pcnt->last_find ) );

    EGOBOO_DELETE( pcnt );

    memset( fs_search, 0, sizeof( *fs_search ) );

}

//--------------------------------------------------------------------------------------------
const char *fs_getBinaryDirectory()
{
    return amiga_binaryPath;
}

//--------------------------------------------------------------------------------------------
const char *fs_getDataDirectory()
{
    return amiga_dataPath;
}

//--------------------------------------------------------------------------------------------
const char *fs_getUserDirectory()
{
    return amiga_userdataPath;
}

//--------------------------------------------------------------------------------------------
const char *fs_getConfigDirectory()
{
    return amiga_configPath;
}
