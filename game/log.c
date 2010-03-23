//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file log.c
/// @brief Basic logging functionality.
/// @details

#include "log.h"

#include "egoboo_strutil.h"
#include "egoboo_config.h"
#include "egoboo_setup.h"

#include <stdlib.h>
#include <stdarg.h>

//So that error popup boxes work on windows
#ifdef WIN32
#include <windows.h>
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define MAX_LOG_MESSAGE 1024

static FILE *logFile = NULL;
static char  logBuffer[MAX_LOG_MESSAGE] = EMPTY_CSTR;
static int   logLevel = 1;

static int _atexit_registered = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void writeLogMessage( const char *prefix, const char *format, va_list args )
{
    if ( logFile != NULL )
    {
        EGO_vsnprintf( logBuffer, MAX_LOG_MESSAGE - 1, format, args );
        EGO_fputs( prefix, logFile );
        EGO_fputs( logBuffer, logFile );

#if defined(_CONSOLE) && defined(USE_DEBUG) && defined(LOG_TO_CONSOLE)
        EGO_fputs( prefix, stdout );
        EGO_fputs( logBuffer, stdout );
#endif

        EGO_fflush( logFile );
    }
}

//--------------------------------------------------------------------------------------------
void log_init( const char * logname )
{
    if ( NULL == logFile )
    {
        logFile = EGO_fopen( logname, "wt" );
        if ( NULL != logFile && !_atexit_registered )
        {
            _atexit_registered = 1;
            atexit( log_shutdown );
        }
    }
}

//--------------------------------------------------------------------------------------------
void log_shutdown()
{
    if ( logFile != NULL )
    {
        EGO_fclose( logFile );
        logFile = NULL;
    }
}

//--------------------------------------------------------------------------------------------
void log_setLoggingLevel( int level )
{
    if ( level > 0 )
    {
        logLevel = level;
    }
}

//--------------------------------------------------------------------------------------------
void log_message( const char *format, ... )
{
    va_list args;

    va_start( args, format );
    writeLogMessage( "", format, args );
    va_end( args );
}

//--------------------------------------------------------------------------------------------
void log_debug( const char *format, ... )
{
    va_list args;

    //Only if developer mode is enabled
    if ( !cfg.dev_mode ) return;

    va_start( args, format );
    if ( logLevel >= 3 )
    {
        writeLogMessage( "DEBUG: ", format, args );
    }
    va_end( args );
}

//--------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------
void log_error( const char *format, ... )
{
    va_list args;

    va_start( args, format );
    writeLogMessage( "FATAL ERROR: ", format, args );

	//Windows users get a proper error message popup box
#ifdef WIN32
	{
		STRING message, buffer;
		snprintf(message, SDL_arraysize( message ), "Egoboo has encountered a problem and is exiting. \nThis is the error report: \n");
	    EGO_vsnprintf( buffer, SDL_arraysize( buffer ), format, args );
		strcat(message, buffer);
		strcat(message, "\n Press OK to exit.");
		MessageBox(NULL, message, "Egoboo: Fatal Error", MB_ICONSTOP|MB_SETFOREGROUND);
	}
#endif

    va_end( args );

    EGO_fflush( logFile );
    exit( -1 );
}

//--------------------------------------------------------------------------------------------
FILE * log_get_file()
{
    return logFile;
}
