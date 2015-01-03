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

/// @file  egolib/file_formats/controls_file.h
/// @brief Functions and data for reading and writing the file <tt>"controls.txt"</tt> and <tt>"scancode.txt"</tt>.

#pragma once

#include "egolib/typedef.h"
#include "egolib/vfs.h"
#include "egolib/input_device.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_control;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define CURRENT_CONTROLS_FILE_VERSION 2

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool input_settings_load_vfs(const char *szFilename, int version);
bool input_settings_save_vfs(const char* szFilename, int version);

    void export_control( vfs_FILE * filewrite, const char * text, int device, struct s_control * pcontrol );
