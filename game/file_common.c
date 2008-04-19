/* Egoboo - common-file.c
 * File operations that are shared between various operating systems.
 * OS-specific code goes in *-file.c (such as win-file.c)
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

#include "egoboo.h"
#include "Log.h"

#ifndef MAX_PATH
#define MAX_PATH 260 // Same value that Windows uses...
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// FIXME: Doesn't handle deleting directories recursively yet.
void fs_removeDirectoryAndContents( const char *dirname )
{
  // ZZ> This function deletes all files in a directory,
  //     and the directory itself
  char filePath[MAX_PATH];
  const char *fileName;

  // List all the files in the directory
  fileName = fs_findFirstFile( dirname, NULL );
  while ( fileName != NULL )
  {
    // Ignore files that start with a ., like .svn for example.
    if ( fileName[0] != '.' )
    {
      snprintf( filePath, MAX_PATH, "%s/%s", dirname, fileName );
      if ( fs_fileIsDirectory( filePath ) )
      {
        //fs_removeDirectoryAndContents(filePath);
      }
      else
      {
        fs_deleteFile( filePath );
      }
    }

    fileName = fs_findNextFile();
  }
  fs_findClose();
  fs_removeDirectory( dirname );
}

//--------------------------------------------------------------------------------------------
void fs_copyDirectory( const char *sourceDir, const char *destDir )
{
  // ZZ> This function copies all files in a directory
  char srcPath[MAX_PATH], destPath[MAX_PATH];
  const char *fileName;

  // List all the files in the directory
  fileName = fs_findFirstFile( sourceDir, NULL );
  if ( fileName != NULL )
  {
    // Make sure the destination directory exists
    fs_createDirectory( destDir );

    do
    {
      // Ignore files that begin with a .
      if ( fileName[0] != '.' )
      {
        snprintf( srcPath, MAX_PATH, "%s/%s", sourceDir, fileName );
        snprintf( destPath, MAX_PATH, "%s/%s", destDir, fileName );
        fs_copyFile( srcPath, destPath );
      }

      fileName = fs_findNextFile();
    }
    while ( fileName != NULL );
  }
  fs_findClose();
}

//--------------------------------------------------------------------------------------------
FILE * fs_fileOpen( PRIORITY priority, const char * src, const char * fname, const char * mode )
{
  // BB > an alias to the standard fopen() command.  Allows proper logging of
  //      bad calls to fopen()

  FILE * fptmp = NULL;

  if ( NULL == src ) src = "";

  if ( NULL == fname || '\0' == fname[0] )
  {
    log_warning( "%s - fs_fileOpen() - invalid file name\n", src );
  }
  else if ( NULL == mode || '\0' == mode[0] )
  {
    log_warning( "%s - fs_fileOpen() - invalid file mode\n", src );
  }
  else
  {
    fptmp = fopen( fname, mode );

    if ( NULL == fptmp )
    {
      switch ( priority )
      {
        case PRI_WARN:
          log_warning( "%s - fs_fileOpen() - could not open file \"%s\" in mode \"%s\"\n", src, fname, mode );
          break;

        case PRI_FAIL:
          log_error( "%s - fs_fileOpen() - could not open file \"%s\" in mode \"%s\"\n", src, fname, mode );
          break;
      };

    }
    else
    {
      switch ( priority )
      {
        case PRI_WARN:
        case PRI_FAIL:
          log_info( "%s - fs_fileOpen() - successfully opened file \"%s\" in mode \"%s\"\n", src, fname, mode );
          break;
      };

    }
  }

  return fptmp;
};

//--------------------------------------------------------------------------------------------
void fs_fileClose( FILE * pfile )
{
  // BB > an alias to the standard fclose() command.

  if ( NULL == pfile )
  {
    log_warning( "fs_fileClose() - invalid file\n" );
  }
  else
  {
    fclose( pfile );
  };
};
