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
#include <sys/wait.h>

#include "egolib/log.h"
#include "egolib/system.h"
#include "egolib/file_common.h" /* for NULL */

//--------------------------------------------------------------------------------------------
//Different methods of displaying messages in Linux
enum dialog_t
{
    ZENITY = 0,
    KDIALOG,
    SDL2,
    XMESSAGE,
    DIALOG_PROGRAM_END,
    DIALOG_PROGRAM_BEGIN = ZENITY
};

/**
 * @brief
 *  Tries using SDL2's message box if available.
 * @param title
 *  The title of the message box.
 * @param message
 *  The message of the message box.
 * @return
 *  @c true if SDL2's message box was successfully used, @c false otherwise.
 */
bool handleSDL2MessageBox(const std::string &title, const std::string message)
{
    void *sdl2Library = SDL_LoadObject("libSDL2.so");
    if (sdl2Library)
    {
        struct SDL_Window;
        uint32_t SDL_MESSAGEBOX_ERROR = 0x10;
        typedef int (*SDLMessageBoxFunc)(uint32_t, const char *, const char *, SDL_Window *);
        SDLMessageBoxFunc showMessageBox = (SDLMessageBoxFunc) SDL_LoadFunction(sdl2Library, "SDL_ShowSimpleMessageBox");
        int ret = -1;
        if (showMessageBox)
        {
            ret = showMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), message.c_str(), nullptr);
        }
        SDL_UnloadObject(sdl2Library);
        if (ret == 0) return true;
    }
    return false;
}

/**
 * @brief
 *  Execute a command without the shell (as in @a system)
 * @param args
 *  Arguments for the process to fork
 * @return
 *  @c true if successfully ran the command, @c false otherwise.
 */
bool executeArgs(const std::vector<std::string> &args)
{
    printf("Trying %s...\n", args[0].c_str());
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork failed");
        return false;
    }
    else if (pid == 0)
    {
        char **argv = new char *[args.size() + 1];
        for (size_t i = 0; i < args.size(); i++)
        {
            argv[i] = new char[args[i].length() + 1];
            strncpy(argv[i], args[i].c_str(), args[i].length() + 1);
        }
        argv[args.size()] = nullptr;
        execvp(argv[0], argv);
        int olderrno = errno;
        perror("execvp failed");
        _Exit(olderrno);
    }
    int status;
    pid_t wait = waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

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
void sys_popup( const char * popup_title, const char * warning, const char * format, va_list args )
{
    //ZF> Basic untested implementation of error messaging in Linux
    // @TODO: It has been reported that this doesn't work (22.02.2011)
    // Should work a lot better now

    std::string title = popup_title;
    std::string message = warning;
    char buffer[4096];
    bool tried[DIALOG_PROGRAM_END];
    int i, type = DIALOG_PROGRAM_BEGIN;
    const char *session = getenv("XDG_CURRENT_DESKTOP");
    if (!session) session = getenv("DESKTOP_SESSION");

    for (int cnt = DIALOG_PROGRAM_BEGIN; cnt < DIALOG_PROGRAM_END; cnt++) tried[cnt] = false;
    
    //Ready the message
    vsnprintf( buffer, SDL_arraysize( buffer ), format, args );
    message += buffer;

    //Figure out if there is a method we prefer
    if ( 0 == strcasecmp( session, "GNOME" ) || 0 == strcasecmp(session, "Unity")) type = ZENITY;
    else if ( 0 == strcasecmp( session, "KDE" ) ) type = KDIALOG;

    while ( true )
    {
        std::vector<std::string> command{};
        bool success = false;
        //Ready the command
        switch ( type )
        {
            case ZENITY:
                command.emplace_back("zenity");
                command.emplace_back("--error");
                command.emplace_back("--text=" + message);
                command.emplace_back("--title=" + title);
                break;
            case KDIALOG:
                command.emplace_back("kdialog");
                command.emplace_back("--error");
                command.emplace_back(message);
                command.emplace_back("--title");
                command.emplace_back(title);
                break;
            case SDL2:
                success = handleSDL2MessageBox(title, message);
                break;
            case XMESSAGE:
                command.emplace_back("xmessage");
                command.emplace_back("-center");
                command.emplace_back(message);
                break;
        }

        //Did we succeed?
        if (!command.empty()) success = executeArgs(command);
        
        if (success) break;

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


