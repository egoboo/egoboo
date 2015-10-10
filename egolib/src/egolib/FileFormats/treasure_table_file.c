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

/// @file egolib/FileFormats/treasure_table_file.c
/// @brief Implementation of code for handling random treasure generation
/// @details

#include "egolib/FileFormats/treasure_table_file.h"

#include "egolib/fileutil.h"
#include "egolib/strutil.h"

#include "egolib/_math.h"      //For randomization

//--------------------------------------------------------------------------------------------
treasure_table_t::treasure_table_t()
	: table_name(""), size(0)
{}

void treasure_table_t::add( treasure_table_t *self, const char *name )
{
    //ZF> Adds a new treasure object to the specified treasure table

    //Avoid null pointers
    if ( NULL == self ) return;

    //Make sure there is enough size to add one more
    if (self->size + 1 >= TREASURE_TABLE_SIZE )
    {
        log_warning( "No more room to add object (%s) to table, consider increasing TREASURE_TABLE_SIZE (currently %i)\n", name, TREASURE_TABLE_SIZE );
        return;
    }

    //Add the element to the list
    strncpy(self->object_list[self->size ], name, SDL_arraysize(self->object_list[self->size ] ) );
	self->size++;
}

//--------------------------------------------------------------------------------------------
void load_one_treasure_table_vfs(ReadContext& ctxt, treasure_table_t* new_table )
{
    //ZF> Creates and loads a treasure table from the specified file until a :END is encountered
    new_table->size = 0;

    //Keep adding objects into the table until we encounter a :END
    while (ctxt.skipToColon(false))
    {
        STRING temporary;
        // We need to distinguish between regular names and references starting with '%'.
        ctxt.skipWhiteSpaces();
        if (ctxt.is('%'))
        {
            ctxt.next();
            temporary[0] = '%';
            vfs_read_name(ctxt, temporary + 1, SDL_arraysize(temporary) - 1);
        }
        else
        {
            vfs_read_name(ctxt, temporary, SDL_arraysize(temporary));
        }


        //Check if we reached the end of this table
        if ( 0 == strcmp( temporary, "END" ) ) break;

        //Nope, add one more to the table
        treasure_table_t::add( new_table, temporary);
    }
}

//--------------------------------------------------------------------------------------------
