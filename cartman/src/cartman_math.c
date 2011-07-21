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

