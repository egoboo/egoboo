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

/// @file egolib/file_formats/configfile.h
/// @details Configuration file loading code.

#include <stdio.h>
#include <stdlib.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_ConfigFileValue;
typedef struct s_ConfigFileValue ConfigFileValue_t;
typedef struct s_ConfigFileValue *ConfigFileValuePtr_t;

struct s_ConfigFileSection;
typedef struct s_ConfigFileSection ConfigFileSection_t;
typedef struct s_ConfigFileSection *ConfigFileSectionPtr_t;

struct s_ConfigFileCarat;
typedef struct s_ConfigFileCarat ConfigFileCarat_t;
typedef struct s_ConfigFileCarat *ConfigFileCaratPtr_t;

struct s_ConfigFile;
typedef struct s_ConfigFile ConfigFile_t;
typedef struct s_ConfigFile *ConfigFilePtr_t;

//--------------------------------------------------------------------------------------------
// BOOLEAN

#if defined(__cplusplus)
typedef bool config_bool_t;
#define config_true true
#define config_false false
#else
enum e_config_bool
{
    config_true  = ( 1 == 1 ),
    config_false = ( !config_true )
};

// this typedef must be after the enum definition or gcc has a fit
typedef enum e_config_bool config_bool_t;
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define ConfigFile_succeed  1
#define ConfigFile_fail 0

#define MAX_CONFIG_SECTION_LENGTH    64
#define MAX_CONFIG_KEY_LENGTH      64
#define MAX_CONFIG_VALUE_LENGTH      256
#define MAX_CONFIG_COMMENTARY_LENGTH  256

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    /// the ConfigFile return value type
    typedef int ConfigFile_retval;

//--------------------------------------------------------------------------------------------
// struct s_ConfigFileValue
//--------------------------------------------------------------------------------------------

/// A single value in the congiguration file, specified by ["TAG"] = "VALUE"
    struct s_ConfigFileValue
    {
        ConfigFileValuePtr_t NextValue;

        char KeyName[MAX_CONFIG_KEY_LENGTH];
        char *Value;
        char *Commentary;
    };

//--------------------------------------------------------------------------------------------
// struct s_ConfigFileSection
//--------------------------------------------------------------------------------------------

    /// One section of the congiguration file, delimited by {"BLAH"}
    struct s_ConfigFileSection
    {
        ConfigFileSectionPtr_t NextSection;

        char SectionName[MAX_CONFIG_SECTION_LENGTH];

        ConfigFileValuePtr_t   FirstValuePtr;
    };

//--------------------------------------------------------------------------------------------
// struct s_ConfigFileCarat
//--------------------------------------------------------------------------------------------

    struct s_ConfigFileCarat
    {
        ConfigFileSectionPtr_t  SectionPtr;
        ConfigFileValuePtr_t    ValuePtr;
    };

//--------------------------------------------------------------------------------------------
// struct s_ConfigFile
//--------------------------------------------------------------------------------------------

    /// The congiguration file
    struct s_ConfigFile
    {
        FILE  *f;
        char   filename[256];

        ConfigFileSectionPtr_t  SectionList;
        ConfigFileCarat_t       Current;
    };

//--------------------------------------------------------------------------------------------
// External module functions
//--------------------------------------------------------------------------------------------

    extern ConfigFilePtr_t   ConfigFile_create( void );
    extern ConfigFile_retval ConfigFile_destroy( ConfigFilePtr_t * ptmp );

    extern ConfigFilePtr_t   ConfigFile_Load( const char *szFileName, config_bool_t force );
    extern ConfigFile_retval ConfigFile_Save( ConfigFilePtr_t pConfigFile );
    extern ConfigFile_retval ConfigFile_SaveAs( ConfigFilePtr_t pConfigFile, const char *szFileName );

    extern ConfigFile_retval ConfigFile_GetValue_String( ConfigFilePtr_t pConfigFile, const char *szSection, const char *szKey, char *pValue, size_t pValueBufferLength );
    extern ConfigFile_retval ConfigFile_GetValue_Boolean( ConfigFilePtr_t pConfigFile, const char *szSection, const char *szKey, config_bool_t *pBool );
    extern ConfigFile_retval ConfigFile_GetValue_Int( ConfigFilePtr_t pConfigFile, const char *szSection, const char *szKey, int *pInt );

    extern ConfigFile_retval ConfigFile_SetValue_String( ConfigFilePtr_t pConfigFile, const char *szSection, const char *szKey, const char *szValue );
    extern ConfigFile_retval ConfigFile_SetValue_Boolean( ConfigFilePtr_t pConfigFile, const char *szSection, const char *szKey, config_bool_t Bool );
    extern ConfigFile_retval ConfigFile_SetValue_Int( ConfigFilePtr_t pConfigFile, const char *szSection, const char *szKey, int Int );
    extern ConfigFile_retval ConfigFile_SetValue_Float( ConfigFilePtr_t pConfigFile, const char *szSection, const char *szKey, float Float );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _file_formats_configfile_h_
