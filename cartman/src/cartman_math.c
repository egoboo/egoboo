//********************************************************************************************
//*
//*    This file is part of Cartman.
//*
//*    Cartman is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Cartman is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Cartman.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "cartman_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t SDL_RectIntersect( SDL_Rect * src, SDL_Rect * dst, SDL_Rect * isect )
{
    Sint16 xmin, xmax, ymin, ymax;

    // should not happen
    if ( NULL == src && NULL == dst ) return bfalse;

    // null cases
    if ( NULL == isect ) return bfalse;
    if ( NULL == src ) { *isect = *dst; return btrue; }
    if ( NULL == dst ) { *isect = *src; return btrue; }

    xmin = MAX( src->x, dst->x );
    xmax = MIN( src->x + src->w, dst->x + dst->w );

    ymin = MAX( src->y, dst->y );
    ymax = MIN( src->y + src->h, dst->y + dst->h );

    isect->x = xmin;
    isect->w = MAX( 0, xmax - xmin );
    isect->y = ymin;
    isect->h = MAX( 0, ymax - ymin );

    return btrue;
}
