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

#include "egoboo_endian.h"

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

#if SDL_BYTEORDER != SDL_LIL_ENDIAN

union u_convert {float f; Uint32 i;};

typedef union u_convert convert_t;

//--------------------------------------------------------------------------------------------
float ENDIAN_FLOAT(float X)
{
    convert_t utmp;

    utmp.f = X;

    utmp.i = SDL_SwapLE32(utmp.i);

    return utmp.f;
}

#endif
