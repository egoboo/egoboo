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
#pragma once
#include <cstddef>
#include <cstdint>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// This is for random naming

#define CHOPPERMODEL                    32
#define MAXCHOP                         (256*CHOPPERMODEL)
#define CHOPSIZE                        8
#define CHOPDATACHUNK                   (MAXCHOP*CHOPSIZE)
#define MAXSECTION                      4              ///< T-wi-n-k...  Most of 4 sections

/// The buffer for the random naming data
struct chop_data_t
{
    size_t    chop_count;             ///< The global number of name parts

    uint32_t  carat;                  ///< The data pointer
    char      buffer[CHOPDATACHUNK];  ///< The name parts
    int       start[MAXCHOP];         ///< The first character of each part
};

chop_data_t * chop_data_init( chop_data_t * pdata );

bool        chop_export_vfs( const char *szSaveName, const char * szChop );

//--------------------------------------------------------------------------------------------

/// Defintion of a single chop section
struct chop_section_t
{
    int size;     ///< Number of choices, 0
    int start;    ///< A reference to a specific offset in the chop_data_t buffer
};

//--------------------------------------------------------------------------------------------

/// Defintion of the chop info needed to create a name
struct chop_definition_t
{
    chop_section_t  section[MAXSECTION];
};

//Globals
extern chop_data_t chop_mem;

//--------------------------------------------------------------------------------------------

//Function prototypes
chop_definition_t * chop_definition_init(chop_definition_t * pdefinition);
const char *  chop_create( chop_data_t * pdata, chop_definition_t * pdef );
bool        chop_load_vfs( chop_data_t * pchop_data, const char *szLoadname, chop_definition_t * pchop_definition );
