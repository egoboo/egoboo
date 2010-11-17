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

#include "egoboo_typedef.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Key/Control input definitions
#define MAXTAG              256                     ///< Number of tags in scancode.txt
#define TAGSIZE              32                     ///< Size of each tag

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A mapping between the state of an input device and an internal game latch
    struct s_scantag
    {
        char   name[TAGSIZE];             ///< Scancode names
        Uint32 value;                     ///< Scancode values
    };
    typedef struct s_scantag scantag_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    extern int       scantag_count;
    extern scantag_t scantag[MAXTAG];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    void        scantag_read_all_vfs( const char *szFilename );
    int         scantag_get_value( const char *string );
    const char* scantag_get_string( Sint32 device, Uint32 tag, bool_t onlykeys );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _scancode_file_h
