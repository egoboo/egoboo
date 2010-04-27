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

/// @file platform/sys_win32.c
/// @brief System Dependent Functions
/// @details Win32 - specific code

#include <windows.h>

#include "system.h"
#include "log.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
double win32_secondsPerTick = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void sys_initialize()
{
    LARGE_INTEGER frequency;
    LONGLONG f;

    log_info( "Initializing high-performance counter...\n" );

    QueryPerformanceFrequency( &frequency );
    win32_secondsPerTick = 1.0f / frequency.QuadPart;

    f = frequency.QuadPart;
    log_info( "Frequency is %ld hz\n", f );
}

//--------------------------------------------------------------------------------------------
double sys_getTime()
{
    LARGE_INTEGER time;

    QueryPerformanceCounter( &time );
    return time.QuadPart * win32_secondsPerTick;
}

//--------------------------------------------------------------------------------------------
void sys_popup( const char * popup_title, const char * warning, const char * format, va_list args )
{
    /// @details ZF@> Windows users get a proper error message popup box

    STRING message, buffer;

    snprintf( message, SDL_arraysize( message ), warning );

    vsnprintf( buffer, SDL_arraysize( buffer ), format, args );
    strcat( message, buffer );
    strcat( message, "\n Press OK to exit." );

    MessageBox( NULL, message, popup_title, MB_ICONSTOP | MB_SETFOREGROUND );
}