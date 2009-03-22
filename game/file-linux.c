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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

// lin-file.c

#include "egoboo.h"

#include <stdio.h>
#include <unistd.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static glob_t last_find_glob;
static size_t glob_find_index;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// File Routines
void fs_init()
{
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
void fs_copyFile( const char *source, const char *dest )
{
    // ZZ> This function copies a file on the local machine
    FILE *sourcef;
    FILE *destf;
    char buf[4096];
    int bytes_read;

    sourcef = fopen( source, "r" );

    if ( !sourcef )
        return;

    destf = fopen( dest, "w" );

    if ( !destf )
    {
        fclose( sourcef );
        return;
    }

    while ( ( bytes_read = fread( buf, 1, sizeof( buf ), sourcef ) ) )
        fwrite( buf, 1, bytes_read, destf );

    fclose( sourcef );
    fclose( destf );
}

//--------------------------------------------------------------------------------------------
void empty_import_directory( void )
{
    // ZZ> This function deletes all the TEMP????.OBJ subdirectories in the IMPORT directory
    system( "rm -rf import" SLASH_STR "temp*.obj\n" );
}

//--------------------------------------------------------------------------------------------
// Read the first directory entry
const char *fs_findFirstFile( const char *searchDir, const char *searchExtension )
{
    char pattern[PATH_MAX];
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
//Open a specific file
FILE * fs_openFile(const char * path, const char * module, const char * player)
{
}

//--------------------------------------------------------------------------------------------
// Close anything left open
void fs_findClose()
{
    globfree( &last_find_glob );
}
