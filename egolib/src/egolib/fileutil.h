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

/// @file  egolib/fileutil.h
/// @brief read/write values from/to files

#pragma once

#include "egolib/typedef.h"

#include "egolib/vfs.h"
#include "egolib/log.h"
#include "egolib/file_common.h"

//--------------------------------------------------------------------------------------------
// TYPEDEFS
//--------------------------------------------------------------------------------------------

    struct oglx_texture_t;

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

// ASCII terminal/printer control codes
#    define ASCII_NUL_CHAR  '\x00'            /**< null */
#    define ASCII_SOH_CHAR  '\x01'            /**< start of heading */
#    define ASCII_STX_CHAR  '\x02'            /**< start of text */
#    define ASCII_ETX_CHAR  '\x03'            /**< end of text */
#    define ASCII_EOT_CHAR  '\x04'            /**< end of transmission */
#    define ASCII_ENQ_CHAR  '\x05'            /**< enquiry */
#    define ASCII_ACK_CHAR  '\x06'            /**< acknowledge */
#    define ASCII_BEL_CHAR  '\x07'            /**< bell */
#    define ASCII_BS_CHAR   '\x08'            /**< backspace */
#    define ASCII_HT_CHAR   '\x09'            /**< horizontal tab */
#    define ASCII_NL_CHAR   '\x0A'            /**< new line (or LF, line feed) */
#    define ASCII_VT_CHAR   '\x0B'            /**< vertical tab */
#    define ASCII_NP_CHAR   '\x0C'            /**< new page (or FF, form feed) */
#    define ASCII_CR_CHAR   '\x0D'            /**< carriage return */
#    define ASCII_SO_CHAR   '\x0E'            /**< shift out */
#    define ASCII_SI_CHAR   '\x0F'            /**< shift in */
#    define ASCII_DLE_CHAR  '\x10'            /**< data link escape */
#    define ASCII_DC1_CHAR  '\x11'            /**< device control 1 */
#    define ASCII_DC2_CHAR  '\x12'            /**< device control 2 */
#    define ASCII_DC3_CHAR  '\x13'            /**< device control 3 */
#    define ASCII_DC4_CHAR  '\x14'            /**< device control 4 */
#    define ASCII_NAK_CHAR  '\x15'            /**< negative acknowledge */
#    define ASCII_SYN_CHAR  '\x16'            /**< synchronous idle */
#    define ASCII_ETB_CHAR  '\x17'            /**< end of transmission block */
#    define ASCII_CAN_CHAR  '\x18'            /**< cancel */
#    define ASCII_EM_CHAR   '\x19'            /**< end of medium */
#    define ASCII_SUB_CHAR  '\x1A'            /**< substitute */
#    define ASCII_ESC_CHAR  '\x1B'            /**< escape */
#    define ASCII_FS_CHAR   '\x1C'            /**< file separator */
#    define ASCII_GS_CHAR   '\x1D'            /**< group separator */
#    define ASCII_RS_CHAR   '\x1E'            /**< record separator */
#    define ASCII_US_CHAR   '\x1F'            /**< unit separator */
#    define ASCII_SP_CHAR   '\x20'            /**< space  */

/// @note win32 systems (and some others) handle newlines by using a combinarion
///       of linefeed and carriage return characters. So...
///       the numerical values of '\n' and '\r' may vary from system to system and
///       may be different for input and output (i.e. '\n' writing '\x0D\x0A" on win32 systems)

#    define ASCII_LINEFEED_CHAR    ASCII_NL_CHAR

/// @note the following escape codes are translated by the compiler
///       to whatever encoding is necessary
#    define C_BELL_CHAR            '\a'
#    define C_BACKSPACE_CHAR       '\b'
#    define C_FORMFEED_CHAR        '\f'
#    define C_NEW_LINE_CHAR        '\n'
#    define C_CARRIAGE_RETURN_CHAR '\r'
#    define C_TAB_CHAR             '\t'
#    define C_VERTICAL TAB_CHAR    '\v'
#    define C_SINGLE_QUOTE_CHAR    '\''
#    define C_DOUBLE_QUOTE_CHAR    '\"'

#define TRANSCOLOR                      0


#include "egolib/Profiles/ReaderUtilities.hpp"
#include "egolib/Script/Location.hpp"
#include "egolib/Script/LexicalError.hpp"

// Forward declaration.
struct ReadContext;
char vfs_get_first_letter(ReadContext& ctxt);

/**
 * @brief
 *  A context for reading a file.
 * @details
 *  A context is associated with a load name (i.e. a name of a file in the virtual file system of Egoboo) from
 *  which it can read data. To read data, the context must be opened and if it is is sucessfully opened, it
 *  can be used to read data from that file. In particular, the associoated file does not have to exist until the
 *  point at which the context is opened.
 * @author
 *  Michael Heilmann
 */
struct ReadContext
{
public:
    /**
     * @brief
     *  The load name of the file of this context.
     */
    std::string _loadName;
    /**
     * @brief
     *  The line number in the file of this context.
     */
    size_t _lineNumber;
    /**
     * @brief
     *  The file handle if the context is open, @a nullptr otherwise.
     */
    vfs_FILE *_file;

    ReadContext(const std::string& loadName)
        : _loadName(loadName), _file(nullptr), _lineNumber(1)
    {
    }

    ~ReadContext()
    {
        if (_file)
        {
            vfs_close(_file);
            _file = nullptr;
        }
            
    }

    /**
     * @brief
     *  Get the load name of the file associated with this context.
     * @return
     *  the load name of the file associated with this context
     */
    const std::string& getLoadName() const
    {
        return _loadName;
    }

    /**
     * @brief
     *  Get if the context is open.
     * @return
     *  @a true if the context is open, @a false otherwise
     */
    bool isOpen() const
    {
        return nullptr != _file;
    }

    /**
     * @brief
     *  Open this context.
     * @return
     *  @a true if the context is open, @a false otherwise
     */
    bool ensureOpen()
    {
        if (!_file)
        {
            if (0 == vfs_exists(_loadName.c_str()))
            {
                return false;
            }
            _file = vfs_openRead(_loadName.c_str());
            if (!_file)
            {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief
     *  Close the context.
     * @post
     *  The context is closed.
     * @remark
     *  If the context is not open, a call to this method is noop.
     */
    void close()
    {
        if (_file)
        {
            vfs_close(_file);
        }
        _file = nullptr;
    }

    /**
     * @brief
     *  Read an enumeration.
     * @param ctxt
     *  the context
     * @param enumReader
     *  the enum reader to be used
     * @return
     *  the enumeration value
     * @todo
     *  Return type should be @a EnumType; add comment about lexical exception.
     */
    template <typename EnumType>
    static int readEnum(ReadContext& ctxt, EnumReader<EnumType>& enumReader)
    {
        using namespace std;
        char chr = vfs_get_first_letter(ctxt);
        auto it = enumReader.get(std::string(1, chr));
        if (it == enumReader.end())
        {
            throw Ego::Script::LexicalError(_loadName,_lineNumber);
        }
        return *it;
    }

    /**
     * @brief
     *  Read an enumeration.
     * @param ctxt
     *  the context
     * @param enumReader
     *  the enum reader to be used
     * @return
     *  the enumeration value
     * @todo
     *  Return type should be @a EnumType; add comment about lexical exception and default.
     */
    template <typename EnumType>
    static int readEnum(ReadContext& ctxt, EnumReader<EnumType>& enumReader, EnumType default)
    {
        using namespace std;
        char chr = vfs_get_first_letter(ctxt);
        auto it = enumReader.get(std::string(1, chr));
        if (it == enumReader.end())
        {
            log_warning("%s:%d: in file `%s`: `%c` is not an element of enum `%s`\n", __FILE__, __LINE__, ctxt._loadName.c_str(), chr, enumReader.getName().c_str());
            return default;
        }
        return *it;
    }
};


extern const char *parse_filename;          ///< For debuggin' goto_colon_vfs
extern int parse_line_number;               ///< For debuggin' goto_colon_vfs

extern  STRING     TxFormatSupported[20]; ///< OpenGL icon surfaces
extern  Uint8      maxformattypes;

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

void   make_newloadname( const char *modname, const char *appendname, char *newloadname );

bool goto_delimiter_vfs( char * buffer, vfs_FILE* fileread, char delim, bool optional );
char   goto_delimiter_list_vfs( char * buffer, vfs_FILE* fileread, const char * delim_list, bool optional );
bool goto_colon_vfs( char * buffer, vfs_FILE* fileread, bool optional );
char * goto_colon_mem( char * buffer, char * pmem, char * pmem_end, bool optional );

bool copy_line_vfs( vfs_FILE * fileread, vfs_FILE * filewrite );
char * copy_to_delimiter_mem( char * pmem, char * pmem_end, vfs_FILE * filewrite, int delim, char * user_buffer, size_t user_buffer_len );
bool copy_to_delimiter_vfs( vfs_FILE * fileread, vfs_FILE * filewrite, int delim, char * buffer, size_t bufflen );

int    vfs_get_version(ReadContext& ctxt);
bool vfs_put_version( vfs_FILE* filewrite, const int version );

char   vfs_get_next_char(ReadContext& ctxt);
int    vfs_get_next_int(ReadContext& ctxt);
float  vfs_get_next_float(ReadContext& ctxt);
UFP8_T vfs_get_next_ufp8(ReadContext& ctxt);
SFP8_T vfs_get_next_sfp8(ReadContext& ctxt);
bool vfs_get_next_name(ReadContext& ctxt, char * name, size_t name_len);
bool vfs_get_next_range(ReadContext& ctxt, FRange *prange);
bool vfs_get_next_pair(ReadContext& ctxt, IPair *ppair);
IDSZ   vfs_get_next_idsz(ReadContext& ctxt);
bool vfs_get_next_bool(ReadContext& ctxt);
bool vfs_get_next_string(ReadContext& ctxt, char *str, size_t str_len);
bool vfs_get_next_line(ReadContext& ctxt, char *str, size_t str_len);

char vfs_get_first_letter(ReadContext& ctxt);
    
UFP8_T vfs_get_ufp8(ReadContext& ctxt);
SFP8_T vfs_get_sfp8(ReadContext& ctxt);
/**
 * @throw Id::LexicalError
 *  if a lexical error occurs
 * @remark
 *   A boolean literal in this revision is the string
 *   @code
 *   boolean := 'T' | 'F'
 *   @endcode
 */
bool vfs_get_bool(ReadContext& ctxt);
float vfs_get_float(ReadContext& ctxt);
Sint32 vfs_get_int(ReadContext& ctxt);
IDSZ   vfs_get_idsz(ReadContext& ctxt);
int vfs_get_damage_type(ReadContext& ctxt);
int vfs_get_next_damage_type(ReadContext& ctxt);
Uint8 vfs_get_damage_modifier(ReadContext& ctxt);
float  vfs_get_damage_resist(ReadContext& ctxt);
bool vfs_get_name(ReadContext& ctxt, char *szName, size_t max_len);
bool vfs_get_string(ReadContext& ctxt, char * str, size_t str_len);
bool vfs_get_line(ReadContext& ctxt, char * str, size_t str_len);
bool vfs_get_range(ReadContext& ctxt, FRange * prange);
bool vfs_get_pair(ReadContext& ctxt, IPair * ppair);

void vfs_put_int( vfs_FILE* filewrite, const char* text, int ival );
void vfs_put_float( vfs_FILE* filewrite, const char* text, float fval );
void vfs_put_ufp8( vfs_FILE* filewrite, const char* text, UFP8_T ival );
void vfs_put_sfp8( vfs_FILE* filewrite, const char* text, SFP8_T ival );
void vfs_put_bool( vfs_FILE* filewrite, const char* text, bool truth );
void vfs_put_damage_type( vfs_FILE* filewrite, const char* text, Uint8 damagetype );
void vfs_put_action( vfs_FILE* filewrite, const char* text, Uint8 action );
void vfs_put_gender( vfs_FILE* filewrite, const char* text, Uint8 gender );
void vfs_put_range( vfs_FILE* filewrite, const char* text, FRange val );
void vfs_put_pair( vfs_FILE* filewrite, const char* text, IPair val );
void vfs_put_string_under( vfs_FILE* filewrite, const char* text, const char* usename );
void vfs_put_idsz( vfs_FILE* filewrite, const char* text, IDSZ idsz );
void vfs_put_expansion( vfs_FILE* filewrite, const char* text, IDSZ idsz, int value );
void vfs_put_expansion_float( vfs_FILE* filewrite, const char* text, IDSZ idsz, float value );
void vfs_put_expansion_string( vfs_FILE* filewrite, const char* text, IDSZ idsz, const char * str );

void vfs_put_range_raw( vfs_FILE* filewrite, FRange val );
int read_skin_vfs( const char *filename );

void    GLSetup_SupportedFormats();
Uint32  ego_texture_load_vfs(oglx_texture_t *texture, const char *filename, Uint32 key);
