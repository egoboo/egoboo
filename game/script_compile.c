// ********************************************************************************************
// *
// *    This file is part of Egoboo.
// *
// *    Egoboo is free software: you can redistribute it and/or modify it
// *    under the terms of the GNU General Public License as published by
// *    the Free Software Foundation, either version 3 of the License, or
// *    (at your option) any later version.
// *
// *    Egoboo is distributed in the hope that it will be useful, but
// *    WITHOUT ANY WARRANTY; without even the implied warranty of
// *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// *    General Public License for more details.
// *
// *    You should have received a copy of the GNU General Public License
// *    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
// *
// ********************************************************************************************

#include "script_compile.h"

#include "log.h"

#include "egoboo_setup.h"
#include "egoboo.h"

#include "SDL_extensions.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static int    iLoadSize;
static int    iNumLine;

static int    iLineSize;
static char   cLineBuffer[MAXLINESIZE];

static Uint8  cLoadBuffer[AISMAXLOADSIZE];

int    iNumCode = 0;
Uint8  cCodeType[MAXCODE];
Uint32 iCodeValue[MAXCODE];
char   cCodeName[MAXCODE][MAXCODENAMESIZE];

static int    Token_iLine;
static int    Token_iIndex;
static int    Token_iValue;
static char   Token_cType;
static char   Token_cWord[MAXCODENAMESIZE];

int    iNumAis = 0;
int    iAisIndex = 0;
STRING szAisName[MAX_AI];
Uint32 iAisStartPosition[MAX_AI];
Uint32 iAisEndPosition[MAX_AI];
Uint32 iCompiledAis[AISMAXCOMPILESIZE];

bool_t debug_scripts = bfalse;
FILE * debug_script_file = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void   insert_space( int position );
// static void   copy_one_line( int write );
static int    load_one_line( int read );
// static int    load_parsed_line( int read );
static void   surround_space( int position );
// static void   parse_null_terminate_comments();
static int    get_indentation();
static void   fix_operators();
// static int    starts_with_capital_letter();
// static Uint32 get_high_bits();
static int    parse_token( int read );
static void   emit_opcode( Uint32 highbits );
static void   parse_line_by_line();
static Uint32 jump_goto( int index, int index_end );
static void   parse_jumps( int ainumber );
// static void   log_code( int ainumber, const char* savename );
static int    ai_goto_colon( int read );
static void   get_code( int read );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void insert_space( int position )
{
    // ZZ> This function adds a space into the load line if there isn't one
    //     there already
    char cTmp, cSwap;
    if ( !isspace(cLineBuffer[position]) )
    {
        cTmp = cLineBuffer[position];
        cLineBuffer[position] = ' ';
        position++;
        iLineSize++;

        while ( position < iLineSize )
        {
            cSwap = cLineBuffer[position];
            cLineBuffer[position] = cTmp;
            cTmp = cSwap;
            position++;
        }

        cLineBuffer[position] = '\0'; // or cTmp as cTmp == 0
    }
}

//--------------------------------------------------------------------------------------------
int load_one_line( int read )
{
    // ZZ> This function loads a line into the line buffer
    int stillgoing, foundtext;
    char cTmp;
    bool_t tabs_warning_needed;

    // Parse to start to maintain indentation
    cLineBuffer[0] = '\0';
    iLineSize = 0;
    stillgoing = btrue;

    // try to trap all end of line conditions so we can properly count the lines
    tabs_warning_needed = bfalse;
    while ( read < iLoadSize )
    {
        cTmp = cLoadBuffer[read];

        if ( cTmp == 0x0a && cLoadBuffer[read+1] == 0x0d )
        {
            iLineSize = 0;
            cLineBuffer[0] = '\0';
            return read + 2;
        }

        if ( cTmp == 0x0d && cLoadBuffer[read+1] == 0x0a )
        {
            iLineSize = 0;
            cLineBuffer[0] = '\0';
            return read + 2;
        }

        if ( cTmp == 0x0a || cTmp == 0x0d )
        {
            iLineSize = 0;
            cLineBuffer[0] = '\0';
            return read + 1;
        }

        if ( '\t' == cTmp )
        {
            tabs_warning_needed = btrue;
            cTmp = ' ';
        }

        if ( !isspace(cTmp) )
        {
            break;
        }

        cLineBuffer[iLineSize] = ' ';
        cLineBuffer[iLineSize+1] = '\0';

        read++;
        iLineSize++;
    }

    // Parse to comment or end of line
    foundtext = bfalse;
    while ( read < iLoadSize )
    {
        cTmp = cLoadBuffer[read];
        if ( cTmp == 0x0d || cTmp == 0x0a )
        {
            break;
        }

        if ( '/' == cTmp && '/' == cLoadBuffer[read] )
        {
            break;
        }

        read++;

        if ( iscntrl(cTmp) )
        {
            cTmp = ' ';
        }

        if ( !isspace(cTmp) )
        {
            foundtext = btrue;

            cLineBuffer[iLineSize]   = cTmp;
            cLineBuffer[iLineSize+1] = '\0';
            iLineSize++;
        }
    }
    if ( !foundtext )
    {
        iLineSize = 0;
    }

    // terminate the line buffer properly
    cLineBuffer[iLineSize] = '\0';

    if ( iLineSize > 0  && tabs_warning_needed )
    {
        log_warning( "Tab character used to define spacing will cause an error \"%s\"(%d) - \n\t\"%s\"\n", globalparsename, Token_iLine, cLineBuffer );
    }

    // Parse to end of line
    while ( read < iLoadSize )
    {
        if ( 0x0a == cLoadBuffer[read] && 0x0d == cLoadBuffer[read+1] )
        {
            read += 2;
            break;
        }
        else if ( 0x0d == cLoadBuffer[read] && 0x0a == cLoadBuffer[read+1] )
        {
            read += 2;
            break;
        }
        else if ( '\0' == cLoadBuffer[read] || 0x0a == cLoadBuffer[read] || 0x0d == cLoadBuffer[read] )
        {
            read += 1;
            break;
        }

        read++;
    }

    return read;
}

//--------------------------------------------------------------------------------------------
void surround_space( int position )
{
    insert_space( position + 1 );
    if ( position > 0 )
    {
        if ( !isspace( cLineBuffer[position-1] ) )
        {
            insert_space( position );
        }
    }
}

//--------------------------------------------------------------------------------------------
int get_indentation()
{
    // ZZ> This function returns the number of starting spaces in a line
    int cnt;
    char cTmp;

    cnt = 0;
    cTmp = cLineBuffer[cnt];
    while ( isspace(cTmp) )
    {
        cnt++;
        cTmp = cLineBuffer[cnt];
    }
    if ( HAS_SOME_BITS(cnt, 1) )
    {
        log_warning( "Invalid indentation \"%s\"(%d) - \"%s\"\n", globalparsename, Token_iLine, cLineBuffer );
        parseerror = btrue;
    }

    cnt >>= 1;
    if ( cnt > 15 )
    {
        log_warning( "Too many levels of indentation \"%s\"(%d) - \"%s\"\n", globalparsename, Token_iLine, cLineBuffer );
        parseerror = btrue;
        cnt = 15;
    }

    return cnt;
}

//--------------------------------------------------------------------------------------------
void fix_operators()
{
    // ZZ> This function puts spaces around operators to seperate words
    //     better
    int cnt;
    char cTmp;

    cnt = 0;

    while ( cnt < iLineSize )
    {
        cTmp = cLineBuffer[cnt];
        if ( '+' == cTmp || '-' == cTmp || '/' == cTmp || '*' == cTmp ||
                '%' == cTmp || '>' == cTmp || '<' == cTmp || '&' == cTmp ||
                '=' == cTmp )
        {
            surround_space( cnt );
            cnt++;
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
int parse_token( int read )
{
    // ZZ> This function tells what code is being indexed by read, it
    //     will return the next spot to read from and stick the code number
    //     in Token_iIndex
    int cnt, wordsize;
    char cTmp;
    IDSZ idsz;

    // Reset the token
    Token_iIndex   = MAXCODE;
    Token_iValue   = 0;
    Token_cType    = '?';
    Token_cWord[0] = '\0';

    // Check bounds
    if ( read >= iLineSize )
        return iLineSize;

    // Skip spaces
    cTmp = cLineBuffer[read];
    while ( isspace(cTmp) && read < iLineSize )
    {
        read++;
        cTmp = cLineBuffer[read];
    }
    if ( read >= iLineSize )  { /* print_token(); */ return read; }

    // Load the word into the other buffer
    wordsize = 0;
    while ( !isspace(cTmp) && '\0' != cTmp )
    {
        Token_cWord[wordsize] = cTmp;  wordsize++;
        read++;
        cTmp = cLineBuffer[read];
    }
    Token_cWord[wordsize] = '\0';

    // Check for numeric constant
    if ( Token_cWord[0] >= '0' && Token_cWord[0] <= '9' )
    {
        sscanf( Token_cWord, "%d", &Token_iValue );
        Token_cType  = 'C';
        Token_iIndex = MAXCODE;
        { /* print_token(); */  return read; }
    }

    // Check for IDSZ constant
    if ( '[' == Token_cWord[0] )
    {
        idsz = 0;

        cTmp = (Token_cWord[1] - 'A') & 0x1F;  idsz |= cTmp << 15;
        cTmp = (Token_cWord[2] - 'A') & 0x1F;  idsz |= cTmp << 10;
        cTmp = (Token_cWord[3] - 'A') & 0x1F;  idsz |= cTmp << 5;
        cTmp = (Token_cWord[4] - 'A') & 0x1F;  idsz |= cTmp;

        Token_iValue = idsz;
        Token_cType  = 'C';
        Token_iIndex = MAXCODE;

        { /* print_token(); */  return read; }
    }

    // compare in a case-insensitive manner. there is a unix-based function that does this,
    // but it is not sommon enough on non-linux compilers to be cross platform compatible
    // for ( cnt = 0; cnt < iNumCode; cnt++ )
    // {
    //    int i, maxlen;
    //    char * ptok, *pcode;
    //    bool_t found;

    //    ptok   = Token_cWord;
    //    pcode  = cCodeName[cnt];
    //    maxlen = MAXCODENAMESIZE;

    //    found = btrue;
    //    for( i = 0; i < maxlen && '\0' != *ptok && '\0' != pcode; i++ )
    //    {
    //        if( toupper( *ptok ) != toupper( *pcode ) )
    //        {
    //            found = bfalse;
    //            break;
    //        }

    //        ptok++;
    //        pcode++;
    //    }

    //    if ( '\0' == *ptok && '\0' == *pcode && found ) break;
    // }

    for ( cnt = 0; cnt < iNumCode; cnt++ )
    {
        if ( 0 == strncmp( Token_cWord, cCodeName[cnt], MAXCODENAMESIZE ) )
        {
            break;
        }
    }
    if ( cnt < iNumCode )
    {
        Token_iValue = iCodeValue[cnt];
        Token_cType  = cCodeType[cnt];
        Token_iIndex = cnt;
    }
    else if ( 0 == strcmp( Token_cWord, "=" ) )
    {
        Token_iValue = -1;
        Token_cType  = 'O';
        Token_iIndex = MAXCODE;
    }
    else
    {
        // Throw out an error code if we're loggin' 'em
        log_message( "SCRIPT ERROR: \"%s\"(%d) - unknown opcode \"%s\"\n", globalparsename, Token_iLine, Token_cWord );

        Token_iValue = -1;
        Token_cType  = '?';
        Token_iIndex = MAXCODE;

        parseerror = btrue;
    }

    { /* print_token(); */  return read; }
}

//--------------------------------------------------------------------------------------------
void emit_opcode( Uint32 highbits )
{
    // detect a constant value
    if ( 'C' == Token_cType || 'F' == Token_cType )
    {
        highbits |= FUNCTION_BIT;
    }
    if ( iAisIndex < AISMAXCOMPILESIZE )
    {
        iCompiledAis[iAisIndex] = highbits | Token_iValue;
        iAisIndex++;
    }
    else
    {
        log_warning( "Script index larger than array\n" );
    }
}

//--------------------------------------------------------------------------------------------
void parse_line_by_line()
{
    // ZZ> This function removes comments and endline codes, replacing
    //     them with a 0
    int read;
    Uint32 highbits;
    int parseposition;

    read = 0;
    for ( Token_iLine = 0; read < iLoadSize; Token_iLine++ )
    {
        read = load_one_line( read );
        if ( 0 == iLineSize ) continue;

        // print_line();

        fix_operators();
        parseposition = 0;

        //------------------------------
        // grab the first opcode

        highbits = SET_DATA_BITS( get_indentation() );
        parseposition = parse_token( parseposition );
        if ( 'F' == Token_cType )
        {
            if ( FEND == Token_iValue && 0 == highbits )
            {
                // stop processing the lines, since we're finished
                break;
            }

            //------------------------------
            // the code type is a function

            // save the opcode
            emit_opcode( highbits );

            // leave a space for the control code
            Token_iValue = 0;
            emit_opcode( 0 );

        }
        else if ( 'V' == Token_cType )
        {
            //------------------------------
            // the code type is a math operation

            int operand_index;
            int operands = 0;

            // save in the value's opcode
            emit_opcode( highbits );

            // save a position for the operand count
            Token_iValue = 0;
            operand_index = iAisIndex;
            emit_opcode( 0 );

            // handle the "="
            highbits = 0;
            parseposition = parse_token( parseposition );  // EQUALS
            if ( 'O' != Token_cType || 0 != strcmp(Token_cWord, "=") )
            {
                log_warning( "Invalid equation \"%s\"(%d) - \"%s\"\n", globalparsename, Token_iLine, cLineBuffer);
            }

            //------------------------------
            // grab the next opcode

            parseposition = parse_token( parseposition );
            if ( 'V' == Token_cType || 'C' == Token_cType )
            {
                // this is a value or a constant
                emit_opcode( 0 );
                operands++;

                parseposition = parse_token( parseposition );
            }
            else if ( 'O' != Token_cType )
            {
                // this is a function or an unknown value. do not break the script.
                log_warning( "Invalid operand \"%s\"(%d) - \"%s\"\n", globalparsename, Token_iLine, Token_cWord);

                emit_opcode( 0 );
                operands++;

                parseposition = parse_token( parseposition );
            }

            // expects a OPERATOR VALUE OPERATOR VALUE OPERATOR VALUE pattern
            while ( parseposition < iLineSize )
            {
                // the current token should be an operator
                if ( 'O' != Token_cType )
                {
                    // problem with the loop
                    log_warning( "Expected an operator \"%s\"(%d) - \"%s\"\n", globalparsename, Token_iLine, cLineBuffer);
                    break;
                }

                // the highbits are the operator's value
                highbits = SET_DATA_BITS( Token_iValue );

                // VALUE
                parseposition = parse_token( parseposition );
                if ( 'C' != Token_cType && 'V' != Token_cType )
                {
                    // not having a constant or a value here breaks the function. stop processing
                    log_warning( "Invalid operand \"%s\"(%d) - \"%s\"\n", globalparsename, Token_iLine, Token_cWord);
                    break;
                }

                emit_opcode( highbits );
                operands++;

                // OPERATOR
                parseposition = parse_token( parseposition );
            }

            iCompiledAis[operand_index] = operands;  // Number of operands
        }
        else if ( 'C' == Token_cType )
        {
            log_warning( "Invalid constant \"%s\"(%d) - \"%s\"\n", globalparsename, Token_iLine, Token_cWord );
        }
        else if ( '?' == Token_cType )
        {
            // unknown opcode, do not process this line
            log_warning( "Invalid operand \"%s\"(%d) - \"%s\"\n", globalparsename, Token_iLine, Token_cWord );
        }
        else
        {
            log_warning( "Compiler is broken \"%s\"(%d) - \"%s\"\n", globalparsename, Token_iLine, Token_cWord );
            break;
        }
    }

    Token_iValue = FEND;
    Token_cType  = 'F';
    emit_opcode( 0 );
    Token_iValue = iAisIndex + 1;
    emit_opcode( 0 );
}

//--------------------------------------------------------------------------------------------
Uint32 jump_goto( int index, int index_end )
{
    // ZZ> This function figures out where to jump to on a fail based on the
    //     starting location and the following code.  The starting location
    //     should always be a function code with indentation
    Uint32 value;
    int targetindent, indent;

    value = iCompiledAis[index];  index += 2;
    targetindent = GET_DATA_BITS( value );
    indent = 100;

    while ( indent > targetindent && index < index_end )
    {
        value = iCompiledAis[index];
        indent = GET_DATA_BITS( value );
        if ( indent > targetindent )
        {
            // Was it a function
            if ( ( value & FUNCTION_BIT ) != 0 )
            {
                // Each function needs a jump
                index++;
                index++;
            }
            else
            {
                // Operations cover each operand
                index++;
                value = iCompiledAis[index];
                index++;
                index += ( value & 255 );
            }
        }
    }

    return MIN ( index, index_end );
}

//--------------------------------------------------------------------------------------------
void parse_jumps( int ainumber )
{
    // ZZ> This function sets up the fail jumps for the down and dirty code
    int index, index_end;
    Uint32 value, iTmp;

    index     = iAisStartPosition[ainumber];
    index_end = iAisEndPosition[ainumber];

    value = iCompiledAis[index];
    while ( index < index_end )
    {
        value = iCompiledAis[index];

        // Was it a function
        if ( HAS_SOME_BITS( value, FUNCTION_BIT ) )
        {
            // Each function needs a jump
            iTmp = jump_goto( index, index_end );
            index++;
            iCompiledAis[index] = iTmp;
            index++;
        }
        else
        {
            // Operations cover each operand
            index++;
            iTmp = iCompiledAis[index];
            index++;
            index += ( iTmp & 0xFF );
        }
    }
}
//--------------------------------------------------------------------------------------------
int ai_goto_colon( int read )
{
    // ZZ> This function goes to spot after the next colon
    char cTmp;

    cTmp = cLoadBuffer[read];

    while ( ':' != cTmp && read < iLoadSize )
    {
        read++;  cTmp = cLoadBuffer[read];
    }
    if ( read < iLoadSize )  read++;

    return read;
}

//--------------------------------------------------------------------------------------------
void get_code( int read )
{
    // ZZ> This function gets code names and other goodies
    char cTmp;
    int iTmp;

    sscanf( ( char* ) &cLoadBuffer[read], "%c%d%s", &cTmp, &iTmp, &cCodeName[iNumCode][0] );
    cCodeType[iNumCode] = toupper(cTmp);
    iCodeValue[iNumCode] = iTmp;
}

//--------------------------------------------------------------------------------------------
void load_ai_codes( const char* loadname )
{
    // ZZ> This function loads all of the function and variable names
    FILE* fileread;
    int read;

    iNumCode = 0;
    fileread = fopen( loadname, "rb" );
    if ( fileread )
    {
        iLoadSize = ( int )fread( cLoadBuffer, 1, AISMAXLOADSIZE, fileread );
        read = 0;
        read = ai_goto_colon( read );

        while ( read != iLoadSize )
        {
            get_code( read );
            iNumCode++;
            read = ai_goto_colon( read );
        }

        fclose( fileread );
    }

    debug_script_file = fopen( "script_debug.txt", "w" );
}

//--------------------------------------------------------------------------------------------
int load_ai_script( const char *loadname )
{
    // ZZ> This function loads a script to memory and
    //     returns bfalse if it fails to do so
    FILE* fileread;
    int retval = 0;

    iNumLine = 0;
    fileread = fopen( loadname, "rb" );

    // No such file
    if ( NULL == fileread )
    {
        if ( cfg.dev_mode )
        {
            log_message( "DEBUG: I am missing a AI script (%s)\n", loadname );
            log_message( "       Using the default AI script instead (basicdat" SLASH_STR "script.txt)\n" );
        }

        return retval;
    }
    if ( iNumAis >= MAX_AI )
    {
        log_warning( "Too many script files. Cannot load file \"%s\"\n", loadname );
        return retval;
    }

    // load the file
    iLoadSize = ( int )fread( cLoadBuffer, 1, AISMAXLOADSIZE, fileread );
    fclose( fileread );

    // if the file is empty, use the default script
    if (0 == iLoadSize)
    {
        log_warning( "Script file is empty. \"%s\"\n", loadname );
        return retval;
    }

    // save the filename for error logging
    strncpy( szAisName[iNumAis], loadname, sizeof(STRING) );
    globalparsename = loadname;

    // initialize the start and end position
    iAisStartPosition[iNumAis] = iAisIndex;
    iAisEndPosition[iNumAis]   = iAisIndex;

    // parse/compile the scripts
    // parse_null_terminate_comments();
    parse_line_by_line();

    // set the end position of the script
    iAisEndPosition[iNumAis] = iAisIndex;

    // determine the correct jumps
    parse_jumps( iNumAis );

    // get the ai script index
    retval = iNumAis;

    // increase the ai script index
    iNumAis++;

    return retval;
}

//--------------------------------------------------------------------------------------------
void release_all_ai_scripts()
{
    // ZZ> This function resets the ai script "pointers"

    iAisIndex = 0;
    iNumAis = 0;
}

void init_all_ai_scripts()
{
    // ZZ> This function initializes the ai script "pointers"

    iAisIndex = 0;
    iNumAis = 0;
}

//--------------------------------------------------------------------------------------------
// int load_parsed_line( int read )
// {
//    // ZZ> This function loads a line into the line buffer
//    char cTmp;
//
//    // Parse to start to maintain indentation
//    iLineSize = 0;
//    cTmp = cLoadBuffer[read];
//
//    while ( '\0' != cTmp )
//    {
//        cLineBuffer[iLineSize] = cTmp;  iLineSize++;
//        read++;  cTmp = cLoadBuffer[read];
//    }
//
//    cLineBuffer[iLineSize] = '\0';
//    read++; // skip terminating zero for next call of load_parsed_line()
//    return read;
// }
//
//--------------------------------------------------------------------------------------------
// void parse_null_terminate_comments()
// {
//    // ZZ> This function removes comments and endline codes, replacing
//    //     them with a 0
//    int read, write;
//
//    read = 0;
//    write = 0;
//
//    while ( read < iLoadSize )
//    {
//        read = load_one_line( read );
//
//        if ( iLineSize > 2 )
//        {
//            copy_one_line( write );
//            write += iLineSize;
//        }
//    }
// }
//
//--------------------------------------------------------------------------------------------
// void print_token()
// {
//    printf("------------\n", globalparsename, Token_iLine);
//    printf("\tToken_iIndex == %d\n", Token_iIndex);
//    printf("\tToken_iValue == %d\n", Token_iValue);
//    printf("\tToken_cType  == \'%c\'\n", Token_cType);
//    printf("\tToken_cWord  == \"%s\"\n", Token_cWord);
// }
//
//--------------------------------------------------------------------------------------------
// void print_line()
// {
//    int i;
//    char cTmp;
//
//    printf("\n===========\n\tfile == \"%s\"\n\tline == %d\n", globalparsename, Token_iLine);
//
//    printf( "\tline == \"" );
//
//    for(i=0; i<iLineSize; i++)
//    {
//        cTmp = cLineBuffer[i];
//        if( isprint(cTmp) )
//        {
//            printf( "%c", cTmp );
//        }
//        else
//        {
//            printf( "\\%03d", cTmp );
//        }
//    };
//
//    printf( "\", length == %d\n", iLineSize);
// }
