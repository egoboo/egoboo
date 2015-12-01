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
    : entries(), size(0), name()
{ }

void treasure_table_t::add(const std::string& name)
{
    // Make sure there is enough size to add one more.
    if (size >= TREASURE_TABLE_SIZE)
    {
        Log::Entry e(Log::Level::Warning, __FILE__, __LINE__, __FUNCTION__);
        e << "no more room to add object `" << name << "` to treasure table. Consider inreasing the treasure table capacity."
          << Log::EndOfEntry;
        Log::get() << e;
        return;
    }

    // Add the element to the list.
    entries[size++] = name;
}
