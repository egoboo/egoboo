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

/// @file platform/sys_mac.c
/// @brief Implementation of mac system-dependent functions
/// @details

#include "System.h"

#include "file_common.h" /* for NULL */
#include <sys/time.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static double _sys_startuptime;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void sys_initialize()
{
    struct timeval now;
    gettimeofday( &now, NULL );
    _sys_startuptime = now.tv_sec + now.tv_usec * 1e-6;
}

//--------------------------------------------------------------------------------------------
double sys_getTime()
{
    struct timeval now;
    gettimeofday( &now, NULL );
    return (( double )now.tv_sec ) + now.tv_usec * 1e-6 - _sys_startuptime;
}

//--------------------------------------------------------------------------------------------
void sys_popup( const char * popup_title, const char * warning, const char * format, va_list args )
{
    /// the system popup is not supported on osx at this time.
}