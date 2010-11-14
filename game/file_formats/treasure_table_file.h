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

/// @file treasure_tables.h
#include "egoboo_typedef.h"
#include "char.h"

#define MAX_TABLES				32				//< Max number of tables
#define TREASURE_TABLE_SIZE		128				//< Max number of objects per table

//Data structure for one treasure table, we can have up to MAX_TABLES of these
struct s_treasure_table 
{
	STRING table_name;							//< What is the name of this treasure table
	STRING object_list[TREASURE_TABLE_SIZE];	//< List of treasure objects in this table
	size_t size;								//< Number of objects loaded into this table
};
typedef struct s_treasure_table treasure_table_t;

//Global variables
//ZF> TODO: This should probably be moved into a game.c data structure or something like that, we should also implement
//    so that any local module can override the default randomtreasure.txt found in basicdat folder
static treasure_table_t treasureTableList[MAX_TABLES];

//Public functions
egoboo_rv init_random_treasure_tables_vfs( const char* filepath );
void get_random_treasure( char* pStr );

#define TREASURE_TABLES_H

