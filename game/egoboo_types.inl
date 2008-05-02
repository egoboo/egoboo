#include "egoboo_types.h"

#if SDL_BYTEORDER == SDL_LIL_ENDIAN

#    define SwapLE_float

#else

    INLINE float SwapLE_float( float val )
    {
      FCONVERT convert;

      convert.f = val;
      convert.i = SDL_SwapLE32(convert.i);

      return convert.f;
    };

#endif