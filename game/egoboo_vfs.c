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

/// @file egoboo_vfs.c
/// @brief Implementation of the egoboo virtual file system
/// @details

#include "egoboo_vfs.h"

#include "file_common.h"
#include "log.h"

#include "egoboo_strutil.h"
#include "egoboo_endian.h"
#include "egoboo_fileutil.h"

#include <physfs.h>

#include "file_common.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define VFS_MAX_PATH 1024

#define VFS_TRUE  (1==1)
#define VFS_FALSE (!VFS_TRUE)

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

typedef char VFS_PATH[VFS_MAX_PATH];

#define MAX_MOUNTINFO 128

/// The bits specifying the possible errors
enum e_vfs_error_bits
{
    VFS_EOF   = ( 1 << 0 ),
    VFS_ERROR = ( 1 << 1 )
};

/// What type of file is actually being referenced in u_vfs_fileptr
enum e_vfs_mode
{
    vfs_unknown = 0,
    vfs_cfile,
    vfs_physfs
};
typedef enum e_vfs_mode vfs_mode_t;

/// An anonymized pointer type
union u_vfs_fileptr
{
    void        * u;
    FILE        * c;
    PHYSFS_File * p;
};
typedef union u_vfs_fileptr vfs_fileptr_t;

/// A container holding either a FILE * or a PHYSFS_File *, and translated error states
struct vfs_FILE
{
    Uint32        flags;    // flags for stuff like EGO_ferror() that doesn't clear every time a filesystem call is made
    vfs_mode_t    type;
    vfs_fileptr_t ptr;
};

/// A container for holding all the data for a search
struct s_vfs_search_context
{
    char ** file_list;
    char ** ptr;

    char    path[VFS_MAX_PATH];
    char    ext[255];
    Uint32  bits;
};
typedef struct s_vfs_search_context vfs_search_context_t;

struct s_vfs_path_data
{
    VFS_PATH mount;
    VFS_PATH path;
};
typedef struct s_vfs_path_data vfs_path_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static vfs_search_context_t _vfs_search_context = {NULL, NULL, EMPTY_CSTR};

static int             _vfs_mount_info_count = 0;
static vfs_path_data_t _vfs_mount_info[MAX_MOUNTINFO];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void         _vfs_exit( void );
static const char * _vfs_search( vfs_search_context_t * ctxt );
static int          _vfs_vfscanf( FILE * file, const char * format, va_list args );

static int          _vfs_ensure_write_directory( const char * filename, bool_t is_directory );
static bool_t       _vfs_ensure_destination_file( const char * filename );

static void _vfs_translate_error( vfs_FILE * pfile );

static bool_t _vfs_add_mount_info( const char * mount_point, const char * local_path );
static int    _vfs_mount_point_matches( const char * mount_point, const char * local_path );
static bool_t _vfs_remove_mount_info( int cnt );
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void vfs_init( const char * argv0 )
{
    fs_init();

    PHYSFS_init( argv0 );

    PHYSFS_setWriteDir( fs_getUserDirectory() );

    // add them in this order to make sure the the write directory is searched
    // first
    PHYSFS_addToSearchPath( fs_getUserDirectory(), 0 );
    PHYSFS_addToSearchPath( fs_getConfigDirectory(), 1 );
    PHYSFS_addToSearchPath( fs_getDataDirectory(), 1 );

    // !!!!make sure the basic directories exist.

    if ( !fs_fileIsDirectory( fs_getUserDirectory() ) )
    {
        fs_createDirectory( fs_getUserDirectory() );
    }

    if ( !fs_fileIsDirectory( fs_getUserDirectory() ) )
    {
        printf( "WARNING: Cannot create write directory %s\n", fs_getUserDirectory() );
    }
    else
    {
        char tmp_path[1024] = EMPTY_CSTR;

        snprintf( tmp_path, SDL_arraysize( tmp_path ), "%s" SLASH_STR "debug", fs_getUserDirectory() );

        str_convert_slash_sys( tmp_path, SDL_arraysize( tmp_path ) );
        fs_createDirectory( tmp_path );
    }

    atexit( _vfs_exit );
}

//--------------------------------------------------------------------------------------------
void _vfs_exit()
{
    PHYSFS_deinit();
}

//--------------------------------------------------------------------------------------------
const char* vfs_getVersion()
{
    /// @details ZF@>  returns the current version of the PhysFS library which was used for compiling the binary
    PHYSFS_Version version;
    static STRING buffer = EMPTY_CSTR;

    //PHYSFS_getLinkedVersion(&version);        //Linked version number
    PHYSFS_VERSION( &version );         //Compiled version number
    snprintf( buffer, SDL_arraysize( buffer ), "%d.%d.%d", version.major, version.minor, version.patch );

    return buffer;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
vfs_FILE * vfs_openReadB( const char * filename )
{
    // open a file for reading in binary mode, using PhysFS

    vfs_FILE    * vfs_file;
    PHYSFS_File * ftmp;

    if ( INVALID_CSTR( filename ) ) return NULL;

    // make sure that PHYSFS gets the filename with the slashes it wants
    filename = vfs_resolveReadFilename( filename );

    ftmp = PHYSFS_openRead( filename );
    if ( NULL == ftmp ) return NULL;

    vfs_file = EGOBOO_NEW( vfs_FILE );
    if ( NULL == vfs_file ) return NULL;

    vfs_file->type  = vfs_physfs;
    vfs_file->ptr.p = ftmp;

    return vfs_file;
}

//--------------------------------------------------------------------------------------------
vfs_FILE * vfs_openWriteB( const char * filename )
{
    // open a file for writing in binary mode, using PhysFS

    VFS_PATH      local_filename = EMPTY_CSTR;
    vfs_FILE    * vfs_file;
    PHYSFS_File * ftmp;

    if ( INVALID_CSTR( filename ) ) return NULL;

    // make a local copy of the filename
    // and make sure that PHYSFS gets the local_filename with the slashes it wants
    strncpy( local_filename, vfs_convert_fname( filename ), SDL_arraysize( local_filename ) );

    // make sure that the output directory exists
    if ( !_vfs_ensure_write_directory( local_filename, bfalse ) ) return NULL;

    ftmp = PHYSFS_openRead( local_filename );
    if ( NULL == ftmp ) return NULL;

    vfs_file = EGOBOO_NEW( vfs_FILE );
    if ( NULL == vfs_file ) return NULL;

    vfs_file->type  = vfs_physfs;
    vfs_file->ptr.p = ftmp;

    return vfs_file;
}

//--------------------------------------------------------------------------------------------
vfs_FILE * vfs_openAppendB( const char * filename )
{
    // open a file for appending in binary mode, using PhysFS

    vfs_FILE    * vfs_file;
    PHYSFS_File * ftmp;

    if ( INVALID_CSTR( filename ) ) return NULL;

    // make sure that the destination directory exists, and that a data is copied
    // from the source file in the read path, if necessary
    if ( !_vfs_ensure_destination_file( filename ) ) return NULL;

    ftmp = PHYSFS_openAppend( filename );
    if ( NULL == ftmp ) return NULL;

    vfs_file = EGOBOO_NEW( vfs_FILE );
    if ( NULL == vfs_file ) return NULL;

    vfs_file->type  = vfs_physfs;
    vfs_file->ptr.p = ftmp;

    return vfs_file;
}

//--------------------------------------------------------------------------------------------
const char * vfs_convert_fname_sys( const char * fname )
{
    // PHYSFS and this vfs use a "/" at the front of relative filenames. if this is not removed
    // when converting to system dependent filenames, it will reference the filename relative to the
    // root point, rather than relative to the current directory.

    size_t          offset;
    VFS_PATH        copy_fname  = EMPTY_CSTR;
    static VFS_PATH local_fname = EMPTY_CSTR;

    // test for a bad iput filename
    if ( INVALID_CSTR( fname ) )
    {
        strncpy( local_fname, SLASH_STR, SDL_arraysize( local_fname ) );
        return local_fname;
    }

    // make a copy of the original filename, in case fname is
    // actualy a pointer to local_fname
    strncpy( copy_fname, fname, SDL_arraysize( copy_fname ) );

    offset = 0;
    if ( '.' == copy_fname[0] && '.' == copy_fname[1] )
    {
        offset++;
    }

    while ( '/' == copy_fname[offset] || '\\' == copy_fname[offset] && offset < SDL_arraysize( copy_fname ) )
    {
        offset++;
    }

    // copy the proper relative filename
    strncpy( local_fname, copy_fname + offset, SDL_arraysize( local_fname ) );

    return str_convert_slash_sys( local_fname, strlen( local_fname ) );
}

//--------------------------------------------------------------------------------------------
const char * vfs_convert_fname( const char * fname )
{
    VFS_PATH        copy_fname  = EMPTY_CSTR;
    static VFS_PATH local_fname = EMPTY_CSTR;

    // test for a bad iput filename
    if ( INVALID_CSTR( fname ) )
    {
        strncpy( local_fname, NET_SLASH_STR, SDL_arraysize( local_fname ) );
        return local_fname;
    }

    // make a copy of the original filename, in case fname is
    // actualy a pointer to local_fname
    strncpy( copy_fname, fname, SDL_arraysize( copy_fname ) );

    if ( '/' == copy_fname[0] || '\\' == copy_fname[0] )
    {
        snprintf( local_fname, SDL_arraysize( local_fname ), "%s", copy_fname );
    }
    else
    {
        snprintf( local_fname, SDL_arraysize( local_fname ), NET_SLASH_STR "%s", copy_fname );
    }

    return str_convert_slash_net( local_fname, strlen( local_fname ) );
}

//--------------------------------------------------------------------------------------------
const char * _vfs_strip_mount_point( const char * some_path )
{
    int cnt;
    size_t offset;
    const char * ptmp, * stripped_pos;

    stripped_pos = some_path;

    // strip any starting slashes
    for ( ptmp = some_path; ( '\0' != *ptmp ) && ptmp < some_path + VFS_MAX_PATH; ptmp++ )
    {
        if ( '/' != *ptmp && '\\' != *ptmp )
        {
            break;
        }
    }
    some_path = ptmp;

    ptmp   = NULL;
    offset = 0;
    for ( cnt = 0; cnt < _vfs_mount_info_count; cnt++ )
    {
        offset = strlen( _vfs_mount_info[cnt].mount );
        if ( offset <= 0 ) continue;

        if ( 0 == strncmp( some_path, _vfs_mount_info[cnt].mount, offset ) )
        {
            stripped_pos = some_path + offset;
            break;
        }
    }

    return stripped_pos;

}

//--------------------------------------------------------------------------------------------
const char * _vfs_potential_mount_point( const char * some_path, const char ** pstripped_pos )
{
    // This helper function was devised to read the first potential directory name
    // from the given path. Because:
    //
    // 1) To use the PHYSFS_getMountPoint() function, the mount point you are testing
    //    must be a system-dependent pathname, ending with a system-dependent path
    //    separator.
    //
    // 2) if you use PHYSFS_getRealDir() to give you a path to a file, what it really does is
    //    resolve the mount point, rather than the whole path name (path name without the
    //    file, that is). To do the job we want in vfs_resolveReadFilename(), we need to be able
    //    to strip the mount point off of the path that we were given before prepending the
    //    path returned by PHYSFS_getRealDir()

    const char * ptmp, *path_begin, *path_end;
    static VFS_PATH found_path;

    found_path[0] = '\0';

    if ( !VALID_CSTR( some_path ) ) return found_path;

    ptmp = some_path;

    path_begin = some_path;
    path_end   = some_path + VFS_MAX_PATH - 1;

    // strip any starting slashes
    for ( ptmp = path_begin; ptmp < path_end; ptmp++ )
    {
        if ( '/' != *ptmp && '\\' != *ptmp )
        {
            path_begin = ptmp;
            break;
        }
    }

    // identify the slash after the mount point
    ptmp = strpbrk( path_begin, "/\\" );
    if ( NULL != ptmp ) path_end = ptmp + 1;

    // make sre we do not have an empty string
    if ( path_end > path_begin )
    {
        size_t count = path_end - path_begin;
        strncpy( found_path, path_begin, count );
        found_path[count] = '\0';
    }

    // export the beginning of the string after the mount point, is poddible
    if ( NULL != pstripped_pos )
    {
        *pstripped_pos = path_end;
    }

    // return the potential mount point in system-dependent format
    return vfs_convert_fname_sys( found_path );
}

//--------------------------------------------------------------------------------------------
const char * vfs_resolveReadFilename( const char * src_filename )
{
    static STRING read_name_str = EMPTY_CSTR;
    VFS_PATH      loc_fname;
    int           retval_len = 0;
    const char   *retval = NULL;

    if ( INVALID_CSTR( src_filename ) ) return NULL;

    // make a copy of the local filename
    // and make sure that PHYSFS gets the filename with the slashes it wants
    strncpy( loc_fname, vfs_convert_fname( src_filename ), SDL_arraysize( loc_fname ) );

    retval = NULL;
    retval_len = 0;
    if ( PHYSFS_isDirectory( loc_fname ) )
    {
        retval = PHYSFS_getRealDir( loc_fname );

        if ( VALID_CSTR( retval ) )
        {
            snprintf( read_name_str, SDL_arraysize( read_name_str ), "%s" SLASH_STR "%s", retval, loc_fname );
            retval     = read_name_str;
            retval_len = SDL_arraysize( read_name_str );
        }
    }
    else
    {
        const char * tmp_dirnane;
        const char * ptmp = loc_fname;

        // make PHYSFS grab the actual directory
        tmp_dirnane = PHYSFS_getRealDir( loc_fname );

        if ( INVALID_CSTR( tmp_dirnane ) )
        {
            // not found... just punt
            strncpy( read_name_str, loc_fname, SDL_arraysize( read_name_str ) );
            retval     = read_name_str;
            retval_len = SDL_arraysize( read_name_str );
        }
        else
        {
            ptmp = _vfs_strip_mount_point( loc_fname );

            snprintf( read_name_str, SDL_arraysize( read_name_str ), "%s" SLASH_STR "%s", tmp_dirnane, ptmp );
            retval     = read_name_str;
            retval_len = SDL_arraysize( read_name_str );
        }
    }

    if ( VALID_CSTR( retval ) && retval_len > 0 )
    {
        retval = str_convert_slash_sys(( char* )retval, retval_len );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char * vfs_resolveWriteFilename( const char * src_filename )
{
    static VFS_PATH  szFname = EMPTY_CSTR;
    const  char    * write_dir;

    if ( INVALID_CSTR( src_filename ) ) return szFname;

    write_dir = PHYSFS_getWriteDir();
    if ( NULL == write_dir )
    {
        log_warning( "PhysFS could not get write directory!\n" );
        return NULL;
    }

    // append the write_dir to the src_filename to get the total path
    snprintf( szFname, SDL_arraysize( szFname ), "%s%s", write_dir, src_filename );

    // make sure that the slashes are correct for this system, and that they are not doubled

    return str_convert_slash_sys( szFname, SDL_arraysize( szFname ) );
}

//--------------------------------------------------------------------------------------------
vfs_FILE * vfs_openRead( const char * filename )
{
    // open a file for reading in text mode, using c stdio

    const char  * real_filename;
    vfs_FILE    * vfs_file;
    FILE        * ftmp;

    parse_filename = "";

    real_filename = vfs_resolveReadFilename( filename );
    if ( NULL == real_filename ) return NULL;

    ftmp = EGO_fopen( real_filename, "r" );
    if ( NULL == ftmp ) return NULL;

    vfs_file = EGOBOO_NEW( vfs_FILE );
    if ( NULL == vfs_file ) return NULL;

    parse_filename = filename;

    vfs_file->type  = vfs_cfile;
    vfs_file->ptr.c = ftmp;

    return vfs_file;
}

//--------------------------------------------------------------------------------------------
int _vfs_ensure_write_directory( const char * filename, bool_t is_directory )
{
    /// @details BB@>

    int           retval;
    VFS_PATH      temp_dirname = EMPTY_CSTR;
    char        * tmpstr;

    if ( INVALID_CSTR( filename ) ) return 0;

    // make a working copy of the filename
    // and make sure that PHYSFS gets the filename with the slashes it wants
    strncpy( temp_dirname, vfs_convert_fname( filename ), SDL_arraysize( temp_dirname ) );

    // grab the system-independent path relative to the write directory
    if ( !is_directory && !vfs_isDirectory( temp_dirname ) )
    {
        tmpstr = strrchr( temp_dirname, NET_SLASH_CHR );
        if ( NULL == tmpstr )
        {
            strncpy( temp_dirname, NET_SLASH_STR, SDL_arraysize( temp_dirname ) );
        }
        else
        {
            *tmpstr = CSTR_END;
        }
    }

    // call mkdir() on this directory. PHYSFS will automatically generate the
    // directories needed between the write directory and the specified directory
    retval = 1;
    if ( VALID_CSTR( temp_dirname ) )
    {
        retval = vfs_mkdir( temp_dirname );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
vfs_FILE * vfs_openWrite( const char * filename )
{
    // open a file for writing in text mode,  using c stdio

    VFS_PATH      local_filename = EMPTY_CSTR;
    const char  * real_filename;
    vfs_FILE    * vfs_file;
    FILE        * ftmp;

    if ( INVALID_CSTR( filename ) ) return NULL;

    // make a local copy of the filename
    // and make sure that PHYSFS gets the filename with the slashes it wants
    strncpy( local_filename, vfs_convert_fname( filename ), SDL_arraysize( local_filename ) );

    // make sure that the output directory exists
    if ( !_vfs_ensure_write_directory( local_filename, bfalse ) ) return NULL;

    // get the system-dependent filename
    real_filename = vfs_resolveWriteFilename( filename );
    if ( NULL == real_filename ) return NULL;

    ftmp = EGO_fopen( real_filename, "w" );
    if ( NULL == ftmp ) return NULL;

    vfs_file = EGOBOO_NEW( vfs_FILE );
    if ( NULL == vfs_file ) return NULL;

    vfs_file->type  = vfs_cfile;
    vfs_file->ptr.c = ftmp;

    return vfs_file;
}

//--------------------------------------------------------------------------------------------
bool_t _vfs_ensure_destination_file( const char * filename )
{
    /// @details BB@> make sure that a copy of filename from the read path exists in
    ///     the write directory, but do not overwrite any existing file

    VFS_PATH      local_filename = EMPTY_CSTR;
    const char  * sys_src_name, * sys_dst_name;
    bool_t        read_exists, write_exists;

    if ( INVALID_CSTR( filename ) ) return bfalse;

    // make a local copy of the filename
    // and make sure that PHYSFS gets the filename with the slashes it wants
    strncpy( local_filename, vfs_convert_fname( filename ), SDL_arraysize( local_filename ) );

    // make sure that the output directory exists
    if ( !_vfs_ensure_write_directory( local_filename, bfalse ) ) return bfalse;

    // be a bit carefil here, in case the file exists in the read path and not in the write
    // directory

    sys_src_name  = vfs_resolveReadFilename( local_filename );
    read_exists   = fs_fileExists( sys_src_name );

    sys_dst_name  = vfs_resolveWriteFilename( local_filename );
    write_exists  = fs_fileExists( sys_dst_name );

    if ( read_exists && !write_exists )
    {
        // read exists but write does not exist.
        // copy the read file to the write file and then append
        fs_copyFile( sys_src_name, sys_dst_name );

        write_exists  = fs_fileExists( sys_dst_name );
    }

    return write_exists;
}

//--------------------------------------------------------------------------------------------
vfs_FILE * vfs_openAppend( const char * filename )
{
    // open a file for appending in text mode,  using c stdio

    vfs_FILE    * vfs_file;
    FILE        * ftmp;
    const char  * sys_dst_name;

    if ( INVALID_CSTR( filename ) ) return NULL;

    // make sure that the destination directory exists, and that a data is copied
    // from the source file in the read path, if necessary
    if ( !_vfs_ensure_destination_file( filename ) ) return NULL;

    sys_dst_name  = vfs_resolveWriteFilename( filename );
    if ( INVALID_CSTR( sys_dst_name ) ) return NULL;

    // now open the file for append normally
    ftmp = EGO_fopen( sys_dst_name, "a+" );
    if ( NULL == ftmp ) return NULL;

    vfs_file = EGOBOO_NEW( vfs_FILE );
    if ( NULL == vfs_file ) return NULL;

    vfs_file->type  = vfs_cfile;
    vfs_file->ptr.c = ftmp;

    return vfs_file;
}

//--------------------------------------------------------------------------------------------
int vfs_close( vfs_FILE * pfile )
{
    // close a file, and git rid of the allocated file descriptor

    int retval;

    if ( NULL == pfile ) return 0;

    parse_filename = "";

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        retval = EGO_fclose( pfile->ptr.c );
        memset( pfile, 0, sizeof( *pfile ) );

        free( pfile );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = PHYSFS_close( pfile->ptr.p );
        memset( pfile, 0, sizeof( *pfile ) );

        free( pfile );
    }
    else
    {
        // corrupted data?
        fprintf( stderr, "Tried to deallocate an invalid vfs file descriptor\n" );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_flush( vfs_FILE * pfile )
{
    int retval;

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        retval = EGO_fflush( pfile->ptr.c );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = PHYSFS_flush( pfile->ptr.p );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int vfs_eof( vfs_FILE * pfile )
{
    int retval;

    if ( NULL == pfile ) return 0;

    // check our own end-of-file condition
    if ( 0 != ( pfile->flags & VFS_EOF ) )
    {
        return pfile->flags & VFS_EOF;
    }

    retval = 1;
    if ( vfs_cfile == pfile->type )
    {
        retval = EGO_feof( pfile->ptr.c );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = PHYSFS_eof( pfile->ptr.p );
    }

    if ( 0 != retval )
    {
        pfile->flags |= VFS_EOF;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_error( vfs_FILE * pfile )
{
    int retval;

    if ( NULL == pfile ) return 0;

    retval = 1;
    if ( vfs_cfile == pfile->type )
    {
        retval = EGO_ferror( pfile->ptr.c );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = ( NULL != PHYSFS_getLastError() );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
long vfs_tell( vfs_FILE * pfile )
{
    long retval;

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        retval = EGO_ftell( pfile->ptr.c );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = PHYSFS_tell( pfile->ptr.p );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_seek( vfs_FILE * pfile, long offset )
{
    int retval;

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        // reset the flags
        pfile->flags = 0;

        // !!!! since we are opening non-binary files in text mode, EGO_fseek might act strangely !!!!
        retval = EGO_fseek( pfile->ptr.c, offset, SEEK_SET );
    }
    else if ( vfs_physfs == pfile->type )
    {
        // reset the flags
        pfile->flags = 0;

        retval = PHYSFS_seek( pfile->ptr.p, offset );
    }

    if ( 0 != offset )
    {
        // set an eof flag if we set it to seek past the end of the file
        vfs_eof( pfile );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
long vfs_fileLength( vfs_FILE * pfile )
{
    long retval;

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        // do a little dance with the file pointer to figure out the file length

        long pos = EGO_ftell( pfile->ptr.c );

        EGO_fseek( pfile->ptr.c, 0, SEEK_END );
        retval = EGO_ftell( pfile->ptr.c );

        EGO_fseek( pfile->ptr.c, pos, SEEK_SET );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = PHYSFS_fileLength( pfile->ptr.p );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int vfs_mkdir( const char *dirName )
{
    int retval = PHYSFS_mkdir( vfs_convert_fname( dirName ) );

    if ( !retval )
    {
        log_debug( "vfs_copyDirectory() - Could not create new folder folder \"%s\". (%s)\n", dirName, vfs_getError() );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_delete_file( const char *filename )
{
    return PHYSFS_delete( vfs_convert_fname( filename ) );
}

//--------------------------------------------------------------------------------------------
int vfs_exists( const char *fname )
{
    return PHYSFS_exists( vfs_convert_fname( fname ) );
}

//--------------------------------------------------------------------------------------------
int vfs_isDirectory( const char *fname )
{
    return PHYSFS_isDirectory( vfs_convert_fname( fname ) );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
size_t vfs_read( void * buffer, size_t size, size_t count, vfs_FILE * pfile )
{
    bool_t error = bfalse;
    size_t retval;

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        retval = EGO_fread( buffer, size, count, pfile->ptr.c );
        error = ( retval != size );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = PHYSFS_read( pfile->ptr.p, buffer, size, count );

        if ( retval < 0 ) pfile->flags |= VFS_ERROR;
        error = ( retval != size );
    }

    if ( error ) _vfs_translate_error( pfile );

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t vfs_write( void * buffer, size_t size, size_t count, vfs_FILE * pfile )
{
    size_t retval;

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        retval = EGO_fwrite( buffer, size, count, pfile->ptr.c );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = PHYSFS_write( pfile->ptr.p, buffer, size, count );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Sint16( vfs_FILE * pfile, Sint16 * val )
{
    int retval;
    bool_t error = bfalse;

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        Uint16 itmp;
        retval = EGO_fread( &itmp, 1, sizeof( Uint16 ), pfile->ptr.c );

        error = ( 1 != retval );

        *val = ENDIAN_INT16( itmp );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = PHYSFS_readSLE16( pfile->ptr.p, val );

        error = ( 0 == retval );
    }

    if ( error ) _vfs_translate_error( pfile );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Uint16( vfs_FILE * pfile, Uint16 * val )
{
    bool_t error = bfalse;
    int retval;

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        Uint16 itmp;
        retval = EGO_fread( &itmp, 1, sizeof( Uint16 ), pfile->ptr.c );

        error = ( 1 != retval );

        *val = ENDIAN_INT16( itmp );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = PHYSFS_readULE16( pfile->ptr.p, val );

        error = ( 0 == retval );
    }

    if ( error ) _vfs_translate_error( pfile );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Sint32( vfs_FILE * pfile, Sint32 * val )
{
    int retval;
    bool_t error = bfalse;

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        Uint32 itmp;
        retval = EGO_fread( &itmp, 1, sizeof( Uint32 ), pfile->ptr.c );

        error = ( 1 != retval );

        *val = ENDIAN_INT32( itmp );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = PHYSFS_readSLE32( pfile->ptr.p, val );

        error = ( 0 == retval );
    }

    if ( error ) _vfs_translate_error( pfile );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Uint32( vfs_FILE * pfile, Uint32 * val )
{
    int retval;
    bool_t error = bfalse;

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        Uint32 itmp;
        retval = EGO_fread( &itmp, 1, sizeof( Uint32 ), pfile->ptr.c );

        error = ( 1 != retval );

        *val = ENDIAN_INT32( itmp );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = PHYSFS_readULE32( pfile->ptr.p, val );

        error = ( 0 == retval );
    }

    if ( error ) _vfs_translate_error( pfile );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Sint64( vfs_FILE * pfile, Sint64 * val )
{
    int retval;
    bool_t error = bfalse;

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        Uint64 itmp;
        retval = EGO_fread( &itmp, 1, sizeof( Uint64 ), pfile->ptr.c );

        error = ( 1 != retval );

        *val = ENDIAN_INT64( itmp );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = PHYSFS_readSLE64( pfile->ptr.p, val );

        error = ( 0 == retval );
    }

    if ( error ) _vfs_translate_error( pfile );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Uint64( vfs_FILE * pfile, Uint64 * val )
{
    int retval;
    bool_t error = bfalse;

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        Uint64 itmp;
        retval = EGO_fread( &itmp, 1, sizeof( Uint64 ), pfile->ptr.c );

        error = ( 1 != retval );

        *val = ENDIAN_INT64( itmp );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = PHYSFS_readULE64( pfile->ptr.p, val );

        error = ( 0 == retval );
    }

    if ( error ) _vfs_translate_error( pfile );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_float( vfs_FILE * pfile, float * val )
{
    int retval;
    bool_t error = bfalse;

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        float ftmp;
        retval = EGO_fread( &ftmp, 1, sizeof( float ), pfile->ptr.c );

        error = ( 1 != retval );

        *val = ENDIAN_FLOAT( ftmp );
    }
    else if ( vfs_physfs == pfile->type )
    {
        union { float f; Uint32 i; } convert;
        retval = PHYSFS_readULE32( pfile->ptr.p, &( convert.i ) );

        error = ( 0 == retval );

        *val = convert.f;
    }

    if ( error ) _vfs_translate_error( pfile );

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//int fake_physfs_vscanf( PHYSFS_File * pfile, const char *format, va_list args )
//{
//    // UGH! Just break the format code into pieces and call fscanf on each piece
//
//    char   sub_format[256] = EMPTY_CSTR;
//    char * format_end, * format_next;
//    int    argcount = 0;
//    void * ptr;
//
//    if( NULL == file || INVALID_CSTR(format) ) return 0;
//
//    format_end = (char *)(format + strlen(format));
//
//    // scan throuh the format string looking for formats
//    argcount = 0;
//    while( format < format_end )
//    {
//        bool_t found_format;
//        char * format_tmp;
//
//        // find everything up to the first valid format code in the format string
//        found_format = bfalse;
//        format_tmp   = (char *)format;
//        format_next  = format_tmp;
//        while( format_next < format_end )
//        {
//            format_next = strchr( format_tmp, '%' );
//
//            // handle the occurrence of "%%"
//            if( '%' == *(format_next + 1) )
//            {
//                format_tmp = format_next + 1;
//            }
//            else
//            {
//                found_format = btrue;
//                break;
//            }
//        }
//
//        // copy the format string fragment
//        if( found_format && format_next < format_end )
//        {
//            // scan the valid format code
//            format_next += strcspn( format_next, "cCsSdioxXnueEfgG" ) + 1;
//        }
//        strncpy( sub_format, format, format_next - format );
//
//        // get a pointer to the variable to be filled
//        ptr = NULL;
//        if( found_format )
//        {
//            ptr = va_arg( args, void * );
//        }
//
//        // do the call to fscanf()
//        if( NULL == ptr )
//        {
//            PHYSFS_scanf( file, sub_format );
//        }
//        else
//        {
//            argcount += PHYSFS_scanf( file, sub_format, ptr );
//        }
//
//        format = format_next;
//    }
//
//    return argcount;
//}
//

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int fake_physfs_vprintf( PHYSFS_File * pfile, const char *format, va_list args )
{
    // fake an actual streaming write to the file by writing the string to a
    // "large" buffer

    int written;
    char buffer[4098] = EMPTY_CSTR;

    if ( NULL == pfile || INVALID_CSTR( format ) ) return 0;

    written = EGO_vsnprintf( buffer, SDL_arraysize( buffer ), format, args );

    if ( written > 0 )
    {
        written = PHYSFS_write( pfile, buffer, sizeof( char ), written );
    }

    return written;
}

//--------------------------------------------------------------------------------------------
int vfs_printf( vfs_FILE * pfile, const char *format, ... )
{
    va_list args;
    int retval;

    if ( NULL == pfile ) return 0;

    retval = 0;
    va_start( args, format );
    if ( vfs_cfile == pfile->type )
    {
        retval = EGO_vfprintf( pfile->ptr.c, format, args );
    }
    else
    {
        retval = fake_physfs_vprintf( pfile->ptr.p, format, args );
    }
    va_end( args );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_scanf( vfs_FILE * pfile, const char *format, ... )
{
    va_list args;
    int retval;

    if ( NULL == pfile ) return 0;

    retval = 0;
    va_start( args, format );
    if ( vfs_cfile == pfile->type )
    {
        retval = _vfs_vfscanf( pfile->ptr.c, format, args );
    }
    //else if( vfs_physfs == pfile->type )
    //{
    //    retval = fake_physfs_vscanf( pfile->ptr.p, format, args );
    //}
    va_end( args );

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
char ** vfs_enumerateFiles( const char * dir_name )
{
    return PHYSFS_enumerateFiles( vfs_convert_fname( dir_name ) );
}

//--------------------------------------------------------------------------------------------
void    vfs_freeList( void * listVar )
{
    PHYSFS_freeList( listVar );
}

//--------------------------------------------------------------------------------------------
void _vfs_findClose( vfs_search_context_t * ctxt )
{
    if ( NULL == ctxt ) return;

    if ( NULL != ctxt->file_list )
    {
        PHYSFS_freeList( ctxt->file_list );
        ctxt->file_list = NULL;
    }
    ctxt->ptr       = NULL;
}

//--------------------------------------------------------------------------------------------
const char * _vfs_search( vfs_search_context_t * ctxt )
{
    const char * retval = NULL;
    static VFS_PATH  path_buffer = EMPTY_CSTR;

    // uninitialized file list?
    if ( NULL == ctxt || NULL == ctxt->file_list )
    {
        return NULL;
    }

    // emptry file list?
    if ( NULL == *( ctxt->file_list ) )
    {
        goto _vfs_search_file_error;
    }

    if ( NULL == ctxt->ptr )
    {
        // if we haven't begun the search yet, get started
        ctxt->ptr = ctxt->file_list;
    }
    else
    {
        ctxt->ptr++;
    }

    // NULL == *(ctxt->ptr) signals the end of the list
    // if we exhausted the list, reset everything
    if ( NULL == ctxt->ptr || NULL == *( ctxt->ptr ) )
    {
        goto _vfs_search_file_error;
    }

    // search for the correct extension (if any)
    retval = NULL;
    if ( CSTR_END == *ctxt->ext )
    {
        int  found;

        for ( /* nothing */; NULL != *( ctxt->ptr ); ctxt->ptr++ )
        {
            int is_dir;
            char * loc_path;

            if ( INVALID_CSTR( ctxt->path ) )
            {
                snprintf( path_buffer, SDL_arraysize( path_buffer ), NET_SLASH_STR "%s", *( ctxt->ptr ) );
            }
            else
            {
                snprintf( path_buffer, SDL_arraysize( path_buffer ), "%s" NET_SLASH_STR "%s", ctxt->path, *( ctxt->ptr ) );
            }

            loc_path = ( char * )vfs_convert_fname( path_buffer );

            // have we found the correct type of object?
            found  = VFS_FALSE;
            is_dir = vfs_isDirectory( loc_path );

            if ( 0 != ( VFS_SEARCH_FILE & ctxt->bits ) )
            {
                found = !is_dir;
            }
            else if ( 0 != ( VFS_SEARCH_DIR & ctxt->bits ) )
            {
                found = is_dir;
            }
            else
            {
                found = VFS_TRUE;
            }

            if ( found )
            {
                retval = loc_path;
                break;
            }
        }
    }
    else
    {
        int extension_length = strlen( ctxt->ext );

        // scan through the list
        for ( /* nothing */; NULL != *( ctxt->ptr ); ctxt->ptr++ )
        {
            int found, is_dir;
            int    string_length;
            char * sztest;
            char * loc_path;

            //---- have we found the correct type of object?

            if ( INVALID_CSTR( ctxt->path ) )
            {
                snprintf( path_buffer, SDL_arraysize( path_buffer ), NET_SLASH_STR "%s", *( ctxt->ptr ) );
            }
            else
            {
                snprintf( path_buffer, SDL_arraysize( path_buffer ), "%s" NET_SLASH_STR "%s", ctxt->path, *( ctxt->ptr ) );
            }

            loc_path = ( char * )vfs_convert_fname( path_buffer );

            found = VFS_FALSE;
            is_dir = vfs_isDirectory( loc_path );
            if ( 0 != ( VFS_SEARCH_FILE & ctxt->bits ) )
            {
                found = !is_dir;
            }
            else if ( 0 != ( VFS_SEARCH_DIR & ctxt->bits ) )
            {
                found = is_dir;
            }
            else
            {
                found = VFS_TRUE;
            }

            if ( !found ) continue;

            //---- does the extension match?
            sztest = *( ctxt->ptr );

            // get the length
            string_length = strlen( sztest );

            // grab the last bit of the test string
            if ( string_length - extension_length >= 0 )
            {
                sztest += ( string_length - extension_length );
            }
            else
            {
                sztest = NULL;
            }
            if ( INVALID_CSTR( sztest ) ) continue;

            if ( 0 == strcmp( sztest, ctxt->ext ) )
            {
                retval = loc_path;
                break;
            }
        };
    }

    // reset the path buffer
    path_buffer[0] = CSTR_END;

    // test for the end condition again
    if ( NULL == ctxt->ptr || NULL == *( ctxt->ptr ) )
    {
        vfs_findClose();
        retval = NULL;
    }
    else
    {
        if ( 0 != ( VFS_SEARCH_BARE & ctxt->bits ) )
        {
            // do the "bare" option
            retval = NULL;
            if ( VALID_CSTR( *( ctxt->ptr ) ) )
            {
                strncpy( path_buffer, *( ctxt->ptr ), SDL_arraysize( path_buffer ) );
                retval = path_buffer;
            }
        }
        else
        {
            // return the full path
            if ( VALID_CSTR( retval ) )
            {
                strncpy( path_buffer, retval, SDL_arraysize( path_buffer ) );
                retval = path_buffer;
            }
            else
            {
                retval = NULL;
            }
        }
    }

    return ( NULL == retval ) ? NULL : path_buffer;

_vfs_search_file_error:
    vfs_findClose();
    return NULL;
}

//--------------------------------------------------------------------------------------------
const char * vfs_findFirst( const char * search_path, const char * search_extension, Uint32 search_bits )
{
    // clear out any old context
    vfs_findClose();

    // grab all the files
    _vfs_search_context.file_list = vfs_enumerateFiles( vfs_convert_fname( search_path ) );
    _vfs_search_context.ptr       = NULL;

    // no search list generated
    if ( NULL == _vfs_search_context.file_list )
    {
        return NULL;
    }

    // empty search list
    if ( NULL == *( _vfs_search_context.file_list ) )
    {
        vfs_findClose();
        return NULL;
    }

    // set the search extension
    if ( INVALID_CSTR( search_extension ) )
    {
        _vfs_search_context.ext[0] = CSTR_END;
    }
    else
    {
        snprintf( _vfs_search_context.ext, SDL_arraysize( _vfs_search_context.ext ), ".%s", search_extension );
    }

    // set the search path
    if ( INVALID_CSTR( search_path ) )
    {
        _vfs_search_context.path[0] = CSTR_END;
    }
    else
    {
        strncpy( _vfs_search_context.path, search_path, SDL_arraysize( _vfs_search_context.path ) );
    }

    _vfs_search_context.bits = search_bits;

    return _vfs_search( &_vfs_search_context );
}

//--------------------------------------------------------------------------------------------
const char * vfs_findNext()
{
    // if there are no files, return an error value

    if ( NULL == _vfs_search_context.file_list )
    {
        return NULL;
    }

    return _vfs_search( &_vfs_search_context );
}

//--------------------------------------------------------------------------------------------
void vfs_findClose()
{
    _vfs_findClose( &_vfs_search_context );
}

//--------------------------------------------------------------------------------------------
int vfs_removeDirectoryAndContents( const char * dirname, int recursive )
{
    // buffer the directory delete through PHYSFS, so that we so not access functions that
    // we have no right to! :)

    const char *  write_dir;

    // make sure that this is a valid directory
    write_dir = vfs_resolveWriteFilename( dirname );
    if ( !fs_fileIsDirectory( write_dir ) ) return VFS_FALSE;

    fs_removeDirectoryAndContents( write_dir, recursive );

    return VFS_TRUE;
}

//--------------------------------------------------------------------------------------------
static bool_t _vfs_copyFile( const char *source, const char *dest )
{
    /// @details ZZ@> This function copies a file on the local machine
    PHYSFS_File *sourcef = NULL, *destf = NULL;
    char         buf[4096] = EMPTY_CSTR;
    int          bytes_read;
    bool_t       retval = VFS_FALSE;

    sourcef = PHYSFS_openRead( source );
    if ( !sourcef )
    {
        goto _vfs_copyFile_end;
    }

    destf = PHYSFS_openWrite( dest );
    if ( !sourcef )
    {
        goto _vfs_copyFile_end;
    }

    retval = VFS_TRUE;

    bytes_read = btrue;
    while ( bytes_read > 0 )
    {
        bytes_read = PHYSFS_read( sourcef, buf, sizeof( char ), SDL_arraysize( buf ) );
        PHYSFS_write( destf, buf, sizeof( char ), bytes_read );
    }

_vfs_copyFile_end:

    if ( NULL != sourcef ) PHYSFS_close( sourcef );
    if ( NULL != destf ) PHYSFS_close( destf );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_copyFile( const char *source, const char *dest )
{
    // buffer the directory delete through PHYSFS, so that we so not access functions that
    // we have no right to! :)

    VFS_PATH     sz_src = EMPTY_CSTR, sz_dst = EMPTY_CSTR;
    const char * real_dst, * real_src;

    if ( INVALID_CSTR( source ) || INVALID_CSTR( dest ) )
    {
        return VFS_FALSE;
    }

    if ( !_vfs_ensure_write_directory( dest, bfalse ) )
    {
        return VFS_FALSE;
    }

    strncpy( sz_src, vfs_resolveReadFilename( source ), SDL_arraysize( sz_src ) );
    real_src = sz_src;

    strncpy( sz_dst, vfs_resolveWriteFilename( dest ), SDL_arraysize( sz_dst ) );
    real_dst = sz_dst;

    if ( INVALID_CSTR( real_src ) || INVALID_CSTR( real_dst ) )
    {
        return VFS_FALSE;
    }

    // if they are the same files, do nothing
    if ( 0 == strcmp( real_src, real_dst ) )
    {
        return VFS_FALSE;
    }

    // !assume! that we are not dealing with archives, and just use the
    // fs_* copy command
    if ( !fs_copyFile( real_src, real_dst ) ) return VFS_FALSE;

    return VFS_TRUE;
}

//--------------------------------------------------------------------------------------------
int vfs_copyDirectory( const char *sourceDir, const char *destDir )
{
    /// @details ZZ@> This function copies all files in a directory
    VFS_PATH srcPath = EMPTY_CSTR, destPath = EMPTY_CSTR;
    const char *fileName;

    VFS_PATH     szDst = EMPTY_CSTR;
    const char * real_dst;

    if ( INVALID_CSTR( sourceDir ) || INVALID_CSTR( destDir ) )
    {
        return VFS_FALSE;
    }

    // make sure the destination directory exists
    if ( !vfs_mkdir( destDir ) )
    {
        return VFS_FALSE;
    }

    // get the a filename that we are allowed to write to
    snprintf( szDst, SDL_arraysize( szDst ), "%s",  vfs_resolveWriteFilename( destDir ) );
    real_dst = szDst;

    // List all the files in the directory
    fileName = vfs_findFirst( vfs_convert_fname( sourceDir ), NULL, VFS_SEARCH_FILE | VFS_SEARCH_BARE );
    while ( VALID_CSTR( fileName ) )
    {
        // Ignore files that begin with a .
        if ( '.' != fileName[0] )
        {
            snprintf( srcPath, SDL_arraysize( srcPath ), "%s" SLASH_STR "%s", sourceDir, fileName );
            snprintf( destPath, SDL_arraysize( destPath ), "%s" SLASH_STR "%s", destDir, fileName );

            if ( !vfs_copyFile( srcPath, destPath ) )
            {
                log_debug( "vfs_copyDirectory() - Failed to copy from \"%s\" to \"%s\" (%s)\n", srcPath, destPath, vfs_getError() );
            }
        }
        fileName = vfs_findNext();
    }
    vfs_findClose();

    return VFS_TRUE;
}

//--------------------------------------------------------------------------------------------
int vfs_ungetc( int c, vfs_FILE * pfile )
{
    int retval = 0;

    if ( NULL == pfile ) return 0;

    if ( vfs_cfile == pfile->type )
    {
        retval = EGO_ungetc( c, pfile->ptr.c );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = 0;
        //retval = PHYSFS_write( pfile->ptr.p, &c, 1, sizeof(char) );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_getc( vfs_FILE * pfile )
{
    int retval = 0;

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( vfs_cfile == pfile->type )
    {
        retval = EGO_fgetc( pfile->ptr.c );
        if ( EOF == retval ) pfile->flags |= VFS_EOF;
    }
    else if ( vfs_physfs == pfile->type )
    {
        char cTmp;
        retval = PHYSFS_read( pfile->ptr.p, &cTmp, 1, sizeof( cTmp ) );

        if ( -1 == retval ) pfile->flags |= VFS_ERROR;
        if ( 0 == retval ) pfile->flags |= VFS_EOF;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_putc( int c, vfs_FILE * pfile )
{
    int retval = 0;

    if ( NULL == pfile ) return 0;

    if ( vfs_cfile == pfile->type )
    {
        retval = EGO_fputc( c, pfile->ptr.c );
    }
    else if ( vfs_physfs == pfile->type )
    {
        retval = PHYSFS_write( pfile->ptr.p, &c, 1, sizeof( char ) );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_puts( const char * str , vfs_FILE * pfile )
{
    int retval = 0;

    if ( NULL == pfile || INVALID_CSTR( str ) ) return 0;

    if ( vfs_cfile == pfile->type )
    {
        retval = EGO_fputs( str, pfile->ptr.c );
    }
    else if ( vfs_physfs == pfile->type )
    {
        int len = strlen( str );

        retval = PHYSFS_write( pfile->ptr.p, str, len + 1, sizeof( char ) );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
char * vfs_gets( char * buffer, int buffer_size, vfs_FILE * pfile )
{
    char * retval = NULL;

    if ( NULL == pfile ) return NULL;

    if ( NULL == buffer || 0 == buffer_size ) return buffer;

    if ( vfs_cfile == pfile->type )
    {
        retval = EGO_fgets( buffer, buffer_size, pfile->ptr.c );
        if ( NULL == retval ) pfile->flags |= VFS_EOF;
    }
    else if ( vfs_physfs == pfile->type )
    {
        int  iTmp;
        char cTmp;
        char * str_ptr, * str_end;

        str_ptr = buffer;
        str_end = buffer + buffer_size;

        iTmp = PHYSFS_read( pfile->ptr.p, &cTmp, 1, sizeof( cTmp ) );
        if ( -1 == iTmp ) pfile->flags |= VFS_ERROR;
        if ( 0 == iTmp ) pfile->flags |= VFS_EOF;

        while ( iTmp && ( str_ptr < str_end - 1 ) && CSTR_END != cTmp && 0 == pfile->flags )
        {
            *str_ptr = cTmp;
            str_ptr++;

            if ( 0x0A == cTmp || 0x0D == cTmp ) break;

            iTmp = PHYSFS_read( pfile->ptr.p, &cTmp, 1, sizeof( cTmp ) );

            if ( -1 == iTmp ) pfile->flags |= VFS_ERROR;
            if ( 0 == iTmp ) pfile->flags |= VFS_EOF;
        }
        *str_ptr = CSTR_END;

        retval = buffer;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void vfs_empty_import_directory()
{
    vfs_removeDirectoryAndContents( "import", VFS_TRUE );
    vfs_removeDirectoryAndContents( "remote", VFS_TRUE );
}

//--------------------------------------------------------------------------------------------
int _vfs_vfscanf( FILE * file, const char * format, va_list args )
{
    // UGH! Just break the format code into pieces and call fscanf on each piece

    char   sub_format[256] = EMPTY_CSTR;
    char * format_end, * format_next;
    int    argcount = 0;
    void * ptr;

    if ( NULL == file || INVALID_CSTR( format ) ) return 0;

    format_end = ( char * )( format + strlen( format ) );

    // scan throuh the format string looking for formats
    argcount = 0;
    while ( format < format_end )
    {
        bool_t found_format;
        char * format_tmp;

        // find everything up to the first valid format code in the format string
        found_format = bfalse;
        format_tmp   = ( char * )format;
        format_next  = format_tmp;
        while ( format_next < format_end )
        {
            format_next = strchr( format_tmp, '%' );

            // handle the occurrence of "%%"
            if ( '%' == *( format_next + 1 ) )
            {
                format_tmp = format_next + 1;
            }
            else
            {
                found_format = btrue;
                break;
            }
        }

        // copy the format string fragment
        if ( found_format && format_next < format_end )
        {
            // scan the valid format code
            format_next += strcspn( format_next, "cCsSdioxXnueEfgG" ) + 1;
        }
        strncpy( sub_format, format, format_next - format );

        // get a pointer to the variable to be filled
        ptr = NULL;
        if ( found_format )
        {
            ptr = va_arg( args, void * );
        }

        // do the call to fscanf()
        if ( NULL == ptr )
        {
            // this may cause a problem...
            fscanf( file, sub_format, NULL );
        }
        else
        {
            argcount += fscanf( file, sub_format, ptr );
        }

        format = format_next;
    }

    return argcount;
}

//--------------------------------------------------------------------------------------------
int vfs_rewind( vfs_FILE * pfile )
{
    if ( NULL == pfile ) return 0;

    return vfs_seek( pfile, 0 );
}

//--------------------------------------------------------------------------------------------
void _vfs_translate_error( vfs_FILE * pfile )
{
    if ( NULL == pfile ) return;

    if ( vfs_cfile == pfile->type )
    {
        if ( EGO_ferror( pfile->ptr.c ) )
        {
            pfile->flags |= VFS_ERROR;
        }

        if ( EGO_feof( pfile->ptr.c ) )
        {
            pfile->flags |= VFS_EOF;
        }
    }
    else if ( vfs_physfs == pfile->type )
    {
        if ( PHYSFS_eof( pfile->ptr.p ) )
        {
            pfile->flags |= VFS_EOF;
        }
    }

}

//--------------------------------------------------------------------------------------------
const char * vfs_getError()
{
    /// @details ZF@> Returns the last error the PHYSFS system reported.

    static char errors[1024];
    const char * physfs_error, * file_error;
    bool_t is_error;

    // load up a default
    strncpy( errors, "unknown error", SDL_arraysize( errors ) );

    // assume no error
    is_error = bfalse;

    // try to get the physfs error state;
    physfs_error = PHYSFS_getLastError();
    if ( NULL == physfs_error ) physfs_error = "no error";

    // try to get the stdio error state
    file_error = strerror( errno );

    snprintf( errors, SDL_arraysize( errors ), "c stdio says:\"%s\" -- physfs says:\"%s\"", file_error, physfs_error );

    return errors;
}

//--------------------------------------------------------------------------------------------
int _vfs_mount_point_matches( const char * mount_point, const char * local_path )
{
    int cnt, retval;
    const char * ptmp;

    // set to an invalid value;
    retval = -1;

    // are there any in the list?
    if ( 0 == _vfs_mount_info_count ) return retval;

    // alias the mount point
    ptmp = mount_point;

    // strip any starting slashes
    if ( VALID_CSTR( ptmp ) )
    {
        for ( /* nothing */; ptmp < mount_point + VFS_MAX_PATH; ptmp++ )
        {
            if ( '/' != *ptmp && '\\' != *ptmp || '\0' == *ptmp )
            {
                break;
            }
        }
    }

    if ( VALID_CSTR( ptmp ) && VALID_CSTR( local_path ) )
    {
        // find the first path info with the given mount_point and local_path
        for ( cnt = 0; cnt < _vfs_mount_info_count; cnt++ )
        {
            if ( 0 == strncmp( _vfs_mount_info[cnt].mount, mount_point, VFS_MAX_PATH ) &&
                 0 == strncmp( _vfs_mount_info[cnt].path,  local_path, VFS_MAX_PATH ) )
            {
                retval = cnt;
                break;
            }
        }
    }
    else if ( VALID_CSTR( ptmp ) )
    {
        // find the first path info with the given mount_point
        for ( cnt = 0; cnt < _vfs_mount_info_count; cnt++ )
        {
            if ( 0 == strncmp( _vfs_mount_info[cnt].mount, mount_point, VFS_MAX_PATH ) )
            {
                retval = cnt;
                break;
            }
        }
    }
    else if ( VALID_CSTR( local_path ) )
    {
        // find the first path info with the given local_path
        for ( cnt = 0; cnt < _vfs_mount_info_count; cnt++ )
        {
            if ( 0 == strncmp( _vfs_mount_info[cnt].path,  local_path, VFS_MAX_PATH ) )
            {
                retval = cnt;
                break;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t _vfs_add_mount_info( const char * mount_point, const char * local_path )
{
    const char * ptmp;

    // can we add it?
    if ( _vfs_mount_info_count >= MAX_MOUNTINFO ) return bfalse;

    // do we want to add it?
    if ( !VALID_CSTR( local_path ) ) return bfalse;

    if ( _vfs_mount_point_matches( mount_point, local_path ) >= 0 ) return bfalse;

    // strip any starting slashes
    for ( ptmp = mount_point; ptmp < mount_point + VFS_MAX_PATH; ptmp++ )
    {
        if ( '/' != *ptmp && '\\' != *ptmp || '\0' == *ptmp )
        {
            break;
        }
    }

    if ( '\0' == *ptmp ) return bfalse;

    // save the mount point in a list for later detection
    strncpy( _vfs_mount_info[_vfs_mount_info_count].mount, ptmp,       VFS_MAX_PATH );
    strncpy( _vfs_mount_info[_vfs_mount_info_count].path,  local_path, VFS_MAX_PATH );

    _vfs_mount_info_count++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t _vfs_remove_mount_info( int cnt )
{

    // does it exist in the list?
    if ( cnt < 0 || cnt > _vfs_mount_info_count ) return bfalse;

    // fill in the hole in the list
    if ( _vfs_mount_info_count > 1 )
    {
        memmove( _vfs_mount_info + cnt, _vfs_mount_info + ( _vfs_mount_info_count - 1 ), sizeof( vfs_path_data_t ) );
    }

    // shorten the list
    _vfs_mount_info_count--;

    return btrue;
}

//--------------------------------------------------------------------------------------------
int vfs_add_mount_point( const char * dirname, const char * mount_point, int append )
{
    /// @details BB@> a wrapper for PHYSFS_mount

    const char * loc_dirname;

    if ( !VALID_CSTR( dirname ) ) return 0;
    if ( 0 == strcmp( mount_point, "/" ) ) return 0;

    loc_dirname = vfs_convert_fname_sys( dirname );

    _vfs_add_mount_info( mount_point, loc_dirname );

    return PHYSFS_mount( loc_dirname, mount_point, append );
}

//--------------------------------------------------------------------------------------------
int vfs_remove_mount_point( const char * mount_point )
{
    /// @details BB@> Remove every single search path related to the given mount point.

    int retval, cnt;

    // don't allow it to remove the default directory
    if ( !VALID_CSTR( mount_point ) ) return 0;
    if ( 0 == strcmp( mount_point, "/" ) ) return 0;

    // assume we are going to fail
    retval = 0;

    // see if we have the mount point
    cnt = _vfs_mount_point_matches( mount_point, NULL );

    // does it exist in the list?
    if ( cnt < 0 ) return bfalse;

    while ( cnt >= 0 )
    {
        int tmp_retval;
        // we have to use the path name to remove the search path, not the mount point name
        tmp_retval = PHYSFS_removeFromSearchPath( _vfs_mount_info[cnt].path );

        // if we succedded once, we succeeded
        if ( 0 != tmp_retval )
        {
            retval = 1;

            // remove the mount info from this index
            // ?should it be removed if PHYSFS_removeFromSearchPath() fails?
            _vfs_remove_mount_info( cnt );
        }

        cnt = _vfs_mount_point_matches( mount_point, NULL );
    }

    return retval;
}