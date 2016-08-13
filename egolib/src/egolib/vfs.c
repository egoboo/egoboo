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

/// @file egolib/vfs.c
/// @brief Implementation of the Egoboo virtual file system
/// @details

#include <physfs.h>

#include "egolib/vfs.h"

#include "egolib/file_common.h"
#include "egolib/Log/_Include.hpp"

#include "egolib/strutil.h"
#include "egolib/endian.h"
#include "egolib/fileutil.h"
#include "egolib/platform.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_vfs_path_data;
typedef struct s_vfs_path_data vfs_path_data_t;

//--------------------------------------------------------------------------------------------

/**
 * @brief
 *  If this is and @a _DEBUG are both defined,
 *  the VFS system runs in debug mode.
 */
#undef _VFS_DEBUG

 //--------------------------------------------------------------------------------------------
#define VFS_MAX_PATH 1024

#define BAIL_IF_NOT_INIT() \
	if(!_vfs_initialized) { \
		std::ostringstream os; \
		os << __FUNCTION__ << ": EgoLib VFS function called while the VFS was not initialized" << std::endl; \
		Log::get().error("%s", os.str().c_str()); \
		throw std::runtime_error(os.str()); \
	}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

typedef char VFS_PATH[VFS_MAX_PATH];

#define MAX_MOUNTINFO 128

/// The following flags set in vfs_file::flags provide information about the state of a file.
typedef enum vfs_file_flags
{

    /// End of the file encountered.
    VFS_FILE_FLAG_EOF = (1 << 0),

    /// Error was encountered.
    VFS_FILE_FLAG_ERROR = (1 << 1),

    /// The file is opened for writing.
    VFS_FILE_FLAG_WRITING = (1 << 2),

    /// The file is opened for reading.
    VFS_FILE_FLAG_READING = (1 << 3),

} vfs_file_flagss;

/// What type of file is actually being referenced in u_vfs_fileptr
typedef enum vfs_file_type
{
    VFS_FILE_TYPE_UNKNOWN = 0,
    VFS_FILE_TYPE_CSTDIO,
    VFS_FILE_TYPE_PHYSFS,
} vfs_file_type;

/// An anonymized pointer type
typedef union vfs_fileptr_t
{
    void *u;
    FILE *c;
    PHYSFS_File *p;
} vfs_file_ptr_t;

/// A container holding either a FILE * or a PHYSFS_File *, and translated error states
struct vsf_file
{
    BIT_FIELD flags;
    vfs_file_type type;
    vfs_fileptr_t ptr;
};

/// A container for holding all the data for a search
struct s_vfs_search_context
{
    char **    file_list;
    char **    ptr;

    char       path[VFS_MAX_PATH];
    char       ext[255];
    BIT_FIELD  bits;

    VFS_PATH found;
};

struct s_vfs_path_data
{
    VFS_PATH mount;

    VFS_PATH full_path;
    VFS_PATH root_path;
    VFS_PATH relative_path;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int             _vfs_mount_info_count = 0;
static vfs_path_data_t _vfs_mount_info[MAX_MOUNTINFO];

static bool _vfs_atexit_registered = false;

static bool _vfs_initialized = false;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//static int _vfs_vfscanf(FILE * file, const char * format, va_list args);
//static bool _vfs_ensure_destination_file(const char * filename);

static void _vfs_exit();
static vfs_search_context_t *_vfs_search(vfs_search_context_t **ctxt);
static int _vfs_ensure_write_directory(const char *filename, bool is_directory);


static void _vfs_translate_error(vfs_FILE *file);

static bool _vfs_mount_info_add(const char *mount_point, const char *root_path, const char *relative_path);
static int _vfs_mount_info_matches(const char *mount_point, const char *local_path);
static bool _vfs_mount_info_remove(int cnt);
static int _vfs_mount_info_search(const char *some_path);

//static const char *_vfs_potential_mount_point(const char *some_path, const char **pstripped_pos);
static void _vfs_findClose(vfs_search_context_t *ctxt);

static int fake_physfs_vprintf(PHYSFS_File *file, const char *format, va_list args);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int vfs_init(const char *argv0, const char *root_dir)
{
    VFS_PATH tmp_path;

    if (fs_init(root_dir))
    {
        return 1;
    }
    
    if (!fs_fileIsDirectory(fs_getDataDirectory()))
    {
        // We can call log functions, they won't try to write to unopened log files
        // But mainly this is used for sys_popup
        Log::get().error("The data path isn't a directory.\nData path: '%s'\n", fs_getDataDirectory());
        return 1;
    }

    if (_vfs_initialized)
    {
        return 0;
    }

    if (!PHYSFS_init(argv0))
    {
        return 1;
    }
    // Append the data directory to the search directories.
    snprintf(tmp_path, SDL_arraysize(tmp_path), "%s" SLASH_STR, fs_getDataDirectory());
    if (!PHYSFS_mount(tmp_path, "/", 1))
    {
        PHYSFS_deinit();
        return 1;
    }

    //---- !!!! make sure the basic directories exist !!!!

    // Ensure that the /user directory exists.
    if (!fs_fileIsDirectory(fs_getUserDirectory()))
    {
        fs_createDirectory(fs_getUserDirectory()); ///< @todo Error handling!
    }

    // Ensure that the /user/debug directory exists.
    if (!fs_fileIsDirectory(fs_getUserDirectory()))
    {
        printf("WARNING: cannot create write directory %s\n", fs_getUserDirectory());
    }
    else
    {
        char loc_path[1024] = EMPTY_CSTR;

        snprintf(loc_path, SDL_arraysize( loc_path ), "%s/debug", fs_getUserDirectory());

        str_convert_slash_sys(loc_path, SDL_arraysize(loc_path));
        fs_createDirectory(loc_path);
    }

    // Set the write directory to the root user directory.
    if (!PHYSFS_setWriteDir(fs_getUserDirectory()))
    {
        PHYSFS_deinit();
        return 1;
    }

    if (!_vfs_atexit_registered)
    {
        atexit(_vfs_exit); /// @todo Error handling?
        _vfs_atexit_registered = true;
    }

    _vfs_initialized = true;
    return 0;
}

//--------------------------------------------------------------------------------------------
void _vfs_exit()
{
    PHYSFS_deinit();
}

//--------------------------------------------------------------------------------------------
const char *vfs_getVersion()
{
    /// @author ZF
    /// @details  returns the current version of the PhysFS library which was used for compiling the binary
    PHYSFS_Version version;
    static STRING buffer = EMPTY_CSTR;

    //PHYSFS_getLinkedVersion(&version);        //Linked version number
    PHYSFS_VERSION( &version );         //Compiled version number
    snprintf( buffer, SDL_arraysize( buffer ), "%d.%d.%d", version.major, version.minor, version.patch );

    return buffer;
}

#include "egolib/VFS/Pathname.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool validate(const std::string& source, std::string& target) {
    try {
        target = Ego::VFS::Pathname(source).toString();
        return true;
    } catch (...) {
        return false;
    }
}

vfs_FILE *vfs_openRead(const std::string& pathname)
{
    BAIL_IF_NOT_INIT();

    std::string temporary;
    if (!validate(pathname,temporary)) {
        return nullptr;
    }

    PHYSFS_File *ftmp = PHYSFS_openRead(temporary.c_str());
    if (!ftmp)
    {
    #if defined(_DEBUG) && defined(_VFS_DEBUG)
        log_warning("unable to open file `%s` for reading - reason: %s\n", pathname.c_str(), PHYSFS_getLastError());
    #endif
        return nullptr;
    }

	vfs_FILE *vfs_file;
	try {
		vfs_file = new vfs_FILE();
	} catch (...) {
        PHYSFS_close(ftmp);
        return nullptr;
    }

    vfs_file->flags = VFS_FILE_FLAG_READING;
    vfs_file->type = VFS_FILE_TYPE_PHYSFS;
    vfs_file->ptr.p = ftmp;

    return vfs_file;
}

vfs_FILE *vfs_openWrite(const std::string& pathname)
{
    BAIL_IF_NOT_INIT();

    std::string temporary;
    if (!validate(pathname, temporary)) {
        return nullptr;
    }

    // Make sure that the output directory exists.
    if (!_vfs_ensure_write_directory(temporary.c_str(), false))
    {
        return NULL;
    }

    // Open the PhysFS file.
    PHYSFS_File *ftmp = PHYSFS_openWrite(temporary.c_str());
    if (!ftmp)
    {
    #if defined(_DEBUG) && defined(_VFS_DEBUG)
        log_warning("unable to open file `%s` for writing - reason: %s\n", pathname.c_str(), PHYSFS_getLastError());
    #endif
        return NULL;
    }

    // Open the VFS file.
	vfs_FILE *vfs_file;
	try {
		vfs_file = new vfs_FILE();
	} catch (...) {
		PHYSFS_close(ftmp);
		return nullptr;
	}
    vfs_file->flags = VFS_FILE_FLAG_WRITING;
    vfs_file->type  = VFS_FILE_TYPE_PHYSFS;
    vfs_file->ptr.p = ftmp;

    return vfs_file;
}

vfs_FILE *vfs_openAppend(const std::string& pathname)
{
    BAIL_IF_NOT_INIT();

    std::string temporary;
    if (!validate(pathname, temporary)) {
        return nullptr;
    }

    PHYSFS_File *ftmp = PHYSFS_openAppend(temporary.c_str());
    if (!ftmp)
    {
    #if defined(_DEBUG) && defined(_VFS_DEBUG)
        log_warning("unable to open file `%s` for appending - reason: %s\n", pathname.c_str(), PHYSFS_getLastError());
    #endif
        return NULL;
    }

	vfs_FILE *vfs_file;
	try {
		vfs_file = new vfs_FILE();
	} catch (...) {
        PHYSFS_close(ftmp);
        return nullptr;
    }
    vfs_file->flags = VFS_FILE_FLAG_WRITING;
    vfs_file->type  = VFS_FILE_TYPE_PHYSFS;
    vfs_file->ptr.p = ftmp;

    return vfs_file;
}

//--------------------------------------------------------------------------------------------
const char * vfs_convert_fname_sys( const char * fname )
{
    // PHYSFS and this vfs use a C_SLASH_STR at the front of relative filenames. if this is not removed
    // when converting to system dependent filenames, it will reference the filename relative to the
    // root point, rather than relative to the current directory.
    //
    // Added complication: if you are trying to specify a root filename,
    // then you might expect a SLASH_CHR at the beginning of the pathname.
    // To fix this, call str_convert_slash_sys(), and then check to see if the
    // directory exists in the filesystem.
    //
    // This is not an ideal solution since we may be trying to specify a path for
    // a tile that needs to be created. The optimal solution might be to check to see if
    // the path belongs to a registered virtual mount point?

    static VFS_PATH local_fname = EMPTY_CSTR;

    size_t   offset;
    VFS_PATH copy_fname  = EMPTY_CSTR;
    char *   string_ptr;

    BAIL_IF_NOT_INIT();

    // test for a bad input filename
    if ( INVALID_CSTR( fname ) )
    {
        strncpy( local_fname, SLASH_STR, SDL_arraysize( local_fname ) );
        return local_fname;
    }

    if ( VFS_TRUE == _vfs_mount_info_search( fname ) )
    {
        // this path contains a virtual mount point
        EGOBOO_ASSERT( false );
        return local_fname;
    }

    // make a local copy of the original filename, in case fname is
    // a string literal or a pointer to local_fname
    strncpy( copy_fname, fname, SDL_arraysize( copy_fname ) );

    // convert the path string to local notation
    string_ptr = str_convert_slash_sys( copy_fname, strlen( fname ) );
    if ( !VALID_CSTR( string_ptr ) )
    {
        strncpy( local_fname, SLASH_STR, SDL_arraysize( local_fname ) );
        return local_fname;
    }

    // resolve the conflict between
    //    -1- directories relative to the PHYSFS root path starting with NET_SLASH
    // and
    //    -2- directories relative to the root of the filesystem beginning with a slash
    // by trying to find the original path
    //
    // The following method is not exactly secure, since it would allow access to generic
    // root directories using this code...

    // if the path already exists, just return the path
    if ( fs_fileExists( copy_fname ) )
    {
        strncpy( local_fname, copy_fname, SDL_arraysize( local_fname ) );
        return local_fname;
    }

    // if the path didn't exist it might be because it contains a file that has not yet been created...
    // no solution to that problem, yet.

    // ---- if we got to this point, we need to strip off any beginning slashes

    offset = 0;
    if ( '.' == copy_fname[0] && '.' == copy_fname[1] )
    {
        offset++;
    }

    // the string has already been converted to a system filename, so just check SLASH_CHR
    while ( offset < SDL_arraysize(copy_fname) && SLASH_CHR == copy_fname[offset] )
    {
        offset++;
    }

    // copy the proper relative filename
    strncpy( local_fname, copy_fname + offset, SDL_arraysize( local_fname ) );

    return local_fname;
}

//--------------------------------------------------------------------------------------------
const char * vfs_convert_fname( const char * fname )
{
    VFS_PATH        copy_fname  = EMPTY_CSTR;
    static VFS_PATH local_fname = EMPTY_CSTR;

    BAIL_IF_NOT_INIT();

    // test for a bad iput filename
    if ( INVALID_CSTR( fname ) )
    {
        strncpy( local_fname, NET_SLASH_STR, SDL_arraysize( local_fname ) );
        return local_fname;
    }

    // make a copy of the original filename, in case fname is
    // a literal string or a pointer to local_fname
    strncpy( copy_fname, fname, SDL_arraysize( copy_fname ) );

    if ( _vfs_mount_info_search( copy_fname ) )
    {
        snprintf( local_fname, SDL_arraysize( local_fname ), "%s", copy_fname );
    }
    else if ( NET_SLASH_CHR == copy_fname[0] || WIN32_SLASH_CHR == copy_fname[0] )
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
/*const char * _vfs_potential_mount_point( const char * some_path, const char ** pstripped_pos )
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

    found_path[0] = CSTR_END;

    BAIL_IF_NOT_INIT();

    if ( !VALID_CSTR( some_path ) ) return found_path;

    ptmp = some_path;

    path_begin = some_path;
    path_end   = some_path + VFS_MAX_PATH - 1;

    // strip any starting slashes
    for ( ptmp = path_begin; ptmp < path_end; ptmp++ )
    {
        if ( NET_SLASH_CHR != *ptmp && WIN32_SLASH_CHR != *ptmp )
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
		assert(path_end > path_begin);
        size_t count = path_end - path_begin;
        strncpy( found_path, path_begin, count );
        found_path[count] = CSTR_END;
    }

    // export the beginning of the string after the mount point, if possible
    if ( NULL != pstripped_pos )
    {
        *pstripped_pos = path_end;
    }

    // return the potential mount point in system-dependent format
    return str_convert_slash_sys( found_path, strlen( found_path ) );
}*/

//--------------------------------------------------------------------------------------------
void vfs_listSearchPaths( void )
{
    //JJ> Lists all search paths that PhysFS uses (for debug use)

    char **i;

    BAIL_IF_NOT_INIT();

    printf( "LISTING ALL PHYSFS SEARCH PATHS:\n" );
    printf( "----------------------------------\n" );
    for ( i = PHYSFS_getSearchPath(); *i != NULL; i++ )   printf( "[%s] is in the search path.\n", *i );
    printf( "----------------------------------\n" );
}

//--------------------------------------------------------------------------------------------
const char * vfs_resolveReadFilename( const char * src_filename )
{
    static STRING read_name_str = EMPTY_CSTR;
    VFS_PATH      loc_fname = EMPTY_CSTR, szTemp = EMPTY_CSTR;
    int           retval_len = 0;
    const char   *retval = NULL;

    BAIL_IF_NOT_INIT();

    if ( INVALID_CSTR( src_filename ) ) return NULL;

    // make a copy of the szTemp, in case we are passed a string literal
    // as the argument of this function
    strncpy( szTemp, src_filename, SDL_arraysize( szTemp ) );

    // make a temporary copy of the given filename with system-dependent slashes
    // to see if the filename is already resolved
    strncpy( loc_fname, szTemp, SDL_arraysize( loc_fname ) );
    str_convert_slash_sys( loc_fname, SDL_arraysize( loc_fname ) );

    if ( fs_fileExists( loc_fname ) )
    {
        strncpy( read_name_str, loc_fname, SDL_arraysize( read_name_str ) );

        return read_name_str;
    }

    // make another copy of the local filename
    // and make sure that PHYSFS gets the filename with the slashes it wants
    strncpy( loc_fname, vfs_convert_fname( szTemp ), SDL_arraysize( loc_fname ) );

    retval = NULL;
    retval_len = 0;
    if ( PHYSFS_isDirectory( loc_fname ) )
    {
        retval = PHYSFS_getRealDir( loc_fname );

        if ( VALID_CSTR( retval ) )
        {
            const char * ptmp = vfs_mount_info_strip_path( loc_fname );

            if ( VALID_CSTR( ptmp ) )
            {
                snprintf( read_name_str, SDL_arraysize( read_name_str ), "%s/%s", retval, ptmp );
            }
            else
            {
                snprintf( read_name_str, SDL_arraysize( read_name_str ), "%s/", retval );
            }

            retval     = read_name_str;
            retval_len = SDL_arraysize( read_name_str );
        }
    }
    else
    {
        const char * tmp_dirname;
        const char * ptmp = loc_fname;

        // make PHYSFS grab the actual directory
        tmp_dirname = PHYSFS_getRealDir( loc_fname );

        if ( INVALID_CSTR( tmp_dirname ) )
        {
            // not found... just punt
            strncpy( read_name_str, loc_fname, SDL_arraysize( read_name_str ) );
            retval     = read_name_str;
            retval_len = SDL_arraysize( read_name_str );
        }
        else
        {
            ptmp = vfs_mount_info_strip_path( loc_fname );

            snprintf( read_name_str, SDL_arraysize( read_name_str ), "%s/%s", tmp_dirname, ptmp );
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
    VFS_PATH szTemp;
    const  char    * write_dir;

    BAIL_IF_NOT_INIT();

    if ( INVALID_CSTR( src_filename ) ) return szFname;

    // make a copy of the src_filename, in case we are passed a string literal
    // as the argument of this function
    strncpy( szTemp, src_filename, SDL_arraysize( szTemp ) );

    write_dir = PHYSFS_getWriteDir();
	if (nullptr == write_dir)
	{
		Log::get().warn("PhysFS could not get write directory!\n");
		return nullptr;
	}

    // append the write_dir to the szTemp to get the total path
    snprintf( szFname, SDL_arraysize( szFname ), "%s" SLASH_STR "%s", write_dir, szTemp );

    // make sure that the slashes are correct for this system, and that they are not doubled

    return str_convert_slash_sys( szFname, SDL_arraysize( szFname ) );
}

//--------------------------------------------------------------------------------------------
int _vfs_ensure_write_directory( const char * filename, bool is_directory )
{
    /// @author BB
    /// @details

    int           retval;
    VFS_PATH      temp_dirname = EMPTY_CSTR;
    char        * tmpstr;

    BAIL_IF_NOT_INIT();

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

        if ( CSTR_END == temp_dirname[0] )
        {
            temp_dirname[0] = C_SLASH_CHR;
            temp_dirname[1] = CSTR_END;
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
#if 0
bool _vfs_ensure_destination_file(const char * filename)
{
    /// @author BB
    /// @details make sure that a copy of filename from the read path exists in
    ///     the write directory, but do not overwrite any existing file

    VFS_PATH local_filename = EMPTY_CSTR;

    BAIL_IF_NOT_INIT();

    if ( INVALID_CSTR( filename ) ) return false;

    // make a local copy of the filename
    // and make sure that PHYSFS gets the filename with the slashes it wants
    strncpy( local_filename, vfs_convert_fname( filename ), SDL_arraysize( local_filename ) );

    // make sure that the output directory exists
    if ( !_vfs_ensure_write_directory( local_filename, false ) ) return false;

    // be a bit carefil here, in case the file exists in the read path and not in the write
    // directory
    
    return vfs_copyFile(filename, filename);
}
#endif

int vfs_isReading(vfs_FILE *file)
{
    if (!file)
    {
        return -1;
    }
    return VFS_FILE_FLAG_READING == (file->flags & VFS_FILE_FLAG_READING);
}

int vfs_isWriting(vfs_FILE *file)
{
    if (!file)
    {
        return -1;
    }
    return VFS_FILE_FLAG_WRITING == (file->flags & VFS_FILE_FLAG_WRITING);
}

int vfs_close(vfs_FILE *file)
{
    BAIL_IF_NOT_INIT();

    if (!file)
    {
        return 0;
    }

    int retval = 0;
    if (VFS_FILE_TYPE_CSTDIO == file->type)
    {
        retval = fclose(file->ptr.c);
		delete file;
    }
    else if (VFS_FILE_TYPE_PHYSFS == file->type)
    {
        retval = PHYSFS_close(file->ptr.p);
		delete file;
    }
    else
    {
        // corrupted data?
        fprintf(stderr, "Tried to deallocate an invalid vfs file descriptor\n");
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_flush( vfs_FILE * pfile )
{
    int retval;

    BAIL_IF_NOT_INIT();

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == pfile->type )
    {
        retval = fflush( pfile->ptr.c );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == pfile->type )
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

    BAIL_IF_NOT_INIT();

    if ( NULL == pfile ) return 0;

    // check our own end-of-file condition
    if ( 0 != ( pfile->flags & VFS_FILE_FLAG_EOF ) )
    {
        return pfile->flags & VFS_FILE_FLAG_EOF;
    }

    retval = 1;
    if ( VFS_FILE_TYPE_CSTDIO == pfile->type )
    {
        retval = feof( pfile->ptr.c );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == pfile->type )
    {
        retval = PHYSFS_eof( pfile->ptr.p );
    }

    if ( 0 != retval )
    {
        pfile->flags |= VFS_FILE_FLAG_EOF;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_error( vfs_FILE * pfile )
{
    int retval;

    BAIL_IF_NOT_INIT();

    if ( NULL == pfile ) return 0;

    retval = 1;
    if ( VFS_FILE_TYPE_CSTDIO == pfile->type )
    {
        retval = ferror( pfile->ptr.c );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == pfile->type )
    {
        retval = VFS_FILE_FLAG_ERROR == (pfile->flags & VFS_FILE_FLAG_ERROR);
        //retval = ( NULL != PHYSFS_getLastError() );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
long vfs_tell( vfs_FILE * pfile )
{
    long retval;

    BAIL_IF_NOT_INIT();

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == pfile->type )
    {
        retval = ftell( pfile->ptr.c );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == pfile->type )
    {
        retval = PHYSFS_tell( pfile->ptr.p );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_seek( vfs_FILE * pfile, long offset )
{
    int retval;

    BAIL_IF_NOT_INIT();

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == pfile->type )
    {
        // reset the flags
        pfile->flags &= ~(VFS_FILE_FLAG_EOF | VFS_FILE_FLAG_ERROR);

        // !!!! since we are opening non-binary files in text mode, fseek might act strangely !!!!
        retval = fseek( pfile->ptr.c, offset, SEEK_SET );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == pfile->type )
    {
        // reset the flags
        pfile->flags &= ~(VFS_FILE_FLAG_EOF | VFS_FILE_FLAG_ERROR);

        retval = PHYSFS_seek( pfile->ptr.p, offset );
        if (retval == 0) pfile->flags &= ~VFS_FILE_FLAG_ERROR;
        else             pfile->flags |= VFS_FILE_FLAG_ERROR;
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

    BAIL_IF_NOT_INIT();

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == pfile->type )
    {
        // do a little dance with the file pointer to figure out the file length

        long pos = ftell( pfile->ptr.c );

        fseek( pfile->ptr.c, 0, SEEK_END );
        retval = ftell( pfile->ptr.c );

        fseek( pfile->ptr.c, pos, SEEK_SET );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == pfile->type )
    {
        retval = PHYSFS_fileLength( pfile->ptr.p );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool vfs_mkdir(const std::string& pathname) {
    BAIL_IF_NOT_INIT();

    std::string temporary;
    if (!validate(pathname, temporary)) {
        return false;
    }

    if (!PHYSFS_mkdir(temporary.c_str())) {
        Log::get().debug("PHYSF_mkdir(%s) failed: %s\n", pathname.c_str(), vfs_getError());
        return false;
    }

    return true;
}

bool vfs_delete_file(const std::string& pathname)
{
    BAIL_IF_NOT_INIT();

    std::string temporary;
    if (!validate(pathname, temporary)) {
        return false;
    }

    if (!PHYSFS_delete(temporary.c_str())) {
        Log::get().debug("PHYSF_delete(%s) failed: %s\n", pathname.c_str(), vfs_getError());
        return false;
    }
    return true;
}

bool vfs_exists(const std::string& pathname) {
    BAIL_IF_NOT_INIT();
    std::string temporary;
    if (!validate(pathname, temporary)) {
        return false;
    }
    return (0 != PHYSFS_exists(temporary.c_str()));
}

bool vfs_isDirectory(const std::string& pathname) {
    BAIL_IF_NOT_INIT();
    std::string temporary;
    if (!validate(pathname, temporary)) {
        return false;
    }
    return 0 != PHYSFS_isDirectory(temporary.c_str());
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
size_t vfs_read( void * buffer, size_t size, size_t count, vfs_FILE * pfile )
{
	bool error = false;
    size_t read_length;

    BAIL_IF_NOT_INIT();

    if ( NULL == pfile ) return 0;

    read_length = 0;
    if ( VFS_FILE_TYPE_CSTDIO == pfile->type )
    {
        read_length = fread( buffer, size, count, pfile->ptr.c );
        error = ( read_length != size );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == pfile->type )
    {
        pfile->flags &= ~VFS_FILE_FLAG_ERROR;
        PHYSFS_sint64 retval = PHYSFS_read( pfile->ptr.p, buffer, size, count );

        if ( retval < 0 ) { error = true; pfile->flags |= VFS_FILE_FLAG_ERROR; }

        if ( !error ) read_length = retval;
    }

    if ( error ) _vfs_translate_error( pfile );

    return read_length;
}

//--------------------------------------------------------------------------------------------
size_t vfs_write( const void * buffer, size_t size, size_t count, vfs_FILE * pfile )
{
    bool error = false;
    size_t retval;

    BAIL_IF_NOT_INIT();

    if ( NULL == pfile ) return 0;

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == pfile->type )
    {
        retval = fwrite( buffer, size, count, pfile->ptr.c );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == pfile->type )
    {
        pfile->flags &= ~VFS_FILE_FLAG_ERROR;
        PHYSFS_sint64 write_length = PHYSFS_write( pfile->ptr.p, buffer, size, count );
        
        if ( write_length < 0 ) { error = true; pfile->flags |= VFS_FILE_FLAG_ERROR; }
        
        if ( !error ) retval = write_length;
    }
    
    if ( error ) _vfs_translate_error( pfile );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Sint8( vfs_FILE& file, Sint8 * val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
    
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        retval = fread( val, 1, sizeof( Sint8 ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_read(file.ptr.p, val, 1, sizeof(Sint8));
        
        error = ( 1 != retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Uint8( vfs_FILE& file, Uint8 * val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
    
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        retval = fread( val, 1, sizeof( Uint8 ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_read(file.ptr.p, val, 1, sizeof(Sint8));
        
        error = ( 1 != retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Sint16( vfs_FILE& file, Sint16 * val )
{
    int retval;
    bool error = false;

    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        Sint16 itmp;
        retval = fread( &itmp, 1, sizeof( Sint16 ), file.ptr.c );

        error = ( 1 != retval );

        *val = ENDIAN_TO_SYS_INT16( itmp );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_readSLE16( file.ptr.p, val );

        error = ( 0 == retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }

    if ( error ) _vfs_translate_error( &file );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Uint16( vfs_FILE& file, Uint16 * val )
{
	bool error = false;
    int retval;

    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        Uint16 itmp;
        retval = fread( &itmp, 1, sizeof( Uint16 ), file.ptr.c );

        error = ( 1 != retval );

        *val = ENDIAN_TO_SYS_INT16( itmp );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_readULE16( file.ptr.p, val );

        error = ( 0 == retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }

    if ( error ) _vfs_translate_error( &file );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Sint32( vfs_FILE& file, Sint32 * val )
{
    int retval;
	bool error = false;

    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        Uint32 itmp;
        retval = fread( &itmp, 1, sizeof( Uint32 ), file.ptr.c );

        error = ( 1 != retval );

        *val = ENDIAN_TO_SYS_INT32( itmp );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_readSLE32( file.ptr.p, val );

        error = ( 0 == retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }

    if ( error ) _vfs_translate_error( &file );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Uint32( vfs_FILE& file, Uint32 * val )
{
    int retval;
	bool error = false;

    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        Uint32 itmp;
        retval = fread( &itmp, 1, sizeof( Uint32 ), file.ptr.c );

        error = ( 1 != retval );

        *val = ENDIAN_TO_SYS_INT32( itmp );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_readULE32( file.ptr.p, val );

        error = ( 0 == retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }

    if ( error ) _vfs_translate_error( &file );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Sint64( vfs_FILE& file, Sint64 * val )
{
    int retval;
	bool error = false;

    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        Uint64 itmp;
        retval = fread( &itmp, 1, sizeof( Uint64 ), file.ptr.c );

        error = ( 1 != retval );

        *val = ENDIAN_TO_SYS_INT64( itmp );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_readSLE64( file.ptr.p, (PHYSFS_sint64*)val );

        error = ( 0 == retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }

    if ( error ) _vfs_translate_error( &file );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Uint64( vfs_FILE& file, Uint64 * val )
{
    int retval;
	bool error = false;

    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        Uint64 itmp;
        retval = fread( &itmp, 1, sizeof( Uint64 ), file.ptr.c );

        error = ( 1 != retval );

        *val = ENDIAN_TO_SYS_INT64( itmp );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_readULE64( file.ptr.p, (PHYSFS_uint64 *)val );

        error = ( 0 == retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }

    if ( error ) _vfs_translate_error( &file );

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_float( vfs_FILE& file, float * val )
{
    int retval;
	bool error = false;

    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        float ftmp;
        retval = fread( &ftmp, 1, sizeof( float ), file.ptr.c );

        error = ( 1 != retval );

        *val = ENDIAN_TO_SYS_IEEE32( ftmp );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        union { float f; Uint32 i; } convert;
        retval = PHYSFS_readULE32( file.ptr.p, &( convert.i ) );

        error = ( 0 == retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;

        *val = convert.f;
    }

    if ( error ) _vfs_translate_error( &file );

    return retval;
}

//--------------------------------------------------------------------------------------------


template <>
int vfs_write<Sint8>( vfs_FILE& file, const Sint8& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
   
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        retval = fwrite( &val, 1, sizeof( Sint8 ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_write(file.ptr.p, &val, 1, sizeof(Sint8));
        
        error = ( 1 != retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

template <>
int vfs_write<Uint8>( vfs_FILE& file, const Uint8& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        retval = fwrite( &val, 1, sizeof( Uint8 ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_write(file.ptr.p, &val, 1, sizeof(Sint8));
        
        error = ( 1 != retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

template <>
int vfs_write<Sint16>( vfs_FILE& file, const Sint16& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
    
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        Sint16 itmp = ENDIAN_TO_FILE_INT16(val);
        retval = fwrite( &itmp, 1, sizeof( Sint16 ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_writeSLE16( file.ptr.p, val );
        
        error = ( 0 == retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

template <>
int vfs_write<Uint16>( vfs_FILE& file, const Uint16& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
   
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        Uint16 itmp = ENDIAN_TO_FILE_INT16(val);
        retval = fwrite( &itmp, 1, sizeof( Uint16 ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_writeULE16( file.ptr.p, val );
        
        error = ( 0 == retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

template <>
int vfs_write<Sint32>( vfs_FILE& file, const Sint32& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
    
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        Sint32 itmp = ENDIAN_TO_FILE_INT32(val);
        retval = fwrite( &itmp, 1, sizeof( Sint32 ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_writeSLE32( file.ptr.p, val );
        
        error = ( 0 == retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

template <>
int vfs_write<Uint32>( vfs_FILE& file, const Uint32& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
    
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        Uint32 itmp = ENDIAN_TO_FILE_INT32(val);
        retval = fwrite( &itmp, 1, sizeof( Uint32 ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_writeULE32( file.ptr.p, val );
        
        error = ( 0 == retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

template <>
int vfs_write<Sint64>( vfs_FILE& file, const Sint64& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
   
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        Sint64 itmp = ENDIAN_TO_FILE_INT64(val);
        retval = fwrite( &itmp, 1, sizeof( Sint64 ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_writeSLE64( file.ptr.p, val );
        
        error = ( 0 == retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

template <>
int vfs_write<Uint64>( vfs_FILE& file, const Uint64& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();  
    
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        Uint64 itmp = ENDIAN_TO_FILE_INT64(val);
        retval = fwrite( &itmp, 1, sizeof( Uint64 ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_writeULE64( file.ptr.p, val );
        
        error = ( 0 == retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

template <>
int vfs_write<float>( vfs_FILE& file, const float& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
    
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        float ftmp = ENDIAN_TO_FILE_IEEE32(val);
        retval = fread( &ftmp, 1, sizeof( float ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        union { float f; Uint32 i; } convert;
        convert.f = val;
        retval = PHYSFS_writeULE32( file.ptr.p, convert.i );
        
        error = ( 0 == retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static int fake_physfs_vscanf_read_char( PHYSFS_File * pfile )
{
    unsigned char ch;
    int retval = PHYSFS_read( pfile, &ch, sizeof( unsigned char ), 1);
	if (1 != retval) return -1;
	return ch;
}

static void fake_physfs_vscanf_eat_whitespace( PHYSFS_File * pfile )
{
    int ch;
    do
    {
        ch = fake_physfs_vscanf_read_char( pfile );
    } while ( ch == -1 || isspace( ch ) );
    PHYSFS_seek( pfile, PHYSFS_tell( pfile ) - 1 );
}

static void fake_physfs_vscanf_read_word( PHYSFS_File * pfile, char * buffer, const int max_length )
{
    int length = 0;
    while ( max_length <= 0 || length < max_length )
    {
        int ch = fake_physfs_vscanf_read_char( pfile );
        if ( ch == -1 || isspace( ch ) )
        {
            PHYSFS_seek( pfile, PHYSFS_tell( pfile ) - 1 );
            break;
        }
        buffer[length] = (char)(ch);
        length++;
    }
    buffer[length] = CSTR_END;
}

int fake_physfs_vscanf( PHYSFS_File * pfile, const char *format, va_list args )
{
    int argcount = 0;
    const char * format_start = format;
    const char * format_end = format + strlen(format);
    
    while ( format < format_end )
    {
        char format_tmp = *format;
        format++;
        
        if ( format_tmp == ' ' )
        {
            fake_physfs_vscanf_eat_whitespace( pfile );
        }
        else if ( format_tmp == '%' )
        {
            STRING buffer = EMPTY_CSTR;
            int buffer_size = 0;
            int max_width = -1;
            char format_spec = CSTR_END;
            bool ignore_argument = false;
            bool invalid = false;
            
            if ( format >= format_end ) break;
            
            if ( *format == '*' )
            {
                ignore_argument = true;
                format++;
            }
            if ( format >= format_end ) break;
            
            while ( isdigit( *format ) && buffer_size < sizeof(buffer) )
            {
                buffer[buffer_size] = *format;
                buffer_size++;
                format++;
            }
            if ( format >= format_end ) break;
            
            if ( buffer_size )
            {
                buffer[buffer_size] = CSTR_END;
                max_width = strtol(buffer, NULL, 10);
                
                buffer_size = 0;
                buffer[buffer_size] = CSTR_END;
            }
            
            while ( format < format_end && format_spec == CSTR_END && !invalid )
            {
                char spec = *format;
                format++;
                
                switch (spec)
                {
                        // specifiers
                    case '%':
                    case 'c':
                    case 's':
                    case 'd':
                    case 'f':
                        format_spec = spec;
                        break;
                        
                        // length modifiers
                        
                        // invalid?
                    default:
                        invalid = true;
						std::ostringstream os;
						os << "invalid format specifier `%" << format_start << "` at `" << (format - 1) << "`" << std::endl;
                        Log::get().error("%s",os.str().c_str());
						throw std::runtime_error(os.str());
                }
            }
            
            EGOBOO_ASSERT( format < format_end || format_spec != CSTR_END );
            
            if ( 'c' == format_spec )
            {
                int ch = fake_physfs_vscanf_read_char( pfile );
                if ( !ignore_argument )
                {
                    char * arg_ptr = va_arg( args, char * );
                    *arg_ptr = ch;
                    argcount++;
                }
            }
            else
            {
                fake_physfs_vscanf_eat_whitespace( pfile );
                if ( '%' == format_spec )
                {
                    int tmp = fake_physfs_vscanf_read_char( pfile );
                    if ( '%' != tmp ) break;
                }
                else if ( 's' == format_spec )
                {
                    fake_physfs_vscanf_read_word( pfile, buffer, max_width );
                    if ( !ignore_argument )
                    {
                        char * arg_ptr = va_arg( args, char * );
                        int length = strlen( buffer );
                        strncpy( arg_ptr, buffer, length );
                        arg_ptr[length] = CSTR_END;
                        argcount++;
                    }
                }
                else if ( 'd' == format_spec )
                {
                    char * buffer_end;
                    int arg;
                    fake_physfs_vscanf_read_word( pfile, buffer, sizeof(buffer) );
                    buffer_size = strlen( buffer );
                    arg = strtol( buffer, &buffer_end, 10 );
                    PHYSFS_seek( pfile, PHYSFS_tell( pfile ) - (buffer_size - (buffer_end - buffer)) );
                    if ( !ignore_argument )
                    {
                        int * arg_ptr = va_arg( args, int * );
                        *arg_ptr = arg;
                        argcount++;
                    }
                }
                else if ( 'f' == format_spec )
                {
                    char * buffer_end;
                    float arg;
                    fake_physfs_vscanf_read_word( pfile, buffer, sizeof(buffer) );
                    buffer_size = strlen( buffer );
                    arg = strtof( buffer, &buffer_end );
                    PHYSFS_seek( pfile, PHYSFS_tell( pfile ) - (buffer_size - (buffer_end - buffer)) );
                    if ( !ignore_argument )
                    {
                        float * arg_ptr = va_arg( args, float * );
                        *arg_ptr = arg;
                        argcount++;
                    }
                }
            }
        }
        else
        {
            int pfile_in = fake_physfs_vscanf_read_char( pfile );
            if ( pfile_in != format_tmp )
                break;
        }
    }
    
    return argcount;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int fake_physfs_vprintf( PHYSFS_File * pfile, const char *format, va_list args )
{
    // fake an actual streaming write to the file by writing the string to a
    // "large" buffer

    int written;
    char buffer[4098] = EMPTY_CSTR;

    BAIL_IF_NOT_INIT();

    if ( NULL == pfile || INVALID_CSTR( format ) ) return 0;

    written = vsnprintf( buffer, SDL_arraysize( buffer ), format, args );

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

    BAIL_IF_NOT_INIT();

    if ( NULL == pfile ) return 0;

    retval = 0;
    va_start( args, format );
    if ( VFS_FILE_TYPE_CSTDIO == pfile->type )
    {
        retval = vfprintf( pfile->ptr.c, format, args );
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

    BAIL_IF_NOT_INIT();

    if ( NULL == pfile ) return 0;

    retval = 0;
    va_start( args, format );
    if ( VFS_FILE_TYPE_CSTDIO == pfile->type )
    {
        retval = vfscanf( pfile->ptr.c, format, args );
    }
    else if( VFS_FILE_TYPE_PHYSFS == pfile->type )
    {
        retval = fake_physfs_vscanf( pfile->ptr.p, format, args );
    }
    va_end( args );

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
char ** vfs_enumerateFiles( const char * dir_name )
{
    BAIL_IF_NOT_INIT();

    return PHYSFS_enumerateFiles( vfs_convert_fname( dir_name ) );
}

//--------------------------------------------------------------------------------------------
void    vfs_freeList( void * listVar )
{
    BAIL_IF_NOT_INIT();

    PHYSFS_freeList( listVar );
}

//--------------------------------------------------------------------------------------------
void _vfs_findClose( vfs_search_context_t * ctxt )
{
    BAIL_IF_NOT_INIT();

    if ( NULL == ctxt ) return;

    if ( NULL != ctxt->file_list )
    {
        PHYSFS_freeList( ctxt->file_list );
        ctxt->file_list = NULL;
    }
    ctxt->ptr       = NULL;
}

//--------------------------------------------------------------------------------------------
vfs_search_context_t * _vfs_search( vfs_search_context_t ** pctxt )
{
    const char * retval = NULL;
    static VFS_PATH  path_buffer = EMPTY_CSTR;

    BAIL_IF_NOT_INIT();

    if ( NULL == pctxt ) return NULL;

    // uninitialized file list?
    if ( NULL == ( *pctxt ) || NULL == ( *pctxt )->file_list )
    {
        ( *pctxt )->found[0] = CSTR_END;
        return ( *pctxt );
    }

    // emptry file list?
    if ( NULL == *(( *pctxt )->file_list ) )
    {
        goto _vfs_search_file_error;
    }

    if ( NULL == ( *pctxt )->ptr )
    {
        // if we haven't begun the search yet, get started
        ( *pctxt )->ptr = ( *pctxt )->file_list;
    }
    else
    {
        ( *pctxt )->ptr++;
    }

    // NULL == *((*pctxt)->ptr) signals the end of the list
    // if we exhausted the list, reset everything
    if ( NULL == ( *pctxt )->ptr || NULL == *(( *pctxt )->ptr ) )
    {
        goto _vfs_search_file_error;
    }

    // search for the correct extension (if any)
    retval = NULL;
    if ( CSTR_END == *( *pctxt )->ext )
    {
        int  found;

        for ( /* nothing */; NULL != *(( *pctxt )->ptr ); ( *pctxt )->ptr++ )
        {
            int is_dir;
            char * loc_path;

            if ( INVALID_CSTR(( *pctxt )->path ) )
            {
                snprintf( path_buffer, SDL_arraysize( path_buffer ), NET_SLASH_STR "%s", *(( *pctxt )->ptr ) );
            }
            else
            {
                snprintf( path_buffer, SDL_arraysize( path_buffer ), "%s" NET_SLASH_STR "%s", ( *pctxt )->path, *(( *pctxt )->ptr ) );
            }

            loc_path = ( char * )vfs_convert_fname( path_buffer );

            // have we found the correct type of object?
            found  = VFS_FALSE;
            is_dir = vfs_isDirectory( loc_path );

            if ( 0 != ( VFS_SEARCH_FILE & ( *pctxt )->bits ) )
            {
                found = !is_dir;
            }
            else if ( 0 != ( VFS_SEARCH_DIR & ( *pctxt )->bits ) )
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
        size_t extension_length = strlen(( *pctxt )->ext );

        // scan through the list
        for ( /* nothing */; NULL != *(( *pctxt )->ptr ); ( *pctxt )->ptr++ )
        {
            int found, is_dir;
            size_t string_length;
            char * sztest;
            char * loc_path;

            //---- have we found the correct type of object?

            if ( INVALID_CSTR(( *pctxt )->path ) )
            {
                snprintf( path_buffer, SDL_arraysize( path_buffer ), NET_SLASH_STR "%s", *(( *pctxt )->ptr ) );
            }
            else
            {
                snprintf( path_buffer, SDL_arraysize( path_buffer ), "%s" NET_SLASH_STR "%s", ( *pctxt )->path, *(( *pctxt )->ptr ) );
            }

            loc_path = ( char * )vfs_convert_fname( path_buffer );

            found = VFS_FALSE;
            is_dir = vfs_isDirectory( loc_path );
            if ( 0 != ( VFS_SEARCH_FILE & ( *pctxt )->bits ) )
            {
                found = !is_dir;
            }
            else if ( 0 != ( VFS_SEARCH_DIR & ( *pctxt )->bits ) )
            {
                found = is_dir;
            }
            else
            {
                found = VFS_TRUE;
            }

            if ( !found ) continue;

            //---- does the extension match?
            sztest = *(( *pctxt )->ptr );

            // get the length
            string_length = strlen( sztest );

            // grab the last bit of the test string
            if ((( signed )string_length - ( signed )extension_length ) >= 0 )
            {
                sztest += ( string_length - extension_length );
            }
            else
            {
                sztest = NULL;
            }
            if ( INVALID_CSTR( sztest ) ) continue;

            if ( 0 == strcmp( sztest, ( *pctxt )->ext ) )
            {
                retval = loc_path;
                break;
            }
        };
    }

    // reset the path buffer
    path_buffer[0] = CSTR_END;

    // test for the end condition again
    if ( NULL == ( *pctxt )->ptr || NULL == *(( *pctxt )->ptr ) )
    {
        vfs_findClose( pctxt );
        retval = NULL;
    }
    else
    {
        if ( 0 != ( VFS_SEARCH_BARE & ( *pctxt )->bits ) )
        {
            // do the "bare" option
            retval = NULL;
            if ( VALID_CSTR( *(( *pctxt )->ptr ) ) )
            {
                strncpy( path_buffer, *(( *pctxt )->ptr ), SDL_arraysize( path_buffer ) );
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

    if ( NULL == retval )
    {
        if ( NULL != *pctxt )
        {
            ( *pctxt )->found[0] = CSTR_END;
        }
    }
    else
    {
        strncpy(( *pctxt )->found, path_buffer, SDL_arraysize(( *pctxt )->found ) );
    }

    return *pctxt;

_vfs_search_file_error:

    vfs_findClose( pctxt );
    return NULL;
}

//--------------------------------------------------------------------------------------------
vfs_search_context_t * vfs_findFirst( const char * search_path, const char * search_extension, Uint32 search_bits )
{
    BAIL_IF_NOT_INIT();

    // create the new context
	vfs_search_context_t *ctxt = new vfs_search_context_t();

    // grab all the files
    ctxt->file_list = vfs_enumerateFiles( vfs_convert_fname( search_path ) );
    ctxt->ptr       = NULL;

    // no search list generated
    if ( NULL == ctxt->file_list )
    {
        return NULL;
    }

    // empty search list
    if ( NULL == *( ctxt->file_list ) )
    {
        vfs_findClose( &ctxt );
        return NULL;
    }

    // set the search extension
    if ( INVALID_CSTR( search_extension ) )
    {
        ctxt->ext[0] = CSTR_END;
    }
    else
    {
        snprintf( ctxt->ext, SDL_arraysize( ctxt->ext ), ".%s", search_extension );
    }

    // set the search path
    if ( INVALID_CSTR( search_path ) )
    {
        ctxt->path[0] = CSTR_END;
    }
    else
    {
        strncpy( ctxt->path, search_path, SDL_arraysize( ctxt->path ) );
    }

    ctxt->bits = search_bits;

    ctxt = _vfs_search( &ctxt );

    return ctxt;
}

//--------------------------------------------------------------------------------------------
vfs_search_context_t * vfs_findNext( vfs_search_context_t ** pctxt )
{
    // if there are no files, return an error value

    BAIL_IF_NOT_INIT();

    if ( NULL == pctxt || NULL == *pctxt ) return NULL;

    if ( NULL == ( *pctxt )->file_list )
    {
        return NULL;
    }

    *pctxt = _vfs_search( pctxt );

    return *pctxt;
}

//--------------------------------------------------------------------------------------------
void vfs_findClose( vfs_search_context_t ** ctxt )
{
    BAIL_IF_NOT_INIT();

    if ( nullptr != ctxt )
    {
        _vfs_findClose( *ctxt );
        delete *ctxt;
		*ctxt = nullptr;
    }
}

//--------------------------------------------------------------------------------------------
int vfs_removeDirectoryAndContents( const char * dirname, int recursive )
{
    // buffer the directory delete through PHYSFS, so that we so not access functions that
    // we have no right to! :)

    const char *  write_dir;

    BAIL_IF_NOT_INIT();

    // make sure that this is a valid directory
    write_dir = vfs_resolveWriteFilename( dirname );
    if ( !fs_fileIsDirectory( write_dir ) ) return VFS_FALSE;

    fs_removeDirectoryAndContents( write_dir, recursive );

    return VFS_TRUE;
}

//--------------------------------------------------------------------------------------------
int vfs_copyFile( const std::string& source, const std::string& target)
{
    char *dataPtr;
    size_t length;
    if (vfs_readEntireFile(source, &dataPtr, &length)) {
        bool retval = vfs_writeEntireFile(target, dataPtr, length);
        std::free(dataPtr);
        return retval;
    }
    return false;
}

//--------------------------------------------------------------------------------------------
int vfs_copyDirectory( const char *sourceDir, const char *destDir )
{
    /// @author ZZ
    /// @details This function copies all files in a directory
    VFS_PATH srcPath = EMPTY_CSTR, destPath = EMPTY_CSTR;
    const char *fileName;

    //VFS_PATH     szDst = EMPTY_CSTR;
    //const char * real_dst;

    vfs_search_context_t * ctxt;

    BAIL_IF_NOT_INIT();

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
    //snprintf( szDst, SDL_arraysize( szDst ), "%s",  vfs_resolveWriteFilename( destDir ) );
    //real_dst = szDst;

    // List all the files in the directory
    ctxt = vfs_findFirst( vfs_convert_fname( sourceDir ), NULL, VFS_SEARCH_FILE | VFS_SEARCH_BARE );
    fileName = vfs_search_context_get_current( ctxt );

    while ( NULL != ctxt && VALID_CSTR( fileName ) )
    {
        // Ignore files that begin with a .
        if ( '.' != fileName[0] )
        {
            snprintf( srcPath, SDL_arraysize( srcPath ), "%s/%s", sourceDir, fileName );
            snprintf( destPath, SDL_arraysize( destPath ), "%s/%s", destDir, fileName );

            if ( !vfs_copyFile( srcPath, destPath ) )
            {
                Log::get().debug("%s:%d:%s:  failed to copy from \"%s\" to \"%s\" (%s)\n", \
					             __FILE__, __LINE__, __FUNCTION__, srcPath, destPath, vfs_getError() );
            }
        }
        ctxt = vfs_findNext( &ctxt );

        fileName = vfs_search_context_get_current( ctxt );
    }
    vfs_findClose( &ctxt );

    return VFS_TRUE;
}

//--------------------------------------------------------------------------------------------
int vfs_ungetc( int c, vfs_FILE * pfile )
{
    int retval = 0;

    BAIL_IF_NOT_INIT();

    if ( NULL == pfile ) return 0;

    if ( VFS_FILE_TYPE_CSTDIO == pfile->type )
    {
        retval = ungetc( c, pfile->ptr.c );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == pfile->type )
    {
        // fake it
        int seeked = PHYSFS_seek(pfile->ptr.p, PHYSFS_tell(pfile->ptr.p) - 1);
        retval = c;
        
        if (!seeked) pfile->flags |= VFS_FILE_FLAG_ERROR;
        else         pfile->flags &= ~VFS_FILE_FLAG_ERROR;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_getc(vfs_FILE *file)
{
    int retval = 0;

    BAIL_IF_NOT_INIT();

    if (!file)
    {
        return 0;
    }

    retval = 0;
    if (VFS_FILE_TYPE_CSTDIO == file->type)
    {
        retval = fgetc(file->ptr.c);
        if (EOF == retval)
        {
            file->flags |= VFS_FILE_FLAG_EOF;
        }
    }
    else if (VFS_FILE_TYPE_PHYSFS == file->type)
    {
        unsigned char cTmp;
        retval = PHYSFS_read(file->ptr.p, &cTmp, sizeof(cTmp), 1);

        if (-1 == retval)
        {
            file->flags |= VFS_FILE_FLAG_ERROR;
            retval = EOF; // MH: Set this explicitly - EOF can be defined as -1, but it does not have.
        }
        else if (0 == retval)
        {
            file->flags |= VFS_FILE_FLAG_EOF;
            retval = EOF;
        }
        else
        {
            retval = cTmp;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_putc(int c, vfs_FILE *file)
{
    int retval = 0;

    BAIL_IF_NOT_INIT();

    if (NULL == file)
    {
        return 0;
    }

    if (VFS_FILE_TYPE_CSTDIO == file->type)
    {
        retval = fputc(c, file->ptr.c);
    }
    else if (VFS_FILE_TYPE_PHYSFS == file->type)
    {
        EGOBOO_ASSERT(0 <= c && c <= 0xff);
        unsigned char ch = static_cast<unsigned char>(c);
        retval = PHYSFS_write(file->ptr.p, &ch, 1, sizeof(unsigned char));
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_puts( const char * str , vfs_FILE * pfile )
{
    int retval = 0;

    BAIL_IF_NOT_INIT();

    if ( NULL == pfile || INVALID_CSTR( str ) ) return 0;

    if ( VFS_FILE_TYPE_CSTDIO == pfile->type )
    {
        retval = fputs( str, pfile->ptr.c );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == pfile->type )
    {
        size_t len = strlen( str );

        retval = vfs_write( str, len, sizeof( char ), pfile );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
char * vfs_gets(char *buffer, int buffer_size, vfs_FILE *file)
{
    char *retval = NULL;

    BAIL_IF_NOT_INIT();

    // According to gets spec, both cases are undefined behavior.
    if (!file || !buffer)
    {
        return NULL;
    }
    // Short read.
    if (0 == buffer_size)
    {
        return buffer;
    }
    if (VFS_FILE_TYPE_CSTDIO == file->type)
    {
        retval = fgets(buffer, buffer_size, file->ptr.c);
        if (feof(file->ptr.c))
        {
            file->flags |= VFS_FILE_FLAG_EOF;
            // retval is NULL if nothing was read and not NULL if something was read.
            return retval;
        }
        if (!retval)
        {
            file->flags |= VFS_FILE_FLAG_ERROR;
            // ferror(file->ptr.c) should return non-zero
            return NULL;
        }
    }
    else if (VFS_FILE_TYPE_PHYSFS == file->type)
    {
        char *str_ptr = buffer;
        char *str_end = buffer + buffer_size;

        int cTmp;
        int iTmp = PHYSFS_read(file->ptr.p, &cTmp, 1, sizeof(cTmp));
        if (-1 == iTmp)
        {
            file->flags |= VFS_FILE_FLAG_ERROR;
            return NULL;
        }
        if (0 == iTmp)
        {
            file->flags |= VFS_FILE_FLAG_EOF;
            return NULL;
        }
        while (iTmp && (str_ptr < str_end - 1) && CSTR_END != cTmp && 0 == (file->flags & (VFS_FILE_FLAG_EOF | VFS_FILE_FLAG_ERROR)))
        {
            *str_ptr = cTmp;
            str_ptr++;

            if (C_LINEFEED_CHAR == cTmp || C_CARRIAGE_RETURN_CHAR == cTmp) break;

            iTmp = PHYSFS_read(file->ptr.p, &cTmp, 1, sizeof(cTmp));

            if (-1 == iTmp)
            {
                file->flags |= VFS_FILE_FLAG_ERROR;
                return NULL;
            }
            if (0 == iTmp)
            {
                file->flags |= VFS_FILE_FLAG_EOF;
                return buffer;
            }
        }
        *str_ptr = CSTR_END;

        retval = buffer;
    }
    else
    {
        // Corrupted file handle.
        return NULL;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void vfs_empty_temp_directories()
{
    BAIL_IF_NOT_INIT();

    vfs_removeDirectoryAndContents("import", VFS_TRUE);
    vfs_removeDirectoryAndContents("remote", VFS_TRUE);
}

//--------------------------------------------------------------------------------------------
int vfs_rewind(vfs_FILE *file)
{
    BAIL_IF_NOT_INIT();

    if (!file)
    {
        return 0;
    }

    return vfs_seek(file, 0);
}

//--------------------------------------------------------------------------------------------
void _vfs_translate_error(vfs_FILE *file)
{
    BAIL_IF_NOT_INIT();

    if (!file)
    {
        return;
    }

    if (VFS_FILE_TYPE_CSTDIO == file->type)
    {
        if (ferror(file->ptr.c))
        {
            SET_BIT(file->flags, VFS_FILE_FLAG_ERROR);
        }

        if (feof(file->ptr.c))
        {
            SET_BIT(file->flags, VFS_FILE_FLAG_EOF);
        }
    }
    else if (VFS_FILE_TYPE_PHYSFS == file->type)
    {
        if (PHYSFS_eof(file->ptr.p))
        {
            SET_BIT(file->flags, VFS_FILE_FLAG_EOF);
        }
    }
}

//--------------------------------------------------------------------------------------------
const char * vfs_getError( void )
{
    /// @author ZF
    /// @details Returns the last error the PHYSFS system reported.

    static char errors[1024];
    const char * physfs_error, * file_error;
	//bool is_error;

    BAIL_IF_NOT_INIT();

    // load up a default
    strncpy( errors, "unknown error", SDL_arraysize( errors ) );

    // assume no error
    //is_error = false;

    // try to get the physfs error state;
    physfs_error = PHYSFS_getLastError();
    if ( NULL == physfs_error ) physfs_error = "no error";

    // try to get the stdio error state
    file_error = strerror( errno );

    snprintf( errors, SDL_arraysize( errors ), "c stdio says:\"%s\" -- physfs says:\"%s\"", file_error, physfs_error );

    return errors;
}

//--------------------------------------------------------------------------------------------
int vfs_add_mount_point( const char * root_path, const char * relative_path, const char * mount_point, int append )
{
    /// @author BB
    /// @details a wrapper for PHYSFS_mount

    int retval = -1;
    const char * loc_dirname;
    VFS_PATH     dirname;

    BAIL_IF_NOT_INIT();

    // a bare slash is taken to mean the PHYSFS root directory, not the root of the currently mounted volume
    if ( !VALID_CSTR( mount_point ) || 0 == strcmp( mount_point, C_SLASH_STR ) ) return 0;

    // make a complete version of the pathname
    if ( VALID_CSTR( root_path ) && VALID_CSTR( relative_path ) )
    {
        snprintf( dirname, SDL_arraysize( dirname ), "%s" SLASH_STR "%s", root_path, relative_path );
    }
    else if ( VALID_CSTR( root_path ) )
    {
        strncpy( dirname, root_path, SDL_arraysize( dirname ) );
    }
    else if ( VALID_CSTR( relative_path ) )
    {
        strncpy( dirname, relative_path, SDL_arraysize( dirname ) );
    }
    else
    {
        return 0;
    }

    /// @note ZF@> 2010-06-30 vfs_convert_fname_sys() broke the Linux version
    /// @note BB@> 2010-06-30 the error in vfs_convert_fname_sys() might be fixed now
    /// @note PF@> 2015-01-01 this should be unneeded. root_path and relative_path should both
    ///                       sys-dependent paths, unless Windows does something strange?
#if 0
    loc_dirname = vfs_convert_fname_sys( dirname );
#else
    loc_dirname = dirname;
#endif

    if ( _vfs_mount_info_add( mount_point, root_path, relative_path ) )
    {
        retval = PHYSFS_mount( loc_dirname, mount_point, append );
        if ( 0 == retval )
        {
            // go back and remove the mount info, since PHYSFS rejected the
            // data we gave it
            int i = _vfs_mount_info_matches( mount_point, loc_dirname );
            _vfs_mount_info_remove( i );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_remove_mount_point( const char * mount_point )
{
    /// @author BB
    /// @details Remove every single search path related to the given mount point.

    int retval, cnt;

    BAIL_IF_NOT_INIT();

    // don't allow it to remove the default directory
    if ( !VALID_CSTR( mount_point ) ) return 0;
    if ( 0 == strcmp( mount_point, C_SLASH_STR ) ) return 0;

    // assume we are going to fail
    retval = 0;

    // see if we have the mount point
    cnt = _vfs_mount_info_matches( mount_point, NULL );

    // does it exist in the list?
    if ( cnt < 0 ) return false;

    while ( cnt >= 0 )
    {
        // we have to use the path name to remove the search path, not the mount point name
        PHYSFS_removeFromSearchPath( _vfs_mount_info[cnt].full_path );

        // remove the mount info from this index
        // PF> we remove it even if PHYSFS_removeFromSearchPath() fails or else we might get an infinite loop
        _vfs_mount_info_remove( cnt );

        cnt = _vfs_mount_info_matches( mount_point, NULL );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char * vfs_search_context_get_current( vfs_search_context_t * ctxt )
{
    BAIL_IF_NOT_INIT();

    if ( NULL == ctxt ) return NULL;

    return ctxt->found;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int _vfs_mount_info_search( const char * some_path )
{
    /// @author BB
    /// @details check to see if the given path is actually relative to a registered
    ///               virtual mount point

    int cnt, retval = VFS_FALSE;
    VFS_PATH temp_path;

    BAIL_IF_NOT_INIT();

    if ( !VALID_CSTR( some_path ) ) return retval;

    for ( cnt = 0; cnt < _vfs_mount_info_count; cnt++ )
    {
        int len;

        if ( 0 == strcmp( _vfs_mount_info[cnt].mount, some_path ) )
        {
            retval = VFS_TRUE;
            break;
        }

        snprintf( temp_path, SDL_arraysize( temp_path ), "%s" NET_SLASH_STR, _vfs_mount_info[cnt].mount );
        len = strlen( temp_path );

        if ( 0 == strncmp( temp_path, some_path, len ) )
        {
            retval = VFS_TRUE;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char * vfs_mount_info_strip_path( const char * some_path )
{
    int cnt;
    size_t offset;
    const char * ptmp, * stripped_pos;

    BAIL_IF_NOT_INIT();

    stripped_pos = some_path;

    // strip any starting slashes
    for ( ptmp = some_path; ( CSTR_END != *ptmp ) && ptmp < some_path + VFS_MAX_PATH; ptmp++ )
    {
        if ( NET_SLASH_CHR != *ptmp && WIN32_SLASH_CHR != *ptmp )
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
int _vfs_mount_info_matches( const char * mount_point, const char * local_path )
{
    int cnt, retval;
    const char * ptmp;

    BAIL_IF_NOT_INIT();

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
            if (( NET_SLASH_CHR != *ptmp && WIN32_SLASH_CHR != *ptmp ) || CSTR_END == *ptmp )
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
            if ( 0 == strncmp( _vfs_mount_info[cnt].mount,     mount_point, VFS_MAX_PATH ) &&
                 0 == strncmp( _vfs_mount_info[cnt].full_path, local_path,  VFS_MAX_PATH ) )
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
            if ( 0 == strncmp( _vfs_mount_info[cnt].full_path, local_path, VFS_MAX_PATH ) )
            {
                retval = cnt;
                break;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool _vfs_mount_info_add(const char * mount_point, const char * root_path, const char * relative_path)
{
    const char * ptmp;

    VFS_PATH     local_path;

    BAIL_IF_NOT_INIT();

    // can we add it?
    if ( _vfs_mount_info_count >= MAX_MOUNTINFO ) return false;

    // if the mount point is not a string, do nothing
    if ( !VALID_CSTR( mount_point ) ) return false;

    // make a complete version of the pathname
    if ( VALID_CSTR( root_path ) && VALID_CSTR( relative_path ) )
    {
        snprintf( local_path, SDL_arraysize( local_path ), "%s" SLASH_STR "%s", root_path, relative_path );
    }
    else if ( VALID_CSTR( root_path ) )
    {
        strncpy( local_path, root_path, SDL_arraysize( local_path ) );
    }
    else if ( VALID_CSTR( relative_path ) )
    {
        strncpy( local_path, relative_path, SDL_arraysize( local_path ) );
    }
    else
    {
        return false;
    }

    // do we want to add it?
    if ( !VALID_CSTR( local_path ) ) return false;

    if ( _vfs_mount_info_matches( mount_point, local_path ) >= 0 ) return false;

    // strip any starting slashes
    for ( ptmp = mount_point; ptmp < mount_point + VFS_MAX_PATH; ptmp++ )
    {
        if (( NET_SLASH_CHR != *ptmp && WIN32_SLASH_CHR != *ptmp ) || CSTR_END == *ptmp )
        {
            break;
        }
    }

    if ( CSTR_END == *ptmp ) return false;

    // save the mount point in a list for later detection
    strncpy( _vfs_mount_info[_vfs_mount_info_count].mount,     ptmp,       VFS_MAX_PATH );
    strncpy( _vfs_mount_info[_vfs_mount_info_count].full_path, local_path, VFS_MAX_PATH );

    _vfs_mount_info[_vfs_mount_info_count].root_path[0] = CSTR_END;
    if ( VALID_CSTR( root_path ) )
    {
        strncpy( _vfs_mount_info[_vfs_mount_info_count].root_path, root_path, VFS_MAX_PATH );
    }

    _vfs_mount_info[_vfs_mount_info_count].relative_path[0] = CSTR_END;
    if ( VALID_CSTR( relative_path ) )
    {
        strncpy( _vfs_mount_info[_vfs_mount_info_count].relative_path, relative_path, VFS_MAX_PATH );
    }

    _vfs_mount_info_count++;

    return true;
}

//--------------------------------------------------------------------------------------------
bool _vfs_mount_info_remove(int cnt)
{
    BAIL_IF_NOT_INIT();

    // does it exist in the list?
    if ( cnt < 0 || cnt > _vfs_mount_info_count ) return false;

    // fill in the hole in the list
    if ( _vfs_mount_info_count > 1 )
    {
        memmove( _vfs_mount_info + cnt, _vfs_mount_info + ( _vfs_mount_info_count - 1 ), sizeof( vfs_path_data_t ) );
    }

    // shorten the list
    _vfs_mount_info_count--;

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void vfs_set_base_search_paths( void )
{
    BAIL_IF_NOT_INIT();

    // Put write dir first in search path...
    PHYSFS_addToSearchPath( fs_getUserDirectory(), 0 );

    // Put base path on search path...
    PHYSFS_addToSearchPath( fs_getDataDirectory(), 1 );
    
    // Put config path on search path...
    PHYSFS_addToSearchPath(fs_getConfigDirectory(), 1);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool vfs_readEntireFile(const std::string& pathname, char **data, size_t *length) {
    if (!data || !length) {
        return false;
    }
    vfs_FILE *file = vfs_openRead(pathname);
    if (!file) {
        return false;
    }
    long fileLen = vfs_fileLength(file);
    
    if (fileLen == -1)
    {
        // file length isn't known
        size_t pos = 0;
        size_t bufferSize = 1024;
        char *buffer = (char *) malloc(bufferSize);
        if (buffer == nullptr)
        {
            vfs_close(file);
            return false;
        }
        while (!vfs_eof(file))
        {
            size_t read = vfs_read(buffer + pos, 1, bufferSize - pos, file);
            pos += read;
            if (vfs_error(file))
            {
                free(buffer);
                vfs_close(file);
                return false;
            }
            if (vfs_eof(file)) break;
            if (0 == read) continue;
            char *newBuffer = (char *)realloc(buffer, pos + 1024);
            if (newBuffer == nullptr)
            {
                free(buffer);
                vfs_close(file);
                return false;
            }
            buffer = newBuffer;
        }
        *data = buffer;
        *length = pos;
    }
    else
    {
        size_t pos = 0;
        char *buffer = (char *) malloc(fileLen);
        if (buffer == nullptr)
        {
            vfs_close(file);
            return false;
        }
        while (pos < fileLen)
        {
            size_t read = vfs_read(buffer + pos, 1, fileLen - pos, file);
            pos += read;
            if (vfs_error(file))
            {
                free(buffer);
                vfs_close(file);
                return false;
            }
            if (vfs_eof(file)) break;
        }
        *data = buffer;
        *length = pos;
    }
    
    vfs_close(file);
    return true;
}

//--------------------------------------------------------------------------------------------
bool vfs_writeEntireFile(const std::string& pathname, const char *data, const size_t length)
{
    if (!data) {
        return false;
    }
    vfs_FILE *pfile = vfs_openWrite(pathname);
    if (nullptr == pfile) {
        return false;
    }
    size_t pos = 0;
    while (pos < length) {
        size_t written = vfs_write(data + pos, 1, length - pos, pfile);
        pos += written;
        if (vfs_error(pfile))
        {
            vfs_close(pfile);
            return false;
        }
    }
    vfs_close(pfile);
    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static int64_t vfs_rwops_size(SDL_RWops *context)
{
    vfs_FILE * pfile = static_cast<vfs_FILE *>(context->hidden.unknown.data1);
    return vfs_fileLength(pfile);
}

static int64_t vfs_rwops_seek( SDL_RWops * context, int64_t offset, int whence )
{
    vfs_FILE * pfile = static_cast<vfs_FILE *>(context->hidden.unknown.data1);
    long pos = vfs_tell(pfile);
    if (SEEK_CUR == whence)
    {
        pos += offset;
    }
    else if (SEEK_END == whence)
    {
        pos = vfs_fileLength(pfile) + offset;
    }
    else if (SEEK_SET == whence)
    {
        pos = offset;
    }
    vfs_seek(pfile, pos);
    return vfs_tell( pfile );
}

static size_t vfs_rwops_read(SDL_RWops *context, void *ptr, size_t size, size_t maxnum)
{
    vfs_FILE *file = (vfs_FILE *)(context->hidden.unknown.data1);
    if (vfs_isReading(file) != 1)
    {
        return 0;
    }
    return vfs_read(ptr, size, maxnum, file);
}

static size_t vfs_rwops_write(SDL_RWops *context, const void *ptr, size_t size, size_t num)
{
    vfs_FILE *file = (vfs_FILE *)(context->hidden.unknown.data1);
    if (vfs_isWriting(file) != 1)
    {
        return 0;
    }
    return vfs_write(ptr, size, num, file);
}

static int vfs_rwops_close(SDL_RWops *context)
{
    vfs_FILE *file = (vfs_FILE *)(context->hidden.unknown.data1);
    if (context->type)
    {
        vfs_close(file);
    }
    free(context);
    return 0;
}

static SDL_RWops *vfs_rwops_create(vfs_FILE *file, bool ownership)
{
    int isWriting = vfs_isWriting(file);
    if (-1 == isWriting)
    {
        return NULL;
    }
    // MH: I allocate the boolean variable tracking ownership after the SDL_RWops struct.
    // PF5: It's been moved to the type variable.
    SDL_RWops *rwops = (SDL_RWops *)malloc(sizeof(SDL_RWops));
    if (!rwops)
    {
        return NULL;
    }
    rwops->type = ownership;
    rwops->size = vfs_rwops_size;
    rwops->seek = vfs_rwops_seek;
    rwops->read = vfs_rwops_read;
    rwops->write = vfs_rwops_write;
    rwops->close = vfs_rwops_close;
    rwops->hidden.unknown.data1 = file;
    return rwops;
}

SDL_RWops *vfs_openRWops(vfs_FILE *file, bool ownership)
{
    SDL_RWops *rwops = vfs_rwops_create(file, ownership);
    if (!rwops) {
        return nullptr;
    }
    return rwops;
}

SDL_RWops *vfs_openRWopsRead(const std::string& pathname)
{
    vfs_FILE *file = vfs_openRead(pathname);
    if (!file) {
        return nullptr;
    }
    SDL_RWops *rwops = vfs_rwops_create(file, true);
    if (!rwops) {
        vfs_close(file);
        return nullptr;
    }
    return rwops;
}

SDL_RWops *vfs_openRWopsWrite(const std::string& pathname)
{
    vfs_FILE *file = vfs_openWrite(pathname);
    if (!file) {
        return nullptr;
    }
    SDL_RWops *rwops = vfs_rwops_create(file, true);
    if (!rwops) {
        vfs_close(file);
        return nullptr;
    }
    return rwops;
}

SDL_RWops *vfs_openRWopsAppend(const std::string& pathname)
{
    vfs_FILE *file = vfs_openAppend(pathname);
    if (!file) {
        return nullptr;
    }
    SDL_RWops *rwops = vfs_rwops_create(file, true);
    if (!rwops) {
        vfs_close(file);
        return nullptr;
    }
    return rwops;
}
