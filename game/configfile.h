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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - configfile.h
 * Configuration file loading code.
 */

#include "egoboo_typedef.h"

#include <stdio.h>
#include <stdlib.h>

#include "egoboo_strutil.h"

typedef int ConfigFile_retval;
#define ConfigFile_succeed  1
#define ConfigFile_fail 0

#define MAX_CONFIG_SECTION_LENGTH    64
#define MAX_CONFIG_KEY_LENGTH      64
#define MAX_CONFIG_VALUE_LENGTH      256
#define MAX_CONFIG_COMMENTARY_LENGTH  256

typedef struct s_ConfigFileValue ConfigFileValue_t;
typedef struct s_ConfigFileValue
{
    char KeyName[MAX_CONFIG_KEY_LENGTH];
    char *Value;
    char *Commentary;
    ConfigFileValue_t *NextValue;
} *ConfigFileValuePtr_t;

typedef struct s_ConfigFileSection ConfigFileSection_t;
typedef struct s_ConfigFileSection
{
    char SectionName[MAX_CONFIG_SECTION_LENGTH];
    ConfigFileSection_t  *NextSection;
    ConfigFileValuePtr_t  FirstValue;
} *ConfigFileSectionPtr_t;

typedef struct s_ConfigFile ConfigFile_t;
typedef struct s_ConfigFile
{
    FILE  *f;
    char   filename[256];

    ConfigFileSectionPtr_t  ConfigSectionList;
    ConfigFileSectionPtr_t  CurrentSection;
    ConfigFileValuePtr_t    CurrentValue;
} *ConfigFilePtr_t;

//
extern ConfigFilePtr_t   ConfigFile_create();
extern ConfigFile_retval ConfigFile_destroy( ConfigFilePtr_t * ptmp );

//
extern ConfigFilePtr_t   LoadConfigFile( const char *szFileName );
extern ConfigFile_retval SaveConfigFile( ConfigFilePtr_t pConfigFile );
extern ConfigFile_retval SaveConfigFileAs( ConfigFilePtr_t pConfigFile, const char *szFileName );

//
extern ConfigFile_retval ConfigFile_GetValue_String( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, char *pValue, Sint32 pValueBufferLength );
extern ConfigFile_retval ConfigFile_GetValue_Boolean( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, bool_t *pBool );
extern ConfigFile_retval ConfigFile_GetValue_Int( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, Sint32 *pInt );

//
extern ConfigFile_retval ConfigFile_SetValue_String( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, const char *pValue );
extern ConfigFile_retval ConfigFile_SetValue_Boolean( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, bool_t pBool );
extern ConfigFile_retval ConfigFile_SetValue_Int( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, int pInt );
extern ConfigFile_retval ConfigFile_SetValue_Float( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, float pFloat );

#define _CONFIGFILE_H_
