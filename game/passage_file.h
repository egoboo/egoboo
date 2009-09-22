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

/* Egoboo - passage_file.h
 * help read/write the passage file
 */

#include "egoboo_typedef.h"

#include "egoboo_vfs.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define NO_MUSIC -1            // For passages that play no music

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_passage
{
    // Passage positions
    irect_t area;
    Sint8   music;                // Music track appointed to the specific passage
    Uint8   mask;                 // Is it IMPASSABLE, SLIPPERY or whatever
    bool_t  open;                 // Is the passage open?
};

typedef struct s_passage passage_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t scan_passage_file( vfs_FILE * fileread, passage_t * ppass  );