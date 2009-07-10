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

///
/// @file
/// @brief Configuration file loading implementation
/// @details All functions to manage a ConfigFile
///
/// A ConfigFile_t contains sections which themselves contains values.
/// A section name is contained between "{}" Ex: {Section Name}
///
/// A value has a Key, a string value and an optional commentary.
/// The Key is contained between "[]" Ex: [Key Name]
/// The string value is contained between '"' Ex: "TRUE"
///  To include a '"' in a string value, the '"' are doubled
///  The commentary follows the string value on the same line and
/// begins with "//"
///
/// Exemple of a ConfigFile_t:
/// {Section 1}
/// [Key1] : "TRUE" // This is a commentary
/// [Key2] : "Hello ""MAN""" // this will become : Hello "MAN"
///
/// @todo optimisation
/// @todo error checking
/// @todo Run-time commentary editing
///
/// @bug Multiple section with the same name will be loaded and saved but only the first
///   one will be looked for value. Should not load sections with same name.
///
///
/// History:
///
/// 2001-01-09
///  @li Implemented ConfigFileString_Encode in ConfigFile_SetValue_String
///
/// 2000-12-10
///  @li Added the length of the string buffer used as parameter for ConfigFile_GetValue_String.
///  @li Added ConfigFile_SetValue_Boolean, SetConfigIntValue and ConfigFile_SetValue_Float to standardise
///      the way to set usual data types.
///  @li Added ConfigFile_GetValue_Boolean, ConfigFile_GetValue_Int and GetConfigFloatValue.

#include "configfile.h"

#include "egoboo_typedef.h"
#include "egoboo_strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// util
static void ConfigFileString_Encode( char *pStr );

//
static ConfigFilePtr_t   ConfigFile_open( ConfigFilePtr_t pConfigFile, const char *szFileName, const char * szAttribute );
static ConfigFile_retval ConfigFile_close( ConfigFilePtr_t pConfigFile );

//
static ConfigFile_retval ConfigFile_read( ConfigFilePtr_t pConfigFile );
static ConfigFile_retval ConfigFile_write( ConfigFilePtr_t pConfigFile );

static ConfigFileSectionPtr_t ConfigFileSection_create();
static ConfigFile_retval      ConfigFileSection_destroy(ConfigFileSectionPtr_t * ptmp);

static ConfigFileValuePtr_t   ConfigFileValue_create();
static ConfigFile_retval      ConfigFileValue_destroy(ConfigFileValuePtr_t * ptmp);

static size_t ConfigFile_ReadSectionName( ConfigFilePtr_t pConfigFile, ConfigFileSectionPtr_t pSection );
static size_t ConfigFile_ReadKeyName( ConfigFilePtr_t pConfigFile, ConfigFileValuePtr_t pValue );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// pseudo functions for adding C++-like new and delete to C

#define CONFIG_NEW( TYPE ) (TYPE *)calloc(1, sizeof(TYPE))
#define CONFIG_NEW_ARY( TYPE, COUNT ) (TYPE *)calloc(COUNT, sizeof(TYPE))
#define CONFIG_DELETE(PTR) if(NULL != PTR) { free(PTR); PTR = NULL; }
#define CONFIG_DELETE_ARY(PTR) if(NULL != PTR) { free(PTR); PTR = NULL; }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ConfigFilePtr_t ConfigFile_create()
{
    ConfigFilePtr_t ptmp = NULL;

    // allocate the data
    ptmp = CONFIG_NEW( ConfigFile_t );
    if (NULL == ptmp) return ptmp;

    // set the file
    ptmp->f           = NULL;
    ptmp->filename[0] = '\0';

    return ptmp;
}

//--------------------------------------------------------------------------------------------
ConfigFile_retval ConfigFile_destroy(ConfigFilePtr_t * ppConfigFile )
{
    ConfigFileSectionPtr_t lTempSection, lDoomedSection;
    if (NULL == ppConfigFile || NULL == *ppConfigFile) return ConfigFile_fail;

    ConfigFile_close( *ppConfigFile );
    (*ppConfigFile)->filename[0] = '\0';

    // delete all sections form the ConfigFile_t
    lTempSection = (*ppConfigFile)->ConfigSectionList;

    while ( NULL != lTempSection  )
    {
        lDoomedSection = lTempSection;
        lTempSection   = lTempSection->NextSection;
        ConfigFileSection_destroy( &lDoomedSection );
    }

    // free the data
    CONFIG_DELETE( *ppConfigFile );

    return ConfigFile_succeed;
}

//--------------------------------------------------------------------------------------------
ConfigFileSectionPtr_t ConfigFile_createSection( ConfigFilePtr_t f, ConfigFileSectionPtr_t s )
{
    ConfigFileSectionPtr_t pnew;

    // create a blank section
    pnew = ConfigFileSection_create();

    // link the new section into the correct place
    if (NULL == s)
    {
        f->ConfigSectionList = pnew;
    }
    else
    {
        s->NextSection = pnew;
    }

    s = pnew;

    // read the section name from the file
    ConfigFile_ReadSectionName( f, pnew );

    // return the proper section
    return s;
}

//--------------------------------------------------------------------------------------------
ConfigFileValuePtr_t ConfigFile_createValue( ConfigFilePtr_t f, ConfigFileSectionPtr_t s, ConfigFileValuePtr_t v)
{
    ConfigFileValuePtr_t pnew;

    // create new value
    pnew = ConfigFileValue_create();

    // link the new value into the correct section
    if ( NULL == v  )
    {
        // first value in section
        s->FirstValue = pnew;
    }
    else
    {
        // link to new value
        v->NextValue = pnew;
    }

    v = pnew;

    // read the data from the file
    ConfigFile_ReadKeyName( f, pnew );

    // return the proper value
    return v;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
char * ConfigFileString_create(size_t len)
{
    char * ptmp;

    ptmp = CONFIG_NEW_ARY(char, len);

    return ptmp;
}

//--------------------------------------------------------------------------------------------
ConfigFile_retval ConfigFileString_destroy(char ** ptmp )
{
    if (NULL == ptmp || NULL == *ptmp) return ConfigFile_fail;

    CONFIG_DELETE( *ptmp );

    return ConfigFile_succeed;
}

//--------------------------------------------------------------------------------------------
char * ConfigFileString_resize(char * str, size_t new_len )
{
    size_t old_len = sizeof( str );

    // if value already exist, verify if allocated memory is enough
    // if not allocate more memory
    if ( new_len >= MAX_CONFIG_VALUE_LENGTH )
    {
        new_len = MAX_CONFIG_VALUE_LENGTH - 1;
    }
    if ( new_len >= old_len )
    {
        str = (char *)realloc(str, new_len);
    }

    memset( str, 0, new_len );

    return str;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ConfigFileValuePtr_t ConfigFileValue_create()
{
    ConfigFileValuePtr_t ptmp;

    ptmp = CONFIG_NEW(ConfigFileValue_t);

    return ptmp;
}

//--------------------------------------------------------------------------------------------
ConfigFile_retval ConfigFileValue_destroy(ConfigFileValuePtr_t * ptmp)
{
    if (NULL == ptmp || NULL == *ptmp) return ConfigFile_fail;

    ConfigFileString_destroy( &(*ptmp)->Commentary );
    ConfigFileString_destroy( &(*ptmp)->Value      );

    CONFIG_DELETE( *ptmp );

    return ConfigFile_succeed;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ConfigFileSectionPtr_t ConfigFileSection_create()
{
    ConfigFileSectionPtr_t ptmp;

    ptmp = CONFIG_NEW(ConfigFileSection_t);

    return ptmp;
}

//--------------------------------------------------------------------------------------------
ConfigFile_retval ConfigFileSection_destroy(ConfigFileSectionPtr_t * ptmp)
{
    ConfigFileValuePtr_t lTempValue, lDoomedValue;
    if (NULL == ptmp || NULL == *ptmp) return ConfigFile_fail;

    // delete all values from the current section
    lTempValue = (*ptmp)->FirstValue;

    while ( NULL != lTempValue  )
    {
        lDoomedValue = lTempValue;
        lTempValue   = lTempValue->NextValue;
        ConfigFileValue_destroy( &lDoomedValue );
    }

    CONFIG_DELETE( *ptmp );

    return ConfigFile_succeed;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Change any non alphanumeric character or space or underscore to an underscore
// It is use for the section and key name
void ConfigFileString_Encode( char *pStr )
{
    if ( NULL == pStr  ) return;

    str_encode( pStr, ( size_t )( -1 ), pStr );
}

//--------------------------------------------------------------------------------------------
// ConfigFile_PassOverCommentary reads the pConfigFile's file until the end of a line.
ConfigFile_retval ConfigFile_PassOverCommentary( ConfigFilePtr_t pConfigFile )
{
    char lc;

    lc = fgetc( pConfigFile->f );

    while ( lc != 13 && lc != 10 && 0 == feof( pConfigFile->f ) )
    {
        lc = fgetc( pConfigFile->f );
    }

    return ConfigFile_fail;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_ReadSectionName reads characters from the config file until it encounters
// a _dtor of section name "}". The resulting string is copied in pSection->SectionName.
// The length of the name is returned, or 0 if there is an error.
//
// if the string is longer than MAX_CONFIG_SECTION_LENGTH, the string is truncated
//
size_t ConfigFile_ReadSectionName( ConfigFilePtr_t pConfigFile, ConfigFileSectionPtr_t pSection )
{
    size_t lLengthName = 0;
    char lc;

    lc = fgetc( pConfigFile->f );
    memset( pSection->SectionName, 0, MAX_CONFIG_SECTION_LENGTH );

    while ( lc != '}' && 0 == feof( pConfigFile->f ) )
    {
        if ( lLengthName < MAX_CONFIG_SECTION_LENGTH )
        {
            pSection->SectionName[lLengthName] = lc;
        }

        lLengthName++;
        lc = fgetc( pConfigFile->f );
    }
    if ( feof( pConfigFile->f ) )
    {
        return 0;
    }

    return lLengthName;
}

//--------------------------------------------------------------------------------------------
// ReadKeyName reads characters from the config file until it encounters
// a _dtor of key name "]". The resulting string is copied in pValue->KeyName.
// The length of the name is returned, or 0 if there is an error.
//
// if the string is longuer than MAX_CONFIG_KEY_LENGTH, the string is truncated
//
size_t ConfigFile_ReadKeyName( ConfigFilePtr_t pConfigFile, ConfigFileValuePtr_t pValue )
{
    size_t lLengthName = 0;
    char lc;

    lc = fgetc( pConfigFile->f );
    memset( pValue->KeyName, 0, MAX_CONFIG_KEY_LENGTH );

    while ( lc != ']' && 0 == feof( pConfigFile->f ) )
    {
        if ( lLengthName < MAX_CONFIG_KEY_LENGTH )
        {
            pValue->KeyName[lLengthName] = lc;
        }

        lLengthName++;
        lc = fgetc( pConfigFile->f );
    }
    if ( feof( pConfigFile->f ) )
    {
        return ConfigFile_fail;
    }

    return lLengthName;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_ReadValue reads characters from the config file until it encounters
// a _dtor of value '"'. The resulting string is copied in pValue->Value.
// The length of the value is returned, or -1 if there is an error.
//
// The memory for pValue->Value is allocated here.
//
ConfigFile_retval ConfigFile_ReadValue( ConfigFilePtr_t pConfigFile, ConfigFileValuePtr_t pValue )
{
    static char lTempStr[MAX_CONFIG_VALUE_LENGTH];
    char lc;
    Sint32  lEndScan = 0;
    Sint32  lState = 0;
    Sint32 lLengthName = 0;

    memset( lTempStr, 0, MAX_CONFIG_VALUE_LENGTH );

    while ( 0 == lEndScan )
    {
        lc = fgetc( pConfigFile->f );

        switch ( lState )
        {
            case 0:

                // search for _dtor of string '"'
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
                    if ( lLengthName < MAX_CONFIG_VALUE_LENGTH )
                    {
                        lTempStr[lLengthName] = lc;
                        lLengthName++;
                    }
                }
                break;

            case 1:

                // check if really _dtor of string
                if ( lc == '"' )
                {
                    // add '"' in string
                    if ( lLengthName < MAX_CONFIG_VALUE_LENGTH )
                    {
                        lTempStr[lLengthName] = lc;
                        lLengthName++;
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
                    pValue->Value = CONFIG_NEW_ARY( char, lLengthName + 1 );
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

    return lLengthName;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_ReadCommentary reads characters from the config file until it encounters
// an end of commentary chr(13). The resulting string is copied in pValue->Value.
// The length of the value is returned, or -1 if there is an error.
//
// The memory for pValue->Commentary is allocated here.
//
ConfigFile_retval ConfigFile_ReadCommentary( ConfigFilePtr_t pConfigFile, ConfigFileValuePtr_t pValue )
{
    static char lTempStr[MAX_CONFIG_COMMENTARY_LENGTH];
    char lc;
    Sint32  lEndScan = 0;
    Sint32  lState = 0;
    Sint32 lLengthName = 0;
    if (NULL == pValue) return ConfigFile_PassOverCommentary( pConfigFile );

    memset( lTempStr, 0, MAX_CONFIG_COMMENTARY_LENGTH );

    while ( 0 == lEndScan )
    {
        lc = fgetc( pConfigFile->f );

        switch ( lState )
        {
            case 0:

                // search for _dtor of string '"'
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
                    if ( lLengthName < MAX_CONFIG_COMMENTARY_LENGTH )
                    {
                        lTempStr[lLengthName] = lc;
                        lLengthName++;
                    }

                    // state change : continu until endl
                    lState = 1;
                }
                break;

            case 1:

                // check if really _dtor of string
                if ( lc == 13 || lc == 10 || feof( pConfigFile->f ) )
                {
                    // allocate memory for commentary
                    pValue->Commentary = CONFIG_NEW_ARY( char, lLengthName + 1 );
                    // copy string
                    strcpy( pValue->Commentary, lTempStr );
                    // exit scan
                    lEndScan = 1;
                }
                else
                {
                    // add in string
                    if ( lLengthName < MAX_CONFIG_COMMENTARY_LENGTH )
                    {
                        lTempStr[lLengthName] = lc;
                        lLengthName++;
                    }
                }
                break;

            default:
                // error
                lEndScan = 1;
                break;

        }
    }

    return lLengthName;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_open opens a ConfigFile_t for reading values.
//
// If the path doesn't exist, ConfigFile_open returns NULL.
// ConfigFile_open allocates the memory for the ConfigFile_t. To
// deallocate the memory, use ConfigFile_close
//
ConfigFilePtr_t ConfigFile_open( ConfigFilePtr_t pConfigFile, const char *szFileName, const char * szAttribute )
{
    FILE   *lTempFile;
    if (NULL == szFileName || '\0' == szFileName[0]) return pConfigFile;
    if (NULL == szAttribute) szAttribute = "rt+";

    // make sure that we have a valid, closed ConfigFile
    if (NULL == pConfigFile)
    {
        pConfigFile = ConfigFile_create();
    }
    else
    {
        ConfigFile_close( pConfigFile );  // close any open file
    }

    // open a file stream for access using the szAttribute attribute
    lTempFile = fopen( szFileName, szAttribute );
    if ( NULL == lTempFile  ) return pConfigFile;

    // assign the file info
    pConfigFile->f = lTempFile;
    strncpy(pConfigFile->filename, szFileName, sizeof(pConfigFile->filename));

    return pConfigFile;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_read
//
// Read in an open ConfigFile_t
//
ConfigFile_retval ConfigFile_read( ConfigFilePtr_t pConfigFile )
{
    ConfigFileSectionPtr_t lCurSection = NULL;
    ConfigFileValuePtr_t   lCurValue = NULL;
    Sint32  lError = 0;
    Sint32  lState = 0;
    char    lc;
    if ( NULL == pConfigFile ) return ConfigFile_fail;
    if ( NULL == pConfigFile->f || feof(pConfigFile->f) ) return ConfigFile_fail;

    // load all values in memory
    while ( 0 == lError && !feof( pConfigFile->f ) )
    {
        lc = fgetc( pConfigFile->f );

        switch ( lState )
        {
            case 0:

                // search for section ( _ctor )
                if ( lc == '{' )
                {
                    // create first section and load name
                    lCurSection = ConfigFile_createSection(pConfigFile, lCurSection);
                    lCurValue   = NULL;

                    // state change : look for new value or section
                    lState = 1;
                }
                else if ( lc == '/' )
                {
                    // pass over commentary ( bad!!! will be lost )
                    ConfigFile_ReadCommentary( pConfigFile, lCurValue );
                }
                break;

            case 1:

                // search for next value ( key name ) or next section
                if ( lc == '{' )
                {
                    // create new section and load name
                    lCurSection = ConfigFile_createSection(pConfigFile, lCurSection);
                    lCurValue   = NULL;
                }
                else if ( lc == '[' )
                {
                    lCurValue = ConfigFile_createValue(pConfigFile, lCurSection, lCurValue);

                    // state change : get value
                    lState = 2;
                }
                else if ( lc == '/' )
                {
                    // pass over commentary ( bad!!! will be lost )
                    ConfigFile_ReadCommentary( pConfigFile, lCurValue );
                }
                break;

            case 2:

                // search for value
                if ( lc == '"' )
                {
                    // load value in current value
                    ConfigFile_ReadValue( pConfigFile, lCurValue );
                    // state change : look for commentary
                    lState = 3;
                }
                break;

            case 3:

                // search for commentary
                if ( lc == '/' )
                {
                    //load commentary in current value
                    ConfigFile_ReadCommentary( pConfigFile, lCurValue );
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

    return (0 == lError) ? ConfigFile_succeed : ConfigFile_fail;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_SetCurrentSection
//
// Set the current section of pConfigFile to the one specified by pSection
// If the section doesn't exist, the current section is set to NULL and the
// function returns 0, otherwise the it returns 1.
//
long ConfigFile_SetCurrentSection( ConfigFilePtr_t pConfigFile, const char *pSection )
{
    long lFound = 0;
    if ( NULL == pConfigFile  || NULL == pSection  )
    {
        return ConfigFile_fail;
    }
    if ( NULL == pConfigFile->CurrentSection )
    {
        pConfigFile->CurrentSection = pConfigFile->ConfigSectionList;

        while ( 0 == lFound && ( NULL != pConfigFile->CurrentSection ) )
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

            while ( 0 == lFound && ( NULL != pConfigFile->CurrentSection) )
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

//--------------------------------------------------------------------------------------------
// SetConfigCurrentValueFromCurrentSection looks for a value in the current
// section having pKey as KeyName
//
// If a value is found, the function returns 1, otherwise the function returns 0
// and CurrentValue is set to NULL.
//
ConfigFile_retval SetConfigCurrentValueFromCurrentSection( ConfigFilePtr_t pConfigFile, const char *pKey )
{
    Sint32 lFound = 0;
    if ( NULL == pKey || NULL == pConfigFile  || NULL == pConfigFile->CurrentSection )
    {
        return ConfigFile_fail;
    }

    // search for a value->KeyName = pKey in current section
    pConfigFile->CurrentValue = pConfigFile->CurrentSection->FirstValue;

    while ( 0 == lFound && ( NULL != pConfigFile->CurrentValue ) )
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

//--------------------------------------------------------------------------------------------
// ConfigFile_FindKey set the current value of pConfigFile to the one specified by
// pSection and pKey
// Returns 0 if failed
//
ConfigFile_retval ConfigFile_FindKey( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey )
{
    Sint32 lFound = 0;

    // search for section
    lFound = ConfigFile_SetCurrentSection( pConfigFile, pSection );
    if ( 1 == lFound )
    {
        // search for key in current section
        lFound = SetConfigCurrentValueFromCurrentSection( pConfigFile, pKey );
    }

    return lFound;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_GetValue_String search for the value from section pSection with the key pKey.
//
// If the value is found, the value is copied in pValue and the function returns 1.
// If the length of pValue is less than the length of the string value, the string is truncated.
// If the value isn't found, the function returns 0.
//
ConfigFile_retval ConfigFile_GetValue_String( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, char *pValue,
        Sint32 pValueBufferLength )
{
    if ( NULL == pConfigFile  || NULL == pValue  || NULL == pSection  || NULL == pKey  || pValueBufferLength <= 0 )
    {
        return ConfigFile_fail;
    }
    if ( 0 == ConfigFile_FindKey( pConfigFile, pSection, pKey ) )
    {
        return ConfigFile_fail;
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

    return ConfigFile_succeed;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_GetValue_Boolean set to btrue or bfalse pBool. If the function can't find the value, it
// returns 0. If the value can't be identified as btrue or bfalse, the default is bfalse.
//
ConfigFile_retval ConfigFile_GetValue_Boolean( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, bool_t   *pBool )
{
    char lBoolStr[16];
    Sint32 lRet;

    memset( lBoolStr, 0, 16 );

    lRet = ConfigFile_GetValue_String( pConfigFile, pSection, pKey, lBoolStr, 16 );
    if ( lRet != 0 )
    {
        // check for btrue
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

//--------------------------------------------------------------------------------------------
// ConfigFile_GetValue_Int set pInt. If the function can't find the value, it
// returns 0.
//
ConfigFile_retval ConfigFile_GetValue_Int( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, Sint32 *pInt )
{
    char lIntStr[24];
    Sint32 lRet;

    memset( lIntStr, 0, 16 );

    lRet = ConfigFile_GetValue_String( pConfigFile, pSection, pKey, lIntStr, 24 );
    if ( lRet != 0 )
    {
        // convert value
        sscanf( lIntStr, "%d", pInt );
    }

    return lRet;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_SetValue_String set the value specified by pSection and pKey. If the value
// doesn't exist, it is created
//
ConfigFile_retval ConfigFile_SetValue_String( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, const char *pValue )
{
    ConfigFileSectionPtr_t lTempSection = NULL;
    ConfigFileValuePtr_t  lTempValue = NULL;
    Sint32 lOK = 0;
    size_t lLengthNewValue;
    char   lNewSectionName[MAX_CONFIG_SECTION_LENGTH];
    char   lNewKeyName[MAX_CONFIG_KEY_LENGTH];
    if ( NULL == pConfigFile  )
    {
        return ConfigFile_fail;
    }
    if ( NULL == pValue  || NULL == pSection  || NULL == pKey  )
    {
        return ConfigFile_fail;
    }

    // make sure section name and key name are made of valid char
    strcpy( lNewSectionName, pSection );
    strcpy( lNewKeyName, pKey );
    ConfigFileString_Encode( lNewSectionName );
    ConfigFileString_Encode( lNewKeyName );

    // look for section = pSection
    if (( lOK = ConfigFile_SetCurrentSection( pConfigFile, lNewSectionName ) )  == 0 )
    {
        // section doesn't exist so create it and create value
        lTempSection = ConfigFileSection_create();
        lTempSection->NextSection = pConfigFile->ConfigSectionList;
        pConfigFile->ConfigSectionList = lTempSection;
        strcpy( lTempSection->SectionName, lNewSectionName );

        // create the new value
        lTempValue = ConfigFileValue_create();
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
            lTempValue = ConfigFileValue_create();
            lTempValue->NextValue = pConfigFile->CurrentSection->FirstValue;
            pConfigFile->CurrentSection->FirstValue = lTempValue;
            strcpy( lTempValue->KeyName, lNewKeyName );

            // set current section and value
            pConfigFile->CurrentValue = lTempValue;
        }
    }

    lLengthNewValue = ( Sint32 ) strlen( pValue );
    if ( NULL == pConfigFile->CurrentValue->Value )
    {
        // if the stirng value doesn't exist than allocate memory for it
        pConfigFile->CurrentValue->Value = ConfigFileString_create( lLengthNewValue + 1 );
    }
    else
    {
        pConfigFile->CurrentValue->Value = ConfigFileString_resize(pConfigFile->CurrentValue->Value, lLengthNewValue + 1 );
    }

    strncpy( pConfigFile->CurrentValue->Value, pValue, lLengthNewValue );

    return ConfigFile_succeed;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_SetValue_Boolean saves a boolean in a value specified by pSection and pKey
//
ConfigFile_retval ConfigFile_SetValue_Boolean( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, bool_t pBool )
{
    if ( pBool )
    {
        // save the value with btrue
        return ConfigFile_SetValue_String( pConfigFile, pSection, pKey, "TRUE" );
    }

    // since it's bfalse
    return ConfigFile_SetValue_String( pConfigFile, pSection, pKey, "FALSE" );
}

//--------------------------------------------------------------------------------------------
// ConfigFile_SetValue_Int saves an integer in a value specified by pSection and pKey
//
ConfigFile_retval ConfigFile_SetValue_Int( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, int pInt )
{
    static char lIntStr[16];

    snprintf( lIntStr, sizeof( lIntStr ), "%i", pInt );
    return ConfigFile_SetValue_String( pConfigFile, pSection, pKey, lIntStr );
}

//--------------------------------------------------------------------------------------------
// ConfigFile_SetValue_Float saves a float in a value specified by pSection and pKey
//
ConfigFile_retval ConfigFile_SetValue_Float( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, float pFloat )
{
    static char lFloatStr[16];

    snprintf( lFloatStr, sizeof( lFloatStr ), "%f", pFloat );
    return ConfigFile_SetValue_String( pConfigFile, pSection, pKey, lFloatStr );
}

//--------------------------------------------------------------------------------------------
// ConfigFile_close close the ConfigFile_t. Does not deallocate its memory.
//
ConfigFile_retval ConfigFile_close( ConfigFilePtr_t pConfigFile )
{
    if ( NULL == pConfigFile ) return ConfigFile_fail;

    if ( NULL != pConfigFile->f )
    {
        fclose( pConfigFile->f );
        pConfigFile->f = NULL;
    }

    return ConfigFile_succeed;
}

//--------------------------------------------------------------------------------------------
// ConfigValue_write saves the value from pValue at the current position
// of the pConfigFile file. The '"' are doubled.
//
ConfigFile_retval ConfigValue_write( FILE *pFile, ConfigFileValuePtr_t pValue )
{
    Sint32 lPos = 0;
    if ( NULL == pFile )  return ConfigFile_fail;
    if ( NULL == pValue || NULL == pValue->Value || '\0' == pValue->Value[0] ) return ConfigFile_fail;

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

    return ConfigFile_succeed;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_write
//
ConfigFile_retval ConfigFile_write( ConfigFilePtr_t pConfigFile )
{
    ConfigFileSectionPtr_t lTempSection;
    ConfigFileValuePtr_t   lTempValue;
    if ( NULL == pConfigFile || NULL == pConfigFile->f  )
    {
        return ConfigFile_fail;
    }

    lTempSection = pConfigFile->ConfigSectionList;
    // rewrite the file
    rewind( pConfigFile->f );

    // saves all sections
    while ( NULL != lTempSection  )
    {
        fprintf( pConfigFile->f, "{%s}\n", lTempSection->SectionName );
        // saves all values form the current section
        lTempValue = lTempSection->FirstValue;

        while ( NULL != lTempValue  )
        {
            fprintf( pConfigFile->f, "[%s] : ", lTempValue->KeyName );
            if ( NULL != lTempValue->Value )
            {
                ConfigValue_write( pConfigFile->f, lTempValue );
            }
            if ( NULL != lTempValue->Commentary )
            {
                fprintf( pConfigFile->f, " // %s", lTempValue->Commentary );
            }

            fprintf( pConfigFile->f, "\n" );
            lTempValue = lTempValue->NextValue;
        }

        fprintf( pConfigFile->f, "\n" );
        lTempSection = lTempSection->NextSection;
    }

    return ConfigFile_succeed;
}

//--------------------------------------------------------------------------------------------
// ReadConfigFile creates a new ConfigFile_t fills it with the data in the file szFileName
//
ConfigFilePtr_t LoadConfigFile( const char *szFileName )
{
    ConfigFilePtr_t lConfigFile;

    // try to open a file
    lConfigFile = ConfigFile_open( NULL, szFileName, "rt+" );
    if (NULL == lConfigFile) return NULL;

    // try to read the data
    if ( ConfigFile_fail == ConfigFile_read( lConfigFile ) )
    {
        ConfigFile_destroy( &lConfigFile );
    }

    return lConfigFile;
}

//--------------------------------------------------------------------------------------------
// SaveConfigFile saves the given ConfigFile into the same file that it was reas from
//
ConfigFile_retval SaveConfigFile( ConfigFilePtr_t pConfigFile )
{
    ConfigFile_retval retval;

    if ( NULL == pConfigFile ) return ConfigFile_succeed;

    if ( !ConfigFile_open( pConfigFile, pConfigFile->filename, "wt" ) )
    {
        return ConfigFile_fail;
    }

    retval = ConfigFile_write( pConfigFile );

    ConfigFile_close( pConfigFile );

    return retval;
}

//--------------------------------------------------------------------------------------------
// SaveConfigFileAs saves pConfigFile at szFileName
// pConfigFile's file is close and set to the new file
//
ConfigFile_retval SaveConfigFileAs( ConfigFilePtr_t pConfigFile, const char *szFileName )
{
    ConfigFile_retval retval;
    char old_filename[256];
    if (NULL == pConfigFile) return ConfigFile_fail;
    if (NULL == szFileName || '\0' == szFileName) return ConfigFile_fail;

    // save the original filename
    strncpy( old_filename, pConfigFile->filename, sizeof(old_filename) );

    // try to open the target file
    if ( !ConfigFile_open( pConfigFile, szFileName, "wt" ) )
    {
        return ConfigFile_fail;
    }

    // try to save the info
    retval = ConfigFile_write( pConfigFile );

    // close the file
    ConfigFile_close( pConfigFile );

    // restore the old filename info
    strncpy( pConfigFile->filename, old_filename, sizeof(old_filename) );

    return retval;
}
