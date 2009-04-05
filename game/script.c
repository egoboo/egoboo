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

/* Egoboo - script.c
 * Implements the game's scripting language.
 */

#include "egoboo.h"
#include "script.h"
#include "log.h"
#include "link.h"

#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

int     valuetmpx         =  0;
int     valuetmpy         =  0;
Uint16  valuetmpturn      =  0;
int     valuetmpdistance  =  0;
int     valuetmpargument  =  0;
Uint32  valuelastindent   =  0;
Uint16  valueoldtarget    =  0;
int     valueoperationsum =  0;
bool_t  valuegopoof       =  bfalse;
bool_t  valuechanged      =  bfalse;
bool_t  valueterminate    =  bfalse;

static bool_t debug_scripts = bfalse;

#define FUNCTION_BIT 0x80000000
#define DATA_BITS    0x78000000
#define VALUE_BITS   0x07FFFFFF
#define END_VALUE    (FUNCTION_BIT | FEND)

#define GET_DATA_BITS(X) ( ( (X) >>   27 ) &  0x0F )
#define SET_DATA_BITS(X) ( ( (X) &  0x0F ) <<   27 )

static char * script_error_classname = NULL;
static Uint16 script_error_model     = (Uint16)(~0);

int    iLoadSize;
int    iNumLine;
char * szScriptName;

int    iLineSize;
char   cLineBuffer[MAXLINESIZE];

Uint32 iCompiledAis[AISMAXCOMPILESIZE];

int    iNumAis;
int    iAisIndex;
Uint32 iAisStartPosition[MAXAI];
Uint32 iAisEndPosition[MAXAI];

int    iNumCode;
Uint8  cCodeType[MAXCODE];
Uint32 iCodeValue[MAXCODE];
char   cCodeName[MAXCODE][MAXCODENAMESIZE];

int    Token_iLine;
int    Token_iIndex;
int    Token_iValue;
char   Token_cType;
char   Token_cWord[MAXCODENAMESIZE];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// AI Script Routines

void insert_space( int position )
{
    // ZZ> This function adds a space into the load line if there isn't one
    //     there already
    char cTmp, cSwap;

    if ( cLineBuffer[position] != ' ' )
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

        cLineBuffer[position] = 0; // or cTmp as cTmp == 0
    }
}

//--------------------------------------------------------------------------------------------
void copy_one_line( int write )
{
    // ZZ> This function copies the line back to the load buffer
    int read;
    char cTmp;

    read = 0;
    cTmp = cLineBuffer[read];

    while ( cTmp != 0 )
    {
        cTmp = cLineBuffer[read];  read++;
        cLoadBuffer[write] = cTmp;  write++;
    }

    iNumLine++;
}

//--------------------------------------------------------------------------------------------
int load_one_line( int read )
{
    // ZZ> This function loads a line into the line buffer
    int stillgoing, foundtext;
    char cTmp;

    // Parse to start to maintain indentation
    iLineSize = 0;
    stillgoing = btrue;

    while ( stillgoing && read < iLoadSize )
    {
        cTmp = cLoadBuffer[read];
        stillgoing = bfalse;

        if ( cTmp == ' ' )
        {
            read++;
            cLineBuffer[iLineSize] = cTmp; iLineSize++;
            stillgoing = btrue;
        }
    }

    // Parse to comment or end of line
    foundtext = bfalse;
    stillgoing = btrue;

    while ( stillgoing && read < iLoadSize )
    {
        cTmp = cLoadBuffer[read];  read++;

        if ( cTmp == '\t' )
            cTmp = ' ';

        if ( cTmp != ' ' && cTmp != 0x0d && cTmp != 0x0a &&
                ( cTmp != '/' || cLoadBuffer[read] != '/' ) )
            foundtext = btrue;

        cLineBuffer[iLineSize] = cTmp;

        if ( cTmp != ' ' || ( cLoadBuffer[read] != ' ' && cLoadBuffer[read] != '\t' ) )
            iLineSize++;

        if ( cTmp == 0x0d || cTmp == 0x0a )
            stillgoing = bfalse;

        if ( cTmp == '/' && cLoadBuffer[read] == '/' )
            stillgoing = bfalse;
    }

    if ( !stillgoing )
        iLineSize--;

    cLineBuffer[iLineSize] = 0;

    if ( iLineSize >= 1 )
    {
        if ( cLineBuffer[iLineSize-1] == ' ' )
        {
            iLineSize--;  cLineBuffer[iLineSize] = 0;
        }
    }

    iLineSize++;

    // Parse to end of line
    stillgoing = btrue;
    read--;

    while ( stillgoing )
    {
        cTmp = cLoadBuffer[read];  read++;

        if ( cTmp == 0x0a || cTmp == 0x0d )
            stillgoing = bfalse;
    }

    if ( !foundtext )
    {
        iLineSize = 0;
    }

    return read;
}

//--------------------------------------------------------------------------------------------
int load_parsed_line( int read )
{
    // ZZ> This function loads a line into the line buffer
    char cTmp;

    // Parse to start to maintain indentation
    iLineSize = 0;
    cTmp = cLoadBuffer[read];

    while ( cTmp != 0 )
    {
        cLineBuffer[iLineSize] = cTmp;  iLineSize++;
        read++;  cTmp = cLoadBuffer[read];
    }

    cLineBuffer[iLineSize] = 0;
    read++; // skip terminating zero for next call of load_parsed_line()
    return read;
}

//--------------------------------------------------------------------------------------------
void surround_space( int position )
{
    insert_space( position + 1 );

    if ( position > 0 )
    {
        if ( cLineBuffer[position-1] != ' ' )
        {
            insert_space( position );
        }
    }
}

//--------------------------------------------------------------------------------------------
void parse_null_terminate_comments()
{
    // ZZ> This function removes comments and endline codes, replacing
    //     them with a 0
    int read, write;

    read = 0;
    write = 0;

    while ( read < iLoadSize )
    {
        read = load_one_line( read );

        if ( iLineSize > 2 )
        {
            copy_one_line( write );
            write += iLineSize;
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
    while ( cTmp == ' ' )
    {
        cnt++;
        cTmp = cLineBuffer[cnt];
    }

    if ( 0 != (cnt & 1) )
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

        if ( cTmp == '+' || cTmp == '-' || cTmp == '/' || cTmp == '*' ||
                cTmp == '%' || cTmp == '>' || cTmp == '<' || cTmp == '&' ||
                cTmp == '=' )
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
    int cnt, wordsize, codecorrect;
    char cTmp;
    IDSZ idsz;

    // Check bounds
    Token_iIndex = MAXCODE;

    if ( read >= iLineSize )  return read;

    // Skip spaces
    cTmp = cLineBuffer[read];
    while ( cTmp == ' ' )
    {
        read++;
        cTmp = cLineBuffer[read];
    }

    if ( read >= iLineSize )  return read;

    // Load the word into the other buffer
    wordsize = 0;
    while ( cTmp != ' ' && cTmp != '\0' )
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
        return read;
    }

    // Check for IDSZ constant
    if ( Token_cWord[0] == '[' )
    {
        idsz = 0;

        cTmp = (Token_cWord[1] - 'A') & 0x1F;  idsz |= cTmp << 15;
        cTmp = (Token_cWord[2] - 'A') & 0x1F;  idsz |= cTmp << 10;
        cTmp = (Token_cWord[3] - 'A') & 0x1F;  idsz |= cTmp << 5;
        cTmp = (Token_cWord[4] - 'A') & 0x1F;  idsz |= cTmp;

        Token_iValue = idsz;
        Token_cType  = 'C';
        Token_iIndex = MAXCODE;

        return read;
    }

    // Compare the word to all the codes
    codecorrect = bfalse;
    
    // compare in a case-insensitive manner. there is a unix-based function that does this,
    // but it is not sommon enough on non-linux compilers to be cross platform compatible
    for ( cnt = 0; cnt < iNumCode; cnt++ )
    {
        int i, maxlen;
        char * ptok, *pcode;
        bool_t found;

        ptok   = Token_cWord;
        pcode  = cCodeName[cnt];
        maxlen = MAXCODENAMESIZE;

        found = btrue;
        for( i = 0; i < maxlen && '\0' != *ptok && '\0' != pcode; i++ )
        {
            if( toupper( *ptok ) != toupper( *pcode ) )
            {
                found = bfalse;
                break;
            }

            ptok++;
            pcode++;
        }

        if ( found ) break;
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

    return read;
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
    for ( Token_iLine = 0; Token_iLine < iNumLine; Token_iLine++ )
    {
        parseposition = 0;

        read = load_parsed_line( read );
        fix_operators();

        //------------------------------
        // grab the first opcode

        highbits = SET_DATA_BITS( get_indentation() );
        parseposition = parse_token( parseposition ); 

        if ( 'F' == Token_cType )
        {
            if( FEND == Token_iValue && 0 == highbits )
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
            Uint32 initial_index = iAisIndex;

            // save in the value's opcode
            emit_opcode( highbits );

            // save a position for the operand count 
            Token_iValue = 0;
            operand_index = iAisIndex;
            emit_opcode( 0 );

            // handle the "="
            highbits = 0;
            parseposition = parse_token( parseposition );  // EQUALS
            if( 'O' != Token_cType || 0 != strcmp(Token_cWord, "=") )
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
                log_warning( "Invalid operand \"%s\"(%d) - \"%s\"\n", globalparsename, Token_iLine, cLineBuffer);

                emit_opcode( 0 );
                operands++;

                parseposition = parse_token( parseposition );
            }

            // expects a OPERATOR VALUE OPERATOR VALUE OPERATOR VALUE pattern
            while ( parseposition < iLineSize )
            {
                // the current token should be an operator

                if( 'O' != Token_cType )
                {
                    // problem with the loop
                    log_warning( "Expected an operator \"%s\"(%d) - \"%s\"\n", globalparsename, Token_iLine, cLineBuffer);
                    break;
                }

                // the highbits are the operator's value
                highbits = SET_DATA_BITS( Token_iValue );

                // VALUE
                parseposition = parse_token( parseposition );
               
                if( 'C' != Token_cType && 'V' != Token_cType )
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
        if ( 0 != ( value & FUNCTION_BIT ) )
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
/*void log_code(int ainumber, char* savename)
{
    // ZZ> This function shows the actual code, saving it in a file
    int index;
    Uint32 value;
    FILE* filewrite;

    filewrite = fopen(savename, "w");
    if(filewrite)
    {
        index = iAisStartPosition[ainumber];
        value = iCompiledAis[index];
        while(value != END_VALUE)  // End Function
        {
            value = iCompiledAis[index];
            fprintf(filewrite, "0x%08x--0x%08x\n", index, value);
            index++;
        }
        fclose(filewrite);
    }
  SDL_Quit();
}*/

//--------------------------------------------------------------------------------------------
int ai_goto_colon( int read )
{
    // ZZ> This function goes to spot after the next colon
    char cTmp;

    cTmp = cLoadBuffer[read];

    while ( cTmp != ':' && read < iLoadSize )
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
void load_ai_codes( char* loadname )
{
    // ZZ> This function loads all of the function and variable names
    FILE* fileread;
    int read;

    iNumCode = 0;
    fileread = fopen( loadname, "rb" );

    if ( fileread )
    {
        iLoadSize = ( int )fread( cLoadBuffer, 1, MD2MAXLOADSIZE, fileread );
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
}

//--------------------------------------------------------------------------------------------
int load_ai_script( char *loadname )
{
    // ZZ> This function loads a script to memory and
    //     returns bfalse if it fails to do so
    FILE* fileread;

    iNumLine = 0;
    fileread = fopen( loadname, "rb" );

    // No such file
    if ( NULL == fileread )
    {
        if( gDevMode )
        {
            log_message( "DEBUG: I am missing a AI script (%s)\n", loadname );
            log_message( "       Using the default AI script instead (basicdat" SLASH_STR "script.txt)\n" );
        }

        return 0;
    }

    if ( iNumAis >= MAXAI )
    {
        log_warning( "Too many script files. Cannot load file \"%s\"\n", loadname );
        return 0;
    }

    // load the file
    iLoadSize = ( int )fread( cLoadBuffer, 1, MD2MAXLOADSIZE, fileread );
    fclose( fileread );

    // if the file is empty, use the default script
    if(0 == iLoadSize) 
    {
        log_warning( "Script file is empty. \"%s\"\n", loadname );
        return 0;
    }

    // save the filename for error logging
    globalparsename = loadname;

    // initialize the start and end position
    iAisStartPosition[iNumAis] = iAisIndex;
    iAisEndPosition[iNumAis]   = iAisIndex;

    // parse/compile the scripts
    parse_null_terminate_comments();
    parse_line_by_line();

    // set the end position of the script
    iAisEndPosition[iNumAis] = iAisIndex;

    // determine the correct jumps
    parse_jumps( iNumAis );

    // increase the ai script index
    iNumAis++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void reset_ai_script()
{
    // ZZ> This function starts ai loading in the right spot
    int cnt;

    for ( cnt = 0; cnt < MAXMODEL; cnt++ )
    {
        madai[cnt] = 0;
    }

    iAisIndex = 0;
    iNumAis = 0;
}

//--------------------------------------------------------------------------------------------
Uint32 run_operation( Uint32 opcode, Uint32 index, Uint32 index_end, int character )
{
    char * variable;
    int i, operand_count;

    index++;

    // debug stuff
    variable = "UNKNOWN";
    if ( debug_scripts )
    {
        Uint32 indent, valuecode;

        indent = GET_DATA_BITS( opcode );
        for(i=0; i<indent; i++) { printf( "  " ); }

        valuecode = opcode & VALUE_BITS; 
        for(i=0; i<MAXCODE; i++)
        {
            if( 'V' == cCodeType[i] && valuecode == iCodeValue[i] )
            {
                variable = cCodeName[i];
                break;
            };
        }

        printf( "%s = ", variable );
    }

    // Get the number of operands
    operand_count = iCompiledAis[index];
    index++;

    // Now run the operation
    valueoperationsum = 0;
    for ( i=0; i < operand_count && index < index_end; i++, index++ )
    {
        run_operand( iCompiledAis[index], character ); // This sets valueoperationsum
    }
    if ( debug_scripts ) printf( "\n" );

    // Save the results in the register that called the arithmetic
    set_operand( opcode & VALUE_BITS );

    return index;
}

//--------------------------------------------------------------------------------------------
Uint8 run_function( Uint32 value, int character )
{
    // ZZ> This function runs a script function for the AI.
    //     It returns bfalse if the script should jump over the
    //     indented code that follows

    // Mask out the indentation
    Uint32 valuecode = value & VALUE_BITS;

    // Assume that the function will pass, as most do
    Uint8 returncode = btrue;

    Uint16 sTmp;
    float fTmp;
    int iTmp, tTmp;
    int volume;
    IDSZ test;
    char cTmp[256];


    if ( MAXCODE == valuecode )
    {
        log_message( "SCRIPT ERROR: run_function() - model == %d, class name == \"%s\" - Unknown opcode found!\n", script_error_model, script_error_classname );
        return bfalse;
    }

    // debug stuff
    if ( debug_scripts )
    {
        int i;
        Uint32 indent;
        indent = GET_DATA_BITS( value );
        for(i=0; i<indent; i++) { printf( "  " ); }

        for(i=0; i<MAXCODE; i++)
        {
            if( 'F' == cCodeType[i] && valuecode == iCodeValue[i] )
            {
                printf( "%s\n", cCodeName[i] );
                break;
            };
        }
    }

    // Figure out which function to run
    switch ( valuecode )
    {
        case FIFSPAWNED:
            // Proceed only if it's a new character
            returncode = ( ( chr[character].alert & ALERTIFSPAWNED ) != 0 );
            break;

        case FIFTIMEOUT:
            // Proceed only if time alert is set
            returncode = ( chr[character].aitime == 0 );
            break;

        case FIFATWAYPOINT:
            // Proceed only if the character reached a waypoint
            returncode = ( ( chr[character].alert & ALERTIFATWAYPOINT ) != 0 );
            break;

        case FIFATLASTWAYPOINT:
            // Proceed only if the character reached its last waypoint
            returncode = ( ( chr[character].alert & ALERTIFATLASTWAYPOINT ) != 0 );
            break;

        case FIFATTACKED:
            // Proceed only if the character was damaged
            returncode = ( ( chr[character].alert & ALERTIFATTACKED ) != 0 );
            break;

        case FIFBUMPED:
            // Proceed only if the character was bumped
            returncode = ( ( chr[character].alert & ALERTIFBUMPED ) != 0 );
            break;

        case FIFORDERED:
            // Proceed only if the character was ordered
            returncode = ( ( chr[character].alert & ALERTIFORDERED ) != 0 );
            break;

        case FIFCALLEDFORHELP:
            // Proceed only if the character was called for help
            returncode = ( ( chr[character].alert & ALERTIFCALLEDFORHELP ) != 0 );
            break;

        case FSETCONTENT:
            // Set the content
            chr[character].aicontent = valuetmpargument;
            break;

        case FIFKILLED:
            // Proceed only if the character's been killed
            returncode = ( ( chr[character].alert & ALERTIFKILLED ) != 0 );
            break;

        case FIFTARGETKILLED:
            // Proceed only if the character's target has just died
            returncode = ( ( chr[character].alert & ALERTIFTARGETKILLED ) != 0 );
            break;

        case FCLEARWAYPOINTS:
            // Clear out all waypoints
            chr[character].aigoto = 0;
            chr[character].aigotoadd = 0;
            break;

        case FADDWAYPOINT:
            // Add a waypoint to the waypoint list
            chr[character].aigotox[chr[character].aigotoadd] = valuetmpx;
            chr[character].aigotoy[chr[character].aigotoadd] = valuetmpy;

            chr[character].aigotoadd++;
            if ( chr[character].aigotoadd > MAXWAY - 1 )  chr[character].aigotoadd = MAXWAY - 1;
            break;

        case FFINDPATH:
            // Yep this is it
            if ( chr[chr[character].aitarget].model != character )
            {
                if ( valuetmpdistance != MOVE_FOLLOW )
                {
                    valuetmpx = chr[chr[ chr[chr[character].aitarget].model ].aitarget].xpos;
                    valuetmpy = chr[chr[ chr[chr[character].aitarget].model ].aitarget].ypos;
                }
                else
                {
                    valuetmpx = ( rand() & 1023 ) - 512 + chr[chr[ chr[chr[character].aitarget].model ].aitarget].xpos;
                    valuetmpy = ( rand() & 1023 ) - 512 + chr[chr[ chr[chr[character].aitarget].model ].aitarget].ypos;
                }

                valuetmpturn = ATAN2( chr[chr[character].aitarget].ypos - chr[character].ypos, chr[chr[character].aitarget].xpos - chr[character].xpos ) * 65535 / ( TWO_PI );
                if ( valuetmpdistance == MOVE_RETREAT )
                {
                    valuetmpturn += ( rand() & 16383 ) - 8192;
                }
                else
                {
                    valuetmpturn += 32768;
                }

                if ( valuetmpdistance == MOVE_CHARGE || valuetmpdistance == MOVE_RETREAT )
                {
                    reset_character_accel( character ); // Force 100% speed
                }

                // Secondly we run the Compass function (If we are not in follow mode)
                if ( valuetmpdistance != MOVE_FOLLOW )
                {
                    sTmp = ( valuetmpturn + 16384 );
                    valuetmpx = valuetmpx - turntocos[valuetmpturn>>2] * valuetmpdistance;
                    valuetmpy = valuetmpy - turntosin[valuetmpturn>>2] * valuetmpdistance;
                }

                // Then we add the waypoint(s), without clearing existing ones...
                chr[character].aigotox[chr[character].aigotoadd] = valuetmpx;
                chr[character].aigotoy[chr[character].aigotoadd] = valuetmpy;

                chr[character].aigotoadd++;
                if ( chr[character].aigotoadd > MAXWAY - 1 ) chr[character].aigotoadd = MAXWAY - 1;

            }
            break;

        case FCOMPASS:
            // This function changes tmpx and tmpy in a circlular manner according
            // to tmpturn and tmpdistance
            sTmp = ( valuetmpturn + 16384 );
            valuetmpx -= turntocos[valuetmpturn>>2] * valuetmpdistance;
            valuetmpy -= turntosin[valuetmpturn>>2] * valuetmpdistance;
            break;

        case FGETTARGETARMORPRICE:
            // This function gets the armor cost for the given skin
            sTmp = valuetmpargument & 3;
            valuetmpx = capskincost[chr[chr[character].aitarget].model][sTmp];
            break;

        case FSETTIME:

            // This function resets the time
            if ( valuetmpargument > -1 )
                chr[character].aitime = valuetmpargument;

            break;

        case FGETCONTENT:
            // Get the content
            valuetmpargument = chr[character].aicontent;
            break;

        case FJOINTARGETTEAM:
            // This function allows the character to leave its own team and join another
            returncode = bfalse;

            if ( chr[chr[character].aitarget].on )
            {
                switch_team( character, chr[chr[character].aitarget].team );
                returncode = btrue;
            }
            break;

        case FSETTARGETTONEARBYENEMY:
            // This function finds a nearby enemy, and proceeds only if there is one
            returncode = bfalse;

            if (get_target( character, NEARBY, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse) != MAXCHR) returncode = btrue;

            break;

        case FSETTARGETTOTARGETLEFTHAND:
            // This function sets the target to the target's left item
            sTmp = chr[chr[character].aitarget].holdingwhich[0];
            returncode = bfalse;

            if ( sTmp != MAXCHR )
            {
                chr[character].aitarget = sTmp;
                returncode = btrue;
            }
            break;

        case FSETTARGETTOTARGETRIGHTHAND:
            // This function sets the target to the target's right item
            sTmp = chr[chr[character].aitarget].holdingwhich[1];
            returncode = bfalse;

            if ( sTmp != MAXCHR )
            {
                chr[character].aitarget = sTmp;
                returncode = btrue;
            }
            break;

        case FSETTARGETTOWHOEVERATTACKED:

            // This function sets the target to whoever attacked the character last,
            // failing for damage tiles
            if ( chr[character].attacklast != MAXCHR )
            {
                chr[character].aitarget = chr[character].attacklast;
            }
            else
            {
                returncode = bfalse;
            }
            break;

        case FSETTARGETTOWHOEVERBUMPED:
            // This function sets the target to whoever bumped into the
            // character last.  It never fails
            chr[character].aitarget = chr[character].bumplast;
            break;

        case FSETTARGETTOWHOEVERCALLEDFORHELP:
            // This function sets the target to whoever needs help
            chr[character].aitarget = teamsissy[chr[character].team];
            break;

        case FSETTARGETTOOLDTARGET:
            // This function reverts to the target with whom the script started
            chr[character].aitarget = valueoldtarget;
            break;

        case FSETTURNMODETOVELOCITY:
            // This function sets the turn mode
            chr[character].turnmode = TURNMODEVELOCITY;
            break;

        case FSETTURNMODETOWATCH:
            // This function sets the turn mode
            chr[character].turnmode = TURNMODEWATCH;
            break;

        case FSETTURNMODETOSPIN:
            // This function sets the turn mode
            chr[character].turnmode = TURNMODESPIN;
            break;

        case FSETBUMPHEIGHT:
            // This function changes a character's bump height
            chr[character].bumpheight = valuetmpargument * chr[character].fat;
            chr[character].bumpheightsave = valuetmpargument;
            break;

        case FIFTARGETHASID:
            // This function proceeds if ID matches tmpargument
            sTmp = chr[chr[character].aitarget].model;
            returncode = capidsz[sTmp][IDSZ_PARENT] == ( Uint32 ) valuetmpargument;
            returncode = returncode | ( capidsz[sTmp][IDSZ_TYPE] == ( Uint32 ) valuetmpargument );
            break;

        case FIFTARGETHASITEMID:
            // This function proceeds if the target has a matching item in his/her pack
            returncode = bfalse;
            // Check the pack
            sTmp = chr[chr[character].aitarget].nextinpack;

            while ( sTmp != MAXCHR )
            {
                if ( capidsz[chr[sTmp].model][IDSZ_PARENT] == ( Uint32 ) valuetmpargument || capidsz[chr[sTmp].model][IDSZ_TYPE] == ( Uint32 ) valuetmpargument )
                {
                    returncode = btrue;
                    sTmp = MAXCHR;
                }
                else
                {
                    sTmp = chr[sTmp].nextinpack;
                }
            }

            // Check left hand
            sTmp = chr[chr[character].aitarget].holdingwhich[0];

            if ( sTmp != MAXCHR )
            {
                sTmp = chr[sTmp].model;

                if ( capidsz[sTmp][IDSZ_PARENT] == ( Uint32 ) valuetmpargument || capidsz[sTmp][IDSZ_TYPE] == ( Uint32 ) valuetmpargument )
                    returncode = btrue;
            }

            // Check right hand
            sTmp = chr[chr[character].aitarget].holdingwhich[1];

            if ( sTmp != MAXCHR )
            {
                sTmp = chr[sTmp].model;

                if ( capidsz[sTmp][IDSZ_PARENT] == ( Uint32 ) valuetmpargument || capidsz[sTmp][IDSZ_TYPE] == ( Uint32 ) valuetmpargument )
                    returncode = btrue;
            }
            break;

        case FIFTARGETHOLDINGITEMID:
            // This function proceeds if ID matches tmpargument and returns the latch for the
            // hand in tmpargument
            returncode = bfalse;
            // Check left hand
            sTmp = chr[chr[character].aitarget].holdingwhich[0];

            if ( sTmp != MAXCHR )
            {
                sTmp = chr[sTmp].model;

                if ( capidsz[sTmp][IDSZ_PARENT] == ( IDSZ ) valuetmpargument || capidsz[sTmp][IDSZ_TYPE] == ( IDSZ ) valuetmpargument )
                {
                    valuetmpargument = LATCHBUTTONLEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            sTmp = chr[chr[character].aitarget].holdingwhich[1];

            if ( sTmp != MAXCHR && !returncode )
            {
                sTmp = chr[sTmp].model;

                if ( capidsz[sTmp][IDSZ_PARENT] == ( IDSZ ) valuetmpargument || capidsz[sTmp][IDSZ_TYPE] == ( IDSZ ) valuetmpargument )
                {
                    valuetmpargument = LATCHBUTTONRIGHT;
                    returncode = btrue;
                }
            }
            break;

        case FIFTARGETHASSKILLID:
            // This function proceeds if ID matches tmpargument
            returncode = bfalse;

            returncode = (0 != check_skills( chr[character].aitarget, ( IDSZ )valuetmpargument ));

            break;

        case FELSE:

            // This function fails if the last one was more indented
            if ( valuelastindent > ( value & DATA_BITS ) )
                returncode = bfalse;

            break;

        case FRUN:
            reset_character_accel( character );
            break;

        case FWALK:
            reset_character_accel( character );
            chr[character].maxaccel *= 0.66f;
            break;

        case FSNEAK:
            reset_character_accel( character );
            chr[character].maxaccel *= 0.33f;
            break;

        case FDOACTION:
            // This function starts a new action, if it is valid for the model
            // It will fail if the action is invalid or if the character is doing
            // something else already
            returncode = bfalse;

            if ( valuetmpargument < MAXACTION && chr[character].actionready )
            {
                if ( madactionvalid[chr[character].model][valuetmpargument] )
                {
                    chr[character].action = valuetmpargument;
                    chr[character].lip = 0;
                    chr[character].lastframe = chr[character].frame;
                    chr[character].frame = madactionstart[chr[character].model][valuetmpargument];
                    chr[character].actionready = bfalse;
                    returncode = btrue;
                }
            }
            break;

        case FKEEPACTION:
            // This function makes the current animation halt on the last frame
            chr[character].keepaction = btrue;
            break;

        case FISSUEORDER:
            // This function issues an order to all teammates
            issue_order( character, valuetmpargument );
            break;

        case FDROPWEAPONS:
            // This funtion drops the character's in hand items/riders
            sTmp = chr[character].holdingwhich[0];

            if ( sTmp != MAXCHR )
            {
                detach_character_from_mount( sTmp, btrue, btrue );

                if ( chr[character].ismount )
                {
                    chr[sTmp].zvel = DISMOUNTZVEL;
                    chr[sTmp].zpos += DISMOUNTZVEL;
                    chr[sTmp].jumptime = JUMPDELAY;
                }
            }

            sTmp = chr[character].holdingwhich[1];

            if ( sTmp != MAXCHR )
            {
                detach_character_from_mount( sTmp, btrue, btrue );

                if ( chr[character].ismount )
                {
                    chr[sTmp].zvel = DISMOUNTZVEL;
                    chr[sTmp].zpos += DISMOUNTZVEL;
                    chr[sTmp].jumptime = JUMPDELAY;
                }
            }
            break;

        case FTARGETDOACTION:
            // This function starts a new action, if it is valid for the model
            // It will fail if the action is invalid or if the target is doing
            // something else already
            returncode = bfalse;

            if ( chr[chr[character].aitarget].alive )
            {
                if ( valuetmpargument < MAXACTION && chr[chr[character].aitarget].actionready )
                {
                    if ( madactionvalid[chr[chr[character].aitarget].model][valuetmpargument] )
                    {
                        chr[chr[character].aitarget].action = valuetmpargument;
                        chr[chr[character].aitarget].lip = 0;
                        chr[chr[character].aitarget].lastframe = chr[chr[character].aitarget].frame;
                        chr[chr[character].aitarget].frame = madactionstart[chr[chr[character].aitarget].model][valuetmpargument];
                        chr[chr[character].aitarget].actionready = bfalse;
                        returncode = btrue;
                    }
                }
            }
            break;

        case FOPENPASSAGE:
            // This function opens the passage specified by tmpargument, failing if the
            // passage was already open
            returncode = open_passage( valuetmpargument );
            break;

        case FCLOSEPASSAGE:
            // This function closes the passage specified by tmpargument, and proceeds
            // only if the passage is clear of obstructions
            returncode = close_passage( valuetmpargument );
            break;

        case FIFPASSAGEOPEN:
            // This function proceeds only if the passage specified by tmpargument
            // is both valid and open
            returncode = bfalse;

            if ( valuetmpargument < numpassage && valuetmpargument >= 0 )
            {
                returncode = passopen[valuetmpargument];
            }
            break;

        case FGOPOOF:
            // This function flags the character to be removed from the game
            returncode = bfalse;

            if ( !chr[character].isplayer )
            {
                returncode = btrue;
                valuegopoof = btrue;
            }
            break;

        case FCOSTTARGETITEMID:
            // This function checks if the target has a matching item, and poofs it
            returncode = bfalse;
            // Check the pack
            iTmp = MAXCHR;
            tTmp = chr[character].aitarget;
            sTmp = chr[tTmp].nextinpack;

            while ( sTmp != MAXCHR )
            {
                if ( capidsz[chr[sTmp].model][IDSZ_PARENT] == ( IDSZ) valuetmpargument || capidsz[chr[sTmp].model][IDSZ_TYPE] == ( IDSZ ) valuetmpargument )
                {
                    returncode = btrue;
                    iTmp = sTmp;
                    sTmp = MAXCHR;
                }
                else
                {
                    tTmp = sTmp;
                    sTmp = chr[sTmp].nextinpack;
                }
            }

            // Check left hand
            sTmp = chr[chr[character].aitarget].holdingwhich[0];

            if ( sTmp != MAXCHR )
            {
                sTmp = chr[sTmp].model;

                if ( capidsz[sTmp][IDSZ_PARENT] == ( IDSZ ) valuetmpargument || capidsz[sTmp][IDSZ_TYPE] == ( IDSZ ) valuetmpargument )
                {
                    returncode = btrue;
                    iTmp = chr[chr[character].aitarget].holdingwhich[0];
                }
            }

            // Check right hand
            sTmp = chr[chr[character].aitarget].holdingwhich[1];

            if ( sTmp != MAXCHR )
            {
                sTmp = chr[sTmp].model;

                if ( capidsz[sTmp][IDSZ_PARENT] == ( IDSZ ) valuetmpargument || capidsz[sTmp][IDSZ_TYPE] == ( IDSZ ) valuetmpargument )
                {
                    returncode = btrue;
                    iTmp = chr[chr[character].aitarget].holdingwhich[1];
                }
            }

            if ( returncode )
            {
                if ( chr[iTmp].ammo <= 1 )
                {
                    // Poof the item
                    if ( chr[iTmp].inpack )
                    {
                        // Remove from the pack
                        chr[tTmp].nextinpack = chr[iTmp].nextinpack;
                        chr[chr[character].aitarget].numinpack--;
                        free_one_character( iTmp );
                    }
                    else
                    {
                        // Drop from hand
                        detach_character_from_mount( iTmp, btrue, bfalse );
                        free_one_character( iTmp );
                    }
                }
                else
                {
                    // Cost one ammo
                    chr[iTmp].ammo--;
                }
            }
            break;

        case FDOACTIONOVERRIDE:
            // This function starts a new action, if it is valid for the model
            // It will fail if the action is invalid
            returncode = bfalse;

            if ( valuetmpargument < MAXACTION )
            {
                if ( madactionvalid[chr[character].model][valuetmpargument] )
                {
                    chr[character].action = valuetmpargument;
                    chr[character].lip = 0;
                    chr[character].lastframe = chr[character].frame;
                    chr[character].frame = madactionstart[chr[character].model][valuetmpargument];
                    chr[character].actionready = bfalse;
                    returncode = btrue;
                }
            }
            break;

        case FIFHEALED:
            // Proceed only if the character was healed
            returncode = ( ( chr[character].alert & ALERTIFHEALED ) != 0 );
            break;

        case FSENDMESSAGE:
            // This function sends a message to the players
            display_message( madmsgstart[chr[character].model] + valuetmpargument, character );
            break;

        case FCALLFORHELP:
            // This function issues a call for help
            call_for_help( character );
            break;

        case FADDIDSZ:
            // This function adds an idsz to the module's menu.txt file
            add_module_idsz( pickedmodule, valuetmpargument );
            break;

        case FSETSTATE:
            // This function sets the character's state variable
            chr[character].aistate = valuetmpargument;
            break;

        case FGETSTATE:
            // This function reads the character's state variable
            valuetmpargument = chr[character].aistate;
            break;

        case FIFSTATEIS:
            // This function fails if the character's state is inequal to tmpargument
            returncode = ( valuetmpargument == chr[character].aistate );
            break;

        case FIFTARGETCANOPENSTUFF:
            // This function fails if the target can't open stuff
            returncode = chr[chr[character].aitarget].openstuff;
            break;

        case FIFGRABBED:
            // Proceed only if the character was picked up
            returncode = ( ( chr[character].alert & ALERTIFGRABBED ) != 0 );
            break;

        case FIFDROPPED:
            // Proceed only if the character was dropped
            returncode = ( ( chr[character].alert & ALERTIFDROPPED ) != 0 );
            break;

        case FSETTARGETTOWHOEVERISHOLDING:
            // This function sets the target to the character's mount or holder,
            // failing if the character has no mount or holder
            returncode = bfalse;

            if ( chr[character].attachedto < MAXCHR )
            {
                chr[character].aitarget = chr[character].attachedto;
                returncode = btrue;
            }
            break;

        case FDAMAGETARGET:
            // This function applies little bit of love to the character's target.
            // The amount is set in tmpargument
            damage_character( chr[character].aitarget, 0, valuetmpargument, 1, chr[character].damagetargettype, chr[character].team, character, DAMFXBLOC );
            break;

        case FIFXISLESSTHANY:
            // Proceed only if tmpx is less than tmpy
            returncode = ( valuetmpx < valuetmpy );
            break;

        case FSETWEATHERTIME:
            // Set the weather timer
            weathertimereset = valuetmpargument;
            weathertime = valuetmpargument;
            break;

        case FGETBUMPHEIGHT:
            // Get the characters bump height
            valuetmpargument = chr[character].bumpheight;
            break;

        case FIFREAFFIRMED:
            // Proceed only if the character was reaffirmed
            returncode = ( ( chr[character].alert & ALERTIFREAFFIRMED ) != 0 );
            break;

        case FUNKEEPACTION:
            // This function makes the current animation start again
            chr[character].keepaction = bfalse;
            break;

        case FIFTARGETISONOTHERTEAM:
            // This function proceeds only if the target is on another team
            returncode = ( chr[chr[character].aitarget].alive && chr[chr[character].aitarget].team != chr[character].team );
            break;

        case FIFTARGETISONHATEDTEAM:
            // This function proceeds only if the target is on an enemy team
            returncode = ( chr[chr[character].aitarget].alive && teamhatesteam[chr[character].team][chr[chr[character].aitarget].team] && !chr[chr[character].aitarget].invictus );
            break;

        case FPRESSLATCHBUTTON:
            // This function sets the latch buttons
            chr[character].latchbutton = chr[character].latchbutton | valuetmpargument;
            break;

        case FSETTARGETTOTARGETOFLEADER:
            // This function sets the character's target to the target of its leader,
            // or it fails with no change if the leader is dead
            returncode = bfalse;

            if ( teamleader[chr[character].team] != NOLEADER )
            {
                chr[character].aitarget = chr[teamleader[chr[character].team]].aitarget;
                returncode = btrue;
            }
            break;

        case FIFLEADERKILLED:
            // This function proceeds only if the character's leader has just died
            returncode = ( ( chr[character].alert & ALERTIFLEADERKILLED ) != 0 );
            break;

        case FBECOMELEADER:
            // This function makes the character the team leader
            teamleader[chr[character].team] = character;
            break;

        case FCHANGETARGETARMOR:
            // This function sets the target's armor type and returns the old type
            // as tmpargument and the new type as tmpx
            iTmp = chr[chr[character].aitarget].texture - madskinstart[chr[chr[character].aitarget].model];
            valuetmpx = change_armor( chr[character].aitarget, valuetmpargument );
            valuetmpargument = iTmp;  // The character's old armor
            break;

        case FGIVEMONEYTOTARGET:
            // This function transfers money from the character to the target, and sets
            // tmpargument to the amount transferred
            iTmp = chr[character].money;
            tTmp = chr[chr[character].aitarget].money;
            iTmp -= valuetmpargument;
            tTmp += valuetmpargument;

            if ( iTmp < 0 ) { tTmp += iTmp;  valuetmpargument += iTmp;  iTmp = 0; }

            if ( tTmp < 0 ) { iTmp += tTmp;  valuetmpargument += tTmp;  tTmp = 0; }

            if ( iTmp > MAXMONEY ) { iTmp = MAXMONEY; }

            if ( tTmp > MAXMONEY ) { tTmp = MAXMONEY; }

            chr[character].money = iTmp;
            chr[chr[character].aitarget].money = tTmp;
            break;

        case FDROPKEYS:
            drop_keys( character );
            break;

        case FIFLEADERISALIVE:
            // This function fails if there is no team leader
            returncode = ( teamleader[chr[character].team] != NOLEADER );
            break;

        case FIFTARGETISOLDTARGET:
            // This function returns bfalse if the target has valuechanged
            returncode = ( chr[character].aitarget == valueoldtarget );
            break;

        case FSETTARGETTOLEADER:

            // This function fails if there is no team leader
            if ( teamleader[chr[character].team] == NOLEADER )
            {
                returncode = bfalse;
            }
            else
            {
                chr[character].aitarget = teamleader[chr[character].team];
            }
            break;

        case FSPAWNCHARACTER:
            // This function spawns a character, failing if x,y is invalid
            sTmp = spawn_one_character( valuetmpx, valuetmpy, 0, chr[character].model, chr[character].team, 0, valuetmpturn, NULL, MAXCHR );
            returncode = bfalse;

            if ( sTmp < MAXCHR )
            {
                if ( __chrhitawall( sTmp ) )
                {
                    free_one_character( sTmp );
                }
                else
                {
                    tTmp = chr[character].turnleftright >> 2;
                    chr[sTmp].xvel += turntocos[( tTmp+8192 )&TRIG_TABLE_MASK] * valuetmpdistance;
                    chr[sTmp].yvel += turntosin[( tTmp+8192 )&TRIG_TABLE_MASK] * valuetmpdistance;
                    chr[sTmp].passage = chr[character].passage;
                    chr[sTmp].iskursed = bfalse;
                    chr[character].aichild = sTmp;
                    chr[sTmp].aiowner = chr[character].aiowner;
                    returncode = btrue;
                }
            }
            break;

        case FRESPAWNCHARACTER:
            // This function respawns the character at its starting location
            respawn_character( character );
            break;

        case FCHANGETILE:
            // This function changes the floor image under the character
            meshtile[chr[character].onwhichfan] = valuetmpargument & ( 255 );
            break;

        case FIFUSED:
            // This function proceeds only if the character has been used
            returncode = ( ( chr[character].alert & ALERTIFUSED ) != 0 );
            break;

        case FDROPMONEY:
            // This function drops some of a character's money
            drop_money( character, valuetmpargument );
            break;

        case FSETOLDTARGET:
            // This function sets the old target to the current target
            valueoldtarget = chr[character].aitarget;
            break;

        case FDETACHFROMHOLDER:

            // This function drops the character, failing only if it was not held
            if ( chr[character].attachedto != MAXCHR )
            {
                detach_character_from_mount( character, btrue, btrue );
            }
            else
            {
                returncode = bfalse;
            }
            break;

        case FIFTARGETHASVULNERABILITYID:
            // This function proceeds if ID matches tmpargument
            returncode = ( capidsz[chr[chr[character].aitarget].model][IDSZ_VULNERABILITY] == ( IDSZ ) valuetmpargument );
            break;

        case FCLEANUP:
            // This function issues the clean up order to all teammates
            issue_clean( character );
            break;

        case FIFCLEANEDUP:
            // This function proceeds only if the character was told to clean up
            returncode = ( ( chr[character].alert & ALERTIFCLEANEDUP ) != 0 );
            break;

        case FIFSITTING:
            // This function proceeds if the character is riding another
            returncode = ( chr[character].attachedto != MAXCHR );
            break;

        case FIFTARGETISHURT:

            // This function passes only if the target is hurt and alive
            if ( !chr[chr[character].aitarget].alive || chr[chr[character].aitarget].life > chr[chr[character].aitarget].lifemax - HURTDAMAGE )
                returncode = bfalse;

            break;

        case FIFTARGETISAPLAYER:
            // This function proceeds only if the target is a player ( may not be local )
            returncode = chr[chr[character].aitarget].isplayer;
            break;

        case FPLAYSOUND:

            // This function plays a sound
            if ( chr[character].oldz > PITNOSOUND && valuetmpargument >= 0 && valuetmpargument < MAXWAVE )
            {
                play_mix( chr[character].oldx, chr[character].oldy, capwaveindex[chr[character].model] + valuetmpargument );
            }
            break;

        case FSPAWNPARTICLE:
            // This function spawns a particle
            tTmp = character;

            if ( chr[character].attachedto != MAXCHR )  tTmp = chr[character].attachedto;

            tTmp = spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos, chr[character].turnleftright, chr[character].model, valuetmpargument, character, valuetmpdistance, chr[character].team, tTmp, 0, MAXCHR );

            if ( tTmp != maxparticles )
            {
                // Detach the particle
                attach_particle_to_character( tTmp, character, valuetmpdistance );
                prtattachedtocharacter[tTmp] = MAXCHR;
                // Correct X, Y, Z spacing
                prtxpos[tTmp] += valuetmpx;
                prtypos[tTmp] += valuetmpy;
                prtzpos[tTmp] += pipzspacingbase[prtpip[tTmp]];

                // Don't spawn in walls
                if ( __prthitawall( tTmp ) )
                {
                    prtxpos[tTmp] = chr[character].xpos;

                    if ( __prthitawall( tTmp ) )
                    {
                        prtypos[tTmp] = chr[character].ypos;
                    }
                }
            }
            break;

        case FIFTARGETISALIVE:
            // This function proceeds only if the target is alive
            returncode = chr[chr[character].aitarget].alive;
            break;

        case FSTOP:
            chr[character].maxaccel = 0;
            break;

        case FDISAFFIRMCHARACTER:
            disaffirm_attached_particles( character );
            break;

        case FREAFFIRMCHARACTER:
            reaffirm_attached_particles( character );
            break;

        case FIFTARGETISSELF:
            // This function proceeds only if the target is the character too
            returncode = ( chr[character].aitarget == character );
            break;

        case FIFTARGETISMALE:
            // This function proceeds only if the target is male
            returncode = ( chr[chr[character].aitarget].gender == GENMALE );
            break;

        case FIFTARGETISFEMALE:
            // This function proceeds only if the target is female
            returncode = ( chr[chr[character].aitarget].gender == GENFEMALE );
            break;

        case FSETTARGETTOSELF:
            // This function sets the target to the character
            chr[character].aitarget = character;
            break;

        case FSETTARGETTORIDER:

            // This function sets the target to the character's left/only grip weapon,
            // failing if there is none
            if ( chr[character].holdingwhich[0] == MAXCHR )
            {
                returncode = bfalse;
            }
            else
            {
                chr[character].aitarget = chr[character].holdingwhich[0];
            }
            break;

        case FGETATTACKTURN:
            // This function sets tmpturn to the direction of the last attack
            valuetmpturn = chr[character].directionlast;
            break;

        case FGETDAMAGETYPE:
            // This function gets the last type of damage
            valuetmpargument = chr[character].damagetypelast;
            break;

        case FBECOMESPELL:
            // This function turns the spellbook character into a spell based on its
            // content
            chr[character].money = ( chr[character].texture - madskinstart[chr[character].model] ) & 3;
            change_character( character, chr[character].aicontent, 0, LEAVENONE );
            chr[character].aicontent = 0;  // Reset so it doesn't mess up
            chr[character].aistate = 0;  // Reset so it doesn't mess up
            valuechanged = btrue;
            break;

        case FBECOMESPELLBOOK:
            // This function turns the spell into a spellbook, and sets the content
            // accordingly
            chr[character].aicontent = chr[character].model;
            change_character( character, SPELLBOOK, chr[character].money&3, LEAVENONE );
            chr[character].aistate = 0;  // Reset so it doesn't burn up
            valuechanged = btrue;
            break;

        case FIFSCOREDAHIT:
            // Proceed only if the character scored a hit
            returncode = ( ( chr[character].alert & ALERTIFSCOREDAHIT ) != 0 );
            break;

        case FIFDISAFFIRMED:
            // Proceed only if the character was disaffirmed
            returncode = ( ( chr[character].alert & ALERTIFDISAFFIRMED ) != 0 );
            break;

        case FTRANSLATEORDER:
            // This function gets the order and sets tmpx, tmpy, tmpargument and the
            // target ( if valid )
            sTmp = chr[character].order >> 24;

            if ( sTmp < MAXCHR )
            {
                chr[character].aitarget = sTmp;
            }

            valuetmpx = ( ( chr[character].order >> 14 ) & 1023 ) << 6;
            valuetmpy = ( ( chr[character].order >> 4 ) & 1023 ) << 6;
            valuetmpargument = chr[character].order & 15;
            break;

        case FSETTARGETTOWHOEVERWASHIT:
            // This function sets the target to whoever the character hit last,
            chr[character].aitarget = chr[character].hitlast;
            break;

        case FSETTARGETTOWIDEENEMY:
            // This function finds an enemy, and proceeds only if there is one
            returncode = bfalse;

            if (get_target( character, WIDE, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse) != MAXCHR) returncode = btrue;

            break;

        case FIFCHANGED:
            // Proceed only if the character was polymorphed
            returncode = ( ( chr[character].alert & ALERTIFCHANGED ) != 0 );
            break;

        case FIFINWATER:
            // Proceed only if the character got wet
            returncode = ( ( chr[character].alert & ALERTIFINWATER ) != 0 );
            break;

        case FIFBORED:
            // Proceed only if the character is bored
            returncode = ( ( chr[character].alert & ALERTIFBORED ) != 0 );
            break;

        case FIFTOOMUCHBAGGAGE:
            // Proceed only if the character tried to grab too much
            returncode = ( ( chr[character].alert & ALERTIFTOOMUCHBAGGAGE ) != 0 );
            break;

        case FIFGROGGED:
            // Proceed only if the character was grogged
            returncode = ( ( chr[character].alert & ALERTIFGROGGED ) != 0 );
            break;

        case FIFDAZED:
            // Proceed only if the character was dazed
            returncode = ( ( chr[character].alert & ALERTIFDAZED ) != 0 );
            break;

        case FIFTARGETHASSPECIALID:
            // This function proceeds if ID matches tmpargument
            returncode = ( capidsz[chr[chr[character].aitarget].model][IDSZ_SPECIAL] == ( IDSZ ) valuetmpargument );
            break;

        case FPRESSTARGETLATCHBUTTON:
            // This function sets the target's latch buttons
            chr[chr[character].aitarget].latchbutton = chr[chr[character].aitarget].latchbutton | valuetmpargument;
            break;

        case FIFINVISIBLE:
            // This function passes if the character is invisible
            returncode = ( chr[character].alpha <= INVISIBLE ) || ( chr[character].light <= INVISIBLE );
            break;

        case FIFARMORIS:
            // This function passes if the character's skin is tmpargument
            tTmp = chr[character].texture - madskinstart[chr[character].model];
            returncode = ( tTmp == valuetmpargument );
            break;

        case FGETTARGETGROGTIME:
            // This function returns tmpargument as the grog time, and passes if it is not 0
            valuetmpargument = chr[character].grogtime;
            returncode = ( valuetmpargument != 0 );
            break;

        case FGETTARGETDAZETIME:
            // This function returns tmpargument as the daze time, and passes if it is not 0
            valuetmpargument = chr[character].dazetime;
            returncode = ( valuetmpargument != 0 );
            break;

        case FSETDAMAGETYPE:
            // This function sets the bump damage type
            chr[character].damagetargettype = valuetmpargument & ( DAMAGE_COUNT - 1 );
            break;

        case FSETWATERLEVEL:
            // This function raises and lowers the module's water
            fTmp = ( valuetmpargument / 10.0f ) - waterdouselevel;
            watersurfacelevel += fTmp;
            waterdouselevel += fTmp;

            for ( iTmp = 0; iTmp < MAXWATERLAYER; iTmp++ )
            {
                waterlayerz[iTmp] += fTmp;
            }
            break;

        case FENCHANTTARGET:
            // This function enchants the target
            sTmp = spawn_enchant( chr[character].aiowner, chr[character].aitarget, character, MAXENCHANT, MAXMODEL );
            returncode = ( sTmp != MAXENCHANT );
            break;

        case FENCHANTCHILD:
            // This function can be used with SpawnCharacter to enchant the
            // newly spawned character
            sTmp = spawn_enchant( chr[character].aiowner, chr[character].aichild, character, MAXENCHANT, MAXMODEL );
            returncode = ( sTmp != MAXENCHANT );
            break;

        case FTELEPORTTARGET:
            // This function teleports the target to the X, Y location, failing if the
            // location is off the map or blocked
            returncode = bfalse;

            if ( valuetmpx > EDGE && valuetmpy > EDGE && valuetmpx < meshedgex - EDGE && valuetmpy < meshedgey - EDGE )
            {
                // Yeah!  It worked!
                sTmp = chr[character].aitarget;
                detach_character_from_mount( sTmp, btrue, bfalse );
                chr[sTmp].oldx = chr[sTmp].xpos;
                chr[sTmp].oldy = chr[sTmp].ypos;
                chr[sTmp].xpos = valuetmpx;
                chr[sTmp].ypos = valuetmpy;
                chr[sTmp].zpos = valuetmpdistance;
                chr[sTmp].turnleftright = valuetmpturn;

                if ( __chrhitawall( sTmp ) )
                {
                    // No it didn't...
                    chr[sTmp].xpos = chr[sTmp].oldx;
                    chr[sTmp].ypos = chr[sTmp].oldy;
                    chr[sTmp].zpos = chr[sTmp].oldz;
                    chr[sTmp].turnleftright = chr[sTmp].oldturn;
                    returncode = bfalse;
                }
                else
                {
                    chr[sTmp].oldx = chr[sTmp].xpos;
                    chr[sTmp].oldy = chr[sTmp].ypos;
                    chr[sTmp].oldz = chr[sTmp].zpos;
                    chr[sTmp].oldturn = chr[sTmp].turnleftright;
                    returncode = btrue;
                }
            }
            break;

        case FGIVEEXPERIENCETOTARGET:
            // This function gives the target some experience, xptype from distance,
            // amount from argument...
            give_experience( chr[character].aitarget, valuetmpargument, valuetmpdistance );
            break;

        case FINCREASEAMMO:

            // This function increases the ammo by one
            if ( chr[character].ammo < chr[character].ammomax )
            {
                chr[character].ammo++;
            }
            break;

        case FUNKURSETARGET:
            // This function unkurses the target
            chr[chr[character].aitarget].iskursed = bfalse;
            break;

        case FGIVEEXPERIENCETOTARGETTEAM:
            // This function gives experience to everyone on the target's team
            give_team_experience( chr[chr[character].aitarget].team, valuetmpargument, valuetmpdistance );
            break;

        case FIFUNARMED:
            // This function proceeds if the character has no item in hand
            returncode = ( chr[character].holdingwhich[0] == MAXCHR && chr[character].holdingwhich[1] == MAXCHR );
            break;

        case FRESTOCKTARGETAMMOIDALL:
            // This function restocks the ammo of every item the character is holding,
            // if the item matches the ID given ( parent or child type )
            iTmp = 0;  // Amount of ammo given
            sTmp = chr[chr[character].aitarget].holdingwhich[0];
            iTmp += restock_ammo( sTmp, valuetmpargument );
            sTmp = chr[chr[character].aitarget].holdingwhich[1];
            iTmp += restock_ammo( sTmp, valuetmpargument );
            sTmp = chr[chr[character].aitarget].nextinpack;

            while ( sTmp != MAXCHR )
            {
                iTmp += restock_ammo( sTmp, valuetmpargument );
                sTmp = chr[sTmp].nextinpack;
            }

            valuetmpargument = iTmp;
            returncode = ( iTmp != 0 );
            break;

        case FRESTOCKTARGETAMMOIDFIRST:
            // This function restocks the ammo of the first item the character is holding,
            // if the item matches the ID given ( parent or child type )
            iTmp = 0;  // Amount of ammo given
            sTmp = chr[chr[character].aitarget].holdingwhich[0];
            iTmp += restock_ammo( sTmp, valuetmpargument );

            if ( iTmp == 0 )
            {
                sTmp = chr[chr[character].aitarget].holdingwhich[1];
                iTmp += restock_ammo( sTmp, valuetmpargument );

                if ( iTmp == 0 )
                {
                    sTmp = chr[chr[character].aitarget].nextinpack;

                    while ( sTmp != MAXCHR && iTmp == 0 )
                    {
                        iTmp += restock_ammo( sTmp, valuetmpargument );
                        sTmp = chr[sTmp].nextinpack;
                    }
                }
            }

            valuetmpargument = iTmp;
            returncode = ( iTmp != 0 );
            break;

        case FFLASHTARGET:
            // This function flashes the character
            flash_character( chr[character].aitarget, 255 );
            break;

        case FSETREDSHIFT:
            // This function alters a character's coloration
            chr[character].redshift = valuetmpargument;
            break;

        case FSETGREENSHIFT:
            // This function alters a character's coloration
            chr[character].grnshift = valuetmpargument;
            break;

        case FSETBLUESHIFT:
            // This function alters a character's coloration
            chr[character].blushift = valuetmpargument;
            break;

        case FSETLIGHT:
            // This function alters a character's transparency
            chr[character].light = valuetmpargument;
            break;

        case FSETALPHA:
            // This function alters a character's transparency
            chr[character].alpha = valuetmpargument;
            break;

        case FIFHITFROMBEHIND:
            // This function proceeds if the character was attacked from behind
            returncode = bfalse;

            if ( chr[character].directionlast >= BEHIND - 8192 && chr[character].directionlast < BEHIND + 8192 )
                returncode = btrue;

            break;

        case FIFHITFROMFRONT:
            // This function proceeds if the character was attacked from the front
            returncode = bfalse;

            if ( chr[character].directionlast >= 49152 + 8192 || chr[character].directionlast < FRONT + 8192 )
                returncode = btrue;

            break;

        case FIFHITFROMLEFT:
            // This function proceeds if the character was attacked from the left
            returncode = bfalse;

            if ( chr[character].directionlast >= LEFT - 8192 && chr[character].directionlast < LEFT + 8192 )
                returncode = btrue;

            break;

        case FIFHITFROMRIGHT:
            // This function proceeds if the character was attacked from the right
            returncode = bfalse;

            if ( chr[character].directionlast >= RIGHT - 8192 && chr[character].directionlast < RIGHT + 8192 )
                returncode = btrue;

            break;

        case FIFTARGETISONSAMETEAM:
            // This function proceeds only if the target is on another team
            returncode = bfalse;

            if ( chr[chr[character].aitarget].team == chr[character].team )
                returncode = btrue;

            break;

        case FKILLTARGET:
            // This function kills the target
            kill_character( chr[character].aitarget, character );
            break;

        case FUNDOENCHANT:
            // This function undoes the last enchant
            returncode = ( chr[character].undoenchant != MAXENCHANT );
            remove_enchant( chr[character].undoenchant );
            break;

        case FGETWATERLEVEL:
            // This function gets the douse level for the water, returning it in tmpargument
            valuetmpargument = waterdouselevel * 10;
            break;

        case FCOSTTARGETMANA:
            // This function costs the target some mana
            returncode = cost_mana( chr[character].aitarget, valuetmpargument, character );
            break;

        case FIFTARGETHASANYID:
            // This function proceeds only if one of the target's IDSZ's matches tmpargument
            returncode = 0;
            tTmp = 0;

            while ( tTmp < IDSZ_COUNT )
            {
                returncode |= ( capidsz[chr[chr[character].aitarget].model][tTmp] == ( IDSZ ) valuetmpargument );
                tTmp++;
            }
            break;

        case FSETBUMPSIZE:
            // This function sets the character's bump size
            fTmp = chr[character].bumpsizebig;
            fTmp = fTmp / chr[character].bumpsize;  // 1.5f or 2.0f
            chr[character].bumpsize = valuetmpargument * chr[character].fat;
            chr[character].bumpsizebig = fTmp * chr[character].bumpsize;
            chr[character].bumpsizesave = valuetmpargument;
            chr[character].bumpsizebigsave = fTmp * chr[character].bumpsizesave;
            break;

        case FIFNOTDROPPED:
            // This function passes if a kursed item could not be dropped
            returncode = ( ( chr[character].alert & ALERTIFNOTDROPPED ) != 0 );
            break;

        case FIFYISLESSTHANX:
            // This function passes only if tmpy is less than tmpx
            returncode = ( valuetmpy < valuetmpx );
            break;

        case FSETFLYHEIGHT:
            // This function sets a character's fly height
            chr[character].flyheight = valuetmpargument;
            break;

        case FIFBLOCKED:
            // This function passes if the character blocked an attack
            returncode = ( ( chr[character].alert & ALERTIFBLOCKED ) != 0 );
            break;

        case FIFTARGETISDEFENDING:
            returncode = ( chr[chr[character].aitarget].action >= ACTIONPA && chr[chr[character].aitarget].action <= ACTIONPD );
            break;

        case FIFTARGETISATTACKING:
            returncode = ( chr[chr[character].aitarget].action >= ACTIONUA && chr[chr[character].aitarget].action <= ACTIONFD );
            break;

        case FIFSTATEIS0:
            returncode = ( 0 == chr[character].aistate );
            break;

        case FIFSTATEIS1:
            returncode = ( 1 == chr[character].aistate );
            break;

        case FIFSTATEIS2:
            returncode = ( 2 == chr[character].aistate );
            break;

        case FIFSTATEIS3:
            returncode = ( 3 == chr[character].aistate );
            break;

        case FIFSTATEIS4:
            returncode = ( 4 == chr[character].aistate );
            break;

        case FIFSTATEIS5:
            returncode = ( 5 == chr[character].aistate );
            break;

        case FIFSTATEIS6:
            returncode = ( 6 == chr[character].aistate );
            break;

        case FIFSTATEIS7:
            returncode = ( 7 == chr[character].aistate );
            break;

        case FIFCONTENTIS:
            returncode = ( valuetmpargument == chr[character].aicontent );
            break;

        case FSETTURNMODETOWATCHTARGET:
            // This function sets the turn mode
            chr[character].turnmode = TURNMODEWATCHTARGET;
            break;

        case FIFSTATEISNOT:
            returncode = ( valuetmpargument != chr[character].aistate );
            break;

        case FIFXISEQUALTOY:
            returncode = ( valuetmpx == valuetmpy );
            break;

        case FDEBUGMESSAGE:
            // This function spits out a debug message
            sprintf( cTmp, "aistate %d, aicontent %d, target %d", chr[character].aistate, chr[character].aicontent, chr[character].aitarget );
            debug_message( cTmp );
            sprintf( cTmp, "tmpx %d, tmpy %d", valuetmpx, valuetmpy );
            debug_message( cTmp );
            sprintf( cTmp, "tmpdistance %d, tmpturn %d", valuetmpdistance, valuetmpturn );
            debug_message( cTmp );
            sprintf( cTmp, "tmpargument %d, selfturn %d", valuetmpargument, chr[character].turnleftright );
            debug_message( cTmp );
            break;

        case FBLACKTARGET:
            // This function makes the target flash black
            flash_character( chr[character].aitarget, 0 );
            break;

        case FSENDMESSAGENEAR:
            // This function sends a message if the camera is in the nearby area.
            iTmp = ABS( chr[character].oldx - camtrackx ) + ABS( chr[character].oldy - camtracky );

            if ( iTmp < MSGDISTANCE )
                display_message( madmsgstart[chr[character].model] + valuetmpargument, character );

            break;

        case FIFHITGROUND:
            // This function passes if the character just hit the ground
            returncode = ( ( chr[character].alert & ALERTIFHITGROUND ) != 0 );
            break;

        case FIFNAMEISKNOWN:
            // This function passes if the character's name is known
            returncode = chr[character].nameknown;
            break;

        case FIFUSAGEISKNOWN:
            // This function passes if the character's usage is known
            returncode = capusageknown[chr[character].model];
            break;

        case FIFHOLDINGITEMID:
            // This function passes if the character is holding an item with the IDSZ given
            // in tmpargument, returning the latch to press to use it
            returncode = bfalse;
            // Check left hand
            sTmp = chr[character].holdingwhich[0];

            if ( sTmp != MAXCHR )
            {
                sTmp = chr[sTmp].model;

                if ( capidsz[sTmp][IDSZ_PARENT] == ( IDSZ ) valuetmpargument || capidsz[sTmp][IDSZ_TYPE] == ( Uint32 ) valuetmpargument )
                {
                    valuetmpargument = LATCHBUTTONLEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            sTmp = chr[character].holdingwhich[1];

            if ( sTmp != MAXCHR )
            {
                sTmp = chr[sTmp].model;

                if ( capidsz[sTmp][IDSZ_PARENT] == ( IDSZ ) valuetmpargument || capidsz[sTmp][IDSZ_TYPE] == ( Uint32 ) valuetmpargument )
                {
                    valuetmpargument = LATCHBUTTONRIGHT;

                    if ( returncode )  valuetmpargument = LATCHBUTTONLEFT + ( rand() & 1 );

                    returncode = btrue;
                }
            }
            break;

        case FIFHOLDINGRANGEDWEAPON:
            // This function passes if the character is holding a ranged weapon, returning
            // the latch to press to use it.  This also checks ammo/ammoknown.
            returncode = bfalse;
            valuetmpargument = 0;
            // Check left hand
            tTmp = chr[character].holdingwhich[0];

            if ( tTmp != MAXCHR )
            {
                sTmp = chr[tTmp].model;

                if ( capisranged[sTmp] && ( chr[tTmp].ammomax == 0 || ( chr[tTmp].ammo != 0 && chr[tTmp].ammoknown ) ) )
                {
                    valuetmpargument = LATCHBUTTONLEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            tTmp = chr[character].holdingwhich[1];

            if ( tTmp != MAXCHR )
            {
                sTmp = chr[tTmp].model;

                if ( capisranged[sTmp] && ( chr[tTmp].ammomax == 0 || ( chr[tTmp].ammo != 0 && chr[tTmp].ammoknown ) ) )
                {
                    if ( valuetmpargument == 0 || ( allframe&1 ) )
                    {
                        valuetmpargument = LATCHBUTTONRIGHT;
                        returncode = btrue;
                    }
                }
            }
            break;

        case FIFHOLDINGMELEEWEAPON:
            // This function passes if the character is holding a melee weapon, returning
            // the latch to press to use it
            returncode = bfalse;
            valuetmpargument = 0;
            // Check left hand
            sTmp = chr[character].holdingwhich[0];

            if ( sTmp != MAXCHR )
            {
                sTmp = chr[sTmp].model;

                if ( !capisranged[sTmp] && capweaponaction[sTmp] != ACTIONPA )
                {
                    valuetmpargument = LATCHBUTTONLEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            sTmp = chr[character].holdingwhich[1];

            if ( sTmp != MAXCHR )
            {
                sTmp = chr[sTmp].model;

                if ( !capisranged[sTmp] && capweaponaction[sTmp] != ACTIONPA )
                {
                    if ( valuetmpargument == 0 || ( allframe&1 ) )
                    {
                        valuetmpargument = LATCHBUTTONRIGHT;
                        returncode = btrue;
                    }
                }
            }
            break;

        case FIFHOLDINGSHIELD:
            // This function passes if the character is holding a shield, returning the
            // latch to press to use it
            returncode = bfalse;
            valuetmpargument = 0;
            // Check left hand
            sTmp = chr[character].holdingwhich[0];

            if ( sTmp != MAXCHR )
            {
                sTmp = chr[sTmp].model;

                if ( capweaponaction[sTmp] == ACTIONPA )
                {
                    valuetmpargument = LATCHBUTTONLEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            sTmp = chr[character].holdingwhich[1];

            if ( sTmp != MAXCHR )
            {
                sTmp = chr[sTmp].model;

                if ( capweaponaction[sTmp] == ACTIONPA )
                {
                    valuetmpargument = LATCHBUTTONRIGHT;
                    returncode = btrue;
                }
            }
            break;

        case FIFKURSED:
            // This function passes if the character is kursed
            returncode = chr[character].iskursed;
            break;

        case FIFTARGETISKURSED:
            // This function passes if the target is kursed
            returncode = chr[chr[character].aitarget].iskursed;
            break;

        case FIFTARGETISDRESSEDUP:
            // This function passes if the character's skin is dressy
            iTmp = chr[character].texture - madskinstart[chr[character].model];
            iTmp = 1 << iTmp;
            returncode = ( ( capskindressy[chr[character].model] & iTmp ) != 0 );
            break;

        case FIFOVERWATER:
            // This function passes if the character is on a water tile
            returncode = ( ( meshfx[chr[character].onwhichfan] & MESHFXWATER ) != 0 && wateriswater );
            break;

        case FIFTHROWN:
            // This function passes if the character was thrown
            returncode = ( ( chr[character].alert & ALERTIFTHROWN ) != 0 );
            break;

        case FMAKENAMEKNOWN:
            // This function makes the name of an item/character known.
            chr[character].nameknown = btrue;
//            chr[character].icon = btrue;
            break;

        case FMAKEUSAGEKNOWN:
            // This function makes the usage of an item known...  For XP gains from
            // using an unknown potion or such
            capusageknown[chr[character].model] = btrue;
            break;

        case FSTOPTARGETMOVEMENT:
            // This function makes the target stop moving temporarily
            chr[chr[character].aitarget].xvel = 0;
            chr[chr[character].aitarget].yvel = 0;

            if ( chr[chr[character].aitarget].zvel > 0 ) chr[chr[character].aitarget].zvel = gravity;

            break;

        case FSETXY:
            // This function stores tmpx and tmpy in the storage array
            chr[character].aix[valuetmpargument&STORAND] = valuetmpx;
            chr[character].aiy[valuetmpargument&STORAND] = valuetmpy;
            break;

        case FGETXY:
            // This function gets previously stored data, setting tmpx and tmpy
            valuetmpx = chr[character].aix[valuetmpargument&STORAND];
            valuetmpy = chr[character].aiy[valuetmpargument&STORAND];
            break;

        case FADDXY:
            // This function adds tmpx and tmpy to the storage array
            chr[character].aix[valuetmpargument&STORAND] += valuetmpx;
            chr[character].aiy[valuetmpargument&STORAND] += valuetmpy;
            break;

        case FMAKEAMMOKNOWN:
            // This function makes the ammo of an item/character known.
            chr[character].ammoknown = btrue;
            break;

        case FSPAWNATTACHEDPARTICLE:
            // This function spawns an attached particle
            tTmp = character;

            if ( chr[character].attachedto != MAXCHR )  tTmp = chr[character].attachedto;

            tTmp = spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos, chr[character].turnleftright, chr[character].model, valuetmpargument, character, valuetmpdistance, chr[character].team, tTmp, 0, MAXCHR );
            break;

        case FSPAWNEXACTPARTICLE:
            // This function spawns an exactly placed particle
            tTmp = character;

            if ( chr[character].attachedto != MAXCHR )  tTmp = chr[character].attachedto;

            spawn_one_particle( valuetmpx, valuetmpy, valuetmpdistance, chr[character].turnleftright, chr[character].model, valuetmpargument, MAXCHR, 0, chr[character].team, tTmp, 0, MAXCHR );
            break;

        case FACCELERATETARGET:
            // This function changes the target's speeds
            chr[chr[character].aitarget].xvel += valuetmpx;
            chr[chr[character].aitarget].yvel += valuetmpy;
            break;

        case FIFDISTANCEISMORETHANTURN:
            // This function proceeds tmpdistance is greater than tmpturn
            returncode = ( valuetmpdistance > ( int ) valuetmpturn );
            break;

        case FIFCRUSHED:
            // This function proceeds only if the character was crushed
            returncode = ( ( chr[character].alert & ALERTIFCRUSHED ) != 0 );
            break;

        case FMAKECRUSHVALID:
            // This function makes doors able to close on this object
            chr[character].canbecrushed = btrue;
            break;

        case FSETTARGETTOLOWESTTARGET:

            // This sets the target to whatever the target is being held by,
            // The lowest in the set.  This function never fails
            while ( chr[chr[character].aitarget].attachedto != MAXCHR )
            {
                chr[character].aitarget = chr[chr[character].aitarget].attachedto;
            }
            break;

        case FIFNOTPUTAWAY:
            // This function proceeds only if the character couln't be put in the pack
            returncode = ( ( chr[character].alert & ALERTIFNOTPUTAWAY ) != 0 );
            break;

        case FIFTAKENOUT:
            // This function proceeds only if the character was taken out of the pack
            returncode = ( ( chr[character].alert & ALERTIFTAKENOUT ) != 0 );
            break;

        case FIFAMMOOUT:
            // This function proceeds only if the character has no ammo
            returncode = ( chr[character].ammo == 0 );
            break;

        case FPLAYSOUNDLOOPED:
            // This function plays a looped sound
            if ( moduleactive )
            {
                // You could use this, but right now there's no way to stop the sound later, so it's better not to start it
                // play_sound_pvf_looped(capwaveindex[chr[character].model][valuetmpargument], PANMID, volume, valuetmpdistance);
            }
            break;

        case FSTOPSOUND:
            // TODO: implement this (the scripter doesn't know which channel to stop)
            // This function stops playing a sound
            // stop_sound([valuetmpargument]);
            break;

        case FHEALSELF:

            // This function heals the character, without setting the alert or modifying
            // the amount
            if ( chr[character].alive )
            {
                iTmp = chr[character].life + valuetmpargument;

                if ( iTmp > chr[character].lifemax ) iTmp = chr[character].lifemax;

                if ( iTmp < 1 ) iTmp = 1;

                chr[character].life = iTmp;
            }
            break;

        case FEQUIP:
            // This function flags the character as being equipped
            chr[character].isequipped = btrue;
            break;

        case FIFTARGETHASITEMIDEQUIPPED:
            // This function proceeds if the target has a matching item equipped
            returncode = bfalse;
            sTmp = chr[chr[character].aitarget].nextinpack;

            while ( sTmp != MAXCHR )
            {
                if ( sTmp != character && chr[sTmp].isequipped && ( capidsz[chr[sTmp].model][IDSZ_PARENT] == ( Uint32 ) valuetmpargument || capidsz[chr[sTmp].model][IDSZ_TYPE] == ( Uint32 ) valuetmpargument ) )
                {
                    returncode = btrue;
                    sTmp = MAXCHR;
                }
                else
                {
                    sTmp = chr[sTmp].nextinpack;
                }
            }
            break;

        case FSETOWNERTOTARGET:
            // This function sets the owner
            chr[character].aiowner = chr[character].aitarget;
            break;

        case FSETTARGETTOOWNER:
            // This function sets the target to the owner
            chr[character].aitarget = chr[character].aiowner;
            break;

        case FSETFRAME:
            // This function sets the character's current frame
            sTmp = valuetmpargument & 3;
            iTmp = valuetmpargument >> 2;
            set_frame( character, iTmp, sTmp );
            break;

        case FBREAKPASSAGE:
            // This function makes the tiles fall away ( turns into damage terrain )
            returncode = break_passage( valuetmpargument, valuetmpturn, valuetmpdistance, valuetmpx, valuetmpy );
            break;

        case FSETRELOADTIME:

            // This function makes weapons fire slower
            if ( valuetmpargument > 0 ) chr[character].reloadtime = valuetmpargument;
            else chr[character].reloadtime = 0;

            break;

        case FSETTARGETTOWIDEBLAHID:
            // This function sets the target based on the settings of
            // tmpargument and tmpdistance
            {
                TARGET_TYPE blahteam = ALL;

                if ( ( valuetmpdistance >> 2 ) & 1 )  blahteam = FRIEND;

                if ( (( valuetmpdistance >> 1 ) & 1) && blahteam == FRIEND ) blahteam = ALL;
                else if ((( valuetmpdistance >> 1 ) & 1)) blahteam = ENEMY;
                else returncode = bfalse;

                if (returncode)
                {
                    returncode = bfalse;

                    if (get_target(character, WIDE, blahteam, ( ( valuetmpdistance >> 3 ) & 1 ),
                                   ( ( valuetmpdistance ) & 1 ), valuetmpargument, (( valuetmpdistance >> 4 ) & 1) ) != MAXCHR) returncode = btrue;
                }
            }
            break;

        case FPOOFTARGET:
            // This function makes the target go away
            returncode = bfalse;

            if ( !chr[chr[character].aitarget].isplayer )
            {
                returncode = btrue;

                if ( chr[character].aitarget == character )
                {
                    // Poof self later
                    valuegopoof = btrue;
                }
                else
                {
                    // Poof others now
                    if ( chr[chr[character].aitarget].attachedto != MAXCHR )
                        detach_character_from_mount( chr[character].aitarget, btrue, bfalse );

                    if ( chr[chr[character].aitarget].holdingwhich[0] != MAXCHR )
                        detach_character_from_mount( chr[chr[character].aitarget].holdingwhich[0], btrue, bfalse );

                    if ( chr[chr[character].aitarget].holdingwhich[1] != MAXCHR )
                        detach_character_from_mount( chr[chr[character].aitarget].holdingwhich[1], btrue, bfalse );

                    free_inventory( chr[character].aitarget );
                    free_one_character( chr[character].aitarget );
                    chr[character].aitarget = character;
                }
            }
            break;

        case FCHILDDOACTIONOVERRIDE:
            // This function starts a new action, if it is valid for the model
            // It will fail if the action is invalid
            returncode = bfalse;

            if ( valuetmpargument < MAXACTION )
            {
                if ( madactionvalid[chr[chr[character].aichild].model][valuetmpargument] )
                {
                    chr[chr[character].aichild].action = valuetmpargument;
                    chr[chr[character].aichild].lip = 0;
                    chr[chr[character].aichild].frame = madactionstart[chr[chr[character].aichild].model][valuetmpargument];
                    chr[chr[character].aichild].lastframe = chr[chr[character].aichild].frame;
                    chr[chr[character].aichild].actionready = bfalse;
                    returncode = btrue;
                }
            }
            break;

        case FSPAWNPOOF:
            // This function makes a lovely little poof at the character's location
            spawn_poof( character, chr[character].model );
            break;

        case FSETSPEEDPERCENT:
            reset_character_accel( character );
            chr[character].maxaccel = chr[character].maxaccel * valuetmpargument / 100.0f;
            break;

        case FSETCHILDSTATE:
            // This function sets the child's state
            chr[chr[character].aichild].aistate = valuetmpargument;
            break;

        case FSPAWNATTACHEDSIZEDPARTICLE:
            // This function spawns an attached particle, then sets its size
            tTmp = character;

            if ( chr[character].attachedto != MAXCHR )  tTmp = chr[character].attachedto;

            tTmp = spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos, chr[character].turnleftright, chr[character].model, valuetmpargument, character, valuetmpdistance, chr[character].team, tTmp, 0, MAXCHR );

            if ( tTmp < maxparticles )
            {
                prtsize[tTmp] = valuetmpturn;
            }
            break;

        case FCHANGEARMOR:
            // This function sets the character's armor type and returns the old type
            // as tmpargument and the new type as tmpx
            valuetmpx = valuetmpargument;
            iTmp = chr[character].texture - madskinstart[chr[character].model];
            valuetmpx = change_armor( character, valuetmpargument );
            valuetmpargument = iTmp;  // The character's old armor
            break;

        case FSHOWTIMER:
            // This function turns the timer on, using the value for tmpargument
            timeron = btrue;
            timervalue = valuetmpargument;
            break;

        case FIFFACINGTARGET:
            // This function proceeds only if the character is facing the target
            sTmp = ATAN2( chr[chr[character].aitarget].ypos - chr[character].ypos, chr[chr[character].aitarget].xpos - chr[character].xpos ) * 65535 / ( TWO_PI );
            sTmp += 32768 - chr[character].turnleftright;
            returncode = ( sTmp > 55535 || sTmp < 10000 );
            break;

        case FPLAYSOUNDVOLUME:

            // This function sets the volume of a sound and plays it
            if ( moduleactive && valuetmpdistance >= 0 )
            {
                volume = valuetmpdistance;
                iTmp = -1;
                if ( valuetmpargument >= 0 && valuetmpargument < MAXWAVE )
                {
                    iTmp = play_mix( chr[character].oldx, chr[character].oldy, capwaveindex[chr[character].model] + valuetmpargument );
                }

                if ( -1 != iTmp )
                {
                    if ( MIX_SND == capwaveindex[chr[character].model][valuetmpargument].type )
                    {
                        Mix_Volume( iTmp, valuetmpdistance );
                    }
                    else if ( MIX_MUS == capwaveindex[chr[character].model][valuetmpargument].type )
                    {
                        Mix_VolumeMusic( valuetmpdistance );
                    }
                }
            }
            break;

        case FSPAWNATTACHEDFACEDPARTICLE:
            // This function spawns an attached particle with facing
            tTmp = character;

            if ( chr[character].attachedto != MAXCHR )  tTmp = chr[character].attachedto;

            tTmp = spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos, valuetmpturn, chr[character].model, valuetmpargument, character, valuetmpdistance, chr[character].team, tTmp, 0, MAXCHR );
            break;

        case FIFSTATEISODD:
            returncode = ( chr[character].aistate & 1 );
            break;

        case FSETTARGETTODISTANTENEMY:
            // This function finds an enemy, within a certain distance to the character, and
            // proceeds only if there is one
            returncode = bfalse;

            if (get_target(character, valuetmpdistance, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse) != MAXCHR) returncode = btrue;

            break;

        case FTELEPORT:
            // This function teleports the character to the X, Y location, failing if the
            // location is off the map or blocked
            returncode = bfalse;

            if ( valuetmpx > EDGE && valuetmpy > EDGE && valuetmpx < meshedgex - EDGE && valuetmpy < meshedgey - EDGE )
            {
                // Yeah!  It worked!
                detach_character_from_mount( character, btrue, bfalse );
                chr[character].oldx = chr[character].xpos;
                chr[character].oldy = chr[character].ypos;
                chr[character].xpos = valuetmpx;
                chr[character].ypos = valuetmpy;

                if ( __chrhitawall( character ) )
                {
                    // No it didn't...
                    chr[character].xpos = chr[character].oldx;
                    chr[character].ypos = chr[character].oldy;
                    returncode = bfalse;
                }
                else
                {
                    chr[character].oldx = chr[character].xpos;
                    chr[character].oldy = chr[character].ypos;
                    returncode = btrue;
                }
            }
            break;

        case FGIVESTRENGTHTOTARGET:

            // Permanently boost the target's strength
            if ( chr[chr[character].aitarget].alive )
            {
                iTmp = valuetmpargument;
                getadd( 0, chr[chr[character].aitarget].strength, PERFECTSTAT, &iTmp );
                chr[chr[character].aitarget].strength += iTmp;
            }
            break;

        case FGIVEWISDOMTOTARGET:

            // Permanently boost the target's wisdom
            if ( chr[chr[character].aitarget].alive )
            {
                iTmp = valuetmpargument;
                getadd( 0, chr[chr[character].aitarget].wisdom, PERFECTSTAT, &iTmp );
                chr[chr[character].aitarget].wisdom += iTmp;
            }
            break;

        case FGIVEINTELLIGENCETOTARGET:

            // Permanently boost the target's intelligence
            if ( chr[chr[character].aitarget].alive )
            {
                iTmp = valuetmpargument;
                getadd( 0, chr[chr[character].aitarget].intelligence, PERFECTSTAT, &iTmp );
                chr[chr[character].aitarget].intelligence += iTmp;
            }
            break;

        case FGIVEDEXTERITYTOTARGET:

            // Permanently boost the target's dexterity
            if ( chr[chr[character].aitarget].alive )
            {
                iTmp = valuetmpargument;
                getadd( 0, chr[chr[character].aitarget].dexterity, PERFECTSTAT, &iTmp );
                chr[chr[character].aitarget].dexterity += iTmp;
            }
            break;

        case FGIVELIFETOTARGET:

            // Permanently boost the target's life
            if ( chr[chr[character].aitarget].alive )
            {
                iTmp = valuetmpargument;
                getadd( LOWSTAT, chr[chr[character].aitarget].lifemax, PERFECTBIG, &iTmp );
                chr[chr[character].aitarget].lifemax += iTmp;

                if ( iTmp < 0 )
                {
                    getadd( 1, chr[chr[character].aitarget].life, PERFECTBIG, &iTmp );
                }

                chr[chr[character].aitarget].life += iTmp;
            }
            break;

        case FGIVEMANATOTARGET:

            // Permanently boost the target's mana
            if ( chr[chr[character].aitarget].alive )
            {
                iTmp = valuetmpargument;
                getadd( 0, chr[chr[character].aitarget].manamax, PERFECTBIG, &iTmp );
                chr[chr[character].aitarget].manamax += iTmp;

                if ( iTmp < 0 )
                {
                    getadd( 0, chr[chr[character].aitarget].mana, PERFECTBIG, &iTmp );
                }

                chr[chr[character].aitarget].mana += iTmp;
            }
            break;

        case FSHOWMAP:

            // Show the map...  Fails if map already visible
            if ( mapon )  returncode = bfalse;

            mapon = mapvalid;
            break;

        case FSHOWYOUAREHERE:
            // Show the camera target location
            youarehereon = mapvalid;
            break;

        case FSHOWBLIPXY:

            // Add a blip
            if ( numblip < MAXBLIP )
            {
                if ( valuetmpx > 0 && valuetmpx < meshedgex && valuetmpy > 0 && valuetmpy < meshedgey )
                {
                    if ( valuetmpargument < NUMBAR && valuetmpargument >= 0 )
                    {
                        blipx[numblip] = valuetmpx * MAPSIZE / meshedgex;
                        blipy[numblip] = valuetmpy * MAPSIZE / meshedgey;
                        blipc[numblip] = valuetmpargument;
                        numblip++;
                    }
                }
            }
            break;

        case FHEALTARGET:

            // Give some life to the target
            if ( chr[chr[character].aitarget].alive )
            {
                iTmp = valuetmpargument;
                getadd( 1, chr[chr[character].aitarget].life, chr[chr[character].aitarget].lifemax, &iTmp );
                chr[chr[character].aitarget].life += iTmp;
                // Check all enchants to see if they are removed
                iTmp = chr[chr[character].aitarget].firstenchant;

                while ( iTmp != MAXENCHANT )
                {
                    test = Make_IDSZ( "HEAL" );  // [HEAL]
                    sTmp = encnextenchant[iTmp];

                    if ( test == everemovedbyidsz[enceve[iTmp]] )
                    {
                        remove_enchant( iTmp );
                    }

                    iTmp = sTmp;
                }
            }
            break;

        case FPUMPTARGET:

            // Give some mana to the target
            if ( chr[chr[character].aitarget].alive )
            {
                iTmp = valuetmpargument;
                getadd( 0, chr[chr[character].aitarget].mana, chr[chr[character].aitarget].manamax, &iTmp );
                chr[chr[character].aitarget].mana += iTmp;
            }
            break;

        case FCOSTAMMO:

            // Take away one ammo
            if ( chr[character].ammo > 0 )
            {
                chr[character].ammo--;
            }
            break;

        case FMAKESIMILARNAMESKNOWN:
            // Make names of matching objects known
            iTmp = 0;

            while ( iTmp < MAXCHR )
            {
                sTmp = btrue;
                tTmp = 0;

                while ( tTmp < IDSZ_COUNT )
                {
                    if ( capidsz[chr[character].model][tTmp] != capidsz[chr[iTmp].model][tTmp] )
                    {
                        sTmp = bfalse;
                    }

                    tTmp++;
                }

                if ( sTmp )
                {
                    chr[iTmp].nameknown = btrue;
                }

                iTmp++;
            }
            break;

        case FSPAWNATTACHEDHOLDERPARTICLE:
            // This function spawns an attached particle, attached to the holder
            tTmp = character;

            if ( chr[character].attachedto != MAXCHR )  tTmp = chr[character].attachedto;

            tTmp = spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos, chr[character].turnleftright, chr[character].model, valuetmpargument, tTmp, valuetmpdistance, chr[character].team, tTmp, 0, MAXCHR );
            break;

        case FSETTARGETRELOADTIME:

            // This function sets the target's reload time
            if ( valuetmpargument > 0 )
                chr[chr[character].aitarget].reloadtime = valuetmpargument;
            else chr[chr[character].aitarget].reloadtime = 0;

            break;

        case FSETFOGLEVEL:
            // This function raises and lowers the module's fog
            fTmp = ( valuetmpargument / 10.0f ) - fogtop;
            fogtop += fTmp;
            fogdistance += fTmp;
            fogon = fogallowed;

            if ( fogdistance < 1.0f )  fogon = bfalse;

            break;

        case FGETFOGLEVEL:
            // This function gets the fog level
            valuetmpargument = fogtop * 10;
            break;

        case FSETFOGTAD:
            // This function changes the fog color
            fogred = valuetmpturn;
            foggrn = valuetmpargument;
            fogblu = valuetmpdistance;
            break;

        case FSETFOGBOTTOMLEVEL:
            // This function sets the module's bottom fog level...
            fTmp = ( valuetmpargument / 10.0f ) - fogbottom;
            fogbottom += fTmp;
            fogdistance -= fTmp;
            fogon = fogallowed;

            if ( fogdistance < 1.0f )  fogon = bfalse;

            break;

        case FGETFOGBOTTOMLEVEL:
            // This function gets the fog level
            valuetmpargument = fogbottom * 10;
            break;

        case FCORRECTACTIONFORHAND:

            // This function turns ZA into ZA, ZB, ZC, or ZD...
            // tmpargument must be set to one of the A actions beforehand...
            if ( chr[character].attachedto != MAXCHR )
            {
                if ( chr[character].inwhichhand == GRIPLEFT )
                {
                    // A or B
                    valuetmpargument = valuetmpargument + ( rand() & 1 );
                }
                else
                {
                    // C or D
                    valuetmpargument = valuetmpargument + 2 + ( rand() & 1 );
                }
            }
            break;

        case FIFTARGETISMOUNTED:
            // This function proceeds if the target is riding a mount
            returncode = bfalse;

            if ( chr[chr[character].aitarget].attachedto != MAXCHR )
            {
                returncode = chr[chr[chr[character].aitarget].attachedto].ismount;
            }
            break;

        case FSPARKLEICON:

            // This function makes a blippie thing go around the icon
            if ( valuetmpargument < NUMBAR && valuetmpargument > -1 )
            {
                chr[character].sparkle = valuetmpargument;
            }
            break;

        case FUNSPARKLEICON:
            // This function stops the blippie thing
            chr[character].sparkle = NOSPARKLE;
            break;

        case FGETTILEXY:

            // This function gets the tile at x,y
            if ( valuetmpx >= 0 && valuetmpx < meshedgex )
            {
                if ( valuetmpy >= 0 && valuetmpy < meshedgey )
                {
                    iTmp = meshfanstart[valuetmpy>>7] + ( valuetmpx >> 7 );
                    valuetmpargument = meshtile[iTmp] & 255;
                }
            }
            break;

        case FSETTILEXY:

            // This function changes the tile at x,y
            if ( valuetmpx >= 0 && valuetmpx < meshedgex )
            {
                if ( valuetmpy >= 0 && valuetmpy < meshedgey )
                {
                    iTmp = meshfanstart[valuetmpy>>7] + ( valuetmpx >> 7 );
                    meshtile[iTmp] = ( valuetmpargument & 255 );
                }
            }
            break;

        case FSETSHADOWSIZE:
            // This function changes a character's shadow size
            chr[character].shadowsize = valuetmpargument * chr[character].fat;
            chr[character].shadowsizesave = valuetmpargument;
            break;

        case FORDERTARGET:
            // This function orders one specific character...  The target
            // Be careful in using this, always checking IDSZ first
            chr[chr[character].aitarget].order = valuetmpargument;
            chr[chr[character].aitarget].counter = 0;
            chr[chr[character].aitarget].alert |= ALERTIFORDERED;
            break;

        case FSETTARGETTOWHOEVERISINPASSAGE:
            // This function lets passage rectangles be used as event triggers
            sTmp = who_is_blocking_passage( valuetmpargument );
            returncode = bfalse;

            if ( sTmp != MAXCHR )
            {
                chr[character].aitarget = sTmp;
                returncode = btrue;
            }
            break;

        case FIFCHARACTERWASABOOK:
            // This function proceeds if the base model is the same as the current
            // model or if the base model is SPELLBOOK
            returncode = ( chr[character].basemodel == SPELLBOOK ||
                           chr[character].basemodel == chr[character].model );
            break;

        case FSETENCHANTBOOSTVALUES:
            // This function sets the boost values for the last enchantment
            iTmp = chr[character].undoenchant;

            if ( iTmp != MAXENCHANT )
            {
                encownermana[iTmp] = valuetmpargument;
                encownerlife[iTmp] = valuetmpdistance;
                enctargetmana[iTmp] = valuetmpx;
                enctargetlife[iTmp] = valuetmpy;
            }
            break;

        case FSPAWNCHARACTERXYZ:
            // This function spawns a character, failing if x,y,z is invalid
            sTmp = spawn_one_character( valuetmpx, valuetmpy, valuetmpdistance, chr[character].model, chr[character].team, 0, valuetmpturn, NULL, MAXCHR );
            returncode = bfalse;

            if ( sTmp < MAXCHR )
            {
                if ( __chrhitawall( sTmp ) )
                {
                    free_one_character( sTmp );
                }
                else
                {
                    chr[sTmp].iskursed = bfalse;
                    chr[character].aichild = sTmp;
                    chr[sTmp].passage = chr[character].passage;
                    chr[sTmp].aiowner = chr[character].aiowner;
                    returncode = btrue;
                }
            }
            break;

        case FSPAWNEXACTCHARACTERXYZ:
            // This function spawns a character ( specific model slot ),
            // failing if x,y,z is invalid
            sTmp = spawn_one_character( valuetmpx, valuetmpy, valuetmpdistance, valuetmpargument, chr[character].team, 0, valuetmpturn, NULL, MAXCHR );
            returncode = bfalse;

            if ( sTmp < MAXCHR )
            {
                if ( __chrhitawall( sTmp ) )
                {
                    free_one_character( sTmp );
                }
                else
                {
                    chr[sTmp].iskursed = bfalse;
                    chr[character].aichild = sTmp;
                    chr[sTmp].passage = chr[character].passage;
                    chr[sTmp].aiowner = chr[character].aiowner;
                    returncode = btrue;
                }
            }
            break;

        case FCHANGETARGETCLASS:
            // This function changes a character's model ( specific model slot )
            change_character( chr[character].aitarget, valuetmpargument, 0, LEAVEALL );
            break;

        case FPLAYFULLSOUND:

            // This function plays a sound loud for everyone...  Victory music
            if ( moduleactive && valuetmpargument >= 0 && valuetmpargument < MAXWAVE )
            {
                play_mix( camtrackx, camtracky, capwaveindex[chr[character].model] + valuetmpargument );
            }
            break;

        case FSPAWNEXACTCHASEPARTICLE:
            // This function spawns an exactly placed particle that chases the target
            tTmp = character;

            if ( chr[character].attachedto != MAXCHR )  tTmp = chr[character].attachedto;

            tTmp = spawn_one_particle( valuetmpx, valuetmpy, valuetmpdistance, chr[character].turnleftright, chr[character].model, valuetmpargument, MAXCHR, 0, chr[character].team, tTmp, 0, MAXCHR );

            if ( tTmp < maxparticles )
            {
                prttarget[tTmp] = chr[character].aitarget;
            }
            break;

        case FCREATEORDER:
            // This function packs up an order, using tmpx, tmpy, tmpargument and the
            // target ( if valid ) to create a new tmpargument
            sTmp = chr[character].aitarget << 24;
            sTmp |= ( ( valuetmpx >> 6 ) & 1023 ) << 14;
            sTmp |= ( ( valuetmpy >> 6 ) & 1023 ) << 4;
            sTmp |= ( valuetmpargument & 15 );
            valuetmpargument = sTmp;
            break;

        case FORDERSPECIALID:
            // This function issues an order to all with the given special IDSZ
            issue_special_order( valuetmpargument, valuetmpdistance );
            break;

        case FUNKURSETARGETINVENTORY:
            // This function unkurses every item a character is holding
            sTmp = chr[chr[character].aitarget].holdingwhich[0];
            chr[sTmp].iskursed = bfalse;
            sTmp = chr[chr[character].aitarget].holdingwhich[1];
            chr[sTmp].iskursed = bfalse;
            sTmp = chr[chr[character].aitarget].nextinpack;

            while ( sTmp != MAXCHR )
            {
                chr[sTmp].iskursed = bfalse;
                sTmp = chr[sTmp].nextinpack;
            }
            break;

        case FIFTARGETISSNEAKING:
            // This function proceeds if the target is doing ACTIONDA or ACTIONWA
            returncode = ( chr[chr[character].aitarget].action == ACTIONDA || chr[chr[character].aitarget].action == ACTIONWA );
            break;

        case FDROPITEMS:
            // This function drops all of the character's items
            drop_all_items( character );
            break;

        case FRESPAWNTARGET:
            // This function respawns the target at its current location
            sTmp = chr[character].aitarget;
            chr[sTmp].oldx = chr[sTmp].xpos;
            chr[sTmp].oldy = chr[sTmp].ypos;
            chr[sTmp].oldz = chr[sTmp].zpos;
            respawn_character( sTmp );
            chr[sTmp].xpos = chr[sTmp].oldx;
            chr[sTmp].ypos = chr[sTmp].oldy;
            chr[sTmp].zpos = chr[sTmp].oldz;
            break;

        case FTARGETDOACTIONSETFRAME:
            // This function starts a new action, if it is valid for the model and
            // sets the starting frame.  It will fail if the action is invalid
            returncode = bfalse;

            if ( valuetmpargument < MAXACTION )
            {
                if ( madactionvalid[chr[chr[character].aitarget].model][valuetmpargument] )
                {
                    chr[chr[character].aitarget].action = valuetmpargument;
                    chr[chr[character].aitarget].lip = 0;
                    chr[chr[character].aitarget].frame = madactionstart[chr[chr[character].aitarget].model][valuetmpargument];
                    chr[chr[character].aitarget].lastframe = chr[chr[character].aitarget].frame;
                    chr[chr[character].aitarget].actionready = bfalse;
                    returncode = btrue;
                }
            }
            break;

        case FIFTARGETCANSEEINVISIBLE:
            // This function proceeds if the target can see invisible
            returncode = chr[chr[character].aitarget].canseeinvisible;
            break;

        case FSETTARGETTONEARESTBLAHID:
            // This function finds the nearest target that meets the
            // requirements
            {
                TARGET_TYPE blahteam = NONE;
                returncode = bfalse;

                if ( ( valuetmpdistance >> 2 ) & 1 )  blahteam = FRIEND;

                if ( (( valuetmpdistance >> 1 ) & 1) && blahteam == FRIEND ) blahteam = ALL;
                else if ((( valuetmpdistance >> 1 ) & 1)) blahteam = ENEMY;

                if (blahteam != NONE)
                {
                    if (get_target(character, NEAREST, blahteam, ( ( valuetmpdistance >> 3 ) & 1 ),
                                   ( ( valuetmpdistance ) & 1 ), valuetmpargument, (( valuetmpdistance >> 4 ) & 1) ) != MAXCHR) returncode = btrue;
                }
            }
            break;

        case FSETTARGETTONEARESTENEMY:
            // This function finds the nearest target that meets the
            // requirements
            returncode = bfalse;

            if (get_target(character, 0, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse ) != MAXCHR) returncode = btrue;

            break;

        case FSETTARGETTONEARESTFRIEND:
            // This function finds the nearest target that meets the
            // requirements
            returncode = bfalse;

            if (get_target(character, 0, FRIEND, bfalse, bfalse, IDSZ_NONE, bfalse ) != MAXCHR) returncode = btrue;

            break;

        case FSETTARGETTONEARESTLIFEFORM:
            // This function finds the nearest target that meets the
            // requirements
            returncode = bfalse;

            if (get_target(character, 0, ALL, bfalse, bfalse, IDSZ_NONE, bfalse ) != MAXCHR) returncode = btrue;

            break;

        case FFLASHPASSAGE:
            // This function makes the passage light or dark...  For debug...
            flash_passage( valuetmpargument, valuetmpdistance );
            break;

        case FFINDTILEINPASSAGE:
            // This function finds the next tile in the passage, tmpx and tmpy are
            // required and set on return
            returncode = find_tile_in_passage( valuetmpargument, valuetmpdistance );
            break;

        case FIFHELDINLEFTHAND:
            // This function proceeds if the character is in the left hand of another
            // character
            returncode = bfalse;
            sTmp = chr[character].attachedto;

            if ( sTmp != MAXCHR )
            {
                returncode = ( chr[sTmp].holdingwhich[0] == character );
            }
            break;

        case FNOTANITEM:
            // This function makes the character a non-item character
            chr[character].isitem = bfalse;
            break;

        case FSETCHILDAMMO:
            // This function sets the child's ammo
            chr[chr[character].aichild].ammo = valuetmpargument;
            break;

        case FIFHITVULNERABLE:
            // This function proceeds if the character was hit by a weapon with the
            // correct vulnerability IDSZ...  [SILV] for Werewolves...
            returncode = ( ( chr[character].alert & ALERTIFHITVULNERABLE ) != 0 );
            break;

        case FIFTARGETISFLYING:
            // This function proceeds if the character target is flying
            returncode = ( chr[chr[character].aitarget].flyheight > 0 );
            break;

        case FIDENTIFYTARGET:
            // This function reveals the target's name, ammo, and usage
            // Proceeds if the target was unknown
            returncode = bfalse;
            sTmp = chr[character].aitarget;

            if ( chr[sTmp].ammomax != 0 )  chr[sTmp].ammoknown = btrue;

            if ( chr[sTmp].name[0] != 'B' ||
                    chr[sTmp].name[1] != 'l' ||
                    chr[sTmp].name[2] != 'a' ||
                    chr[sTmp].name[3] != 'h' ||
                    chr[sTmp].name[4] != 0 )
            {
                returncode = !chr[sTmp].nameknown;
                chr[sTmp].nameknown = btrue;
            }

            capusageknown[chr[sTmp].model] = btrue;
            break;

        case FBEATMODULE:
            // This function displays the Module Ended message
            beatmodule = btrue;
            break;

        case FENDMODULE:
            // This function presses the Escape key
            if ( NULL != keyb.state_ptr )
            {
                keyb.state_ptr[SDLK_ESCAPE] = 1;
            }
            break;

        case FDISABLEEXPORT:
            // This function turns export off
            exportvalid = bfalse;
            break;

        case FENABLEEXPORT:
            // This function turns export on
            exportvalid = btrue;
            break;

        case FGETTARGETSTATE:
            // This function sets tmpargument to the state of the target
            valuetmpargument = chr[chr[character].aitarget].aistate;
            break;

        case FIFEQUIPPED:
            // This proceeds if the character is equipped
            returncode = bfalse;

            if ( chr[character].isequipped ) returncode = btrue;

            break;

        case FDROPTARGETMONEY:
            // This function drops some of the target's money
            drop_money( chr[character].aitarget, valuetmpargument );
            break;

        case FGETTARGETCONTENT:
            // This sets tmpargument to the current target's content value
            valuetmpargument = chr[chr[character].aitarget].aicontent;
            break;

        case FDROPTARGETKEYS:
            // This function makes the target drops keys in inventory (Not inhand)
            drop_keys( chr[character].aitarget );
            break;

        case FJOINTEAM:
            // This makes the character itself join a specified team (A = 0, B = 1, 23 = Z, etc.)
            switch_team( character, valuetmpargument );
            break;

        case FTARGETJOINTEAM:
            // This makes the target join a team specified in tmpargument (A = 0, 23 = Z, etc.)
            switch_team( chr[character].aitarget, valuetmpargument );
            break;

        case FCLEARMUSICPASSAGE:
            // This clears the music for a specified passage
            passagemusic[valuetmpargument] = -1;
            break;

        case FCLEARENDMESSAGE:
            // This function empties the end-module text buffer
            endtext[0] = 0;
            endtextwrite = 0;
            break;

        case FADDENDMESSAGE:
            // This function appends a message to the end-module text buffer
            append_end_text( madmsgstart[chr[character].model] + valuetmpargument, character );
            break;

        case FPLAYMUSIC:

            // This function begins playing a new track of music
            if ( musicvalid && ( songplaying != valuetmpargument ) )
            {
                play_music( valuetmpargument, valuetmpdistance, -1 );
            }
            break;

        case FSETMUSICPASSAGE:
            // This function makes the given passage play music if a player enters it
            // tmpargument is the passage to set and tmpdistance is the music track to play...
            passagemusic[valuetmpargument] = valuetmpdistance;
            break;

        case FMAKECRUSHINVALID:
            // This function makes doors unable to close on this object
            chr[character].canbecrushed = bfalse;
            break;

        case FSTOPMUSIC:
            // This function stops the interactive music
            stop_music();
            break;

        case FFLASHVARIABLE:
            // This function makes the character flash according to tmpargument
            flash_character( character, valuetmpargument );
            break;

        case FACCELERATEUP:
            // This function changes the character's up down velocity
            chr[character].zvel += valuetmpargument / 100.0f;
            break;

        case FFLASHVARIABLEHEIGHT:
            // This function makes the character flash, feet one color, head another...
            flash_character_height( character, valuetmpturn, valuetmpx,
                                    valuetmpdistance, valuetmpy );
            break;

        case FSETDAMAGETIME:
            // This function makes the character invincible for a little while
            chr[character].damagetime = valuetmpargument;
            break;

        case FIFSTATEIS8:
            returncode = ( 8 == chr[character].aistate );
            break;

        case FIFSTATEIS9:
            returncode = ( 9 == chr[character].aistate );
            break;

        case FIFSTATEIS10:
            returncode = ( 10 == chr[character].aistate );
            break;

        case FIFSTATEIS11:
            returncode = ( 11 == chr[character].aistate );
            break;

        case FIFSTATEIS12:
            returncode = ( 12 == chr[character].aistate );
            break;

        case FIFSTATEIS13:
            returncode = ( 13 == chr[character].aistate );
            break;

        case FIFSTATEIS14:
            returncode = ( 14 == chr[character].aistate );
            break;

        case FIFSTATEIS15:
            returncode = ( 15 == chr[character].aistate );
            break;

        case FIFTARGETISAMOUNT:
            returncode = chr[chr[character].aitarget].ismount;
            break;

        case FIFTARGETISAPLATFORM:
            returncode = chr[chr[character].aitarget].platform;
            break;

        case FADDSTAT:

            if ( !chr[character].staton ) add_stat( character );

            break;

        case FDISENCHANTTARGET:
            returncode = ( chr[chr[character].aitarget].firstenchant != MAXENCHANT );
            disenchant_character( chr[character].aitarget );
            break;

        case FDISENCHANTALL:
            iTmp = 0;

            while ( iTmp < MAXENCHANT )
            {
                remove_enchant( iTmp );
                iTmp++;
            }
            break;

        case FSETVOLUMENEARESTTEAMMATE:
            /*PORT
                        if(moduleactive && valuetmpdistance >= 0)
                        {
                            // Find the closest teammate
                            iTmp = 10000;
                            sTmp = 0;
                            while(sTmp < MAXCHR)
                            {
                                if(chr[sTmp].on && chr[sTmp].alive && chr[sTmp].team == chr[character].team)
                                {
                                    distance = ABS(camtrackx-chr[sTmp].oldx)+ABS(camtracky-chr[sTmp].oldy);
                                    if(distance < iTmp)  iTmp = distance;
                                }
                                sTmp++;
                            }
                            distance=iTmp+valuetmpdistance;
                            volume = -distance;
                            volume = volume<<VOLSHIFT;
                            if(volume < VOLMIN) volume = VOLMIN;
                            iTmp = capwaveindex[chr[character].model][valuetmpargument];
                            if(iTmp < numsound && iTmp >= 0 && soundon)
                            {
                                lpDSBuffer[iTmp]->SetVolume(volume);
                            }
                        }
            */
            break;

        case FADDSHOPPASSAGE:
            // This function defines a shop area
            add_shop_passage( character, valuetmpargument );
            break;

        case FTARGETPAYFORARMOR:
            // This function costs the target some money, or fails if 'e doesn't have
            // enough...
            // tmpx is amount needed
            // tmpy is cost of new skin
            sTmp = chr[character].aitarget;   // The target
            tTmp = chr[sTmp].model;           // The target's model
            iTmp =  capskincost[tTmp][valuetmpargument&3];
            valuetmpy = iTmp;                // Cost of new skin
            iTmp -= capskincost[tTmp][( chr[sTmp].texture - madskinstart[tTmp] ) & 3];  // Refund

            if ( iTmp > chr[sTmp].money )
            {
                // Not enough...
                valuetmpx = iTmp - chr[sTmp].money;  // Amount needed
                returncode = bfalse;
            }
            else
            {
                // Pay for it...  Cost may be negative after refund...
                chr[sTmp].money -= iTmp;

                if ( chr[sTmp].money > MAXMONEY )  chr[sTmp].money = MAXMONEY;

                valuetmpx = 0;
                returncode = btrue;
            }
            break;

        case FJOINEVILTEAM:
            // This function adds the character to the evil team...
            switch_team( character, EVILTEAM );
            break;

        case FJOINNULLTEAM:
            // This function adds the character to the null team...
            switch_team( character, NULLTEAM );
            break;

        case FJOINGOODTEAM:
            // This function adds the character to the good team...
            switch_team( character, GOODTEAM );
            break;

        case FPITSKILL:
            // This function activates pit deaths...
            pitskill = btrue;
            break;

        case FSETTARGETTOPASSAGEID:
            // This function finds a character who is both in the passage and who has
            // an item with the given IDSZ
            sTmp = who_is_blocking_passage_ID( valuetmpargument, valuetmpdistance );
            returncode = bfalse;

            if ( sTmp != MAXCHR )
            {
                chr[character].aitarget = sTmp;
                returncode = btrue;
            }
            break;

        case FMAKENAMEUNKNOWN:
            // This function makes the name of an item/character unknown.
            chr[character].nameknown = bfalse;
            break;

        case FSPAWNEXACTPARTICLEENDSPAWN:
            // This function spawns a particle that spawns a character...
            tTmp = character;

            if ( chr[character].attachedto != MAXCHR )  tTmp = chr[character].attachedto;

            tTmp = spawn_one_particle( valuetmpx, valuetmpy, valuetmpdistance, chr[character].turnleftright, chr[character].model, valuetmpargument, MAXCHR, 0, chr[character].team, tTmp, 0, MAXCHR );

            if ( tTmp != maxparticles )
            {
                prtspawncharacterstate[tTmp] = valuetmpturn;
            }
            break;

        case FSPAWNPOOFSPEEDSPACINGDAMAGE:
            // This function makes a lovely little poof at the character's location,
            // adjusting the xy speed and spacing and the base damage first
            // Temporarily adjust the values for the particle type
            sTmp = chr[character].model;
            sTmp = madprtpip[sTmp][capgopoofprttype[sTmp]];
            iTmp = pipxyvelbase[sTmp];
            tTmp = pipxyspacingbase[sTmp];
            test = pipdamagebase[sTmp];
            pipxyvelbase[sTmp] = valuetmpx;
            pipxyspacingbase[sTmp] = valuetmpy;
            pipdamagebase[sTmp] = valuetmpargument;
            spawn_poof( character, chr[character].model );
            // Restore the saved values
            pipxyvelbase[sTmp] = iTmp;
            pipxyspacingbase[sTmp] = tTmp;
            pipdamagebase[sTmp] = test;
            break;

        case FGIVEEXPERIENCETOGOODTEAM:
            // This function gives experience to everyone on the G Team
            give_team_experience( GOODTEAM, valuetmpargument, valuetmpdistance );
            break;

        case FDONOTHING:
            // This function does nothing (For use with combination with Else function or debugging)
            break;

        case FGROGTARGET:
            // This function grogs the target for a duration equal to tmpargument
            chr[chr[character].aitarget].grogtime += valuetmpargument;
            break;

        case FDAZETARGET:
            // This function dazes the target for a duration equal to tmpargument
            chr[chr[character].aitarget].dazetime += valuetmpargument;
            break;

        case FENABLERESPAWN:
            // This function turns respawn with JUMP button on
            respawnvalid = btrue;
            break;

        case FDISABLERESPAWN:
            // This function turns respawn with JUMP button off
            respawnvalid = bfalse;
            break;

        case FIFHOLDERSCOREDAHIT:
            // Proceed only if the character's holder scored a hit
            returncode = bfalse;

            if ( ( chr[chr[character].attachedto].alert&ALERTIFSCOREDAHIT ) != 0 )
            {
                returncode = btrue;
                chr[character].aitarget = chr[chr[character].attachedto].hitlast;
            }
            break;

        case FIFHOLDERBLOCKED:
            // This function passes if the holder blocked an attack
            returncode = bfalse;

            if ( ( chr[chr[character].attachedto].alert&ALERTIFBLOCKED ) != 0 )
            {
                returncode = btrue;
                chr[character].aitarget = chr[chr[character].attachedto].attacklast;
            }
            break;

        //case FGETSKILLLEVEL:
        //        // This function sets tmpargument to the shield profiency level of the target
        //        valuetmpargument = capshieldprofiency[chr[character].attachedto];
        //    break;

        case FIFTARGETHASNOTFULLMANA:

            // This function passes only if the target is not at max mana and alive
            if ( !chr[chr[character].aitarget].alive || chr[chr[character].aitarget].mana > chr[chr[character].aitarget].manamax - HURTDAMAGE )
                returncode = bfalse;

            break;

        case FENABLELISTENSKILL:
            // This function increases sound play range by 25%
            local_listening = btrue;
            break;

        case FSETTARGETTOLASTITEMUSED:

            // This sets the target to the last item the character used
            if ( chr[character].lastitemused == character ) returncode = bfalse;
            else chr[character].aitarget = chr[character].lastitemused;

            break;

        case FFOLLOWLINK:
            // Skips to the next module!
            returncode = link_follow( LinkList, valuetmpargument );
            break;

        case FIFOPERATORISLINUX:
            // Proceeds if running on linux
#ifdef __unix__
            returncode = btrue;
#else
            returncode = bfalse;
#endif
            break;

        case FIFTARGETISAWEAPON:
            // Proceeds if the AI target is a melee or ranged weapon
            sTmp = chr[chr[character].aitarget].model;
            returncode = capisranged[sTmp] || (capweaponaction[sTmp] != ACTIONPA);
            break;

        case FIFSOMEONEISSTEALING:
            // This function passes if someone stealed from it's shop
            returncode = ( chr[character].order == STOLEN && chr[character].counter == 3 );
            break;

        case FIFTARGETISASPELL:
            // Proceeds if the AI target has any particle with the [IDAM] or [WDAM] expansion
            iTmp = 0;
            returncode = bfalse;

            while (iTmp < MAXPRTPIPPEROBJECT)
            {

                if (pipintdamagebonus[madprtpip[chr[character].aitarget][iTmp]] || pipwisdamagebonus[madprtpip[chr[character].aitarget][iTmp]])
                {
                    returncode = btrue;
                    break;
                }

                iTmp++;
            }
            break;

        case FIFBACKSTABBED:
            // Proceeds if HitFromBehind, target has [DISA] skill and damage is physical
            returncode = bfalse;

            if ( ( chr[character].alert & ALERTIFATTACKED ) != 0 )
            {
                sTmp = chr[chr[character].attacklast].model;

                if ( chr[character].directionlast >= BEHIND - 8192 && chr[character].directionlast < BEHIND + 8192 )
                {
                    if ( capidsz[sTmp][IDSZ_SKILL] == Make_IDSZ( "STAB" ) )
                    {
                        iTmp = chr[character].damagetypelast;

                        if ( iTmp == DAMAGE_CRUSH || iTmp == DAMAGE_POKE || iTmp == DAMAGE_SLASH ) returncode = btrue;
                    }
                }
            }
            break;

        case FGETTARGETDAMAGETYPE:
            // This function gets the last type of damage for the target
            valuetmpargument = chr[chr[character].aitarget].damagetypelast;
            break;

        case FADDQUEST:

            //This function adds a quest idsz set in tmpargument into the targets quest.txt
            if ( chr[chr[character].aitarget].isplayer )
            {
                add_quest_idsz( chr[chr[character].aitarget].name, valuetmpargument );
                returncode = btrue;
            }
            else returncode = bfalse;

            break;

        case FBEATQUESTALLPLAYERS:
            //This function marks a IDSZ in the targets quest.txt as beaten
            returncode = bfalse;
            iTmp = 0;

            while (iTmp < MAXCHR)
            {
                if ( chr[iTmp].isplayer )
                {
                    if (modify_quest_idsz( chr[iTmp].name, (IDSZ)valuetmpargument, 0 ) == QUEST_BEATEN) returncode = btrue;
                }

                iTmp++;
            }
            break;

        case FIFTARGETHASQUEST:

            //This function proceeds if the target has the unfinished quest specified in tmpargument
            //and sets tmpdistance to the Quest Level of the specified quest.
            if ( chr[chr[character].aitarget].isplayer )
            {
                iTmp = check_player_quest( chr[chr[character].aitarget].name, valuetmpargument );

                if ( iTmp > QUEST_BEATEN )
                {
                    returncode = btrue;
                    valuetmpdistance = iTmp;
                }
                else returncode = bfalse;
            }
            break;

        case FSETQUESTLEVEL:
            //This function modifies the quest level for a specific quest IDSZ
            //tmpargument specifies quest idsz and tmpdistance the adjustment (which may be negative)
            returncode = bfalse;

            if ( chr[chr[character].aitarget].isplayer && valuetmpdistance != 0 )
            {
                if (modify_quest_idsz( chr[chr[character].aitarget].name, valuetmpargument, valuetmpdistance ) > QUEST_NONE) returncode = btrue;
            }
            break;

        case FADDQUESTALLPLAYERS:
            //This function adds a quest idsz set in tmpargument into all local player's quest logs
            //The quest level is set to tmpdistance if the level is not already higher or QUEST_BEATEN
            iTmp = 0;
            returncode = bfalse;

            while (iTmp < MAXPLAYER)
            {
                if ( chr[plaindex[iTmp]].isplayer )
                {
                    returncode = btrue;

                    if (!add_quest_idsz(chr[plaindex[iTmp]].name , valuetmpargument ))       //Try to add it if not already there or beaten
                    {
                        Sint16 i;
                        i = check_player_quest( chr[plaindex[iTmp]].name, valuetmpargument);   //Get the current quest level

                        if (i < 0 || i >= valuetmpdistance) returncode = bfalse;      //It was already beaten
                        else modify_quest_idsz( chr[plaindex[iTmp]].name, valuetmpargument, valuetmpdistance );//Not beaten yet, increase level by 1
                    }
                }

                iTmp++;
            }
            break;

        case FADDBLIPALLENEMIES:
            // show all enemies on the minimap who match the IDSZ given in tmpargument
            // it show only the enemies of the AI target
            local_senseenemies = chr[character].aitarget;
            local_senseenemiesID = valuetmpargument;
            break;

        case FPITSFALL:

            // This function activates pit teleportation...
            if ( valuetmpx > EDGE && valuetmpy > EDGE && valuetmpx < meshedgex - EDGE && valuetmpy < meshedgey - EDGE )
            {
                pitsfall = btrue;
                pitx = valuetmpx;
                pity = valuetmpy;
                pitz = valuetmpdistance;
            }
            else pitskill = btrue;

            break;

        case FIFTARGETISOWNER:
            // This function proceeds only if the target is on another team
            returncode = ( chr[chr[character].aitarget].alive && chr[character].aiowner == chr[character].aitarget );
            break;

        case FEND:
            valueterminate = btrue;
            returncode     = bfalse;
            break;

            // If none of the above, skip the line and log an error
        default:
            log_message( "SCRIPT ERROR: run_function() - ai script %d - unhandled script function %d\n", chr[character].aitype, valuecode );
            returncode = bfalse;
            break;
    }

    return returncode;
}

//--------------------------------------------------------------------------------------------
void set_operand( Uint8 variable )
{
    // ZZ> This function sets one of the tmp* values for scripted AI
    switch ( variable )
    {
        case VARTMPX:
            valuetmpx = valueoperationsum;
            break;

        case VARTMPY:
            valuetmpy = valueoperationsum;
            break;

        case VARTMPDISTANCE:
            valuetmpdistance = valueoperationsum;
            break;

        case VARTMPTURN:
            valuetmpturn = valueoperationsum;
            break;

        case VARTMPARGUMENT:
            valuetmpargument = valueoperationsum;
            break;

    }
}

//--------------------------------------------------------------------------------------------
void run_operand( Uint32 opcode, int character )
{
    // ZZ> This function does the scripted arithmetic in OPERATOR, OPERAND pairs

    char * varname, * op;

    STRING buffer;
    Uint8  variable;
    Uint8  operation;

    Uint32 iTmp;

    // get the operator
    iTmp      = 0;
    varname   = buffer;
    operation = GET_DATA_BITS( opcode );

    if ( 0 != (opcode & FUNCTION_BIT) )
    {
        // Get the working opcode from a constant, constants are all but high 5 bits
        iTmp = opcode & VALUE_BITS;

        snprintf( buffer, sizeof(STRING), "%d", iTmp );
    }
    else
    {
        // Get the variable opcode from a register
        variable = opcode & 0x07FFFFFF;

        switch ( variable )
        {
            case VARTMPX:
                varname = "TMPX";
                iTmp = valuetmpx;
                break;

            case VARTMPY:
                varname = "TMPY";
                iTmp = valuetmpy;
                break;

            case VARTMPDISTANCE:
                varname = "TMPDISTANCE";
                iTmp = valuetmpdistance;
                break;

            case VARTMPTURN:
                varname = "TMPTURN";
                iTmp = valuetmpturn;
                break;

            case VARTMPARGUMENT:
                varname = "TMPARGUMENT";
                iTmp = valuetmpargument;
                break;

            case VARRAND:
                varname = "RAND";
                iTmp = RANDIE;
                break;

            case VARSELFX:
                varname = "SELFX";
                iTmp = chr[character].xpos;
                break;

            case VARSELFY:
                varname = "SELFY";
                iTmp = chr[character].ypos;
                break;

            case VARSELFTURN:
                varname = "SELFTURN";
                iTmp = chr[character].turnleftright;
                break;

            case VARSELFCOUNTER:
                varname = "SELFCOUNTER";
                iTmp = chr[character].counter;
                break;

            case VARSELFORDER:
                varname = "SELFORDER";
                iTmp = chr[character].order;
                break;

            case VARSELFMORALE:
                varname = "SELFMORALE";
                iTmp = teammorale[chr[character].baseteam];
                break;

            case VARSELFLIFE:
                varname = "SELFLIFE";
                iTmp = chr[character].life;
                break;

            case VARTARGETX:
                varname = "TARGETX";
                iTmp = chr[chr[character].aitarget].xpos;
                break;

            case VARTARGETY:
                varname = "TARGETY";
                iTmp = chr[chr[character].aitarget].ypos;
                break;

            case VARTARGETDISTANCE:
                varname = "TARGETDISTANCE";
                iTmp = ABS( ( int )( chr[chr[character].aitarget].xpos - chr[character].xpos ) ) +
                       ABS( ( int )( chr[chr[character].aitarget].ypos - chr[character].ypos ) );
                break;

            case VARTARGETTURN:
                varname = "TARGETTURN";
                iTmp = chr[chr[character].aitarget].turnleftright;
                break;

            case VARLEADERX:
                varname = "LEADERX";
                iTmp = chr[character].xpos;

                if ( teamleader[chr[character].team] != NOLEADER )
                    iTmp = chr[teamleader[chr[character].team]].xpos;

                break;

            case VARLEADERY:
                varname = "LEADERY";
                iTmp = chr[character].ypos;

                if ( teamleader[chr[character].team] != NOLEADER )
                    iTmp = chr[teamleader[chr[character].team]].ypos;

                break;

            case VARLEADERDISTANCE:
                varname = "LEADERDISTANCE";
                iTmp = 10000;

                if ( teamleader[chr[character].team] != NOLEADER )
                    iTmp = ABS( ( int )( chr[teamleader[chr[character].team]].xpos - chr[character].xpos ) ) +
                           ABS( ( int )( chr[teamleader[chr[character].team]].ypos - chr[character].ypos ) );

                break;

            case VARLEADERTURN:
                varname = "LEADERTURN";
                iTmp = chr[character].turnleftright;

                if ( teamleader[chr[character].team] != NOLEADER )
                    iTmp = chr[teamleader[chr[character].team]].turnleftright;

                break;

            case VARGOTOX:
                varname = "GOTOX";
                if (chr[character].aigoto == chr[character].aigotoadd)
                {
                    iTmp = chr[character].xpos;
                }
                else
                {
                    iTmp = chr[character].aigotox[chr[character].aigoto];
                }
                break;

            case VARGOTOY:
                varname = "GOTOY";
                if (chr[character].aigoto == chr[character].aigotoadd)
                {
                    iTmp = chr[character].ypos;
                }
                else
                {
                    iTmp = chr[character].aigotoy[chr[character].aigoto];
                }
                break;

            case VARGOTODISTANCE:
                varname = "GOTODISTANCE";
                if (chr[character].aigoto == chr[character].aigotoadd)
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = ABS( ( int )( chr[character].aigotox[chr[character].aigoto] - chr[character].xpos ) ) +
                           ABS( ( int )( chr[character].aigotoy[chr[character].aigoto] - chr[character].ypos ) );
                }
                break;

            case VARTARGETTURNTO:
                varname = "TARGETTURNTO";
                iTmp = ATAN2( chr[chr[character].aitarget].ypos - chr[character].ypos, chr[chr[character].aitarget].xpos - chr[character].xpos ) * 65535 / ( TWO_PI );
                iTmp += 32768;
                iTmp &= 65535;
                break;

            case VARPASSAGE:
                varname = "PASSAGE";
                iTmp = chr[character].passage;
                break;

            case VARWEIGHT:
                varname = "WEIGHT";
                iTmp = chr[character].holdingweight;
                break;

            case VARSELFALTITUDE:
                varname = "SELFALTITUDE";
                iTmp = chr[character].zpos - chr[character].level;
                break;

            case VARSELFID:
                varname = "SELFID";
                iTmp = capidsz[chr[character].model][IDSZ_TYPE];
                break;

            case VARSELFHATEID:
                varname = "SELFHATEID";
                iTmp = capidsz[chr[character].model][IDSZ_HATE];
                break;

            case VARSELFMANA:
                varname = "SELFMANA";
                iTmp = chr[character].mana;

                if ( chr[character].canchannel )  iTmp += chr[character].life;

                break;

            case VARTARGETSTR:
                varname = "TARGETSTR";
                iTmp = chr[chr[character].aitarget].strength;
                break;

            case VARTARGETWIS:
                varname = "TARGETWIS";
                iTmp = chr[chr[character].aitarget].wisdom;
                break;

            case VARTARGETINT:
                varname = "TARGETINT";
                iTmp = chr[chr[character].aitarget].intelligence;
                break;

            case VARTARGETDEX:
                varname = "TARGETDEX";
                iTmp = chr[chr[character].aitarget].dexterity;
                break;

            case VARTARGETLIFE:
                varname = "TARGETLIFE";
                iTmp = chr[chr[character].aitarget].life;
                break;

            case VARTARGETMANA:
                varname = "TARGETMANA";
                iTmp = chr[chr[character].aitarget].mana;

                if ( chr[chr[character].aitarget].canchannel )  iTmp += chr[chr[character].aitarget].life;

                break;

            case VARTARGETLEVEL:
                varname = "TARGETLEVEL";
                iTmp = chr[chr[character].aitarget].experiencelevel;
                break;

            case VARTARGETSPEEDX:
                varname = "TARGETSPEEDX";
                iTmp = chr[chr[character].aitarget].xvel;
                break;

            case VARTARGETSPEEDY:
                varname = "TARGETSPEEDY";
                iTmp = chr[chr[character].aitarget].yvel;
                break;

            case VARTARGETSPEEDZ:
                varname = "TARGETSPEEDZ";
                iTmp = chr[chr[character].aitarget].zvel;
                break;

            case VARSELFSPAWNX:
                varname = "SELFSPAWNX";
                iTmp = chr[character].xstt;
                break;

            case VARSELFSPAWNY:
                varname = "SELFSPAWNY";
                iTmp = chr[character].ystt;
                break;

            case VARSELFSTATE:
                varname = "SELFSTATE";
                iTmp = chr[character].aistate;
                break;

            case VARSELFSTR:
                varname = "SELFSTR";
                iTmp = chr[character].strength;
                break;

            case VARSELFWIS:
                varname = "SELFWIS";
                iTmp = chr[character].wisdom;
                break;

            case VARSELFINT:
                varname = "SELFINT";
                iTmp = chr[character].intelligence;
                break;

            case VARSELFDEX:
                varname = "SELFDEX";
                iTmp = chr[character].dexterity;
                break;

            case VARSELFMANAFLOW:
                varname = "SELFMANAFLOW";
                iTmp = chr[character].manaflow;
                break;

            case VARTARGETMANAFLOW:
                varname = "TARGETMANAFLOW";
                iTmp = chr[chr[character].aitarget].manaflow;
                break;

            case VARSELFATTACHED:
                varname = "SELFATTACHED";
                iTmp = number_of_attached_particles( character );
                break;

            case VARSWINGTURN:
                varname = "SWINGTURN";
                iTmp = camswing << 2;
                break;

            case VARXYDISTANCE:
                varname = "XYDISTANCE";
                iTmp = SQRT( valuetmpx * valuetmpx + valuetmpy * valuetmpy );
                break;

            case VARSELFZ:
                varname = "SELFZ";
                iTmp = chr[character].zpos;
                break;

            case VARTARGETALTITUDE:
                varname = "TARGETALTITUDE";
                iTmp = chr[chr[character].aitarget].zpos - chr[chr[character].aitarget].level;
                break;

            case VARTARGETZ:
                varname = "TARGETZ";
                iTmp = chr[chr[character].aitarget].zpos;
                break;

            case VARSELFINDEX:
                varname = "SELFINDEX";
                iTmp = character;
                break;

            case VAROWNERX:
                varname = "OWNERX";
                iTmp = chr[chr[character].aiowner].xpos;
                break;

            case VAROWNERY:
                varname = "OWNERY";
                iTmp = chr[chr[character].aiowner].ypos;
                break;

            case VAROWNERTURN:
                varname = "OWNERTURN";
                iTmp = chr[chr[character].aiowner].turnleftright;
                break;

            case VAROWNERDISTANCE:
                varname = "OWNERDISTANCE";
                iTmp = ABS( ( int )( chr[chr[character].aiowner].xpos - chr[character].xpos ) ) +
                       ABS( ( int )( chr[chr[character].aiowner].ypos - chr[character].ypos ) );
                break;

            case VAROWNERTURNTO:
                varname = "OWNERTURNTO";
                iTmp = ATAN2( chr[chr[character].aiowner].ypos - chr[character].ypos, chr[chr[character].aiowner].xpos - chr[character].xpos ) * 65535 / ( TWO_PI );
                iTmp += 32768;
                iTmp &= 65535;
                break;

            case VARXYTURNTO:
                varname = "XYTURNTO";
                iTmp = ATAN2( valuetmpy - chr[character].ypos, valuetmpx - chr[character].xpos ) * 65535 / ( TWO_PI );
                iTmp += 32768;
                iTmp &= 65535;
                break;

            case VARSELFMONEY:
                varname = "SELFMONEY";
                iTmp = chr[character].money;
                break;

            case VARSELFACCEL:
                varname = "SELFACCEL";
                iTmp = ( chr[character].maxaccel * 100.0f );
                break;

            case VARTARGETEXP:
                varname = "TARGETEXP";
                iTmp = chr[chr[character].aitarget].experience;
                break;

            case VARSELFAMMO:
                varname = "SELFAMMO";
                iTmp = chr[character].ammo;
                break;

            case VARTARGETAMMO:
                varname = "TARGETAMMO";
                iTmp = chr[chr[character].aitarget].ammo;
                break;

            case VARTARGETMONEY:
                varname = "TARGETMONEY";
                iTmp = chr[chr[character].aitarget].money;
                break;

            case VARTARGETTURNAWAY:
                varname = "TARGETTURNAWAY";
                iTmp = ATAN2( chr[chr[character].aitarget].ypos - chr[character].ypos, chr[chr[character].aitarget].xpos - chr[character].xpos ) * 65535 / ( TWO_PI );
                iTmp += 32768;
                iTmp &= 65535;
                break;

            case VARSELFLEVEL:
                varname = "SELFLEVEL";
                iTmp = chr[character].experiencelevel;
                break;

            case VARTARGETRELOADTIME:
                varname = "TARGETRELOADTIME";
                iTmp = chr[chr[character].aitarget].reloadtime;
                break;

            default: 
                log_message( "SCRIPT ERROR: run_operand() - model == %d, class name == \"%s\" - Unknown variable found!\n", script_error_model, script_error_classname );
                break;
        }
    }

    // Now do the math
    op = "UNKNOWN";
    switch ( operation )
    {
        case OPADD: 
            op = "ADD";
            valueoperationsum += iTmp;
            break;

        case OPSUB: 
            op = "SUB";
            valueoperationsum -= iTmp;
            break;

        case OPAND: 
            op = "AND";
            valueoperationsum &= iTmp;
            break;

        case OPSHR: 
            op = "SHR";
            valueoperationsum >>= iTmp;
            break;

        case OPSHL: 
            op = "SHL";
            valueoperationsum <<= iTmp;
            break;

        case OPMUL: 
            op = "MUL";
            valueoperationsum *= iTmp;
            break;

        case OPDIV: 
            op = "DIV";
            if ( iTmp != 0 )
            {
                valueoperationsum /= iTmp;
            }
            else 
            {
                log_message( "SCRIPT ERROR: run_operand() - model == %d, class name == \"%s\" - Cannot divide by zero!\n", script_error_model, script_error_classname );
            }
            break;

        case OPMOD: 
            op = "MOD";
            if ( iTmp != 0 )
            {
                valueoperationsum %= iTmp;
            }
            else 
            {
                log_message( "SCRIPT ERROR: run_operand() - model == %d, class name == \"%s\" - Cannot modulo by zero!\n", script_error_model, script_error_classname );
            }
            break;

        default:
            log_message( "SCRIPT ERROR: run_operand() - model == %d, class name == \"%s\" - unknown op\n", script_error_model, script_error_classname );
            break;
    }

    if ( debug_scripts ) printf( "%s %s(%d) ", op, varname, iTmp );
}

//--------------------------------------------------------------------------------------------
void let_character_think( int character )
{
    // ZZ> This function lets one character do AI stuff
    Uint16 aicode;
    Uint32 index, index_end;
    Uint32 opcode;
    Uint32 iTmp;
    Uint8 functionreturn;

    if( character >= MAXCHR || !chr[character].on )  return;

    // Make life easier
    valueoldtarget = chr[character].aitarget;
    aicode = chr[character].aitype;

    // Figure out alerts that weren't already set
    set_alerts( character );

    script_error_classname = "UNKNOWN";
    script_error_model     = chr[character].model;
    if( script_error_model < MAXMODEL )
    {
        script_error_classname = capclassname[ script_error_model ];
    }

    if ( debug_scripts )
    {
        printf( "\n\n--------\n%d - %s\n", script_error_model, script_error_classname );
    }

    // Clear the button latches
    if ( !chr[character].isplayer )
    {
        chr[character].latchbutton = 0;
    }

    // Reset the target if it can't be seen
    if ( !chr[character].canseeinvisible && chr[character].alive )
    {
        if ( chr[chr[character].aitarget].alpha <= INVISIBLE || chr[chr[character].aitarget].light <= INVISIBLE )
        {
            chr[character].aitarget = character;
        }
    }

    // Run the AI Script
    valuegopoof    = bfalse;
    valuechanged   = bfalse;
    valueterminate = bfalse;

    index     = iAisStartPosition[aicode];
    index_end = iAisEndPosition[aicode];
    while ( !valueterminate && index < index_end ) // End Function
    {
        opcode = iCompiledAis[index];

        // Was it a function
        if ( 0 != ( opcode & FUNCTION_BIT ) )
        {
            int new_index;

            // Run the function
            functionreturn = run_function( opcode, character );

            // Get the jump code
            index++;
            iTmp = iCompiledAis[index];

            if ( functionreturn )
            {
                // Proceed to the next function
                new_index = index + 1;
            }
            else
            {
                // Jump to where the jump code says to go
                new_index = iTmp;
            }

            assert( new_index > index && new_index >= iAisStartPosition[aicode] );

            index = new_index;
        }
        else
        {
            index = run_operation( opcode, index, index_end, character );
        }

        // This is used by the Else function
        valuelastindent = opcode & DATA_BITS;
    }

    // Set latches
    if ( !chr[character].isplayer )
    {
        float latch2;

        if ( chr[character].ismount && MAXCHR != chr[character].holdingwhich[0] && chr[chr[character].holdingwhich[0]].on )
        {
            // Mount
            chr[character].latchx = chr[chr[character].holdingwhich[0]].latchx;
            chr[character].latchy = chr[chr[character].holdingwhich[0]].latchy;
        }
        else if ( chr[character].aigoto != chr[character].aigotoadd )
        {
            // Normal AI
            chr[character].latchx = ( chr[character].aigotox[chr[character].aigoto] - chr[character].xpos ) / (128 << 2);
            chr[character].latchy = ( chr[character].aigotoy[chr[character].aigoto] - chr[character].ypos ) / (128 << 2);
        }
        else
        {
            // AI, but no valid waypoints
            chr[character].latchx = 0;
            chr[character].latchy = 0;
        }

        latch2 = chr[character].latchx * chr[character].latchx + chr[character].latchy * chr[character].latchy;
        if (latch2 > 1.0f)
        {
            latch2 = 1.0f / sqrt(latch2);
            chr[character].latchx *= latch2;
            chr[character].latchy *= latch2;
        }
    }

    // Clear alerts for next time around
    chr[character].alert = 0;

    if ( valuechanged )  chr[character].alert = ALERTIFCHANGED;

    // Do poofing
    if ( valuegopoof )
    {
        if ( chr[character].attachedto != MAXCHR )
            detach_character_from_mount( character, btrue, bfalse );

        if ( chr[character].holdingwhich[0] != MAXCHR )
            detach_character_from_mount( chr[character].holdingwhich[0], btrue, bfalse );

        if ( chr[character].holdingwhich[1] != MAXCHR )
            detach_character_from_mount( chr[character].holdingwhich[1], btrue, bfalse );

        free_inventory( character );
        free_one_character( character );
        // If this character was killed in another's script, we don't want the poof to
        // carry over...
        valuegopoof = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
void let_ai_think()
{
    // ZZ> This function lets every computer controlled character do AI stuff
    int character;

    numblip = 0;

    for ( character = 0; character < MAXCHR; character++ )
    {
        if ( chr[character].on && ( !chr[character].inpack || capisequipment[chr[character].model] ) && ( chr[character].alive || ( chr[character].alert&ALERTIFCLEANEDUP ) || ( chr[character].alert&ALERTIFCRUSHED ) ) )
        {
            // Crushed characters shouldn't be alert to anything else
            if ( chr[character].alert & ALERTIFCRUSHED )  chr[character].alert = ALERTIFCRUSHED;

            // Cleaned up characters shouldn't be alert to anything else
            if ( chr[character].alert & ALERTIFCLEANEDUP )  chr[character].alert = ALERTIFCLEANEDUP;

            let_character_think( character );
        }
    }
}
