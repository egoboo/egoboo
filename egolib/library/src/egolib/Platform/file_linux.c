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

/// @file egolib/Platform/file_linux.c
/// @brief Implementation of the linux system-dependent filesystem functions
/// @details

#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <limits.h>
#include "egolib/file_common.h"
#include "egolib/strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern int sys_fs_init(const char *root_dir);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Paths that the game will deal with
static char _dataPath[PATH_MAX]     = EMPTY_CSTR;
static char _userPath[PATH_MAX] = EMPTY_CSTR;
static char _configPath[PATH_MAX]   = EMPTY_CSTR;

int sys_fs_init(const char *root_dir)
{
    // root_dir currently has no use in linux, since all of the egoboo game directories
    // are in fixed locations

    printf("initializing filesystem services\n");

#if defined(_NIX_PREFIX) && defined(PREFIX)
    // the access to these directories is completely unknown
    // The default setting from the Makefile is to set PREFIX = "/usr/local",
    // so that the program will compile and install just like any other
    // .rpm or .deb package.

    // grab the user's home directory
    char *userHome = getenv("HOME");
    snprintf(_userPath, SDL_arraysize(_userPath), "%s/.egoboo-2.x", userHome);

    snprintf(_configPath, SDL_arraysize(_configPath), "%s/etc/egoboo-2.x/", PREFIX);
    snprintf(_dataPath, SDL_arraysize(_dataPath), "%s/share/games/egoboo-2.x/", PREFIX);
#elif !defined(_NIX_PREFIX) && defined(_DEBUG)
    // assume we are debugging using the "install directory" rather than using a real installation
    strncpy(_configPath, ".", SDL_arraysize(_configPath));
    strncpy(_dataPath, ".", SDL_arraysize(_dataPath));
    strncpy(_userPath, ".", SDL_arraysize(_userPath));
#else
    //Writeable directories
    char* applicationPreferencePath = SDL_GetPrefPath("egoboo", "egoboo");
    if(!applicationPreferencePath) {
        applicationPreferencePath = (char*)SDL_malloc(128);
        snprintf(_userPath, SDL_arraysize(_userPath), "%s/.egoboo", getenv("HOME"));        
    }
    strncpy(_configPath, applicationPreferencePath, SDL_arraysize(_configPath));
    strncpy(_userPath, applicationPreferencePath, SDL_arraysize(_userPath));
    SDL_free(applicationPreferencePath);

    // these are read-only directories
    char* applicationPath = SDL_GetBasePath();
    if(applicationPath == nullptr) {
        applicationPath = SDL_strdup("./");
    }
    strncpy(_dataPath, applicationPath, SDL_arraysize(_dataPath));
    SDL_free(applicationPath);
#endif

    // the log file cannot be started until there is a user data path to dump the file into
    // so dump this debug info to stdout
    printf("Game directories are:\n"
           "\tData: %s\n"
           "\tUser: %s\n"
           "\tConfiguration: %s\n",
           _dataPath, _userPath, _configPath);

    if (!fs_fileIsDirectory(_userPath))
    {
        fs_createDirectory(_userPath); /// @todo Error handling.
    }
    return 0;
}

std::string fs_getDataDirectory()
{
    return _dataPath;
}

std::string fs_getUserDirectory()
{
    return _userPath;
}

std::string fs_getConfigDirectory()
{
    return _configPath;
}
