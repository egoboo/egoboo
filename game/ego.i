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

#include "passage.h"

#include "sound.h"

#include "script.h"

#include "module.h"

#include "graphic.h"
#include "gltexture.h"

#include "camera.h"
%}

// A stupid hack because SDL can't deal with including 
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
%include "proto.h"
%include "link.h"

%include "char.h"

%include "particle.h"

%include "enchant.h"

%include "passage.h"

%include "sound.h"

%include "module.h"

%include "graphic.h"
%include "gltexture.h"

%include "camera.h"