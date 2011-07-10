#pragma once

//********************************************************************************************
//*
//*    This file is part of the SDL extensions library.
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

/// @file egolib.h
/// @brief all-in-one header file

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "egoboo_typedef.h"
#include "egoboo_clock.h"
#include "egoboo_config.h"
#include "egoboo_console.h"
#include "egoboo_endian.h"
#include "egoboo_fileutil.h"
#include "egoboo_frustum.h"
#include "egoboo_math.h"
#include "egoboo_platform.h"
#include "egoboo_process.h"
#include "egoboo_state_machine.h"
#include "egoboo_strutil.h"
#include "egoboo_timer.h"
#include "egoboo_vfs.h"
#include "egoboo_setup.h"

#include "font_bmp.h"
#include "font_ttf.h"
#include "lua_console.h"
#include "system.h"
#include "clock.h"
#include "file_common.h"
#include "log.h"
#include "hash.h"

#include "extensions/ogl_debug.h"
#include "extensions/ogl_extensions.h"
#include "extensions/ogl_include.h"
#include "extensions/ogl_texture.h"
#include "extensions/SDL_extensions.h"
#include "extensions/SDL_GL_extensions.h"

#include "file_formats/cap_file.h"
#include "file_formats/configfile.h"
#include "file_formats/controls_file.h"
#include "file_formats/eve_file.h"
#include "file_formats/id_md2.h"
#include "file_formats/module_file.h"
#include "file_formats/mpd_file.h"
#include "file_formats/passage_file.h"
#include "file_formats/pip_file.h"
#include "file_formats/quest_file.h"
#include "file_formats/scancode_file.h"
#include "file_formats/spawn_file.h"
#include "file_formats/template.h"
#include "file_formats/treasure_table_file.h"
#include "file_formats/wawalite_file.h"

//!!! watch this one. if you are using the Fluid Studios memory manager,
//!!! this must be the absolute last include file.
#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egolib_h