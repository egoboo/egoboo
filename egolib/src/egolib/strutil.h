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

/// @file  egolib/strutil.h
/// @brief String manipulation functions.

#pragma once

#include "egolib/typedef.h"

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

/// end-of-string character. assume standard null terminated string
#   define CSTR_END '\0'
#   define EMPTY_CSTR { CSTR_END }

#   define VALID_CSTR(PSTR)   ((NULL!=PSTR) && (CSTR_END != PSTR[0]))
#   define INVALID_CSTR(PSTR) ((NULL==PSTR) || (CSTR_END == PSTR[0]))

/**
 * @brief
 *  Convert a character to upper case using the current locale.
 * @param chr
 *  the character
 * @return
 *  the upper case character
 */
template <class CharType>
inline CharType char_toupper(CharType chr) { return std::toupper(chr, std::locale()); }

/**
* @brief
*  Convert a character to lower case using the current locale.
* @param chr
*  the character
* @return
*  the lower case character
*/
template <class CharType>
inline CharType char_tolower(CharType chr) { return std::tolower(chr, std::locale()); }

/**
 * @brief
 *  Get if a character is a whitespace character using the current locale.
 * @param chr
 *  the character
 * @return
 *  @a true if the character is a whitespace character, @a false otherwise
 */
template <class CharType>
inline bool char_isspace(CharType chr) { return std::isspace(chr, std::locale()); }

/**
 * @brief
 *  Trim leading and trailing spaces.
 * @param str
 *  the string
 * @return
 *  a string equal to @a str but with leading and trailing spaces removed
 */
inline std::string trim(const std::string& str)
{
    auto front = std::find_if_not(str.begin(), str.end(), std::isspace);
    return std::string(front, std::find_if_not(str.rbegin(), std::string::const_reverse_iterator(front), std::isspace).base());
}

// libc++ doesn't define std::toupper<int>(int, std::locale),
// so narrow the int to a char.
#ifdef _LIBCPP_VERSION
template <>
inline int char_toupper<int>(int chr)
{
    EGOBOO_ASSERT(chr >= 0 && chr <= 255);
    return char_toupper(static_cast<char>(chr));
}
template <>
inline int char_tolower<int>(int chr)
{
    EGOBOO_ASSERT(chr >= 0 && chr <= 255);
    return char_tolower(static_cast<char>(chr));
}
template <>
inline bool char_isspace<int>(int chr)
{
    EGOBOO_ASSERT(chr >= 0 && chr <= 255);
    return char_isspace(static_cast<char>(chr));
}
#endif

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

    char * str_decode( char *strout, size_t insize, const char * strin );
    char * str_encode( char *strout, size_t insize, const char * strin );
    char * str_encode_path( const char *character );

    char * str_clean_path( char * str, size_t size );
    char * str_convert_slash_net( char * str, size_t size );
    char * str_convert_slash_sys( char * str, size_t size );

    char * str_append_slash( char * str, size_t size );
    char * str_append_slash_net( char * str, size_t size );

    void   str_trim( char *pStr );
    void   str_add_linebreaks( char * text, size_t text_len, size_t line_len );

#if defined(__GNUC__) && !(defined (__MINGW) || defined(__MINGW32__))
    char* strupr( char * str );
    char* strlwr( char * str );
#endif
