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

/// @file egolib/strutil.c
/// @brief String manipulation functions.
/// @details

#include "egolib/strutil.h"
#include "egolib/fileutil.h" /**<< @tood Remove this include. */
#include "egolib/Core/StringUtilities.hpp"
#include "egolib/platform.h" /**<< @todo Remove this include. */

//--------------------------------------------------------------------------------------------
std::string str_decode(const std::string& source) {
    static const auto transcode = [](char source) {
        switch (source) {
            case '_': return ' ';
            case '~': return '\t';
            default: return source;
        }
    };
    auto temporary = source;
    std::transform(temporary.begin(), temporary.end(), temporary.begin(), transcode);
    return temporary;
}

std::string str_encode(const std::string& source) {
    static const auto transcode = [](char source) {
        switch (source) {
            case ' ': return '_';
            case '\t': return '~';
            default: return source;
        };
    };
    auto temporary = source;
    std::transform(temporary.begin(), temporary.end(), temporary.begin(), transcode);
    return temporary;
}

char * str_decode( char *strout, size_t insize, const char * strin )
{
    /// @author BB
    /// @details str_decode converts a string from "storage mode" to an actual string

    char *pin = ( char * )strin, *pout = strout, *plast = pout + insize;

    if ( NULL == strin || NULL == strout || 0 == insize ) return NULL;
    while ( pout < plast && CSTR_END != *pin )
    {
        *pout = *pin;
        if ( '_' == *pout ) *pout = ' ';
        else if ( '~' == *pout ) *pout = C_TAB_CHAR;
        pout++;
        pin++;
    };

    if ( pout < plast ) *pout = CSTR_END;

    return strout;
}

//--------------------------------------------------------------------------------------------
char * str_encode( char *strout, size_t insize, const char * strin )
{
    /// @author BB
    /// @details str_encode converts an actual string to "storage mode"

    char chrlast = 0;
    char *pin = ( char * )strin, *pout = strout, *plast = pout + insize;

    if ( NULL == strin || NULL == strout || 0 == insize ) return NULL;
    while ( pout < plast && CSTR_END != *pin )
    {
        if (!Ego::isspace(*pin) && Ego::isprint(*pin))
        {
            chrlast = *pout = Ego::tolower(*pin);
            pin++;
            pout++;
        }
        else if (' ' == *pin)
        {
            chrlast = *pout = '_';
            pin++;
            pout++;
        }
        else if (C_TAB_CHAR == *pin)
        {
            chrlast = *pout = '~';
            pin++;
            pout++;
        }
        else if (Ego::isspace(*pin))
        {
            chrlast = *pout = '_';
            pin++;
            pout++;
        }
        else if ( '_' != chrlast )
        {
            chrlast = *pout = '_';
            pin++;
            pout++;
        }
        else
        {
            pin++;
        }
    };

    if ( pout < plast ) *pout = CSTR_END;

    return strout;
}

//--------------------------------------------------------------------------------------------
char * str_clean_path( char * str, size_t size )
{
    /// @author BB
    /// @details remove any accidentally doubled slash characters from the stream

    char *psrc, *psrc_end, *pdst, *pdst_end;

    if ( INVALID_CSTR( str ) ) return str;

    for ( psrc = str, pdst = str, psrc_end = str + size, pdst_end = str + size;
          psrc < psrc_end && pdst < pdst_end;
          /*nothing*/ )
    {
        if ( C_SLASH_CHR == *psrc || C_BACKSLASH_CHR == *psrc )
        {
            *pdst = *psrc;
            psrc++;
            pdst++;

            while ( psrc < psrc_end && ( C_SLASH_CHR == *psrc || C_BACKSLASH_CHR == *psrc ) )
            {
                psrc++;
            }
        }
        else
        {
            *pdst = *psrc;
            psrc++;
            pdst++;
        }
    }

    if ( pdst < pdst_end )
    {
        *pdst = CSTR_END;
    }

    return str;
}

//--------------------------------------------------------------------------------------------
char * str_convert_slash_net( char * str, size_t size )
{
    /// @author BB
    /// @details converts the slashes in a string to those appropriate for the Net

    char * psrc, *psrc_end, *pdst, *pdst_end;

    if ( INVALID_CSTR( str ) ) return str;

    for ( psrc = str, pdst = str, psrc_end = str + size, pdst_end = str + size;
          CSTR_END != *psrc && psrc < psrc_end && pdst < pdst_end;
          psrc++, pdst++ )
    {
        char cTmp = *psrc;

        if ( C_SLASH_CHR == cTmp || C_BACKSLASH_CHR == cTmp ) cTmp = NET_SLASH_CHR;

        *pdst = cTmp;
    }

    if ( pdst < pdst_end )
    {
        *pdst = CSTR_END;
    }

    return str_clean_path( str, size );
}

//--------------------------------------------------------------------------------------------
char * str_convert_slash_sys( char * str, size_t size )
{
    /// @author BB
    /// @details converts the slashes in a string to those appropriate this system

    char * psrc, *psrc_end, *pdst, *pdst_end;

    if ( INVALID_CSTR( str ) ) return str;

    for ( psrc = str, pdst = str, psrc_end = str + size, pdst_end = str + size;
          CSTR_END != *psrc && psrc < psrc_end && pdst < pdst_end;
          psrc++, pdst++ )
    {
        char cTmp = *psrc;

        if ( C_SLASH_CHR == cTmp || C_BACKSLASH_CHR == cTmp ) cTmp = SLASH_CHR;

        *pdst = cTmp;
    }

    if ( pdst < pdst_end )
    {
        *pdst = CSTR_END;
    }

    return str_clean_path( str, size );
}

//--------------------------------------------------------------------------------------------
std::string str_append_slash_net(const std::string& filename) {
	size_t l = filename.length();
	if (!l) {
		return NET_SLASH_STR;
	} else if (C_SLASH_CHR != filename[l - 1] && C_BACKSLASH_CHR != filename[l - 1]) {
		return filename + NET_SLASH_STR;
	} else {
		return filename;
	}
}

//--------------------------------------------------------------------------------------------
std::string str_append_slash( const std::string& filename ) {
    size_t l = filename.length();
	if (!l) {
		return SLASH_STR;
	} else if (C_SLASH_CHR != filename[l-1] && C_BACKSLASH_CHR != filename[l-1]) {
		return filename + SLASH_STR;
	} else {
		return filename;
	}
}

//--------------------------------------------------------------------------------------------

std::string str_encode_path( const std::string& objectName)
{
	std::string encodedName = objectName + ".obj";
	// alphabetic uppercase -> alphabetic lowercase
	// space -> underscore
	auto f = [](const char chr) -> char {
		if (std::isalpha(chr)) {
			return std::tolower(chr);
		} else if (std::isspace(chr)) {
			return '_';
		} else {
			return chr;
		}};
	transform(encodedName.begin(), encodedName.end(), encodedName.begin(), f);
	return encodedName;
}

//--------------------------------------------------------------------------------------------

std::string add_linebreak_cpp(const std::string& text, size_t lineLength) {
    if (0 == text.length() || 0 == lineLength) return text;

    struct line_t {
        line_t(size_t start, size_t end) : start(start), end(end) {}
        size_t length() const {
            return end - start;
        }
        size_t start, end;
    };
    std::string newText = text;
    line_t line(0, 0);
    // Memoize the last whitespace in this line.
    size_t last_ws = std::string::npos;
    while (newText[line.end] != '\0') {
        if (newText[line.end] == ' ') {
            // If a whitespace is encountered, memoize that whitespace.
            last_ws = line.end;
            // Expand line.
            line.end++;
        } else if (newText[line.end] == '\n') {
            // Begin a new line.
            line.start = line.end + 1;
            line.end = line.start;
            // No whitespace in that new line (so far).
            last_ws = std::string::npos;
        } else {
            // Expand line.
            line.end++;
        }
        // If the sub-length exceeds the limit
        if (line.length() > lineLength) {
            // If a whitespace exists in that line ...
            if (last_ws != std::string::npos) {
                // If a whitespace character in this line was found replace it by a newline character.
                // This effectively starts a new line.
                newText[last_ws] = '\n';
                line.start = last_ws + 1;
            } else {
                // Otherwise there is nothing we can do: advance to end of string, whitespace, or end of line.
                while (newText[line.end] != '\n' && newText[line.end] != ' ' && newText[line.end] != '\0') line.end++;
            }
        }
    }
    return newText;
}

void str_add_linebreaks( char * text, size_t text_len, size_t line_len )
{
    char * text_end, * text_break, * text_stt;

    if ( INVALID_CSTR( text ) || 0 == text_len  || 0 == line_len ) return;

    text_end = text + text_len;
    text_break = text_stt = text;
    while ( text < text_end && CSTR_END != *text )
    {
        // scan for the next whitespace
        text = strpbrk( text, " \n" );

        if ( text >= text_end )
        {
            break;
        }
        else if ( NULL == text )
        {
            // reached the end of the string
            break;
        }
        else if ( C_LINEFEED_CHAR == *text )
        {
            // respect existing line breaks
            text_break = text;
            text++;
            continue;
        }

        // until the line is too long, then insert
        // replace the last good ' ' with C_NEW_LINE_CHAR
        if ((( size_t )text - ( size_t )text_break ) > line_len )
        {
            if ( ' ' != *text_break )
            {
                text_break = text;
            }

            // convert the character
            *text_break = C_LINEFEED_CHAR;

            // start over again
            text = text_break;
        }

        text++;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// gcc does not define these functions on linux (at least not Ubuntu),
// but it is defined under MinGW, which is yucky.
// I actually had to spend like 45 minutes looking up the compiler flags
// to catch this... good documentation, guys!
#if defined(__GNUC__) && !(defined (__MINGW) || defined(__MINGW32__))
char* strupr( char * str )
{
    char *ret = str;
    if ( NULL != str )
    {
        while ( CSTR_END != *str )
        {
            *str = Ego::toupper( *str );
            str++;
        }
    }
    return ret;
}

char* strlwr( char * str )
{
    char *ret = str;
    if ( NULL != str )
    {
        while ( CSTR_END != *str )
        {
            *str = Ego::tolower( *str );
            str++;
        }
    }
    return ret;
}

#endif
