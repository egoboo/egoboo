/* Egoboo - egoboostrutil.c
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
    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#include "egoboostrutil.h"

// TrimStr remove all space and tabs in the beginning and at the end of the string
void TrimStr( char *pStr )
{
  Sint32 DebPos, EndPos, CurPos;

  if ( pStr == NULL )
  {
    return;
  }
  // look for the first character in string
  DebPos = 0;
  while ( isspace( pStr[DebPos] ) && pStr[DebPos] != 0 )
  {
    DebPos++;
  }

  // look for the last character in string
  EndPos = CurPos = DebPos;
  while ( pStr[CurPos] != 0 )
  {
    if ( !isspace( pStr[CurPos] ) )
    {
      EndPos = CurPos;
    }
    CurPos++;
  }

  if ( DebPos != 0 )
  {
    // shift string left
    for ( CurPos = 0; CurPos <= ( EndPos - DebPos ); CurPos++ )
    {
      pStr[CurPos] = pStr[CurPos + DebPos];
    }
    pStr[CurPos] = 0;
  }
  else
  {
    pStr[EndPos + 1] = 0;
  }
}
