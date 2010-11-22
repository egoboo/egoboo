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
/// @file file_formats/configfile.c
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
/// begins with "// "
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

#include "log.h"
#include "string.h"

//--------------------------------------------------------------------------------------------
// MACROS
//
// duplicate these macro definitions here since configfile should have no dependence on egoboo...
//
//--------------------------------------------------------------------------------------------

#define CSTR_END '\0'
#define EMPTY_CSTR { CSTR_END }

#define VALID_CSTR(PSTR)   ((NULL!=PSTR) && (CSTR_END != PSTR[0]))
#define INVALID_CSTR(PSTR) ((NULL==PSTR) || (CSTR_END == PSTR[0]))

#define C_BELL_CHAR            '\a'
#define C_BACKSPACE_CHAR       '\b'
#define C_FORMFEED_CHAR        '\f'
#define C_NEW_LINE_CAHR        '\n'
#define C_CARRIAGE_RETURN_CHAR '\r'
#define C_TAB_CHAR             '\t'
#define C_VERTICAL TAB_CHAR    '\v'

#define DOUBLE_QUOTE_CHAR   '\"'
#define SINGLE_QUOTE_CHAR   '\''
#define ASCII_LINEFEED_CHAR '\x0A'

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// util
static void ConfigFileString_Encode( char *pStr );

static ConfigFilePtr_t   ConfigFile_open( ConfigFilePtr_t pConfigFile, const char *szFileName, const char * szAttribute, bool_t force );
static ConfigFile_retval ConfigFile_close( ConfigFilePtr_t pConfigFile );

static ConfigFile_retval ConfigFile_read( ConfigFilePtr_t pConfigFile );
static ConfigFile_retval ConfigFile_write( ConfigFilePtr_t pConfigFile );

static ConfigFileSectionPtr_t ConfigFileSection_create();
static ConfigFile_retval      ConfigFileSection_destroy( ConfigFileSectionPtr_t * ptmp );

static ConfigFileValuePtr_t   ConfigFileValue_create();
static ConfigFile_retval      ConfigFileValue_destroy( ConfigFileValuePtr_t * ptmp );

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
    if ( NULL == ptmp ) return ptmp;

    // set the file
    ptmp->f           = NULL;
    ptmp->filename[0] = CSTR_END;

    return ptmp;
}

//--------------------------------------------------------------------------------------------
ConfigFile_retval ConfigFile_destroy( ConfigFilePtr_t * ppConfigFile )
{
    ConfigFileSectionPtr_t lTempSection, lDoomedSection;
    if ( NULL == ppConfigFile || NULL == *ppConfigFile ) return ConfigFile_fail;

    ConfigFile_close( *ppConfigFile );
    ( *ppConfigFile )->filename[0] = CSTR_END;

    // delete all sections form the ConfigFile_t
    lTempSection = ( *ppConfigFile )->ConfigSectionList;

    while ( NULL != lTempSection )
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
    if ( NULL == s )
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
ConfigFileValuePtr_t ConfigFile_createValue( ConfigFilePtr_t f, ConfigFileSectionPtr_t s, ConfigFileValuePtr_t v )
{
    ConfigFileValuePtr_t pnew;

    // create new value
    pnew = ConfigFileValue_create();

    // link the new value into the correct section
    if ( NULL == v )
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
char * ConfigFileString_create( size_t len )
{
    /// @details BB@> must use malloc/calloc instead of new, since we have to accommodate realoc()

    char * ptmp;

    ptmp = ( char * )malloc( len * sizeof( char ) );
    if ( NULL != ptmp ) *ptmp = CSTR_END;

    return ptmp;
}

//--------------------------------------------------------------------------------------------
ConfigFile_retval ConfigFileString_destroy( char ** ptmp )
{
    /// @details BB@> must use free instead of delete, since we have to accommodate realoc()

    ConfigFile_retval retval;

    if ( NULL == ptmp ) return ConfigFile_fail;

    retval = ConfigFile_fail;
    if ( NULL != *ptmp )
    {
        free( *ptmp );
        *ptmp = NULL;
        retval = ConfigFile_succeed;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
char * ConfigFileString_resize( char * str, size_t new_len )
{
    size_t old_len = INVALID_CSTR( str ) ? 0 : strlen( str );

    // if value already exists, verify if allocated memory is enough
    // if not allocate more memory
    if ( new_len >= MAX_CONFIG_VALUE_LENGTH )
    {
        new_len = MAX_CONFIG_VALUE_LENGTH - 1;
    }
    if ( new_len > old_len )
    {
        str = ( char * )realloc( str, new_len );

        // only blank the new memory, not the whole string
        memset( str + old_len, 0, ( new_len - old_len ) * sizeof( char ) );
    }

    return str;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ConfigFileValuePtr_t ConfigFileValue_create()
{
    ConfigFileValuePtr_t ptmp;

    ptmp = CONFIG_NEW( ConfigFileValue_t );

    return ptmp;
}

//--------------------------------------------------------------------------------------------
ConfigFile_retval ConfigFileValue_destroy( ConfigFileValuePtr_t * ptmp )
{
    if ( NULL == ptmp || NULL == *ptmp ) return ConfigFile_fail;

    ConfigFileString_destroy( &( *ptmp )->Commentary );
    ConfigFileString_destroy( &( *ptmp )->Value );

    CONFIG_DELETE( *ptmp );

    return ConfigFile_succeed;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ConfigFileSectionPtr_t ConfigFileSection_create()
{
    ConfigFileSectionPtr_t ptmp;

    ptmp = CONFIG_NEW( ConfigFileSection_t );

    return ptmp;
}

//--------------------------------------------------------------------------------------------
ConfigFile_retval ConfigFileSection_destroy( ConfigFileSectionPtr_t * ptmp )
{
    ConfigFileValuePtr_t lTempValue, lDoomedValue;
    if ( NULL == ptmp || NULL == *ptmp ) return ConfigFile_fail;

    // delete all values from the current section
    lTempValue = ( *ptmp )->FirstValue;

    while ( NULL != lTempValue )
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
    if ( NULL == pStr ) return;

    str_encode( pStr, ( size_t )( -1 ), pStr );
}

//--------------------------------------------------------------------------------------------
// ConfigFile_PassOverCommentary reads the pConfigFile's file until the end of a line.
ConfigFile_retval ConfigFile_PassOverCommentary( ConfigFilePtr_t pConfigFile )
{
    char lc;

    lc = fgetc( pConfigFile->f );

    while ( C_CARRIAGE_RETURN_CHAR != lc && ASCII_LINEFEED_CHAR != lc && !feof( pConfigFile->f ) )
    {
        lc = fgetc( pConfigFile->f );
    }

    return ConfigFile_fail;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_ReadSectionName reads characters from the config file until it encounters
// a _dtor of section name "}". The resulting string is copied in pSection->SectionName.
// The length of the name is returned, or 0 if there is an error.

// if the string is longer than MAX_CONFIG_SECTION_LENGTH, the string is truncated

size_t ConfigFile_ReadSectionName( ConfigFilePtr_t pConfigFile, ConfigFileSectionPtr_t pSection )
{
    size_t lLengthName = 0;
    char lc;

    lc = fgetc( pConfigFile->f );
    memset( pSection->SectionName, 0, sizeof( pSection->SectionName ) );

    while ( '}' != lc && 0 == feof( pConfigFile->f ) )
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

// if the string is longuer than MAX_CONFIG_KEY_LENGTH, the string is truncated

size_t ConfigFile_ReadKeyName( ConfigFilePtr_t pConfigFile, ConfigFileValuePtr_t pValue )
{
    size_t lLengthName = 0;
    char lc;

    lc = fgetc( pConfigFile->f );
    memset( pValue->KeyName, 0, sizeof( pValue->KeyName ) );

    while ( ']' != lc && 0 == feof( pConfigFile->f ) )
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
// a _dtor of value DOUBLE_QUOTE_CHAR. The resulting string is copied in pValue->Value.
// The length of the value is returned, or -1 if there is an error.

// The memory for pValue->Value is allocated here.

ConfigFile_retval ConfigFile_ReadValue( ConfigFilePtr_t pConfigFile, ConfigFileValuePtr_t pValue )
{
    static char lTempStr[MAX_CONFIG_VALUE_LENGTH] = EMPTY_CSTR;
    char lc;
    Sint32  lEndScan = 0;
    Sint32  lState = 0;
    Sint32 lLengthName = 0;

    memset( lTempStr, 0, sizeof( lTempStr ) );

    while ( 0 == lEndScan )
    {
        lc = fgetc( pConfigFile->f );

        switch ( lState )
        {
            case 0:

                // search for _dtor of string DOUBLE_QUOTE_CHAR
                if ( DOUBLE_QUOTE_CHAR == lc )
                {
                    // state change :
                    lState = 1;
                }
                else if ( C_CARRIAGE_RETURN_CHAR == lc || ASCII_LINEFEED_CHAR == lc || feof( pConfigFile->f ) )
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
                if ( DOUBLE_QUOTE_CHAR == lc )
                {
                    // add DOUBLE_QUOTE_CHAR in string
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
                    pValue->Value = ConfigFileString_create( lLengthName + 1 );
                    // copy string
                    strncpy( pValue->Value, lTempStr, lLengthName + 1 );
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

// The memory for pValue->Commentary is allocated here.

ConfigFile_retval ConfigFile_ReadCommentary( ConfigFilePtr_t pConfigFile, ConfigFileValuePtr_t pValue )
{
    static char lTempStr[MAX_CONFIG_COMMENTARY_LENGTH] = EMPTY_CSTR;
    char lc;
    Sint32  lEndScan = 0;
    Sint32  lState = 0;
    Sint32 lLengthName = 0;
    if ( NULL == pValue ) return ConfigFile_PassOverCommentary( pConfigFile );

    memset( lTempStr, 0, sizeof( lTempStr ) );

    while ( 0 == lEndScan )
    {
        lc = fgetc( pConfigFile->f );

        switch ( lState )
        {
            case 0:

                // search for _dtor of string DOUBLE_QUOTE_CHAR
                if ( C_SLASH_CHR == lc ||  ' ' == lc )
                {
                    // continue scan until a letter appears
                }
                else if ( C_CARRIAGE_RETURN_CHAR == lc || ASCII_LINEFEED_CHAR == lc || feof( pConfigFile->f ) )
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
                if ( C_CARRIAGE_RETURN_CHAR == lc || ASCII_LINEFEED_CHAR == lc || feof( pConfigFile->f ) )
                {
                    // allocate memory for commentary
                    pValue->Commentary = CONFIG_NEW_ARY( char, lLengthName + 1 );
                    // copy string
                    strncpy( pValue->Commentary, lTempStr, lLengthName );
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

// If the path doesn't exist, ConfigFile_open returns NULL.
// ConfigFile_open allocates the memory for the ConfigFile_t. To
// deallocate the memory, use ConfigFile_close

ConfigFilePtr_t ConfigFile_open( ConfigFilePtr_t pConfigFile, const char *szFileName, const char * szAttribute, bool_t force )
{
    char    local_attribute[32] = EMPTY_CSTR;
    FILE   *lTempFile;

    if ( INVALID_CSTR( szFileName ) ) return pConfigFile;

    if ( INVALID_CSTR( szAttribute ) )
    {
        strncpy( local_attribute, "rt", SDL_arraysize( local_attribute ) - 2 );
    }
    else
    {
        strncpy( local_attribute, szAttribute, SDL_arraysize( local_attribute ) - 2 );
    }

    if ( force )
    {
        strcat( local_attribute, "+" );
    }

    // make sure that we have a valid, closed ConfigFile
    if ( NULL == pConfigFile )
    {
        pConfigFile = ConfigFile_create();
    }
    else
    {
        ConfigFile_close( pConfigFile );  // close any open file
    }

    // open a file stream for access using the szAttribute attribute
    lTempFile = fopen( szFileName, local_attribute );
    if ( NULL == lTempFile )
    {
        return pConfigFile;
    }
    // assign the file info
    pConfigFile->f = lTempFile;
    strncpy( pConfigFile->filename, szFileName, SDL_arraysize( pConfigFile->filename ) );

    return pConfigFile;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_read

// Read in an open ConfigFile_t

ConfigFile_retval ConfigFile_read( ConfigFilePtr_t pConfigFile )
{
    ConfigFileSectionPtr_t lCurSection = NULL;
    ConfigFileValuePtr_t   lCurValue = NULL;
    Sint32  lError = 0;
    Sint32  lState = 0;
    char    lc;

    if ( NULL == pConfigFile ) return ConfigFile_fail;
    if ( NULL == pConfigFile->f || feof( pConfigFile->f ) ) return ConfigFile_fail;

    // load all values in memory
    while ( 0 == lError && !feof( pConfigFile->f ) )
    {
        lc = fgetc( pConfigFile->f );

        switch ( lState )
        {
            case 0:

                // search for section ( _ctor )
                if ( '{' == lc )
                {
                    // create first section and load name
                    lCurSection = ConfigFile_createSection( pConfigFile, lCurSection );
                    lCurValue   = NULL;

                    // state change : look for new value or section
                    lState = 1;
                }
                else if ( C_SLASH_CHR == lc )
                {
                    // pass over commentary ( bad!!! will be lost )
                    ConfigFile_ReadCommentary( pConfigFile, lCurValue );
                }
                break;

            case 1:

                // search for next value ( key name ) or next section
                if ( '{' == lc )
                {
                    // create new section and load name
                    lCurSection = ConfigFile_createSection( pConfigFile, lCurSection );
                    lCurValue   = NULL;
                }
                else if ( '[' == lc )
                {
                    lCurValue = ConfigFile_createValue( pConfigFile, lCurSection, lCurValue );

                    // state change : get value
                    lState = 2;
                }
                else if ( C_SLASH_CHR == lc )
                {
                    // pass over commentary ( bad!!! will be lost )
                    ConfigFile_ReadCommentary( pConfigFile, lCurValue );
                }
                break;

            case 2:

                // search for value
                if ( DOUBLE_QUOTE_CHAR == lc )
                {
                    // load value in current value
                    ConfigFile_ReadValue( pConfigFile, lCurValue );
                    // state change : look for commentary
                    lState = 3;
                }
                break;

            case 3:

                // search for commentary
                if ( C_SLASH_CHR == lc )
                {
                    // load commentary in current value
                    ConfigFile_ReadCommentary( pConfigFile, lCurValue );
                    // state change : look for new value or section
                    lState = 1;
                }
                else if ( ASCII_LINEFEED_CHAR == lc || C_CARRIAGE_RETURN_CHAR == lc )
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

    return ( 0 == lError ) ? ConfigFile_succeed : ConfigFile_fail;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_SetCurrentSection

// Set the current section of pConfigFile to the one specified by pSection
// If the section doesn't exist, the current section is set to NULL and the
// function returns 0, otherwise the it returns 1.

long ConfigFile_SetCurrentSection( ConfigFilePtr_t pConfigFile, const char *pSection )
{
    long lFound = 0;
    if ( NULL == pConfigFile  || NULL == pSection )
    {
        return ConfigFile_fail;
    }
    if ( NULL == pConfigFile->CurrentSection )
    {
        pConfigFile->CurrentSection = pConfigFile->ConfigSectionList;

        while ( 0 == lFound && ( NULL != pConfigFile->CurrentSection ) )
        {
            if ( 0 == strcmp( pConfigFile->CurrentSection->SectionName, pSection ) )
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
        if ( 0 == strcmp( pConfigFile->CurrentSection->SectionName, pSection ) )
        {
            lFound = 1;
        }
        else
        {
            pConfigFile->CurrentSection = pConfigFile->ConfigSectionList;

            while ( 0 == lFound && ( NULL != pConfigFile->CurrentSection ) )
            {
                if ( 0 == strcmp( pConfigFile->CurrentSection->SectionName, pSection ) )
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

// If a value is found, the function returns 1, otherwise the function returns 0
// and CurrentValue is set to NULL.

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
        if ( 0 == strcmp( pConfigFile->CurrentValue->KeyName, pKey ) )
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

// If the value is found, the value is copied in pValue and the function returns 1.
// If the length of pValue is less than the length of the string value, the string is truncated.
// If the value isn't found, the function returns 0.

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
        strncpy( pValue, pConfigFile->CurrentValue->Value, pValueBufferLength );
    }

    return ConfigFile_succeed;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_GetValue_Boolean set to btrue or bfalse pBool. If the function can't find the value, it
// returns 0. If the value can't be identified as btrue or bfalse, the default is bfalse.

ConfigFile_retval ConfigFile_GetValue_Boolean( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, bool_t   *pBool )
{
    char lBoolStr[16] = EMPTY_CSTR;
    Sint32 lRet;

    memset( lBoolStr, 0, sizeof( lBoolStr ) );

    lRet = ConfigFile_GetValue_String( pConfigFile, pSection, pKey, lBoolStr, 16 );
    if ( lRet != 0 )
    {
        // check for btrue
        if ( 0 == strcmp( lBoolStr, "TRUE" ) )
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

ConfigFile_retval ConfigFile_GetValue_Int( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, Sint32 *pInt )
{
    char lIntStr[24] = EMPTY_CSTR;
    Sint32 lRet;

    memset( lIntStr, 0, sizeof( lIntStr ) );

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

ConfigFile_retval ConfigFile_SetValue_String( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, const char *pValue )
{
    ConfigFileSectionPtr_t lTempSection = NULL;
    ConfigFileValuePtr_t  lTempValue = NULL;
    Sint32 lOK = 0;
    size_t lLengthNewValue;
    char   lNewSectionName[MAX_CONFIG_SECTION_LENGTH] = EMPTY_CSTR;
    char   lNewKeyName[MAX_CONFIG_KEY_LENGTH] = EMPTY_CSTR;
    if ( NULL == pConfigFile )
    {
        return ConfigFile_fail;
    }
    if ( NULL == pValue  || NULL == pSection  || NULL == pKey )
    {
        return ConfigFile_fail;
    }

    // make sure section name and key name are made of valid char
    strncpy( lNewSectionName, pSection, sizeof( lNewSectionName ) );
    strncpy( lNewKeyName, pKey, sizeof( lNewKeyName ) );
    ConfigFileString_Encode( lNewSectionName );
    ConfigFileString_Encode( lNewKeyName );

    // look for section = pSection
    lOK = ConfigFile_SetCurrentSection( pConfigFile, lNewSectionName );
    if ( 0 == lOK )
    {
        // section doesn't exist so create it and create value
        lTempSection = ConfigFileSection_create();
        lTempSection->NextSection = pConfigFile->ConfigSectionList;
        pConfigFile->ConfigSectionList = lTempSection;
        strncpy( lTempSection->SectionName, lNewSectionName, sizeof( lTempSection->SectionName ) );

        // create the new value
        lTempValue = ConfigFileValue_create();
        lTempSection->FirstValue = lTempValue;
        strncpy( lTempValue->KeyName, lNewKeyName, sizeof( lTempValue->KeyName ) );

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
            strncpy( lTempValue->KeyName, lNewKeyName, sizeof( lTempValue->KeyName ) );

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
        pConfigFile->CurrentValue->Value = ConfigFileString_resize( pConfigFile->CurrentValue->Value, lLengthNewValue + 1 );
    }

    strncpy( pConfigFile->CurrentValue->Value, pValue, lLengthNewValue + 1 );

    return ConfigFile_succeed;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_SetValue_Boolean saves a boolean in a value specified by pSection and pKey

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

ConfigFile_retval ConfigFile_SetValue_Int( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, int pInt )
{
    static char lIntStr[16] = EMPTY_CSTR;

    snprintf( lIntStr, SDL_arraysize( lIntStr ), "%i", pInt );
    return ConfigFile_SetValue_String( pConfigFile, pSection, pKey, lIntStr );
}

//--------------------------------------------------------------------------------------------
// ConfigFile_SetValue_Float saves a float in a value specified by pSection and pKey

ConfigFile_retval ConfigFile_SetValue_Float( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, float pFloat )
{
    static char lFloatStr[16] = EMPTY_CSTR;

    snprintf( lFloatStr, SDL_arraysize( lFloatStr ), "%f", pFloat );
    return ConfigFile_SetValue_String( pConfigFile, pSection, pKey, lFloatStr );
}

//--------------------------------------------------------------------------------------------
// ConfigFile_close close the ConfigFile_t. Does not deallocate its memory.

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
// of the pConfigFile file. The DOUBLE_QUOTE_CHAR are doubled.

ConfigFile_retval ConfigValue_write( FILE *pFile, ConfigFileValuePtr_t pValue )
{
    Sint32 lPos = 0;
    if ( NULL == pFile )  return ConfigFile_fail;
    if ( NULL == pValue || INVALID_CSTR( pValue->Value ) ) return ConfigFile_fail;

    fputc( DOUBLE_QUOTE_CHAR, pFile );

    while ( pValue->Value[lPos] != 0 )
    {
        fputc( pValue->Value[lPos], pFile );
        if ( DOUBLE_QUOTE_CHAR == pValue->Value[lPos] )
        {
            // double the DOUBLE_QUOTE_CHAR
            fputc( DOUBLE_QUOTE_CHAR, pFile );
        }

        lPos++;
    }

    fputc( DOUBLE_QUOTE_CHAR, pFile );

    return ConfigFile_succeed;
}

//--------------------------------------------------------------------------------------------
// ConfigFile_write

ConfigFile_retval ConfigFile_write( ConfigFilePtr_t pConfigFile )
{
    ConfigFileSectionPtr_t lTempSection;
    ConfigFileValuePtr_t   lTempValue;
    if ( NULL == pConfigFile || NULL == pConfigFile->f )
    {
        return ConfigFile_fail;
    }

    lTempSection = pConfigFile->ConfigSectionList;
    // rewrite the file
    rewind( pConfigFile->f );

    // saves all sections
    while ( NULL != lTempSection )
    {
        fprintf( pConfigFile->f, "{%s}\n", lTempSection->SectionName );
        // saves all values form the current section
        lTempValue = lTempSection->FirstValue;

        while ( NULL != lTempValue )
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

ConfigFilePtr_t LoadConfigFile( const char *szFileName, bool_t force )
{
    ConfigFilePtr_t lConfigFile;

    // try to open a file
    lConfigFile = ConfigFile_open( NULL, szFileName, "", force );
    if ( NULL == lConfigFile ) return NULL;

    // try to read the data
    if ( ConfigFile_fail == ConfigFile_read( lConfigFile ) )
    {
        ConfigFile_destroy( &lConfigFile );
    }

    return lConfigFile;
}

//--------------------------------------------------------------------------------------------
// SaveConfigFile saves the given ConfigFile into the same file that it was reas from

ConfigFile_retval SaveConfigFile( ConfigFilePtr_t pConfigFile )
{
    ConfigFile_retval retval;

    if ( NULL == pConfigFile ) return ConfigFile_succeed;

    if ( !ConfigFile_open( pConfigFile, pConfigFile->filename, "wt", btrue ) )
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

ConfigFile_retval SaveConfigFileAs( ConfigFilePtr_t pConfigFile, const char *szFileName )
{
    ConfigFile_retval retval;
    char old_filename[256] = EMPTY_CSTR;

    if ( NULL == pConfigFile ) return ConfigFile_fail;
    if ( INVALID_CSTR( szFileName ) ) return ConfigFile_fail;

    // save the original filename
    strncpy( old_filename, pConfigFile->filename, SDL_arraysize( old_filename ) );

    // try to open the target file
    if ( !ConfigFile_open( pConfigFile, szFileName, "wt", btrue ) )
    {
        return ConfigFile_fail;
    }

    // try to save the info
    retval = ConfigFile_write( pConfigFile );

    // close the file
    ConfigFile_close( pConfigFile );

    // restore the old filename info
    strncpy( pConfigFile->filename, old_filename, SDL_arraysize( old_filename ) );

    return retval;
}
