// ********************************************************************************************
// *
// *    This file is part of Egoboo.
// *
// *    Egoboo is free software: you can redistribute it and/or modify it
// *    under the terms of the GNU General Public License as published by
// *    the Free Software Foundation, either version 3 of the License, or
// *    (at your option) any later version.
// *
// *    Egoboo is distributed in the hope that it will be useful, but
// *    WITHOUT ANY WARRANTY; without even the implied warranty of
// *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// *    General Public License for more details.
// *
// *    You should have received a copy of the GNU General Public License
// *    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
// *
// ********************************************************************************************

#include "egoboo_fileutil.h"

#include "ogl_texture.h"
#include "log.h"

#include "egoboo_setup.h"
#include "egoboo_strutil.h"
#include "egoboo.h"

#include "mad.h"

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------

const char *parse_filename  = NULL;

int   pairbase, pairrand;
float pairfrom, pairto;

STRING          TxFormatSupported[20]; // OpenGL icon surfaces
Uint8           maxformattypes;

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
IDSZ fget_idsz( FILE* fileread )
{
    // ZZ> This function reads and returns an IDSZ tag, or IDSZ_NONE if there wasn't one

    IDSZ idsz = IDSZ_NONE;
    char cTmp = fget_first_letter( fileread );
    if ( cTmp == '[' )
    {
        idsz = 0;
        cTmp = ( fgetc( fileread ) - 'A' ) & 0x1F;  idsz |= cTmp << 15;
        cTmp = ( fgetc( fileread ) - 'A' ) & 0x1F;  idsz |= cTmp << 10;
        cTmp = ( fgetc( fileread ) - 'A' ) & 0x1F;  idsz |= cTmp << 5;
        cTmp = ( fgetc( fileread ) - 'A' ) & 0x1F;  idsz |= cTmp;

        cTmp = fgetc( fileread );
        if ( ']' != cTmp )
        {
            log_warning("Problem reading IDSZ in \"%s\"\n", parse_filename );
        }
    }

    return idsz;
}

// --------------------------------------------------------------------------------------------
bool_t fcopy_line(FILE * fileread, FILE * filewrite)
{
    // / @details BB@> copy a line of arbitrary length, in chunks of length sizeof(linebuffer)
    // / @todo This should be moved to file_common.c

    char linebuffer[64];
    if (NULL == fileread || NULL == filewrite) return bfalse;
    if ( feof(fileread) || feof(filewrite) ) return bfalse;

    fgets(linebuffer, sizeof(linebuffer), fileread);
    fputs(linebuffer, filewrite);
    while ( strlen(linebuffer) == SDL_arraysize(linebuffer) )
    {
        fgets(linebuffer, SDL_arraysize(linebuffer), fileread);
        fputs(linebuffer, filewrite);
    }

    return btrue;
}

// --------------------------------------------------------------------------------------------
bool_t goto_colon( char * buffer, FILE* fileread, bool_t optional )
{
    // ZZ> This function moves a file read pointer to the next colon char cTmp;
    // BB> buffer points to a 256 character buffer that will get the data between the newline and the ':'
    //     Also, the two functions goto_colon and goto_colon_yesno have been combined

    int cTmp, write;

    if ( feof(fileread) || ferror(fileread) ) return bfalse;

    write = 0;
    if (NULL != buffer) buffer[0] = '\0';
    cTmp = fgetc( fileread );
    while ( !feof(fileread) && !ferror(fileread) )
    {
        if ( ':' == cTmp ) break;

        if ( 0x0A == cTmp || 0x0D == cTmp )
        {
            write = 0;
        }
        else
        {
            if (NULL != buffer) buffer[write++] = cTmp;
        }

        cTmp = fgetc( fileread );
    }
    if (NULL != buffer) buffer[write] = '\0';

    if ( !optional && ':' != cTmp )
    {
        // not enough colons in file!
        log_error( "There are not enough colons in file! (%s)\n", parse_filename );
    }

    return (':' == cTmp);
}

// --------------------------------------------------------------------------------------------
char * goto_colon_mem( char * buffer, char * pmem, char * pmem_end, bool_t optional )
{
    // ZZ> This function moves a file read pointer to the next colon char *pmem;
    // BB> buffer points to a 256 character buffer that will get the data between the newline and the ':'
    //     Also, the two functions goto_colon and goto_colon_yesno have been combined

    char cTmp;
    int    write;

    if ( NULL == pmem || pmem >= pmem_end ) return pmem;

    write = 0;
    if (NULL != buffer) buffer[0] = '\0';
    cTmp = *(pmem++);
    while ( pmem < pmem_end )
    {
        if ( ':' == cTmp ) { pmem++; break; }

        if ( 0x0A == cTmp || 0x0D == cTmp )
        {
            write = 0;
        }
        else
        {
            if (NULL != buffer) buffer[write++] = cTmp;
        }

        cTmp = *(pmem++);
    }
    if (NULL != buffer) buffer[write] = '\0';

    if ( !optional && ':' != cTmp )
    {
        // not enough colons in file!
        log_error( "There are not enough colons in file! (%s)\n", parse_filename );
    }

    return pmem;
}

// --------------------------------------------------------------------------------------------
char fget_first_letter( FILE* fileread )
{
    // ZZ> This function returns the next non-whitespace character
    char cTmp;
    fscanf( fileread, "%c", &cTmp );
    while ( isspace( cTmp ) )
    {
        fscanf( fileread, "%c", &cTmp );
    }

    return cTmp;
}

// --------------------------------------------------------------------------------------------
bool_t fget_name( FILE* fileread,  char *szName, size_t max_len )
{
    // ZZ> This function loads a string of up to MAXCAPNAMESIZE characters, parsing
    //     it for underscores.  The szName argument is rewritten with the null terminated
    //     string

    int fields;

    STRING format;

    if (NULL == szName) return bfalse;
    szName[0] = '\0';

    if ( NULL == fileread || (0 != ferror(fileread)) || feof(fileread) ) return bfalse;

    // limit the max length of the string!
    // return value if the number of fields fields, not amount fields from file
    sprintf( format, "%%%ds", max_len - 1 );

    szName[0] = '\0';
    fields = fscanf( fileread, format, szName );

    if ( fields > 0 )
    {
        szName[max_len-1] = '\0';
        str_decode( szName, max_len, szName );
    };

    return (1 == fields) && ferror(fileread);
}

// --------------------------------------------------------------------------------------------
void ftruthf( FILE* filewrite, const char* text, Uint8 truth )
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     btrue bfalse statements

    fprintf( filewrite, "%s", text );
    if ( truth )
    {
        fprintf( filewrite, "TRUE\n" );
    }
    else
    {
        fprintf( filewrite, "FALSE\n" );
    }
}

// --------------------------------------------------------------------------------------------
void fdamagf( FILE* filewrite, const char* text, Uint8 damagetype )
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements
    fprintf( filewrite, "%s", text );
    if ( damagetype == DAMAGE_SLASH )
        fprintf( filewrite, "SLASH\n" );
    if ( damagetype == DAMAGE_CRUSH )
        fprintf( filewrite, "CRUSH\n" );
    if ( damagetype == DAMAGE_POKE )
        fprintf( filewrite, "POKE\n" );
    if ( damagetype == DAMAGE_HOLY )
        fprintf( filewrite, "HOLY\n" );
    if ( damagetype == DAMAGE_EVIL )
        fprintf( filewrite, "EVIL\n" );
    if ( damagetype == DAMAGE_FIRE )
        fprintf( filewrite, "FIRE\n" );
    if ( damagetype == DAMAGE_ICE )
        fprintf( filewrite, "ICE\n" );
    if ( damagetype == DAMAGE_ZAP )
        fprintf( filewrite, "ZAP\n" );
    if ( damagetype == DAMAGE_NONE )
        fprintf( filewrite, "NONE\n" );
}

// --------------------------------------------------------------------------------------------
void factiof( FILE* filewrite, const char* text, Uint8 action )
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements

    fprintf( filewrite, "%s", text );

    switch ( action )
    {
        case ACTION_DA: fprintf( filewrite, "DANCE\n");    break;
        case ACTION_UA: fprintf( filewrite, "UNARMED\n"); break;
        case ACTION_TA: fprintf( filewrite, "THRUST\n");  break;
        case ACTION_CA: fprintf( filewrite, "CHOP\n");    break;
        case ACTION_SA: fprintf( filewrite, "SLASH\n");   break;
        case ACTION_BA: fprintf( filewrite, "BASH\n");    break;
        case ACTION_LA: fprintf( filewrite, "LONGBOW\n"); break;
        case ACTION_XA: fprintf( filewrite, "XBOW\n");    break;
        case ACTION_FA: fprintf( filewrite, "FLING\n");   break;
        case ACTION_PA: fprintf( filewrite, "PARRY\n");   break;
        case ACTION_ZA: fprintf( filewrite, "ZAP\n");     break;
        case ACTION_WA: fprintf( filewrite, "WALK\n");    break;
        case ACTION_HA: fprintf( filewrite, "HIT\n");     break;
        case ACTION_KA: fprintf( filewrite, "KILLED\n");  break;
        default:        fprintf( filewrite, "NONE\n");    break;
    }
}

// --------------------------------------------------------------------------------------------
void fgendef( FILE* filewrite, const char* text, Uint8 gender )
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     MALE FEMALE OTHER statements

    fprintf( filewrite, "%s", text );
    if ( gender == GENMALE )
        fprintf( filewrite, "MALE\n" );
    if ( gender == GENFEMALE )
        fprintf( filewrite, "FEMALE\n" );
    if ( gender == GENOTHER )
        fprintf( filewrite, "OTHER\n" );
}

// --------------------------------------------------------------------------------------------
void fpairof( FILE* filewrite, const char* text, int base, int rand )
{
    // ZZ> This function mimics fprintf in spitting out
    //     damage/stat pairs
    undo_pair( base, rand );
    fprintf( filewrite, "%s", text );
    fprintf( filewrite, "%4.2f-%4.2f\n", pairfrom, pairto );
}

// --------------------------------------------------------------------------------------------
void funderf( FILE* filewrite, const char* text, const char* usename )
{
    // ZZ> This function mimics fprintf in spitting out
    //     a name with underscore spaces
    char cTmp;
    int cnt;

    fprintf( filewrite, "%s", text );
    cnt = 0;
    cTmp = usename[0];
    cnt++;
    while ( cTmp != 0 )
    {
        if ( cTmp == ' ' )
        {
            fprintf( filewrite, "_" );
        }
        else
        {
            fprintf( filewrite, "%c", cTmp );
        }

        cTmp = usename[cnt];
        cnt++;
    }

    fprintf( filewrite, "\n" );
}

// --------------------------------------------------------------------------------------------
bool_t fget_pair( FILE* fileread )
{
    // ZZ> This function reads a damage/stat pair ( eg. 5-9 )
    char cTmp;
    float  fBase, fRand;

    if ( NULL == fileread || ferror(fileread) || feof(fileread) ) return bfalse;

    fBase = fget_float( fileread );  // The first number
    pairbase = fBase * 256;

    cTmp = fget_first_letter( fileread );  // The hyphen
    if ( cTmp != '-' )
    {
        // Not in correct format, so fail
        pairrand = 1;
        return btrue;
    }

    fRand = fget_float( fileread );  // The second number
    pairrand = fRand * 256;

    pairrand = pairrand - pairbase;
    if ( pairrand < 1 )
        pairrand = 1;

    return btrue;
}

// --------------------------------------------------------------------------------------------
void undo_pair( int base, int rand )
{
    // ZZ> This function generates a damage/stat pair ( eg. 3-6.5f )
    //     from the base and random values.  It set pairfrom and
    //     pairto
    pairfrom = base / 256.0f;
    pairto = rand / 256.0f;
    if ( pairfrom < 0.0f )
    {
        pairfrom = 0.0f;
        log_warning( "We got a randomization error again! (Base is less than 0)\n" );
    }
    if ( pairto < 0.0f )
    {
        pairto = 0.0f;
        log_warning( "We got a randomization error again! (Max is less than 0)\n" );
    }

    pairto += pairfrom;
}

// --------------------------------------------------------------------------------------------
void make_newloadname( const char *modname, const char *appendname,  char *newloadname )
{
    // ZZ> This function takes some names and puts 'em together
    int cnt, tnc;
    char ctmp;

    cnt = 0;
    ctmp = modname[cnt];
    while ( ctmp != 0 )
    {
        newloadname[cnt] = ctmp;
        cnt++;
        ctmp = modname[cnt];
    }

    tnc = 0;
    ctmp = appendname[tnc];
    while ( ctmp != 0 )
    {
        newloadname[cnt] = ctmp;
        cnt++;
        tnc++;
        ctmp = appendname[tnc];
    }

    newloadname[cnt] = 0;
}

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
int fget_version( FILE* fileread )
{
    // BB> scanr the file for a "// file_version blah" flag
    long filepos;
    int  ch;
    bool_t newline, iscomment;
    STRING keyword;
    int file_version, fields;

    if ( ferror(fileread) ) return -1;

    filepos = ftell( fileread );

    rewind( fileread );

    file_version = -1;
    iscomment = bfalse;
    while ( !feof( fileread ) )
    {
        ch = fgetc( fileread  );

        // trap new lines
        if ( 0x0A == ch || 0x0D == ch ) { newline = btrue; iscomment = bfalse; continue; }

        // ignore whitespace
        if ( isspace( ch ) ) continue;

        // possible comment
        if ( '/' == ch )
        {
            ch = fgetc( fileread  );
            if ( '/' == ch )
            {
                iscomment = btrue;
            }
        }

        if ( iscomment )
        {
            // this is a comment. if the first word is not "file_version", then it is
            // the wrong type of line to be a file_version statement

            fields = fscanf( fileread, "%255s %d", keyword, &file_version );
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

            ch = fgetc( fileread  );
            while ( !feof( fileread ) && 0x0A != ch && 0x0D != ch )
            {
                ch = fgetc( fileread  );
            }

            iscomment = bfalse;
            continue;
        }
    };

    // reset the file pointer
    fseek( fileread, filepos, SEEK_SET );

    // flear any error we may have generated
    clearerr( fileread );

    return file_version;
}

// --------------------------------------------------------------------------------------------
bool_t fput_version( FILE* filewrite, int file_version )
{
    if ( ferror( filewrite ) ) return bfalse;

    return 0 != fprintf( filewrite, "\n// version %d\n", file_version );
}

// --------------------------------------------------------------------------------------------
char * copy_mem_to_delimiter( char * pmem, char * pmem_end, FILE * filewrite, int delim, char * user_buffer, size_t user_buffer_len )
{
    // BB> copy data from one file to another until the delimiter delim has been found
    //     could be used to merge a template file with data

    int write;
    char cTmp, temp_buffer[1024];

    if ( NULL == pmem || NULL == filewrite ) return pmem;

    if ( ferror(filewrite) ) return pmem;

    write = 0;
    temp_buffer[0] = '\0';
    cTmp = *(pmem++);
    while ( pmem < pmem_end )
    {
        if ( delim == cTmp ) break;

        if ( 0x0A == cTmp || 0x0D == cTmp )
        {
            // output the temp_buffer
            temp_buffer[write] = '\0';
            fputs( temp_buffer, filewrite );
            fputc( cTmp, filewrite );

            // reset the temp_buffer pointer
            write = 0;
            temp_buffer[0] = '\0';
        }
        else
        {
            if ( write > SDL_arraysize(temp_buffer) - 2 )
            {
                log_error( "copy_mem_to_delimiter() - temp_buffer overflow.\n" );
            }

            temp_buffer[write++] = cTmp;
        }

        // only copy if it is not the
        cTmp = *(pmem++);
    }
    temp_buffer[write] = '\0';

    if ( NULL != user_buffer )
    {
        strncpy( user_buffer, temp_buffer, user_buffer_len - 1 );
        user_buffer[user_buffer_len - 1] = '\0';
    }

    if (delim == cTmp)
    {
        pmem--;
    }

    return pmem;
}

// --------------------------------------------------------------------------------------------
char fget_next_char( FILE * fileread )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_first_letter( fileread );
}

// --------------------------------------------------------------------------------------------
int fget_int( FILE * fileread )
{
    int iTmp = 0;

    fscanf( fileread, "%d", &iTmp );

    return iTmp;
}

// --------------------------------------------------------------------------------------------
int fget_next_int( FILE * fileread )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_int( fileread );
}

// --------------------------------------------------------------------------------------------
bool_t fget_string( FILE * fileread, char * str, size_t str_len )
{
    int fields;
    STRING format_str;

    if ( NULL == str || 0 == str_len ) return bfalse;

    sprintf( format_str, "%%%ds", str_len - 1 );

    str[0] = '\0';
    fields = fscanf( fileread, format_str, str );
    str[str_len-1] = '\0';

    return 1 == fields;
}

// --------------------------------------------------------------------------------------------
bool_t fget_next_string( FILE * fileread, char * str, size_t str_len )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_string( fileread, str, str_len );
}

// --------------------------------------------------------------------------------------------
float fget_float( FILE * fileread )
{
    float fTmp;

    fTmp = 0;
    fscanf( fileread, "%f", &fTmp );

    return fTmp;
}

// --------------------------------------------------------------------------------------------
float  fget_next_float( FILE * fileread )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_float( fileread );
}

// --------------------------------------------------------------------------------------------
bool_t fget_next_name ( FILE * fileread, char * name, size_t name_len )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_name( fileread, name, name_len );
}

// --------------------------------------------------------------------------------------------
bool_t fget_next_pair( FILE * fileread )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_pair( fileread );
}

// --------------------------------------------------------------------------------------------
IDSZ fget_next_idsz( FILE * fileread )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_idsz( fileread );
}

// --------------------------------------------------------------------------------------------
int fget_damage_type( FILE * fileread )
{
    char cTmp;
    int type = DAMAGE_NONE;

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
    }

    return type;
}

// --------------------------------------------------------------------------------------------
int fget_next_damage_type( FILE * fileread )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_damage_type( fileread );
}

// --------------------------------------------------------------------------------------------
bool_t fget_bool( FILE * fileread )
{
    char cTmp = fget_first_letter( fileread );

    return ( 'T' == toupper(cTmp) );
}

// --------------------------------------------------------------------------------------------
bool_t fget_next_bool( FILE * fileread )
{
    goto_colon( NULL, fileread, bfalse );

    return fget_bool( fileread );
}

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
void GLSetup_SupportedFormats()
{
    // ZF> This need only to be once
    Uint8 type = 0;

    // define extra supported file types with SDL_image
    // these should probably be ordered so that the types that
    // support transparency are first
    if ( cfg.sdl_image_allowed )
    {
        snprintf(TxFormatSupported[type], sizeof(TxFormatSupported[type]), ".png"); type++;
        snprintf(TxFormatSupported[type], sizeof(TxFormatSupported[type]), ".tif"); type++;
        snprintf(TxFormatSupported[type], sizeof(TxFormatSupported[type]), ".tiff"); type++;
        snprintf(TxFormatSupported[type], sizeof(TxFormatSupported[type]), ".gif"); type++;
        snprintf(TxFormatSupported[type], sizeof(TxFormatSupported[type]), ".pcx"); type++;
        snprintf(TxFormatSupported[type], sizeof(TxFormatSupported[type]), ".ppm"); type++;
        snprintf(TxFormatSupported[type], sizeof(TxFormatSupported[type]), ".jpg"); type++;
        snprintf(TxFormatSupported[type], sizeof(TxFormatSupported[type]), ".jpeg"); type++;
        snprintf(TxFormatSupported[type], sizeof(TxFormatSupported[type]), ".xpm"); type++;
        snprintf(TxFormatSupported[type], sizeof(TxFormatSupported[type]), ".pnm"); type++;
        snprintf(TxFormatSupported[type], sizeof(TxFormatSupported[type]), ".lbm"); type++;
        snprintf(TxFormatSupported[type], sizeof(TxFormatSupported[type]), ".tga"); type++;
    }

    // These typed are natively supported with SDL
    // Place them *after* the SDL_image types, so that if both are present,
    // the other types will be preferred over bmp
    snprintf(TxFormatSupported[type], sizeof(TxFormatSupported[type]), ".bmp"); type++;
    snprintf(TxFormatSupported[type], sizeof(TxFormatSupported[type]), ".BMP"); type++;

    // Save the amount of format types we have in store
    maxformattypes = type;
    if ( !cfg.sdl_image_allowed )
    {
        log_message( "Failed!\n" );
        log_info( "[SDL_IMAGE] set to \"FALSE\" in setup.txt, only support for .bmp files\n");
    }
    else
    {
        log_message( "Success!\n" );
    }
}

// --------------------------------------------------------------------------------------------
Uint32  ego_texture_load( oglx_texture *texture, const char *filename, Uint32 key )
{
    STRING fullname;
    Uint32 retval;
    Uint8 type = 0;
    SDL_Surface * image = NULL;
    GLenum tx_target;

    // get rid of any old data
    oglx_texture_Release( texture );

    // load the image
    retval = INVALID_TX_ID;
    if ( cfg.sdl_image_allowed )
    {
        // try all different formats
        for (type = 0; type < maxformattypes; type++)
        {
            snprintf(fullname, sizeof(fullname), "%s%s", filename, TxFormatSupported[type]);
            retval = oglx_texture_Load( GL_TEXTURE_2D, texture, fullname, key );
            if ( INVALID_TX_ID != retval ) break;
        }
    }
    else
    {
        image = NULL;

        // normal SDL only supports bmp
        snprintf(fullname, sizeof(fullname), "%s.bmp", filename);
        image = SDL_LoadBMP(fullname);

        // We could not load the image
        if ( NULL == image ) return INVALID_TX_ID;

        tx_target = GL_TEXTURE_2D;
        if ( image->w != image->h && (image->w == 1 || image->h) )
        {
            tx_target = GL_TEXTURE_1D;
        }

        retval = oglx_texture_Convert( tx_target, texture, image, key );
        strncpy(texture->name, fullname, sizeof(texture->name));

        texture->base.wrap_s = GL_REPEAT;
        texture->base.wrap_t = GL_REPEAT;
    }

    return retval;
}
