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

/// @file file_common.c
/// @brief Base implementation of the Egoboo filesystem
/// @details File operations that are shared between various operating systems.
/// OS-specific code goes in *-file.c (such as win-file.c)

#include "file_common.h"

#include "egoboo_strutil.h"
#include "egoboo_vfs.h"
#include "egoboo_config.h"

#ifndef MAX_PATH
#define MAX_PATH 260  // Same value that Windows uses...
#endif

// FIXME: Doesn't handle deleting directories recursively yet.
void fs_removeDirectoryAndContents( const char *dirname, int recursive )
{
    /// @details ZZ@> This function deletes all files in a directory,
    ///    and the directory itself

    char filePath[MAX_PATH] = EMPTY_CSTR;
    const char *fileName;
    fs_find_context_t fs_search;

    // List all the files in the directory
    fileName = fs_findFirstFile( dirname, NULL, &fs_search );
    while ( NULL != fileName )
    {
        // Ignore files that start with a ., like .svn for example.
        if ( '.' != fileName[0] )
        {
            snprintf( filePath, MAX_PATH, "%s" SLASH_STR "%s", dirname, fileName );
            if ( fs_fileIsDirectory( filePath ) )
            {
                if ( recursive )
                {
                    fs_removeDirectoryAndContents( filePath, recursive );
                }
                else
                {
                    fs_removeDirectory( filePath );
                }
            }
            else
            {
                fs_deleteFile( filePath );
            }
        }
        fileName = fs_findNextFile( &fs_search );
    }
    fs_findClose( &fs_search );

    fs_removeDirectory( dirname );
}

//--------------------------------------------------------------------------------------------
void fs_copyDirectory( const char *sourceDir, const char *destDir )
{
    /// @details ZZ@> This function copies all files in a directory
    char srcPath[MAX_PATH] = EMPTY_CSTR, destPath[MAX_PATH] = EMPTY_CSTR;

    const char *fileName;
    fs_find_context_t fs_search;

    // List all the files in the directory
    fileName = fs_findFirstFile( sourceDir, NULL, &fs_search );
    if ( NULL != fileName )
    {
        // Make sure the destination directory exists
        fs_createDirectory( destDir );

        while ( NULL != fileName )
        {
            // Ignore files that begin with a .
            if ( '.' != fileName[0] )
            {
                snprintf( srcPath, MAX_PATH, "%s" SLASH_STR "%s", sourceDir, fileName );
                snprintf( destPath, MAX_PATH, "%s" SLASH_STR "%s", destDir, fileName );
                fs_copyFile( srcPath, destPath );
            }

            fileName = fs_findNextFile( &fs_search );
        }
    }

    fs_findClose( &fs_search );
}

//--------------------------------------------------------------------------------------------
int fs_fileExists( const char *filename )
{
    FILE * ptmp;

    int retval = 0;

    ptmp = fopen( filename, "rb" );
    if ( NULL != ptmp )
    {
        fclose( ptmp );
        retval = 1;
    }

    return retval;
}
