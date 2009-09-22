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

/* Egoboo - template.c
 */

#include "template.h"

#include "char.h"

#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static char * template_dump_buffer( vfs_FILE * outfile, char * buffer_beg, char * buffer_end, char * pcarat );

static bool_t template_seek_marker( vfs_FILE * tempfile, const char * marker_str );
static bool_t template_copy_to_marker( vfs_FILE * tempfile, vfs_FILE * outfile, const char * marker_str );
static void   template_copy_to_eof( vfs_FILE * tempfile, vfs_FILE * outfile );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
char * template_dump_buffer( vfs_FILE * outfile, char * buffer_beg, char * buffer_end, char * pcarat )
{
    // BB> copy any buffer characters to the output file

    char * ptmp;

    ptmp = buffer_beg;
    while( ptmp < pcarat && ptmp < buffer_end )
    {
        vfs_putc( *ptmp++, outfile );
    }

    return buffer_beg;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t template_seek_marker( vfs_FILE * tempfile, const char * marker_str )
{
    bool_t       found;
    char         cTmp;
    char const * pmark = marker_str;

    found = bfalse;
    pmark = marker_str;
    while( !vfs_eof( tempfile )  )
    {
        cTmp = vfs_getc( tempfile );
        if( cTmp == *pmark )
        {
            pmark++;
            if( '\0' == *pmark )
            {
                found = btrue;
                break;
            }
        }
        else
        {
            pmark = marker_str;
        }
    }

    return found;
}

//--------------------------------------------------------------------------------------------
bool_t template_copy_to_marker( vfs_FILE * tempfile, vfs_FILE * outfile, const char * marker_str )
{
    bool_t       found;
    char         cTmp;
    char const * pmark = marker_str;

    // the buffer only has to be longer than marker_str, which should
    // never be any longer than 2 characters
    char buffer[256], * pcarat, * buffer_end = buffer + 255;

    found  = bfalse;
    pmark  = marker_str;
    pcarat = buffer;
    while( !vfs_eof( tempfile )  )
    {
        cTmp = vfs_getc( tempfile );
        if( cTmp == *pmark )
        {
            *pcarat++ = cTmp;
            pmark++;
            if( '\0' == *pmark )
            {
                found = btrue;
                break;
            }
        }
        else
        {
            // reset the marker string
            pmark = marker_str;

            // copy any stored values to the output file
            pcarat = template_dump_buffer( outfile, buffer, buffer_end, pcarat );
        }
    }

    // dump any buffer that remains
    template_dump_buffer( outfile, buffer, buffer_end, pcarat );

    return found;
}

//--------------------------------------------------------------------------------------------
void template_copy_to_eof( vfs_FILE * tempfile, vfs_FILE * outfile )
{
    while( !vfs_eof( tempfile )  )
    {
        vfs_putc( vfs_getc( tempfile ), outfile );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
vfs_FILE* template_open( const char * filename )
{
    return vfs_openRead( filename );
}

//--------------------------------------------------------------------------------------------
int template_close( vfs_FILE* filetemp )
{
    return vfs_close( filetemp );
}

//--------------------------------------------------------------------------------------------
bool_t template_seek_free( vfs_FILE* filetemp, vfs_FILE* filewrite )
{
    return template_copy_to_marker( filewrite, filetemp, "##" );
}

//--------------------------------------------------------------------------------------------
void template_flush( vfs_FILE* filetemp, vfs_FILE* filewrite )
{
    template_copy_to_eof( filewrite, filetemp );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void template_put_char ( vfs_FILE* filetemp, vfs_FILE* filewrite, char cval )
{
    if( template_copy_to_marker( filewrite, filetemp, "#" ) )
    {
        vfs_putc( cval, filewrite );
    }
}

//--------------------------------------------------------------------------------------------
void template_put_int( vfs_FILE* filetemp, vfs_FILE* filewrite, int ival )
{
    if( template_copy_to_marker( filewrite, filetemp, "#" ) )
    {
        vfs_printf( filewrite, "%d", ival );
    }
}

//--------------------------------------------------------------------------------------------
void template_put_float( vfs_FILE* filetemp, vfs_FILE* filewrite, float fval )
{
    if( template_copy_to_marker( filewrite, filetemp, "#" ) )
    {
        vfs_printf( filewrite, "%f", fval );
    }
}

//--------------------------------------------------------------------------------------------
void template_put_bool( vfs_FILE* filetemp, vfs_FILE* filewrite, bool_t truth )
{
    if( template_copy_to_marker( filewrite, filetemp, "#" ) )
    {
        vfs_printf( filewrite, truth ? "TRUE" : "FALSE" );
    }
}

//--------------------------------------------------------------------------------------------
void template_put_damage_type ( vfs_FILE* filetemp, vfs_FILE* filewrite, Uint8 damagetype )
{
    if( template_copy_to_marker( filewrite, filetemp, "#" ) )
    {
        switch( damagetype )
        {
            case DAMAGE_SLASH: vfs_printf( filewrite, "SLASH" ); break;
            case DAMAGE_CRUSH: vfs_printf( filewrite, "CRUSH" ); break;
            case DAMAGE_POKE : vfs_printf( filewrite, "POKE"  ); break;
            case DAMAGE_HOLY : vfs_printf( filewrite, "HOLY"  ); break;
            case DAMAGE_EVIL : vfs_printf( filewrite, "EVIL"  ); break;
            case DAMAGE_FIRE : vfs_printf( filewrite, "FIRE"  ); break;
            case DAMAGE_ICE  : vfs_printf( filewrite, "ICE"   ); break;
            case DAMAGE_ZAP  : vfs_printf( filewrite, "ZAP"   ); break;

            default:
            case DAMAGE_NONE : vfs_printf( filewrite, "NONE"  ); break;
        }
    }
}

//--------------------------------------------------------------------------------------------
void template_put_action( vfs_FILE* filetemp, vfs_FILE* filewrite, Uint8 action )
{
    if( template_copy_to_marker( filewrite, filetemp, "#" ) )
    {
        switch ( action )
        {
            case ACTION_DA: vfs_printf( filewrite, "DANCE"  ); break;
            case ACTION_UA: vfs_printf( filewrite, "UNARMED"); break;
            case ACTION_TA: vfs_printf( filewrite, "THRUST" ); break;
            case ACTION_CA: vfs_printf( filewrite, "CHOP"   ); break;
            case ACTION_SA: vfs_printf( filewrite, "SLASH"  ); break;
            case ACTION_BA: vfs_printf( filewrite, "BASH"   ); break;
            case ACTION_LA: vfs_printf( filewrite, "LONGBOW"); break;
            case ACTION_XA: vfs_printf( filewrite, "XBOW"   ); break;
            case ACTION_FA: vfs_printf( filewrite, "FLING"  ); break;
            case ACTION_PA: vfs_printf( filewrite, "PARRY"  ); break;
            case ACTION_ZA: vfs_printf( filewrite, "ZAP"    ); break;
            case ACTION_WA: vfs_printf( filewrite, "WALK"   ); break;
            case ACTION_HA: vfs_printf( filewrite, "HIT"    ); break;
            case ACTION_KA: vfs_printf( filewrite, "KILLED" ); break;
            default:        vfs_printf( filewrite, "NONE"   ); break;
        }
    }
}

//--------------------------------------------------------------------------------------------
void template_put_gender( vfs_FILE* filetemp, vfs_FILE* filewrite, Uint8 gender )
{
    if( template_copy_to_marker( filewrite, filetemp, "#" ) )
    {
        switch( gender )
        {
            case GENDER_MALE  : vfs_printf( filewrite, "MALE"   ); break;
            case GENDER_FEMALE: vfs_printf( filewrite, "FEMALE" ); break;
            default:
            case GENDER_OTHER : vfs_printf( filewrite, "OTHER"  ); break;
        }
    }
}

//--------------------------------------------------------------------------------------------
void template_put_pair( vfs_FILE* filetemp, vfs_FILE* filewrite, IPair val )
{
    if( template_copy_to_marker( filewrite, filetemp, "#" ) )
    {
        undo_pair( val.base, val.rand );
        vfs_printf( filewrite, "%4.2f-%4.2f", range.from, range.to );
    }
}

//--------------------------------------------------------------------------------------------
void template_put_string_under( vfs_FILE* filetemp, vfs_FILE* filewrite, const char* usename )
{
    if( template_copy_to_marker( filewrite, filetemp, "#" ) )
    {
        STRING tmp_str;

        str_encode( tmp_str, SDL_arraysize(tmp_str), usename );

        vfs_printf( filewrite, "%s", tmp_str );
    }
}

//--------------------------------------------------------------------------------------------
void template_put_idsz( vfs_FILE* filetemp, vfs_FILE* filewrite, IDSZ idsz )
{
    if( template_copy_to_marker( filewrite, filetemp, "#" ) )
    {
        vfs_printf( filewrite, "[%s]", undo_idsz(idsz) );
    }
}

//--------------------------------------------------------------------------------------------
void template_put_damage_modifier( vfs_FILE* filetemp, vfs_FILE* filewrite, Uint8 mod )
{
    // this eats two '#'s in the template file

    // put the mod bits
    if( template_copy_to_marker( filewrite, filetemp, "#" ) )
    {
        if ( 0 != (mod & DAMAGEINVERT) )
        {
            vfs_putc( 'T', filewrite);
        }
        else if ( 0 != (mod & DAMAGECHARGE) )
        {
            vfs_putc( 'C', filewrite);
        }
        else if ( 0 != (mod & DAMAGEMANA) )
        {
            vfs_putc( 'M', filewrite);
        }
        else
        {
            vfs_putc( 'F', filewrite);
        }
    }

    // now put the damage shift
    template_put_int( filetemp, filewrite, mod & DAMAGESHIFT );
}