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

/// @file egolib/log.h
/// @details Basic logging functionality.
#pragma once

#include "egolib/typedef.h"

#if defined(__cplusplus)
extern "C"
{
#endif

enum LogLevel : uint8_t
{
    LOG_NONE,       ///< No log level, always printed directly to output
    LOG_ERROR,      ///< Fatal unrecoverable error (also terminates program)
    LOG_WARNING,    ///< Warning, something went wrong but nothing critical
    LOG_INFO,       ///< Information logging, default level
    LOG_DEBUG       ///< Verbose debug logging, useful for developers and debugging
};

//--------------------------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

    void   log_init(const char * logname, LogLevel logLevel);
    void   log_shutdown( void );

    FILE * log_get_file( void );

    void   log_message( const char *format, ... ) GCC_PRINTF_FUNC( 1 );
    void   log_debug( const char *format, ... ) GCC_PRINTF_FUNC( 1 );
    void   log_info( const char *format, ... ) GCC_PRINTF_FUNC( 1 );
    void   log_warning( const char *format, ... ) GCC_PRINTF_FUNC( 1 );
	/// @todo "logging" should have no side effects except of (eventually)
	/// writing the log entry. However, log_error in fact terminates the
	/// program.
    void   log_error( const char *format, ... ) GCC_PRINTF_FUNC( 1 );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif
