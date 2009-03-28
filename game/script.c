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
            returncode = ( ( chralert[character] & ALERTIFSPAWNED ) != 0 );
            break;

        case FIFTIMEOUT:
            // Proceed only if time alert is set
            returncode = ( chraitime[character] == 0 );
            break;

        case FIFATWAYPOINT:
            // Proceed only if the character reached a waypoint
            returncode = ( ( chralert[character] & ALERTIFATWAYPOINT ) != 0 );
            break;

        case FIFATLASTWAYPOINT:
            // Proceed only if the character reached its last waypoint
            returncode = ( ( chralert[character] & ALERTIFATLASTWAYPOINT ) != 0 );
            break;

        case FIFATTACKED:
            // Proceed only if the character was damaged
            returncode = ( ( chralert[character] & ALERTIFATTACKED ) != 0 );
            break;

        case FIFBUMPED:
            // Proceed only if the character was bumped
            returncode = ( ( chralert[character] & ALERTIFBUMPED ) != 0 );
            break;

        case FIFORDERED:
            // Proceed only if the character was ordered
            returncode = ( ( chralert[character] & ALERTIFORDERED ) != 0 );
            break;

        case FIFCALLEDFORHELP:
            // Proceed only if the character was called for help
            returncode = ( ( chralert[character] & ALERTIFCALLEDFORHELP ) != 0 );
            break;

        case FSETCONTENT:
            // Set the content
            chraicontent[character] = valuetmpargument;
            break;

        case FIFKILLED:
            // Proceed only if the character's been killed
            returncode = ( ( chralert[character] & ALERTIFKILLED ) != 0 );
            break;

        case FIFTARGETKILLED:
            // Proceed only if the character's target has just died
            returncode = ( ( chralert[character] & ALERTIFTARGETKILLED ) != 0 );
            break;

        case FCLEARWAYPOINTS:
            // Clear out all waypoints
            chraigoto[character] = 0;
            chraigotoadd[character] = 0;
            break;

        case FADDWAYPOINT:
            // Add a waypoint to the waypoint list
            chraigotox[character][chraigotoadd[character]] = valuetmpx;
            chraigotoy[character][chraigotoadd[character]] = valuetmpy;

            chraigotoadd[character]++;
            if ( chraigotoadd[character] > MAXWAY - 1 )  chraigotoadd[character] = MAXWAY - 1;
            break;

        case FFINDPATH:
            // Yep this is it
            if ( chrmodel[chraitarget[character]] != character )
            {
                if ( valuetmpdistance != MOVE_FOLLOW )
                {
                    valuetmpx = chrxpos[chraitarget[ chrmodel[chraitarget[character]] ]];
                    valuetmpy = chrypos[chraitarget[ chrmodel[chraitarget[character]] ]];
                }
                else
                {
                    valuetmpx = ( rand() & 1023 ) - 512 + chrxpos[chraitarget[ chrmodel[chraitarget[character]] ]];
                    valuetmpy = ( rand() & 1023 ) - 512 + chrypos[chraitarget[ chrmodel[chraitarget[character]] ]];
                }

                valuetmpturn = ATAN2( chrypos[chraitarget[character]] - chrypos[character], chrxpos[chraitarget[character]] - chrxpos[character] ) * 65535 / ( TWO_PI );
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
                chraigotox[character][chraigotoadd[character]] = valuetmpx;
                chraigotoy[character][chraigotoadd[character]] = valuetmpy;

                chraigotoadd[character]++;
                if ( chraigotoadd[character] > MAXWAY - 1 ) chraigotoadd[character] = MAXWAY - 1;

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
            valuetmpx = capskincost[chrmodel[chraitarget[character]]][sTmp];
            break;

        case FSETTIME:

            // This function resets the time
            if ( valuetmpargument > -1 )
                chraitime[character] = valuetmpargument;

            break;

        case FGETCONTENT:
            // Get the content
            valuetmpargument = chraicontent[character];
            break;

        case FJOINTARGETTEAM:
            // This function allows the character to leave its own team and join another
            returncode = bfalse;

            if ( chron[chraitarget[character]] )
            {
                switch_team( character, chrteam[chraitarget[character]] );
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
            sTmp = chrholdingwhich[chraitarget[character]][0];
            returncode = bfalse;

            if ( sTmp != MAXCHR )
            {
                chraitarget[character] = sTmp;
                returncode = btrue;
            }
            break;

        case FSETTARGETTOTARGETRIGHTHAND:
            // This function sets the target to the target's right item
            sTmp = chrholdingwhich[chraitarget[character]][1];
            returncode = bfalse;

            if ( sTmp != MAXCHR )
            {
                chraitarget[character] = sTmp;
                returncode = btrue;
            }
            break;

        case FSETTARGETTOWHOEVERATTACKED:

            // This function sets the target to whoever attacked the character last,
            // failing for damage tiles
            if ( chrattacklast[character] != MAXCHR )
            {
                chraitarget[character] = chrattacklast[character];
            }
            else
            {
                returncode = bfalse;
            }
            break;

        case FSETTARGETTOWHOEVERBUMPED:
            // This function sets the target to whoever bumped into the
            // character last.  It never fails
            chraitarget[character] = chrbumplast[character];
            break;

        case FSETTARGETTOWHOEVERCALLEDFORHELP:
            // This function sets the target to whoever needs help
            chraitarget[character] = teamsissy[chrteam[character]];
            break;

        case FSETTARGETTOOLDTARGET:
            // This function reverts to the target with whom the script started
            chraitarget[character] = valueoldtarget;
            break;

        case FSETTURNMODETOVELOCITY:
            // This function sets the turn mode
            chrturnmode[character] = TURNMODEVELOCITY;
            break;

        case FSETTURNMODETOWATCH:
            // This function sets the turn mode
            chrturnmode[character] = TURNMODEWATCH;
            break;

        case FSETTURNMODETOSPIN:
            // This function sets the turn mode
            chrturnmode[character] = TURNMODESPIN;
            break;

        case FSETBUMPHEIGHT:
            // This function changes a character's bump height
            chrbumpheight[character] = valuetmpargument * chrfat[character];
            chrbumpheightsave[character] = valuetmpargument;
            break;

        case FIFTARGETHASID:
            // This function proceeds if ID matches tmpargument
            sTmp = chrmodel[chraitarget[character]];
            returncode = capidsz[sTmp][IDSZ_PARENT] == ( Uint32 ) valuetmpargument;
            returncode = returncode | ( capidsz[sTmp][IDSZ_TYPE] == ( Uint32 ) valuetmpargument );
            break;

        case FIFTARGETHASITEMID:
            // This function proceeds if the target has a matching item in his/her pack
            returncode = bfalse;
            // Check the pack
            sTmp = chrnextinpack[chraitarget[character]];

            while ( sTmp != MAXCHR )
            {
                if ( capidsz[chrmodel[sTmp]][IDSZ_PARENT] == ( Uint32 ) valuetmpargument || capidsz[chrmodel[sTmp]][IDSZ_TYPE] == ( Uint32 ) valuetmpargument )
                {
                    returncode = btrue;
                    sTmp = MAXCHR;
                }
                else
                {
                    sTmp = chrnextinpack[sTmp];
                }
            }

            // Check left hand
            sTmp = chrholdingwhich[chraitarget[character]][0];

            if ( sTmp != MAXCHR )
            {
                sTmp = chrmodel[sTmp];

                if ( capidsz[sTmp][IDSZ_PARENT] == ( Uint32 ) valuetmpargument || capidsz[sTmp][IDSZ_TYPE] == ( Uint32 ) valuetmpargument )
                    returncode = btrue;
            }

            // Check right hand
            sTmp = chrholdingwhich[chraitarget[character]][1];

            if ( sTmp != MAXCHR )
            {
                sTmp = chrmodel[sTmp];

                if ( capidsz[sTmp][IDSZ_PARENT] == ( Uint32 ) valuetmpargument || capidsz[sTmp][IDSZ_TYPE] == ( Uint32 ) valuetmpargument )
                    returncode = btrue;
            }
            break;

        case FIFTARGETHOLDINGITEMID:
            // This function proceeds if ID matches tmpargument and returns the latch for the
            // hand in tmpargument
            returncode = bfalse;
            // Check left hand
            sTmp = chrholdingwhich[chraitarget[character]][0];

            if ( sTmp != MAXCHR )
            {
                sTmp = chrmodel[sTmp];

                if ( capidsz[sTmp][IDSZ_PARENT] == ( IDSZ ) valuetmpargument || capidsz[sTmp][IDSZ_TYPE] == ( IDSZ ) valuetmpargument )
                {
                    valuetmpargument = LATCHBUTTONLEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            sTmp = chrholdingwhich[chraitarget[character]][1];

            if ( sTmp != MAXCHR && !returncode )
            {
                sTmp = chrmodel[sTmp];

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

            returncode = (0 != check_skills( chraitarget[character], ( IDSZ )valuetmpargument ));

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
            chrmaxaccel[character] *= 0.66f;
            break;

        case FSNEAK:
            reset_character_accel( character );
            chrmaxaccel[character] *= 0.33f;
            break;

        case FDOACTION:
            // This function starts a new action, if it is valid for the model
            // It will fail if the action is invalid or if the character is doing
            // something else already
            returncode = bfalse;

            if ( valuetmpargument < MAXACTION && chractionready[character] )
            {
                if ( madactionvalid[chrmodel[character]][valuetmpargument] )
                {
                    chraction[character] = valuetmpargument;
                    chrlip[character] = 0;
                    chrlastframe[character] = chrframe[character];
                    chrframe[character] = madactionstart[chrmodel[character]][valuetmpargument];
                    chractionready[character] = bfalse;
                    returncode = btrue;
                }
            }
            break;

        case FKEEPACTION:
            // This function makes the current animation halt on the last frame
            chrkeepaction[character] = btrue;
            break;

        case FISSUEORDER:
            // This function issues an order to all teammates
            issue_order( character, valuetmpargument );
            break;

        case FDROPWEAPONS:
            // This funtion drops the character's in hand items/riders
            sTmp = chrholdingwhich[character][0];

            if ( sTmp != MAXCHR )
            {
                detach_character_from_mount( sTmp, btrue, btrue );

                if ( chrismount[character] )
                {
                    chrzvel[sTmp] = DISMOUNTZVEL;
                    chrzpos[sTmp] += DISMOUNTZVEL;
                    chrjumptime[sTmp] = JUMPDELAY;
                }
            }

            sTmp = chrholdingwhich[character][1];

            if ( sTmp != MAXCHR )
            {
                detach_character_from_mount( sTmp, btrue, btrue );

                if ( chrismount[character] )
                {
                    chrzvel[sTmp] = DISMOUNTZVEL;
                    chrzpos[sTmp] += DISMOUNTZVEL;
                    chrjumptime[sTmp] = JUMPDELAY;
                }
            }
            break;

        case FTARGETDOACTION:
            // This function starts a new action, if it is valid for the model
            // It will fail if the action is invalid or if the target is doing
            // something else already
            returncode = bfalse;

            if ( chralive[chraitarget[character]] )
            {
                if ( valuetmpargument < MAXACTION && chractionready[chraitarget[character]] )
                {
                    if ( madactionvalid[chrmodel[chraitarget[character]]][valuetmpargument] )
                    {
                        chraction[chraitarget[character]] = valuetmpargument;
                        chrlip[chraitarget[character]] = 0;
                        chrlastframe[chraitarget[character]] = chrframe[chraitarget[character]];
                        chrframe[chraitarget[character]] = madactionstart[chrmodel[chraitarget[character]]][valuetmpargument];
                        chractionready[chraitarget[character]] = bfalse;
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

            if ( !chrisplayer[character] )
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
            tTmp = chraitarget[character];
            sTmp = chrnextinpack[tTmp];

            while ( sTmp != MAXCHR )
            {
                if ( capidsz[chrmodel[sTmp]][IDSZ_PARENT] == ( IDSZ) valuetmpargument || capidsz[chrmodel[sTmp]][IDSZ_TYPE] == ( IDSZ ) valuetmpargument )
                {
                    returncode = btrue;
                    iTmp = sTmp;
                    sTmp = MAXCHR;
                }
                else
                {
                    tTmp = sTmp;
                    sTmp = chrnextinpack[sTmp];
                }
            }

            // Check left hand
            sTmp = chrholdingwhich[chraitarget[character]][0];

            if ( sTmp != MAXCHR )
            {
                sTmp = chrmodel[sTmp];

                if ( capidsz[sTmp][IDSZ_PARENT] == ( IDSZ ) valuetmpargument || capidsz[sTmp][IDSZ_TYPE] == ( IDSZ ) valuetmpargument )
                {
                    returncode = btrue;
                    iTmp = chrholdingwhich[chraitarget[character]][0];
                }
            }

            // Check right hand
            sTmp = chrholdingwhich[chraitarget[character]][1];

            if ( sTmp != MAXCHR )
            {
                sTmp = chrmodel[sTmp];

                if ( capidsz[sTmp][IDSZ_PARENT] == ( IDSZ ) valuetmpargument || capidsz[sTmp][IDSZ_TYPE] == ( IDSZ ) valuetmpargument )
                {
                    returncode = btrue;
                    iTmp = chrholdingwhich[chraitarget[character]][1];
                }
            }

            if ( returncode )
            {
                if ( chrammo[iTmp] <= 1 )
                {
                    // Poof the item
                    if ( chrinpack[iTmp] )
                    {
                        // Remove from the pack
                        chrnextinpack[tTmp] = chrnextinpack[iTmp];
                        chrnuminpack[chraitarget[character]]--;
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
                    chrammo[iTmp]--;
                }
            }
            break;

        case FDOACTIONOVERRIDE:
            // This function starts a new action, if it is valid for the model
            // It will fail if the action is invalid
            returncode = bfalse;

            if ( valuetmpargument < MAXACTION )
            {
                if ( madactionvalid[chrmodel[character]][valuetmpargument] )
                {
                    chraction[character] = valuetmpargument;
                    chrlip[character] = 0;
                    chrlastframe[character] = chrframe[character];
                    chrframe[character] = madactionstart[chrmodel[character]][valuetmpargument];
                    chractionready[character] = bfalse;
                    returncode = btrue;
                }
            }
            break;

        case FIFHEALED:
            // Proceed only if the character was healed
            returncode = ( ( chralert[character] & ALERTIFHEALED ) != 0 );
            break;

        case FSENDMESSAGE:
            // This function sends a message to the players
            display_message( madmsgstart[chrmodel[character]] + valuetmpargument, character );
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
            chraistate[character] = valuetmpargument;
            break;

        case FGETSTATE:
            // This function reads the character's state variable
            valuetmpargument = chraistate[character];
            break;

        case FIFSTATEIS:
            // This function fails if the character's state is inequal to tmpargument
            returncode = ( valuetmpargument == chraistate[character] );
            break;

        case FIFTARGETCANOPENSTUFF:
            // This function fails if the target can't open stuff
            returncode = chropenstuff[chraitarget[character]];
            break;

        case FIFGRABBED:
            // Proceed only if the character was picked up
            returncode = ( ( chralert[character] & ALERTIFGRABBED ) != 0 );
            break;

        case FIFDROPPED:
            // Proceed only if the character was dropped
            returncode = ( ( chralert[character] & ALERTIFDROPPED ) != 0 );
            break;

        case FSETTARGETTOWHOEVERISHOLDING:
            // This function sets the target to the character's mount or holder,
            // failing if the character has no mount or holder
            returncode = bfalse;

            if ( chrattachedto[character] < MAXCHR )
            {
                chraitarget[character] = chrattachedto[character];
                returncode = btrue;
            }
            break;

        case FDAMAGETARGET:
            // This function applies little bit of love to the character's target.
            // The amount is set in tmpargument
            damage_character( chraitarget[character], 0, valuetmpargument, 1, chrdamagetargettype[character], chrteam[character], character, DAMFXBLOC );
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
            valuetmpargument = chrbumpheight[character];
            break;

        case FIFREAFFIRMED:
            // Proceed only if the character was reaffirmed
            returncode = ( ( chralert[character] & ALERTIFREAFFIRMED ) != 0 );
            break;

        case FUNKEEPACTION:
            // This function makes the current animation start again
            chrkeepaction[character] = bfalse;
            break;

        case FIFTARGETISONOTHERTEAM:
            // This function proceeds only if the target is on another team
            returncode = ( chralive[chraitarget[character]] && chrteam[chraitarget[character]] != chrteam[character] );
            break;

        case FIFTARGETISONHATEDTEAM:
            // This function proceeds only if the target is on an enemy team
            returncode = ( chralive[chraitarget[character]] && teamhatesteam[chrteam[character]][chrteam[chraitarget[character]]] && !chrinvictus[chraitarget[character]] );
            break;

        case FPRESSLATCHBUTTON:
            // This function sets the latch buttons
            chrlatchbutton[character] = chrlatchbutton[character] | valuetmpargument;
            break;

        case FSETTARGETTOTARGETOFLEADER:
            // This function sets the character's target to the target of its leader,
            // or it fails with no change if the leader is dead
            returncode = bfalse;

            if ( teamleader[chrteam[character]] != NOLEADER )
            {
                chraitarget[character] = chraitarget[teamleader[chrteam[character]]];
                returncode = btrue;
            }
            break;

        case FIFLEADERKILLED:
            // This function proceeds only if the character's leader has just died
            returncode = ( ( chralert[character] & ALERTIFLEADERKILLED ) != 0 );
            break;

        case FBECOMELEADER:
            // This function makes the character the team leader
            teamleader[chrteam[character]] = character;
            break;

        case FCHANGETARGETARMOR:
            // This function sets the target's armor type and returns the old type
            // as tmpargument and the new type as tmpx
            iTmp = chrtexture[chraitarget[character]] - madskinstart[chrmodel[chraitarget[character]]];
            valuetmpx = change_armor( chraitarget[character], valuetmpargument );
            valuetmpargument = iTmp;  // The character's old armor
            break;

        case FGIVEMONEYTOTARGET:
            // This function transfers money from the character to the target, and sets
            // tmpargument to the amount transferred
            iTmp = chrmoney[character];
            tTmp = chrmoney[chraitarget[character]];
            iTmp -= valuetmpargument;
            tTmp += valuetmpargument;

            if ( iTmp < 0 ) { tTmp += iTmp;  valuetmpargument += iTmp;  iTmp = 0; }

            if ( tTmp < 0 ) { iTmp += tTmp;  valuetmpargument += tTmp;  tTmp = 0; }

            if ( iTmp > MAXMONEY ) { iTmp = MAXMONEY; }

            if ( tTmp > MAXMONEY ) { tTmp = MAXMONEY; }

            chrmoney[character] = iTmp;
            chrmoney[chraitarget[character]] = tTmp;
            break;

        case FDROPKEYS:
            drop_keys( character );
            break;

        case FIFLEADERISALIVE:
            // This function fails if there is no team leader
            returncode = ( teamleader[chrteam[character]] != NOLEADER );
            break;

        case FIFTARGETISOLDTARGET:
            // This function returns bfalse if the target has valuechanged
            returncode = ( chraitarget[character] == valueoldtarget );
            break;

        case FSETTARGETTOLEADER:

            // This function fails if there is no team leader
            if ( teamleader[chrteam[character]] == NOLEADER )
            {
                returncode = bfalse;
            }
            else
            {
                chraitarget[character] = teamleader[chrteam[character]];
            }
            break;

        case FSPAWNCHARACTER:
            // This function spawns a character, failing if x,y is invalid
            sTmp = spawn_one_character( valuetmpx, valuetmpy, 0, chrmodel[character], chrteam[character], 0, valuetmpturn, NULL, MAXCHR );
            returncode = bfalse;

            if ( sTmp < MAXCHR )
            {
                if ( __chrhitawall( sTmp ) )
                {
                    free_one_character( sTmp );
                }
                else
                {
                    tTmp = chrturnleftright[character] >> 2;
                    chrxvel[sTmp] += turntocos[( tTmp+8192 )&TRIG_TABLE_MASK] * valuetmpdistance;
                    chryvel[sTmp] += turntosin[( tTmp+8192 )&TRIG_TABLE_MASK] * valuetmpdistance;
                    chrpassage[sTmp] = chrpassage[character];
                    chriskursed[sTmp] = bfalse;
                    chraichild[character] = sTmp;
                    chraiowner[sTmp] = chraiowner[character];
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
            meshtile[chronwhichfan[character]] = valuetmpargument & ( 255 );
            break;

        case FIFUSED:
            // This function proceeds only if the character has been used
            returncode = ( ( chralert[character] & ALERTIFUSED ) != 0 );
            break;

        case FDROPMONEY:
            // This function drops some of a character's money
            drop_money( character, valuetmpargument );
            break;

        case FSETOLDTARGET:
            // This function sets the old target to the current target
            valueoldtarget = chraitarget[character];
            break;

        case FDETACHFROMHOLDER:

            // This function drops the character, failing only if it was not held
            if ( chrattachedto[character] != MAXCHR )
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
            returncode = ( capidsz[chrmodel[chraitarget[character]]][IDSZ_VULNERABILITY] == ( IDSZ ) valuetmpargument );
            break;

        case FCLEANUP:
            // This function issues the clean up order to all teammates
            issue_clean( character );
            break;

        case FIFCLEANEDUP:
            // This function proceeds only if the character was told to clean up
            returncode = ( ( chralert[character] & ALERTIFCLEANEDUP ) != 0 );
            break;

        case FIFSITTING:
            // This function proceeds if the character is riding another
            returncode = ( chrattachedto[character] != MAXCHR );
            break;

        case FIFTARGETISHURT:

            // This function passes only if the target is hurt and alive
            if ( !chralive[chraitarget[character]] || chrlife[chraitarget[character]] > chrlifemax[chraitarget[character]] - HURTDAMAGE )
                returncode = bfalse;

            break;

        case FIFTARGETISAPLAYER:
            // This function proceeds only if the target is a player ( may not be local )
            returncode = chrisplayer[chraitarget[character]];
            break;

        case FPLAYSOUND:

            // This function plays a sound
            if ( chroldz[character] > PITNOSOUND && valuetmpargument >= 0 && valuetmpargument < MAXWAVE )
            {
                play_mix( chroldx[character], chroldy[character], capwaveindex[chrmodel[character]] + valuetmpargument );
            }
            break;

        case FSPAWNPARTICLE:
            // This function spawns a particle
            tTmp = character;

            if ( chrattachedto[character] != MAXCHR )  tTmp = chrattachedto[character];

            tTmp = spawn_one_particle( chrxpos[character], chrypos[character], chrzpos[character], chrturnleftright[character], chrmodel[character], valuetmpargument, character, valuetmpdistance, chrteam[character], tTmp, 0, MAXCHR );

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
                    prtxpos[tTmp] = chrxpos[character];

                    if ( __prthitawall( tTmp ) )
                    {
                        prtypos[tTmp] = chrypos[character];
                    }
                }
            }
            break;

        case FIFTARGETISALIVE:
            // This function proceeds only if the target is alive
            returncode = chralive[chraitarget[character]];
            break;

        case FSTOP:
            chrmaxaccel[character] = 0;
            break;

        case FDISAFFIRMCHARACTER:
            disaffirm_attached_particles( character );
            break;

        case FREAFFIRMCHARACTER:
            reaffirm_attached_particles( character );
            break;

        case FIFTARGETISSELF:
            // This function proceeds only if the target is the character too
            returncode = ( chraitarget[character] == character );
            break;

        case FIFTARGETISMALE:
            // This function proceeds only if the target is male
            returncode = ( chrgender[chraitarget[character]] == GENMALE );
            break;

        case FIFTARGETISFEMALE:
            // This function proceeds only if the target is female
            returncode = ( chrgender[chraitarget[character]] == GENFEMALE );
            break;

        case FSETTARGETTOSELF:
            // This function sets the target to the character
            chraitarget[character] = character;
            break;

        case FSETTARGETTORIDER:

            // This function sets the target to the character's left/only grip weapon,
            // failing if there is none
            if ( chrholdingwhich[character][0] == MAXCHR )
            {
                returncode = bfalse;
            }
            else
            {
                chraitarget[character] = chrholdingwhich[character][0];
            }
            break;

        case FGETATTACKTURN:
            // This function sets tmpturn to the direction of the last attack
            valuetmpturn = chrdirectionlast[character];
            break;

        case FGETDAMAGETYPE:
            // This function gets the last type of damage
            valuetmpargument = chrdamagetypelast[character];
            break;

        case FBECOMESPELL:
            // This function turns the spellbook character into a spell based on its
            // content
            chrmoney[character] = ( chrtexture[character] - madskinstart[chrmodel[character]] ) & 3;
            change_character( character, chraicontent[character], 0, LEAVENONE );
            chraicontent[character] = 0;  // Reset so it doesn't mess up
            chraistate[character] = 0;  // Reset so it doesn't mess up
            valuechanged = btrue;
            break;

        case FBECOMESPELLBOOK:
            // This function turns the spell into a spellbook, and sets the content
            // accordingly
            chraicontent[character] = chrmodel[character];
            change_character( character, SPELLBOOK, chrmoney[character]&3, LEAVENONE );
            chraistate[character] = 0;  // Reset so it doesn't burn up
            valuechanged = btrue;
            break;

        case FIFSCOREDAHIT:
            // Proceed only if the character scored a hit
            returncode = ( ( chralert[character] & ALERTIFSCOREDAHIT ) != 0 );
            break;

        case FIFDISAFFIRMED:
            // Proceed only if the character was disaffirmed
            returncode = ( ( chralert[character] & ALERTIFDISAFFIRMED ) != 0 );
            break;

        case FTRANSLATEORDER:
            // This function gets the order and sets tmpx, tmpy, tmpargument and the
            // target ( if valid )
            sTmp = chrorder[character] >> 24;

            if ( sTmp < MAXCHR )
            {
                chraitarget[character] = sTmp;
            }

            valuetmpx = ( ( chrorder[character] >> 14 ) & 1023 ) << 6;
            valuetmpy = ( ( chrorder[character] >> 4 ) & 1023 ) << 6;
            valuetmpargument = chrorder[character] & 15;
            break;

        case FSETTARGETTOWHOEVERWASHIT:
            // This function sets the target to whoever the character hit last,
            chraitarget[character] = chrhitlast[character];
            break;

        case FSETTARGETTOWIDEENEMY:
            // This function finds an enemy, and proceeds only if there is one
            returncode = bfalse;

            if (get_target( character, WIDE, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse) != MAXCHR) returncode = btrue;

            break;

        case FIFCHANGED:
            // Proceed only if the character was polymorphed
            returncode = ( ( chralert[character] & ALERTIFCHANGED ) != 0 );
            break;

        case FIFINWATER:
            // Proceed only if the character got wet
            returncode = ( ( chralert[character] & ALERTIFINWATER ) != 0 );
            break;

        case FIFBORED:
            // Proceed only if the character is bored
            returncode = ( ( chralert[character] & ALERTIFBORED ) != 0 );
            break;

        case FIFTOOMUCHBAGGAGE:
            // Proceed only if the character tried to grab too much
            returncode = ( ( chralert[character] & ALERTIFTOOMUCHBAGGAGE ) != 0 );
            break;

        case FIFGROGGED:
            // Proceed only if the character was grogged
            returncode = ( ( chralert[character] & ALERTIFGROGGED ) != 0 );
            break;

        case FIFDAZED:
            // Proceed only if the character was dazed
            returncode = ( ( chralert[character] & ALERTIFDAZED ) != 0 );
            break;

        case FIFTARGETHASSPECIALID:
            // This function proceeds if ID matches tmpargument
            returncode = ( capidsz[chrmodel[chraitarget[character]]][IDSZ_SPECIAL] == ( IDSZ ) valuetmpargument );
            break;

        case FPRESSTARGETLATCHBUTTON:
            // This function sets the target's latch buttons
            chrlatchbutton[chraitarget[character]] = chrlatchbutton[chraitarget[character]] | valuetmpargument;
            break;

        case FIFINVISIBLE:
            // This function passes if the character is invisible
            returncode = ( chralpha[character] <= INVISIBLE ) || ( chrlight[character] <= INVISIBLE );
            break;

        case FIFARMORIS:
            // This function passes if the character's skin is tmpargument
            tTmp = chrtexture[character] - madskinstart[chrmodel[character]];
            returncode = ( tTmp == valuetmpargument );
            break;

        case FGETTARGETGROGTIME:
            // This function returns tmpargument as the grog time, and passes if it is not 0
            valuetmpargument = chrgrogtime[character];
            returncode = ( valuetmpargument != 0 );
            break;

        case FGETTARGETDAZETIME:
            // This function returns tmpargument as the daze time, and passes if it is not 0
            valuetmpargument = chrdazetime[character];
            returncode = ( valuetmpargument != 0 );
            break;

        case FSETDAMAGETYPE:
            // This function sets the bump damage type
            chrdamagetargettype[character] = valuetmpargument & ( DAMAGE_COUNT - 1 );
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
            sTmp = spawn_enchant( chraiowner[character], chraitarget[character], character, MAXENCHANT, MAXMODEL );
            returncode = ( sTmp != MAXENCHANT );
            break;

        case FENCHANTCHILD:
            // This function can be used with SpawnCharacter to enchant the
            // newly spawned character
            sTmp = spawn_enchant( chraiowner[character], chraichild[character], character, MAXENCHANT, MAXMODEL );
            returncode = ( sTmp != MAXENCHANT );
            break;

        case FTELEPORTTARGET:
            // This function teleports the target to the X, Y location, failing if the
            // location is off the map or blocked
            returncode = bfalse;

            if ( valuetmpx > EDGE && valuetmpy > EDGE && valuetmpx < meshedgex - EDGE && valuetmpy < meshedgey - EDGE )
            {
                // Yeah!  It worked!
                sTmp = chraitarget[character];
                detach_character_from_mount( sTmp, btrue, bfalse );
                chroldx[sTmp] = chrxpos[sTmp];
                chroldy[sTmp] = chrypos[sTmp];
                chrxpos[sTmp] = valuetmpx;
                chrypos[sTmp] = valuetmpy;
                chrzpos[sTmp] = valuetmpdistance;
                chrturnleftright[sTmp] = valuetmpturn;

                if ( __chrhitawall( sTmp ) )
                {
                    // No it didn't...
                    chrxpos[sTmp] = chroldx[sTmp];
                    chrypos[sTmp] = chroldy[sTmp];
                    chrzpos[sTmp] = chroldz[sTmp];
                    chrturnleftright[sTmp] = chroldturn[sTmp];
                    returncode = bfalse;
                }
                else
                {
                    chroldx[sTmp] = chrxpos[sTmp];
                    chroldy[sTmp] = chrypos[sTmp];
                    chroldz[sTmp] = chrzpos[sTmp];
                    chroldturn[sTmp] = chrturnleftright[sTmp];
                    returncode = btrue;
                }
            }
            break;

        case FGIVEEXPERIENCETOTARGET:
            // This function gives the target some experience, xptype from distance,
            // amount from argument...
            give_experience( chraitarget[character], valuetmpargument, valuetmpdistance );
            break;

        case FINCREASEAMMO:

            // This function increases the ammo by one
            if ( chrammo[character] < chrammomax[character] )
            {
                chrammo[character]++;
            }
            break;

        case FUNKURSETARGET:
            // This function unkurses the target
            chriskursed[chraitarget[character]] = bfalse;
            break;

        case FGIVEEXPERIENCETOTARGETTEAM:
            // This function gives experience to everyone on the target's team
            give_team_experience( chrteam[chraitarget[character]], valuetmpargument, valuetmpdistance );
            break;

        case FIFUNARMED:
            // This function proceeds if the character has no item in hand
            returncode = ( chrholdingwhich[character][0] == MAXCHR && chrholdingwhich[character][1] == MAXCHR );
            break;

        case FRESTOCKTARGETAMMOIDALL:
            // This function restocks the ammo of every item the character is holding,
            // if the item matches the ID given ( parent or child type )
            iTmp = 0;  // Amount of ammo given
            sTmp = chrholdingwhich[chraitarget[character]][0];
            iTmp += restock_ammo( sTmp, valuetmpargument );
            sTmp = chrholdingwhich[chraitarget[character]][1];
            iTmp += restock_ammo( sTmp, valuetmpargument );
            sTmp = chrnextinpack[chraitarget[character]];

            while ( sTmp != MAXCHR )
            {
                iTmp += restock_ammo( sTmp, valuetmpargument );
                sTmp = chrnextinpack[sTmp];
            }

            valuetmpargument = iTmp;
            returncode = ( iTmp != 0 );
            break;

        case FRESTOCKTARGETAMMOIDFIRST:
            // This function restocks the ammo of the first item the character is holding,
            // if the item matches the ID given ( parent or child type )
            iTmp = 0;  // Amount of ammo given
            sTmp = chrholdingwhich[chraitarget[character]][0];
            iTmp += restock_ammo( sTmp, valuetmpargument );

            if ( iTmp == 0 )
            {
                sTmp = chrholdingwhich[chraitarget[character]][1];
                iTmp += restock_ammo( sTmp, valuetmpargument );

                if ( iTmp == 0 )
                {
                    sTmp = chrnextinpack[chraitarget[character]];

                    while ( sTmp != MAXCHR && iTmp == 0 )
                    {
                        iTmp += restock_ammo( sTmp, valuetmpargument );
                        sTmp = chrnextinpack[sTmp];
                    }
                }
            }

            valuetmpargument = iTmp;
            returncode = ( iTmp != 0 );
            break;

        case FFLASHTARGET:
            // This function flashes the character
            flash_character( chraitarget[character], 255 );
            break;

        case FSETREDSHIFT:
            // This function alters a character's coloration
            chrredshift[character] = valuetmpargument;
            break;

        case FSETGREENSHIFT:
            // This function alters a character's coloration
            chrgrnshift[character] = valuetmpargument;
            break;

        case FSETBLUESHIFT:
            // This function alters a character's coloration
            chrblushift[character] = valuetmpargument;
            break;

        case FSETLIGHT:
            // This function alters a character's transparency
            chrlight[character] = valuetmpargument;
            break;

        case FSETALPHA:
            // This function alters a character's transparency
            chralpha[character] = valuetmpargument;
            break;

        case FIFHITFROMBEHIND:
            // This function proceeds if the character was attacked from behind
            returncode = bfalse;

            if ( chrdirectionlast[character] >= BEHIND - 8192 && chrdirectionlast[character] < BEHIND + 8192 )
                returncode = btrue;

            break;

        case FIFHITFROMFRONT:
            // This function proceeds if the character was attacked from the front
            returncode = bfalse;

            if ( chrdirectionlast[character] >= 49152 + 8192 || chrdirectionlast[character] < FRONT + 8192 )
                returncode = btrue;

            break;

        case FIFHITFROMLEFT:
            // This function proceeds if the character was attacked from the left
            returncode = bfalse;

            if ( chrdirectionlast[character] >= LEFT - 8192 && chrdirectionlast[character] < LEFT + 8192 )
                returncode = btrue;

            break;

        case FIFHITFROMRIGHT:
            // This function proceeds if the character was attacked from the right
            returncode = bfalse;

            if ( chrdirectionlast[character] >= RIGHT - 8192 && chrdirectionlast[character] < RIGHT + 8192 )
                returncode = btrue;

            break;

        case FIFTARGETISONSAMETEAM:
            // This function proceeds only if the target is on another team
            returncode = bfalse;

            if ( chrteam[chraitarget[character]] == chrteam[character] )
                returncode = btrue;

            break;

        case FKILLTARGET:
            // This function kills the target
            kill_character( chraitarget[character], character );
            break;

        case FUNDOENCHANT:
            // This function undoes the last enchant
            returncode = ( chrundoenchant[character] != MAXENCHANT );
            remove_enchant( chrundoenchant[character] );
            break;

        case FGETWATERLEVEL:
            // This function gets the douse level for the water, returning it in tmpargument
            valuetmpargument = waterdouselevel * 10;
            break;

        case FCOSTTARGETMANA:
            // This function costs the target some mana
            returncode = cost_mana( chraitarget[character], valuetmpargument, character );
            break;

        case FIFTARGETHASANYID:
            // This function proceeds only if one of the target's IDSZ's matches tmpargument
            returncode = 0;
            tTmp = 0;

            while ( tTmp < IDSZ_COUNT )
            {
                returncode |= ( capidsz[chrmodel[chraitarget[character]]][tTmp] == ( IDSZ ) valuetmpargument );
                tTmp++;
            }
            break;

        case FSETBUMPSIZE:
            // This function sets the character's bump size
            fTmp = chrbumpsizebig[character];
            fTmp = fTmp / chrbumpsize[character];  // 1.5f or 2.0f
            chrbumpsize[character] = valuetmpargument * chrfat[character];
            chrbumpsizebig[character] = fTmp * chrbumpsize[character];
            chrbumpsizesave[character] = valuetmpargument;
            chrbumpsizebigsave[character] = fTmp * chrbumpsizesave[character];
            break;

        case FIFNOTDROPPED:
            // This function passes if a kursed item could not be dropped
            returncode = ( ( chralert[character] & ALERTIFNOTDROPPED ) != 0 );
            break;

        case FIFYISLESSTHANX:
            // This function passes only if tmpy is less than tmpx
            returncode = ( valuetmpy < valuetmpx );
            break;

        case FSETFLYHEIGHT:
            // This function sets a character's fly height
            chrflyheight[character] = valuetmpargument;
            break;

        case FIFBLOCKED:
            // This function passes if the character blocked an attack
            returncode = ( ( chralert[character] & ALERTIFBLOCKED ) != 0 );
            break;

        case FIFTARGETISDEFENDING:
            returncode = ( chraction[chraitarget[character]] >= ACTIONPA && chraction[chraitarget[character]] <= ACTIONPD );
            break;

        case FIFTARGETISATTACKING:
            returncode = ( chraction[chraitarget[character]] >= ACTIONUA && chraction[chraitarget[character]] <= ACTIONFD );
            break;

        case FIFSTATEIS0:
            returncode = ( 0 == chraistate[character] );
            break;

        case FIFSTATEIS1:
            returncode = ( 1 == chraistate[character] );
            break;

        case FIFSTATEIS2:
            returncode = ( 2 == chraistate[character] );
            break;

        case FIFSTATEIS3:
            returncode = ( 3 == chraistate[character] );
            break;

        case FIFSTATEIS4:
            returncode = ( 4 == chraistate[character] );
            break;

        case FIFSTATEIS5:
            returncode = ( 5 == chraistate[character] );
            break;

        case FIFSTATEIS6:
            returncode = ( 6 == chraistate[character] );
            break;

        case FIFSTATEIS7:
            returncode = ( 7 == chraistate[character] );
            break;

        case FIFCONTENTIS:
            returncode = ( valuetmpargument == chraicontent[character] );
            break;

        case FSETTURNMODETOWATCHTARGET:
            // This function sets the turn mode
            chrturnmode[character] = TURNMODEWATCHTARGET;
            break;

        case FIFSTATEISNOT:
            returncode = ( valuetmpargument != chraistate[character] );
            break;

        case FIFXISEQUALTOY:
            returncode = ( valuetmpx == valuetmpy );
            break;

        case FDEBUGMESSAGE:
            // This function spits out a debug message
            sprintf( cTmp, "aistate %d, aicontent %d, target %d", chraistate[character], chraicontent[character], chraitarget[character] );
            debug_message( cTmp );
            sprintf( cTmp, "tmpx %d, tmpy %d", valuetmpx, valuetmpy );
            debug_message( cTmp );
            sprintf( cTmp, "tmpdistance %d, tmpturn %d", valuetmpdistance, valuetmpturn );
            debug_message( cTmp );
            sprintf( cTmp, "tmpargument %d, selfturn %d", valuetmpargument, chrturnleftright[character] );
            debug_message( cTmp );
            break;

        case FBLACKTARGET:
            // This function makes the target flash black
            flash_character( chraitarget[character], 0 );
            break;

        case FSENDMESSAGENEAR:
            // This function sends a message if the camera is in the nearby area.
            iTmp = ABS( chroldx[character] - camtrackx ) + ABS( chroldy[character] - camtracky );

            if ( iTmp < MSGDISTANCE )
                display_message( madmsgstart[chrmodel[character]] + valuetmpargument, character );

            break;

        case FIFHITGROUND:
            // This function passes if the character just hit the ground
            returncode = ( ( chralert[character] & ALERTIFHITGROUND ) != 0 );
            break;

        case FIFNAMEISKNOWN:
            // This function passes if the character's name is known
            returncode = chrnameknown[character];
            break;

        case FIFUSAGEISKNOWN:
            // This function passes if the character's usage is known
            returncode = capusageknown[chrmodel[character]];
            break;

        case FIFHOLDINGITEMID:
            // This function passes if the character is holding an item with the IDSZ given
            // in tmpargument, returning the latch to press to use it
            returncode = bfalse;
            // Check left hand
            sTmp = chrholdingwhich[character][0];

            if ( sTmp != MAXCHR )
            {
                sTmp = chrmodel[sTmp];

                if ( capidsz[sTmp][IDSZ_PARENT] == ( IDSZ ) valuetmpargument || capidsz[sTmp][IDSZ_TYPE] == ( Uint32 ) valuetmpargument )
                {
                    valuetmpargument = LATCHBUTTONLEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            sTmp = chrholdingwhich[character][1];

            if ( sTmp != MAXCHR )
            {
                sTmp = chrmodel[sTmp];

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
            tTmp = chrholdingwhich[character][0];

            if ( tTmp != MAXCHR )
            {
                sTmp = chrmodel[tTmp];

                if ( capisranged[sTmp] && ( chrammomax[tTmp] == 0 || ( chrammo[tTmp] != 0 && chrammoknown[tTmp] ) ) )
                {
                    valuetmpargument = LATCHBUTTONLEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            tTmp = chrholdingwhich[character][1];

            if ( tTmp != MAXCHR )
            {
                sTmp = chrmodel[tTmp];

                if ( capisranged[sTmp] && ( chrammomax[tTmp] == 0 || ( chrammo[tTmp] != 0 && chrammoknown[tTmp] ) ) )
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
            sTmp = chrholdingwhich[character][0];

            if ( sTmp != MAXCHR )
            {
                sTmp = chrmodel[sTmp];

                if ( !capisranged[sTmp] && capweaponaction[sTmp] != ACTIONPA )
                {
                    valuetmpargument = LATCHBUTTONLEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            sTmp = chrholdingwhich[character][1];

            if ( sTmp != MAXCHR )
            {
                sTmp = chrmodel[sTmp];

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
            sTmp = chrholdingwhich[character][0];

            if ( sTmp != MAXCHR )
            {
                sTmp = chrmodel[sTmp];

                if ( capweaponaction[sTmp] == ACTIONPA )
                {
                    valuetmpargument = LATCHBUTTONLEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            sTmp = chrholdingwhich[character][1];

            if ( sTmp != MAXCHR )
            {
                sTmp = chrmodel[sTmp];

                if ( capweaponaction[sTmp] == ACTIONPA )
                {
                    valuetmpargument = LATCHBUTTONRIGHT;
                    returncode = btrue;
                }
            }
            break;

        case FIFKURSED:
            // This function passes if the character is kursed
            returncode = chriskursed[character];
            break;

        case FIFTARGETISKURSED:
            // This function passes if the target is kursed
            returncode = chriskursed[chraitarget[character]];
            break;

        case FIFTARGETISDRESSEDUP:
            // This function passes if the character's skin is dressy
            iTmp = chrtexture[character] - madskinstart[chrmodel[character]];
            iTmp = 1 << iTmp;
            returncode = ( ( capskindressy[chrmodel[character]] & iTmp ) != 0 );
            break;

        case FIFOVERWATER:
            // This function passes if the character is on a water tile
            returncode = ( ( meshfx[chronwhichfan[character]] & MESHFXWATER ) != 0 && wateriswater );
            break;

        case FIFTHROWN:
            // This function passes if the character was thrown
            returncode = ( ( chralert[character] & ALERTIFTHROWN ) != 0 );
            break;

        case FMAKENAMEKNOWN:
            // This function makes the name of an item/character known.
            chrnameknown[character] = btrue;
//            chricon[character] = btrue;
            break;

        case FMAKEUSAGEKNOWN:
            // This function makes the usage of an item known...  For XP gains from
            // using an unknown potion or such
            capusageknown[chrmodel[character]] = btrue;
            break;

        case FSTOPTARGETMOVEMENT:
            // This function makes the target stop moving temporarily
            chrxvel[chraitarget[character]] = 0;
            chryvel[chraitarget[character]] = 0;

            if ( chrzvel[chraitarget[character]] > 0 ) chrzvel[chraitarget[character]] = gravity;

            break;

        case FSETXY:
            // This function stores tmpx and tmpy in the storage array
            chraix[character][valuetmpargument&STORAND] = valuetmpx;
            chraiy[character][valuetmpargument&STORAND] = valuetmpy;
            break;

        case FGETXY:
            // This function gets previously stored data, setting tmpx and tmpy
            valuetmpx = chraix[character][valuetmpargument&STORAND];
            valuetmpy = chraiy[character][valuetmpargument&STORAND];
            break;

        case FADDXY:
            // This function adds tmpx and tmpy to the storage array
            chraix[character][valuetmpargument&STORAND] += valuetmpx;
            chraiy[character][valuetmpargument&STORAND] += valuetmpy;
            break;

        case FMAKEAMMOKNOWN:
            // This function makes the ammo of an item/character known.
            chrammoknown[character] = btrue;
            break;

        case FSPAWNATTACHEDPARTICLE:
            // This function spawns an attached particle
            tTmp = character;

            if ( chrattachedto[character] != MAXCHR )  tTmp = chrattachedto[character];

            tTmp = spawn_one_particle( chrxpos[character], chrypos[character], chrzpos[character], chrturnleftright[character], chrmodel[character], valuetmpargument, character, valuetmpdistance, chrteam[character], tTmp, 0, MAXCHR );
            break;

        case FSPAWNEXACTPARTICLE:
            // This function spawns an exactly placed particle
            tTmp = character;

            if ( chrattachedto[character] != MAXCHR )  tTmp = chrattachedto[character];

            spawn_one_particle( valuetmpx, valuetmpy, valuetmpdistance, chrturnleftright[character], chrmodel[character], valuetmpargument, MAXCHR, 0, chrteam[character], tTmp, 0, MAXCHR );
            break;

        case FACCELERATETARGET:
            // This function changes the target's speeds
            chrxvel[chraitarget[character]] += valuetmpx;
            chryvel[chraitarget[character]] += valuetmpy;
            break;

        case FIFDISTANCEISMORETHANTURN:
            // This function proceeds tmpdistance is greater than tmpturn
            returncode = ( valuetmpdistance > ( int ) valuetmpturn );
            break;

        case FIFCRUSHED:
            // This function proceeds only if the character was crushed
            returncode = ( ( chralert[character] & ALERTIFCRUSHED ) != 0 );
            break;

        case FMAKECRUSHVALID:
            // This function makes doors able to close on this object
            chrcanbecrushed[character] = btrue;
            break;

        case FSETTARGETTOLOWESTTARGET:

            // This sets the target to whatever the target is being held by,
            // The lowest in the set.  This function never fails
            while ( chrattachedto[chraitarget[character]] != MAXCHR )
            {
                chraitarget[character] = chrattachedto[chraitarget[character]];
            }
            break;

        case FIFNOTPUTAWAY:
            // This function proceeds only if the character couln't be put in the pack
            returncode = ( ( chralert[character] & ALERTIFNOTPUTAWAY ) != 0 );
            break;

        case FIFTAKENOUT:
            // This function proceeds only if the character was taken out of the pack
            returncode = ( ( chralert[character] & ALERTIFTAKENOUT ) != 0 );
            break;

        case FIFAMMOOUT:
            // This function proceeds only if the character has no ammo
            returncode = ( chrammo[character] == 0 );
            break;

        case FPLAYSOUNDLOOPED:
            // This function plays a looped sound
            if ( moduleactive )
            {
                // You could use this, but right now there's no way to stop the sound later, so it's better not to start it
                // play_sound_pvf_looped(capwaveindex[chrmodel[character]][valuetmpargument], PANMID, volume, valuetmpdistance);
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
            if ( chralive[character] )
            {
                iTmp = chrlife[character] + valuetmpargument;

                if ( iTmp > chrlifemax[character] ) iTmp = chrlifemax[character];

                if ( iTmp < 1 ) iTmp = 1;

                chrlife[character] = iTmp;
            }
            break;

        case FEQUIP:
            // This function flags the character as being equipped
            chrisequipped[character] = btrue;
            break;

        case FIFTARGETHASITEMIDEQUIPPED:
            // This function proceeds if the target has a matching item equipped
            returncode = bfalse;
            sTmp = chrnextinpack[chraitarget[character]];

            while ( sTmp != MAXCHR )
            {
                if ( sTmp != character && chrisequipped[sTmp] && ( capidsz[chrmodel[sTmp]][IDSZ_PARENT] == ( Uint32 ) valuetmpargument || capidsz[chrmodel[sTmp]][IDSZ_TYPE] == ( Uint32 ) valuetmpargument ) )
                {
                    returncode = btrue;
                    sTmp = MAXCHR;
                }
                else
                {
                    sTmp = chrnextinpack[sTmp];
                }
            }
            break;

        case FSETOWNERTOTARGET:
            // This function sets the owner
            chraiowner[character] = chraitarget[character];
            break;

        case FSETTARGETTOOWNER:
            // This function sets the target to the owner
            chraitarget[character] = chraiowner[character];
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
            if ( valuetmpargument > 0 ) chrreloadtime[character] = valuetmpargument;
            else chrreloadtime[character] = 0;

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

            if ( !chrisplayer[chraitarget[character]] )
            {
                returncode = btrue;

                if ( chraitarget[character] == character )
                {
                    // Poof self later
                    valuegopoof = btrue;
                }
                else
                {
                    // Poof others now
                    if ( chrattachedto[chraitarget[character]] != MAXCHR )
                        detach_character_from_mount( chraitarget[character], btrue, bfalse );

                    if ( chrholdingwhich[chraitarget[character]][0] != MAXCHR )
                        detach_character_from_mount( chrholdingwhich[chraitarget[character]][0], btrue, bfalse );

                    if ( chrholdingwhich[chraitarget[character]][1] != MAXCHR )
                        detach_character_from_mount( chrholdingwhich[chraitarget[character]][1], btrue, bfalse );

                    free_inventory( chraitarget[character] );
                    free_one_character( chraitarget[character] );
                    chraitarget[character] = character;
                }
            }
            break;

        case FCHILDDOACTIONOVERRIDE:
            // This function starts a new action, if it is valid for the model
            // It will fail if the action is invalid
            returncode = bfalse;

            if ( valuetmpargument < MAXACTION )
            {
                if ( madactionvalid[chrmodel[chraichild[character]]][valuetmpargument] )
                {
                    chraction[chraichild[character]] = valuetmpargument;
                    chrlip[chraichild[character]] = 0;
                    chrframe[chraichild[character]] = madactionstart[chrmodel[chraichild[character]]][valuetmpargument];
                    chrlastframe[chraichild[character]] = chrframe[chraichild[character]];
                    chractionready[chraichild[character]] = bfalse;
                    returncode = btrue;
                }
            }
            break;

        case FSPAWNPOOF:
            // This function makes a lovely little poof at the character's location
            spawn_poof( character, chrmodel[character] );
            break;

        case FSETSPEEDPERCENT:
            reset_character_accel( character );
            chrmaxaccel[character] = chrmaxaccel[character] * valuetmpargument / 100.0f;
            break;

        case FSETCHILDSTATE:
            // This function sets the child's state
            chraistate[chraichild[character]] = valuetmpargument;
            break;

        case FSPAWNATTACHEDSIZEDPARTICLE:
            // This function spawns an attached particle, then sets its size
            tTmp = character;

            if ( chrattachedto[character] != MAXCHR )  tTmp = chrattachedto[character];

            tTmp = spawn_one_particle( chrxpos[character], chrypos[character], chrzpos[character], chrturnleftright[character], chrmodel[character], valuetmpargument, character, valuetmpdistance, chrteam[character], tTmp, 0, MAXCHR );

            if ( tTmp < maxparticles )
            {
                prtsize[tTmp] = valuetmpturn;
            }
            break;

        case FCHANGEARMOR:
            // This function sets the character's armor type and returns the old type
            // as tmpargument and the new type as tmpx
            valuetmpx = valuetmpargument;
            iTmp = chrtexture[character] - madskinstart[chrmodel[character]];
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
            sTmp = ATAN2( chrypos[chraitarget[character]] - chrypos[character], chrxpos[chraitarget[character]] - chrxpos[character] ) * 65535 / ( TWO_PI );
            sTmp += 32768 - chrturnleftright[character];
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
                    iTmp = play_mix( chroldx[character], chroldy[character], capwaveindex[chrmodel[character]] + valuetmpargument );
                }

                if ( -1 != iTmp )
                {
                    if ( MIX_SND == capwaveindex[chrmodel[character]][valuetmpargument].type )
                    {
                        Mix_Volume( iTmp, valuetmpdistance );
                    }
                    else if ( MIX_MUS == capwaveindex[chrmodel[character]][valuetmpargument].type )
                    {
                        Mix_VolumeMusic( valuetmpdistance );
                    }
                }
            }
            break;

        case FSPAWNATTACHEDFACEDPARTICLE:
            // This function spawns an attached particle with facing
            tTmp = character;

            if ( chrattachedto[character] != MAXCHR )  tTmp = chrattachedto[character];

            tTmp = spawn_one_particle( chrxpos[character], chrypos[character], chrzpos[character], valuetmpturn, chrmodel[character], valuetmpargument, character, valuetmpdistance, chrteam[character], tTmp, 0, MAXCHR );
            break;

        case FIFSTATEISODD:
            returncode = ( chraistate[character] & 1 );
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
                chroldx[character] = chrxpos[character];
                chroldy[character] = chrypos[character];
                chrxpos[character] = valuetmpx;
                chrypos[character] = valuetmpy;

                if ( __chrhitawall( character ) )
                {
                    // No it didn't...
                    chrxpos[character] = chroldx[character];
                    chrypos[character] = chroldy[character];
                    returncode = bfalse;
                }
                else
                {
                    chroldx[character] = chrxpos[character];
                    chroldy[character] = chrypos[character];
                    returncode = btrue;
                }
            }
            break;

        case FGIVESTRENGTHTOTARGET:

            // Permanently boost the target's strength
            if ( chralive[chraitarget[character]] )
            {
                iTmp = valuetmpargument;
                getadd( 0, chrstrength[chraitarget[character]], PERFECTSTAT, &iTmp );
                chrstrength[chraitarget[character]] += iTmp;
            }
            break;

        case FGIVEWISDOMTOTARGET:

            // Permanently boost the target's wisdom
            if ( chralive[chraitarget[character]] )
            {
                iTmp = valuetmpargument;
                getadd( 0, chrwisdom[chraitarget[character]], PERFECTSTAT, &iTmp );
                chrwisdom[chraitarget[character]] += iTmp;
            }
            break;

        case FGIVEINTELLIGENCETOTARGET:

            // Permanently boost the target's intelligence
            if ( chralive[chraitarget[character]] )
            {
                iTmp = valuetmpargument;
                getadd( 0, chrintelligence[chraitarget[character]], PERFECTSTAT, &iTmp );
                chrintelligence[chraitarget[character]] += iTmp;
            }
            break;

        case FGIVEDEXTERITYTOTARGET:

            // Permanently boost the target's dexterity
            if ( chralive[chraitarget[character]] )
            {
                iTmp = valuetmpargument;
                getadd( 0, chrdexterity[chraitarget[character]], PERFECTSTAT, &iTmp );
                chrdexterity[chraitarget[character]] += iTmp;
            }
            break;

        case FGIVELIFETOTARGET:

            // Permanently boost the target's life
            if ( chralive[chraitarget[character]] )
            {
                iTmp = valuetmpargument;
                getadd( LOWSTAT, chrlifemax[chraitarget[character]], PERFECTBIG, &iTmp );
                chrlifemax[chraitarget[character]] += iTmp;

                if ( iTmp < 0 )
                {
                    getadd( 1, chrlife[chraitarget[character]], PERFECTBIG, &iTmp );
                }

                chrlife[chraitarget[character]] += iTmp;
            }
            break;

        case FGIVEMANATOTARGET:

            // Permanently boost the target's mana
            if ( chralive[chraitarget[character]] )
            {
                iTmp = valuetmpargument;
                getadd( 0, chrmanamax[chraitarget[character]], PERFECTBIG, &iTmp );
                chrmanamax[chraitarget[character]] += iTmp;

                if ( iTmp < 0 )
                {
                    getadd( 0, chrmana[chraitarget[character]], PERFECTBIG, &iTmp );
                }

                chrmana[chraitarget[character]] += iTmp;
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
            if ( chralive[chraitarget[character]] )
            {
                iTmp = valuetmpargument;
                getadd( 1, chrlife[chraitarget[character]], chrlifemax[chraitarget[character]], &iTmp );
                chrlife[chraitarget[character]] += iTmp;
                // Check all enchants to see if they are removed
                iTmp = chrfirstenchant[chraitarget[character]];

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
            if ( chralive[chraitarget[character]] )
            {
                iTmp = valuetmpargument;
                getadd( 0, chrmana[chraitarget[character]], chrmanamax[chraitarget[character]], &iTmp );
                chrmana[chraitarget[character]] += iTmp;
            }
            break;

        case FCOSTAMMO:

            // Take away one ammo
            if ( chrammo[character] > 0 )
            {
                chrammo[character]--;
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
                    if ( capidsz[chrmodel[character]][tTmp] != capidsz[chrmodel[iTmp]][tTmp] )
                    {
                        sTmp = bfalse;
                    }

                    tTmp++;
                }

                if ( sTmp )
                {
                    chrnameknown[iTmp] = btrue;
                }

                iTmp++;
            }
            break;

        case FSPAWNATTACHEDHOLDERPARTICLE:
            // This function spawns an attached particle, attached to the holder
            tTmp = character;

            if ( chrattachedto[character] != MAXCHR )  tTmp = chrattachedto[character];

            tTmp = spawn_one_particle( chrxpos[character], chrypos[character], chrzpos[character], chrturnleftright[character], chrmodel[character], valuetmpargument, tTmp, valuetmpdistance, chrteam[character], tTmp, 0, MAXCHR );
            break;

        case FSETTARGETRELOADTIME:

            // This function sets the target's reload time
            if ( valuetmpargument > 0 )
                chrreloadtime[chraitarget[character]] = valuetmpargument;
            else chrreloadtime[chraitarget[character]] = 0;

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
            if ( chrattachedto[character] != MAXCHR )
            {
                if ( chrinwhichhand[character] == GRIPLEFT )
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

            if ( chrattachedto[chraitarget[character]] != MAXCHR )
            {
                returncode = chrismount[chrattachedto[chraitarget[character]]];
            }
            break;

        case FSPARKLEICON:

            // This function makes a blippie thing go around the icon
            if ( valuetmpargument < NUMBAR && valuetmpargument > -1 )
            {
                chrsparkle[character] = valuetmpargument;
            }
            break;

        case FUNSPARKLEICON:
            // This function stops the blippie thing
            chrsparkle[character] = NOSPARKLE;
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
            chrshadowsize[character] = valuetmpargument * chrfat[character];
            chrshadowsizesave[character] = valuetmpargument;
            break;

        case FORDERTARGET:
            // This function orders one specific character...  The target
            // Be careful in using this, always checking IDSZ first
            chrorder[chraitarget[character]] = valuetmpargument;
            chrcounter[chraitarget[character]] = 0;
            chralert[chraitarget[character]] |= ALERTIFORDERED;
            break;

        case FSETTARGETTOWHOEVERISINPASSAGE:
            // This function lets passage rectangles be used as event triggers
            sTmp = who_is_blocking_passage( valuetmpargument );
            returncode = bfalse;

            if ( sTmp != MAXCHR )
            {
                chraitarget[character] = sTmp;
                returncode = btrue;
            }
            break;

        case FIFCHARACTERWASABOOK:
            // This function proceeds if the base model is the same as the current
            // model or if the base model is SPELLBOOK
            returncode = ( chrbasemodel[character] == SPELLBOOK ||
                           chrbasemodel[character] == chrmodel[character] );
            break;

        case FSETENCHANTBOOSTVALUES:
            // This function sets the boost values for the last enchantment
            iTmp = chrundoenchant[character];

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
            sTmp = spawn_one_character( valuetmpx, valuetmpy, valuetmpdistance, chrmodel[character], chrteam[character], 0, valuetmpturn, NULL, MAXCHR );
            returncode = bfalse;

            if ( sTmp < MAXCHR )
            {
                if ( __chrhitawall( sTmp ) )
                {
                    free_one_character( sTmp );
                }
                else
                {
                    chriskursed[sTmp] = bfalse;
                    chraichild[character] = sTmp;
                    chrpassage[sTmp] = chrpassage[character];
                    chraiowner[sTmp] = chraiowner[character];
                    returncode = btrue;
                }
            }
            break;

        case FSPAWNEXACTCHARACTERXYZ:
            // This function spawns a character ( specific model slot ),
            // failing if x,y,z is invalid
            sTmp = spawn_one_character( valuetmpx, valuetmpy, valuetmpdistance, valuetmpargument, chrteam[character], 0, valuetmpturn, NULL, MAXCHR );
            returncode = bfalse;

            if ( sTmp < MAXCHR )
            {
                if ( __chrhitawall( sTmp ) )
                {
                    free_one_character( sTmp );
                }
                else
                {
                    chriskursed[sTmp] = bfalse;
                    chraichild[character] = sTmp;
                    chrpassage[sTmp] = chrpassage[character];
                    chraiowner[sTmp] = chraiowner[character];
                    returncode = btrue;
                }
            }
            break;

        case FCHANGETARGETCLASS:
            // This function changes a character's model ( specific model slot )
            change_character( chraitarget[character], valuetmpargument, 0, LEAVEALL );
            break;

        case FPLAYFULLSOUND:

            // This function plays a sound loud for everyone...  Victory music
            if ( moduleactive && valuetmpargument >= 0 && valuetmpargument < MAXWAVE )
            {
                play_mix( camtrackx, camtracky, capwaveindex[chrmodel[character]] + valuetmpargument );
            }
            break;

        case FSPAWNEXACTCHASEPARTICLE:
            // This function spawns an exactly placed particle that chases the target
            tTmp = character;

            if ( chrattachedto[character] != MAXCHR )  tTmp = chrattachedto[character];

            tTmp = spawn_one_particle( valuetmpx, valuetmpy, valuetmpdistance, chrturnleftright[character], chrmodel[character], valuetmpargument, MAXCHR, 0, chrteam[character], tTmp, 0, MAXCHR );

            if ( tTmp < maxparticles )
            {
                prttarget[tTmp] = chraitarget[character];
            }
            break;

        case FCREATEORDER:
            // This function packs up an order, using tmpx, tmpy, tmpargument and the
            // target ( if valid ) to create a new tmpargument
            sTmp = chraitarget[character] << 24;
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
            sTmp = chrholdingwhich[chraitarget[character]][0];
            chriskursed[sTmp] = bfalse;
            sTmp = chrholdingwhich[chraitarget[character]][1];
            chriskursed[sTmp] = bfalse;
            sTmp = chrnextinpack[chraitarget[character]];

            while ( sTmp != MAXCHR )
            {
                chriskursed[sTmp] = bfalse;
                sTmp = chrnextinpack[sTmp];
            }
            break;

        case FIFTARGETISSNEAKING:
            // This function proceeds if the target is doing ACTIONDA or ACTIONWA
            returncode = ( chraction[chraitarget[character]] == ACTIONDA || chraction[chraitarget[character]] == ACTIONWA );
            break;

        case FDROPITEMS:
            // This function drops all of the character's items
            drop_all_items( character );
            break;

        case FRESPAWNTARGET:
            // This function respawns the target at its current location
            sTmp = chraitarget[character];
            chroldx[sTmp] = chrxpos[sTmp];
            chroldy[sTmp] = chrypos[sTmp];
            chroldz[sTmp] = chrzpos[sTmp];
            respawn_character( sTmp );
            chrxpos[sTmp] = chroldx[sTmp];
            chrypos[sTmp] = chroldy[sTmp];
            chrzpos[sTmp] = chroldz[sTmp];
            break;

        case FTARGETDOACTIONSETFRAME:
            // This function starts a new action, if it is valid for the model and
            // sets the starting frame.  It will fail if the action is invalid
            returncode = bfalse;

            if ( valuetmpargument < MAXACTION )
            {
                if ( madactionvalid[chrmodel[chraitarget[character]]][valuetmpargument] )
                {
                    chraction[chraitarget[character]] = valuetmpargument;
                    chrlip[chraitarget[character]] = 0;
                    chrframe[chraitarget[character]] = madactionstart[chrmodel[chraitarget[character]]][valuetmpargument];
                    chrlastframe[chraitarget[character]] = chrframe[chraitarget[character]];
                    chractionready[chraitarget[character]] = bfalse;
                    returncode = btrue;
                }
            }
            break;

        case FIFTARGETCANSEEINVISIBLE:
            // This function proceeds if the target can see invisible
            returncode = chrcanseeinvisible[chraitarget[character]];
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
            sTmp = chrattachedto[character];

            if ( sTmp != MAXCHR )
            {
                returncode = ( chrholdingwhich[sTmp][0] == character );
            }
            break;

        case FNOTANITEM:
            // This function makes the character a non-item character
            chrisitem[character] = bfalse;
            break;

        case FSETCHILDAMMO:
            // This function sets the child's ammo
            chrammo[chraichild[character]] = valuetmpargument;
            break;

        case FIFHITVULNERABLE:
            // This function proceeds if the character was hit by a weapon with the
            // correct vulnerability IDSZ...  [SILV] for Werewolves...
            returncode = ( ( chralert[character] & ALERTIFHITVULNERABLE ) != 0 );
            break;

        case FIFTARGETISFLYING:
            // This function proceeds if the character target is flying
            returncode = ( chrflyheight[chraitarget[character]] > 0 );
            break;

        case FIDENTIFYTARGET:
            // This function reveals the target's name, ammo, and usage
            // Proceeds if the target was unknown
            returncode = bfalse;
            sTmp = chraitarget[character];

            if ( chrammomax[sTmp] != 0 )  chrammoknown[sTmp] = btrue;

            if ( chrname[sTmp][0] != 'B' ||
                    chrname[sTmp][1] != 'l' ||
                    chrname[sTmp][2] != 'a' ||
                    chrname[sTmp][3] != 'h' ||
                    chrname[sTmp][4] != 0 )
            {
                returncode = !chrnameknown[sTmp];
                chrnameknown[sTmp] = btrue;
            }

            capusageknown[chrmodel[sTmp]] = btrue;
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
            valuetmpargument = chraistate[chraitarget[character]];
            break;

        case FIFEQUIPPED:
            // This proceeds if the character is equipped
            returncode = bfalse;

            if ( chrisequipped[character] ) returncode = btrue;

            break;

        case FDROPTARGETMONEY:
            // This function drops some of the target's money
            drop_money( chraitarget[character], valuetmpargument );
            break;

        case FGETTARGETCONTENT:
            // This sets tmpargument to the current target's content value
            valuetmpargument = chraicontent[chraitarget[character]];
            break;

        case FDROPTARGETKEYS:
            // This function makes the target drops keys in inventory (Not inhand)
            drop_keys( chraitarget[character] );
            break;

        case FJOINTEAM:
            // This makes the character itself join a specified team (A = 0, B = 1, 23 = Z, etc.)
            switch_team( character, valuetmpargument );
            break;

        case FTARGETJOINTEAM:
            // This makes the target join a team specified in tmpargument (A = 0, 23 = Z, etc.)
            switch_team( chraitarget[character], valuetmpargument );
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
            append_end_text( madmsgstart[chrmodel[character]] + valuetmpargument, character );
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
            chrcanbecrushed[character] = bfalse;
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
            chrzvel[character] += valuetmpargument / 100.0f;
            break;

        case FFLASHVARIABLEHEIGHT:
            // This function makes the character flash, feet one color, head another...
            flash_character_height( character, valuetmpturn, valuetmpx,
                                    valuetmpdistance, valuetmpy );
            break;

        case FSETDAMAGETIME:
            // This function makes the character invincible for a little while
            chrdamagetime[character] = valuetmpargument;
            break;

        case FIFSTATEIS8:
            returncode = ( 8 == chraistate[character] );
            break;

        case FIFSTATEIS9:
            returncode = ( 9 == chraistate[character] );
            break;

        case FIFSTATEIS10:
            returncode = ( 10 == chraistate[character] );
            break;

        case FIFSTATEIS11:
            returncode = ( 11 == chraistate[character] );
            break;

        case FIFSTATEIS12:
            returncode = ( 12 == chraistate[character] );
            break;

        case FIFSTATEIS13:
            returncode = ( 13 == chraistate[character] );
            break;

        case FIFSTATEIS14:
            returncode = ( 14 == chraistate[character] );
            break;

        case FIFSTATEIS15:
            returncode = ( 15 == chraistate[character] );
            break;

        case FIFTARGETISAMOUNT:
            returncode = chrismount[chraitarget[character]];
            break;

        case FIFTARGETISAPLATFORM:
            returncode = chrplatform[chraitarget[character]];
            break;

        case FADDSTAT:

            if ( !chrstaton[character] ) add_stat( character );

            break;

        case FDISENCHANTTARGET:
            returncode = ( chrfirstenchant[chraitarget[character]] != MAXENCHANT );
            disenchant_character( chraitarget[character] );
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
                                if(chron[sTmp] && chralive[sTmp] && chrteam[sTmp] == chrteam[character])
                                {
                                    distance = ABS(camtrackx-chroldx[sTmp])+ABS(camtracky-chroldy[sTmp]);
                                    if(distance < iTmp)  iTmp = distance;
                                }
                                sTmp++;
                            }
                            distance=iTmp+valuetmpdistance;
                            volume = -distance;
                            volume = volume<<VOLSHIFT;
                            if(volume < VOLMIN) volume = VOLMIN;
                            iTmp = capwaveindex[chrmodel[character]][valuetmpargument];
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
            sTmp = chraitarget[character];   // The target
            tTmp = chrmodel[sTmp];           // The target's model
            iTmp =  capskincost[tTmp][valuetmpargument&3];
            valuetmpy = iTmp;                // Cost of new skin
            iTmp -= capskincost[tTmp][( chrtexture[sTmp] - madskinstart[tTmp] ) & 3];  // Refund

            if ( iTmp > chrmoney[sTmp] )
            {
                // Not enough...
                valuetmpx = iTmp - chrmoney[sTmp];  // Amount needed
                returncode = bfalse;
            }
            else
            {
                // Pay for it...  Cost may be negative after refund...
                chrmoney[sTmp] -= iTmp;

                if ( chrmoney[sTmp] > MAXMONEY )  chrmoney[sTmp] = MAXMONEY;

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
                chraitarget[character] = sTmp;
                returncode = btrue;
            }
            break;

        case FMAKENAMEUNKNOWN:
            // This function makes the name of an item/character unknown.
            chrnameknown[character] = bfalse;
            break;

        case FSPAWNEXACTPARTICLEENDSPAWN:
            // This function spawns a particle that spawns a character...
            tTmp = character;

            if ( chrattachedto[character] != MAXCHR )  tTmp = chrattachedto[character];

            tTmp = spawn_one_particle( valuetmpx, valuetmpy, valuetmpdistance, chrturnleftright[character], chrmodel[character], valuetmpargument, MAXCHR, 0, chrteam[character], tTmp, 0, MAXCHR );

            if ( tTmp != maxparticles )
            {
                prtspawncharacterstate[tTmp] = valuetmpturn;
            }
            break;

        case FSPAWNPOOFSPEEDSPACINGDAMAGE:
            // This function makes a lovely little poof at the character's location,
            // adjusting the xy speed and spacing and the base damage first
            // Temporarily adjust the values for the particle type
            sTmp = chrmodel[character];
            sTmp = madprtpip[sTmp][capgopoofprttype[sTmp]];
            iTmp = pipxyvelbase[sTmp];
            tTmp = pipxyspacingbase[sTmp];
            test = pipdamagebase[sTmp];
            pipxyvelbase[sTmp] = valuetmpx;
            pipxyspacingbase[sTmp] = valuetmpy;
            pipdamagebase[sTmp] = valuetmpargument;
            spawn_poof( character, chrmodel[character] );
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
            chrgrogtime[chraitarget[character]] += valuetmpargument;
            break;

        case FDAZETARGET:
            // This function dazes the target for a duration equal to tmpargument
            chrdazetime[chraitarget[character]] += valuetmpargument;
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

            if ( ( chralert[chrattachedto[character]]&ALERTIFSCOREDAHIT ) != 0 )
            {
                returncode = btrue;
                chraitarget[character] = chrhitlast[chrattachedto[character]];
            }
            break;

        case FIFHOLDERBLOCKED:
            // This function passes if the holder blocked an attack
            returncode = bfalse;

            if ( ( chralert[chrattachedto[character]]&ALERTIFBLOCKED ) != 0 )
            {
                returncode = btrue;
                chraitarget[character] = chrattacklast[chrattachedto[character]];
            }
            break;

        //case FGETSKILLLEVEL:
        //        // This function sets tmpargument to the shield profiency level of the target
        //        valuetmpargument = capshieldprofiency[chrattachedto[character]];
        //    break;

        case FIFTARGETHASNOTFULLMANA:

            // This function passes only if the target is not at max mana and alive
            if ( !chralive[chraitarget[character]] || chrmana[chraitarget[character]] > chrmanamax[chraitarget[character]] - HURTDAMAGE )
                returncode = bfalse;

            break;

        case FENABLELISTENSKILL:
            // This function increases sound play range by 25%
            local_listening = btrue;
            break;

        case FSETTARGETTOLASTITEMUSED:

            // This sets the target to the last item the character used
            if ( chrlastitemused[character] == character ) returncode = bfalse;
            else chraitarget[character] = chrlastitemused[character];

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
            sTmp = chrmodel[chraitarget[character]];
            returncode = capisranged[sTmp] || (capweaponaction[sTmp] != ACTIONPA);
            break;

        case FIFSOMEONEISSTEALING:
            // This function passes if someone stealed from it's shop
            returncode = ( chrorder[character] == STOLEN && chrcounter[character] == 3 );
            break;

        case FIFTARGETISASPELL:
            // Proceeds if the AI target has any particle with the [IDAM] or [WDAM] expansion
            iTmp = 0;
            returncode = bfalse;

            while (iTmp < MAXPRTPIPPEROBJECT)
            {

                if (pipintdamagebonus[madprtpip[chraitarget[character]][iTmp]] || pipwisdamagebonus[madprtpip[chraitarget[character]][iTmp]])
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

            if ( ( chralert[character] & ALERTIFATTACKED ) != 0 )
            {
                sTmp = chrmodel[chrattacklast[character]];

                if ( chrdirectionlast[character] >= BEHIND - 8192 && chrdirectionlast[character] < BEHIND + 8192 )
                {
                    if ( capidsz[sTmp][IDSZ_SKILL] == Make_IDSZ( "STAB" ) )
                    {
                        iTmp = chrdamagetypelast[character];

                        if ( iTmp == DAMAGE_CRUSH || iTmp == DAMAGE_POKE || iTmp == DAMAGE_SLASH ) returncode = btrue;
                    }
                }
            }
            break;

        case FGETTARGETDAMAGETYPE:
            // This function gets the last type of damage for the target
            valuetmpargument = chrdamagetypelast[chraitarget[character]];
            break;

        case FADDQUEST:

            //This function adds a quest idsz set in tmpargument into the targets quest.txt
            if ( chrisplayer[chraitarget[character]] )
            {
                add_quest_idsz( chrname[chraitarget[character]], valuetmpargument );
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
                if ( chrisplayer[iTmp] )
                {
                    if (modify_quest_idsz( chrname[iTmp], (IDSZ)valuetmpargument, 0 ) == QUEST_BEATEN) returncode = btrue;
                }

                iTmp++;
            }
            break;

        case FIFTARGETHASQUEST:

            //This function proceeds if the target has the unfinished quest specified in tmpargument
            //and sets tmpdistance to the Quest Level of the specified quest.
            if ( chrisplayer[chraitarget[character]] )
            {
                iTmp = check_player_quest( chrname[chraitarget[character]], valuetmpargument );

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

            if ( chrisplayer[chraitarget[character]] && valuetmpdistance != 0 )
            {
                if (modify_quest_idsz( chrname[chraitarget[character]], valuetmpargument, valuetmpdistance ) > QUEST_NONE) returncode = btrue;
            }
            break;

        case FADDQUESTALLPLAYERS:
            //This function adds a quest idsz set in tmpargument into all local player's quest logs
            //The quest level is set to tmpdistance if the level is not already higher or QUEST_BEATEN
            iTmp = 0;
            returncode = bfalse;

            while (iTmp < MAXPLAYER)
            {
                if ( chrisplayer[plaindex[iTmp]] )
                {
                    returncode = btrue;

                    if (!add_quest_idsz(chrname[plaindex[iTmp]] , valuetmpargument ))       //Try to add it if not already there or beaten
                    {
                        Sint16 i;
                        i = check_player_quest( chrname[plaindex[iTmp]], valuetmpargument);   //Get the current quest level

                        if (i < 0 || i >= valuetmpdistance) returncode = bfalse;      //It was already beaten
                        else modify_quest_idsz( chrname[plaindex[iTmp]], valuetmpargument, valuetmpdistance );//Not beaten yet, increase level by 1
                    }
                }

                iTmp++;
            }
            break;

        case FADDBLIPALLENEMIES:
            // show all enemies on the minimap who match the IDSZ given in tmpargument
            // it show only the enemies of the AI target
            local_senseenemies = chraitarget[character];
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
            returncode = ( chralive[chraitarget[character]] && chraiowner[character] == chraitarget[character] );
            break;

        case FEND:
            valueterminate = btrue;
            returncode     = bfalse;
            break;

            // If none of the above, skip the line and log an error
        default:
            log_message( "SCRIPT ERROR: run_function() - ai script %d - unhandled script function %d\n", chraitype[character], valuecode );
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
                iTmp = chrxpos[character];
                break;

            case VARSELFY:
                varname = "SELFY";
                iTmp = chrypos[character];
                break;

            case VARSELFTURN:
                varname = "SELFTURN";
                iTmp = chrturnleftright[character];
                break;

            case VARSELFCOUNTER:
                varname = "SELFCOUNTER";
                iTmp = chrcounter[character];
                break;

            case VARSELFORDER:
                varname = "SELFORDER";
                iTmp = chrorder[character];
                break;

            case VARSELFMORALE:
                varname = "SELFMORALE";
                iTmp = teammorale[chrbaseteam[character]];
                break;

            case VARSELFLIFE:
                varname = "SELFLIFE";
                iTmp = chrlife[character];
                break;

            case VARTARGETX:
                varname = "TARGETX";
                iTmp = chrxpos[chraitarget[character]];
                break;

            case VARTARGETY:
                varname = "TARGETY";
                iTmp = chrypos[chraitarget[character]];
                break;

            case VARTARGETDISTANCE:
                varname = "TARGETDISTANCE";
                iTmp = ABS( ( int )( chrxpos[chraitarget[character]] - chrxpos[character] ) ) +
                       ABS( ( int )( chrypos[chraitarget[character]] - chrypos[character] ) );
                break;

            case VARTARGETTURN:
                varname = "TARGETTURN";
                iTmp = chrturnleftright[chraitarget[character]];
                break;

            case VARLEADERX:
                varname = "LEADERX";
                iTmp = chrxpos[character];

                if ( teamleader[chrteam[character]] != NOLEADER )
                    iTmp = chrxpos[teamleader[chrteam[character]]];

                break;

            case VARLEADERY:
                varname = "LEADERY";
                iTmp = chrypos[character];

                if ( teamleader[chrteam[character]] != NOLEADER )
                    iTmp = chrypos[teamleader[chrteam[character]]];

                break;

            case VARLEADERDISTANCE:
                varname = "LEADERDISTANCE";
                iTmp = 10000;

                if ( teamleader[chrteam[character]] != NOLEADER )
                    iTmp = ABS( ( int )( chrxpos[teamleader[chrteam[character]]] - chrxpos[character] ) ) +
                           ABS( ( int )( chrypos[teamleader[chrteam[character]]] - chrypos[character] ) );

                break;

            case VARLEADERTURN:
                varname = "LEADERTURN";
                iTmp = chrturnleftright[character];

                if ( teamleader[chrteam[character]] != NOLEADER )
                    iTmp = chrturnleftright[teamleader[chrteam[character]]];

                break;

            case VARGOTOX:
                varname = "GOTOX";
                if (chraigoto[character] == chraigotoadd[character])
                {
                    iTmp = chrxpos[character];
                }
                else
                {
                    iTmp = chraigotox[character][chraigoto[character]];
                }
                break;

            case VARGOTOY:
                varname = "GOTOY";
                if (chraigoto[character] == chraigotoadd[character])
                {
                    iTmp = chrypos[character];
                }
                else
                {
                    iTmp = chraigotoy[character][chraigoto[character]];
                }
                break;

            case VARGOTODISTANCE:
                varname = "GOTODISTANCE";
                if (chraigoto[character] == chraigotoadd[character])
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = ABS( ( int )( chraigotox[character][chraigoto[character]] - chrxpos[character] ) ) +
                           ABS( ( int )( chraigotoy[character][chraigoto[character]] - chrypos[character] ) );
                }
                break;

            case VARTARGETTURNTO:
                varname = "TARGETTURNTO";
                iTmp = ATAN2( chrypos[chraitarget[character]] - chrypos[character], chrxpos[chraitarget[character]] - chrxpos[character] ) * 65535 / ( TWO_PI );
                iTmp += 32768;
                iTmp &= 65535;
                break;

            case VARPASSAGE:
                varname = "PASSAGE";
                iTmp = chrpassage[character];
                break;

            case VARWEIGHT:
                varname = "WEIGHT";
                iTmp = chrholdingweight[character];
                break;

            case VARSELFALTITUDE:
                varname = "SELFALTITUDE";
                iTmp = chrzpos[character] - chrlevel[character];
                break;

            case VARSELFID:
                varname = "SELFID";
                iTmp = capidsz[chrmodel[character]][IDSZ_TYPE];
                break;

            case VARSELFHATEID:
                varname = "SELFHATEID";
                iTmp = capidsz[chrmodel[character]][IDSZ_HATE];
                break;

            case VARSELFMANA:
                varname = "SELFMANA";
                iTmp = chrmana[character];

                if ( chrcanchannel[character] )  iTmp += chrlife[character];

                break;

            case VARTARGETSTR:
                varname = "TARGETSTR";
                iTmp = chrstrength[chraitarget[character]];
                break;

            case VARTARGETWIS:
                varname = "TARGETWIS";
                iTmp = chrwisdom[chraitarget[character]];
                break;

            case VARTARGETINT:
                varname = "TARGETINT";
                iTmp = chrintelligence[chraitarget[character]];
                break;

            case VARTARGETDEX:
                varname = "TARGETDEX";
                iTmp = chrdexterity[chraitarget[character]];
                break;

            case VARTARGETLIFE:
                varname = "TARGETLIFE";
                iTmp = chrlife[chraitarget[character]];
                break;

            case VARTARGETMANA:
                varname = "TARGETMANA";
                iTmp = chrmana[chraitarget[character]];

                if ( chrcanchannel[chraitarget[character]] )  iTmp += chrlife[chraitarget[character]];

                break;

            case VARTARGETLEVEL:
                varname = "TARGETLEVEL";
                iTmp = chrexperiencelevel[chraitarget[character]];
                break;

            case VARTARGETSPEEDX:
                varname = "TARGETSPEEDX";
                iTmp = chrxvel[chraitarget[character]];
                break;

            case VARTARGETSPEEDY:
                varname = "TARGETSPEEDY";
                iTmp = chryvel[chraitarget[character]];
                break;

            case VARTARGETSPEEDZ:
                varname = "TARGETSPEEDZ";
                iTmp = chrzvel[chraitarget[character]];
                break;

            case VARSELFSPAWNX:
                varname = "SELFSPAWNX";
                iTmp = chrxstt[character];
                break;

            case VARSELFSPAWNY:
                varname = "SELFSPAWNY";
                iTmp = chrystt[character];
                break;

            case VARSELFSTATE:
                varname = "SELFSTATE";
                iTmp = chraistate[character];
                break;

            case VARSELFSTR:
                varname = "SELFSTR";
                iTmp = chrstrength[character];
                break;

            case VARSELFWIS:
                varname = "SELFWIS";
                iTmp = chrwisdom[character];
                break;

            case VARSELFINT:
                varname = "SELFINT";
                iTmp = chrintelligence[character];
                break;

            case VARSELFDEX:
                varname = "SELFDEX";
                iTmp = chrdexterity[character];
                break;

            case VARSELFMANAFLOW:
                varname = "SELFMANAFLOW";
                iTmp = chrmanaflow[character];
                break;

            case VARTARGETMANAFLOW:
                varname = "TARGETMANAFLOW";
                iTmp = chrmanaflow[chraitarget[character]];
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
                iTmp = chrzpos[character];
                break;

            case VARTARGETALTITUDE:
                varname = "TARGETALTITUDE";
                iTmp = chrzpos[chraitarget[character]] - chrlevel[chraitarget[character]];
                break;

            case VARTARGETZ:
                varname = "TARGETZ";
                iTmp = chrzpos[chraitarget[character]];
                break;

            case VARSELFINDEX:
                varname = "SELFINDEX";
                iTmp = character;
                break;

            case VAROWNERX:
                varname = "OWNERX";
                iTmp = chrxpos[chraiowner[character]];
                break;

            case VAROWNERY:
                varname = "OWNERY";
                iTmp = chrypos[chraiowner[character]];
                break;

            case VAROWNERTURN:
                varname = "OWNERTURN";
                iTmp = chrturnleftright[chraiowner[character]];
                break;

            case VAROWNERDISTANCE:
                varname = "OWNERDISTANCE";
                iTmp = ABS( ( int )( chrxpos[chraiowner[character]] - chrxpos[character] ) ) +
                       ABS( ( int )( chrypos[chraiowner[character]] - chrypos[character] ) );
                break;

            case VAROWNERTURNTO:
                varname = "OWNERTURNTO";
                iTmp = ATAN2( chrypos[chraiowner[character]] - chrypos[character], chrxpos[chraiowner[character]] - chrxpos[character] ) * 65535 / ( TWO_PI );
                iTmp += 32768;
                iTmp &= 65535;
                break;

            case VARXYTURNTO:
                varname = "XYTURNTO";
                iTmp = ATAN2( valuetmpy - chrypos[character], valuetmpx - chrxpos[character] ) * 65535 / ( TWO_PI );
                iTmp += 32768;
                iTmp &= 65535;
                break;

            case VARSELFMONEY:
                varname = "SELFMONEY";
                iTmp = chrmoney[character];
                break;

            case VARSELFACCEL:
                varname = "SELFACCEL";
                iTmp = ( chrmaxaccel[character] * 100.0f );
                break;

            case VARTARGETEXP:
                varname = "TARGETEXP";
                iTmp = chrexperience[chraitarget[character]];
                break;

            case VARSELFAMMO:
                varname = "SELFAMMO";
                iTmp = chrammo[character];
                break;

            case VARTARGETAMMO:
                varname = "TARGETAMMO";
                iTmp = chrammo[chraitarget[character]];
                break;

            case VARTARGETMONEY:
                varname = "TARGETMONEY";
                iTmp = chrmoney[chraitarget[character]];
                break;

            case VARTARGETTURNAWAY:
                varname = "TARGETTURNAWAY";
                iTmp = ATAN2( chrypos[chraitarget[character]] - chrypos[character], chrxpos[chraitarget[character]] - chrxpos[character] ) * 65535 / ( TWO_PI );
                iTmp += 32768;
                iTmp &= 65535;
                break;

            case VARSELFLEVEL:
                varname = "SELFLEVEL";
                iTmp = chrexperiencelevel[character];
                break;

            case VARTARGETRELOADTIME:
                varname = "TARGETRELOADTIME";
                iTmp = chrreloadtime[chraitarget[character]];
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

    if( character >= MAXCHR || !chron[character] )  return;

    // Make life easier
    valueoldtarget = chraitarget[character];
    aicode = chraitype[character];

    // Figure out alerts that weren't already set
    set_alerts( character );

    script_error_classname = "UNKNOWN";
    script_error_model     = chrmodel[character];
    if( script_error_model < MAXMODEL )
    {
        script_error_classname = capclassname[ script_error_model ];
    }

    if ( debug_scripts )
    {
        printf( "\n\n--------\n%d - %s\n", script_error_model, script_error_classname );
    }

    // Clear the button latches
    if ( !chrisplayer[character] )
    {
        chrlatchbutton[character] = 0;
    }

    // Reset the target if it can't be seen
    if ( !chrcanseeinvisible[character] && chralive[character] )
    {
        if ( chralpha[chraitarget[character]] <= INVISIBLE || chrlight[chraitarget[character]] <= INVISIBLE )
        {
            chraitarget[character] = character;
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
    if ( !chrisplayer[character] )
    {
        float latch2;

        if ( chrismount[character] && MAXCHR != chrholdingwhich[character][0] && chron[chrholdingwhich[character][0]] )
        {
            // Mount
            chrlatchx[character] = chrlatchx[chrholdingwhich[character][0]];
            chrlatchy[character] = chrlatchy[chrholdingwhich[character][0]];
        }
        else if ( chraigoto[character] != chraigotoadd[character] )
        {
            // Normal AI
            chrlatchx[character] = ( chraigotox[character][chraigoto[character]] - chrxpos[character] ) / (128 << 2);
            chrlatchy[character] = ( chraigotoy[character][chraigoto[character]] - chrypos[character] ) / (128 << 2);
        }
        else
        {
            // AI, but no valid waypoints
            chrlatchx[character] = 0;
            chrlatchy[character] = 0;
        }

        latch2 = chrlatchx[character] * chrlatchx[character] + chrlatchy[character] * chrlatchy[character];
        if (latch2 > 1.0f)
        {
            latch2 = 1.0f / sqrt(latch2);
            chrlatchx[character] *= latch2;
            chrlatchy[character] *= latch2;
        }
    }

    // Clear alerts for next time around
    chralert[character] = 0;

    if ( valuechanged )  chralert[character] = ALERTIFCHANGED;

    // Do poofing
    if ( valuegopoof )
    {
        if ( chrattachedto[character] != MAXCHR )
            detach_character_from_mount( character, btrue, bfalse );

        if ( chrholdingwhich[character][0] != MAXCHR )
            detach_character_from_mount( chrholdingwhich[character][0], btrue, bfalse );

        if ( chrholdingwhich[character][1] != MAXCHR )
            detach_character_from_mount( chrholdingwhich[character][1], btrue, bfalse );

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
        if ( chron[character] && ( !chrinpack[character] || capisequipment[chrmodel[character]] ) && ( chralive[character] || ( chralert[character]&ALERTIFCLEANEDUP ) || ( chralert[character]&ALERTIFCRUSHED ) ) )
        {
            // Crushed characters shouldn't be alert to anything else
            if ( chralert[character] & ALERTIFCRUSHED )  chralert[character] = ALERTIFCRUSHED;

            // Cleaned up characters shouldn't be alert to anything else
            if ( chralert[character] & ALERTIFCLEANEDUP )  chralert[character] = ALERTIFCLEANEDUP;

            let_character_think( character );
        }
    }
}
