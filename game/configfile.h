/* Egoboo - configfile.h
 * Configuration file loading code.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_

#include "egoboo_types.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_CONFIG_SECTION_LENGTH  64
#define MAX_CONFIG_KEY_LENGTH   64
#define MAX_CONFIG_VALUE_LENGTH   256
#define MAX_CONFIG_COMMENTARY_LENGTH 256

typedef struct configfile_value_t
{
  char KeyName[MAX_CONFIG_KEY_LENGTH];
  char *Value;
  char *Commentary;
  struct configfile_value_t *NextValue;
} ConfigFileValue, *ConfigFileValuePtr;

typedef struct configfile_section_t
{
  char SectionName[MAX_CONFIG_SECTION_LENGTH];
  struct configfile_section_t *NextSection;
  ConfigFileValuePtr FirstValue;
} ConfigFileSection, *ConfigFileSectionPtr;

typedef struct config_file_t
{
  FILE *f;
  ConfigFileSectionPtr ConfigSectionList;

  ConfigFileSectionPtr CurrentSection;
  ConfigFileValuePtr  CurrentValue;
} ConfigFile, *ConfigFilePtr;


// util
void ConvertToKeyCharacters( char *pStr );

//
ConfigFilePtr OpenConfigFile( const char *pPath );

//
Sint32 GetConfigValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, char *pValue, Sint32 pValueBufferLength );
Sint32 GetConfigBooleanValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, bool_t *pBool );
Sint32 GetConfigIntValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, Sint32 *pInt );

//
Sint32 SetConfigValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, const char *pValue );
Sint32 SetConfigBooleanValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, bool_t pBool );
Sint32 SetConfigIntValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, int pInt );
Sint32 SetConfigFloatValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, float pFloat );

//
void CloseConfigFile( ConfigFilePtr pConfigFile );

//
void SaveConfigFile( ConfigFilePtr pConfigFile );
Sint32 SaveConfigFileAs( ConfigFilePtr pConfigFile, const char *pPath );


#endif // #ifndef _CONFIGFILE_H_

