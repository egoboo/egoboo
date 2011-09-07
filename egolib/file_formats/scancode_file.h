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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file file_formats/scancode_file.h
/// @details routines for reading and writing the file "scancode.txt"

#include "../typedef.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_control;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_scantag;
    typedef struct s_scantag scantag_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Key/Control input definitions
#define MAXTAG              256                     ///< Number of tags in scancode.txt
#define TAGSIZE              32                     ///< Size of each tag

    typedef char TAG_STRING[TAGSIZE];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A mapping between the state of an input device and an internal game latch
    struct s_scantag
    {
        TAG_STRING name;             ///< tag name
        Uint32     value;            ///< tag value
    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
    void         scantag_read_all_vfs( const char *szFilename );

    size_t       scantag_get_count( void );
    int          scantag_find_index( const char *string );
    scantag_t *  scantag_get_tag( int index );
    bool_t       scantag_get_value( int index, Uint32 * pvalue );
    const char * scantag_get_name( int index );

    const char       * scantag_get_string( int device, struct s_control * pcontrol, char * buffer, size_t buffer_size );
    struct s_control * scantag_parse_control( char * tag_string, struct s_control * pcontrol );

    Uint32             scancode_get_kmod( Uint32 scancode );

    scantag_t * scantag_find_bits( scantag_t * ptag_src, char device_char, Uint32 tag_bits );
    scantag_t * scantag_find_value( scantag_t * ptag_src, char device_char, Uint32 tag_value );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _file_formats_scancode_file_h
