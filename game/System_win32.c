/* Egoboo - sys_win32.c
 * Microsoft Windows-specific platform code
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

#include "proto.h"
#include "Log.h"
#include "System.h"
#include <windows.h>

double win32_secondsPerTick = 0;

void sys_initialize()
{
  LARGE_INTEGER frequency;
  Uint32 f;

  log_info( "Initializing high-performance counter...\n" );

  QueryPerformanceFrequency( &frequency );
  win32_secondsPerTick = 1.0 / frequency.QuadPart;

  f = frequency.QuadPart;
  log_info( "Frequency is %d hz\n", f );
}

void sys_shutdown()
{
}

double sys_getTime()
{
  LARGE_INTEGER time;

  QueryPerformanceCounter( &time );
  return time.QuadPart * win32_secondsPerTick;
}

int sys_frameStep()
{
  return 0;
}
