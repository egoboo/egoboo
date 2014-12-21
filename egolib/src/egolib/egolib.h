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

/// @file egolib/egolib.h
/// @brief All-in-one header file.

#pragma once

#include "egolib/typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "egolib/clock.h"
#include "egolib/console.h"
#include "egolib/endian.h"
#include "egolib/fileutil.h"
#include "egolib/_math.h"
#include "egolib/platform.h"
#include "egolib/process.h"
#include "egolib/state_machine.h"
#include "egolib/strutil.h"
#include "egolib/timer.h"
#include "egolib/vfs.h"
#include "egolib/egoboo_setup.h"
#include "egolib/font_bmp.h"
#include "egolib/font_ttf.h"
#include "egolib/system.h"
#include "egolib/clock.h"
#include "egolib/file_common.h"
#include "egolib/log.h"
#include "egolib/hash.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "egolib/extensions/ogl_debug.h"
#include "egolib/extensions/ogl_extensions.h"
#include "egolib/extensions/ogl_include.h"
#include "egolib/extensions/ogl_texture.h"
#include "egolib/extensions/SDL_extensions.h"
#include "egolib/extensions/SDL_GL_extensions.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "egolib/file_formats/cap_file.h"
#include "egolib/file_formats/configfile.h"
#include "egolib/file_formats/controls_file.h"
#include "egolib/file_formats/eve_file.h"
#include "egolib/file_formats/id_md2.h"
#include "egolib/file_formats/module_file.h"
#include "egolib/file_formats/map_file.h"
#include "egolib/file_formats/passage_file.h"
#include "egolib/file_formats/pip_file.h"
#include "egolib/file_formats/quest_file.h"
#include "egolib/file_formats/scancode_file.h"
#include "egolib/file_formats/spawn_file.h"
#include "egolib/file_formats/template.h"
#include "egolib/file_formats/treasure_table_file.h"
#include "egolib/file_formats/wawalite_file.h"

#if defined(USE_LUA_CONSOLE)
	#include "egolib/lua/lua_console.h"
#endif

//!!! watch this one. if you are using the Fluid Studios memory manager,
//!!! this must be the absolute last include file.
#include "egolib/mem.h"
