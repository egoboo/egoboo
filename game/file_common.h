#pragma once

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

//---------------------------------------------------------------------------------------------
// Filesystem functions

int DirGetAttrib( const char *fromdir );

void fs_init();

const char *fs_getBinaryDirectory();
const char *fs_getDataDirectory();
const char *fs_getUserDirectory();
const char *fs_getConfigDirectory();

int  fs_fileExists( const char *filename );
int  fs_fileIsDirectory( const char *filename );
int  fs_createDirectory( const char *dirname );
int  fs_removeDirectory( const char *dirname );
void fs_deleteFile( const char *filename );
void fs_copyFile( const char *source, const char *dest );
void fs_removeDirectoryAndContents( const char *dirname, int recursive );
void fs_copyDirectory( const char *sourceDir, const char *destDir );

// Enumerate directory contents
const char *fs_findFirstFile( const char *path, const char *extension );
const char *fs_findNextFile( void );
void        fs_findClose();
