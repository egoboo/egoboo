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

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

    char * str_decode( char *strout, size_t insize, const char * strin );
    char * str_encode( char *strout, size_t insize, const char * strin );
	/**
	 * @brief
	 *	Turn an entity name into a pathname for loading and saving files.
	 * @param objectName
	 *	the object name
	 * @return
	 *	the pathname
	 * @remark
	 *	Replaces space by underscore,
	 *	replaces uppercase alphabetic character by the corresponding lowercase alphabetic character, and
	 *	appends <tt>".obj"</tt> to the resulting string.  
	 */
    std::string str_encode_path(const std::string& objectName);

    char * str_clean_path( char * str, size_t size );
    char * str_convert_slash_net( char * str, size_t size );
    char * str_convert_slash_sys( char * str, size_t size );

	/// Append a operating system slash to a filename
	/// if there is no slash or backslash.
    std::string str_append_slash(const std::string& filename);
	/// Append a network slash to a filename
	/// if there is no slash or backslash.
    std::string str_append_slash_net(const std::string& filename);

    void   str_trim( char *pStr );
    void   str_add_linebreaks( char * text, size_t text_len, size_t line_len );

#if defined(__GNUC__) && !(defined (__MINGW) || defined(__MINGW32__))
    char* strupr( char * str );
    char* strlwr( char * str );
#endif
