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

/// @file egoboo_strutil.c
/// @brief String manipulation functions.
/// @details

#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void str_trim( char *pStr )
{
    /// @details ZZ@> str_trim remove all space and tabs in the beginning and at the end of the string

    Sint32 DebPos = 0, EndPos = 0, CurPos = 0;

    if ( INVALID_CSTR( pStr ) )
    {
        return;
    }

    // look for the first character in string
    DebPos = 0;
    while ( isspace( pStr[DebPos] ) && CSTR_END != pStr[DebPos] )
    {
        DebPos++;
    }

    // look for the last character in string
    CurPos = DebPos;
    while ( pStr[CurPos] != 0 )
    {
        if ( !isspace( pStr[CurPos] ) )
        {
            EndPos = CurPos;
        }
        CurPos++;
    }

    if ( DebPos != 0 )
    {
        // shift string left
        for ( CurPos = 0; CurPos <= ( EndPos - DebPos ); CurPos++ )
        {
            pStr[CurPos] = pStr[CurPos + DebPos];
        }
        pStr[CurPos] = CSTR_END;
    }
    else
    {
        pStr[EndPos + 1] = CSTR_END;
    }
}

//--------------------------------------------------------------------------------------------
char * str_decode( char *strout, size_t insize, const char * strin )
{
    /// @details BB@> str_decode converts a string from "storage mode" to an actual string

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
    /// @details BB@> str_encode converts an actual string to "storage mode"

    char chrlast = 0;
    char *pin = ( char * )strin, *pout = strout, *plast = pout + insize;

    if ( NULL == strin || NULL == strout || 0 == insize ) return NULL;
    while ( pout < plast && CSTR_END != *pin )
    {
        if ( !isspace( *pin ) && isprint( *pin ) )
        {
            chrlast = *pout = tolower( *pin );
            pin++;
            pout++;
        }
        else if ( ' ' == *pin )
        {
            chrlast = *pout = '_';
            pin++;
            pout++;
        }
        else if ( C_TAB_CHAR == *pin )
        {
            chrlast = *pout = '~';
            pin++;
            pout++;
        }
        else if ( isspace( *pin ) )
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
    /// @details BB@> remove any accidentally doubled slash characters from the stream

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
    /// @details BB@> converts the slashes in a string to those appropriate for the Net

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
    /// @details BB@> converts the slashes in a string to those appropriate this system

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
char * str_append_slash_net( char * str, size_t size )
{
    /// @details BB@> appends a network-type slash to a string, if it does not already have one

    size_t len;

    if ( INVALID_CSTR( str ) ) return str;

    len = strlen( str );
    if ( C_SLASH_CHR != str[len-1] && C_BACKSLASH_CHR != str[len-1] )
    {
        strncat( str, NET_SLASH_STR, size );
    }

    return str;
}

//--------------------------------------------------------------------------------------------
char * str_append_slash( char * str, size_t size )
{
    /// @details BB@> appends this system's slash to a string, if it does not already have one

    size_t len;

    if ( INVALID_CSTR( str ) ) return NULL;

    len = strlen( str );
    if ( C_SLASH_CHR != str[len-1] && C_BACKSLASH_CHR != str[len-1] )
    {
        strncat( str, SLASH_STR, size );
    }

    return str;
}

//--------------------------------------------------------------------------------------------
char * str_encode_path( const char *szName )
{
    /// @details ZF@> This turns a szName name into a proper filepath for loading and saving files
    ///   also turns all letter to lower case in case of case sensitive OS.

    static STRING szPathname;

    char * pname, * pname_end;
    char * ppath, * ppath_end;
    char letter;

    pname     = ( char * )szName;
    pname_end = pname + 255;

    ppath     = szPathname;
    ppath_end = szPathname + SDL_arraysize( szPathname ) - 5;
    while ( CSTR_END != *pname && pname < pname_end && ppath < ppath_end )
    {
        letter = tolower( *pname );

        if ( isspace( letter ) || !( isalpha( letter ) || isdigit( letter ) ) ) letter = '_';

        *ppath = letter;

        pname++;
        ppath++;
    }
    *ppath = CSTR_END;

    strncat( szPathname, ".obj", SDL_arraysize( szPathname ) - strlen( szPathname ) );

    return szPathname;
}

//--------------------------------------------------------------------------------------------
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
        else if ( CSTR_END == text )
        {
            // reached the end of the string
            break;
        }
        else if ( C_NEW_LINE_CHAR == *text )
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
            *text_break = C_NEW_LINE_CHAR;

            // start over again
            text = text_break;
        }

        text++;
    }
}
