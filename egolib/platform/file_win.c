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

/// @file platform/file_win.c
/// @brief Windows-specific filesystem functions.
/// @details

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>

#include "../file_common.h"
#include "../log.h"
#include "../strutil.h"
#include "../platform.h"

// this include must be the absolute last include
#include "../mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define HAS_ATTRIBS(ATTRIBS,VAR) ((INVALID_FILE_ATTRIBUTES != (VAR)) && ( (ATTRIBS) == ( (VAR) & (ATTRIBS) ) ))

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_win32_find_context;
typedef struct s_win32_find_context win32_find_context_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern void sys_fs_init( const char * root_dir );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Paths that the game will deal with
static char win32_binaryPath[MAX_PATH]   = EMPTY_CSTR;
static char win32_dataPath[MAX_PATH]     = EMPTY_CSTR;
static char win32_userDataPath[MAX_PATH] = EMPTY_CSTR;
static char win32_configPath[MAX_PATH]   = EMPTY_CSTR;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// File Routines -----------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void sys_fs_init( const char * root_dir )
{
    /// @author JF
    /// @details This function determines the temporary, import,
    /// game data and save paths

    DWORD attrib;
    char currentPath[MAX_PATH] = EMPTY_CSTR;
    char basicdatPath[MAX_PATH] = EMPTY_CSTR;

    printf( "Initializing filesystem services...\n" );

    // The save path goes into the user's ApplicationData directory,
    // according to Microsoft's standards.  Will people like this, or
    // should I stick saves someplace easier to find, like My Documents?
    SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, 0, win32_userDataPath );
    strncat( win32_userDataPath, SLASH_STR "egoboo", MAX_PATH );

    // grab the actual location of the binary
    GetModuleFileName( NULL, win32_binaryPath, MAX_PATH );
    PathRemoveFileSpec( win32_binaryPath );

    // if the root_path is not defined, let the use the starting directory
    if ( INVALID_CSTR( root_dir ) )
    {
        // Last, try and determine where the game data is.  First, try the working
        // directory.  If it's not there, try the directory where the executable
        // is located.
        GetCurrentDirectory( MAX_PATH, currentPath );
    }
    else
    {
        strncpy( currentPath, root_dir, SDL_arraysize( currentPath ) );
    }

    // try to find the basicdat directory in the current directory
    win32_dataPath[0] = CSTR_END;
    snprintf( basicdatPath, MAX_PATH, "%s" SLASH_STR "basicdat", currentPath, MAX_PATH );
    attrib = GetFileAttributes( basicdatPath );
    if ( HAS_ATTRIBS( FILE_ATTRIBUTE_DIRECTORY, attrib ) )
    {
        strncpy( win32_dataPath, currentPath, MAX_PATH );
    }
    else
    {
        // look in the binary directory
        snprintf( basicdatPath, MAX_PATH, "%s" SLASH_STR "basicdat", win32_binaryPath, MAX_PATH );
        attrib = GetFileAttributes( basicdatPath );
        if ( HAS_ATTRIBS( FILE_ATTRIBUTE_DIRECTORY, attrib ) )
        {
            strncpy( win32_dataPath, win32_binaryPath, MAX_PATH );
        }
    }

    if ( CSTR_END == win32_dataPath[0] )
    {
        // fatal error here, we can't find the game data.
        printf( "Could not find basicdat directory!\n" );
        exit( -1 );
    }

    // config path is the same as the data path in win32
    strncpy( win32_configPath, win32_dataPath, SDL_arraysize( win32_configPath ) );

    // the log file cannot be started until there is a user data path to dump the file into
    // so dump this debug info to stdout
    printf( "Game directories are:\n\tBinaries: %s\n\tData: %s\n\tUser Data: %s\n\tConfig Files: %s\n",
            win32_binaryPath, win32_dataPath, win32_userDataPath, win32_configPath );
}

//--------------------------------------------------------------------------------------------
const char *fs_getBinaryDirectory( void )
{
    return win32_binaryPath;
}

//--------------------------------------------------------------------------------------------
const char *fs_getDataDirectory( void )
{
    return win32_dataPath;
}

//--------------------------------------------------------------------------------------------
const char *fs_getUserDirectory( void )
{
    return win32_userDataPath;
}

//--------------------------------------------------------------------------------------------
const char *fs_getConfigDirectory( void )
{
    return win32_configPath;
}

//--------------------------------------------------------------------------------------------
int fs_fileIsDirectory( const char *filename )
{
    // Returns 1 if this filename is a directory
    DWORD fileAttrs;

    if ( INVALID_CSTR( filename ) ) return bfalse;

    fileAttrs = GetFileAttributes( filename );

    return HAS_ATTRIBS( FILE_ATTRIBUTE_DIRECTORY, fileAttrs );
}

//--------------------------------------------------------------------------------------------
// Had to revert back to prog x code to prevent import/skin bug
int fs_createDirectory( const char *dirname )
{
    if ( INVALID_CSTR( dirname ) ) return FALSE;

    return ( 0 != CreateDirectory( dirname, NULL ) );
}

//--------------------------------------------------------------------------------------------
int fs_removeDirectory( const char *dirname )
{
    if ( INVALID_CSTR( dirname ) ) return FALSE;

    return ( 0 != RemoveDirectory( dirname ) );
}

//--------------------------------------------------------------------------------------------
void fs_deleteFile( const char *filename )
{
    /// @author ZZ
    /// @details This function deletes a file

    if ( VALID_CSTR( filename ) )
    {
        DeleteFile( filename );
    }
}

//--------------------------------------------------------------------------------------------
bool_t fs_copyFile( const char *source, const char *dest )
{
    if ( INVALID_CSTR( source ) || INVALID_CSTR( dest ) ) return bfalse;

    return ( TRUE == CopyFile( source, dest, bfalse ) );
}

//--------------------------------------------------------------------------------------------
// Directory Functions--------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_win32_find_context
{
    WIN32_FIND_DATA wfdData;
    HANDLE          hFind;
};

//--------------------------------------------------------------------------------------------
// Read the first directory entry
const char *fs_findFirstFile( const char *searchDir, const char *searchExtension, fs_find_context_t * fs_search )
{
    char searchSpec[MAX_PATH] = EMPTY_CSTR;
    size_t len;
    win32_find_context_t * pcnt;

    if ( INVALID_CSTR( searchDir ) || NULL == fs_search ) return NULL;

    pcnt = EGOBOO_NEW( win32_find_context_t );
    fs_search->type = win32_find;
    fs_search->ptr.w = pcnt;

    len = strlen( searchDir ) + 1;
    if ( C_SLASH_CHR != searchDir[len] || C_BACKSLASH_CHR != searchDir[len] )
    {
        _snprintf( searchSpec, MAX_PATH, "%s" SLASH_STR, searchDir );
    }
    else
    {
        strncpy( searchSpec, searchDir, MAX_PATH );
    }
    if ( NULL != searchExtension )
    {
        _snprintf( searchSpec, MAX_PATH, "%s*.%s", searchSpec, searchExtension );
    }
    else
    {
        strncat( searchSpec, "*", MAX_PATH );
    }

    pcnt->hFind = FindFirstFile( searchSpec, &pcnt->wfdData );
    if ( pcnt->hFind == INVALID_HANDLE_VALUE )
    {
        return NULL;
    }

    return pcnt->wfdData.cFileName;
}

//--------------------------------------------------------------------------------------------
// Read the next directory entry (NULL if done)
const char *fs_findNextFile( fs_find_context_t * fs_search )
{
    win32_find_context_t * pcnt;

    if ( NULL == fs_search || win32_find != fs_search->type ) return NULL;

    pcnt = fs_search->ptr.w;
    if ( NULL == pcnt ) return NULL;

    if ( NULL == pcnt->hFind || INVALID_HANDLE_VALUE == pcnt->hFind )
    {
        return NULL;
    }
    if ( !FindNextFile( pcnt->hFind, &pcnt->wfdData ) )
    {
        return NULL;
    }

    return pcnt->wfdData.cFileName;
}

//--------------------------------------------------------------------------------------------
// Close anything left open
void fs_findClose( fs_find_context_t * fs_search )
{
    win32_find_context_t * pcnt;

    if ( NULL == fs_search || win32_find != fs_search->type ) return;

    pcnt = fs_search->ptr.w;
    if ( NULL == pcnt ) return;

    if ( NULL != pcnt->hFind )
    {
        FindClose( pcnt->hFind );
        pcnt->hFind = NULL;
    }

    EGOBOO_DELETE( pcnt );

    BLANK_STRUCT_PTR( fs_search )
}

int DirGetAttrib( const char *fromdir )
{
    return( GetFileAttributes( fromdir ) );
}
