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
#include <grp.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __linux__
#include <linux/limits.h>
#endif

#include "egolib/file_common.h"
#include "egolib/Log/_Include.hpp"
#include "egolib/strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_linux_find_context;
typedef struct s_linux_find_context linux_find_context_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern int sys_fs_init(const char *root_dir);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Paths that the game will deal with
static char _binaryPath[PATH_MAX]   = EMPTY_CSTR;
static char _dataPath[PATH_MAX]     = EMPTY_CSTR;
static char _userPath[PATH_MAX] = EMPTY_CSTR;
static char _configPath[PATH_MAX]   = EMPTY_CSTR;

struct s_linux_find_context
{
    glob_t last_find;
    size_t find_index;
};

int sys_fs_init(const char *root_dir)
{
    // root_dir currently has no use in linux, since all of the egoboo game directories
    // are in fixed locations

    printf("initializing filesystem services\n");

    // grab the user's home directory
    char *userHome = getenv("HOME");
    snprintf(_userPath, SDL_arraysize(_userPath), "%s/.egoboo-2.x", userHome);

#if defined(_NIX_PREFIX) && defined(PREFIX)
    // the access to these directories is completely unknown
    // The default setting from the Makefile is to set PREFIX = "/usr/local",
    // so that the program will compile and install just like any other
    // .rpm or .deb package.

    snprintf(_configPath, SDL_arraysize(_configPath), "%s/etc/egoboo-2.x/", PREFIX);
    snprintf(_binaryPath, SDL_arraysize(_binaryPath), "%s/games/", PREFIX);
    snprintf(_dataPath, SDL_arraysize(_dataPath), "%s/share/games/egoboo-2.x/", PREFIX);
#elif !defined(_NIX_PREFIX) && defined(_DEBUG)
    // assume we are debugging using the "install directory" rather than using a real installation
    strncpy(_configPath, ".", SDL_arraysize(_configPath));
    strncpy(_binaryPath, ".", SDL_arraysize(_binaryPath));
    strncpy(_dataPath, ".", SDL_arraysize(_dataPath));
    strncpy(_userPath, ".", SDL_arraysize(_userPath));
#else
    // these are read-only directories
    strncpy(_configPath, "/etc/egoboo-2.x/", SDL_arraysize(_configPath));
    strncpy(_binaryPath, "/usr/games/", SDL_arraysize(_binaryPath));
    strncpy(_dataPath, "/usr/share/games/egoboo-2.x/", SDL_arraysize(_dataPath));
#endif

    // the log file cannot be started until there is a user data path to dump the file into
    // so dump this debug info to stdout
    printf("Game directories are:\n"
           "\tBinaries: %s\n"
           "\tData: %s\n"
           "\tUser: %s\n"
           "\tConfiguration: %s\n",
           _binaryPath, _dataPath, _userPath, _configPath);

    if (!fs_fileIsDirectory(_userPath))
    {
        fs_createDirectory(_userPath); /// @todo Error handling.
    }
    return 0;
}

int fs_fileIsDirectory(const std::string& pathname)
{
    struct stat stats;
    if (!stat(pathname.c_str(), &stats))
    {
        return S_ISDIR(stats.st_mode);
    }
    return 0;
}

int fs_createDirectory(const std::string& pathname)
{
    if (0 != mkdir(pathname.c_str(), 0755))
    {
        errno = 0; /// Clear errno.
        return 1;
    }
    return 0;
}

int fs_removeDirectory(const std::string& pathname)
{
    if (0 != rmdir(pathname.c_str()))
    {
        errno = 0; // Clear errno.
        return 1;
    }
    return 0;
}

void fs_deleteFile(const std::string& pathname)
{
    unlink(pathname.c_str());
}

bool fs_copyFile(const std::string& source, const std::string& target)
{  
    char buf[4096] = EMPTY_CSTR;
    int bytes_read;

    // Open source file descriptor.
    FILE *sourcefd = fopen(source.c_str(), "rb");
    if (!sourcefd)
    {
        return false;
    }
    // Open target file descriptor.
    FILE *targetfd = fopen(target.c_str(), "wb");
    if (!targetfd)
    {
        fclose(sourcefd);
        return false;
    }
    // Read Bytes from the target source file and write them into the target file.
    while ((bytes_read = fread(buf, 1, sizeof(buf), sourcefd)))
    {
        fwrite(buf, 1, bytes_read, targetfd);
    }
    // Finish file descriptors.
    fclose(sourcefd);
    fclose(targetfd);
    return true;
}

const char *fs_findFirstFile(const char *directory, const char *extension, fs_find_context_t *fs_search)
{
    char pattern[PATH_MAX] = EMPTY_CSTR;

    if (INVALID_CSTR(directory) || NULL == fs_search)
    {
        return NULL;
    }
    linux_find_context_t *pcnt = new linux_find_context_t();
    fs_search->type = linux_find;
    fs_search->ptr.l = pcnt;

    if (extension) {
        snprintf(pattern, PATH_MAX, "%s" SLASH_STR "*.%s", directory, extension);
    } else {
        snprintf(pattern, PATH_MAX, "%s" SLASH_STR "*", directory);
    }
 
    pcnt->last_find.gl_offs = 0;
    glob(pattern, GLOB_NOSORT, NULL, &pcnt->last_find);
    if (!pcnt->last_find.gl_pathc) {
        return nullptr;
    }
    pcnt->find_index = 0;
    char *last_slash = strrchr(pcnt->last_find.gl_pathv[pcnt->find_index], C_SLASH_CHR);
    if (last_slash) {
        return last_slash + 1;
    }
    return nullptr; /* should never happen */
}

const char *fs_findNextFile(fs_find_context_t *fs_search)
{
    if (!fs_search || fs_search->type != linux_find)
    {
        return NULL;
    }
    linux_find_context_t *pcnt = fs_search->ptr.l;
    if (!pcnt)
    {
        return NULL;
    }

    ++pcnt->find_index;
    if (pcnt->find_index >= pcnt->last_find.gl_pathc)
    {
        return NULL;
    }
    char *last_slash = strrchr(pcnt->last_find.gl_pathv[pcnt->find_index], C_SLASH_CHR);
    if (last_slash)
    {
        return last_slash + 1;
    }

    return NULL; /* should never happen */
}

void fs_findClose(fs_find_context_t *fs_search)
{
    if (NULL == fs_search || fs_search->type != linux_find)
    {
        return;
    }
    linux_find_context_t *pcnt = fs_search->ptr.l;
    if (NULL == pcnt)
    {
        return;
    }
    globfree(&(pcnt->last_find));

    delete pcnt;

	fs_search->type = unknown_find;
	fs_search->ptr.v = nullptr;
}

std::string fs_getBinaryDirectory()
{
    return _binaryPath;
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
