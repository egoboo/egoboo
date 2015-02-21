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
#include "egolib/Script/Buffer.hpp"
#include "egolib/Script/Errors.hpp"

// Forward declaration.
struct ReadContext;
#if 0
char vfs_get_first_letter(ReadContext& ctxt);
#endif

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
protected:
    /**
     * @brief
     *  Read the next input character.
     * @return
     *  the character, EndOfInput (if the end of the input was reached) or Error (if an error occured)
     */
    int readInput();
    /**
     * @brief
     *  The file handle if the context is open, @a nullptr otherwise.
      */
    vfs_FILE *_file;
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
     *  The current extended character.
     */
    int _current;
    /**
     * @brief
     *  Lexeme accumulation buffer.
     */
    Ego::Script::Buffer _buffer;

    ReadContext(const std::string& loadName);

    ~ReadContext();

    /**
     * @brief
     *  Convert the contents of the buffer to a string value.
     * @return
     *  the string value
     * @throw Ego::Script::LexicalError
     *  if a lexical error occurs
     */
    std::string toString() const;

    /**
     * @brief
     *  Convert the contents of the buffer to a float value.
     * @return
     *  the float value
     * @throw Ego::Script::LexicalError
     *  if a lexical error occurs
     */
    float toReal() const;

    /**
     * @brief
     *  Convert the contents of the buffer to an signed int value.
     * @return
     *  the signed int value
     * @throw Ego::Script::LexicalError
     *  if a lexical error occurs
     */
    signed int toInteger() const;

    /**
     * @brief
     *  Convert the contents of the buffer to an unsigned int value.
     * @return
     *  the unsigned int value
     * @throw Ego::Script::LexicalError
     *  if a lexical error occurs
     */
    unsigned int toNatural() const;

    /**
     * @brief
     *  Get the load name of the file associated with this context.
     * @return
     *  the load name of the file associated with this context
     */
    const std::string& getLoadName() const;

    /**
     * @brief
     *  Get the line number within the file associated with this context.
     * @return
     *  the line number within the file associated with this context
     */
    size_t getLineNumber() const;

    /**
     * @brief
     *  Get if the context is open.
     * @return
     *  @a true if the context is open, @a false otherwise
     */
    bool isOpen() const;

    /**
     * @brief
     *  Open this context.
     * @return
     *  @a true if the context is open, @a false otherwise
     */
    bool ensureOpen();

    /**
     * @brief
     *  Close the context.
     * @post
     *  The context is closed.
     * @remark
     *  If the context is not open, a call to this method is noop.
     */
    void close();

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
     *  Add comment about lexical error.
     */
    template <typename EnumType>
    static EnumType readEnum(ReadContext& ctxt, EnumReader<EnumType>& enumReader)
    {
        using namespace std;
        std::string name = ctxt.readName();
        auto it = enumReader.get(name);
        if (it == enumReader.end())
        {
            throw Ego::Script::LexicalError(__FILE__,__LINE__,Ego::Script::Location(ctxt._loadName,ctxt._lineNumber));
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
     *  Add comment about lexical error and default.
     */
    template <typename EnumType>
    static EnumType readEnum(ReadContext& ctxt, EnumReader<EnumType>& enumReader, EnumType defaultValue)
    {
        using namespace std;
        std::string name = ctxt.readName();
        auto it = enumReader.get(name);
        if (it == enumReader.end())
        {
            log_warning("%s:%d: in file `%s`: `%s` is not an element of enum `%s`\n", __FILE__, __LINE__, ctxt._loadName.c_str(), name.c_str(), enumReader.getName().c_str());
            return defaultValue;
        }
        return *it;
    }

    /// Special value for extended character indicating the start of the input.
    static const int StartOfInput = -1;
    /// Special value for extended character indicating the end of the input.
    static const int EndOfInput = -2;
    /// Special value for extended character indicating an error.
    static const int Error = -3;

    void write(char chr);
    void save();
    void next();
    void writeAndNext(char chr);
    void saveAndNext();
    bool is(int chr);
    bool is(int first, int last);

    /**
     * @brief
     *  Get if a character is a whitespace character.
     * @param chr
     *  the character
     * @return
     *  @a true if the character is a whitespace character,
     *  @a false otherwise
     * @remark
     *  @code
     *  whitespace = Space | Tabulator
     *  @endcode
     * @todo
     *  Remove parameterized form.
     */
    bool isWhiteSpace(char chr);
    bool isWhiteSpace();

    /**
     * @brief
     *  Get if a character is a new line character.
     * @param chr
     *  the character
     * @return
     *  @a true if the character is a new line character,
     *  @a false otherwise
     * @remark
     *  @code
     *  newline = LineFeed | CarriageReturn
     *  @endcode
     * @todo
     *  Remove parameterized form.
     */
    bool isNewLine(char chr);
    bool isNewLine();

    bool isAlpha();
    bool isDigit();

    /// The new line character.
    static const char LineFeed = '\n';
    /// The carriage return character.
    static const char CarriageReturn = '\r';
    /// The zero terminator character.
    static const char ZeroTerminator = '\0';
    /// The tabulator character.
    static const char Tabulator = '\t';
    /// The space character.
    static const char Space = ' ';

    /**
     * @brief
     *  Read everything from the current character to the next newline character or end of file.
     *  Whitespaces in front and the newline character are dropped.
     */
    std::string readToEndOfLine();
    /**
     * @throw Ego::Script::LexicalError
     *  if a lexical error occurs
     * @remark
     *  A single line comment in this revision are the strings
     *  @code
     *  singleLineComment := '//'(any - NewLine)* 
     *  @endcode
     */
    std::string readSingleLineComment();

    /**
     * @throw Ego::Script::LexicalError
     *  if a lexical error occurs
     * @remark
     *  A printable character in this revision are the strings
     *  @code
     *  verbatimChar := alphabetic | digit | '!' | '?' | '='
     *  @endcode
     */
    char readPrintable();

    /**
     * @throw Ego::Script::LexicalError
     *  if a lexical error occurs
     * @remark
     *  A character literal in this revision are the strings
     *  @code
     *  charLit := '\'' charLitBody '\''
     *  charLitBody := (any - '\'') | ('\\n' | '\\t' | '\\\'')
     *  @endcode
     * @todo
     *  Use Unicode notation for better readbility.
     */
    char readCharLit();

    /**
     * @remark
     *  A string literal in this revision are the strings
     *  @code
     *  stringLiteral := (character|digit|'~'|'_')*
     *  @endcode
     */
    std::string readStringLit();

    /**
     * @throw Ego::Script::LexicalError
     *  if a lexical error occurs
     * @remark
     *  An integer literal in this revision are the strings
     *  @code
     *  integer := ('+'|'-')? digit+ ('e'|'E' digit+)?
     *  @endcode
     */
    int readInt();

    /**
     * @throw Ego::Script::LexicalError
     *  if a lexical error occurs
     * @remark
     *  A natural literal in this revision is a string
     *  @code
     *  integer := '+'? digit+ ('e'|'E' digit+)?
     *  @endcode
     */
    unsigned int readNat();

    /**
     * @throw Ego::Script::LexicalError
     *  if a lexical error occurs
     * @remark
     *  A real literal in this revision is a string
     *  @code
     *  float := ('+'|'-')? digit+ ('.' digit)* realExponent?
     *        | ('+'|'-')? '.' digit+ realExponent?
     *  realExponent := ('e'|'E' ('+'|'-')? digit+)?
     *  @endcode
     */
    float readReal();

    /**
     * @throw Ego::Script::LexicalError
     *  if a lexical error occurs
     * @remark
     *   A boolean literal in this revision is a string
     *   @code
     *   boolean := 'T' | 'F'
     *   @endcode
     */
    bool readBool();

    /**
    * @throw Ego::Script::LexicalError
    *  if a lexical error occurs
    * @remark
    *   A name in this revision is a string
    *   @code
    *   name := (alphabetic|underscore) (digit|alphabetic|underscore)*
    *   @endcode
    */
    std::string readName();

    /**
     * @throw Ego::Script::LexicalError
     *  if a lexical error occurs
     * @remark
     *  An IDSZ literal in this revision is a string
     *  @code
     *  idsz := '[' (alphabetic|digit|'_')^4 ']'
     *  @endcode
     */
    IDSZ readIDSZ();

    /**
     * @brief
     *  Skip input until either the end of the input or a non-whitespace character is reached.
     */
    void skipWhiteSpaces();

    /**
     * @brief
     *  Skip input until either the end of the input or the specified delimiter is reached.
     *  If the specified delimiter is reached, skip it as well.
     * @param option
     *  if @a true and the specified delimiter was not encountered, a lexical error is raised
     * @throw Ego::Script::LexicalError
     *  if a lexical error occurs
     * @return
     *  @a true if the specifed delimiter is was skipped, @a false otherwise
     */
    bool skipToDelimiter(char delimiter, bool optional);

    /**
     * @brief
     *  Skip input until either the end of the input or a colon is reached.
     *  If a colon is reached, skip it as well.
     * @param option
     *  if @a true and a colon was not encountered, a lexical error is raised
     * @throw Ego::Script::LexicalError
     *  if a lexical error occurs
     * @return
     *  @a true if a colon was skipped, @a false otherwise
     */
    bool skipToColon(bool optional);

};

// Utility functions.
void vfs_get_next_name(ReadContext& ctxt, char *buf, size_t max);
bool vfs_get_next_range(ReadContext& ctxt, FRange *range);
bool vfs_get_next_pair(ReadContext& ctxt, IPair *pair);
IDSZ vfs_get_next_idsz(ReadContext& ctxt);
bool vfs_get_next_bool(ReadContext& ctxt);
void vfs_get_next_string_lit(ReadContext& ctxt, char *str, size_t max);
UFP8_T vfs_get_ufp8(ReadContext& ctxt);
SFP8_T vfs_get_sfp8(ReadContext& ctxt);
char vfs_get_next_printable(ReadContext& ctxt);
signed int vfs_get_next_int(ReadContext& ctxt);
unsigned int vfs_get_next_nat(ReadContext& ctxt);
float vfs_get_next_float(ReadContext& ctxt);
UFP8_T vfs_get_next_ufp8(ReadContext& ctxt);
SFP8_T vfs_get_next_sfp8(ReadContext& ctxt);

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

int    vfs_get_version(ReadContext& ctxt);
DamageType vfs_get_damage_type(ReadContext& ctxt);
DamageModifier vfs_get_damage_modifier(ReadContext& ctxt);
float  vfs_get_damage_resist(ReadContext& ctxt);
/**
 * @brief
 *  Read a name (transitional C form).
 * @param buf, max
 *  @a buf is a pointer to a buffer of <tt>max+1</tt> bytes
 * @see
 *  ReadContext::readName
 * @todo
 *  Transitional C form, remove this.
 */
void vfs_read_name(ReadContext& ctxt, char *buf, size_t max);
/**
 * @brief
 *  Read a string (transitional C form).
 * @param buf, max
 *  @a buf is a pointer to a buffer of <tt>max+1</tt> bytes
 * @see
 *  ReadContext::readString
 * @todo
 *  Transitional C form, remove this.
 */
void vfs_read_string_lit(ReadContext& ctxt, char *buf, size_t max);
#if 0
/**
 * @brief
 *  Read a line.
 * @param buf, max
 *  @a buf is a pointer to a buffer of <tt>max+1</tt> bytes
 * @remark
 *  A line in this revision are the strings
 *  @code
 *  line := string
 *  @endcode
 */
void vfs_get_line(ReadContext& ctxt, char *str, size_t max);
#endif
/**
 * @brief
 *  Read a range.
 * @remark
 *  In this revision a range are the strings
 *  @code
 *  range := real ('-' real)?
 *  @endcode
 */
bool vfs_get_range(ReadContext& ctxt, FRange *range);
/**
 * @brief
 *  Read a pair.
 * @remark
 *  In this revision a pair are the strings
 *  @code
 *  pair := range
 *  @endcode
 */
bool vfs_get_pair(ReadContext& ctxt, IPair *pair);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#if 0
char vfs_get_next_printable(ReadContext& ctxt);
void vfs_get_next_line(ReadContext& ctxt, char *str, size_t max);
#endif
DamageType vfs_get_next_damage_type(ReadContext& ctxt);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
// Stuff to remove.

bool read_to_delimiter_vfs(ReadContext& ctxt, std::string& buffer, char delimiter, bool optional);
bool read_to_colon_vfs(ReadContext& ctxt, std::string& buffer, bool optional);
bool read_to_delimiter_list_vfs(ReadContext& ctxt, std::string& buffer, const char *delimiters, bool optional);

void make_newloadname(const char *modname, const char *appendname, char *newloadname);

bool copy_line_vfs(vfs_FILE * fileread, vfs_FILE * filewrite);
char vfs_get_first_letter(ReadContext& ctxt);
char * copy_to_delimiter_mem(char * pmem, char * pmem_end, vfs_FILE * filewrite, int delim, char * user_buffer, size_t user_buffer_len);
bool copy_to_delimiter_vfs(vfs_FILE * fileread, vfs_FILE * filewrite, int delim, char * buffer, size_t bufflen);
int read_skin_vfs(const char *filename);
void GLSetup_SupportedFormats();
Uint32  ego_texture_load_vfs(oglx_texture_t *texture, const char *filename, Uint32 key);
extern STRING TxFormatSupported[20]; ///< OpenGL icon surfaces
extern Uint8 maxformattypes;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
// Stuff to encapsulte in a WriterContext.
bool vfs_put_version(vfs_FILE* filewrite, const int version);
void vfs_put_int(vfs_FILE* filewrite, const char* text, int value);
void vfs_put_float(vfs_FILE* filewrite, const char* text, float value);
void vfs_put_ufp8(vfs_FILE* filewrite, const char* text, UFP8_T value);
void vfs_put_sfp8(vfs_FILE* filewrite, const char* text, SFP8_T value);
void vfs_put_bool(vfs_FILE* filewrite, const char* text, bool value);
void vfs_put_damage_type(vfs_FILE* filewrite, const char* text, Uint8 value);
void vfs_put_action(vfs_FILE* filewrite, const char* text, Uint8 value);
void vfs_put_gender(vfs_FILE* filewrite, const char* text, Uint8 value);
void vfs_put_range(vfs_FILE* filewrite, const char* text, FRange value);
void vfs_put_pair(vfs_FILE* filewrite, const char* text, IPair value);
void vfs_put_string_under(vfs_FILE* filewrite, const char* text, const char* usename);
void vfs_put_idsz(vfs_FILE* filewrite, const char* text, IDSZ idsz);
void vfs_put_expansion(vfs_FILE* filewrite, const char* text, IDSZ idsz, int value);
void vfs_put_expansion_float(vfs_FILE* filewrite, const char* text, IDSZ idsz, float value);
void vfs_put_expansion_string(vfs_FILE* filewrite, const char* text, IDSZ idsz, const char * value);
void vfs_put_range_raw(vfs_FILE* filewrite, FRange val);


