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

/// @file file_formats/scancode_file.c
/// @brief Functions to read and write Egoboo's basicdat/scantag.txt file
/// @details

#include "scancode_file.h"

#include "../log.h"
#include "../input_device.h"

#include "../vfs.h"
#include "../fileutil.h"
#include "../strutil.h"
#include "../platform.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static size_t    scantag_count = 0;
static scantag_t scantag_lst[MAXTAG];

static void   scantag_reset( void );
static bool_t scantag_read_one( vfs_FILE *fileread );
static bool_t scantag_matches_control( scantag_t * ptag, control_t * pcontrol );
static bool_t scantag_matches_device( scantag_t * ptag, int device_type );

static const char * scantag_tok( const char * tag_string );

//--------------------------------------------------------------------------------------------
// Tag Reading
//--------------------------------------------------------------------------------------------
void scantag_reset()
{
    /// @details ZZ@> This function resets the tags
    scantag_count = 0;
}

//--------------------------------------------------------------------------------------------
bool_t scantag_read_one( vfs_FILE *fileread )
{
    /// @details ZZ@> This function finds the next tag, returning btrue if it found one

    bool_t retval;

    retval = goto_colon_vfs( NULL, fileread, btrue ) && ( scantag_count < MAXTAG );
    if ( retval )
    {
        vfs_get_string( fileread, scantag_lst[scantag_count].name, SDL_arraysize( scantag_lst[scantag_count].name ) );
        scantag_lst[scantag_count].value = vfs_get_int( fileread );
        scantag_count++;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void scantag_read_all_vfs( const char *szFilename )
{
    /// @details ZZ@> This function reads the scancode.txt file
    vfs_FILE* fileread;
    int cnt;

    scantag_reset();

    fileread = vfs_openRead( szFilename );
    if ( NULL == fileread )
    {
        log_error( "Cannot read %s.", szFilename );
    }

    // read in all the scantags from the file
    while ( scantag_read_one( fileread ) );

    vfs_close( fileread );

    // make extra scantags to support joystick buttons up to 32 bits
    for ( cnt = 0; cnt < 32; cnt++ )
    {
        STRING str_tmp;

        snprintf( str_tmp, SDL_arraysize( str_tmp ), "JOY_%d", cnt );
        if ( -1 != scantag_find_index( str_tmp ) ) continue;

        strncpy( scantag_lst[scantag_count].name, str_tmp, SDL_arraysize( scantag_lst[scantag_count].name ) );
        scantag_lst[scantag_count].value = 1 << cnt;
        scantag_count++;
    }
}

//--------------------------------------------------------------------------------------------
const char * scantag_tok( const char * tag_string )
{
    /// @details BB@> scan through a tag string, finding all valid tags. This will allow multiple
    ///               keys, buttons, and keymods per command

    static char scantag_delimiters[] = " ,|+&\t\n";

    const char * token = NULL;
    const char * name  = NULL;
    int          index = -1;

    // get the next token
    token = strtok( tag_string, scantag_delimiters );
    if( NULL == token ) return NULL;

    // does the scantag exist?
    index = scantag_find_index( token );
    if( index < 0 || index >= scantag_count ) return NULL;

    // get the name of this tag tag
    name = scantag_get_name( index );

    return name;
}

//--------------------------------------------------------------------------------------------
Uint32 scancode_get_kmod( Uint32 scancode )
{
    Uint32 kmod = 0;

    switch( scancode )
    {
        case SDLK_NUMLOCK:  kmod = KMOD_NUM;    break;
	    case SDLK_CAPSLOCK: kmod = KMOD_CAPS;   break;
	    case SDLK_RSHIFT:   kmod = KMOD_LSHIFT; break;
	    case SDLK_LSHIFT:   kmod = KMOD_RSHIFT; break;
	    case SDLK_RCTRL:    kmod = KMOD_LCTRL;  break;
	    case SDLK_LCTRL:    kmod = KMOD_RCTRL;  break;
	    case SDLK_RALT:     kmod = KMOD_LALT;   break;
	    case SDLK_LALT:     kmod = KMOD_RALT;   break;
	    case SDLK_RMETA:    kmod = KMOD_LMETA;  break;
	    case SDLK_LMETA:    kmod = KMOD_RMETA;  break;

        // unhandled cases
        case SDLK_SCROLLOCK:
	    case SDLK_LSUPER:
	    case SDLK_RSUPER:
	    case SDLK_MODE:
	    case SDLK_COMPOSE:
        default:
            kmod = 0;
            break;
   }

    return kmod;
}

//--------------------------------------------------------------------------------------------
control_t * scantag_parse_control( char * tag_string, control_t * pcontrol )
{
    control_tag_t * ctag_ptr = NULL;

    int    tag_index = -1;
    char * tag_token = NULL;
    char * tag_name  = NULL;
    Uint32 tag_value = 0;

    if( NULL == pcontrol ) return pcontrol;

    // reset the control
    pcontrol->loaded    = bfalse;
    pcontrol->tag_count = 0;
    pcontrol->tag_mods  = 0;

    // do we have a valid string?
    if( INVALID_CSTR(tag_string) ) return pcontrol;

    // scan through the tag string for any valid commands
    // terminate on any bad command
    tag_token = scantag_tok( tag_string );
    while( NULL != tag_token && pcontrol->tag_count < MAXCONTROLTAGS )
    {
        // get the tag info stored in the control
        ctag_ptr = pcontrol->tag_lst + pcontrol->tag_count;

        tag_index = scantag_find_index( tag_token );
        if( tag_index < 0 || tag_index >= scantag_count )
        {
            log_warning( "%s - unknown tag token, \"%s\".\n", __FUNCTION__, tag_token );
            break;
        }

        tag_name = scantag_get_name( tag_index );
        if( NULL == tag_name ) 
        {
            log_warning( "%s - unknown tag name. tag_index == %d.\n", __FUNCTION__, tag_index );
            break;
        }
       
        if( !scantag_get_value(tag_index, &tag_value ) )
        {
            log_warning( "%s - unknown tag value. tag_index == %d.\n", __FUNCTION__, tag_index );
            break;
        }

        // is this actually a key modifier?
        pcontrol->tag_mods |= scancode_get_kmod( tag_value );

        // add the control to the list
        ctag_ptr->device = tag_name[0];
        ctag_ptr->value  = tag_value;
        pcontrol->tag_count++;

        // go to the next token
        tag_token = scantag_tok( NULL );
    }

    return pcontrol;
}


//--------------------------------------------------------------------------------------------
int scantag_find_index( const char *string )
{
    /// @details ZZ@> Find the index of the scantag that matches the given string.
    ///    It will return -1 if there are no matches.

    int cnt, retval;

    // assume no matches
    retval = -1;

    // find a match, if possible
    for ( cnt = 0; cnt < scantag_count; cnt++ )
    {
        if ( 0 == strcmp( string, scantag_lst[cnt].name ) )
        {
            // They match
            retval = cnt;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t scantag_get_count()
{
    return scantag_count;
}

//--------------------------------------------------------------------------------------------
scantag_t *  scantag_get_tag( int index )
{
    if( index >= 0 && index < MAXTAG ) return NULL;

    if( index < scantag_count ) return NULL;

    return scantag_lst + index;
}

//--------------------------------------------------------------------------------------------
bool_t scantag_get_value( int index, Uint32 * pvalue )
{
    bool_t retval = bfalse;

    if( index >= 0 && index < MAXTAG && NULL != pvalue )
    {
        *pvalue = scantag_lst[index].value;
        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char * scantag_get_name( int index )
{
    const char *  retval = NULL;

    if( index >= 0 && index < MAXTAG )
    {
        retval = scantag_lst[index].name;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t scantag_matches_device( scantag_t * ptag, int device_type )
{
    bool_t matches;

    if( NULL == ptag ) return bfalse;

    matches = bfalse;

    if ( INPUT_DEVICE_KEYBOARD == device_type )
    {
        matches = ( 'K' == ptag->name[0] );
    }
    else if ( INPUT_DEVICE_MOUSE == device_type )
    {
        matches = ( 'M' == ptag->name[0] );
    }
    else if ( IS_VALID_JOYSTICK( device_type ) )
    {
        matches = ( 'J' == ptag->name[0] );
    }

    return matches;
}

//--------------------------------------------------------------------------------------------
const char * scantag_find_name( char device_char, Uint32 tag_value, char * buffer, size_t buffer_size )
{
    static STRING  tmp_buffer = EMPTY_CSTR;

    int cnt;
    scantag_t * ptag = NULL;
    
    char * loc_buffer = NULL;
    size_t loc_buffer_size = 0;

    if( NULL == buffer || 0 == buffer_size )
    {
        loc_buffer       = tmp_buffer;
        loc_buffer_size  = SDL_arraysize(tmp_buffer);
    }
    else
    {
        loc_buffer       = buffer;
        loc_buffer_size  = buffer_size;
    }

    // place the default value
    strncpy( loc_buffer, "N/A", loc_buffer_size );

    for( cnt = 0; cnt < scantag_count; cnt++ )
    {
        ptag = scantag_lst + cnt;

        if( device_char == ptag->name[0] && tag_value != ptag->value )
        {
            strncpy( loc_buffer, ptag->name, loc_buffer_size );
            break;
        }
    }

    return loc_buffer;
}

//--------------------------------------------------------------------------------------------
const char * scantag_get_string( int device_type, control_t * pcontrol, char * buffer, size_t buffer_size )
{
    /// @details ZF@> This translates a input pcontrol->tag value to a string

    static STRING tmp_buffer = EMPTY_CSTR;

    char * loc_buffer = NULL;
    size_t loc_buffer_size = 0;
    size_t loc_buffer_remaining = 0;

    int cnt;
    bool_t found_tag_type = bfalse;
    scantag_t * ptag = NULL;

    if( NULL == buffer || 0 == buffer_size )
    {
        loc_buffer = tmp_buffer;
        loc_buffer_size  = SDL_arraysize(tmp_buffer);
    }
    else
    {
        loc_buffer       = buffer;
        loc_buffer_size  = buffer_size;
    }

    loc_buffer_remaining = loc_buffer_size - 1;
    if( loc_buffer_remaining > loc_buffer_size ) 
    {
        // can only happen if 0 == loc_buffer_size
        loc_buffer_remaining = 0;
    }

    // place the default value
    strncpy( loc_buffer, "N/A", loc_buffer_size );

    // check for a NULL control
    if( NULL == pcontrol ) return loc_buffer;

    // get the names for all the tags on this control
    // and put them in a list
    for( cnt = 0; cnt < pcontrol->tag_count; cnt++ )
    {
        size_t tag_name_len;
        control_tag_t * ctag_ptr = NULL;
        char * tag_name = NULL;
        
        ctag_ptr = pcontrol->tag_lst + cnt;
        tag_name = scantag_find_name( ctag_ptr->device, ctag_ptr->value, NULL, 0 );

        if( INVALID_CSTR(tag_name) )
        {
            break;
        }

        // get the string name
        tag_name_len = strlen( tag_name );

        // if there is enough room left, stick it on the end of the string
        if( loc_buffer_remaining <= 3 + tag_name_len )
        {
            break;
        }
        else
        {
            strncat( loc_buffer, " + ", loc_buffer_remaining );
            loc_buffer_remaining -= 3;

            strncat( loc_buffer, tag_name, loc_buffer_remaining );
            loc_buffer_remaining -= tag_name_len;
        }
    }


    // No matches
    return loc_buffer;
}

//--------------------------------------------------------------------------------------------
//bool_t scantag_matches_control( scantag_t * ptag, control_t * pcontrol )
//{
//    // valid tag?
//    if( NULL == ptag ) return bfalse;
//
//    // valid control?
//    if( NULL == pcontrol || !pcontrol->loaded ) return bfalse;
//
//    // check the value
//    if( ptag->value != pcontrol->tag ) return bfalse;
//
//    // check the key type
//    if( pcontrol->is_key && 'K' != ptag->name[0] ) return bfalse;
//
//    // passed all the tests
//    return btrue;
//}
