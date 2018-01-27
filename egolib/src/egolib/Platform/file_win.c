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

/// @file egolib/Platform/file_win.c
/// @brief Windows-specific filesystem functions.
/// @details
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef NOMINMAX
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>

#include "egolib/file_common.h"
#include "egolib/Log/_Include.hpp"
#include "egolib/strutil.h"
#include "egolib/platform.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define HAS_ATTRIBS(ATTRIBS,VAR) ((INVALID_FILE_ATTRIBUTES != (VAR)) && ( (ATTRIBS) == ( (VAR) & (ATTRIBS) ) ))

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern int sys_fs_init(const char *argv0);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Paths that the game will deal with
static char _dataPath[MAX_PATH] = EMPTY_CSTR;
static char _userPath[MAX_PATH] = EMPTY_CSTR;
static char _configPath[MAX_PATH] = EMPTY_CSTR;

//--------------------------------------------------------------------------------------------
// File Routines
//--------------------------------------------------------------------------------------------

/// @brief Get the user data path.
/// @return @a true on success, @a false on failure
static bool computeUserDataPath()
{
    // The save path goes into the user's ApplicationData directory,
    // according to Microsoft's standards.  Will people like this, or
    // should I stick saves someplace easier to find, like My Documents?
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, _userPath);
    strncat(_userPath, SLASH_STR "Egoboo", MAX_PATH);
    return true;
}

/// @brief Compute the basicdata path.
/// @return @a true on success, @a false on failure
static bool computeBasicDataPath()
{
    char temporary[MAX_PATH];
    // (1) Check for data in the working directory
    char workingDirectory[MAX_PATH] = EMPTY_CSTR;
    GetCurrentDirectoryA(MAX_PATH, workingDirectory);
    snprintf(temporary, MAX_PATH, "%s" SLASH_STR "basicdat", workingDirectory);
    DWORD attrib = GetFileAttributesA(temporary);
    if (HAS_ATTRIBS(FILE_ATTRIBUTE_DIRECTORY, attrib))
    {
        strncpy(_dataPath, workingDirectory, MAX_PATH);
        return true;
    }
    // IF (1) failed
    // THEN check for data in the binary directory
    char binaryPath[MAX_PATH];
    GetModuleFileNameA(NULL, binaryPath, MAX_PATH);
    PathRemoveFileSpecA(binaryPath);

    snprintf(temporary, MAX_PATH, "%s" SLASH_STR "basicdat", binaryPath);
    attrib = GetFileAttributesA(temporary);
    if (HAS_ATTRIBS(FILE_ATTRIBUTE_DIRECTORY, attrib))
    {
        strncpy(_dataPath, binaryPath, MAX_PATH);
        return true;
    }
    return false;
}

int sys_fs_init(const char *argv0)
{
    printf("Initializing filesystem services ...");

    if (!computeUserDataPath())
    {
        // Fatal error here, we can't find the user data path.
        printf(" FAILURE: could not find user data path!\n");
        return 1;
    }
    if (!computeBasicDataPath())
    {
        // Fatal error here, we can't find the basic data path.
        printf(" FAILURE: could not find data path!\n");
        return 1;
    }
    printf(" SUCCESS\n");
    // config path is the same as the data path in win32
    strncpy(_configPath, _dataPath, SDL_arraysize(_configPath));

    // the log file cannot be started until there is a user data path to dump the file into
    // so dump this debug info to stdout
    printf("Game directories are:\n\tData: %s\n\tUser Data: %s\n\tConfig Files: %s\n",
           _dataPath, _userPath, _configPath);
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
