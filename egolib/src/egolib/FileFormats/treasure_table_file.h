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

/// @file egolib/FileFormats/treasure_table_file.h

#pragma once

#include "egolib/typedef.h"
#include "egolib/fileutil.h"

constexpr size_t MAX_TABLES = 32;           ///< Maximum number of tables.
constexpr size_t TREASURE_TABLE_SIZE = 128; ///< Maximum number of objects per table.

///Data structure for one treasure table, we can have up to MAX_TABLES of these
struct treasure_table_t
{
	/// @brief The name of this treasure table.
    STRING table_name;
	/// @brief List of treasure objects in this treasure table
    STRING object_list[TREASURE_TABLE_SIZE];
	/// @brief The size of the list
    size_t size;
	/// @brief Adds a new treasure object to the specified treasure table
	/// @param self the treasure table
	/// @param name the name of the object
	void add(const std::string& name);
	treasure_table_t();
};

/// @brief Reload the treasure table from the specified read context.
void load_one_treasure_table_vfs(ReadContext& ctxt, treasure_table_t& newTable);
