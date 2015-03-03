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

/// @file egolib/platform/sys_linux.c
/// @brief System Dependent Functions
/// @details Unix/GNU/Linux/*nix - specific code

#include <unistd.h>      // For message box in linux
#include <stdarg.h>
#include <sys/time.h>

#include "egolib/log.h"
#include "egolib/system.h"
#include "egolib/file_common.h" /* for NULL */

//--------------------------------------------------------------------------------------------
//Different methods of displaying messages in Linux
enum e_dialog
{
    ZENITY = 0,
    KDIALOG,
    XMESSAGE,
    DIALOG_PROGRAM_END,
    DIALOG_PROGRAM_BEGIN = ZENITY
};

// this typedef must be after the enum definition or gcc has a fit
typedef enum e_dialog dialog_t;

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
#if 0
int main( int argc, char* argv[] )
{
    return SDL_main( argc, argv );
}
#endif

//--------------------------------------------------------------------------------------------
void sys_popup( const char * popup_title, const char * warning, const char * format, va_list args )
{
    //ZF> Basic untested implementation of error messaging in Linux
    // @TODO: It has been reported that this doesn't work (22.02.2011)

    STRING message, buffer;
    bool tried[DIALOG_PROGRAM_END];
    int i, type = DIALOG_PROGRAM_BEGIN;
    const char *session = getenv( "DESKTOP_SESSION" );

    for (int cnt = DIALOG_PROGRAM_BEGIN; cnt < DIALOG_PROGRAM_END; cnt++) tried[cnt] = false;
    //Ready the message
    strncpy( message, warning, SDL_arraysize( message ) );
    vsnprintf( buffer, SDL_arraysize( buffer ), format, args );
    strcat( message, buffer );
    strcat( message, "\n Press OK to exit." );

    //Figure out if there is a method we prefer
    if ( 0 == strcmp( session, "gnome" ) ) type = ZENITY;
    else if ( 0 == strcmp( session, "kde-plasma" ) ) type = KDIALOG;

    while ( true )
    {
        //Ready the command
        switch ( type )
        {
            case ZENITY:   sprintf( buffer, "zenity --error --text='%s' --title='%s'", message, popup_title ); break;
            case KDIALOG:  sprintf( buffer, "kdialog --error '%s' --title '%s'", message, popup_title ); break;
            case XMESSAGE: sprintf( buffer, "xmessage -center '%s'", message ); break;
        }

        //Did we succeed?
        if ( 0 == system( buffer ) ) break;

        //Nope, try the next solution
        tried[type] = true;

        for ( i = DIALOG_PROGRAM_BEGIN; i < DIALOG_PROGRAM_END; i++ )
        {
            if ( tried[i] ) continue;
            type = i;
            break;
        }

        //Did everything fail? If so we just give up
        if ( i == DIALOG_PROGRAM_END ) break;
    }

}


