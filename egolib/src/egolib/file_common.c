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

/// @file egolib/file_common.c
/// @brief Base implementation of the Egoboo filesystem
/// @details File operations that are shared between various operating systems.
/// OS-specific code goes in *-file.c (such as win-file.c)

#include "egolib/file_common.h"

#include "egolib/Log/_Include.hpp"

#include "egolib/strutil.h"
#include "egolib/vfs.h"
#include "egolib/platform.h"

#if !defined(MAX_PATH)
#define MAX_PATH 260  // Same value that Windows uses...
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool _fs_initialized = false;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/**
 * @brief
 *  Initialize the platform file system.
 * @param argv0
 *  the first argument of the command-line
 * @return
 *  @a 0 on success, a non-zero value on failure
 */
int sys_fs_init(const char *argv0);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

int fs_init(const char *argv0)
{
    if (_fs_initialized)
    {
        return 0;
    }
    if (sys_fs_init(argv0))
    {
        return 1;
    }
    _fs_initialized = true;
    return 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void fs_removeDirectoryAndContents(const char *dirname)
{
    /// @author ZZ
    /// @details This function deletes all files in a directory,
    ///    and the directory itself

    char filePath[MAX_PATH] = EMPTY_CSTR;
    const char *fileName;
    fs_find_context_t fs_search;

    // List all the files in the directory
    fileName = fs_findFirstFile(dirname, NULL, &fs_search);
    while (NULL != fileName)
    {
        // Ignore files that start with a ., like .svn for example.
        if ('.' != fileName[0])
        {
            snprintf(filePath, MAX_PATH, "%s" SLASH_STR "%s", dirname, fileName);
            if (fs_fileIsDirectory(filePath))
            {
                fs_removeDirectoryAndContents(filePath);
            }
            else
            {
                fs_deleteFile(filePath);
            }
        }
        fileName = fs_findNextFile(&fs_search);
    }
    fs_findClose(&fs_search);

    fs_removeDirectory(dirname);
}

//--------------------------------------------------------------------------------------------
void fs_copyDirectory(const char *sourceDir, const char *targetDir)
{
    fs_find_context_t fs_search;

    // List all the files in the directory
    const char *filename = fs_findFirstFile(sourceDir, NULL, &fs_search);
    if (filename)
    {
        // Make sure the destination directory exists.
        fs_createDirectory(targetDir); /// @todo Error handling here - if the directory does not exist, we can stop.

        while (filename)
        {
            // Ignore files that begin with a `'.'`.
            if ('.' != filename[0])
            {
                char sourcePath[MAX_PATH] = EMPTY_CSTR, targetPath[MAX_PATH] = EMPTY_CSTR;
                snprintf(sourcePath, MAX_PATH, "%s" SLASH_STR "%s", sourceDir, filename);
                snprintf(targetPath, MAX_PATH, "%s" SLASH_STR "%s", targetDir, filename);
                fs_copyFile(sourcePath, targetPath);
            }

            filename = fs_findNextFile(&fs_search);
        }
    }

    fs_findClose(&fs_search);
}

//--------------------------------------------------------------------------------------------
int fs_fileIsDirectory(const std::string& pathname)
{
    return id::file_system::is_directory(pathname) ? 1 : 0;
}

//--------------------------------------------------------------------------------------------
int fs_fileExists(const std::string& filename)
{
    if (filename.empty())
    {
        return -1;
    }
    FILE *ptmp = fopen(filename.c_str(), "rb");
    if (ptmp)
    {
        fclose(ptmp);
        return 1;
    }
    return 0;
}

//--------------------------------------------------------------------------------------------
bool fs_ensureUserFile( const char * relative_filename, bool required )
{
    /// @author BB
    /// @details if the file does not exist in the user data directory, it is copied from the
    /// data directory. Pass this function a system-dependent pathneme, relative the the root of the
    /// data directory.
    ///
    /// @note we can't use the vfs to do this in win32 because of the dir structure and
    /// the fact that PHYSFS will not add the same directory to 2 different mount points...
    /// seems pretty stupid to me, but there you have it.

    std::string path_str = fs_getUserDirectory() + std::string(SLASH_STR) + relative_filename;
    path_str = str_convert_slash_sys( path_str );

    int found = fs_fileExists( path_str );
    if ( 0 == found )
    {
        // copy the file from the Data Directory to the User Directory
        std::string src_path_str = fs_getConfigDirectory() + std::string(SLASH_STR) + relative_filename;
        fs_copyFile( src_path_str.c_str(), path_str.c_str() );
        found = fs_fileExists( path_str.c_str() );
    }

    // if it still doesn't exist, we have problems
    if (( 0 == found ) && required )
    {
        auto e = Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "unable to find file ", "`",
                                    relative_filename, "`", Log::EndOfEntry);
        Log::get() << e;
		throw std::runtime_error(e.getText());
    }

    return ( 0 != found );
}
