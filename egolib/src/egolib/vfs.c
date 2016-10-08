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

static void _vfs_exit();
static vfs_search_context_t *_vfs_search(vfs_search_context_t **ctxt);
static int _vfs_ensure_write_directory(const std::string& filename, bool is_directory);


static void _vfs_translate_error(vfs_FILE *file);

static bool _vfs_mount_info_add(const char *mount_point, const char *root_path, const char *relative_path);
static int _vfs_mount_info_matches(const char *mount_point, const char *local_path);
static bool _vfs_mount_info_remove(int cnt);
static int _vfs_mount_info_search(const std::string& pathname);


static int fake_physfs_vprintf(PHYSFS_File *file, const char *format, va_list args);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int vfs_init(const char *argv0, const char *root_dir)
{
    std::string temp_path;

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
    temp_path = std::string(fs_getDataDirectory()) + SLASH_STR;
    if (!PHYSFS_mount(temp_path.c_str(), "/", 1))
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
        temp_path = std::string(fs_getUserDirectory()) + "/debug";
        temp_path = str_convert_slash_sys(temp_path);
        fs_createDirectory(temp_path.c_str());
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
std::string vfs_getLinkedVersion()
{
    PHYSFS_Version version;
    PHYSFS_getLinkedVersion(&version);        // Linked version number
    std::stringstream stream;
    stream << version.major << "." << version.minor << "." << version.patch;
    return stream.str();
}

std::string vfs_getVersion()
{
    PHYSFS_Version version;
    PHYSFS_VERSION(&version);                 // Compiled version number
    std::stringstream stream;
    stream << version.major << "." << version.minor << "." << version.patch;
    return stream.str();
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
    if (!_vfs_ensure_write_directory(temporary, false))
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
std::string vfs_convert_fname( const std::string& fname )
{
    BAIL_IF_NOT_INIT();

    // test for a bad iput filename
    if ( fname  == "" )
    {
        return NET_SLASH_STR;
    }

    // make a copy of the original filename, in case fname is
    // a literal string or a pointer to local_fname
    std::string copy_fname = fname;
    std::string local_fname;
    if ( _vfs_mount_info_search( copy_fname.c_str() ) )
    {
        local_fname = copy_fname;
    }
    else if ( NET_SLASH_CHR == copy_fname[0] || WIN32_SLASH_CHR == copy_fname[0] )
    {
        local_fname = copy_fname;
    }
    else
    {
        local_fname = std::string(NET_SLASH_STR) + copy_fname;
    }

    return str_convert_slash_net( local_fname );
}

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
std::pair<bool, std::string> vfs_resolveReadFilename( const std::string& filename )
{
    BAIL_IF_NOT_INIT();

    if (filename == "") std::make_pair(false, filename);

    // make a temporary copy of the given filename with system-dependent slashes
    // to see if the filename is already resolved
    std::string filename_specific = str_convert_slash_sys(filename);

    if (fs_fileExists(filename_specific.c_str())) {
        return std::make_pair(true, filename_specific);
    }

    // The specified filename (in system-specific notation) does not exist.
    // Convert the filename in PhysFS-specific notation.
    filename_specific = vfs_convert_fname(filename_specific.c_str());

    // If the specified filename denotes an existing file or directory, then this file or directory must have a containing directory.
    const char *prefix = PHYSFS_getRealDir(filename_specific.c_str());
    if (nullptr == prefix) {
        return std::make_pair(false, filename);
    }

    // The specified filename denotes an existing file or directory.
    if (PHYSFS_isDirectory(filename_specific.c_str())) {
        // If it denotes a directory then it must be splittable into a prefix and a suffix.
        const char *suffix = vfs_mount_info_strip_path(filename_specific.c_str());
        if (nullptr != suffix) {
            return std::make_pair(true, str_convert_slash_sys(std::string(prefix) + "/" + suffix));
        } else {
            return std::make_pair(true, str_convert_slash_sys(std::string(prefix) + "/"));
        }
    } else {
        // The specified filename denotes a file.
        const char *suffix = vfs_mount_info_strip_path(filename_specific.c_str());
        if (nullptr != suffix) {
            return std::make_pair(true, str_convert_slash_sys(std::string(prefix) + "/" + suffix));
        } else {
            return std::make_pair(false, filename);
        }
    }
}

//--------------------------------------------------------------------------------------------
std::pair<bool, std::string> vfs_resolveWriteFilename(const std::string& filename) {
    // Validate state.
    BAIL_IF_NOT_INIT();
    // Validate arguments.
    if (filename == "") {
        return std::make_pair(false, filename);
    }
    // Get the write directory.
    const char *writeDirectory = PHYSFS_getWriteDir();
    if (!writeDirectory) {
        throw std::runtime_error("unable to get write directory");
    }
    // Append the filename to the write directory.
    auto resolvedFilename = std::string(writeDirectory) + SLASH_STR + filename;
    // Ensure system-specific encoding of the resolved filename.
    return std::make_pair(true, str_convert_slash_sys(resolvedFilename));
}

//--------------------------------------------------------------------------------------------
int _vfs_ensure_write_directory( const std::string& filename, bool is_directory )
{
    /// @author BB
    /// @details
    BAIL_IF_NOT_INIT();

    if ( filename == "" ) return 0;

    // make a working copy of the filename
    // and make sure that PHYSFS gets the filename with the slashes it wants
    std::string temp_dirname = vfs_convert_fname(filename.c_str());

    // grab the system-independent path relative to the write directory
    if ( !is_directory && !vfs_isDirectory( temp_dirname ) )
    {
        size_t index = temp_dirname.rfind(NET_SLASH_CHR);
        if (std::string::npos == index)
        {
            temp_dirname = NET_SLASH_STR;
        }
        else
        {
            temp_dirname = temp_dirname.substr(0, index);
        }

        if ( temp_dirname.length() == 0 )
        {
            temp_dirname = C_SLASH_STR;
        }
    }

    // call mkdir() on this directory. PHYSFS will automatically generate the
    // directories needed between the write directory and the specified directory
    int retval = 1;
    if ( temp_dirname != "" )
    {
        retval = vfs_mkdir( temp_dirname.c_str() );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------

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

vfs_search_context_t::vfs_search_context_t() :
    file_list_2(), file_list_iterator_2(),
    path_2(), extension_2(),
    bits(0), found_2() {
   /* Intentionally empty. */
}

vfs_search_context_t::~vfs_search_context_t() {
    BAIL_IF_NOT_INIT();
    this->file_list_iterator_2 = this->file_list_2.cend();
}

//--------------------------------------------------------------------------------------------
vfs_search_context_t * vfs_findFirst( const char * search_path, const char * search_extension, Uint32 search_bits )
{
    BAIL_IF_NOT_INIT();

    // create the new context
	vfs_search_context_t *ctxt = new vfs_search_context_t();

    // grab all the files
    char **file_list = PHYSFS_enumerateFiles(vfs_convert_fname(search_path).c_str());
    if (!file_list) {
        delete ctxt;
        throw std::runtime_error("unable to enumerate files");
    }
    char **file = file_list;
    for (; nullptr != *file; ++file) {
        try {
            ctxt->file_list_2.push_back(*file);
        } catch (...) {
            PHYSFS_freeList(file_list);
            std::rethrow_exception(std::current_exception());
        }
    }
    PHYSFS_freeList(file_list);

    // Set the search extension.
    if (INVALID_CSTR(search_extension)) {
        ctxt->extension_2 = "";
    } else {
        ctxt->extension_2 = std::string(".") + search_extension;
    }

    // Set the search path.
    if (INVALID_CSTR(search_path)) {
        ctxt->path_2 = "";
    } else {
        ctxt->path_2 = search_path;
    }

    // Set the search bits.
    ctxt->bits = search_bits;

    // Begin the iterator.
    try {
        ctxt->file_list_iterator_2 = ctxt->file_list_2.begin();
        // Search the first acceptable filename.
        for (; ctxt->file_list_iterator_2 != ctxt->file_list_2.cend(); ctxt->file_list_iterator_2++) {
            if (ctxt->predicate(*ctxt->file_list_iterator_2)) {
                break;
            }
        }
        if (ctxt->file_list_iterator_2 != ctxt->file_list_2.cend()) {
            if (0 != (VFS_SEARCH_BARE & ctxt->bits)) {
                ctxt->found_2 = *ctxt->file_list_iterator_2;
            } else {
                ctxt->found_2 = ctxt->makeAbsolute(*ctxt->file_list_iterator_2);
            }
        }
    } catch (...) {
        delete ctxt;
        std::rethrow_exception(std::current_exception());
    }
    return ctxt;
}

//--------------------------------------------------------------------------------------------
bool vfs_search_context_t::predicate(const std::string& pathname) const {
    auto pathname_absoluteSanitized = makeAbsolute(pathname);
    // Filter by type if desired.
    if (VFS_SEARCH_ALL != (this->bits & VFS_SEARCH_ALL)) {
        bool isDirectory = vfs_isDirectory(pathname_absoluteSanitized);
        bool isFile = !isDirectory;
        if (isFile) {
            return (VFS_SEARCH_FILE == (VFS_SEARCH_FILE & this->bits));
        }
        if (isDirectory) {
            return (VFS_SEARCH_DIR == (VFS_SEARCH_DIR & this->bits));
        }
    }

    // Filter by extension if desired.
    if (this->extension_2.length() > 0) {
        size_t extensionPointPosition = pathname_absoluteSanitized.rfind('.');
        if (extensionPointPosition == std::string::npos) {
            return false;
        }

        auto extensionString = pathname_absoluteSanitized.substr(extensionPointPosition + 1);
        if (extensionString != this->extension_2) {
            return false;
        }
    }
    return true;
}

std::string vfs_search_context_t::makeAbsolute(const std::string& pathname) const {
    auto pathname_absoluteSanitized = pathname;
    if (this->path_2.length() == 0) {
        // If no path was specified, prepend a slash.
        pathname_absoluteSanitized = std::string(NET_SLASH_STR) + pathname_absoluteSanitized;
    } else {
        // Otherwise: Prepend the specified path.
        pathname_absoluteSanitized = this->path_2 + NET_SLASH_STR + pathname_absoluteSanitized;
    }

    // Sanitize the path.
    pathname_absoluteSanitized = vfs_convert_fname(pathname_absoluteSanitized.c_str());

    // Return result.
    return pathname_absoluteSanitized;
}

void vfs_search_context_t::nextData()
{
    // if there are no files, return an error value

    BAIL_IF_NOT_INIT();

    // Hit the end? Nothing to do.
    if (this->file_list_iterator_2 == this->file_list_2.cend()) {
        return;
    }

    // Increment at least once. Increment until a file is accepted or the end is reached.
    while (true) {
        this->file_list_iterator_2++;
        if (this->file_list_iterator_2 == this->file_list_2.cend()) break;
        if (this->predicate(*this->file_list_iterator_2)) break;
    }

    // If no suitable file was found ...
    if (this->file_list_iterator_2 == this->file_list_2.cend()) {
        // ... return.
        this->found_2 = "";
        return;
    }

    if (0 != (VFS_SEARCH_BARE & this->bits)) {
        this->found_2 = *this->file_list_iterator_2;
    } else {
        this->found_2 = makeAbsolute(*this->file_list_iterator_2);
    }
}

//--------------------------------------------------------------------------------------------
int vfs_removeDirectoryAndContents(const char * dirname, int recursive) {
    // buffer the directory delete through PHYSFS, so that we so not access functions that
    // we have no right to! :)

    BAIL_IF_NOT_INIT();

    // make sure that this is a valid directory
    auto resolvedWriteFilename = vfs_resolveWriteFilename(dirname);
    if (!resolvedWriteFilename.first) return VFS_FALSE;
    if (!fs_fileIsDirectory(resolvedWriteFilename.second.c_str())) return VFS_FALSE;

    fs_removeDirectoryAndContents(resolvedWriteFilename.second.c_str(), recursive);

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
    ctxt = vfs_findFirst( vfs_convert_fname( sourceDir ).c_str(), NULL, VFS_SEARCH_FILE | VFS_SEARCH_BARE );
    if (!ctxt) return VFS_FALSE;
    while (ctxt->hasData())
    {
        auto fileName = ctxt->getData();
        // Ignore files that begin with a .
        if ( '.' != fileName[0] )
        {
            snprintf( srcPath, SDL_arraysize( srcPath ), "%s/%s", sourceDir, fileName.c_str() );
            snprintf( destPath, SDL_arraysize( destPath ), "%s/%s", destDir, fileName.c_str() );

            if ( !vfs_copyFile( srcPath, destPath ) )
            {
                Log::get().debug("%s:%d:%s:  failed to copy from \"%s\" to \"%s\" (%s)\n", \
					             __FILE__, __LINE__, __FUNCTION__, srcPath, destPath, vfs_getError() );
            }
        }
        ctxt->nextData();
    }
    delete ctxt;
    ctxt = nullptr;

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
bool vfs_search_context_t::hasData() const {
    return this->file_list_iterator_2 != this->file_list_2.cend();
}
const std::string& vfs_search_context_t::getData() const {
    return this->found_2;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// @brief Get if the specified path is equivalent to a virtual mount point.
/// @param pathname the pathname
/// @return #VFS_TRUE if @a pathname is equivalent to a virtual mount point, #VFS_FALSE otherwise
int _vfs_mount_info_search(const std::string& pathname) {
    BAIL_IF_NOT_INIT();

    if (pathname == "") return VFS_FALSE;

    // Get the sanitized pathname.
    auto sanitizedPathname = str_clean_path(pathname);

    for (auto cnt = 0; cnt < _vfs_mount_info_count; cnt++) {
        if (sanitizedPathname == _vfs_mount_info[cnt].mount) {
            return VFS_TRUE;
        }

        if (sanitizedPathname == (std::string(_vfs_mount_info[cnt].mount) + NET_SLASH_STR)) {
            return VFS_TRUE;
        }
    }

    return VFS_FALSE;
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

void vfs_readEntireFile(const std::string& pathname, std::function<void(size_t, const char *)> receive) {
    auto deleter = [](vfs_FILE *file) { if (file) vfs_close(file); };
    std::unique_ptr<vfs_FILE, decltype(deleter)> file(vfs_openRead(pathname), deleter);
    if (!file) {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "unable to open file `" + pathname + "` for reading");
    }
    // Read in 2048 Byte chunks.
    char buffer[2048];
    while (!vfs_eof(file.get())) {
        size_t read = vfs_read(buffer, 1, 2048, file.get());
        if (vfs_error(file.get())) {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "error while reading file `" + pathname + "`");
        }
        // If not a short read, invoke receive.
        if (0 != read) {
            receive(read, buffer);
        }
    }
    file = nullptr;
}

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
