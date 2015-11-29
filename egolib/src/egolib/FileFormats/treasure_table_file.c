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
#include "egolib/_math.h"      // For randomization

treasure_table_t::treasure_table_t()
    : size(0)
{
    strcpy(table_name, "");
}

void treasure_table_t::add(const std::string& name)
{
    // Make sure there is enough size to add one more.
    if (size + 1 >= TREASURE_TABLE_SIZE)
    {
        Log::Entry e(Log::Level::Warning, __FILE__, __LINE__, __FUNCTION__);
        e << "no more room to add object `" << name << "` to treasure table. Consider inreasing the treasure table capacity."
          << Log::EndOfEntry;
        Log::get() << e;
        return;
    }

    // Add the element to the list.
    strncpy(object_list[size], name.c_str(), SDL_arraysize(object_list[size]));
    size++;
}

void load_one_treasure_table_vfs(ReadContext& ctxt, treasure_table_t& newTable)
{
    newTable.size = 0;

    // Keep adding objects into the table until we encounter a ':END'.
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


        // Check if we reached the end of this table.
        if (0 == strcmp(temporary, "END")) break;

        // Nope, add one more to the table.
        newTable.add(temporary);
    }
}
