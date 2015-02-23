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

/// @file egolib/fileutil.c
/// @brief Implementation of Egoboo file utilities
/// @details

#include "egolib/fileutil.h"

#include "egolib/log.h"
#include "egolib/strutil.h"
#include "egolib/platform.h"
#include "egolib/egoboo_setup.h"

#include "egolib/extensions/ogl_texture.h"

#include "egolib/_math.h"

// includes for egoboo constants
#include "game/mad.h"                    // for ACTION_* constants

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

STRING          TxFormatSupported[20]; // OpenGL icon surfaces
Uint8           maxformattypes;


const int ReadContext::FirstValidChar = static_cast<int>(std::numeric_limits<char>::min()) - 1;
const int ReadContext::LastValidChar = static_cast<int>(std::numeric_limits<char>::max()) + 1;
const int ReadContext::StartOfInput = ReadContext::FirstValidChar - 1;
const int ReadContext::EndOfInput = ReadContext::FirstValidChar - 2;
const int ReadContext::Error = ReadContext::FirstValidChar - 3;

ReadContext::ReadContext(const std::string& loadName) :
    _loadName(loadName), _file(nullptr), _lineNumber(1), _buffer(5012), _current(StartOfInput)
{
}

ReadContext::~ReadContext()
{
    if (_file)
    {
        vfs_close(_file);
        _file = nullptr;
    }

}

std::string ReadContext::toString() const
{
    return _buffer.toString();
}

float ReadContext::toReal() const
{
    float x;
    try
    {
        x = std::stof(_buffer.toString());
    }
    catch (std::exception&)
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    return x;

}

signed int ReadContext::toInteger() const
{
    long x;
    try
    {
        x = std::stol(_buffer.toString());
    }
    catch (std::exception&)
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    if (x < std::numeric_limits<signed int>::min() || x > std::numeric_limits<signed int>::max())
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    return static_cast<int>(x);
}

unsigned int ReadContext::toNatural() const
{
    unsigned long x;
    try
    {
        x = std::stoul(_buffer.toString());
    }
    catch (std::exception&)
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    if (x > std::numeric_limits<unsigned int>::max())
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    return static_cast<unsigned int>(x);
}

const std::string& ReadContext::getLoadName() const
{
    return _loadName;
}

size_t ReadContext::getLineNumber() const
{
    return _lineNumber;
}

bool ReadContext::isOpen() const
{
    return nullptr != _file;
}

bool ReadContext::ensureOpen()
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
        _current = StartOfInput;
        _lineNumber = 1;
    }
    return true;
}

void ReadContext::close()
{
    if (_file)
    {
        vfs_close(_file);
    }
    _file = nullptr;
}

void ReadContext::skipWhiteSpaces()
{
    if (is(StartOfInput))
    {
        next();
    }
    if (is(Error))
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(getLoadName(), getLineNumber()));
    }
    if (is(EndOfInput))
    {
        return;
    }
    while (isWhiteSpace())
    {
        next();
        if (is(Error))
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(getLoadName(), getLineNumber()));
        }
        if (is(EndOfInput))
        {
            return;
        }
    }
}

bool ReadContext::is(int chr)
{
    return chr == _current;
}

bool ReadContext::is(int first, int last)
{
    if (first > last) throw std::invalid_argument("first > last");
    return (first <= _current)
        && (_current <= last);
}

bool ReadContext::isNewLine(char chr)
{
    return LineFeed == chr || CarriageReturn == chr;
}

bool ReadContext::isNewLine()
{
    return is(LineFeed) || is(CarriageReturn);

}

bool ReadContext::isWhiteSpace(char chr)
{
    return Space == chr || Tabulator == chr;
}

bool ReadContext::isWhiteSpace()
{
    return is(Space) || is(Tabulator);
}

bool ReadContext::isAlpha()
{
    return is('a', 'z') || is('A', 'Z');
}

bool ReadContext::isDigit()
{
    return is('0', '9');
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

IDSZ ReadContext::readIDSZ()
{
    /// @author ZZ
    /// @details This function reads and returns an IDSZ tag, or IDSZ_NONE if there wasn't one

    char c[4];
    if (is(StartOfInput))
    {
        next();
    }
    skipWhiteSpaces();
    // `'['`
    if (!is('['))
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName,_lineNumber));
    }
    next();
    // `(<alphabetic>|<digit>|'_')^4`
    for (size_t i = 0; i < 4; ++i)
    {
        if (!isAlpha() && !isDigit() && !is('_'))
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
        }
        c[i] = static_cast<char>(_current);
        next();
    }
    // `']'`
    if (!is(']'))
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    next();
    return MAKE_IDSZ(c[0], c[1], c[2], c[3]);
}

//--------------------------------------------------------------------------------------------
bool copy_line_vfs( vfs_FILE * fileread, vfs_FILE * filewrite )
{
    /// @author BB
    /// @details copy a line of arbitrary length, in chunks of length sizeof(linebuffer)
    /// @todo This should be moved to file_common.c

    char linebuffer[64];
    if ( NULL == fileread || NULL == filewrite ) return false;
    if ( vfs_eof( fileread ) || vfs_eof( filewrite ) ) return false;

    vfs_gets( linebuffer, SDL_arraysize( linebuffer ), fileread );
    vfs_puts( linebuffer, filewrite );
    while ( strlen( linebuffer ) == SDL_arraysize( linebuffer ) )
    {
        vfs_gets( linebuffer, SDL_arraysize( linebuffer ), fileread );
        vfs_puts( linebuffer, filewrite );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool ReadContext::skipToDelimiter(char delimiter, bool optional)
{
    if ('\0' == delimiter) std::invalid_argument("\\0 == delimiter");
    if (is(StartOfInput))
    {
        next();
    }
    while (true)
    {
        if (is(Error))
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(getLoadName(), getLineNumber()));
        }
        if (is(EndOfInput))
        {
            if (optional)
            {
                return false;
            }
            else
            {
                throw Ego::Script::MissingDelimiterError(__FILE__, __LINE__, Ego::Script::Location(getLoadName(), getLineNumber()), delimiter);
            }
        }
        bool isDelimiter = is(delimiter);
        if (isNewLine())
        {
            skipNewLine();
        }
        else
        {
            next();
        }
        if (isDelimiter)
        {
            return true;
        }
    }
}

bool read_to_delimiter_vfs(ReadContext& ctxt, std::string& buffer, char delimiter, bool optional)
{
    if ('\0' == delimiter) std::invalid_argument("\\0 == delimiter");
    if (ctxt.is(ReadContext::StartOfInput))
    {
        ctxt.next();
    }
    ctxt._buffer.clear();
    while (true)
    {
        if (ctxt.is(ReadContext::Error))
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(ctxt.getLoadName(), ctxt.getLineNumber()));
        }
        if (ctxt.is(ReadContext::EndOfInput))
        {
            if (optional)
            {
                buffer = ctxt._buffer.toString();
                return false;
            }
            else
            {
                throw Ego::Script::MissingDelimiterError(__FILE__, __LINE__, Ego::Script::Location(ctxt.getLoadName(), ctxt.getLineNumber()), delimiter);
            }
        }
        bool isDelimiter = ctxt.is(delimiter);
        if (ctxt.isNewLine())
        {
            int old = ctxt._current;
            ctxt.next();
            if (ctxt.isNewLine() && old != ctxt._current)
            {
                ctxt.next();
            }
            ctxt.write('\n');
            ctxt._lineNumber++;
        }
        else
        {
            ctxt.saveAndNext();
        }
        if (isDelimiter)
        {
            buffer = ctxt._buffer.toString();
            return true;
        }
    }
    
}

//--------------------------------------------------------------------------------------------
bool read_to_delimiter_list_vfs(ReadContext& ctxt, std::string& buffer, const char *delimiters, bool optional)
{
    if (!delimiters) std::invalid_argument("\\0 == delimiter");
    if (ctxt.is(ReadContext::StartOfInput))
    {
        ctxt.next();
    }
    ctxt._buffer.clear();
    while (true)
    {
        if (ctxt.is(ReadContext::Error))
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(ctxt.getLoadName(), ctxt.getLineNumber()));
        }
        if (ctxt.is(ReadContext::EndOfInput))
        {
            if (optional)
            {
                buffer = ctxt._buffer.toString();
                return false;
            }
            else
            {
                /// @todo Need to be able to pass a list of delimiters.
                throw Ego::Script::MissingDelimiterError(__FILE__, __LINE__, Ego::Script::Location(ctxt.getLoadName(), ctxt.getLineNumber()), delimiters[0]);
            }
        }

        bool isDelimiter = false;
        for (size_t i = 0, n = strlen(delimiters); i < n; ++i)
        {
            isDelimiter |= ctxt.is(delimiters[i]);
        }
        if (ctxt.isNewLine())
        {
            int old = ctxt._current;
            ctxt.next();
            if (ctxt.isNewLine() && old != ctxt._current)
            {
                ctxt.next();
            }
            ctxt.write('\n');
            ctxt._lineNumber++;
        }
        else
        {
            ctxt.saveAndNext();
        }
        if (isDelimiter)
        {
            buffer = ctxt._buffer.toString();
            return true;
        }
    }

}

//--------------------------------------------------------------------------------------------

int ReadContext::readInput()
{
    // (1) Read a single Byte.
    uint8_t byte;
    size_t result = vfs_read(&byte, 1, 1, _file);
    if (result != 1)
    {
        if (result > 1)
        {
            printf("%s:%d: unreachable code reached\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (vfs_error(_file))
        {
            return Error;
        }
        else
        {
            if (!vfs_eof(_file))
            {
                printf("%s:%d: unreachable code reached\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            return EndOfInput;
        }
    }
    // (2) Verify that it is a Byte the represents the starting Byte of a UTF-8 character sequence of length 1.
    if (byte > 0x7F)
    {
        return Error;
    }
    // (3) Propage the Byte to an extended character.
    int chr = byte;
    // (4) Verify that it is not the zero terminator.
    if (ZeroTerminator == chr)
    {
        return Error;
    }
    return byte;
}

bool ReadContext::skipToColon(bool optional)
{
    return skipToDelimiter(':', optional);
}

bool read_to_colon_vfs(ReadContext& ctxt,std::string& buffer, bool optional)
{
    return read_to_delimiter_vfs(ctxt, buffer, ':', optional);
}

//--------------------------------------------------------------------------------------------
void vfs_read_string_lit(ReadContext& ctxt, char *buffer, size_t max)
{
    std::string _literal = ctxt.readStringLit();
    strncpy(buffer,_literal.c_str(), max);
    str_decode(buffer, max, buffer);
}
void vfs_read_name(ReadContext& ctxt, char *buffer, size_t max)
{
    std::string _literal = ctxt.readName();
    strncpy(buffer,_literal.c_str(),max);
}
//--------------------------------------------------------------------------------------------
void vfs_put_int( vfs_FILE* filewrite, const char* text, int ival )
{
    /// @author ZZ
    /// @details This function kinda mimics fprintf for integers

    vfs_printf( filewrite, "%s %d\n", text, ival );
}

//--------------------------------------------------------------------------------------------
void vfs_put_float( vfs_FILE* filewrite, const char* text, float fval )
{
    /// @author ZZ
    /// @details This function kinda mimics fprintf for integers

    vfs_printf( filewrite, "%s %f\n", text, fval );
}

//--------------------------------------------------------------------------------------------
void vfs_put_ufp8( vfs_FILE* filewrite, const char* text, UFP8_T ival )
{
    /// @author ZZ
    /// @details This function kinda mimics fprintf for integers

    vfs_printf( filewrite, "%s %f\n", text, FP8_TO_FLOAT( ival ) );
}

//--------------------------------------------------------------------------------------------
void vfs_put_sfp8( vfs_FILE* filewrite, const char* text, SFP8_T ival )
{
    /// @author ZZ
    /// @details This function kinda mimics fprintf for integers

    vfs_printf( filewrite, "%s %f\n", text, FP8_TO_FLOAT( ival ) );
}

//--------------------------------------------------------------------------------------------
void vfs_put_bool(vfs_FILE* filewrite, const char* text, bool truth)
{
    /// @author ZZ
    /// @details This function kinda mimics vfs_printf for the output of
    ///    true false statements

    vfs_printf( filewrite, "%s", text );
    vfs_printf( filewrite, truth ? "TRUE" : "FALSE" );
    vfs_printf( filewrite, "\n" );
}

//--------------------------------------------------------------------------------------------
void vfs_put_damage_type( vfs_FILE* filewrite, const char* text, Uint8 damagetype )
{
    /// @author ZZ
    /// @details This function kinda mimics vfs_printf for the output of
    ///    SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements

    vfs_printf( filewrite, "%s", text );

    switch ( damagetype )
    {
        case DAMAGE_SLASH: vfs_printf( filewrite, "SLASH" ); break;
        case DAMAGE_CRUSH: vfs_printf( filewrite, "CRUSH" ); break;
        case DAMAGE_POKE : vfs_printf( filewrite, "POKE" ); break;
        case DAMAGE_HOLY : vfs_printf( filewrite, "HOLY" ); break;
        case DAMAGE_EVIL : vfs_printf( filewrite, "EVIL" ); break;
        case DAMAGE_FIRE : vfs_printf( filewrite, "FIRE" ); break;
        case DAMAGE_ICE  : vfs_printf( filewrite, "ICE" ); break;
        case DAMAGE_ZAP  : vfs_printf( filewrite, "ZAP" ); break;

        default:
        case DAMAGE_NONE : vfs_printf( filewrite, "NONE" ); break;
    }

    vfs_printf( filewrite, "\n" );
}

//--------------------------------------------------------------------------------------------
void vfs_put_action( vfs_FILE* filewrite, const char* text, Uint8 action )
{
    /// @author ZZ
    /// @details This function kinda mimics vfs_printf for the output of
    ///    SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements

    vfs_printf( filewrite, "%s", text );

    switch ( action )
    {
        case ACTION_DA: vfs_printf( filewrite, "DANCE\n" );    break;
        case ACTION_UA: vfs_printf( filewrite, "UNARMED\n" ); break;
        case ACTION_TA: vfs_printf( filewrite, "THRUST\n" );  break;
        case ACTION_CA: vfs_printf( filewrite, "CHOP\n" );    break;
        case ACTION_SA: vfs_printf( filewrite, "SLASH\n" );   break;
        case ACTION_BA: vfs_printf( filewrite, "BASH\n" );    break;
        case ACTION_LA: vfs_printf( filewrite, "LONGBOW\n" ); break;
        case ACTION_XA: vfs_printf( filewrite, "XBOW\n" );    break;
        case ACTION_FA: vfs_printf( filewrite, "FLING\n" );   break;
        case ACTION_PA: vfs_printf( filewrite, "PARRY\n" );   break;
        case ACTION_ZA: vfs_printf( filewrite, "ZAP\n" );     break;
        case ACTION_WA: vfs_printf( filewrite, "WALK\n" );    break;
        case ACTION_HA: vfs_printf( filewrite, "HIT\n" );     break;
        case ACTION_KA: vfs_printf( filewrite, "KILLED\n" );  break;
        default:        vfs_printf( filewrite, "NONE\n" );    break;
    }
}

//--------------------------------------------------------------------------------------------
void vfs_put_gender( vfs_FILE* filewrite, const char* text, Uint8 gender )
{
    /// @author ZZ
    /// @details This function kinda mimics vfs_printf for the output of
    ///    MALE FEMALE OTHER statements

    vfs_printf( filewrite, "%s", text );

    switch ( gender )
    {
        case GENDER_MALE  : vfs_printf( filewrite, "MALE\n" ); break;
        case GENDER_FEMALE: vfs_printf( filewrite, "FEMALE\n" ); break;
        default:
        case GENDER_OTHER : vfs_printf( filewrite, "OTHER\n" ); break;
    }
}

//--------------------------------------------------------------------------------------------
void vfs_put_range_raw( vfs_FILE* filewrite, FRange val )
{
    if ( val.from == val.to )
    {
        if ( val.from == FLOOR( val.from ) )
        {
            vfs_printf( filewrite, "%d", ( int )val.from );
        }
        else
        {
            vfs_printf( filewrite, "%4.2f", val.from );
        }
    }
    else
    {
        if ( val.from != FLOOR( val.from ) || val.to != FLOOR( val.to ) )
        {
            vfs_printf( filewrite, "%4.2f-%4.2f", val.from, val.to );
        }
        else
        {
            vfs_printf( filewrite, "%d-%d", ( int )val.from, ( int )val.to );
        }
    }
}

//--------------------------------------------------------------------------------------------
void vfs_put_range( vfs_FILE* filewrite, const char* text, FRange val )
{
    /// @author ZZ
    /// @details This function mimics vfs_printf in spitting out
    ///    damage/stat pairs. Try to print out the least amount of text.

    vfs_printf( filewrite, "%s", text );

    vfs_put_range_raw( filewrite, val );

    vfs_printf( filewrite, "\n" );
}

//--------------------------------------------------------------------------------------------
void vfs_put_pair( vfs_FILE* filewrite, const char* text, IPair val )
{
    /// @author ZZ
    /// @details This function mimics vfs_printf in spitting out
    ///    damage/stat pairs

    FRange loc_range;

    pair_to_range( val, &loc_range );

    vfs_printf( filewrite, "%s", text );
    vfs_printf( filewrite, "%4.2f-%4.2f\n", loc_range.from, loc_range.to );
}

//--------------------------------------------------------------------------------------------
void vfs_put_string_under( vfs_FILE* filewrite, const char* text, const char* usename )
{
    /// @author ZZ
    /// @details This function mimics vfs_printf in spitting out
    ///    a name with underscore spaces

    char cTmp;
    int cnt;

    vfs_printf( filewrite, "%s", text );
    cnt = 0;
    cTmp = usename[0];
    cnt++;
    while ( CSTR_END != cTmp )
    {
        if ( ' ' == cTmp )
        {
            vfs_printf( filewrite, "_" );
        }
        else
        {
            vfs_printf( filewrite, "%c", cTmp );
        }

        cTmp = usename[cnt];
        cnt++;
    }

    vfs_printf( filewrite, "\n" );
}

//--------------------------------------------------------------------------------------------
void vfs_put_idsz( vfs_FILE* filewrite, const char* text, IDSZ idsz )
{
    vfs_printf( filewrite, "%s", text );
    vfs_printf( filewrite, "[%s]\n", undo_idsz( idsz ) );
}

//--------------------------------------------------------------------------------------------
void vfs_put_expansion( vfs_FILE* filewrite, const char* text, IDSZ idsz, int value )
{
    /// @author ZZ
    /// @details This function mimics vfs_printf in spitting out
    ///    damage/stat pairs

    vfs_printf( filewrite, "%s: [%s] %d\n", text, undo_idsz( idsz ), value );
}

//--------------------------------------------------------------------------------------------
void vfs_put_expansion_float( vfs_FILE* filewrite, const char* text, IDSZ idsz, float value )
{
    /// @author ZF
    /// @details This function mimics vfs_printf in spitting out
    ///    damage/stat pairs for floating point values

    vfs_printf( filewrite, "%s: [%s] %4.2f\n", text, undo_idsz( idsz ), value );
}

//--------------------------------------------------------------------------------------------
void vfs_put_expansion_string( vfs_FILE* filewrite, const char* text, IDSZ idsz, const char * str )
{
    /// @author ZF
    /// @details This function mimics vfs_printf in spitting out
    ///    damage/stat pairs for floating point values

    if ( !VALID_CSTR( str ) )
    {
        vfs_put_expansion( filewrite, text, idsz, 0 );
    }
    else
    {
        vfs_printf( filewrite, "%s: [%s] %s\n", text, undo_idsz( idsz ), str );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool vfs_get_range(ReadContext& ctxt, FRange *range)
{
    // Read minimum.
    ctxt.skipWhiteSpaces();
    float from = ctxt.readReal();
    float to = from;
    // Read hyphen and maximum if present.
    ctxt.skipWhiteSpaces();
    if (ctxt.is('-'))
    {
        ctxt.next();

        // Read maximum.
        ctxt.skipWhiteSpaces();
        to = ctxt.readReal();
    }

    if (range)
    {
        range->from = std::min(from, to);
        range->to   = std::max(from, to);
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool vfs_get_next_range(ReadContext& ctxt, FRange * prange)
{
    /// @author ZZ
    /// @details This function reads a damage/stat range ( eg. 5-9 )

    ctxt.skipToColon(false);

    return vfs_get_range(ctxt, prange);
}

//--------------------------------------------------------------------------------------------
bool vfs_get_pair(ReadContext& ctxt, IPair *pair)
{
    FRange range;

    if (!vfs_get_range(ctxt, &range)) return false;

    if (pair)
    {
        // Convert the range to a pair.
        pair->base = FLOAT_TO_FP8(range.from );
        pair->rand = FLOAT_TO_FP8(range.to - range.from);
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void make_newloadname( const char *modname, const char *appendname,  char *newloadname )
{
    /// @author ZZ
    /// @details This function takes some names and puts 'em together
    int cnt, tnc;
    char ctmp;

    cnt = 0;
    ctmp = modname[cnt];
    while ( CSTR_END != ctmp )
    {
        newloadname[cnt] = ctmp;
        cnt++;
        ctmp = modname[cnt];
    }

    tnc = 0;
    ctmp = appendname[tnc];
    while ( CSTR_END != ctmp )
    {
        newloadname[cnt] = ctmp;
        cnt++;
        tnc++;
        ctmp = appendname[tnc];
    }

    newloadname[cnt] = 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Read a version number.
/// @code
/// versionNumber = '$VERSION_NUMBER' natural
/// @endcode
int vfs_get_version(ReadContext& ctxt)
{
    if (ctxt.is(ReadContext::StartOfInput))
    {
        ctxt.next();
    }
    ctxt.skipWhiteSpaces();
    if (!ctxt.is('$'))
    {
        return -1;
    }
    ctxt.next();
    std::string tag = ctxt.readName();
    if (tag != "FILE_VERSION")
    {
        return -1;
    }
    
    // Get the version number.
    return ctxt.readInt();
    
}

//--------------------------------------------------------------------------------------------
bool vfs_put_version( vfs_FILE* filewrite, const int version )
{
    if ( vfs_error( filewrite ) ) return false;

    return 0 != vfs_printf( filewrite, "$FILE_VERSION %i\n\n", version );
}

//--------------------------------------------------------------------------------------------
char * copy_to_delimiter_mem( char * pmem, char * pmem_end, vfs_FILE * filewrite, int delim, char * user_buffer, size_t user_buffer_len )
{
    /// @author BB
    /// @details copy data from one file to another until the delimiter delim has been found
    ///    could be used to merge a template file with data

    size_t write;
    char cTmp, temp_buffer[1024] = EMPTY_CSTR;

    if ( NULL == pmem || NULL == filewrite ) return pmem;

    if ( vfs_error( filewrite ) ) return pmem;

    write = 0;
    temp_buffer[0] = CSTR_END;
    cTmp = *( pmem++ );
    while ( pmem < pmem_end )
    {
        if ( delim == cTmp ) break;

        if ( ASCII_LINEFEED_CHAR ==  cTmp || C_CARRIAGE_RETURN_CHAR ==  cTmp )
        {
            // output the temp_buffer
            temp_buffer[write] = CSTR_END;
            vfs_puts( temp_buffer, filewrite );
            vfs_putc( cTmp, filewrite );

            // reset the temp_buffer pointer
            write = 0;
            temp_buffer[0] = CSTR_END;
        }
        else
        {
            if ( write > SDL_arraysize( temp_buffer ) - 2 )
            {
                log_error( "copy_to_delimiter_mem() - temp_buffer overflow.\n" );
            }

            temp_buffer[write++] = cTmp;
        }

        // only copy if it is not the
        cTmp = *( pmem++ );
    }
    temp_buffer[write] = CSTR_END;

    if ( NULL != user_buffer )
    {
        strncpy( user_buffer, temp_buffer, user_buffer_len - 1 );
        user_buffer[user_buffer_len - 1] = CSTR_END;
    }

    if ( delim == cTmp )
    {
        pmem--;
    }

    return pmem;
}

//--------------------------------------------------------------------------------------------
char vfs_get_next_printable(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return ctxt.readPrintable();
}

//--------------------------------------------------------------------------------------------
void ReadContext::skipNewLine()
{
    if (isNewLine())
    {
        int old = _current;
        next();
        if (isNewLine() && old != _current)
        {
            next();
        }
        _lineNumber++;
    }
}
void ReadContext::skipNewLines()
{
    while (isNewLine())
    {
        int old = _current;
        next();
        if (isNewLine() && old != _current)
        {
            next();
        }
        _lineNumber++;
    }
}
std::string ReadContext::readToEndOfLine()
{
    if (is(StartOfInput))
    {
        next();
    }
    skipWhiteSpaces();
    _buffer.clear();
    while (!is(EndOfInput))
    {
        if (is(Error))
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
        }
        if (isNewLine())
        {
            skipNewLine();
            break;
        }
        saveAndNext();
    }
    return _buffer.toString();
}

std::string ReadContext::readSingleLineComment()
{
    if (is(StartOfInput))
    {
        next();
    }
    skipWhiteSpaces();
    _buffer.clear();
    if (!is('/'))
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    next();
    if (!is('/'))
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    next();
    skipWhiteSpaces();
    while (!is(EndOfInput))
    {
        if (is(Error))
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
        }
        if (isNewLine())
        {
            skipNewLine();
            break;
        }
        saveAndNext();
    }
    return _buffer.toString();
}

char ReadContext::readPrintable()
{
    if (is(StartOfInput))
    {
        next();
    }
    skipWhiteSpaces();
    if (is(EndOfInput) || is(Error))
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    if (!isAlpha() && !isDigit() && !is('!') && !is('?') && !is('='))
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    char tmp = static_cast<char>(_current);
    next();
    return tmp;
}

char ReadContext::readCharLit()
{
    if (is(StartOfInput))
    {
        next();
    }
    skipWhiteSpaces();
    if (is(EndOfInput) || is(Error))
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    char chr;
    if (!is('\''))
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    next();
    if (is('\\'))
    {
        next();
        if (is('\''))
        {
            chr = '\'';
        }
        else if (is('n'))
        {
            chr = '\n';
        }
        else if (is('t'))
        {
            chr = '\t';
        }
        else if (is('\\'))
        {
            chr = '\\';
        }
        else
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
        }
        next();
    }
    else
    {
        if (is(EndOfInput) || is(Error))
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
        }
        chr = static_cast<char>(_current);
        next();
    }
    if (!is('\''))
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    next();
    return chr;
}
int ReadContext::readInt()
{
    skipWhiteSpaces();
    _buffer.clear();
    if (is(StartOfInput))
    {
        next();
    }
    if (is('+') || is('-'))
    {
        saveAndNext();
    }
    if (!isDigit())
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    do
    {
        saveAndNext();
    } while (isDigit());
    if (is('e') || is('E'))
    {
        saveAndNext();
        if (is('+'))
        {
            saveAndNext();
        }
        if (!isDigit())
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
        }
        do
        {
            saveAndNext();
        } while (isDigit());
    }
    return toInteger();
}

unsigned int ReadContext::readNat()
{
    skipWhiteSpaces();
    _buffer.clear();
    if (is(StartOfInput))
    {
        next();
    }
    if (is('+'))
    {
        saveAndNext();
    }
    if (!isDigit())
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    do
    {
        saveAndNext();
    } while (isDigit());
    if (is('e') || is('E'))
    {
        saveAndNext();
        if (is('+'))
        {
            saveAndNext();
        }
        if (!isDigit())
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
        }
        do
        {
            saveAndNext();
        } while (isDigit());
    }
    return toNatural();
}

//--------------------------------------------------------------------------------------------
UFP8_T vfs_get_ufp8(ReadContext& ctxt)
{
    float x = ctxt.readReal();
    if (x < 0.0f)
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(ctxt._loadName,ctxt._lineNumber));
    }
    return FLOAT_TO_FP8(x);
}

//--------------------------------------------------------------------------------------------
SFP8_T vfs_get_sfp8(ReadContext& ctxt)
{
    float x = ctxt.readReal();
    return FLOAT_TO_FP8(x);
}

//--------------------------------------------------------------------------------------------
int vfs_get_next_int(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return ctxt.readInt();
}

unsigned int vfs_get_next_nat(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return ctxt.readNat();
}

UFP8_T vfs_get_next_ufp8(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return vfs_get_ufp8(ctxt);
}

SFP8_T vfs_get_next_sfp8(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return vfs_get_sfp8(ctxt);
}

//--------------------------------------------------------------------------------------------
void vfs_read_string(ReadContext& ctxt, char *str, size_t max)
{
    ctxt.skipWhiteSpaces();
    ctxt._buffer.clear();
    if (!max)
    {
        str[0] = '\0';
        return;
    }
    else
    {
        size_t cur = 0;
        while (cur < max && !ctxt.isNewLine() && !ctxt.isWhiteSpace() && !ctxt.is(ReadContext::EndOfInput) && !ctxt.is(ReadContext::Error))
        {
            ctxt.saveAndNext();
            cur++;
        }
        if (ctxt.is(ReadContext::Error))
        {
            throw Ego::Script::LexicalError(__FILE__,__LINE__,Ego::Script::Location(ctxt._loadName,ctxt._lineNumber));
        }
        EGOBOO_ASSERT(ctxt._buffer.getSize() == cur && cur <= max);
        strcpy(str, ctxt._buffer.toString().c_str());
        return;
    }
}

void vfs_get_next_string_lit(ReadContext& ctxt, char *str, size_t max)
{
    ctxt.skipToColon(false);
    vfs_read_string_lit(ctxt, str, max);
}

//--------------------------------------------------------------------------------------------

void ReadContext::write(char chr)
{
    EGOBOO_ASSERT('\0' != chr);
    _buffer.append(chr);
}
void ReadContext::save()
{
    EGOBOO_ASSERT(0x00 <= _current && _current <= 0xff);
    write(static_cast<char>(_current)); 
}
void ReadContext::next()
{
    _current = readInput();
}
void ReadContext::writeAndNext(char chr)
{
    write(chr); next();
}
void ReadContext::saveAndNext()
{
    save(); next();
}
float ReadContext::readReal()
{
    skipWhiteSpaces();
    _buffer.clear();
    if (is(StartOfInput))
    {
        next();
    }
    if (is('+') || is('-'))
    {
        saveAndNext();
    }
    if (is('.'))
    {
        saveAndNext();
        if (!isDigit())
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
        }
        do
        {
            saveAndNext();
        } while (isDigit());
    }
    else if (isDigit())
    {
        do
        {
            saveAndNext();
        } while (isDigit());
        if (is('.'))
        {
            saveAndNext();
            while (isDigit())
            {
                saveAndNext();
            }
        }
    }
    if (is('e') || is('E'))
    {
        saveAndNext();
        if (is('+') || is('-'))
        {
            saveAndNext();
        }
        if (!isDigit())
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
        }
        do
        {
            saveAndNext();
        } while (isDigit());
    }
    return toReal();
}

//--------------------------------------------------------------------------------------------
float vfs_get_next_float(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return ctxt.readReal();
}

//--------------------------------------------------------------------------------------------
void vfs_get_next_name(ReadContext& ctxt, char *buffer, size_t max)
{
    ctxt.skipToColon(false);
    vfs_read_name(ctxt, buffer, max);
}

//--------------------------------------------------------------------------------------------
bool vfs_get_next_pair(ReadContext& ctxt, IPair *pair)
{
    ctxt.skipToColon(false);
    return vfs_get_pair(ctxt, pair);
}

//--------------------------------------------------------------------------------------------
IDSZ vfs_get_next_idsz(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return ctxt.readIDSZ();
}

//--------------------------------------------------------------------------------------------

DamageType vfs_get_damage_type(ReadContext& ctxt)
{
    EnumReader<DamageType> rdr
    (
        "DamageType",
        {
            // Slash.
            { "Slash", DAMAGE_SLASH },
            { "SLASH", DAMAGE_SLASH },
            { "S",     DAMAGE_SLASH },
            // Crush.
            { "Crush", DAMAGE_CRUSH },
            { "CRUSH", DAMAGE_CRUSH },
            { "C",     DAMAGE_CRUSH },
            // Poke.
            { "Poke",  DAMAGE_POKE  },
            { "POKE",  DAMAGE_POKE  },
            { "P",     DAMAGE_POKE  },
            // Holy.
            { "Holy",  DAMAGE_HOLY  },
            { "HOLY",  DAMAGE_HOLY  },
            { "H",     DAMAGE_HOLY  },
            // Evil.
            { "Evil",  DAMAGE_EVIL  },
            { "EVIL",  DAMAGE_EVIL  },
            { "E",     DAMAGE_EVIL  },
            // Fire.
            { "Fire",  DAMAGE_FIRE  },
            { "FIRE",  DAMAGE_FIRE  },
            { "F",     DAMAGE_FIRE  },
            // Ice.
            { "Ice",   DAMAGE_ICE   },
            { "ICE",   DAMAGE_ICE   },
            { "I",     DAMAGE_ICE   },
            // Zap.
            { "Zap",   DAMAGE_ZAP   },
            { "ZAP",   DAMAGE_ZAP   },
            { "Z",     DAMAGE_ZAP   },
            // None.
            { "None",  DAMAGE_NONE  },
            { "NONE",  DAMAGE_NONE  },
            { "N",     DAMAGE_NONE  },
        }
    );
    return ReadContext::readEnum(ctxt,rdr,DAMAGE_NONE);
}

//--------------------------------------------------------------------------------------------
DamageType vfs_get_next_damage_type(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return vfs_get_damage_type(ctxt);
}

//--------------------------------------------------------------------------------------------
std::string ReadContext::readStringLit()
{
    if (is(StartOfInput))
    {
        next();
    }
    skipWhiteSpaces();
    _buffer.clear();
    while (true)
    {
        if (is(Error))
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
        }
        else if (is('~'))
        {
            writeAndNext('\t');
        }
        else if (is('_'))
        {
            writeAndNext(' ');
        }
        else if (isNewLine() || isWhiteSpace() || is(EndOfInput))
        {
            break;
        }
        else
        {
            saveAndNext();
        }
    }
    return toString();
}

void ReadContext::readName0()
{
    if (!isAlpha() && !is('_'))
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    do
    {
        saveAndNext();
    } while (isAlpha() || isDigit() || is('_'));
}

std::string ReadContext::readName()
{
    if (is(StartOfInput))
    {
        next();
    }
    skipWhiteSpaces();
    _buffer.clear();
    readName0();
    return toString();
}

void ReadContext::readReference0()
{
    if (!is('%'))
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
    saveAndNext();
    readName0();
}

std::string ReadContext::readReference()
{
    if (is(StartOfInput))
    {
        next();
    }
    skipWhiteSpaces();
    _buffer.clear();
    readReference0();
    return toString();
}

bool ReadContext::readBool()
{
    std::string name = readName();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name == "true" || name == "t")
    {
        return true;
    }
    else if (name == "false" || name == "f")
    {
        return false;
    }
    else
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(_loadName, _lineNumber));
    }
}

//--------------------------------------------------------------------------------------------
bool vfs_get_next_bool(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    ctxt.skipWhiteSpaces();
    return ctxt.readBool();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void GLSetup_SupportedFormats()
{
    /// @author ZF
    /// @details This need only to be once

    Uint8 type = 0;

    // define extra supported file types with SDL_image
    // these should probably be ordered so that the types that
    // support transparency are first
    if ( cfg.sdl_image_allowed )
    {
        snprintf( TxFormatSupported[type], SDL_arraysize( TxFormatSupported[type] ), ".png" ); type++;
        snprintf( TxFormatSupported[type], SDL_arraysize( TxFormatSupported[type] ), ".tif" ); type++;
        snprintf( TxFormatSupported[type], SDL_arraysize( TxFormatSupported[type] ), ".tiff" ); type++;
        snprintf( TxFormatSupported[type], SDL_arraysize( TxFormatSupported[type] ), ".gif" ); type++;
        snprintf( TxFormatSupported[type], SDL_arraysize( TxFormatSupported[type] ), ".pcx" ); type++;
        snprintf( TxFormatSupported[type], SDL_arraysize( TxFormatSupported[type] ), ".ppm" ); type++;
        snprintf( TxFormatSupported[type], SDL_arraysize( TxFormatSupported[type] ), ".jpg" ); type++;
        snprintf( TxFormatSupported[type], SDL_arraysize( TxFormatSupported[type] ), ".jpeg" ); type++;
        snprintf( TxFormatSupported[type], SDL_arraysize( TxFormatSupported[type] ), ".xpm" ); type++;
        snprintf( TxFormatSupported[type], SDL_arraysize( TxFormatSupported[type] ), ".pnm" ); type++;
        snprintf( TxFormatSupported[type], SDL_arraysize( TxFormatSupported[type] ), ".lbm" ); type++;
        snprintf( TxFormatSupported[type], SDL_arraysize( TxFormatSupported[type] ), ".tga" ); type++;
    }

    // These typed are natively supported with SDL
    // Place them *after* the SDL_image types, so that if both are present,
    // the other types will be preferred over bmp
    snprintf( TxFormatSupported[type], SDL_arraysize( TxFormatSupported[type] ), ".bmp" ); type++;
    snprintf( TxFormatSupported[type], SDL_arraysize( TxFormatSupported[type] ), ".BMP" ); type++;

    // Save the amount of format types we have in store
    maxformattypes = type;
    if ( !cfg.sdl_image_allowed )
    {
        log_message( "Failed!\n" );
        log_info( "[SDL_IMAGE] set to \"FALSE\" in setup.txt, only support for .bmp files\n" );
    }
    else
    {
        log_message( "Success!\n" );
    }
}

//--------------------------------------------------------------------------------------------
Uint32  ego_texture_load_vfs( oglx_texture_t *texture, const char *filename, Uint32 key )
{
    STRING fullname;
    Uint32 retval;
    Uint8 type = 0;
    SDL_Surface * image = NULL;
    //GLenum tx_target;

    // get rid of any old data
    oglx_texture_release( texture );

    // load the image
    retval = INVALID_GL_ID;
    if ( cfg.sdl_image_allowed )
    {
        // try all different formats
        for ( type = 0; type < maxformattypes; type++ )
        {
            snprintf( fullname, SDL_arraysize( fullname ), "%s%s", filename, TxFormatSupported[type] );
            retval = oglx_texture_load( texture, fullname, key );
            if ( INVALID_GL_ID != retval ) break;
        }
    }
    else
    {
        image = NULL;

        // normal SDL only supports bmp
        snprintf( fullname, SDL_arraysize( fullname ), "%s.bmp", filename );
        image = SDL_LoadBMP_RW(vfs_openRWopsRead(fullname), 1);

        // We could not load the image
        if ( NULL == image ) return INVALID_GL_ID;

        //tx_target = GL_TEXTURE_2D;
        //if ( image->w != image->h && ( 1 == image->w || image->h ) )
        //{
        //    tx_target = GL_TEXTURE_1D;
        //}

        retval = oglx_texture_convert( texture, image, key );
        strncpy( texture->name, fullname, SDL_arraysize( texture->name ) );

        texture->base.wrap_s = GL_REPEAT;
        texture->base.wrap_t = GL_REPEAT;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
DamageModifier vfs_get_damage_modifier(ReadContext& ctxt)
{
    EnumReader<DamageModifier> rdr
        (
        "damageModifier",
        {
            { "T", DamageModifier::DAMAGEINVERT },
            { "C", DamageModifier::DAMAGECHARGE },
            { "M", DamageModifier::DAMAGEMANA },
            { "I", DamageModifier::DAMAGEINVICTUS },
            { "F", DamageModifier::NONE },
        }
    );
    return ReadContext::readEnum(ctxt, rdr, DamageModifier::NONE);
}

//--------------------------------------------------------------------------------------------
float vfs_get_damage_resist(ReadContext& ctxt)
{
    /// @todo Ugly hack to allow it to work with the old damage system assume that numbers below 4 are shifts.
    float resistance = ctxt.readReal();
    if (resistance == 1) resistance = 0.50f;        //50% reduction, same as shift 1
    else if (resistance == 2) resistance = 0.75f;   //75% reduction, same as shift 2
    else if (resistance == 3) resistance = 0.90f;   //90% reduction, same as shift 3
    else resistance = resistance / 100.0f;

    return resistance;
}

//--------------------------------------------------------------------------------------------
int read_skin_vfs( const char *filename )
{
    /// @author ZZ
    /// @details This function reads the skin.txt file...
    ReadContext ctxt(filename);
    if (!ctxt.ensureOpen())
    {
        return NO_SKIN_OVERRIDE;
    }
    // Read the contents.
    int skin = vfs_get_next_int(ctxt);
    if (skin < 0 || skin > MAX_SKIN)
    {
        /** @todo Use context to produce a nice warning message. */
    }
    skin %= MAX_SKIN;
    return skin;
}
