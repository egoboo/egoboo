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

/// @file quest.h
/// @brief read/write/modify the quest.txt file

#include "egoboo_typedef.h"

/// Quest system
#define QUEST_BEATEN         -1
#define QUEST_NONE           -2

bool_t quest_add_idsz_vfs( const char *player_directory, const IDSZ idsz );
int quest_modify_idsz_vfs( const char *player_directory, const IDSZ idsz, const int adjustment );
int quest_check_vfs( const char *player_directory, const IDSZ idsz );
