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

/// @file  egolib/FileFormats/map_file-v3.h
/// @brief Load and save version 3 ("MapC") files
/// @details

#pragma once

#include "egolib/FileFormats/map_file.h"

/// Load a map
bool map_read_v3(vfs_FILE& file, map_t& map);
/// Save a map
bool map_write_v3(vfs_FILE& file, const map_t& map);
