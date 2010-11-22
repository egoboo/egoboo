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

#include "log.h"

#include "egoboo_strutil.h"
#include "egoboo_vfs.h"
#include "egoboo_config.h"

#if !defined(MAX_PATH)
#define MAX_PATH 260  // Same value that Windows uses...
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t _fs_initialized = bfalse;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void sys_fs_init();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void fs_init()
{
    if ( _fs_initialized ) return;

    sys_fs_init();

    _fs_initialized = btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
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

    if ( INVALID_CSTR( filename ) ) return retval;

    ptmp = fopen( filename, "rb" );
    if ( NULL != ptmp )
    {
        fclose( ptmp );
        retval = 1;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
const char * fs_createBinaryDirectoryFilename( const char * relative_pathname )
{
    static char path[1024];
    const char * dir_name_ptr;

    path[0] = CSTR_END;
    if ( !VALID_CSTR( relative_pathname ) ) return path;

    dir_name_ptr = fs_getBinaryDirectory();

    if ( VALID_CSTR( dir_name_ptr ) )
    {
        snprintf( path, SDL_arraysize( path ), "%s" SLASH_STR "%s", dir_name_ptr, relative_pathname );
    }
    else
    {
        snprintf( path, SDL_arraysize( path ), "." SLASH_STR "%s", relative_pathname );
    }

    return path;
}

//--------------------------------------------------------------------------------------------
const char * fs_createDataDirectoryFilename( const char * relative_pathname )
{
    static char path[1024];
    const char * dir_name_ptr;

    path[0] = CSTR_END;
    if ( !VALID_CSTR( relative_pathname ) ) return path;

    dir_name_ptr = fs_getDataDirectory();

    if ( VALID_CSTR( dir_name_ptr ) )
    {
        snprintf( path, SDL_arraysize( path ), "%s" SLASH_STR "%s", dir_name_ptr, relative_pathname );
    }
    else
    {
        snprintf( path, SDL_arraysize( path ), "." SLASH_STR "%s", relative_pathname );
    }

    return path;
}

//--------------------------------------------------------------------------------------------
const char * fs_createUserDirectoryFilename( const char * relative_pathname )
{
    static char path[1024];
    const char * dir_name_ptr;

    path[0] = CSTR_END;
    if ( !VALID_CSTR( relative_pathname ) ) return path;

    dir_name_ptr = fs_getUserDirectory();

    if ( VALID_CSTR( dir_name_ptr ) )
    {
        snprintf( path, SDL_arraysize( path ), "%s" SLASH_STR "%s", dir_name_ptr, relative_pathname );
    }
    else
    {
        snprintf( path, SDL_arraysize( path ), "." SLASH_STR "%s", relative_pathname );
    }

    return path;
}

//--------------------------------------------------------------------------------------------
const char * fs_createConfigDirectoryFilename( const char * relative_pathname )
{
    static char path[1024];
    const char * dir_name_ptr;

    path[0] = CSTR_END;
    if ( !VALID_CSTR( relative_pathname ) ) return path;

    dir_name_ptr = fs_getConfigDirectory();

    if ( VALID_CSTR( dir_name_ptr ) )
    {
        snprintf( path, SDL_arraysize( path ), "%s" SLASH_STR "%s", dir_name_ptr, relative_pathname );
    }
    else
    {
        snprintf( path, SDL_arraysize( path ), "." SLASH_STR "%s", relative_pathname );
    }

    return path;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
FILE * fs_openBinaryDirectoryFile( const char * relative_pathname, const char * mode )
{
    FILE       * file_ptr = NULL;
    const char * path_ptr;

    if ( !VALID_CSTR( relative_pathname ) ) return NULL;

    path_ptr = fs_createBinaryDirectoryFilename( relative_pathname );

    if ( VALID_CSTR( path_ptr ) )
    {
        file_ptr = fopen( path_ptr, mode );
    }
    else
    {
        file_ptr = fopen( relative_pathname, mode );
    }

    return file_ptr;
}

//--------------------------------------------------------------------------------------------
FILE * fs_openDataDirectoryFile( const char * relative_pathname, const char * mode )
{
    FILE       * file_ptr = NULL;
    const char * path_ptr;

    if ( !VALID_CSTR( relative_pathname ) ) return NULL;

    path_ptr = fs_createDataDirectoryFilename( relative_pathname );

    if ( VALID_CSTR( path_ptr ) )
    {
        file_ptr = fopen( path_ptr, mode );
    }
    else
    {
        file_ptr = fopen( relative_pathname, mode );
    }

    return file_ptr;
}

//--------------------------------------------------------------------------------------------
FILE * fs_openUserDirectoryFile( const char * relative_pathname, const char * mode )
{
    FILE       * file_ptr = NULL;
    const char * path_ptr;

    if ( !VALID_CSTR( relative_pathname ) ) return NULL;

    path_ptr = fs_createUserDirectoryFilename( relative_pathname );

    if ( VALID_CSTR( path_ptr ) )
    {
        file_ptr = fopen( path_ptr, mode );
    }
    else
    {
        file_ptr = fopen( relative_pathname, mode );
    }

    return file_ptr;
}

//--------------------------------------------------------------------------------------------
FILE * fs_openConfigDirectoryFile( const char * relative_pathname, const char * mode )
{
    FILE       * file_ptr = NULL;
    const char * path_ptr;

    if ( !VALID_CSTR( relative_pathname ) ) return NULL;

    path_ptr = fs_createConfigDirectoryFilename( relative_pathname );

    if ( VALID_CSTR( path_ptr ) )
    {
        file_ptr = fopen( path_ptr, mode );
    }
    else
    {
        file_ptr = fopen( relative_pathname, mode );
    }

    return file_ptr;
}

//--------------------------------------------------------------------------------------------
bool_t fs_ensureUserFile( const char * relative_filename, bool_t required )
{
    /// @details BB@> if the file does not exist in the user data directory, it is copied from the
    /// data directory. Pass this function a system-dependent pathneme, relative the the root of the
    /// data directory.
    ///
    /// @note we can't use the vfs to do this in win32 because of the dir structure and
    /// the fact that PHYSFS will not add the same directory to 2 different mount points...
    /// seems pretty stupid to me, but there you have it.

    STRING path_str;
    int found;

    snprintf( path_str, SDL_arraysize( path_str ), "%s" SLASH_STR "%s", fs_getUserDirectory(), relative_filename );
    str_convert_slash_sys( path_str, SDL_arraysize( path_str ) );

    found = fs_fileExists( path_str );
    if ( !found )
    {
        STRING src_path_str;

        // copy the file from the Data Directory to the User Directory

        snprintf( src_path_str, SDL_arraysize( src_path_str ), "%s" SLASH_STR "%s", fs_getConfigDirectory(), relative_filename );

        fs_copyFile( src_path_str, path_str );

        found = fs_fileExists( path_str );
    }

    // if it still doesn't exist, we have problems
    if ( !found && required )
    {
        log_error( "Cannot find the file \"%s\".\n", relative_filename );
    }

    return ( 0 != found );
}