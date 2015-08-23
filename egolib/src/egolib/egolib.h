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

/// @file  egolib/egolib.h
/// @brief All-in-one header file.

#pragma once

#include "egolib/Audio/AudioSystem.hpp"
#include "egolib/Core/StringUtilities.hpp"
#include "egolib/Core/CollectionUtilities.hpp"
#include "egolib/Core/System.hpp"
#include "egolib/Core/EnvironmentError.hpp"
#include "egolib/Core/UnhandledSwitchCaseException.hpp"
#include "egolib/Math/_Include.hpp"
#include "egolib/Logic/Attribute.hpp"
#include "egolib/Logic/PerkHandler.hpp"
#include "egolib/Logic/ObjectSlot.hpp"
#include "egolib/Renderer/DeferredOpenGLTexture.hpp"
#include "egolib/bbox.h"
#include "egolib/clock.h"
#include "egolib/egoboo_setup.h"
#include "egolib/endian.h"
#include "egolib/file_common.h"
#include "egolib/fileutil.h"
#include "egolib/font_bmp.h"
#include "egolib/frustum.h"
#include "egolib/_math.h"
#include "egolib/Image/ImageManager.hpp"
#include "egolib/input_device.h"
#include "egolib/log.h"
#include "egolib/Math/Matrix44.hpp"
#include "egolib/map_functions.h"
#include "egolib/network.h"
#include "egolib/platform.h"
#include "egolib/egoboo_setup.h"
#include "egolib/strutil.h"
#include "egolib/timer.h"
#include "egolib/typedef.h"
#include "egolib/vfs.h"
#include "egolib/VFS/Pathname.hpp"
#include "egolib/Math/Vector.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "egolib/Time/LocalTime.hpp"
#include "egolib/Time/SlidingWindow.hpp"
#include "egolib/Time/Stopwatch.hpp"

#include "egolib/Graphics/FontManager.hpp"
#include "egolib/Graphics/Font.hpp"
#include "egolib/Graphics/TextureManager.hpp"
#include "egolib/Graphics/PixelFormat.hpp"
#include "egolib/Graphics/VertexBuffer.hpp"

#include "egolib/Renderer/Renderer.hpp"

#include "egolib/Profiles/_Include.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "egolib/FileFormats/configfile.h"
#include "egolib/FileFormats/controls_file.h"
#include "egolib/FileFormats/id_md2.h"
#include "egolib/FileFormats/map_file.h"
#include "egolib/FileFormats/map_tile_dictionary.h"
#include "egolib/FileFormats/quest_file.h"
#include "egolib/FileFormats/scancode_file.h"
#include "egolib/FileFormats/spawn_file.h"
#include "egolib/FileFormats/template.h"
#include "egolib/FileFormats/treasure_table_file.h"
#include "egolib/FileFormats/wawalite_file.h"

#if defined(USE_LUA_CONSOLE)
	#include "egolib/Lua/lua_console.h"
#else
	#include "egolib/console.h"
#endif

//!!! watch this one. if you are using the Fluid Studios memory manager,
//!!! this must be the absolute last include file.
#include "egolib/mem.h"
