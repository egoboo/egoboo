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

/* Egoboo - win-file.c
 * Windows specific filesystem functions.
 */

#include "file_common.h"
#include "log.h"

#include "egoboo.h"

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
WIN32_FIND_DATA win32_wfdData;
HANDLE win32_hFind;

// Paths that the game will deal with
static char win32_tempPath[MAX_PATH] = {0};
static char win32_importPath[MAX_PATH] = {0};
static char win32_savePath[MAX_PATH] = {0};
static char win32_gamePath[MAX_PATH] = {0};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// File Routines -----------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void fs_init()
{
    // JF> This function determines the temporary, import,
    // game data and save paths
    HANDLE hFile;
    char basicdatPath[MAX_PATH];

    log_info( "Initializing filesystem services...\n" );

    // Put the import path in the user's temporary data directory
    GetTempPath( MAX_PATH, win32_tempPath );
    strncpy( win32_importPath, win32_tempPath, MAX_PATH );
    strncat( win32_importPath, "import" SLASH_STR, MAX_PATH );

    // The save path goes into the user's ApplicationData directory,
    // according to Microsoft's standards.  Will people like this, or
    // should I stick saves someplace easier to find, like My Documents?
    SHGetFolderPath( NULL, CSIDL_PERSONAL, NULL, 0, win32_savePath );
    strncat( win32_savePath, SLASH_STR "egoboo" SLASH_STR, MAX_PATH );

    // Last, try and determine where the game data is.  First, try the working
    // directory.  If it's not there, try the directory where the executable
    // is located.
    GetCurrentDirectory( MAX_PATH, win32_gamePath );

    // See if the basicdat directory exists
    strncpy( basicdatPath, win32_gamePath, MAX_PATH );
    strncpy( basicdatPath, "basicdat", MAX_PATH );
    hFile = CreateFile( basicdatPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, 0, NULL );
    if ( hFile == NULL )
    {
        // didn't find the basicdat directory, give the executable's directory
        // a try next
        GetModuleFileName( NULL, win32_gamePath, MAX_PATH );
        PathRemoveFileSpec( win32_gamePath );

        // See if the basicdat directory exists
        strncpy( basicdatPath, win32_gamePath, MAX_PATH );
        strncpy( basicdatPath, "basicdat", MAX_PATH );
        hFile = CreateFile( basicdatPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                            OPEN_EXISTING, 0, NULL );
        if ( hFile == NULL )
        {
            // fatal error here, we can't find the game data.
            log_error( "Could not find basicdat directory!\n" );
        }
    }

    CloseHandle( hFile );

    log_info( "Game directories are:\n\tGame: %s\n\tTemp: %s\n\tSave: %s\n\tImport: %s\n",
              win32_gamePath, win32_tempPath, win32_savePath, win32_importPath );
}

const char *fs_getTempDirectory()
{
    return win32_tempPath;
}

const char *fs_getImportDirectory()
{
    return win32_importPath;
}

const char *fs_getGameDirectory()
{
    return win32_gamePath;
}

const char *fs_getSaveDirectory()
{
    return win32_savePath;
}

//---------------------------------------------------------------------------------------------
int fs_fileIsDirectory( const char *filename )
{
    // Returns 1 if this filename is a directory
    DWORD fileAttrs;
    fileAttrs = GetFileAttributes( filename );
    fileAttrs = fileAttrs & FILE_ATTRIBUTE_DIRECTORY;

    return fileAttrs;
}

// Had to revert back to prog x code to prevent import/skin bug
int fs_createDirectory( const char *dirname )
{
    return ( CreateDirectory( dirname, NULL ) != 0 );
}

int fs_removeDirectory( const char *dirname )
{
    return ( RemoveDirectory( dirname ) != 0 );
}

//---------------------------------------------------------------------------------------------
void fs_deleteFile( const char *filename )
{
    // ZZ> This function deletes a file
    DeleteFile( filename );
}

void fs_copyFile( const char *source, const char *dest )
{
    CopyFile( source, dest, btrue );
}

//---------------------------------------------------------------------------------------------
// Directory Functions--------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
WIN32_FIND_DATA win32_wfdData;
HANDLE win32_hFind;

//---------------------------------------------------------------------------------------------
// Read the first directory entry
const char *fs_findFirstFile( const char *searchDir, const char *searchExtension )
{
    char searchSpec[MAX_PATH];
    size_t len;

    len = strlen( searchDir ) + 1;
    if ( searchDir[len] != '/' || searchDir[len] != '\\' )
    {
        _snprintf( searchSpec, MAX_PATH, "%s" SLASH_STR, searchDir );
    }
    else
    {
        strncpy( searchSpec, searchDir, MAX_PATH );
    }
    if ( searchExtension != NULL )
    {
        _snprintf( searchSpec, MAX_PATH, "%s*.%s", searchSpec, searchExtension );
    }
    else
    {
        strncat( searchSpec, "*", MAX_PATH );
    }

    win32_hFind = FindFirstFile( searchSpec, &win32_wfdData );
    if ( win32_hFind == INVALID_HANDLE_VALUE )
    {
        return NULL;
    }

    return win32_wfdData.cFileName;
}

//---------------------------------------------------------------------------------------------
// Read the next directory entry (NULL if done)
const char *fs_findNextFile( void )
{
    if ( win32_hFind == NULL || win32_hFind == INVALID_HANDLE_VALUE )
    {
        return NULL;
    }
    if ( !FindNextFile( win32_hFind, &win32_wfdData ) )
    {
        return NULL;
    }

    return win32_wfdData.cFileName;
}

//---------------------------------------------------------------------------------------------
// Close anything left open
void fs_findClose()
{
    FindClose( win32_hFind );
    win32_hFind = NULL;
}

int DirGetAttrib(  const char *fromdir )
{
    return( GetFileAttributes( fromdir ) );
}

//---------------------------------------------------------------------------------------------
void empty_import_directory( void )
{
    // ZZ> This function deletes all the TEMP????.OBJ subdirectories in the IMPORT directory
    WIN32_FIND_DATA wfdData;
    HANDLE hFind;
    char searchName[MAX_PATH];
    char filePath[MAX_PATH];
    char *fileName;

    // List all the files in the directory
    _snprintf( searchName, MAX_PATH, "import" SLASH_STR "*.obj" );
    hFind = FindFirstFile( searchName, &wfdData );

    while ( hFind != NULL && hFind != INVALID_HANDLE_VALUE )
    {
        fileName = wfdData.cFileName;

        // Ignore files that start with a ., like .svn for example.
        if ( fileName[0] != '.' )
        {
            _snprintf( filePath, MAX_PATH, "import" SLASH_STR "%s", fileName );
            if ( fs_fileIsDirectory( filePath ) )
            {
                fs_removeDirectoryAndContents( filePath );
            }
        }
        if ( !FindNextFile( hFind, &wfdData ) ) break;
    }

    FindClose( hFind );
}
