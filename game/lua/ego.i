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

%{
#include "camera.h"
#include "char.h"
#include "enchant.h"
#include "game.h"
#include "input.h"
#include "link.h"
#include "mad.h"
#include "mesh.h"
#include "profile.h"
#include "particle.h"
#include "passage.h"
#include "script.h"
#include "script_functions.h"

#include "file_formats/module_file.h"
#include "file_formats/mpd_file.h"
#include "file_formats/wawalite_file.h"

#include "egoboo_typedef.h"
#include "egoboo_config.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"
#include "egoboo.h"
%}

// A stupid hack because SWIG can't deal with including
// the SDL_stdinc.h to get the system dependent types!
#define Uint8  unsigned char
#define Uint16 unsigned short
#define Uint32 unsigned int

#define Sint8  signed char
#define Sint16 signed short
#define Sint32 signed int

// this is in lieu of %include "ogl_include.h" which would give the user access to certain
// lower-level graphics functions
typedef float   GLfloat;
typedef GLfloat GLXvector3f[3];

%module ego

%include "egoboo_typedef.h"
%include "egoboo.h"
%include "egoboo_config.h"
%include "egoboo_setup.h"
%include "egoboo_strutil.h"

%include "game.h"
%include "input.h"
%include "link.h"

%include "camera.h"
%include "char.h"
%include "enchant.h"
%include "mad.h"
%include "mesh.h"
%include "profile.h"
%include "particle.h"
%include "passage.h"

%include "script.h"
%include "script_functions.h"

%include "file_formats/module_file.h"
%include "file_formats/mpd_file.h"
%include "file_formats/wawalite_file.h"
