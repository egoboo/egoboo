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

#pragma once

#include "egolib/typedef.h"

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
#if 0
#define C_NEW_LINE_CHAR        '\n'
#endif
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

struct s_vfs_search_context;
typedef struct s_vfs_search_context vfs_search_context_t;

// use this ugly thing, since there is no other way to hide the actual structure of the vfs_FILE...
struct s_vfs_FILE;
typedef struct s_vfs_FILE vfs_FILE;

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

/// the initlization routing. there is no need to call the de-initialization. That
/// will be called automatically at program termination
void vfs_init(const char *argv0, const char *root_dir);

/// these functions open in "binary mode" this means that they are reading using
/// physfs and not using the c stdio routines
vfs_FILE *vfs_openReadB(const char *filename);
vfs_FILE *vfs_openWriteB(const char *filename);
vfs_FILE *vfs_openAppendB(const char *filename);

// these functions open in "text mode" this means that they are reading using
// the c stdio routines. we use physfs to resolve the actual filename
vfs_FILE *vfs_openRead(const char *filename);
vfs_FILE *vfs_openWrite(const char *filename);
vfs_FILE *vfs_openAppend(const char *filename);

int vfs_close(vfs_FILE *file);
int vfs_flush(vfs_FILE *file);

int vfs_eof(vfs_FILE *file);
int vfs_error(vfs_FILE *file);
long vfs_tell(vfs_FILE *file);
int vfs_seek(vfs_FILE *file , long offset);

int vfs_mkdir(const char *dirName);
int vfs_delete_file(const char *filename);

int vfs_exists(const char *fname);
int vfs_isDirectory(const char *fname);

// binary reading and writing
    size_t vfs_read( void * buffer, size_t size, size_t count, vfs_FILE * pfile );
    int    vfs_read_Sint8(vfs_FILE *pfile, Sint8 *val);
    int    vfs_read_Uint8(vfs_FILE *pfile, Uint8 *val);
    int    vfs_read_Sint16( vfs_FILE * pfile, Sint16 * val );
    int    vfs_read_Uint16( vfs_FILE * pfile, Uint16 * val );
    int    vfs_read_Sint32( vfs_FILE * pfile, Sint32 * val );
    int    vfs_read_Uint32( vfs_FILE * pfile, Uint32 * val );
    int    vfs_read_Sint64( vfs_FILE * pfile, Sint64 * val );
    int    vfs_read_Uint64( vfs_FILE * pfile, Uint64 * val );
    int    vfs_read_float( vfs_FILE * pfile, float * val );
    
    size_t vfs_write( const void * buffer, size_t size, size_t count, vfs_FILE * pfile );
    int    vfs_write_Sint8(vfs_FILE *pfile, const Sint8 val);
    int    vfs_write_Uint8(vfs_FILE *pfile, const Uint8 val);
    int    vfs_write_Sint16( vfs_FILE * pfile, const Sint16 val );
    int    vfs_write_Uint16( vfs_FILE * pfile, const Uint16 val );
    int    vfs_write_Sint32( vfs_FILE * pfile, const Sint32 val );
    int    vfs_write_Uint32( vfs_FILE * pfile, const Uint32 val );
    int    vfs_write_Sint64( vfs_FILE * pfile, const Sint64 val );
    int    vfs_write_Uint64( vfs_FILE * pfile, const Uint64 val );
    int    vfs_write_float(vfs_FILE *pfile, const float val);

/// the file searching routines
    char ** vfs_enumerateFiles( const char * dir_name );
    void    vfs_freeList( void * listVar );

    const char * vfs_search_context_get_current( struct s_vfs_search_context * ctxt );

    vfs_search_context_t * vfs_findFirst( const char * search_path, const char * search_extension, Uint32 search_bits );
    vfs_search_context_t * vfs_findNext( vfs_search_context_t ** pctxt );
    void                   vfs_findClose( vfs_search_context_t ** pctxt );

    long         vfs_fileLength( vfs_FILE * pfile );
    int          vfs_rewind( vfs_FILE * pfile );

    int          vfs_scanf( vfs_FILE * pfile, const char *format, ... ) GCC_SCANF_FUNC( 2 );
    int          vfs_printf( vfs_FILE * pfile, const char *format, ... ) GCC_PRINTF_FUNC( 2 );

    int          vfs_putc( int c, vfs_FILE * pfile );
    int          vfs_getc( vfs_FILE * pfile );
    int          vfs_ungetc( int c, vfs_FILE * pfile );
    int          vfs_puts( const char * , vfs_FILE * );
    char *       vfs_gets( char *, int, vfs_FILE * );

    void         vfs_empty_temp_directories();

    int          vfs_copyFile( const char *source, const char *dest );
    int          vfs_copyDirectory( const char *sourceDir, const char *destDir );

    int    vfs_removeDirectoryAndContents( const char * dirname, int recursive );

const char *vfs_resolveReadFilename(const char *src_filename);
const char *vfs_resolveWriteFilename(const char *src_filename);

const char *vfs_getError();
const char *vfs_getVersion();

int vfs_add_mount_point(const char *dirname, const char *relative_path, const char *mount_point, int append);
int vfs_remove_mount_point(const char *mount_point);

const char *vfs_convert_fname(const char *fname);
const char *vfs_convert_fname_sys(const char *fname);

void vfs_set_base_search_paths();
const char *vfs_mount_info_strip_path(const char * some_path);

void vfs_listSearchPaths();
    
bool vfs_readEntireFile(const char *filename, char **data, size_t *length);
bool vfs_writeEntireFile(const char *filename, const char *data, const size_t length);

// Wrap vfs into SDL_RWops
struct SDL_RWops;
   
/**
 * @brief
 *  Create SDL RW ops for the given filename.
 * @param filename
 *  the filename
 * @return
 *  the SDL RW ops on success, @a nullptr on failure
 */
SDL_RWops *vfs_openRWopsRead(const char *filename);

/**
 * @brief
 *  Create SDL RW ops for the given filename.
 * @param filename
 *  the filename
 * @return
 *  the SDL RW ops on success, @a nullptr on failure
 */
SDL_RWops *vfs_openRWopsWrite(const char *filename);

/**
 * @brief
 *  Create SDL RW ops for the given filename.
 * @param filename
 *  the filename
 * @return
 *  the SDL RW ops on success, @a nullptr on failure
 */
SDL_RWops *vfs_openRWopsAppend(const char *filename);
