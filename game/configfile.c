/* Egoboo - configfile.c
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

//---------------------------------------------------------------------
//
// ConfigFile.c
//
// - All functions to manage a ConfigFile
//
//  A ConfigFile contains sections which themselves contains values.
// A section name is contained between "{}" Ex: {Section Name}
//
// A value has a Key, a string value and an optional commentary.
// The Key is contained between "[]" Ex: [Key Name]
// The string value is contained between '"' Ex: "TRUE"
//  To include a '"' in a string value, the '"' are doubled
//  The commentary follows the string value on the same line and
// begins with "//"
//
// Exemple of a ConfigFile:
// {Section 1}
// [Key1] : "TRUE" // This is a commentary
// [Key2] : "Hello ""MAN""" // this will become : Hello "MAN"
//
// To do :
//   optimisation
//   error checking
//   Run-time commentary editing
//
// Bugs:
//  - Multiple section with the same name will be loaded and saved but only the first
//   one will be looked for value. Should not load sections with same name.
//
//
// History:
//
// 2001-01-09
//  - Implemented ConvertToKeyCharacters in SetConfigValue
//
// 2000-12-10
//  - Added the length of the string buffer used as parameter for GetConfigValue.
//  - Added SetConfigBooleanValue, SetConfigIntVaalue and SetConfigFloatValue to standardise
//   the way to set usual data types.
//  - Added GetConfigBooleanValue, GetConfigIntValue and GetConfigFloatValue.
//---------------------------------------------------------------------


#include "proto.h"
#include "configfile.h"
#include "egoboo_strutil.h"

// Change any non alphanumeric character or space or underscore to an underscore
// It is use for the section and key name
void ConvertToKeyCharacters( char *pStr )
{

  if ( pStr == NULL )
  {
    return;
  }

  str_convert_spaces( pStr, ( size_t )( -1 ), pStr );
}


// PassOverConfigCommentary reads the pConfigFile's file until the end of a line.
Sint32 PassOverConfigCommentary( ConfigFilePtr pConfigFile )
{
  char lc;

  lc = fgetc( pConfigFile->f );
  while ( lc != 13 && lc != 10 && 0 == feof( pConfigFile->f ) )
  {
    lc = fgetc( pConfigFile->f );
  }
  return 0;
}

// ReadSectionName reads characters from the config file until it encounters
// an end of section name "}". The resulting string is copied in pSection->SectionName.
// The length of the name is returned, or 0 if there is an error.
//
// if the string is longer than MAX_CONFIG_SECTION_LENGTH, the string is truncated
//
Sint32 ReadConfigSectionName( ConfigFilePtr pConfigFile, ConfigFileSectionPtr pSection )
{
  Sint32 lLenghtName = 0;
  char lc;

  lc = fgetc( pConfigFile->f );
  memset( pSection->SectionName, 0, MAX_CONFIG_SECTION_LENGTH );
  while ( lc != '}' && 0 == feof( pConfigFile->f ) )
  {
    if ( lLenghtName < MAX_CONFIG_SECTION_LENGTH )
    {
      pSection->SectionName[lLenghtName] = lc;
    }
    lLenghtName++;
    lc = fgetc( pConfigFile->f );
  }
  if ( feof( pConfigFile->f ) )
  {
    return 0;
  }
  return lLenghtName;
}

// ReadKeyName reads characters from the config file until it encounters
// an end of key name "]". The resulting string is copied in pValue->KeyName.
// The length of the name is returned, or 0 if there is an error.
//
// if the string is longuer than MAX_CONFIG_KEY_LENGTH, the string is truncated
//
long ReadConfigKeyName( ConfigFilePtr pConfigFile, ConfigFileValuePtr pValue )
{
  Sint32 lLenghtName = 0;
  char lc;

  lc = fgetc( pConfigFile->f );
  memset( pValue->KeyName, 0, MAX_CONFIG_KEY_LENGTH );
  while ( lc != ']' && 0 == feof( pConfigFile->f ) )
  {
    if ( lLenghtName < MAX_CONFIG_KEY_LENGTH )
    {
      pValue->KeyName[lLenghtName] = lc;
    }
    lLenghtName++;
    lc = fgetc( pConfigFile->f );
  }
  if ( feof( pConfigFile->f ) )
  {
    return 0;
  }
  return lLenghtName;
}

// ReadConfigValue reads characters from the config file until it encounters
// an end of value '"'. The resulting string is copied in pValue->Value.
// The length of the value is returned, or -1 if there is an error.
//
// The memory for pValue->Value is allocated here.
//
Sint32 ReadConfigValue( ConfigFilePtr pConfigFile, ConfigFileValuePtr pValue )
{
  static char lTempStr[MAX_CONFIG_VALUE_LENGTH];
  char lc;
  Sint32  lEndScan = 0;
  Sint32  lState = 0;
  Sint32 lLenghtName = 0;

  memset( lTempStr, 0, MAX_CONFIG_VALUE_LENGTH );
  while ( 0 == lEndScan )
  {
    lc = fgetc( pConfigFile->f );
    switch ( lState )
    {
      case 0:
        // search for end of string '"'
        if ( lc == '"' )
        {
          // state change :
          lState = 1;
        }
        else if ( lc == 13 || lc == 10 || feof( pConfigFile->f ) )
        {
          // error
          lEndScan = 1;
        }
        else
        {
          // add in string
          if ( lLenghtName < MAX_CONFIG_VALUE_LENGTH )
          {
            lTempStr[lLenghtName] = lc;
            lLenghtName++;
          }
        }
        break;

      case 1:
        // check if really end of string
        if ( lc == '"' )
        {
          // add '"' in string
          if ( lLenghtName < MAX_CONFIG_VALUE_LENGTH )
          {
            lTempStr[lLenghtName] = lc;
            lLenghtName++;
          }
          lState = 0;
          // continue scan
        }
        else
        {
          // restore the char for next scan
          ungetc( lc, pConfigFile->f );
          // succesfull scan
          // allocate memory for value
          pValue->Value = ( char * ) malloc( lLenghtName + 1 );
          // copy string
          strcpy( pValue->Value, lTempStr );
          // exit scan
          lEndScan = 1;
        }
        break;

      default:
        // error
        lEndScan = 1;
        break;

    }
  }
  return lLenghtName;
}

// ReadConfigCommentary reads characters from the config file until it encounters
// an end of commentary chr(13). The resulting string is copied in pValue->Value.
// The length of the value is returned, or -1 if there is an error.
//
// The memory for pValue->Commentary is allocated here.
//
Sint32 ReadConfigCommentary( ConfigFilePtr pConfigFile, ConfigFileValuePtr pValue )
{
  static char lTempStr[MAX_CONFIG_COMMENTARY_LENGTH];
  char lc;
  Sint32  lEndScan = 0;
  Sint32  lState = 0;
  Sint32 lLenghtName = 0;

  memset( lTempStr, 0, MAX_CONFIG_COMMENTARY_LENGTH );
  while ( 0 == lEndScan )
  {
    lc = fgetc( pConfigFile->f );
    switch ( lState )
    {
      case 0:
        // search for end of string '"'
        if ( lc == '/' ||  lc == ' ' )
        {
          // continue scan until a letter appears

        }
        else if ( lc == 13 || lc == 10 || feof( pConfigFile->f ) )
        {
          // error
          lEndScan = 1;
        }
        else
        {
          // add in string
          if ( lLenghtName < MAX_CONFIG_COMMENTARY_LENGTH )
          {
            lTempStr[lLenghtName] = lc;
            lLenghtName++;
          }
          // state change : continu until endl
          lState = 1;
        }
        break;

      case 1:
        // check if really end of string
        if ( lc == 13 || lc == 10 || feof( pConfigFile->f ) )
        {
          // allocate memory for commentary
          pValue->Commentary = ( char * ) malloc( lLenghtName + 1 );
          // copy string
          strcpy( pValue->Commentary, lTempStr );
          // exit scan
          lEndScan = 1;
        }
        else
        {
          // add in string
          if ( lLenghtName < MAX_CONFIG_COMMENTARY_LENGTH )
          {
            lTempStr[lLenghtName] = lc;
            lLenghtName++;
          }
        }
        break;

      default:
        // error
        lEndScan = 1;
        break;

    }
  }
  return lLenghtName;
}

// OpenConfigFile opens a ConfigFile for reading values.
//
// If the path doesn't exist, OpenConfigFile returns NULL.
// OpenConfigFile allocates the memory for the ConfigFile. To
// deallocate the memory, use CloseConfigFile
ConfigFilePtr OpenConfigFile( const char *pPath )
{
  ConfigFilePtr   lTempConfig = NULL;
  FILE     *lTempFile;
  ConfigFileSectionPtr lCurSection = NULL;
  ConfigFileValuePtr  lCurValue = NULL;
  Sint32      lError = 0;
  Sint32      lState = 0;
  char     lc;

  lTempFile = fs_fileOpen( PRI_NONE, NULL, pPath, "rt+" );
  if ( lTempFile != NULL )
  {
    lTempConfig = ( ConfigFilePtr ) malloc( sizeof( ConfigFile ) );
    lTempConfig->f = lTempFile;
    lTempConfig->ConfigSectionList = NULL;
    lTempConfig->CurrentSection = NULL;
    lTempConfig->CurrentValue = NULL;

    // load all values in memory
    while ( 0 == lError && !feof( lTempConfig->f ) )
    {
      lc = fgetc( lTempConfig->f );
      switch ( lState )
      {
        case 0:
          // search for section ( start )
          if ( lc == '{' )
          {
            // create first section and load name
            lTempConfig->ConfigSectionList = ( ConfigFileSectionPtr ) malloc( sizeof( ConfigFileSection ) );
            memset( lTempConfig->ConfigSectionList, 0, sizeof( ConfigFileSection ) );
            lCurSection = lTempConfig->ConfigSectionList;
            ReadConfigSectionName( lTempConfig, lTempConfig->ConfigSectionList );
            lCurValue = NULL; // just to be safe
            // state change : look for new value or section
            lState = 1;
          }
          else if ( lc == '/' )
          {
            // pass over commentary ( bad!!! will be lost )
            PassOverConfigCommentary( lTempConfig );
          }
          break;

        case 1:
          // search for next value ( key name ) or next section
          if ( lc == '{' )
          {
            // create new section and load name
            lCurSection->NextSection = ( ConfigFileSectionPtr ) malloc( sizeof( ConfigFileSection ) );
            lCurSection = lCurSection->NextSection;
            memset( lCurSection, 0, sizeof( ConfigFileSection ) );
            ReadConfigSectionName( lTempConfig, lCurSection );
            lCurValue = NULL;
          }
          else if ( lc == '[' )
          {
            // create new value in current section and load key name
            if ( lCurValue == NULL )
            {
              // first value in section
              lCurSection->FirstValue = ( ConfigFileValuePtr ) malloc( sizeof( ConfigFileValue ) );
              lCurValue = lCurSection->FirstValue;
            }
            else
            {
              // link to new value
              lCurValue->NextValue = ( ConfigFileValuePtr ) malloc( sizeof( ConfigFileValue ) );
              lCurValue = lCurValue->NextValue;
            }
            memset( lCurValue, 0, sizeof( ConfigFileValue ) );
            ReadConfigKeyName( lTempConfig, lCurValue );

            // state change : get value
            lState = 2;
          }
          else if ( lc == '/' )
          {
            // pass over commentary ( bad!!! will be lost )
            PassOverConfigCommentary( lTempConfig );
          }
          break;

        case 2:
          // search for value
          if ( lc == '"' )
          {
            // load value in current value
            ReadConfigValue( lTempConfig, lCurValue );
            // state change : look for commentary
            lState = 3;
          }
          break;

        case 3:
          // search for commentary
          if ( lc == '/' )
          {
            //load commentary in current value
            ReadConfigCommentary( lTempConfig, lCurValue );
            // state change : look for new value or section
            lState = 1;
          }
          else if ( lc == 10 || lc == 13 )
          {
            // state change : look for new value or section
            lState = 1;
          }
          break;

        case 100:
          // error
          lError = 1;
          break;

        default:
          // error
          lError = 1;
          break;

      }
    }
  }
  return lTempConfig;
}

// SetConfigCurrentSection
//
// Set the current section of pConfigFile to the one specified by pSection
// If the section doesn't exist, the current section is set to NULL and the
// function returns 0, otherwise the it returns 1.
long SetConfigCurrentSection( ConfigFilePtr pConfigFile, const char *pSection )
{
  long lFound = 0;
  if ( pConfigFile == NULL || pSection == NULL )
  {
    return 0;
  }
  if ( pConfigFile->CurrentSection == NULL )
  {
    pConfigFile->CurrentSection = pConfigFile->ConfigSectionList;
    while ( 0 == lFound && ( pConfigFile->CurrentSection != NULL ) )
    {
      if ( strcmp( pConfigFile->CurrentSection->SectionName, pSection ) == 0 )
      {
        lFound = 1;
      }
      else
      {
        pConfigFile->CurrentSection = pConfigFile->CurrentSection->NextSection;
      }
    }
  }
  else
  {
    if ( strcmp( pConfigFile->CurrentSection->SectionName, pSection ) == 0 )
    {
      lFound = 1;
    }
    else
    {
      pConfigFile->CurrentSection = pConfigFile->ConfigSectionList;
      while ( 0 == lFound && ( pConfigFile->CurrentSection != NULL ) )
      {
        if ( strcmp( pConfigFile->CurrentSection->SectionName, pSection ) == 0 )
        {
          lFound = 1;
        }
        else
        {
          pConfigFile->CurrentSection = pConfigFile->CurrentSection->NextSection;
        }
      }
    }
  }
  return lFound;
}

// SetConfigCurrentValueFromCurrentSection looks for a value in the current
// section having pKey as KeyName
//
// If a value is found, the function returns 1, otherwise the function returns 0
// and CurrentValue is set to NULL.
Sint32 SetConfigCurrentValueFromCurrentSection( ConfigFilePtr pConfigFile, const char *pKey )
{
  Sint32 lFound = 0;
  if ( pConfigFile == NULL || pKey == NULL )
  {
    return 0;
  }
  if ( pConfigFile->CurrentSection == NULL )
  {
    return 0;
  }

  // search for a value->KeyName = pKey in current section
  pConfigFile->CurrentValue = pConfigFile->CurrentSection->FirstValue;
  while ( 0 == lFound && ( pConfigFile->CurrentValue != NULL ) )
  {
    if ( strcmp( pConfigFile->CurrentValue->KeyName, pKey ) == 0 )
    {
      lFound = 1;
    }
    else
    {
      pConfigFile->CurrentValue = pConfigFile->CurrentValue->NextValue;
    }
  }
  return lFound;
}

// SetConfigCurrentValue set the current value of pConfigFile to the one specified by
// pSection and pKey
// Returns 0 if failed
Sint32 SetConfigCurrentValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey )
{
  Sint32 lFound = 0;
  // search for section
  lFound = SetConfigCurrentSection( pConfigFile, pSection );
  if ( 1 == lFound )
  {
    // search for key in current section
    lFound = SetConfigCurrentValueFromCurrentSection( pConfigFile, pKey );
  }
  return lFound;
}

// GetConfigValue search for the value from section pSection with the key pKey.
//
// If the value is found, the value is copied in pValue and the function returns 1.
// If the length of pValue is less than the length of the string value, the string is truncated.
// If the value isn't found, the function returns 0.
Sint32 GetConfigValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, char *pValue,
                       Sint32 pValueBufferLength )
{
  if ( pConfigFile == NULL || pValue == NULL || pSection == NULL || pKey == NULL || pValueBufferLength <= 0 )
  {
    return 0;
  }

  if ( 0 == SetConfigCurrentValue( pConfigFile, pSection, pKey ) )
  {
    return 0;
  }
  if ( strlen( pConfigFile->CurrentValue->Value ) >= ( size_t ) pValueBufferLength )
  {
    strncpy( pValue, pConfigFile->CurrentValue->Value, pValueBufferLength - 1 );
    pValue[pValueBufferLength] = 0;
  }
  else
  {
    strcpy( pValue, pConfigFile->CurrentValue->Value );
  }
  return 1;
}

// GetConfigBooleanValue set to true or false pBool. If the function can't find the value, it
// returns 0. If the value can't be identified as true or false, the default is false.
Sint32 GetConfigBooleanValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, bool_t *pBool )
{
  char lBoolStr[16];
  Sint32 lRet;

  memset( lBoolStr, 0, 16 );
  lRet = GetConfigValue( pConfigFile, pSection, pKey, lBoolStr, 16 );
  if ( lRet != 0 )
  {
    // check for true
    if ( strcmp( lBoolStr, "TRUE" ) == 0 )
    {
      *pBool = btrue;
    }
    else
    {
      *pBool = bfalse;
    }
  }
  return lRet;
}

// GetConfigIntValue set pInt. If the function can't find the value, it
// returns 0.
Sint32 GetConfigIntValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, Sint32 *pInt )
{
  char lIntStr[24];
  Sint32 lRet;

  memset( lIntStr, 0, 16 );
  lRet = GetConfigValue( pConfigFile, pSection, pKey, lIntStr, 24 );
  if ( lRet != 0 )
  {
    // convert value
    sscanf( lIntStr, "%d", pInt );
  }
  return lRet;
}

// SetConfigValue set the value specified by pSection and pKey. If the value
// doesn't exist, it is created
Sint32 SetConfigValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, const char *pValue )
{
  ConfigFileSectionPtr lTempSection = NULL;
  ConfigFileValuePtr  lTempValue = NULL;
  Sint32 lOK = 0;
  Sint32 lLentghtNewValue, lLengthValue;
  char     lNewSectionName[MAX_CONFIG_SECTION_LENGTH];
  char     lNewKeyName[MAX_CONFIG_KEY_LENGTH];

  if ( pConfigFile == NULL )
  {
    return 0;
  }
  if ( pValue == NULL || pSection == NULL || pKey == NULL )
  {
    return 0;
  }

  // make sure section name and key name are made of valid char
  strcpy( lNewSectionName, pSection );
  strcpy( lNewKeyName, pKey );
  ConvertToKeyCharacters( lNewSectionName );
  ConvertToKeyCharacters( lNewKeyName );
  // look for section = pSection
  if (( lOK = SetConfigCurrentSection( pConfigFile, lNewSectionName ) )  == 0 )
  {
    // section doesn't exist so create it and create value
    lTempSection = ( ConfigFileSectionPtr ) malloc( sizeof( ConfigFileValue ) );
    memset( lTempSection, 0, sizeof( ConfigFileValue ) );
    lTempSection->NextSection = pConfigFile->ConfigSectionList;
    pConfigFile->ConfigSectionList = lTempSection;
    strcpy( lTempSection->SectionName, lNewSectionName );

    // create the new value
    lTempValue = ( ConfigFileValuePtr ) malloc( sizeof( ConfigFileValue ) );
    memset( lTempValue, 0, sizeof( ConfigFileValue ) );
    lTempSection->FirstValue = lTempValue;
    strcpy( lTempValue->KeyName, lNewKeyName );

    // set current section and value
    pConfigFile->CurrentSection = lTempSection;
    pConfigFile->CurrentValue = lTempValue;
  }

  // if the section already existed
  if ( lOK )
  {
    // search for value
    if ( 0 == SetConfigCurrentValueFromCurrentSection( pConfigFile, lNewKeyName ) )
    {
      // create new value in current section
      lTempValue = ( ConfigFileValuePtr ) malloc( sizeof( ConfigFileValue ) );
      memset( lTempValue, 0, sizeof( ConfigFileValue ) );
      lTempValue->NextValue = pConfigFile->CurrentSection->FirstValue;
      pConfigFile->CurrentSection->FirstValue = lTempValue;
      strcpy( lTempValue->KeyName, lNewKeyName );

      // set current section and value
      pConfigFile->CurrentValue = lTempValue;
    }
  }

  lLentghtNewValue = ( Sint32 ) strlen( pValue );
  if ( pConfigFile->CurrentValue->Value == NULL )
  {
    // if the stirng value doesn't exist than allocate memory for it
    pConfigFile->CurrentValue->Value = ( char * ) malloc( lLentghtNewValue  + 1 );
    memset( pConfigFile->CurrentValue->Value, 0, lLentghtNewValue + 1 );
  }
  else
  {
    lLengthValue = sizeof( pConfigFile->CurrentValue->Value );
    // if value already exist, verify if allocated memory is enough
    // if not allocate more memory
    if ( lLentghtNewValue >= MAX_CONFIG_VALUE_LENGTH )
    {
      lLentghtNewValue = MAX_CONFIG_VALUE_LENGTH - 1;
    }
    if ( lLentghtNewValue >= lLengthValue )
    {
      FREE( pConfigFile->CurrentValue->Value );
      pConfigFile->CurrentValue->Value = ( char * ) malloc( lLentghtNewValue  + 1 );
      memset( pConfigFile->CurrentValue->Value, 0, lLentghtNewValue + 1 );
    }
    else
    {
      memset( pConfigFile->CurrentValue->Value, 0, lLengthValue );
    }
  }
  strncpy( pConfigFile->CurrentValue->Value, pValue, lLentghtNewValue );
  return 1;
}

// SetConfigBooleanValue saves a boolean in a value specified by pSection and pKey
Sint32 SetConfigBooleanValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, bool_t pBool )
{
  if ( pBool )
  {
    // save the value with btrue
    return SetConfigValue( pConfigFile, pSection, pKey, "TRUE" );
  }

  // since it's false
  return SetConfigValue( pConfigFile, pSection, pKey, "FALSE" );
}

// SetConfigIntValue saves an integer in a value specified by pSection and pKey
Sint32 SetConfigIntValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, int pInt )
{
  static char lIntStr[16];

  snprintf( lIntStr, sizeof( lIntStr ), "%i", pInt );
  return SetConfigValue( pConfigFile, pSection, pKey, lIntStr );
}

// SetConfigFloatValue saves a float in a value specified by pSection and pKey
Sint32 SetConfigFloatValue( ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, float pFloat )
{
  static char lFloatStr[16];

  snprintf( lFloatStr, sizeof( lFloatStr ), "%f", pFloat );
  return SetConfigValue( pConfigFile, pSection, pKey, lFloatStr );
}

// CloseConfigFile close the ConfigFile and deallocate all its memory.
void CloseConfigFile( ConfigFilePtr pConfigFile )
{
  ConfigFileSectionPtr lTempSection, lDoomedSection;
  ConfigFileValuePtr   lTempValue, lDoomedValue;

  if ( pConfigFile == NULL )
  {
    return;
  }
  if ( pConfigFile->f != NULL )
  {
    fs_fileClose( pConfigFile->f );
  }
  // delete all sections form the ConfigFile
  lTempSection = pConfigFile->ConfigSectionList;
  while ( lTempSection != NULL )
  {
    // delete all values form the current section
    lTempValue = lTempSection->FirstValue;
    while ( lTempValue != NULL )
    {
      FREE ( lTempValue->Value );
      FREE ( lTempValue->Commentary );
      lDoomedValue = lTempValue;
      lTempValue = lTempValue->NextValue;
      FREE( lDoomedValue );
    }
    lDoomedSection = lTempSection;
    lTempSection = lTempSection->NextSection;
    FREE( lDoomedSection );
  }
  FREE( pConfigFile );
}

// SaveConfigValue saves the value from pValue at the current position
// of the pConfigFile file. The '"' are doubled.
Sint32 SaveConfigValue( FILE *pFile, ConfigFileValuePtr pValue )
{
  Sint32 lPos = 0;

  if ( pValue->Value != NULL )
  {
    fputc( '"', pFile );
    while ( pValue->Value[lPos] != 0 )
    {
      fputc( pValue->Value[lPos], pFile );
      if ( pValue->Value[lPos] == '"' )
      {
        // double the '"'
        fputc( '"', pFile );
      }
      lPos++;
    }
    fputc( '"', pFile );
  }
  return 0;
}

// SaveConfigFile
void SaveConfigFile( ConfigFilePtr pConfigFile )
{
  ConfigFileSectionPtr lTempSection;
  ConfigFileValuePtr   lTempValue;

  if ( pConfigFile == NULL )
  {
    return;
  }
  if ( pConfigFile->f == NULL )
  {
    return;
  }

  lTempSection = pConfigFile->ConfigSectionList;
  // rewrite the file
  rewind( pConfigFile->f );
  // saves all sections
  while ( lTempSection != NULL )
  {
    fprintf( pConfigFile->f, "{%s}\n", lTempSection->SectionName );
    // saves all values form the current section
    lTempValue = lTempSection->FirstValue;
    while ( lTempValue != NULL )
    {
      fprintf( pConfigFile->f, "[%s] : ", lTempValue->KeyName );
      if ( lTempValue->Value != NULL )
      {
        SaveConfigValue( pConfigFile->f, lTempValue );
      }
      if ( lTempValue->Commentary != NULL )
      {
        fprintf( pConfigFile->f, " // %s", lTempValue->Commentary );
      }
      fprintf( pConfigFile->f, "\n" );
      lTempValue = lTempValue->NextValue;
    }
    fprintf( pConfigFile->f, "\n" );
    lTempSection = lTempSection->NextSection;
  }
}

// SaveConfigFileAs saves pConfigFile at pPath
// pConfigFile's file is close and set to the new file
Sint32 SaveConfigFileAs( ConfigFilePtr pConfigFile, const char *pPath )
{
  FILE *lFileTemp;

  lFileTemp = fs_fileOpen( PRI_NONE, NULL, pPath, "wt" );
  if ( lFileTemp == NULL )
  {
    return 0;
  }
  if ( pConfigFile->f != NULL )
  {
    fs_fileClose( pConfigFile->f );
  }
  pConfigFile->f = lFileTemp;
  SaveConfigFile( pConfigFile );
  return 1;
}
