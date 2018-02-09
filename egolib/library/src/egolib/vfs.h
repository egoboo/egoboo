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

/// @file egolib/vfs.h
/// @brief A virtual filesystem for Egoboo.
///
/// @details Almost all filesystem reads and writes should be handled through this interface. The only possible
/// exceptions would be the log file (?) or something similar.
/// Currently, this basically just wraps PhysicsFS functions
/**
 * @changelog
 * - MH:
 *     - added functions vfs_isWriting and vfs_isReading.
 *     - added support for SDL RW ops that do not take ownership of a file
 *     - fixed leaks
 */

#pragma once

#include "egolib/egolib_config.h"
#include "egolib/VFS/FsPath.hpp"
#include "egolib/VFS/VfsPath.hpp"
#include <vector>
#include <functional>
#include "egolib/integrations/filesystem.hpp"

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

// ASCII terminal/printer control codes
#define ASCII_NUL_CHAR  '\x00'            /**< null */
#define ASCII_SOH_CHAR  '\x01'            /**< start of heading */
#define ASCII_STX_CHAR  '\x02'            /**< start of text */
#define ASCII_ETX_CHAR  '\x03'            /**< end of text */
#define ASCII_EOT_CHAR  '\x04'            /**< end of transmission */
#define ASCII_ENQ_CHAR  '\x05'            /**< enquiry */
#define ASCII_ACK_CHAR  '\x06'            /**< acknowledge */
#define ASCII_BEL_CHAR  '\x07'            /**< bell */
#define ASCII_BS_CHAR   '\x08'            /**< backspace */
#define ASCII_HT_CHAR   '\x09'            /**< horizontal tab */
#define ASCII_NL_CHAR   '\x0A'            /**< new line (or LF, line feed) */
#define ASCII_VT_CHAR   '\x0B'            /**< vertical tab */
#define ASCII_NP_CHAR   '\x0C'            /**< new page (or FF, form feed) */
#define ASCII_CR_CHAR   '\x0D'            /**< carriage return */
#define ASCII_SO_CHAR   '\x0E'            /**< shift out */
#define ASCII_SI_CHAR   '\x0F'            /**< shift in */
#define ASCII_DLE_CHAR  '\x10'            /**< data link escape */
#define ASCII_DC1_CHAR  '\x11'            /**< device control 1 */
#define ASCII_DC2_CHAR  '\x12'            /**< device control 2 */
#define ASCII_DC3_CHAR  '\x13'            /**< device control 3 */
#define ASCII_DC4_CHAR  '\x14'            /**< device control 4 */
#define ASCII_NAK_CHAR  '\x15'            /**< negative acknowledge */
#define ASCII_SYN_CHAR  '\x16'            /**< synchronous idle */
#define ASCII_ETB_CHAR  '\x17'            /**< end of transmission block */
#define ASCII_CAN_CHAR  '\x18'            /**< cancel */
#define ASCII_EM_CHAR   '\x19'            /**< end of medium */
#define ASCII_SUB_CHAR  '\x1A'            /**< substitute */
#define ASCII_ESC_CHAR  '\x1B'            /**< escape */
#define ASCII_FS_CHAR   '\x1C'            /**< file separator */
#define ASCII_GS_CHAR   '\x1D'            /**< group separator */
#define ASCII_RS_CHAR   '\x1E'            /**< record separator */
#define ASCII_US_CHAR   '\x1F'            /**< unit separator */
#define ASCII_SP_CHAR   '\x20'            /**< space  */

/// @note the following escape codes are translated by the compiler
///       to whatever encoding is necessary
#define C_BELL_CHAR            '\a'
#define C_BACKSPACE_CHAR       '\b'
#define C_FORMFEED_CHAR        '\f'
#define C_CARRIAGE_RETURN_CHAR '\r'
#define C_TAB_CHAR             '\t'
#define C_VERTICAL_TAB_CHAR    '\v'
#define C_SINGLE_QUOTE_CHAR    '\''
#define C_DOUBLE_QUOTE_CHAR    '\"'
#define C_LINEFEED_CHAR        '\n'

/// @note win32 systems (and some others) handle newlines by using a combinarion
///       of linefeed and carriage return characters. So...
///       the numerical values of '\n' and '\r' may vary from system to system and
///       may be different for input and output (i.e. '\n' writing '\x0D\x0A" on win32 systems)
#define ASCII_LINEFEED_CHAR    ASCII_NL_CHAR

/// @todo Remove this, use @a true.
#define VFS_TRUE  ((int)(1==1))
/// @todo Remove this, use @a false.
#define VFS_FALSE ((int)(!VFS_TRUE))

//--------------------------------------------------------------------------------------------
// TYPEDEFS
//--------------------------------------------------------------------------------------------

/// A container for holding all the data for a search
struct SearchContext {   
public:
    Ego::VfsPath path_2;
    std::vector<std::string> file_list_2;
    std::vector<std::string>::const_iterator file_list_iterator_2;
    Ego::VfsPath found_2;
    // 
    bool bare;
    /// The predicates: a pathname must be accepted by all predicates in this list in order to be considered.
    std::vector<std::function<bool(const Ego::VfsPath&)>> predicates;

public:
    /// @brief Construct this search context.
    /// @param searchPath the search path
    /// @param searchBits the search bits
    SearchContext(const Ego::VfsPath& searchPath, uint32_t searchBits);

    /// @brief Construct this search context.
    /// @param searchPath the search path
    /// @param searchExtension the search extension
    /// @param searchBits the search bits
    SearchContext(const Ego::VfsPath& searchPath, const Ego::Extension& searchExtension, uint32_t searchBits);

public:
    /// @brief Construct this search context.
    /// @param searchBits the search bits
    /// @remark
    /// Same as
    /// @code
    /// SearchContext(Path("/"), searchBits)
    /// @endcode
    SearchContext(uint32_t searchBits);

    /// @brief Construct this search context.
    /// @param searchExtension the search extension
    /// @param searchBits the search bits
    /// @remark
    /// Same as
    /// @code
    /// SearchContext(Path("/"), searchExtension, searchBits)
    /// @endcode
    SearchContext(const Ego::Extension& searchExtension, uint32_t searchBits);

public:
    /// Destruct this search context.
    ~SearchContext();

public:
    /// Get if there is data in this context available.
    bool hasData() const;

    /// Get the data available in this context.
    /// Raises an exception if there is no data available.
    const Ego::VfsPath& getData() const;

    /// Compute the next data.
    void nextData();

protected:
    /// @brief Create a predicate that filters a pathname by the specified search bits.
    /// @param searchBits the search bits
    /// @return the predicate, expects an absolute, sanitized pathname
    static std::function<bool(const Ego::VfsPath&)> makePredicate(uint32_t searchBits);
    
    /// @brief Create a predicate that filters a pathname by the specified extension.
    /// @param extension the extension without the extension separator <c>.</c>
    /// @return the predicate, expects an absolute, sanitized pathname
    static std::function<bool(const Ego::VfsPath&)> makePredicate(std::string extension);

protected:
    /// @return @a true if the specified path is acceptable, @a false otherwise
    bool predicate(const Ego::VfsPath& path) const;
    /// @todo Move to traits.
    static std::vector<std::string> enumerateFiles(const Ego::VfsPath& pathname);
};

// use this ugly thing, since there is no other way to hide the actual structure of the vfs_FILE...
struct vsf_file;
typedef struct vsf_file vfs_FILE;

//--------------------------------------------------------------------------------------------
// CONSTANTS
//--------------------------------------------------------------------------------------------

/// What type of things are we searching for?
enum e_vfs_serach_bits
{
    // file types
    VFS_SEARCH_NONE = 0,               ///< NONE == ALL
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

/**
 * @brief
 *  Initialize the VFS.
 *  There is no need to uninitialize the VFS: Uninitialization is performed automatically at program termination.
 * @return
 *  @a 0 on success, a non-zero value on failure
 */
int vfs_init(const char *argv0, const char *root_dir);

/**@{*/
/**
 * These functions open in "binary mode" this means that they are reading using
 * PhysFS and not using the C stdio routines.
 */

/**
 * @brief
 *  Open a file for reading in binary mode, using PhysFS.
 * @param pathname
 *  the pathname of the file
 * @return
 *  a pointer to the file on success, a null pointer on failure
 */
vfs_FILE *vfs_openRead(const std::string& pathname);

/**
 * @brief
 *  Open a file for writing in binary mode, using PhysFS.
 * @param pathname
 *  the pathname of the file
 * @return
 *  a pointer to the file on success, a null pointer on failure
 */
vfs_FILE *vfs_openWrite(const std::string& pathname);

/**
 * @brief
 *  Open a file for appending in binary mode, using PhysFS.
 * @param pathname
 *  the pathname of the file
 * @return
 *  a pointer to the file on success, a null pointer on failure
 */
vfs_FILE *vfs_openAppend(const std::string& pathname);

/**@}*/

/**
 * @brief
 *  Get if a file is opened for writing.
 * @param file
 *  the file
 * @return
 *  @a -1 on failure. Otherwise, @a 1 if the file is opened for writing and @a 0 if it is not.
 */
int vfs_isWriting(vfs_FILE *file);

/**
 * @brief
 *  Get if a file is opened for writing.
 * @param file
 *  the file
 * @return
 *  @a -1 on failure. Otherwise, @a 1 if the file is opened for reading and @a 0 if it is not.
 */
int vfs_isReading(vfs_FILE *file);

/**
 * @brief
 *  Close and deallocate a file.
 * @param file
 *  the file
 */
int vfs_close(vfs_FILE *file);
int vfs_flush(vfs_FILE *file);

int vfs_eof(vfs_FILE *file);
int vfs_error(vfs_FILE *file);
long vfs_tell(vfs_FILE *file);
int vfs_seek(vfs_FILE *file , long offset);

/** @return @a true on success, @a false on failure */
bool vfs_mkdir(const std::string& pathname);
/** @return @a true on success, @a false on failure */
bool vfs_delete_file(const std::string& pathname);
/** @return @a true if the path refers to a file that exists, @a false otherwise */
bool vfs_exists(const std::string& pathname);
/** @return @a true if the pathname refers to an existing directory file, @a false otherwise */
bool vfs_isDirectory(const std::string& pathname);

// binary reading and writing
size_t vfs_read(void *buffer, size_t size, size_t count, vfs_FILE *file);
int vfs_read_Sint8(vfs_FILE& file, int8_t *val);
int vfs_read_Uint8(vfs_FILE& file, uint8_t *val);
int vfs_read_Sint16(vfs_FILE& file, int16_t *val);
int vfs_read_Uint16(vfs_FILE& file, uint16_t *val);
int vfs_read_Sint32(vfs_FILE& file, int32_t *val);
int vfs_read_Uint32(vfs_FILE& file, uint32_t *val);
int vfs_read_Sint64(vfs_FILE& file, int64_t *val);
int vfs_read_Uint64(vfs_FILE& file, uint64_t *val);
int vfs_read_float(vfs_FILE& file, float *val);

size_t vfs_write(const void *buffer, size_t size, size_t count, vfs_FILE *file);

template <typename Type>
int vfs_write(vfs_FILE& file, const Type& value);

template <>
int vfs_write<int8_t>(vfs_FILE& file, const int8_t& val);
template <>
int vfs_write<uint8_t>(vfs_FILE& file, const uint8_t& val);
template <>
int vfs_write<int16_t>(vfs_FILE& file, const int16_t& val);
template <>
int vfs_write<uint16_t>(vfs_FILE& file, const uint16_t& val);
template <>
int vfs_write<int32_t>(vfs_FILE& file, const int32_t& val);
template <>
int vfs_write<uint32_t>(vfs_FILE& file, const uint32_t& val);
template <>
int vfs_write<int64_t>(vfs_FILE& file, const int64_t& val);
template <>
int vfs_write<uint64_t>(vfs_FILE& file, const uint64_t& val);
template <>
int vfs_write<float>(vfs_FILE& file, const float& val);

long vfs_fileLength(vfs_FILE *file);
int vfs_rewind(vfs_FILE *file);

int vfs_printf(vfs_FILE *file, const char *format, ...) GCC_PRINTF_FUNC(2);

int vfs_putc(int c, vfs_FILE *file);
int vfs_getc(vfs_FILE *file);
int vfs_ungetc(int c, vfs_FILE *file);
int vfs_puts(const char *s, vfs_FILE *file);

void         vfs_empty_temp_directories();

int          vfs_copyFile(const std::string& source, const std::string& target);
int          vfs_copyDirectory(const char *sourceDir, const char *destDir);

int    vfs_removeDirectoryAndContents(const char * dirname);

// @brief Resolve the specified filename.
// @param filename the filename
// @remark Resolution follows the same rules as if the file was opened for reading.
// @return <c>(true,resolvedFilename)</c> on success, <c>(false,filename)</c> on failure
// where <c>resolvedFilename</c> is the resolved filename in system-specific notation.
std::pair<bool, std::string> vfs_resolveReadFilename(const std::string& filename);
// @brief Resolve the specified filename.
// @param filename the filename
// @remark Resolution follows the same rules as if the file was opened/created for writing.
// @return <c>(true,resolvedFilename)</c> on success, <c>(false,filename)</c> on failure
// where <c>resolvedFilename</c> is the resolved filename in system-specific notation.
// @throw std::runtime_error if no write directory was specified
std::pair<bool, std::string> vfs_resolveWriteFilename(const std::string& filename);

const char *vfs_getError();
/// @brief Get the version of the PhysFS library which was used for linking the binary.
std::string vfs_getLinkedVersion();
/// @brief Get the version of the PhysFS library which used for compiling the binary.
std::string vfs_getVersion();

/// @param rootPath the root path in platform-specific notation e.g. <c>C:\Program Files\Egoboo\data</c>
/// @param relativePath the path relative to the root path in platform-specific notation e.g. <c>modules</c>
/// @param mountPoint the mount point in vfs-specific notation e.g. <c>mp_modules</c>
/// <c>""</c> is equivalent to <c>"/"</c>.
int vfs_add_mount_point(const std::string& rootPath, const Ego::FsPath& relativePath, const Ego::VfsPath& mountPoint, int append);
/// @brief Remove every search path related to the given mount point
/// @param mountPoint the mount point in vfs-specific notation e.g. <c>mp_modules</c>
int vfs_remove_mount_point(const Ego::VfsPath& mountPoint);

Ego::VfsPath vfs_convert_fname(const Ego::VfsPath& path);
Ego::VfsPath vfs_convert_fname(const std::string& pathString);

void vfs_set_base_search_paths();
std::pair<bool, std::string> vfs_mount_info_strip_path(const std::string& some_path);

void vfs_listSearchPaths();
    
/// @brief Read the contents of a file.
/// @param pathname the pathname of the file
/// @param receive function invoked if bytes are received
/// @throw idlib::runtime_error the file can not be opened for reading or an error occurs while reading.
/// @throw any exception raised by @a receive
void vfs_readEntireFile(const std::string& pathname, std::function<void(size_t, const char *)> receive);
bool vfs_readEntireFile(const std::string& pathname, char **data, size_t *length);
bool vfs_writeEntireFile(const std::string& pathname, const char *data, const size_t length);

// Wrap vfs into SDL_RWops
struct SDL_RWops;

/**
 * @brief
 *  Create SDL RW ops for the given file.
 * @param file
 *  the file
 * @param ownership
 *  if SDL RW ops are returned by this function and this is @a true,
 *  then the SDL RW ops have taken ownership of the file i.e. when
 *  the SDL RW ops are closed, they also also the file.
 * @return
 *  the SDL RW ops on success, a null pointer on failure
 */
SDL_RWops *vfs_openRWops(vfs_FILE *file, bool ownership);
   
/**
 * @brief
 *  Create SDL RW ops for the given filename.
 * @param pathname
 *  the pathname
 * @return
 *  the SDL RW ops on success, a null pointer on failure
 */
SDL_RWops *vfs_openRWopsRead(const std::string& pathname);

/**
 * @brief
 *  Create SDL RW ops for the given filename.
 * @param pathname
 *  the pathname
 * @return
 *  the SDL RW ops on success, a null pointer on failure
 */
SDL_RWops *vfs_openRWopsWrite(const std::string& pathname);

/**
 * @brief
 *  Create SDL RW ops for the given filename.
 * @param pathname
 *  the pathname
 * @return
 *  the SDL RW ops on success, a null pointer on failure
 */
SDL_RWops *vfs_openRWopsAppend(const std::string& pathname);
