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

/// @file platform/sys_linux.c
/// @brief System Dependent Functions
/// @details Unix/GNU/Linux/*nix - specific code

#include "log.h"
#include "system.h"

//    #include <unistd.h>
#include "file_common.h" /* for NULL */
#include <sys/time.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Linux hook into the main function

extern int SDL_main( int argc, char **argv );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static double _sys_startuptime;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void sys_initialize()
{
    struct timeval now;
    log_info( "Initializing Linux file system...\n" );
    gettimeofday( &now, NULL );
    _sys_startuptime = now.tv_sec + now.tv_usec * 1.0e-6;
}

//--------------------------------------------------------------------------------------------
double sys_getTime()
{
    struct timeval now;
    gettimeofday( &now, NULL );
    return (( double )now.tv_sec ) + now.tv_usec * 1.0e-6 - _sys_startuptime;
}

//--------------------------------------------------------------------------------------------
// This is where the whole thing actually starts in Linux
int main( int argc, char* argv[] )
{
    return SDL_main( argc, argv );
}

//--------------------------------------------------------------------------------------------
void sys_popup( const char * popup_title, const char * warning, const char * format, va_list args )
{
    //ZF> Basic untested implementation of error messaging in Linux

    /*    STRING message, buffer;
        snprintf( message, SDL_arraysize( message ), warning );
        vsnprintf( buffer, SDL_arraysize( buffer ), format, args );
        strcat( message, buffer );
        strcat( message, "\n Press OK to exit." );

        sprintf(buffer, "xmessage -center \"%s\"", message);

        if( 0 == fork() )
        {
            close(1); close(2);
            system(cmd);
        }*/
}