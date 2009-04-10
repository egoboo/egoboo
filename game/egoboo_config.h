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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

#if defined(_MSC_VER)
// Microsoft Visual C compiler

// snprintf and vsnprintf are not considered native functions in MSVC
// they are defined with an underscore to indicate this
#    define snprintf  _snprintf
#    define vsnprintf _vsnprintf

// sets the packing of a data structure at declaration
//#    define SET_PACKING(NAME,PACKING) __declspec( align( PACKING ) ) NAME

// Turn off warnings that we don't care about.
#    pragma warning(disable : 4305) // truncation from 'double' to 'float'
#    pragma warning(disable : 4244) // conversion from 'double' to 'float'
#    pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
//#    pragma warning(disable : 4554) // possibly operator precendence error
//#    pragma warning(disable : 4761)
//#    pragma warning(disable : 4244) // truncation from 'type' to 'type'
#endif

#if defined(__GNUC__)
// the gcc C compiler

// sets the packing of a data structure at declaration
//#    define SET_PACKING(NAME,PACKING) NAME __attribute__ ((aligned (PACKING)))

#endif

#ifdef __unix__
#    include <unistd.h>
#endif

// Speeds up compile times a bit.  We don't need everything in windows.h
#ifdef WIN32
#    define WIN32_LEAN_AND_MEAN
#endif

// Define the filesystem appropriate slash characters
#ifdef WIN32
#    define SLASH_STR "\\"
#    define SLASH_CHR '\\'
#else
#    define SLASH_STR "/"
#    define SLASH_CHR '/'
#endif

#if !defined(SET_PACKING)
#define SET_PACKING(NAME,PACKING) NAME
#endif
