/* Egoboo - win-file.c
 * Windows specific filesystem functions.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "egoboo_strutil.h"
#include "egoboo.h"

#include "Log.h"
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>

typedef struct fs_find_info_win32_t
{
  WIN32_FIND_DATA win32_wfdData;
  HANDLE          win32_hFind;
} FS_FIND_INFO_WIN32;

FS_FIND_INFO * fs_find_info_new(FS_FIND_INFO * i)
{
  if(NULL==i) return i;

  i->type = FS_WIN32;
  i->W    = calloc(1, sizeof(FS_FIND_INFO_WIN32));

  return i;
};

// Paths that the game will deal with
char win32_tempPath[MAX_PATH] = {0};
char win32_importPath[MAX_PATH] = {0};
char win32_savePath[MAX_PATH] = {0};
char win32_gamePath[MAX_PATH] = {0};

//---------------------------------------------------------------------------------------------
//File Routines-------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void fs_init()
{
  // JF> This function determines the temporary, import,
  // game data and save paths
  HANDLE hFile;
  char basicdatPath[MAX_PATH];

  log_info( "Initializing filesystem services... " );

  // Put the import path in the user's temporary data directory
  GetTempPath( MAX_PATH, win32_tempPath );
  strncpy( win32_importPath, win32_tempPath, MAX_PATH );
  strncat( win32_importPath, CData.import_dir, MAX_PATH );
  strncat( win32_importPath, SLASH_STRING, MAX_PATH );


  // The save path goes into the user's ApplicationData directory,
  // according to Microsoft's standards.  Will people like this, or
  // should I stick saves someplace easier to find, like My Documents?
  SHGetFolderPath( NULL, CSIDL_PERSONAL, NULL, 0, win32_savePath );
  strncat( win32_savePath, SLASH_STRING "egoboo" SLASH_STRING, MAX_PATH );

  // Last, try and determine where the game data is.  First, try the working
  // directory.  If it's not there, try the directory where the executable
  // is located.
  GetCurrentDirectory( MAX_PATH, win32_gamePath );

  // See if the basicdat directory exists
  strncpy( basicdatPath, win32_gamePath, MAX_PATH );
  strncpy( basicdatPath, CData.basicdat_dir, MAX_PATH );
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
    strncat( basicdatPath, CData.basicdat_dir, MAX_PATH );
    hFile = CreateFile( basicdatPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, 0, NULL );

    if ( hFile == NULL )
    {
      // fatal error here, we can't find the game data.
      log_message( "Failed!\n" );
      log_error( "fs_init: Could not find directory \"%s\"!\n", CData.basicdat_dir );
    }
  }
  CloseHandle( hFile );

  log_message( "Succeeded!\n" );
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
  fileAttrs &= FILE_ATTRIBUTE_DIRECTORY;

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
//Directory Functions--------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
WIN32_FIND_DATA win32_wfdData;
HANDLE win32_hFind;

//---------------------------------------------------------------------------------------------
// Read the first directory entry
const char *fs_findFirstFile( FS_FIND_INFO * i, const char *searchDir, const char *searchBody, const char *searchExtension )
{
  char searchSpec[MAX_PATH];
  size_t len;

  if( NULL == i || FS_WIN32 != i->type ) return NULL;

  strncpy( searchSpec, searchDir, MAX_PATH );
  str_append_slash( searchSpec, MAX_PATH );

  if ( searchBody != NULL )
  {
    _snprintf( searchSpec, MAX_PATH, "%s%s", searchSpec, searchBody );
  }

  if ( searchExtension != NULL )
  {
    _snprintf( searchSpec, MAX_PATH, "%s%s", searchSpec, searchExtension );
  }


  i->W->win32_hFind = FindFirstFile( searchSpec, &(i->W->win32_wfdData) );
  if ( i->W->win32_hFind == INVALID_HANDLE_VALUE )
  {
    return NULL;
  }

  return i->W->win32_wfdData.cFileName;
}

//---------------------------------------------------------------------------------------------
// Read the next directory entry (NULL if done)
const char *fs_findNextFile( FS_FIND_INFO * i )
{
  if(NULL == i || FS_WIN32 != i->type) return NULL;

  if ( i->W->win32_hFind == NULL || i->W->win32_hFind == INVALID_HANDLE_VALUE )
  {
    return NULL;
  }

  if ( !FindNextFile( i->W->win32_hFind, &(i->W->win32_wfdData) ) )
  {
    return NULL;
  }

  return i->W->win32_wfdData.cFileName;
}

//---------------------------------------------------------------------------------------------
// Close anything left open
void fs_findClose(FS_FIND_INFO * i)
{

  if(NULL != i && FS_WIN32 == i->type)
  {
    FindClose( i->W->win32_hFind );
  };

  fs_find_info_delete(i);
}

int DirGetAttrib( char *fromdir )
{
  return GetFileAttributes( fromdir );
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
  _snprintf( searchName, MAX_PATH, "import" SLASH_STRING "*.obj" );
  hFind = FindFirstFile( searchName, &wfdData );
  while ( hFind != NULL && hFind != INVALID_HANDLE_VALUE )
  {
    fileName = wfdData.cFileName;
    // Ignore files that start with a ., like .svn for example.
    if ( fileName[0] != '.' )
    {
      _snprintf( filePath, MAX_PATH, "import" SLASH_STRING "%s", fileName );
      if ( fs_fileIsDirectory( filePath ) )
      {
        fs_removeDirectoryAndContents( filePath );
      }
    }

    if ( !FindNextFile( hFind, &wfdData ) ) break;

  }
  FindClose( hFind );
}
