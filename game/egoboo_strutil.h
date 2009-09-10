#pragma once

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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - egoboo_strutil.h
 * String manipulation functions.  Not currently in use.
 */

#include "egoboo_typedef.h"
#include <string.h>
#include <ctype.h>

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

// end-of-string character. assume standard null terminated string
#define EOS '\0'
#define NULL_STRING { EOS }

#define EMPTY_CSTR(PSTR) ((NULL!=PSTR) && (EOS == PSTR[0]))
#define VALID_CSTR(PSTR) ((NULL!=PSTR) && (EOS != PSTR[0]))
#define INVALID_CSTR(PSTR) ((NULL==PSTR) || (EOS == PSTR[0]))

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

char * str_decode( char *strout, size_t insize, const char * strin );
char * str_encode( char *strout, size_t insize, const char * strin );
char * str_encode_path( const char *character );

char * str_clean_path(char * str, size_t size);
char * str_convert_slash_net( char * str, size_t size );
char * str_convert_slash_sys( char * str, size_t size );

char * str_append_slash(char * str, size_t size );
char * str_append_slash_net(char * str, size_t size );

void   str_trim( char *pStr );
void   str_add_linebreaks( char * text, size_t text_len, size_t line_len );

#define _EGOBOOSTRUTIL_H_
