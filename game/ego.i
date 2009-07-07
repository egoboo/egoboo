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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

%module ego

%{
#include "egoboo_typedef.h"
#include "egoboo_config.h"
#include "egoboo_endian.h"
#include "egoboo_math.h"

#include "egoboo_typedef.h"
#include "egoboo.h"
#include "proto.h"
#include "egoboo_math.h"
#include "link.h"

#include "char.h"
#include "particle.h"
#include "enchant.h"
#include "Md2.h"
#include "mpd.h"

#include "passage.h"
#include "sound.h"
#include "module.h"
#include "camera.h"

#include "script.h"
#include "script_functions.h"

#include "graphic.h"
#include "ogl_texture.h"

%}

// A stupid hack because SWIG can't deal with including
// the SDL_stdinc.h to get the system dependent types!
#define Uint8  unsigned char
#define Uint16 unsigned short
#define Uint32 unsigned int

#define Sint8  signed char
#define Sint16 signed short
#define Sint32 signed int

%include "egoboo_typedef.h"
%include "egoboo.h"

%include "script.h"
%include "script_functions.h"

%include "proto.h"
%include "link.h"

%include "char.h"
%include "particle.h"
%include "enchant.h"
%include "Md2.h"
%include "mpd.h"

%include "passage.h"
%include "sound.h"
%include "module.h"

%include "graphic.h"
%include "ogl_texture.h"

%include "camera.h"
