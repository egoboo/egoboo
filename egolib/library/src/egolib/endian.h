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

/// @file egolib/endian.h

#pragma once

#include "idlib/idlib.hpp"

/// Conversion from the "Ego File" Byte order to the "host" Byte order.
#define Endian_FileToHost(x) \
    idlib::convert_byte_order(x, idlib::get_byte_order(), idlib::byte_order::little_endian)

/// Conversion from the "host" Byte order to the "Ego file" Byte order.
#define Endian_HostToFile(x) \
    idlib::convert_byte_order(x, idlib::byte_order::little_endian, idlib::get_byte_order())
