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

/// @file egolib/file_formats/passage_file.h
/// @details help read/write the passage file

#pragma once

#include "egolib/typedef.h"
#include "egolib/vfs.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_passage;
    typedef struct s_passage passage_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define NO_MUSIC -1            ///< For passages that play no music

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The definition of an "active area" in the game
/// @details Used for a variety of purposes, including doors and shops.
struct s_passage
{
    // Passage positions
    irect_t area;
    int music;   ///< Music track appointed to the specific passage
    Uint8 mask;  ///< Is it IMPASSABLE, SLIPPERY or whatever
    bool open;   ///< Is the passage open?
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
    bool scan_passage_file( vfs_FILE * fileread, passage_t * ppass );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define  _passage_file_h
