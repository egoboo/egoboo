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

struct s_win32_find_context;
typedef struct s_win32_find_context win32_find_context_t;

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
    SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, _userPath);
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
    GetCurrentDirectory(MAX_PATH, workingDirectory);
    snprintf(temporary, MAX_PATH, "%s" SLASH_STR "basicdat", workingDirectory);
    DWORD attrib = GetFileAttributes(temporary);
    if (HAS_ATTRIBS(FILE_ATTRIBUTE_DIRECTORY, attrib))
    {
        strncpy(_dataPath, workingDirectory, MAX_PATH);
        return true;
    }
    // IF (1) failed
    // THEN check for data in the binary directory
    char binaryPath[MAX_PATH];
    GetModuleFileName(NULL, binaryPath, MAX_PATH);
    PathRemoveFileSpec(binaryPath);

    snprintf(temporary, MAX_PATH, "%s" SLASH_STR "basicdat", binaryPath);
    attrib = GetFileAttributes(temporary);
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

// Had to revert back to prog x code to prevent import/skin bug
int fs_createDirectory(const std::string& dirname)
{
    if (dirname.empty())
    {
        return 1;
    }
    return (0 != CreateDirectory(dirname.c_str(), NULL)) ? 0 : 1;
}

bool fs_copyFile(const std::string& source, const std::string& target)
{
    if (source.empty() || target.empty())
    {
        return false;
    }
    return (TRUE == CopyFile(source.c_str(), target.c_str(), false));
}

//--------------------------------------------------------------------------------------------
// Directory Functions
//--------------------------------------------------------------------------------------------
struct s_win32_find_context
{
    WIN32_FIND_DATA wfdData;
    HANDLE          hFind;
};

const char *fs_findFirstFile(const char *searchDir, const char *searchExtension, fs_find_context_t *fs_search)
{
    char searchSpec[MAX_PATH] = EMPTY_CSTR;

    if (INVALID_CSTR(searchDir) || !fs_search)
    {
        return nullptr;
    }

	win32_find_context_t *pcnt = new win32_find_context_t();
    fs_search->type = win32_find;
    fs_search->ptr.w = pcnt;

    size_t len = strlen(searchDir) + 1;
    if (C_SLASH_CHR != searchDir[len] || C_BACKSLASH_CHR != searchDir[len]) {
        _snprintf(searchSpec, MAX_PATH, "%s" SLASH_STR, searchDir);
    } else {
        strncpy(searchSpec, searchDir, MAX_PATH);
    }
    if (nullptr != searchExtension) {
        _snprintf(searchSpec, MAX_PATH, "%s*.%s", searchSpec, searchExtension);
    } else {
        strncat(searchSpec, "*", MAX_PATH);
    }

    pcnt->hFind = FindFirstFile( searchSpec, &pcnt->wfdData );
	if (pcnt->hFind == INVALID_HANDLE_VALUE) {
		return nullptr;
	}

    return pcnt->wfdData.cFileName;
}

const char *fs_findNextFile(fs_find_context_t *fs_search)
{
    if (!fs_search || win32_find != fs_search->type)
    {
        return NULL;
    }
    win32_find_context_t *pcnt = fs_search->ptr.w;
    if (!pcnt)
    {
        return NULL;
    }
    if (NULL == pcnt->hFind || INVALID_HANDLE_VALUE == pcnt->hFind)
    {
        return NULL;
    }
    if (!FindNextFile( pcnt->hFind, &pcnt->wfdData))
    {
        return NULL;
    }
    return pcnt->wfdData.cFileName;
}

void fs_findClose(fs_find_context_t *fs_search)
{
    if (NULL == fs_search || win32_find != fs_search->type)
    {
        return;
    }
    win32_find_context_t *pcnt = fs_search->ptr.w;
    if (NULL == pcnt)
    {
        return;
    }
    if (NULL != pcnt->hFind)
    {
        FindClose(pcnt->hFind);
        pcnt->hFind = NULL;
    }
    delete pcnt;

	fs_search->type = unknown_find;
	fs_search->ptr.v = nullptr;
}

int DirGetAttrib(const char *fromdir)
{
    return(GetFileAttributes(fromdir));
}
