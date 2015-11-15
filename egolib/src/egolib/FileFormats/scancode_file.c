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

/// @file egolib/FileFormats/scancode_file.c
/// @brief Functions to read and write Egoboo's basicdat/scantag.txt file
/// @details

#include "egolib/FileFormats/scancode_file.h"

#include "egolib/Core/StringUtilities.hpp"

#include "egolib/log.h"
#include "egolib/input_device.h"

#include "egolib/vfs.h"
#include "egolib/fileutil.h"
#include "egolib/strutil.h"
#include "egolib/platform.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static const scantag_t scantag_lst[] = {
    // key scancodes
    {"KEY_BACK_SPACE", SDLK_BACKSPACE},
    {"KEY_TAB", SDLK_TAB},
    {"KEY_RETURN", SDLK_RETURN},
    {"KEY_ESCAPE", SDLK_ESCAPE},
    {"KEY_SPACE", SDLK_SPACE},
    {"KEY_APOSTROPHE", SDLK_QUOTEDBL},
    {"KEY_GRAVE", SDLK_RIGHTPAREN},
    {"KEY_COMMA", SDLK_COMMA},
    {"KEY_MINUS", SDLK_MINUS},
    {"KEY_PERIOD", SDLK_PERIOD},
    {"KEY_SLASH", SDLK_SLASH},
    {"KEY_0", SDLK_0},
    {"KEY_1", SDLK_1},
    {"KEY_2", SDLK_2},
    {"KEY_3", SDLK_3},
    {"KEY_4", SDLK_4},
    {"KEY_5", SDLK_5},
    {"KEY_6", SDLK_6},
    {"KEY_7", SDLK_7},
    {"KEY_8", SDLK_8},
    {"KEY_9", SDLK_9},
    {"KEY_SEMICOLON", SDLK_SEMICOLON},
    {"KEY_EQUALS", SDLK_EQUALS},
    {"KEY_LEFT_BRACKET", SDLK_LEFTBRACKET},
    {"KEY_BACKSLASH", SDLK_BACKSLASH},
    {"KEY_RIGHT_BRACKET", SDLK_RIGHTBRACKET},
    {"KEY_A", SDLK_a},
    {"KEY_B", SDLK_b},
    {"KEY_C", SDLK_c},
    {"KEY_D", SDLK_d},
    {"KEY_E", SDLK_e},
    {"KEY_F", SDLK_f},
    {"KEY_G", SDLK_g},
    {"KEY_H", SDLK_h},
    {"KEY_I", SDLK_i},
    {"KEY_J", SDLK_j},
    {"KEY_K", SDLK_k},
    {"KEY_L", SDLK_l},
    {"KEY_M", SDLK_m},
    {"KEY_N", SDLK_n},
    {"KEY_O", SDLK_o},
    {"KEY_P", SDLK_p},
    {"KEY_Q", SDLK_q},
    {"KEY_R", SDLK_r},
    {"KEY_S", SDLK_s},
    {"KEY_T", SDLK_t},
    {"KEY_U", SDLK_u},
    {"KEY_V", SDLK_v},
    {"KEY_W", SDLK_w},
    {"KEY_X", SDLK_x},
    {"KEY_Y", SDLK_y},
    {"KEY_Z", SDLK_z},
    {"KEY_DELETE", SDLK_DELETE},
    {"KEY_PAD_0", SDLK_KP_0},
    {"KEY_PAD_1", SDLK_KP_1},
    {"KEY_PAD_2", SDLK_KP_2},
    {"KEY_PAD_3", SDLK_KP_3},
    {"KEY_PAD_4", SDLK_KP_4},
    {"KEY_PAD_5", SDLK_KP_5},
    {"KEY_PAD_6", SDLK_KP_6},
    {"KEY_PAD_7", SDLK_KP_7},
    {"KEY_PAD_8", SDLK_KP_8},
    {"KEY_PAD_9", SDLK_KP_9},
    {"KEY_PAD_PERIOD", SDLK_KP_PERIOD},
    {"KEY_PAD_SLASH", SDLK_KP_DIVIDE},
    {"KEY_PAD_ASTERISK", SDLK_KP_MULTIPLY},
    {"KEY_PAD_MINUS", SDLK_KP_MINUS},
    {"KEY_PAD_PLUS", SDLK_KP_PLUS},
    {"KEY_ENTER", SDLK_KP_ENTER},
    {"KEY_PAD_ENTER", SDLK_KP_ENTER},
    {"KEY_UP", SDLK_UP},
    {"KEY_DOWN", SDLK_DOWN},
    {"KEY_RIGHT", SDLK_RIGHT},
    {"KEY_LEFT", SDLK_LEFT},
    {"KEY_INSERT", SDLK_INSERT},
    {"KEY_HOME", SDLK_HOME},
    {"KEY_END", SDLK_END},
    {"KEY_PAGE_UP", SDLK_PAGEUP},
    {"KEY_PAGE_DOWN", SDLK_PAGEDOWN},
    {"KEY_F1", SDLK_F1},
    {"KEY_F2", SDLK_F2},
    {"KEY_F3", SDLK_F3},
    {"KEY_F4", SDLK_F4},
    {"KEY_F5", SDLK_F5},
    {"KEY_F6", SDLK_F6},
    {"KEY_F7", SDLK_F7},
    {"KEY_F8", SDLK_F8},
    {"KEY_F9", SDLK_F9},
    {"KEY_F10", SDLK_F10},
    {"KEY_F11", SDLK_F11},
    {"KEY_F12", SDLK_F12},
    {"KEY_F13", SDLK_F13},
    {"KEY_F14", SDLK_F14},
    {"KEY_F15", SDLK_F15},
    
    // key modifiers
    {"KEY_NUM_LOCK", SDLK_NUMLOCKCLEAR},
    {"KEY_CAPS_LOCK", SDLK_CAPSLOCK},
    {"KEY_SCROLL_LOCK", SDLK_SCROLLLOCK},
    {"KEY_RIGHT_SHIFT", SDLK_RSHIFT},
    {"KEY_LEFT_SHIFT", SDLK_LSHIFT},
    {"KEY_RIGHT_CONTROL", SDLK_RCTRL},
    {"KEY_LEFT_CONTROL", SDLK_LCTRL},
    {"KEY_RIGHT_ALT", SDLK_RALT},
    {"KEY_LEFT_ALT", SDLK_LALT},
    {"KEY_RIGHT_META", SDLK_RGUI},
    {"KEY_LEFT_META", SDLK_LGUI},
    {"KEY_RIGHT_SUPER", SDLK_LGUI},
    {"KEY_LEFT_SUPER", SDLK_RGUI},
    
    // mouse button bits
    {"MOS_LEFT", 1},
    {"MOS_RIGHT", 2},
    {"MOS_MIDDLE", 4},
    {"MOS_FOURTH", 8},
    {"MOS_LEFT_AND_RIGHT", 3},
    {"MOS_LEFT_AND_MIDDLE", 5},
    {"MOS_RIGHT_AND_MIDDLE", 6},
    
    // joy button bits
    {"JOY_0", 1 << 0},
    {"JOY_1", 1 << 1},
    {"JOY_2", 1 << 2},
    {"JOY_3", 1 << 3},
    {"JOY_4", 1 << 4},
    {"JOY_5", 1 << 5},
    {"JOY_6", 1 << 6},
    {"JOY_7", 1 << 7},
    {"JOY_8", 1 << 8},
    {"JOY_9", 1 << 9},
    {"JOY_10", 1 << 10},
    {"JOY_11", 1 << 11},
    {"JOY_12", 1 << 12},
    {"JOY_13", 1 << 13},
    {"JOY_14", 1 << 14},
    {"JOY_15", 1 << 15},
    {"JOY_16", 1 << 16},
    {"JOY_17", 1 << 17},
    {"JOY_18", 1 << 18},
    {"JOY_19", 1 << 19},
    {"JOY_20", 1 << 20},
    {"JOY_21", 1 << 21},
    {"JOY_22", 1 << 22},
    {"JOY_23", 1 << 23},
    {"JOY_24", 1 << 24},
    {"JOY_25", 1 << 25},
    {"JOY_26", 1 << 26},
    {"JOY_27", 1 << 27},
    {"JOY_28", 1 << 28},
    {"JOY_29", 1 << 29},
    {"JOY_30", 1 << 30},
    {"JOY_31", 1u << 31},
    {"JOY_0_AND_1", 3},
    {"JOY_0_AND_2", 5},
    {"JOY_0_AND_3", 9},
    {"JOY_1_AND_2", 6},
    {"JOY_1_AND_3", 10},
    {"JOY_2_AND_3", 12},
};
static const size_t scantag_count = sizeof(scantag_lst) / sizeof(scantag_lst[0]);

//static bool scantag_matches_control( scantag_t * ptag, control_t * pcontrol );
//static bool scantag_matches_device( scantag_t * ptag, int device_type );

//--------------------------------------------------------------------------------------------
Uint32 scancode_get_kmod( Uint32 scancode )
{
    
    Uint32 kmod = 0;
    
    switch ( scancode )
    {
        case SDLK_NUMLOCKCLEAR:  kmod = KMOD_NUM;    break;
        case SDLK_CAPSLOCK: kmod = KMOD_CAPS;   break;
        case SDLK_LSHIFT:   kmod = KMOD_LSHIFT; break;
        case SDLK_RSHIFT:   kmod = KMOD_RSHIFT; break;
        case SDLK_LCTRL:    kmod = KMOD_LCTRL;  break;
        case SDLK_RCTRL:    kmod = KMOD_RCTRL;  break;
        case SDLK_LALT:     kmod = KMOD_LALT;   break;
        case SDLK_RALT:     kmod = KMOD_RALT;   break;
        case SDLK_LGUI:     kmod = KMOD_LGUI;  break;
        case SDLK_RGUI:     kmod = KMOD_RGUI;  break;
        case SDLK_MODE:     kmod = KMOD_MODE;  break;
            
            // unhandled cases
        case SDLK_SCROLLLOCK:
        default:
            kmod = 0;
            break;
    }
    
    return kmod;
}

//--------------------------------------------------------------------------------------------
void scantag_parse_control( const char * tag_string, control_t &pcontrol )
{
    std::string delimiters = " ,|+&\t\n";
    int    tag_index = -1;
    const char * tag_token = NULL;
    const char * tag_name  = NULL;
    Uint32 tag_value = 0;

    // reset the control
    pcontrol.clear();

    // do we have a valid string?
    if ( INVALID_CSTR( tag_string ) ) return;

    // scan through the tag string for any valid commands
    // terminate on any bad command
    std::vector<std::string> tokens = Ego::split(std::string(tag_string), delimiters);
    for (const auto &token : tokens)
    {
        if (delimiters.find(token) != std::string::npos) continue;
        
        tag_token = token.c_str();
        tag_index = scantag_find_index( tag_token );
        if ( tag_index < 0 || tag_index >= scantag_count )
        {
            if(token != "N/A") {
				Log::get().warn("%s - unknown tag token, \"%s\".\n", __FUNCTION__, tag_token);
            }
            break;
        }

        tag_name = scantag_get_name( tag_index );
        if ( NULL == tag_name )
        {
            break;
        }

        if ( !scantag_get_value( tag_index, &tag_value ) )
        {
            break;
        }
			Log::get().warn("%s - unknown tag name. tag_index == %d.\n", __FUNCTION__, tag_index);
			Log::get().warn("%s - unknown tag value. tag_index == %d.\n", __FUNCTION__, tag_index);

        if ( 'K' == tag_name[0] )
        {
            // add the key control to the list
            pcontrol.mappedKeys.push_front(tag_value);

            // add in any key modifiers
            pcontrol.tag_key_mods |= scancode_get_kmod( tag_value );

            pcontrol.loaded = true;
        }
        else
        {
            pcontrol.tag_bits |= tag_value;

            pcontrol.loaded = true;
        }
    }
}

//--------------------------------------------------------------------------------------------
int scantag_find_index( const char *string )
{
    /// @author ZZ
    /// @details Find the index of the scantag that matches the given string.
    ///    It will return -1 if there are no matches.

    int retval;

    // assume no matches
    retval = -1;

    // find a match, if possible
    for (size_t cnt = 0; cnt < scantag_count; cnt++ )
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
size_t scantag_get_count( void )
{
    return scantag_count;
}

//--------------------------------------------------------------------------------------------
const scantag_t *  scantag_get_tag( int index )
{
    if ( index >= 0 && index < scantag_count ) return NULL;

    return scantag_lst + index;
}

//--------------------------------------------------------------------------------------------
bool scantag_get_value( int index, Uint32 * pvalue )
{
    bool retval = false;

    if ( index >= 0 && index < scantag_count && NULL != pvalue )
    {
        *pvalue = scantag_lst[index].value;
        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char * scantag_get_name( int index )
{
    const char *  retval = NULL;

    if ( index >= 0 && index < scantag_count )
    {
        retval = scantag_lst[index].name;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
const scantag_t * scantag_find_bits( const scantag_t * ptag_src, char device_char, Uint32 tag_bits )
{
    const scantag_t * retval = NULL;

    const scantag_t * ptag     = NULL;
    const scantag_t * ptag_stt = NULL;
    const scantag_t * ptag_end = NULL;

    // get the correct start position in the list
    if ( NULL == ptag_src )
    {
        ptag_stt = scantag_lst + 0;
        ptag_end = scantag_lst + scantag_count;
    }
    else
    {
        ptag_stt = ptag_src;
        ptag_end = scantag_lst + scantag_count;
    }

    // check for an ending condition
    if ( ptag_stt < scantag_lst || ptag_stt >= ptag_end )
    {
        return NULL;
    }

    for ( ptag = ptag_stt; ptag < ptag_end; ptag++ )
    {
        if ( device_char != ptag->name[0] ) continue;

        if ( HAS_ALL_BITS( tag_bits, ptag->value ) )
        {
            retval = ptag;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
const scantag_t * scantag_find_value( const scantag_t * ptag_src, char device_char, Uint32 tag_value )
{
    const scantag_t * retval = NULL;

    const scantag_t * ptag     = NULL;
    const scantag_t * ptag_stt = NULL;
    const scantag_t * ptag_end = NULL;

    // get the correct start position in the list
    if ( NULL == ptag_src )
    {
        ptag_stt = scantag_lst + 0;
        ptag_end = scantag_lst + scantag_count;
    }
    else
    {
        ptag_stt = ptag_src;
        ptag_end = scantag_lst + scantag_count;
    }

    // check for an ending condition
    if ( ptag_stt < scantag_lst || ptag_stt >= ptag_end )
    {
        return NULL;
    }

    for ( ptag = ptag_stt; ptag < ptag_end; ptag++ )
    {
        if ( device_char != ptag->name[0] ) continue;

        if ( tag_value == ptag->value )
        {
            retval = ptag;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
char * buffer_append_str( char * buffer_ptr, const char * buffer_end, const char * str_new )
{
    size_t buffer_size = 0;
    size_t str_new_len = 0;

    // is there a buffer
    if ( NULL == buffer_ptr || NULL == buffer_end ) return NULL;

    // is the buffer empty?
    if ( buffer_ptr >= buffer_end ) return ( char * )buffer_end;

    // is there any string to append?
    if ( !VALID_CSTR( str_new ) ) return buffer_ptr;

    // get the string length
    str_new_len = strlen( str_new );

    // append the string
    strncat( buffer_ptr, str_new, buffer_size );

    // if there is enough room left, stick it on the end of the string
    if ( buffer_ptr + str_new_len >= buffer_end )
    {
        buffer_ptr = ( char * )buffer_end;
    }
    else
    {
        buffer_ptr += str_new_len;
    }

    // terminate the string if possible
    if ( buffer_ptr < buffer_end )
    {
        *buffer_ptr = CSTR_END;
    }

    return buffer_ptr;
}

//--------------------------------------------------------------------------------------------
const char * scantag_get_string( int device_type, const control_t &pcontrol, char * buffer, size_t buffer_size )
{
    /// @author ZF
    /// @details This translates a input pcontrol->tag value to a string

    static STRING tmp_buffer = EMPTY_CSTR;

    char      device_char;

    char * loc_buffer_stt = NULL;
    char * loc_buffer_end = NULL;
    char * loc_buffer_ptr = NULL;
    size_t loc_buffer_size_remaining = 0;

    size_t tag_count = 0;
    size_t tag_name_len = 0;
    const char * tag_name = NULL;

    const scantag_t * ptag = NULL;

    if ( NULL == buffer || 0 == buffer_size )
    {
        loc_buffer_stt = tmp_buffer;
        loc_buffer_end = tmp_buffer + SDL_arraysize( tmp_buffer );
    }
    else
    {
        loc_buffer_stt = buffer;
        loc_buffer_end = buffer + buffer_size;
    }
    loc_buffer_ptr = loc_buffer_stt;

    // do we have a valid buffer?
    if ( loc_buffer_stt >= loc_buffer_end ) return loc_buffer_stt;
    loc_buffer_size_remaining = loc_buffer_end - loc_buffer_stt;

    // no tags yet
    tag_count = 0;

    // blank out the buffer
    loc_buffer_stt[0] = CSTR_END;

    // no tag decoded yet
    tag_name = NULL;
    tag_name_len = 0;

    device_char = get_device_char_from_device_type( device_type );

    // find all the tags for the bits
    if ( pcontrol.tag_bits.any() )
    {
        unsigned long tag_bits = pcontrol.tag_bits.to_ulong();
        ptag = scantag_find_bits( NULL, device_char, tag_bits );
        while ( NULL != ptag )
        {
            // get the string name
            tag_name = ptag->name;
            tag_name_len = strlen( ptag->name );

            // remove the tag bits from the control bits
            // so that similar tag bits are not counted multiple times.
            // example: "joy_0 + joy_1" should not be decoded as "joy_0 + joy_1 + joy_0_and_1"
            tag_bits &= ~( ptag->value );

            // append the string
            if ( 0 == tag_name_len )
            {
                /* do nothing */
            }
            else if ( loc_buffer_ptr + tag_name_len > loc_buffer_end )
            {
                // we're too full
                goto scantag_get_string_end;
            }
            else if ( 0 == tag_count )
            {
                strncat( loc_buffer_ptr, tag_name, loc_buffer_end - loc_buffer_ptr - 1 );
                loc_buffer_ptr += tag_name_len;

                tag_count++;
            }
            else
            {
                strncat( loc_buffer_ptr, " + ", loc_buffer_end - loc_buffer_ptr - 1 );
                loc_buffer_ptr += 3;

                strncat( loc_buffer_ptr, tag_name, loc_buffer_end - loc_buffer_ptr - 1 );
                loc_buffer_ptr += tag_name_len;

                tag_count++;
            }

            ptag = scantag_find_bits( ptag + 1, device_char, tag_bits );
        }
    }

    // get the names for all the keyboard tags on this control
    // and put them in a list
    if ( !pcontrol.mappedKeys.empty() )
    {
        for(uint32_t keycode : pcontrol.mappedKeys)
        {
            ptag = scantag_find_value( NULL, 'K', keycode );
            if ( NULL == ptag ) continue;

            // get the string name
            tag_name = ptag->name;
            tag_name_len = strlen( ptag->name );

            // append the string
            if ( 0 == tag_name_len )
            {
                /* do nothing */
            }
            else if ( loc_buffer_ptr + tag_name_len > loc_buffer_end )
            {
                // we're too full
                goto scantag_get_string_end;
            }
            else if ( 0 == tag_count )
            {
                strncat( loc_buffer_ptr, tag_name, loc_buffer_end - loc_buffer_ptr - 1 );
                loc_buffer_ptr += tag_name_len;

                tag_count++;
            }
            else
            {
                strncat( loc_buffer_ptr, " + ", loc_buffer_end - loc_buffer_ptr - 1 );
                loc_buffer_ptr += 3;

                strncat( loc_buffer_ptr, tag_name, loc_buffer_end - loc_buffer_ptr - 1 );
                loc_buffer_ptr += tag_name_len;

                tag_count++;
            }
        }
    }

scantag_get_string_end:

    if ( 0 == tag_count )
    {
        strncpy( loc_buffer_stt, "N/A", loc_buffer_size_remaining );
    }

    // return the result
    return loc_buffer_stt;
}

//--------------------------------------------------------------------------------------------
//bool scantag_matches_control( scantag_t * ptag, control_t * pcontrol )
//{
//    // valid tag?
//    if( NULL == ptag ) return false;
//
//    // valid control?
//    if( NULL == pcontrol || !pcontrol->loaded ) return false;
//
//    // check the value
//    if( ptag->value != pcontrol->tag ) return false;
//
//    // check the key type
//    if( pcontrol->is_key && 'K' != ptag->name[0] ) return false;
//
//    // passed all the tests
//    return true;
//}

//--------------------------------------------------------------------------------------------
//bool scantag_matches_device( scantag_t * ptag, int device_type )
//{
//    bool matches;
//
//    if( NULL == ptag ) return false;
//
//    matches = false;
//
//    if ( INPUT_DEVICE_KEYBOARD == device_type )
//    {
//        matches = ( 'K' == ptag->name[0] );
//    }
//    else if ( INPUT_DEVICE_MOUSE == device_type )
//    {
//        matches = ( 'M' == ptag->name[0] );
//    }
//    else if ( IS_VALID_JOYSTICK( device_type ) )
//    {
//        matches = ( 'J' == ptag->name[0] );
//    }
//
//    return matches;
//}
