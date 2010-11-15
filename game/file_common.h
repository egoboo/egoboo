#pragma once

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

#include "egoboo_typedef.h"

#include <stdio.h>

#if defined(__cplusplus)
extern "C"
{
#endif

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
    enum e_fs_find_type
    {
        unknown_find = 0,
        win32_find,
        linux_find,
        mac_find
    };
    typedef enum e_fs_find_type fs_find_type_t;

//--------------------------------------------------------------------------------------------

/// struct to alias the 3 types of find data
    union u_fs_find_ptr
    {
        void                        * v;
        struct s_win32_find_context * w;
        struct s_linux_find_context * l;
        struct s_mac_find_context   * m;
    };
    typedef union u_fs_find_ptr fs_find_ptr_t;

//--------------------------------------------------------------------------------------------
    struct s_fs_find_context
    {
        fs_find_type_t type;
        fs_find_ptr_t  ptr;
    };

    typedef struct s_fs_find_context fs_find_context_t;

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

    void fs_init();

    const char * fs_getBinaryDirectory();
    const char * fs_getDataDirectory();
    const char * fs_getUserDirectory();
    const char * fs_getConfigDirectory();

    const char * fs_createBinaryDirectoryFilename( const char * relative_pathname );
    const char * fs_createDataDirectoryFilename( const char * relative_pathname );
    const char * fs_createUserDirectoryFilename( const char * relative_pathname );
    const char * fs_createConfigDirectoryFilename( const char * relative_pathname );

    FILE * fs_openBinaryDirectoryFile( const char * relative_pathname, const char * mode );
    FILE * fs_openDataDirectoryFile( const char * relative_pathname, const char * mode );
    FILE * fs_openUserDirectoryFile( const char * relative_pathname, const char * mode );
    FILE * fs_openConfigDirectoryFile( const char * relative_pathname, const char * mode );

    int    fs_fileExists( const char *filename );
    int    fs_fileIsDirectory( const char *filename );
    int    fs_createDirectory( const char *dirname );
    int    fs_removeDirectory( const char *dirname );
    void   fs_deleteFile( const char *filename );
    bool_t fs_copyFile( const char *source, const char *dest );
    void   fs_removeDirectoryAndContents( const char *dirname, int recursive );
    void   fs_copyDirectory( const char *sourceDir, const char *destDir );

// Enumerate directory contents
    const char *fs_findFirstFile( const char *path, const char *extension, fs_find_context_t * fs_search );
    const char *fs_findNextFile( fs_find_context_t * fs_search );
    void        fs_findClose( fs_find_context_t * fs_search );

    bool_t fs_ensureUserFile( const char * relative_filename, bool_t required );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _file_common_h
