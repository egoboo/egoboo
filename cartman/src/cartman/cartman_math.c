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

#include "cartman/cartman_math.h"

//--------------------------------------------------------------------------------------------

bool SDL_RectIntersect( SDL_Rect * src, SDL_Rect * dst, SDL_Rect * isect )
{
    int16_t xmin, xmax, ymin, ymax;

    // should not happen
    if ( NULL == src && NULL == dst ) return false;

    // null cases
    if ( NULL == isect ) return false;
    if ( NULL == src ) { *isect = *dst; return true; }
    if ( NULL == dst ) { *isect = *src; return true; }

    xmin = std::max( src->x, dst->x );
    xmax = std::min( src->x + src->w, dst->x + dst->w );

    ymin = std::max( src->y, dst->y );
    ymax = std::min( src->y + src->h, dst->y + dst->h );

    isect->x = xmin;
    isect->w = std::max( 0, xmax - xmin );
    isect->y = ymin;
    isect->h = std::max( 0, ymax - ymin );

    return true;
}
