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

/// @file egolib/file_common.h
/// @brief file system functionality
/**
 * @remark
 *  Some terminology; a pathname is composed of filenames separated by filename separators.
 *  A filename consists of a filename base and optionally a filename extension.
 *  A pathname can start with a root filename that and is then an absolute pathname; otherwise it is a relative pathname.
 *  @remark
 *  Usually a filename separator is a slash or a backlash.
 *  A filename extension is usually the string up to but not including the first period in the filename
 *  when searching from right to left (!). If no period is found, then the filename has no filename
 *  extension.
 */

#pragma once

#include "egolib/typedef.h"

//--------------------------------------------------------------------------------------------
// TYPEDEFS
//--------------------------------------------------------------------------------------------

struct s_win32_find_context;
struct s_linux_find_context;
struct s_mac_find_context;

//--------------------------------------------------------------------------------------------
// struct s_fs_find_context
//--------------------------------------------------------------------------------------------

/// enum to label 3 typed of find data
typedef enum fs_find_type
{
    unknown_find = 0,
    win32_find,
    linux_find,
    mac_find
} fs_find_type_t;

//--------------------------------------------------------------------------------------------

/// struct to alias the 3 types of find data
typedef union fs_find_ptr_t
{
    void *v;
    struct s_win32_find_context *w;
    struct s_linux_find_context *l;
    struct s_mac_find_context *m;
} fs_find_ptr_t;

//--------------------------------------------------------------------------------------------
typedef struct fs_find_context_t
{
    fs_find_type_t type;
    fs_find_ptr_t  ptr;
} fs_find_context_t;

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

/**
 * @brief
 *  Initialize the file system.
 * @param argv0
 *  the first argument of the command-line
 * @return
 *  @a 0 on success, a non-zero value on failure
 * @remark
 *  Among other things, it determines the paths returned by
 *  fs_getBinarDirectory(), fs_getDataDirectory(), fs_getUserDirectory()
 *  and fs_getConfigDirectory().
 */
int fs_init(const char *argv0);

/**@{*/

/**
 * @brief
 *  Functions to obtain absolute pathnames of the binary, configuration, data, and user directories.
 * @remark
 *  Those functions return pointers to C strings and never return a null pointer.
 *  The C strings may not be modified and remain unchanged until the file system is uninitialized..
 */

/**
 * @brief
 *  Get the binary directory pathname.
 * @return
 *  the binary directory pathname
 */
const char *fs_getBinaryDirectory();

/**
 * @brief
 *  Get the data directory pathname
 * @return
 *  the data directory pathname
 */
const char *fs_getDataDirectory();

/**
 * @breif
 *  Get the user directory pathname
 * @return
 *  the user directory pathname
 */
const char *fs_getUserDirectory();

/**
 * @brief
 *  Get the configuration directory pathname.
 * @return
 *  the configuration directory pathname
 */
const char *fs_getConfigDirectory();

/**@}*/

#if 0
const char *fs_createBinaryDirectoryFilename(const char *relative_pathname);
const char *fs_createDataDirectoryFilename(const char *relative_pathname);
const char *fs_createUserDirectoryFilename(const char *relative_pathname);
const char *fs_createConfigDirectoryFilename(const char *relative_pathname);
FILE *fs_openBinaryDirectoryFile(const char *relative_pathname, const char *mode);
FILE *fs_openDataDirectoryFile(const char *relative_pathname, const char *mode);
FILE *fs_openUserDirectoryFile(const char *relative_pathname, const char *mode);
FILE *fs_openConfigDirectoryFile(const char *relative_pathname, const char *mode);
#endif

/**
 * @brief
 *  Get if a file exists.
 * @param pathname
 *  a pathname
 * @return
 *  @a 1 if the file exists, @a 0 if it does not exist, @a -1 if an invalid argument was passed.
 */
int fs_fileExists(const char *pathname);

/**
 * @brief
 *  Get if a directory exists.
 * @param pathname
 *  the pathname
 * @return
 *  @a 1 if the directory exists, @a 0 if it does not exist, @a -1 if an invalid argument was passed.
 */
int fs_fileIsDirectory(const char *pathname);

/**
 * @brief
 *  Create a directory.
 * @param pathname
 *  the pathname of the directory
 * @return
 *  @a 0 on success, a non-zero value on failure
 * @remark
 *  All intermediate directories must exist;
 *  this function will only create the final directory in the path.
 */
int fs_createDirectory(const char *pathname);
/**
 * @brief
 *  Remove a directory.
 * @param pathname
 *  the pathname of the directory
 * @return
 *  @a 0 on success, a non-zero value on failure
 * @remark
 *  This function will only remove the final directory in the path.
 */
int fs_removeDirectory(const char *pathname);
/**
 * @brief
 *  Delete a file.
 * @param pathname
 *  the pathname of the file
 */
void fs_deleteFile(const char *pathname);
bool fs_copyFile(const char *source, const char *dest);
void fs_removeDirectoryAndContents(const char *pathname, int recursive);
/**
 * @brief
 *  Copy all files in a directory into another directory.
 * @param sourceDir
 *  the pathname of the source directory
 * @param targetDir
 *  the pathname of the target directory
 * @remark
 *  If the target directory does not exist, it is created.
 */
void fs_copyDirectory(const char *source, const char *target);

/**
 * @brief
 *  Begin a search.
 * @param directory
 *  the pathname of the directory to search in. Must not be a null pointer.
 * @param extension
 *  the filename extension of the files to search for.
 *  Only files with filenames ending with a period followed by the specified file extension are seached.
 * @param fs_search
 *  the context
 * @return
 *  a pointer to a C string with the filename of the first file found.
 *  A null pointer is returned if no file was found or an error occurred.
 * @remark
 *  If a pointer to a C string is returned,
 *  then this string remains valid as long as the search context is not modified.
 */
const char *fs_findFirstFile(const char *directory, const char *extension, fs_find_context_t *fs_search);
/**
 * @brief
 *  Continue a search.
 * @param fs_search
 *  the search context
 * @return
 *  a pointer to a C string with the filename of the next file found.
 *  A null pointer is returned if no file was found or an error occurred.
 * @remark
 *  If a pointer to a C string is returned,
 *  then this string remains valid as long as the search context is not modified.
 */
const char *fs_findNextFile(fs_find_context_t *fs_search);
/**
 * @brief
 *  End a search.
 * @param fs_search
 *  the context
 */
void fs_findClose(fs_find_context_t *fs_search);

bool fs_ensureUserFile(const char * relative_filename, bool required);
