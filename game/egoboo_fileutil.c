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

/// @file egoboo_fileutil.c
/// @brief Implementation of Egoboo file utilities
/// @details

#include "egoboo_fileutil.h"

#include "extensions/ogl_texture.h"
#include "log.h"

#include "mad.h"
#include "char.inl"

#include "egoboo_setup.h"
#include "egoboo_strutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

const char *parse_filename  = NULL;

IPair   pair;
FRange  range;

STRING          TxFormatSupported[20]; // OpenGL icon surfaces
Uint8           maxformattypes;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
IDSZ fget_idsz( vfs_FILE* fileread )
{
    /// @details ZZ@> This function reads and returns an IDSZ tag, or IDSZ_NONE if there wasn't one

    IDSZ idsz = IDSZ_NONE;

    char cTmp = fget_first_letter( fileread );
    if ( '[' == cTmp )
    {
        long fpos;
        int  i;
        char idsz_str[5] = EMPTY_CSTR;

        fpos = vfs_tell( fileread );

        for ( i = 0; i < 4; i++ )
        {
            cTmp = toupper( vfs_getc( fileread ) );
            if ( !isalpha( cTmp ) && !isdigit( cTmp ) && ( '_' != cTmp ) ) break;

            idsz_str[i] = cTmp;
        }

        if ( i != 4 )
        {
            log_warning( "Problem reading IDSZ in \"%s\"\n", parse_filename );
        }
        else
        {
            idsz = MAKE_IDSZ( idsz_str[0], idsz_str[1], idsz_str[2], idsz_str[3] );

            cTmp = vfs_getc( fileread );
            if ( ']' != cTmp )
            {
                log_warning( "Problem reading IDSZ in \"%s\"\n", parse_filename );
            }
        }
    }

    return idsz;
}

//--------------------------------------------------------------------------------------------
bool_t fcopy_line( vfs_FILE * fileread, vfs_FILE * filewrite )
{
    /// @details BB@> copy a line of arbitrary length, in chunks of length sizeof(linebuffer)
    /// @todo This should be moved to file_common.c

    char linebuffer[64];
    if ( NULL == fileread || NULL == filewrite ) return bfalse;
    if ( vfs_eof( fileread ) || vfs_eof( filewrite ) ) return bfalse;

    vfs_gets( linebuffer, SDL_arraysize( linebuffer ), fileread );
    vfs_puts( linebuffer, filewrite );
    while ( strlen( linebuffer ) == SDL_arraysize( linebuffer ) )
    {
        vfs_gets( linebuffer, SDL_arraysize( linebuffer ), fileread );
        vfs_puts( linebuffer, filewrite );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t goto_delimiter( char * buffer, vfs_FILE* fileread, char delim, bool_t optional )
{
    /// @details ZZ@> This function moves a file read pointer to the next delimiter char cTmp;
    /// BB@> buffer points to a 256 character buffer that will get the data between the newline and the delim

    int cTmp, write;

    if ( vfs_eof( fileread )  || vfs_error( fileread ) ) return bfalse;

    write = 0;
    if ( NULL != buffer ) buffer[0] = CSTR_END;
    cTmp = vfs_getc( fileread );
    while ( !vfs_eof( fileread ) && !vfs_error( fileread ) )
    {
        if ( delim == cTmp ) break;

        if ( ASCII_LINEFEED_CHAR ==  cTmp || C_CARRIAGE_RETURN_CHAR ==  cTmp || CSTR_END == cTmp )
        {
            write = 0;
        }
        else
        {
            if ( NULL != buffer ) buffer[write++] = cTmp;
        }

        cTmp = vfs_getc( fileread );
    }
    if ( NULL != buffer ) buffer[write] = CSTR_END;

    if ( !optional && delim != cTmp )
    {
        // not enough colons in file!
        log_error( "There are not enough %c's in file! (%s)\n", delim, parse_filename );
    }

    return ( delim == cTmp );
}

//--------------------------------------------------------------------------------------------
char goto_delimiter_list( char * buffer, vfs_FILE* fileread, const char * delim_list, bool_t optional )
{
    /// @details ZZ@> This function moves a file read pointer to the next colon char cTmp;
    /// BB@> buffer points to a 256 character buffer that will get the data between the newline and the ':'
    ///
    ///    returns the delimiter that was found, or CSTR_END if no delimiter found

    char   retval = CSTR_END;
    int    cTmp, write;
    bool_t is_delim;

    if ( INVALID_CSTR( delim_list ) ) return bfalse;

    if ( vfs_eof( fileread ) || vfs_error( fileread ) ) return bfalse;

    // use a simpler function if it is easier
    if ( 1 == strlen( delim_list ) )
    {
        bool_t rv = goto_delimiter( buffer, fileread, delim_list[0], optional );
        retval = rv ? delim_list[0] : retval;
    }

    if ( NULL != buffer ) buffer[0] = CSTR_END;

    is_delim = bfalse;
    write    = 0;
    cTmp = vfs_getc( fileread );
    while ( !vfs_eof( fileread ) && !vfs_error( fileread ) )
    {
        is_delim = ( NULL != strchr( delim_list, cTmp ) );

        if ( is_delim )
        {
            retval = cTmp;
            break;
        }

        if ( ASCII_LINEFEED_CHAR ==  cTmp || C_CARRIAGE_RETURN_CHAR ==  cTmp || CSTR_END == cTmp )
        {
            write = 0;
        }
        else
        {
            if ( NULL != buffer ) buffer[write++] = cTmp;
        }

        cTmp = vfs_getc( fileread );
    }
    if ( NULL != buffer ) buffer[write] = CSTR_END;

    if ( !optional && !is_delim )
    {
        // not enough colons in file!
        log_error( "There are not enough delimiters (%s) in file! (%s)\n", delim_list, parse_filename );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t goto_colon( char * buffer, vfs_FILE* fileread, bool_t optional )
{
    /// @details BB@> the two functions goto_colon and goto_colon_yesno have been combined

    return goto_delimiter( buffer, fileread, ':', optional );
}

//--------------------------------------------------------------------------------------------
char * goto_colon_mem( char * buffer, char * pmem, char * pmem_end, bool_t optional )
{
    /// @details ZZ@> This function moves a file read pointer to the next colon char *pmem;
    /// BB@> buffer points to a 256 character buffer that will get the data between the newline and the ':'
    ///    Also, the two functions goto_colon and goto_colon_yesno have been combined

    char cTmp;
    int    write;

    if ( NULL == pmem || pmem >= pmem_end ) return pmem;

    write = 0;
    if ( NULL != buffer ) buffer[0] = CSTR_END;
    cTmp = *( pmem++ );
    while ( pmem < pmem_end )
    {
        if ( ':' == cTmp ) { pmem++; break; }

        if ( ASCII_LINEFEED_CHAR ==  cTmp || C_CARRIAGE_RETURN_CHAR ==  cTmp )
        {
            write = 0;
        }
        else
        {
            if ( NULL != buffer ) buffer[write++] = cTmp;
        }

        cTmp = *( pmem++ );
    }
    if ( NULL != buffer ) buffer[write] = CSTR_END;

    if ( !optional && ':' != cTmp )
    {
        // not enough colons in file!
        log_error( "There are not enough colons in file! (%s)\n", parse_filename );
    }

    return pmem;
}

//--------------------------------------------------------------------------------------------
char fget_first_letter( vfs_FILE* fileread )
{
    /// @details ZZ@> This function returns the next non-whitespace character
    char cTmp;
    vfs_scanf( fileread, "%c", &cTmp );
    while ( isspace( cTmp ) )
    {
        vfs_scanf( fileread, "%c", &cTmp );
    }

    return cTmp;
}

//--------------------------------------------------------------------------------------------
bool_t fget_name( vfs_FILE* fileread,  char *szName, size_t max_len )
{
    /// @details ZZ@> This function loads a string of up to MAXCAPNAMESIZE characters, parsing
    ///    it for underscores.  The szName argument is rewritten with the null terminated
    ///    string

    int fields;

    STRING format;

    if ( NULL == szName ) return bfalse;
    szName[0] = CSTR_END;

    if ( NULL == fileread || ( 0 != vfs_error( fileread ) ) || vfs_eof( fileread ) ) return bfalse;

    // limit the max length of the string!
    // return value if the number of fields fields, not amount fields from file
    snprintf( format, SDL_arraysize( format ), "%%%ds", max_len - 1 );

    szName[0] = CSTR_END;
    fields = vfs_scanf( fileread, format, szName );

    if ( fields > 0 )
    {
        szName[max_len-1] = CSTR_END;
        str_decode( szName, max_len, szName );
    };

    return ( 1 == fields ) && vfs_error( fileread );
}

//--------------------------------------------------------------------------------------------
void fput_int( vfs_FILE* filewrite, const char* text, int ival )
{
    /// @details ZZ@> This function kinda mimics fprintf for integers

    vfs_printf( filewrite, "%s %d\n", text, ival );
}

//--------------------------------------------------------------------------------------------
void fput_float( vfs_FILE* filewrite, const char* text, float fval )
{
    /// @details ZZ@> This function kinda mimics fprintf for integers

    vfs_printf( filewrite, "%s %f\n", text, fval );
}

//--------------------------------------------------------------------------------------------
void fput_bool( vfs_FILE* filewrite, const char* text, bool_t truth )
{
    /// @details ZZ@> This function kinda mimics vfs_printf for the output of
    ///    btrue bfalse statements

    vfs_printf( filewrite, "%s", text );
    vfs_printf( filewrite, truth ? "TRUE" : "FALSE" );
    vfs_printf( filewrite, "\n" );
}

//--------------------------------------------------------------------------------------------
void fput_damage_type( vfs_FILE* filewrite, const char* text, Uint8 damagetype )
{
    /// @details ZZ@> This function kinda mimics vfs_printf for the output of
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
void fput_action( vfs_FILE* filewrite, const char* text, Uint8 action )
{
    /// @details ZZ@> This function kinda mimics vfs_printf for the output of
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
void fput_gender( vfs_FILE* filewrite, const char* text, Uint8 gender )
{
    /// @details ZZ@> This function kinda mimics vfs_printf for the output of
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
void fput_range_raw( vfs_FILE* filewrite, FRange val )
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
void fput_range( vfs_FILE* filewrite, const char* text, FRange val )
{
    /// @details ZZ@> This function mimics vfs_printf in spitting out
    ///    damage/stat pairs. Try to print out the least amount of text.

    vfs_printf( filewrite, "%s", text );

    fput_range_raw( filewrite, val );

    vfs_printf( filewrite, "\n", text );
}

//--------------------------------------------------------------------------------------------
void fput_pair( vfs_FILE* filewrite, const char* text, IPair val )
{
    /// @details ZZ@> This function mimics vfs_printf in spitting out
    ///    damage/stat pairs

    FRange loc_range;

    pair_to_range( val, &loc_range );

    vfs_printf( filewrite, "%s", text );
    vfs_printf( filewrite, "%4.2f-%4.2f\n", loc_range.from, loc_range.to );
}

//--------------------------------------------------------------------------------------------
void fput_string_under( vfs_FILE* filewrite, const char* text, const char* usename )
{
    /// @details ZZ@> This function mimics vfs_printf in spitting out
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
void fput_idsz( vfs_FILE* filewrite, const char* text, IDSZ idsz )
{
    vfs_printf( filewrite, "%s", text );
    vfs_printf( filewrite, "[%s]\n", undo_idsz( idsz ) );
}

//--------------------------------------------------------------------------------------------
void fput_expansion( vfs_FILE* filewrite, const char* text, IDSZ idsz, int value )
{
    /// @details ZZ@> This function mimics vfs_printf in spitting out
    ///    damage/stat pairs

    vfs_printf( filewrite, "%s: [%s] %d\n", text, undo_idsz( idsz ), value );
}

//--------------------------------------------------------------------------------------------
void fput_expansion_float( vfs_FILE* filewrite, const char* text, IDSZ idsz, float value )
{
    /// @details ZF@> This function mimics vfs_printf in spitting out
    ///    damage/stat pairs for floating point values

    vfs_printf( filewrite, "%s: [%s] %4.2f\n", text, undo_idsz( idsz ), value );
}

//--------------------------------------------------------------------------------------------
void fput_expansion_string( vfs_FILE* filewrite, const char* text, IDSZ idsz, const char * str )
{
    /// @details ZF@> This function mimics vfs_printf in spitting out
    ///    damage/stat pairs for floating point values

    if ( !VALID_CSTR( str ) )
    {
        fput_expansion( filewrite, text, idsz, 0 );
    }
    else
    {
        vfs_printf( filewrite, "%s: [%s] %s\n", text, undo_idsz( idsz ), str );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fget_range( vfs_FILE* fileread, FRange * prange )
{
    /// @details ZZ@> This function reads a damage/stat range ( eg. 5-9 )

    char  cTmp;
    float fFrom, fTo;
    long fpos;

    if ( NULL == fileread || vfs_error( fileread ) || vfs_eof( fileread ) ) return bfalse;

    // read the range
    fFrom = fget_float( fileread );  // The first number
    fTo   = fFrom;

    // The optional hyphen
    fpos = vfs_tell( fileread );
    cTmp = fget_first_letter( fileread );

    if ( '-' != cTmp )
    {
        // oops... reset the file position, just in calse
        //vfs_seek( fileread, fpos );
    }
    else
    {
        // The optional second number
        fTo = fget_float( fileread );
    }

    if ( NULL != prange )
    {
        prange->from = MIN( fFrom, fTo );
        prange->to   = MAX( fFrom, fTo );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_range( vfs_FILE* fileread, FRange * prange )
{
    /// @details ZZ@> This function reads a damage/stat range ( eg. 5-9 )

    goto_colon( NULL, fileread, bfalse );

    return fget_range( fileread, prange );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fget_pair( vfs_FILE* fileread, IPair * ppair )
{
    /// @details ZZ@> This function reads a damage/stat loc_pair ( eg. 5-9 )

    FRange loc_range;

    if ( !fget_range( fileread, &loc_range ) ) return bfalse;

    if ( NULL != ppair )
    {
        // convert the range to a pair
        ppair->base = FLOAT_TO_FP8( loc_range.from );
        ppair->rand = FLOAT_TO_FP8( loc_range.to - loc_range.from );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void make_newloadname( const char *modname, const char *appendname,  char *newloadname )
{
    /// @details ZZ@> This function takes some names and puts 'em together
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
int fget_version( vfs_FILE* fileread )
{
    /// @details BB@> scanr the file for a "// file_version blah" flag
    long filepos;
    int  ch;
    bool_t newline, iscomment;
    STRING keyword;
    int file_version, fields;

    if ( vfs_error( fileread ) ) return -1;

    filepos = vfs_tell( fileread );

    vfs_seek( fileread, 0 );

    file_version = -1;
    iscomment = bfalse;
    while ( !vfs_eof( fileread ) )
    {
        ch = vfs_getc( fileread );

        // trap new lines
        if ( ASCII_LINEFEED_CHAR ==  ch || C_CARRIAGE_RETURN_CHAR ==  ch ) { newline = btrue; iscomment = bfalse; continue; }

        // ignore whitespace
        if ( isspace( ch ) ) continue;

        // possible comment
        if ( C_SLASH_CHR == ch )
        {
            ch = vfs_getc( fileread );
            if ( C_SLASH_CHR == ch )
            {
                iscomment = btrue;
            }
        }

        if ( iscomment )
        {
            // this is a comment. if the first word is not "file_version", then it is
            // the wrong type of line to be a file_version statement

            fields = vfs_scanf( fileread, "%255s %d", keyword, &file_version );
            if ( 2 == fields && 0 == stricmp( keyword, "file_version" ) )
            {
                // !! found it !!
                break;
            }
            else
            {
                iscomment = bfalse;
            }
        }
        else
        {
            // read everything to the end of the line because it is
            // the wrong type of line to be a file_version statement

            ch = vfs_getc( fileread );
            while ( !vfs_eof( fileread ) && ASCII_LINEFEED_CHAR != ch && C_CARRIAGE_RETURN_CHAR != ch )
            {
                ch = vfs_getc( fileread );
            }

            iscomment = bfalse;
            continue;
        }
    };

    // reset the file pointer
    vfs_seek( fileread, filepos );

    // flear any error we may have generated
    /* clearerr( fileread ); */

    return file_version;
}

//--------------------------------------------------------------------------------------------
bool_t fput_version( vfs_FILE* filewrite, int file_version )
{
    if ( vfs_error( filewrite ) ) return bfalse;

    return 0 != vfs_printf( filewrite, "\n// version %d\n", file_version );
}

//--------------------------------------------------------------------------------------------
char * copy_mem_to_delimiter( char * pmem, char * pmem_end, vfs_FILE * filewrite, int delim, char * user_buffer, size_t user_buffer_len )
{
    /// @details BB@> copy data from one file to another until the delimiter delim has been found
    ///    could be used to merge a template file with data

    int write;
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
                log_error( "copy_mem_to_delimiter() - temp_buffer overflow.\n" );
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
char fget_next_char( vfs_FILE * fileread )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_first_letter( fileread );
}

//--------------------------------------------------------------------------------------------
int fget_int( vfs_FILE * fileread )
{
    int iTmp = 0;

    vfs_scanf( fileread, "%d", &iTmp );

    return iTmp;
}

//--------------------------------------------------------------------------------------------
int fget_next_int( vfs_FILE * fileread )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_int( fileread );
}

//--------------------------------------------------------------------------------------------
bool_t fget_string( vfs_FILE * fileread, char * str, size_t str_len )
{
    int fields;
    STRING format_str;

    if ( NULL == str || 0 == str_len ) return bfalse;

    snprintf( format_str, SDL_arraysize( format_str ), "%%%ds", str_len - 1 );

    str[0] = CSTR_END;
    fields = vfs_scanf( fileread, format_str, str );
    str[str_len-1] = CSTR_END;

    return 1 == fields;
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_string( vfs_FILE * fileread, char * str, size_t str_len )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_string( fileread, str, str_len );
}

//--------------------------------------------------------------------------------------------
float fget_float( vfs_FILE * fileread )
{
    float fTmp;

    fTmp = 0;
    vfs_scanf( fileread, "%f", &fTmp );

    return fTmp;
}

//--------------------------------------------------------------------------------------------
float  fget_next_float( vfs_FILE * fileread )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_float( fileread );
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_name( vfs_FILE * fileread, char * name, size_t name_len )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_name( fileread, name, name_len );
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_pair( vfs_FILE * fileread, IPair * ppair )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_pair( fileread, ppair );
}

//--------------------------------------------------------------------------------------------
IDSZ fget_next_idsz( vfs_FILE * fileread )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_idsz( fileread );
}

//--------------------------------------------------------------------------------------------
int fget_damage_type( vfs_FILE * fileread )
{
    char cTmp;
    int type;

    cTmp = fget_first_letter( fileread );

    switch ( toupper( cTmp ) )
    {
        case 'S': type = DAMAGE_SLASH; break;
        case 'C': type = DAMAGE_CRUSH; break;
        case 'P': type = DAMAGE_POKE;  break;
        case 'H': type = DAMAGE_HOLY;  break;
        case 'E': type = DAMAGE_EVIL;  break;
        case 'F': type = DAMAGE_FIRE;  break;
        case 'I': type = DAMAGE_ICE;   break;
        case 'Z': type = DAMAGE_ZAP;   break;

        default: type = DAMAGE_NONE; break;
    }

    return type;
}

//--------------------------------------------------------------------------------------------
int fget_next_damage_type( vfs_FILE * fileread )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_damage_type( fileread );
}

//--------------------------------------------------------------------------------------------
bool_t fget_bool( vfs_FILE * fileread )
{
    char cTmp = fget_first_letter( fileread );

    return ( 'T' == toupper( cTmp ) );
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_bool( vfs_FILE * fileread )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_bool( fileread );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void GLSetup_SupportedFormats()
{
    /// @details ZF@> This need only to be once

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
    GLenum tx_target;

    // get rid of any old data
    oglx_texture_Release( texture );

    // load the image
    retval = INVALID_GL_ID;
    if ( cfg.sdl_image_allowed )
    {
        // try all different formats
        for ( type = 0; type < maxformattypes; type++ )
        {
            snprintf( fullname, SDL_arraysize( fullname ), "%s%s", filename, TxFormatSupported[type] );
            retval = oglx_texture_Load( texture, vfs_resolveReadFilename( fullname ), key );
            if ( INVALID_GL_ID != retval ) break;
        }
    }
    else
    {
        image = NULL;

        // normal SDL only supports bmp
        snprintf( fullname, SDL_arraysize( fullname ), "%s.bmp", filename );
        image = SDL_LoadBMP( vfs_resolveReadFilename( fullname ) );

        // We could not load the image
        if ( NULL == image ) return INVALID_GL_ID;

        tx_target = GL_TEXTURE_2D;
        if ( image->w != image->h && ( 1 == image->w || image->h ) )
        {
            tx_target = GL_TEXTURE_1D;
        }

        retval = oglx_texture_Convert( texture, image, key );
        strncpy( texture->name, fullname, SDL_arraysize( texture->name ) );

        texture->base.wrap_s = GL_REPEAT;
        texture->base.wrap_t = GL_REPEAT;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
Uint8 fget_damage_modifier( vfs_FILE * fileread )
{
    int  iTmp, tTmp;
    char cTmp;

    cTmp = fget_first_letter( fileread );

    switch ( toupper( cTmp ) )
    {
        case 'T': iTmp = DAMAGEINVERT;      break;
        case 'C': iTmp = DAMAGECHARGE;      break;
        case 'M': iTmp = DAMAGEMANA;        break;
        case 'I': iTmp = DAMAGEINVICTUS;    break;
        default:  iTmp = 0;                 break;
    };

    tTmp = fget_int( fileread );

    return iTmp | tTmp;
}

//--------------------------------------------------------------------------------------------
int read_skin_vfs( const char *filename )
{
    /// @details ZZ@> This function reads the skin.txt file...
    vfs_FILE*   fileread;
    int skin = NO_SKIN_OVERRIDE;

    fileread = vfs_openRead( filename );
    if ( NULL != fileread )
    {
        //Read the contents
        skin = fget_next_int( fileread );
        skin %= MAX_SKIN;

        vfs_close( fileread );
    }

    return skin;
}