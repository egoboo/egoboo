/* Egoboo - egoboostrutil.h
 * String manipulation functions.  Not currently in use.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _EGOBOOSTRUTIL_H_
#define _EGOBOOSTRUTIL_H_

#include "egoboo_types.inl"
#include "egoboo_config.h"

#include <string.h>
#include <ctype.h>

void TrimStr( char *pStr );
char * convert_underscores( char *strout, size_t insize, char * strin );
char * convert_spaces( char *strout, size_t insize, char * strin );

#endif // #ifndef _EGOBOOSTRUTIL_H_

