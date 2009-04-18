#pragma once

//---------------------------------------------------------------------------------------------
// Filesystem functions

int DirGetAttrib(  const char *fromdir );

void fs_init();
const char *fs_getTempDirectory();
const char *fs_getImportDirectory();
const char *fs_getGameDirectory();
const char *fs_getSaveDirectory();
int fs_fileExists( const char *filename );
int fs_fileIsDirectory( const char *filename );
int fs_createDirectory( const char *dirname );
int fs_removeDirectory( const char *dirname );
void fs_deleteFile( const char *filename );
void fs_copyFile( const char *source, const char *dest );
void fs_removeDirectoryAndContents( const char *dirname );
void fs_copyDirectory( const char *sourceDir, const char *destDir );

void empty_import_directory();

// Enumerate directory contents
const char *fs_findFirstFile( const char *path, const char *extension );
const char *fs_findNextFile( void );
void fs_findClose();
