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

#include "egolib/platform.h"
#include "egolib/integrations/filesystem.hpp"

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

/// @brief Get the binary directory pathname.
/// @return the binary directory pathname
/// @brief Get the data directory pathname
/// @return the data directory pathname
std::string fs_getDataDirectory();

/// @brief Get the user directory pathname
/// @return the user directory pathname
std::string fs_getUserDirectory();

/// @brief Get the configuration directory pathname.
/// @return the configuration directory pathname
std::string fs_getConfigDirectory();

/**@}*/

/// @brief Get if a file exists.
/// @param pathname the pathname
/// @return @a 1 if the file exists, @a 0 if it does not exist, @a -1 if an invalid argument was passed.
int fs_fileExists(const std::string& pathname);

/// @brief Get if a directory exists.
/// @param pathname the pathname
/// @return @a 1 if the directory exists, @a 0 if it does not exist, @a -1 if an invalid argument was passed.
int fs_fileIsDirectory(const std::string& pathname);

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
int fs_createDirectory(const std::string& pathname);
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
void fs_removeDirectory(const std::string& pathname);
/**
 * @brief
 *  Delete a file.
 * @param pathname
 *  the pathname of the file
 */
void fs_deleteFile(const std::string& pathname);
bool fs_copyFile(const std::string& source, const std::string& target);
void fs_removeDirectoryAndContents(const char *pathname);

bool fs_ensureUserFile(const char * relative_filename, bool required);
