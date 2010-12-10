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

/// @file egoboo_vfs.h
/// @brief A virtual filesystem for Egoboo.
///
/// @details Almost all filesystem reads and writes should be handled through this interface. The only possible
/// exceptions would be the log file (?) or something similar.
/// Currently, this basically just wraps PhysicsFS functions

#include "file_common.h"
#include <SDL_types.h>

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

#   define VFS_TRUE  (1==1)
#   define VFS_FALSE (!VFS_TRUE)

//--------------------------------------------------------------------------------------------
// TYPEDEFS
//--------------------------------------------------------------------------------------------

    struct s_vfs_search_context;
    typedef struct s_vfs_search_context vfs_search_context_t;

// use this ugly thing, since there is no other way to hide the actual structure of the vfs_FILE...
    struct vfs_FILE;
    typedef struct vfs_FILE vfs_FILE;

//--------------------------------------------------------------------------------------------
// CONSTANTS
//--------------------------------------------------------------------------------------------

/// What type of things are we searching for?
    enum e_vfs_serach_bits
    {
        // file types
        VFS_SEARCH_NONE = 0,                ///< NONE == ALL
        VFS_SEARCH_DIR  = ( 1 << 0 ),
        VFS_SEARCH_FILE = ( 1 << 1 ),

        // search options
        VFS_SEARCH_BARE = ( 1 << 2 ),      ///< return only the bare filename, not the whole relative path

        VFS_SEARCH_ALL  = VFS_SEARCH_DIR | VFS_SEARCH_FILE
    };

/// physfs does not distinguish between these functions
/// but if we change the package we are using, it might care...
#define vfs_delete_directory vfs_delete_file

//--------------------------------------------------------------------------------------------
// FUNCTION PROTOYPES
//--------------------------------------------------------------------------------------------

/// the initlization routing. there is no need to call the de-initialization. That
/// will be called automatically at program termination
    void vfs_init();

/// these functions open in "binary mode" this means that they are reading using
/// physfs and not using the c stdio routines
    vfs_FILE * vfs_openReadB( const char * filename );
    vfs_FILE * vfs_openWriteB( const char * filename );
    vfs_FILE * vfs_openAppendB( const char * filename );

// these functions open in "text mode" this means that they are reading using
// the c stdio routines. we use physfs to resolve the actual filename
    vfs_FILE * vfs_openRead( const char * filename );
    vfs_FILE * vfs_openWrite( const char * filename );
    vfs_FILE * vfs_openAppend( const char * filename );

    int        vfs_close( vfs_FILE * pfile );
    int        vfs_flush( vfs_FILE * pfile );

    int  vfs_eof( vfs_FILE * pfile );
    int  vfs_error( vfs_FILE * pfile );
    long vfs_tell( vfs_FILE * pfile );
    int  vfs_seek( vfs_FILE * pfile , long offset );

    int vfs_mkdir( const char *dirName );
    int vfs_delete_file( const char *filename );

    int vfs_exists( const char *fname );
    int vfs_isDirectory( const char *fname );

// binary reading and writing
    size_t vfs_read( void * buffer, size_t size, size_t count, vfs_FILE * pfile );
    size_t vfs_write( void * buffer, size_t size, size_t count, vfs_FILE * pfile );
    int    vfs_read_Sint16( vfs_FILE * pfile, Sint16 * val );
    int    vfs_read_Uint16( vfs_FILE * pfile, Uint16 * val );
    int    vfs_read_Sint32( vfs_FILE * pfile, Sint32 * val );
    int    vfs_read_Uint32( vfs_FILE * pfile, Uint32 * val );
    int    vfs_read_Sint64( vfs_FILE * pfile, Sint64 * val );
    int    vfs_read_Uint64( vfs_FILE * pfile, Uint64 * val );

/// the file searching routines
    char ** vfs_enumerateFiles( const char * dir_name );
    void    vfs_freeList( void * listVar );

    const char * vfs_search_context_get_current( struct s_vfs_search_context * ctxt );

    vfs_search_context_t * vfs_findFirst( const char * search_path, const char * search_extension, Uint32 search_bits );
    vfs_search_context_t * vfs_findNext( vfs_search_context_t ** pctxt );
    void                   vfs_findClose( vfs_search_context_t ** pctxt );

    long         vfs_fileLength( vfs_FILE * pfile );

    int          vfs_scanf( vfs_FILE * pfile, const char *format, ... );
    int          vfs_printf( vfs_FILE * pfile, const char *format, ... );

    int          vfs_putc( int c, vfs_FILE * pfile );
    int          vfs_getc( vfs_FILE * pfile );
    int          vfs_ungetc( int c, vfs_FILE * pfile );

    void         vfs_empty_temp_directories();

    int          vfs_copyFile( const char *source, const char *dest );
    int          vfs_copyDirectory( const char *sourceDir, const char *destDir );

    int    vfs_ungetc( int, vfs_FILE * );
    int    vfs_getc( vfs_FILE * );
    int    vfs_removeDirectoryAndContents( const char * dirname, int recursive );
    int    vfs_putc( int , vfs_FILE * );
    int    vfs_puts( const char * , vfs_FILE * );
    char * vfs_gets( char *, int, vfs_FILE * );

    const char * vfs_resolveReadFilename( const char * src_filename );
    const char * vfs_resolveWriteFilename( const char * src_filename );

    const char* vfs_getError();
    const char* vfs_getVersion();

    int vfs_add_mount_point( const char * dirname, const char * relative_path, const char * mount_point, int append );
    int vfs_remove_mount_point( const char * mount_point );

    const char * vfs_convert_fname( const char * fname );
    const char * vfs_convert_fname_sys( const char * fname );

    void vfs_set_base_search_paths();
    const char * vfs_mount_info_strip_path( const char * some_path );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egoboo_vfs_h
