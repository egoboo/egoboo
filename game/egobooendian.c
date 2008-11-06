#include "egoboo_endian.h"


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
float LoadFloatByteswapped( float *ptr )
{
  union
  {
    float f;
    int i;
  } u;

  u.f = *ptr;
  u.i = SDL_Swap32( u.i );

  return u.f;
}
#endif
