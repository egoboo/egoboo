/* Egoboo - Log.c
 * Basic logging functionality.
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

#include "Log.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

#define MAX_LOG_MESSAGE 1024

static FILE *logFile = NULL;
static char logBuffer[MAX_LOG_MESSAGE];
static int logLevel = 1;

static void writeLogMessage( const char *prefix, const char *format, va_list args )
{
  if ( logFile != NULL )
  {
    vsnprintf( logBuffer, MAX_LOG_MESSAGE - 1, format, args );
    fputs( prefix, logFile );
    fputs( logBuffer, logFile );
    fflush( logFile );
  }
}

void log_init()
{
  if ( logFile == NULL )
  {
    logFile = fopen( "log.txt", "wt" );
    atexit( log_shutdown );
  }
}

void log_shutdown()
{
  if ( logFile != NULL )
  {
    fclose( logFile );
    logFile = NULL;
  }
}

void log_setLoggingLevel( int level )
{
  if ( level > 0 )
  {
    logLevel = level;
  }
}

void log_message( const char *format, ... )
{
  va_list args;

  va_start( args, format );
  writeLogMessage( "", format, args );
  va_end( args );
}

void log_info( const char *format, ... )
{
  va_list args;

  if ( logLevel >= 2 )
  {
    va_start( args, format );
    writeLogMessage( "INFO: ", format, args );
    va_end( args );
  }
}

void log_warning( const char *format, ... )
{
  va_list args;

  if ( logLevel >= 1 )
  {
    va_start( args, format );
    writeLogMessage( "WARN: ", format, args );
    va_end( args );
  }
}

void log_error( const char *format, ... )
{
  va_list args;

  va_start( args, format );
  writeLogMessage( "FATAL ERROR: ", format, args );
  va_end( args );

  fflush( logFile );
  exit( -1 );
}
