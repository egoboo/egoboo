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
#include "egolib/Core/StringUtilities.hpp"

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
		throw std::runtime_error(os.str()); \
	}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

typedef char VFS_PATH[VFS_MAX_PATH];

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
    std::string mount;
    std::string full_path;
    std::string root_path;
    std::string relative_path;

    s_vfs_path_data()
        : mount(),
          full_path(),
          root_path(),
          relative_path() {}

    s_vfs_path_data(const s_vfs_path_data& other)
        : mount(other.mount),
          full_path(other.full_path),
          root_path(other.root_path),
          relative_path(other.relative_path) {}
    
    s_vfs_path_data& operator=(const s_vfs_path_data& other) {
        mount = other.mount;
        full_path = other.full_path;
        root_path = other.root_path;
        relative_path = other.relative_path;
        return *this;
    }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static std::vector<vfs_path_data_t> _vfs_mount_infos;
static bool _vfs_atexit_registered = false;
static bool _vfs_initialized = false;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void _vfs_exit();
static int _vfs_ensure_write_directory(const std::string& filename, bool is_directory);


static void _vfs_translate_error(vfs_FILE *file);

static bool _vfs_mount_info_add(const Ego::VfsPath& mountPoint, const std::string& rootPath, const std::string& relativePath);
static int _vfs_mount_info_matches(const Ego::VfsPath& mountPoint);
static int _vfs_mount_info_matches(const Ego::VfsPath& mountPoint, const std::string& localPath);
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
        Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "the data path ", "`", fs_getDataDirectory(), "`", " is not a directory", Log::EndOfEntry);
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
        printf("WARNING: cannot create write directory %s\n", fs_getUserDirectory().c_str());
    }
    else
    {
        temp_path = std::string(fs_getUserDirectory()) + "/debug";
        temp_path = str_convert_slash_sys(temp_path);
        fs_createDirectory(temp_path.c_str());
    }

    // Set the write directory to the root user directory.
    if (!PHYSFS_setWriteDir(fs_getUserDirectory().c_str()))
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

//--------------------------------------------------------------------------------------------

bool validate(const std::string& source, std::string& target) {
    try {
        target = Ego::VfsPath(source).string();
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
Ego::VfsPath vfs_convert_fname(const Ego::VfsPath& path) {
    BAIL_IF_NOT_INIT();
    static const auto slash = Ego::VfsPath(Ego::VfsPath::getDefaultPathSeparator());
    // If the path is empty ...
    if (path.empty()) {
        // ... assume root.
        return slash;
    }
    if (_vfs_mount_info_search(path.string().c_str()) || NETWORK_SLASH_CHR == path.string()[0]) {
        return path;
    } else {
        return (slash + path);
    }
}

Ego::VfsPath vfs_convert_fname(const std::string& pathString) {
    return vfs_convert_fname(Ego::VfsPath(pathString));
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

    if (filename.empty()) {
        std::make_pair(false, filename);
    }

    // make a temporary copy of the given filename with system-dependent slashes
    // to see if the filename is already resolved
    std::string filename_specific = str_convert_slash_sys(filename);

    if (fs_fileExists(filename_specific)) {
        return std::make_pair(true, filename_specific);
    }

    // The specified filename (in system-specific notation) does not exist.
    // Convert the filename in PhysFS-specific notation.
    filename_specific = vfs_convert_fname(Ego::VfsPath(filename_specific)).string();

    // If the specified filename denotes an existing file or directory, then this file or directory must have a containing directory.
    const char *prefix = PHYSFS_getRealDir(filename_specific.c_str());
    if (nullptr == prefix) {
        return std::make_pair(false, filename);
    }
    // The specified filename denotes an existing file or directory.
    if (PHYSFS_isDirectory(filename_specific.c_str())) {
        // If it denotes a directory then it must be splittable into a prefix and a suffix.
        auto suffix = vfs_mount_info_strip_path(filename_specific.c_str());
        if (suffix.first) {
            return std::make_pair(true, (Ego::VfsPath(prefix) + Ego::VfsPath(suffix.second)).string(Ego::VfsPath::Kind::System));
        } else {
            return std::make_pair(true, (Ego::VfsPath(prefix) + Ego::VfsPath("/")).string(Ego::VfsPath::Kind::System));
        }
    } else {
        // The specified filename denotes a file.
        auto suffix = vfs_mount_info_strip_path(filename_specific.c_str());
        if (suffix.first) {
            return std::make_pair(true, (Ego::VfsPath(prefix) + Ego::VfsPath(suffix.second)).string(Ego::VfsPath::Kind::System));
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
    if (filename.empty()) {
        return std::make_pair(false, filename);
    }
    // Get the write directory.
    const char *writeDirectory = PHYSFS_getWriteDir();
    if (!writeDirectory) {
        throw std::runtime_error("unable to get write directory");
    }
    // Append the filename to the write directory.
    auto resolvedFilename = Ego::VfsPath(writeDirectory) + Ego::VfsPath(filename);
    // Ensure system-specific encoding of the resolved filename.
    return std::make_pair(true, resolvedFilename.string(Ego::VfsPath::Kind::System));
}

//--------------------------------------------------------------------------------------------
int _vfs_ensure_write_directory( const std::string& filename, bool is_directory )
{
    /// @author BB
    /// @details
    BAIL_IF_NOT_INIT();

    if ( filename.empty() ) return 0;

    // make a working copy of the filename
    // and make sure that PHYSFS gets the filename with the slashes it wants
    std::string temp_dirname = vfs_convert_fname(Ego::VfsPath(filename)).string();

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
    std::string temporary = Ego::VfsPath(pathname).string();
    if (!PHYSFS_mkdir(temporary.c_str())) {
        Log::get() << Log::Entry::create(Log::Level::Debug, __FILE__, __LINE__, "PHYSF_mkdir(", pathname, ") failed: ", vfs_getError());
        return false;
    }
    return true;
}

bool vfs_delete_file(const std::string& pathname)
{
    BAIL_IF_NOT_INIT();

    std::string temporary = Ego::VfsPath(pathname).string();

    if (!PHYSFS_delete(temporary.c_str())) {
        Log::get() << Log::Entry::create(Log::Level::Debug, __FILE__, __LINE__, "PHYSF_delete(", pathname, ") failed: ", vfs_getError(), Log::EndOfEntry);
        return false;
    }
    return true;
}

bool vfs_exists(const std::string& pathname) {
    BAIL_IF_NOT_INIT();
    std::string temporary = Ego::VfsPath(pathname).string();
    return (0 != PHYSFS_exists(temporary.c_str()));
}

bool vfs_isDirectory(const std::string& pathname) {
    BAIL_IF_NOT_INIT();
    std::string temporary = Ego::VfsPath(pathname).string();
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
int vfs_read_Sint8( vfs_FILE& file, int8_t * val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
    
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        retval = fread( val, 1, sizeof( int8_t ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_read(file.ptr.p, val, 1, sizeof(int8_t));
        
        error = ( 1 != retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Uint8( vfs_FILE& file, uint8_t * val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
    
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        retval = fread( val, 1, sizeof( uint8_t ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_read(file.ptr.p, val, 1, sizeof(int8_t));
        
        error = ( 1 != retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_read_Sint16( vfs_FILE& file, int16_t * val )
{
    int retval;
    bool error = false;

    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        int16_t itmp;
        retval = fread( &itmp, 1, sizeof( int16_t ), file.ptr.c );

        error = ( 1 != retval );

        *val = Endian_FileToHost( itmp );
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
int vfs_read_Uint16( vfs_FILE& file, uint16_t * val )
{
	bool error = false;
    int retval;

    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        uint16_t itmp;
        retval = fread( &itmp, 1, sizeof( uint16_t ), file.ptr.c );

        error = ( 1 != retval );

        *val = Endian_FileToHost( itmp );
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
int vfs_read_Sint32( vfs_FILE& file, int32_t * val )
{
    int retval;
	bool error = false;

    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        uint32_t itmp;
        retval = fread( &itmp, 1, sizeof( uint32_t ), file.ptr.c );

        error = ( 1 != retval );

        *val = Endian_FileToHost( itmp );
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
int vfs_read_Uint32( vfs_FILE& file, uint32_t * val )
{
    int retval;
	bool error = false;

    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        uint32_t itmp;
        retval = fread( &itmp, 1, sizeof( uint32_t ), file.ptr.c );

        error = ( 1 != retval );

        *val = Endian_FileToHost( itmp );
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
int vfs_read_Sint64( vfs_FILE& file, int64_t * val )
{
    int retval;
	bool error = false;

    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        uint64_t itmp;
        retval = fread( &itmp, 1, sizeof( uint64_t ), file.ptr.c );

        error = ( 1 != retval );

        *val = Endian_FileToHost( itmp );
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
int vfs_read_Uint64( vfs_FILE& file, uint64_t * val )
{
    int retval;
	bool error = false;

    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        uint64_t itmp;
        retval = fread( &itmp, 1, sizeof( uint64_t ), file.ptr.c );

        error = ( 1 != retval );

        *val = Endian_FileToHost( itmp );
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

        *val = Endian_FileToHost( ftmp );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        union { float f; uint32_t i; } convert;
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
int vfs_write<int8_t>( vfs_FILE& file, const int8_t& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
   
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        retval = fwrite( &val, 1, sizeof( int8_t ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_write(file.ptr.p, &val, 1, sizeof(int8_t));
        
        error = ( 1 != retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

template <>
int vfs_write<uint8_t>( vfs_FILE& file, const uint8_t& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();

    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        retval = fwrite( &val, 1, sizeof( uint8_t ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        retval = PHYSFS_write(file.ptr.p, &val, 1, sizeof(uint8_t));
        
        error = ( 1 != retval );
        
        if (error) file.flags |= VFS_FILE_FLAG_ERROR;
        else       file.flags &= ~VFS_FILE_FLAG_ERROR;
    }
    
    if ( error ) _vfs_translate_error( &file );
    
    return retval;
}

template <>
int vfs_write<int16_t>( vfs_FILE& file, const int16_t& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
    
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        int16_t itmp = Endian_HostToFile(val);
        retval = fwrite( &itmp, 1, sizeof( int16_t ), file.ptr.c );
        
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
int vfs_write<uint16_t>( vfs_FILE& file, const uint16_t& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
   
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        uint16_t itmp = Endian_HostToFile(val);
        retval = fwrite( &itmp, 1, sizeof( uint16_t ), file.ptr.c );
        
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
int vfs_write<int32_t>( vfs_FILE& file, const int32_t& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
    
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        int32_t itmp = Endian_HostToFile(val);
        retval = fwrite( &itmp, 1, sizeof( int32_t ), file.ptr.c );
        
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
int vfs_write<uint32_t>( vfs_FILE& file, const uint32_t& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
    
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        uint32_t itmp = Endian_HostToFile(val);
        retval = fwrite( &itmp, 1, sizeof( uint32_t ), file.ptr.c );
        
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
int vfs_write<int64_t>( vfs_FILE& file, const int64_t& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();
   
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        int64_t itmp = Endian_HostToFile(val);
        retval = fwrite( &itmp, 1, sizeof( int64_t ), file.ptr.c );
        
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
int vfs_write<uint64_t>( vfs_FILE& file, const uint64_t& val )
{
    int retval;
    bool error = false;
    
    BAIL_IF_NOT_INIT();  
    
    retval = 0;
    if ( VFS_FILE_TYPE_CSTDIO == file.type )
    {
        uint64_t itmp = Endian_HostToFile(val);
        retval = fwrite( &itmp, 1, sizeof( uint64_t ), file.ptr.c );
        
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
        float ftmp = Endian_HostToFile(val);
        retval = fwrite( &ftmp, 1, sizeof( float ), file.ptr.c );
        
        error = ( 1 != retval );
    }
    else if ( VFS_FILE_TYPE_PHYSFS == file.type )
    {
        union { float f; uint32_t i; } convert;
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

std::vector<std::string> SearchContext::enumerateFiles(const Ego::VfsPath& pathname) {
    std::vector<std::string> result;
    char **fileList = PHYSFS_enumerateFiles(pathname.string().c_str());
    if (!fileList) {
        throw std::runtime_error("unable to enumerate files");
    }
    for (char **file = fileList; nullptr != *file; ++file) {
        try {
            result.push_back(*file);
        } catch (...) {
            PHYSFS_freeList(fileList);
            std::rethrow_exception(std::current_exception());
        }
    }
    PHYSFS_freeList(fileList);
    return result;
}

SearchContext::SearchContext(const Ego::VfsPath& searchPath, const Ego::Extension& searchExtension, uint32_t searchBits)
    : file_list_2(), file_list_iterator_2(), path_2(), bare(false), predicates(), found_2() {
    // Bare search results?
    if (VFS_SEARCH_BARE == (VFS_SEARCH_BARE & searchBits)) {
        bare = true;
    }
    // Filter by type?
    predicates.push_back(makePredicate(searchBits));
    // Enumerate using PhysFS.
    path_2 = vfs_convert_fname(searchPath);
    file_list_2 = enumerateFiles(path_2);
    // Filter by extension?
    predicates.push_back(makePredicate(searchExtension.to_string()));
    // Begin iteration.
    file_list_iterator_2 = file_list_2.begin();
    // Search the first acceptable filename.
    for (; file_list_iterator_2 != file_list_2.cend(); file_list_iterator_2++) {
        /// @todo: Possibly virtual function call. Not acceptable.
        if (predicate(Ego::VfsPath(*file_list_iterator_2))) {
            break;
        }
    }
    if (file_list_iterator_2 != file_list_2.cend()) {
        if (bare) {
            found_2 = Ego::VfsPath(*file_list_iterator_2);
        } else {
            /// @todo Possibly virtual function call. Not acceptable.
            found_2 = path_2 + Ego::VfsPath(NETWORK_SLASH_STR) + Ego::VfsPath(*file_list_iterator_2);
        }
    }
}

SearchContext::SearchContext(const Ego::VfsPath& searchPath, uint32_t searchBits) :
    file_list_2(), file_list_iterator_2(), path_2(), bare(false), predicates(), found_2() {
    // Bare search results?
    if (VFS_SEARCH_BARE == (VFS_SEARCH_BARE & searchBits)) {
        bare = true;
    }
    // Filter by type?
    predicates.push_back(makePredicate(searchBits));
    // Enumerate using PhysFS.
    path_2 = vfs_convert_fname(searchPath);
    file_list_2 = enumerateFiles(path_2);

    // Begin iteration.
    file_list_iterator_2 = file_list_2.begin();
    // Search the first acceptable filename.
    for (; file_list_iterator_2 != file_list_2.cend(); file_list_iterator_2++) {
        /// @todo: Possibly virtual function call. Not acceptable.
        if (predicate(Ego::VfsPath(*file_list_iterator_2))) {
            break;
        }
    }
    if (file_list_iterator_2 != file_list_2.cend()) {
        if (bare) {
            found_2 = Ego::VfsPath(*file_list_iterator_2);
        } else {
            /// @todo Possibly virtual function call. Not acceptable.
            found_2 = path_2 + Ego::VfsPath(NETWORK_SLASH_STR) + Ego::VfsPath(*file_list_iterator_2);
        }
    }
}

SearchContext::SearchContext(uint32_t searchBits)
    : SearchContext(Ego::VfsPath("/"), searchBits)
{ /* Intentionally empty. */}

SearchContext::SearchContext(const Ego::Extension& searchExtension, uint32_t searchBits)
    : SearchContext(Ego::VfsPath("/"), searchExtension, searchBits)
{ /* Intentionally empty. */ }

SearchContext::~SearchContext()
{ /* Intentionally empty. */ }

//--------------------------------------------------------------------------------------------
std::function<bool(const Ego::VfsPath&)> SearchContext::makePredicate(uint32_t searchBits) {
    auto predicate = [searchBits](const Ego::VfsPath& path) {
        if (VFS_SEARCH_ALL != (searchBits & VFS_SEARCH_ALL)) {
            bool isDirectory = vfs_isDirectory(path.string());
            bool isFile = !isDirectory;
            if (isFile) {
                return (VFS_SEARCH_FILE == (VFS_SEARCH_FILE & searchBits));
            }
            if (isDirectory) {
                return (VFS_SEARCH_DIR == (VFS_SEARCH_DIR & searchBits));
            }
        }
        return true;
    };
    return predicate;
}

std::function<bool(const Ego::VfsPath&)> SearchContext::makePredicate(std::string extension) {
    auto predicate = [extension](const Ego::VfsPath& path) {
        return path.getExtension() == extension;
    };
    return predicate;
}

bool SearchContext::predicate(const Ego::VfsPath& path) const {
    auto fullPath = path_2 + Ego::VfsPath(NETWORK_SLASH_STR) + path;
    // Apply predicates.
    for (const auto& predicate : predicates) {
        if (nullptr != predicate) {
            if (!predicate(fullPath)) {
                return false;
            }
        }
    }
    return true;
}

void SearchContext::nextData()
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
        if (this->predicate(Ego::VfsPath(*this->file_list_iterator_2))) break;
    }

    // If no suitable file was found ...
    if (this->file_list_iterator_2 == this->file_list_2.cend()) {
        // ... return.
        this->found_2 = Ego::VfsPath();
        return;
    }

    if (this->bare) {
        this->found_2 = Ego::VfsPath(*this->file_list_iterator_2);
    } else {
        this->found_2 = path_2 + Ego::VfsPath(NETWORK_SLASH_STR) + Ego::VfsPath(*this->file_list_iterator_2);
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

    SearchContext *ctxt;

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
    ctxt = new SearchContext(vfs_convert_fname(sourceDir), VFS_SEARCH_FILE | VFS_SEARCH_BARE );
    if (!ctxt) return VFS_FALSE;
    while (ctxt->hasData())
    {
        auto fileName = ctxt->getData();
        // Ignore files that begin with a .
        if ( '.' != fileName.string()[0] )
        {
            snprintf( srcPath, SDL_arraysize( srcPath ), "%s/%s", sourceDir, fileName.string().c_str() );
            snprintf( destPath, SDL_arraysize( destPath ), "%s/%s", destDir, fileName.string().c_str() );

            if ( !vfs_copyFile( srcPath, destPath ) )
            {
                Log::get() << Log::Entry::create(Log::Level::Debug, __FILE__, __LINE__, "failed to copy from ", "`", 
                                                 srcPath, "`", " to ", "`", destPath, "`", ": ", vfs_getError(),
                                                 Log::EndOfEntry);
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
int vfs_add_mount_point( const std::string& rootPath, const Ego::FsPath& relativePath, const Ego::VfsPath& mountPoint, int append )
{
    int retval = -1;

    BAIL_IF_NOT_INIT();

    // If mount point is empty or a slash indicates the PhysFS root directory, not the root of the currently mounted volume.
    if ( mountPoint.empty() || mountPoint == Ego::VfsPath("/") ) return 0;

    Ego::FsPath dirname;
    if ( !rootPath.empty() && !relativePath.empty() )
    {
        // both the root path and the relative path are non-empty:
        // the directory is meant to be the concatenation of both.
        dirname = Ego::FsPath(rootPath + SYSTEM_SLASH_STR + relativePath.string());
    }
    else if ( !rootPath.empty() )
    {
        // the root path is non-empty, the relative path is empty:
        // the direcotry i meant to be the root path.
        dirname = Ego::FsPath(rootPath);
    }
    else if ( !relativePath.empty() )
    {
        // the root path is empty, the relative path is non-empty:
        // the directory is meant to be the relative path.
        dirname = relativePath;
    }
    else
    {
        // both the root path and the relative path are empty:
        // reject.
        return 0;
    }

    /// @note ZF@> 2010-06-30 vfs_convert_fname_sys() broke the Linux version
    /// @note BB@> 2010-06-30 the error in vfs_convert_fname_sys() might be fixed now
    /// @note PF@> 2015-01-01 this should be unneeded. root_path and relative_path should both
    ///                       sys-dependent paths, unless Windows does something strange?
#if 0
    std::string loc_dirname = vfs_convert_fname_sys( dirname );
#else
    Ego::FsPath loc_dirname = dirname;
#endif

    if ( _vfs_mount_info_add( mountPoint, rootPath, relativePath.string() ) )
    {
        retval = PHYSFS_mount( loc_dirname.string().c_str(), mountPoint.string().c_str(), append );
        if ( 0 == retval )
        {
            // go back and remove the mount info, since PHYSFS rejected the
            // data we gave it
            int i = _vfs_mount_info_matches( mountPoint, loc_dirname.string() );
            _vfs_mount_info_remove( i );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_remove_mount_point( const Ego::VfsPath& mountPoint )
{
    BAIL_IF_NOT_INIT();

    // don't allow it to remove the default directory
    if ( mountPoint.empty() || mountPoint == Ego::VfsPath("/") ) return 0;

    // assume we are going to fail
    int retval = 0;

    // see if we have the mount point
    int cnt = _vfs_mount_info_matches( mountPoint );

    // does it exist in the list?
    if ( cnt < 0 ) return false;

    while ( cnt >= 0 )
    {
        // we have to use the path name to remove the search path, not the mount point name
        PHYSFS_removeFromSearchPath( _vfs_mount_infos[cnt].full_path.c_str() );

        // remove the mount info from this index
        // PF> we remove it even if PHYSFS_removeFromSearchPath() fails or else we might get an infinite loop
        _vfs_mount_info_remove( cnt );

        cnt = _vfs_mount_info_matches( mountPoint );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool SearchContext::hasData() const {
    return this->file_list_iterator_2 != this->file_list_2.cend();
}

const Ego::VfsPath& SearchContext::getData() const {
    return this->found_2;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// @brief Get if the specified path is equivalent to a virtual mount point.
/// @param pathname the pathname
/// @return #VFS_TRUE if @a pathname is equivalent to a virtual mount point, #VFS_FALSE otherwise
int _vfs_mount_info_search(const std::string& pathname) {
    BAIL_IF_NOT_INIT();

    if (pathname.empty()) return VFS_FALSE;

    // Get the sanitized pathname.
    auto sanitizedPathname = str_clean_path(pathname);

    for (const auto& mount_info : _vfs_mount_infos) {
        if (sanitizedPathname == mount_info.mount) {
            return VFS_TRUE;
        }

        if (sanitizedPathname == (std::string(mount_info.mount) + NET_SLASH_STR)) {
            return VFS_TRUE;
        }
    }

    return VFS_FALSE;
}

//--------------------------------------------------------------------------------------------
std::pair<bool, std::string> vfs_mount_info_strip_path( const std::string& path )
{
    BAIL_IF_NOT_INIT();

    // Strip any starting slashes.
    std::string path_2 = Ego::left_trim<char>(path, [](const char& chr) { return chr == NET_SLASH_CHR || chr == WIN32_SLASH_CHR; });

    // Find the first mount point path that is a prefix of the specified path.
    // If such a path is discovered, return the specified path with the prefix removed.
    for (const auto& mount_info : _vfs_mount_infos) {
        if (id::is_prefix(path_2, mount_info.mount)) {
            return std::make_pair(true, path_2.substr(mount_info.mount.length()));
        }
    }
    return std::make_pair(false, path);
}

//--------------------------------------------------------------------------------------------
int _vfs_mount_info_matches(const Ego::VfsPath& mountPoint) {
    BAIL_IF_NOT_INIT();

    // are there any in the list?
    if (_vfs_mount_infos.empty()) return -1;

    // Strip any starting slashes.
    auto tmp = Ego::VfsPath(Ego::left_trim<char>(mountPoint.string(), [](const char& chr) { return chr == NET_SLASH_CHR || chr == WIN32_SLASH_CHR; }));

    if (!tmp.empty()) {
        // find the first path info with the given mount_point
        for (auto cnt = 0; cnt < _vfs_mount_infos.size(); cnt++) {
            if (_vfs_mount_infos[cnt].mount == mountPoint.string()) {
                return cnt;
            }
        }
    }

    return -1;
}
int _vfs_mount_info_matches(const Ego::VfsPath& mountPoint, const std::string& local_path) {
    BAIL_IF_NOT_INIT();

    // are there any in the list?
    if (_vfs_mount_infos.empty()) return -1;

    // Strip any starting slashes.
    auto tmp = Ego::VfsPath(Ego::left_trim<char>(mountPoint.string(), [](const char& chr) { return chr == NET_SLASH_CHR || chr == WIN32_SLASH_CHR; }));

    if (!tmp.empty() && !local_path.empty()) {
        // find the first path info with the given mount_point and local_path
        for (auto cnt = 0; cnt < _vfs_mount_infos.size(); cnt++) {
            if (_vfs_mount_infos[cnt].mount == mountPoint.string() &&
                _vfs_mount_infos[cnt].full_path == local_path) {
                return cnt;
            }
        }
    } else if (!tmp.empty()) {
        // find the first path info with the given mount_point
        for (auto cnt = 0; cnt < _vfs_mount_infos.size(); cnt++) {
            if (_vfs_mount_infos[cnt].mount == mountPoint.string()) {
                return cnt;
            }
        }
    } else if (!local_path.empty()) {
        // find the first path info with the given local_path
        for (auto cnt = 0; cnt < _vfs_mount_infos.size(); cnt++) {
            if (_vfs_mount_infos[cnt].full_path == local_path) {
                return cnt;
            }
        }
    }

    return -1;
}

//--------------------------------------------------------------------------------------------
bool _vfs_mount_info_add(const Ego::VfsPath& mountPoint, const std::string& rootPath, const std::string& relativePath) {
    BAIL_IF_NOT_INIT();

    // If the mount point is empty, do nothing.
    if (mountPoint.empty()) return false;

    // make a complete version of the pathname
    std::string local_path;
    if (!rootPath.empty() && !relativePath.empty()) {
        local_path = rootPath + SLASH_STR + relativePath;
    } else if (!rootPath.empty()) {
        local_path = rootPath;
    } else if (!relativePath.empty()) {
        local_path = relativePath;
    } else {
        return false;
    }

    // do we want to add it?
    if (local_path.empty()) return false;

    if (_vfs_mount_info_matches(mountPoint, local_path) >= 0) return false;

    // strip any starting slashes
    auto tmp = Ego::VfsPath(Ego::left_trim<char>(mountPoint.string(), [](const char& chr) { return chr == NET_SLASH_CHR || chr == WIN32_SLASH_CHR; }));
    if (tmp.empty()) return false;

    // save the mount point in a list for later detection
    vfs_path_data_t path_data;
    path_data.mount = tmp.string();
    path_data.full_path = local_path;
    if (!rootPath.empty()) {
        path_data.root_path = rootPath;
    }
    if (!relativePath.empty()) {
        path_data.relative_path = relativePath;
    }
    _vfs_mount_infos.push_back(path_data);

    return true;
}

//--------------------------------------------------------------------------------------------
bool _vfs_mount_info_remove(int cnt)
{
    BAIL_IF_NOT_INIT();

    // does it exist in the list?
    if ( cnt < 0 || cnt >= _vfs_mount_infos.size() ) return false;

    // fill in the hole in the list
    _vfs_mount_infos.erase(_vfs_mount_infos.begin() + cnt);

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void vfs_set_base_search_paths( void )
{
    BAIL_IF_NOT_INIT();

    // Put write dir first in search path...
    PHYSFS_addToSearchPath( fs_getUserDirectory().c_str(), 0 );

    // Put base path on search path...
    PHYSFS_addToSearchPath( fs_getDataDirectory().c_str(), 1 );
    
    // Put config path on search path...
    PHYSFS_addToSearchPath(fs_getConfigDirectory().c_str(), 1);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void vfs_readEntireFile(const std::string& pathname, std::function<void(size_t, const char *)> receive) {
    auto deleter = [](vfs_FILE *file) { if (file) vfs_close(file); };
    std::unique_ptr<vfs_FILE, decltype(deleter)> file(vfs_openRead(pathname), deleter);
    if (!file) {
        throw id::runtime_error(__FILE__, __LINE__, "unable to open file `" + pathname + "` for reading");
    }
    // Read in 2048 Byte chunks.
    char buffer[2048];
    while (!vfs_eof(file.get())) {
        size_t read = vfs_read(buffer, 1, 2048, file.get());
        if (vfs_error(file.get())) {
            throw id::runtime_error(__FILE__, __LINE__, "error while reading file `" + pathname + "`");
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
