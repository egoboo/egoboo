/* Egoboo - script.c
 * Implements the game's scripting language.
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

#include <assert.h>

#include "egoboo.h"
#include "egoboo_math.h"
#include "Log.h"
#include "mesh.h"

SCRIPT_GLOBAL_VALUES scr_globals = {0, 0, 0, 0, 0, 0, 0, 0};

#define FUNCTION_FLAG_BIT 0x80000000
#define CONSTANT_FLAG_BIT 0x80000000
#define CONSTANT_BITS     0x07FFFFFF

#define INDENT_BITS  0x78000000
#define INDENT_SHIFT 27

#define OP_BITS  0x78000000
#define OP_SHIFT 27

#define VAR_BITS     0x07FFFFFF
#define OPCODE_BITS  0x07FFFFFF

#define HIGH_BITS  0xF8000000
#define HIGH_SHIFT 27

#define END_FUNCTION  (FUNCTION_FLAG_BIT | F_End)
#define IS_FUNCTION(XX)  HAS_SOME_BITS( XX, FUNCTION_FLAG_BIT )
#define NOT_FUNCTION(XX) HAS_NO_BITS( XX, FUNCTION_FLAG_BIT )

#define IS_CONSTANT(XX) HAS_SOME_BITS( XX, CONSTANT_FLAG_BIT )
#define GET_CONSTANT(XX) ( (XX) & CONSTANT_BITS )
#define PUT_CONSTANT(XX) ( ((XX) & CONSTANT_BITS) | CONSTANT_FLAG_BIT )

#define GET_FUNCTION_BITS(XX) ( (XX) & (FUNCTION_FLAG_BIT | 0x07FFFFFF) )

#define GET_FUNCTION_BIT(XX) ( ((XX) & FUNCTION_FLAG_BIT) >> 31 )
#define PUT_FUNCTION_BIT(XX) ( ((XX) << 31) & FUNCTION_FLAG_BIT )

#define GET_CONSTANT_BIT(XX) ( ((XX) & CONSTANT_FLAG_BIT) >> 31 )
#define PUT_CONSTANT_BIT(XX) ( ((XX) << 31) & CONSTANT_FLAG_BIT )

#define GET_OP_BITS(XX) ( ((XX) & OP_BITS) >> OP_SHIFT )
#define PUT_OP_BITS(XX) ( ((XX) << OP_SHIFT) & OP_BITS  )

#define GET_VAR_BITS(XX) ( (XX) & VAR_BITS )
#define PUT_VAR_BITS(XX) ( (XX) & VAR_BITS )

#define GET_OPCODE_BITS(XX) ( (XX) & VAR_BITS )
#define PUT_OPCODE_BITS(XX) ( (XX) & VAR_BITS )

#define GET_INDENT(XX) ( ((XX) & INDENT_BITS) >> INDENT_SHIFT )
#define PUT_INDENT(XX) ( ((XX) << INDENT_SHIFT) & INDENT_BITS )

#define IS_END(XX) 
typedef struct opcode_element_t
{
  Uint8           type;
  Uint32          value;
  char            name[MAXCODENAMESIZE];
} OPCODE_ELEMENT;

typedef struct opcode_list_t
{
  int            count;
  OPCODE_ELEMENT opcode[MAXCODE];
} OPCODE_LIST;

static OPCODE_LIST opcode_lst = {0};

typedef struct script_internal_values_t
{
  Uint8           buffer[MAXLINESIZE];
  size_t          load_size;
  size_t          line_size;
  int             line_num;


  int             opcode_index;
  int             opcode_value_tmp;

  Uint32          compiled[AISMAXCOMPILESIZE];
  int             script_address[MAXAI];
  int             script_count;
  int             script_index;
} SCRIPT_INTERNAL_VALUES;

static SCRIPT_INTERNAL_VALUES scr_intern;


//------------------------------------------------------------------------------
//AI Script Routines------------------------------------------------------------
//------------------------------------------------------------------------------
void insert_space( size_t position )
{
  // ZZ> This function adds a space into the load line if there isn't one
  //     there already
  Uint8 cTmp, cSwap;

  if ( scr_intern.buffer[position] != ' ' )
  {
    cTmp = scr_intern.buffer[position];
    scr_intern.buffer[position] = ' ';
    position++;
    scr_intern.line_size++;
    while ( position < scr_intern.line_size )
    {
      cSwap = scr_intern.buffer[position];
      scr_intern.buffer[position] = cTmp;
      cTmp = cSwap;
      position++;
    }
    scr_intern.buffer[position] = 0;
  }
}

//------------------------------------------------------------------------------
void copy_one_line( size_t write )
{
  // ZZ> This function copies the line back to the load buffer
  size_t read;
  Uint8 cTmp;


  read = 0;
  cTmp = scr_intern.buffer[read];
  while ( cTmp != 0 )
  {
    cTmp = scr_intern.buffer[read];  read++;
    cLoadBuffer[write] = cTmp;  write++;
  }



  scr_intern.line_num++;
}

//------------------------------------------------------------------------------
size_t load_one_line( size_t read )
{
  // ZZ> This function loads a line into the line buffer
  bool_t stillgoing, foundtext;
  Uint8 cTmp;


  // Parse to start to maintain indentation
  scr_intern.line_size = 0;
  stillgoing = btrue;
  while ( stillgoing && read < scr_intern.load_size )
  {
    cTmp = cLoadBuffer[read];
    stillgoing = bfalse;
    if ( cTmp == ' ' )
    {
      read++;
      scr_intern.buffer[scr_intern.line_size] = cTmp; scr_intern.line_size++;
      stillgoing = btrue;
    }
    // scr_intern.buffer[scr_intern.line_size] = 0; // or cTmp as cTmp == 0
  }


  // Parse to comment or end of line
  foundtext = bfalse;
  stillgoing = btrue;
  while ( stillgoing && read < scr_intern.load_size )
  {
    cTmp = cLoadBuffer[read];  read++;
    if ( cTmp == '\t' )
      cTmp = ' ';
    if ( cTmp != ' ' && cTmp != 0x0d && cTmp != 0x0a &&
         ( cTmp != '/' || cLoadBuffer[read] != '/' ) )
      foundtext = btrue;
    scr_intern.buffer[scr_intern.line_size] = cTmp;
    if ( cTmp != ' ' || ( cLoadBuffer[read] != ' ' && cLoadBuffer[read] != '\t' ) )
      scr_intern.line_size++;
    if ( cTmp == 0x0d || cTmp == 0x0a )
      stillgoing = bfalse;
    if ( cTmp == '/' && cLoadBuffer[read] == '/' )
      stillgoing = bfalse;
  }
  if ( !stillgoing ) scr_intern.line_size--;

  scr_intern.buffer[scr_intern.line_size] = 0;
  if ( scr_intern.line_size >= 1 )
  {
    if ( scr_intern.buffer[scr_intern.line_size-1] == ' ' )
    {
      scr_intern.line_size--;  scr_intern.buffer[scr_intern.line_size] = 0;
    }
  }
  scr_intern.line_size++;


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
    scr_intern.line_size = 0;
  }


  return read;
}

//------------------------------------------------------------------------------
size_t load_parsed_line( size_t read )
{
  // ZZ> This function loads a line into the line buffer
  Uint8 cTmp;


  // Parse to start to maintain indentation
  scr_intern.line_size = 0;
  cTmp = cLoadBuffer[read];
  while ( cTmp != 0 )
  {
    scr_intern.buffer[scr_intern.line_size] = cTmp;  scr_intern.line_size++;
    read++;  cTmp = cLoadBuffer[read];
  }
  scr_intern.buffer[scr_intern.line_size] = 0;
  read++; // skip terminating zero for next call of load_parsed_line()
  return read;
}

//------------------------------------------------------------------------------
void surround_space( size_t position )
{
  insert_space( position + 1 );
  if ( position > 0 )
  {
    if ( scr_intern.buffer[position-1] != ' ' )
    {
      insert_space( position );
    }
  }
}

//------------------------------------------------------------------------------
void parse_null_terminate_comments()
{
  // ZZ> This function removes comments and endline codes, replacing
  //     them with a 0
  size_t read, write;


  read = 0;
  write = 0;
  while ( read < scr_intern.load_size )
  {
    read = load_one_line( read );
    if ( scr_intern.line_size > 2 )
    {
      copy_one_line( write );
      write += scr_intern.line_size;
    }
  }
}

//------------------------------------------------------------------------------
int get_indentation()
{
  // ZZ> This function returns the number of starting spaces in a line
  int cnt;
  Uint8 cTmp;

  cnt = 0;
  cTmp = scr_intern.buffer[cnt];
  while ( cTmp == ' ' )
  {
    cnt++;
    cTmp = scr_intern.buffer[cnt];
  }

  if( iscntrl(cTmp) )
  {
    log_message( "SCRIPT ERROR FOUND: %s (%d) - %d illegal control code\n", globalparsename, scr_intern.line_num, cnt );
    parseerror = btrue;
  }

  if( HAS_SOME_BITS(cnt, 0x01) )
  {
    log_message( "SCRIPT ERROR FOUND: %s (%d) - %d odd number of spaces\n", globalparsename, scr_intern.line_num, cnt );
    parseerror = btrue;
  };

  cnt >>= 1;

  if ( cnt > 15 )
  {
    log_warning( "SCRIPT ERROR FOUND: %s (%d) - %d levels of indentation\n", globalparsename, scr_intern.line_num, cnt );
    parseerror = btrue;
    cnt = 15;
  };

  return cnt;
}

//------------------------------------------------------------------------------
void fix_operators()
{
  // ZZ> This function puts spaces around operators to seperate words
  //     better
  Uint32 cnt;
  Uint8 cTmp;

  cnt = 0;
  while ( cnt < scr_intern.line_size )
  {
    cTmp = scr_intern.buffer[cnt];
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

//------------------------------------------------------------------------------
int starts_with_capital_letter()
{
  // ZZ> This function returns btrue if the line starts with a capital
  int cnt;
  Uint8 cTmp;

  cnt = 0;
  cTmp = scr_intern.buffer[cnt];
  while ( cTmp == ' ' )
  {
    cnt++;
    cTmp = scr_intern.buffer[cnt];
  }
  if ( cTmp >= 'A' && cTmp <= 'Z' )
    return btrue;
  return bfalse;
}

//------------------------------------------------------------------------------
Uint32 get_high_bits()
{
  // ZZ> This function looks at the first word and generates the high
  //     bit codes for it
  Uint32 highbits, indent;

  indent = get_indentation();
  highbits = PUT_INDENT(indent);
  if ( starts_with_capital_letter() )
  {
    highbits |= FUNCTION_FLAG_BIT;
  }

  return highbits;
}

//------------------------------------------------------------------------------
size_t tell_code( size_t read )
{
  // ZZ> This function tells what code is being indexed by read, it
  //     will return the next spot to read from and stick the code number
  //     in scr_intern.opcode_index

  int cnt, wordsize;
  bool_t codecorrect;
  Uint8 cTmp;
  int idsz;
  char cWordBuffer[MAXCODENAMESIZE];


  // Check bounds
  scr_intern.opcode_index = MAXCODE;
  if ( read >= scr_intern.line_size )  return read;


  // Skip spaces
  cTmp = scr_intern.buffer[read];
  while ( cTmp == ' ' )
  {
    read++;
    cTmp = scr_intern.buffer[read];
  }
  if ( read >= scr_intern.line_size )  return read;


  // Load the word into the other buffer
  wordsize = 0;
  while ( !isspace( cTmp ) && '\0' != cTmp )
  {
    cWordBuffer[wordsize] = cTmp;  wordsize++;
    read++;
    cTmp = scr_intern.buffer[read];
  }
  cWordBuffer[wordsize] = 0;


  // Check for numeric constant
  if ( isdigit( cWordBuffer[0] ) )
  {
    sscanf( &cWordBuffer[0], "%d", &scr_intern.opcode_value_tmp );
    scr_intern.opcode_index = -1;
    return read;
  }


  // Check for IDSZ constant
  idsz = IDSZ_NONE;
  if ( cWordBuffer[0] == '[' )
  {
    idsz = MAKE_IDSZ( &cWordBuffer[1] );
    scr_intern.opcode_value_tmp = idsz;
    scr_intern.opcode_index = -1;
    return read;
  }



  // Compare the word to all the codes
  codecorrect = bfalse;
  scr_intern.opcode_index = 0;
  while ( scr_intern.opcode_index < opcode_lst.count && !codecorrect )
  {
    codecorrect = bfalse;
    if ( cWordBuffer[0] == opcode_lst.opcode[scr_intern.opcode_index].name[0] && !codecorrect )
    {
      codecorrect = btrue;
      cnt = 1;
      while ( cnt < wordsize )
      {
        if ( cWordBuffer[cnt] != opcode_lst.opcode[scr_intern.opcode_index].name[cnt] )
        {
          codecorrect = bfalse;
          cnt = wordsize;
        }
        cnt++;
      }
      if ( cnt < MAXCODENAMESIZE )
      {
        if ( opcode_lst.opcode[scr_intern.opcode_index].name[cnt] != 0 )  codecorrect = bfalse;
      }
    }
    scr_intern.opcode_index++;
  }

  if ( codecorrect )
  {
    scr_intern.opcode_index--;
    scr_intern.opcode_value_tmp = opcode_lst.opcode[scr_intern.opcode_index].value;
    if ( opcode_lst.opcode[scr_intern.opcode_index].type == 'C' )
    {
      // Check for constants
      scr_intern.opcode_index = -1;
    }
  }
  else
  {
    // Throw out an error code if we're loggin' 'em
    if ( '=' != cWordBuffer[0] || '\0' != cWordBuffer[1])
    {
      assert(bfalse);       
      log_message( "SCRIPT ERROR FOUND: %s (%d) - %s undefined\n", globalparsename, scr_intern.line_num, cWordBuffer );

      parseerror = btrue;
    }
  }

  return read;
}

//------------------------------------------------------------------------------
void add_code( Uint32 highbits )
{
  Uint32 value;

  if ( scr_intern.opcode_index == -1 )  highbits |= FUNCTION_FLAG_BIT;
  if ( scr_intern.opcode_index != MAXCODE )
  {
    value = highbits | scr_intern.opcode_value_tmp;
    scr_intern.compiled[scr_intern.script_index] = value;
    scr_intern.script_index++;
  }
}

//------------------------------------------------------------------------------
void parse_line_by_line()
{
  // ZZ> This function removes comments and endline codes, replacing
  //     them with a 0
  size_t read, line;
  Uint32 highbits;
  size_t parseposition;
  int operands;


  line = 0;
  read = 0;
  while ( line < scr_intern.line_num )
  {
    read = load_parsed_line( read );
    fix_operators();
    highbits = get_high_bits();
    parseposition = 0;
    parseposition = tell_code( parseposition );   // VALUE
    add_code( highbits );
    scr_intern.opcode_value_tmp = 0;  // SKIP FOR CONTROL CODES
    add_code( 0 );
    if ( NOT_FUNCTION( highbits ) )
    {
      parseposition = tell_code( parseposition );   // EQUALS
      parseposition = tell_code( parseposition );   // VALUE
      add_code( 0 );
      operands = 1;
      while ( parseposition < scr_intern.line_size )
      {
        parseposition = tell_code( parseposition );   // OPERATOR
        if ( scr_intern.opcode_index == -1 ) scr_intern.opcode_index = 1;
        else scr_intern.opcode_index = 0;
        highbits = PUT_OP_BITS(scr_intern.opcode_value_tmp) | PUT_CONSTANT_BIT( scr_intern.opcode_index );
        parseposition = tell_code( parseposition );   // VALUE
        add_code( highbits );
        if ( scr_intern.opcode_index != MAXCODE )
          operands++;
      }
      scr_intern.compiled[scr_intern.script_index-operands-1] = operands;  // Number of operands
    }
    line++;
  }
}

//------------------------------------------------------------------------------
Uint32 jump_goto( int index )
{
  // ZZ> This function figures out where to jump to on a fail based on the
  //     starting location and the following code.  The starting location
  //     should always be a function code with indentation
  Uint32 value;
  int targetindent, indent;


  value = scr_intern.compiled[index];  index += 2;
  targetindent = GET_INDENT( value );
  indent = 100;
  while ( indent > targetindent )
  {
    value = scr_intern.compiled[index];
    indent = GET_INDENT( value );
    if ( indent > targetindent )
    {
      // Was it a function
      if ( IS_FUNCTION( value ) )
      {
        // Each function needs a jump
        index++;
        index++;
      }
      else
      {
        // Operations cover each operand
        index++;
        value = scr_intern.compiled[index];
        index++;
        index += ( value & 255 );
      }
    }
  }
  return index;
}

//------------------------------------------------------------------------------
void parse_jumps( int ainumber )
{
  // ZZ> This function sets up the fail jumps for the down and dirty code
  int index;
  Uint32 value, iTmp;


  index = scr_intern.script_address[ainumber];
  value = scr_intern.compiled[index];
  while ( GET_FUNCTION_BITS( value ) != END_FUNCTION ) // End Function
  {
    value = scr_intern.compiled[index];
    // Was it a function
    if ( IS_FUNCTION( value ) )
    {
      // Each function needs a jump
      iTmp = jump_goto( index );
      index++;
      scr_intern.compiled[index] = iTmp;
      index++;
    }
    else
    {
      // Operations cover each operand
      index++;
      iTmp = scr_intern.compiled[index];
      index++;
      index += ( iTmp & 255 );
    }
  }
}

//------------------------------------------------------------------------------
void log_code( int ainumber, char* savename )
{
  // ZZ> This function shows the actual code, saving it in a file
  int index;
  Uint32 value;
  FILE* filewrite;

  filewrite = fs_fileOpen( PRI_NONE, NULL, savename, "w" );
  if ( filewrite )
  {
    index = scr_intern.script_address[ainumber];
    value = scr_intern.compiled[index];
    while ( GET_FUNCTION_BITS( value ) != END_FUNCTION ) // End Function
    {
      value = scr_intern.compiled[index];
      fprintf( filewrite, "0x%08x--0x%08x\n", index, value );
      index++;
    }
    fs_fileClose( filewrite );
  }
  SDL_Quit();
}

//------------------------------------------------------------------------------
int ai_goto_colon( int read )
{
  // ZZ> This function goes to spot after the next colon
  Uint8 cTmp;

  cTmp = cLoadBuffer[read];
  while ( cTmp != ':' && read < scr_intern.load_size )
  {
    read++;  cTmp = cLoadBuffer[read];
  }
  if ( read < scr_intern.load_size )  read++;
  return read;
}

//------------------------------------------------------------------------------
void fget_code( FILE * pfile )
{
  // ZZ> This function gets code names and other goodies
  char cTmp;
  int iTmp;

  fscanf( pfile, "%c%d%s", &cTmp, &iTmp, opcode_lst.opcode[opcode_lst.count].name );
  opcode_lst.opcode[opcode_lst.count].type = cTmp;
  opcode_lst.opcode[opcode_lst.count].value = iTmp;
  opcode_lst.count++;
}

//------------------------------------------------------------------------------
//void load_ai_codes(char* loadname)
//{
//  // ZZ> This function loads all of the function and variable names
//  FILE* fileread;
//  int read;
//
//  opcode_lst.count = 0;
//  fileread = fs_fileOpen(PRI_NONE, NULL, loadname, "rb");
//  if (NULL != fileread)
//  {
//    scr_intern.load_size = (int)fread(&cLoadBuffer[0], 1, MD2MAXLOADSIZE, fileread);
//    read = 0;
//    read = ai_goto_colon(read);
//    while (read != scr_intern.load_size)
//    {
//      fget_code(read);
//      read = ai_goto_colon(read);
//    }
//    fs_fileClose(fileread);
//  }
//  else log_warning("Could not load script AI functions or variables (%s)\n", loadname);
//}

// Don't use the aicodes file, just register them in the program.
// There will never be a reason to change them, and it will prevent transcription mistakes between the
// source code and the aicodes.txt file

#define REGISTER_OPCODE(LIST,TYPE,CODE,NAME)  { strncpy(LIST.opcode[LIST.count].name, NAME, MAXCODENAMESIZE); LIST.opcode[LIST.count].type = (Uint8)TYPE; LIST.opcode[LIST.count].value = (Uint16)CODE; LIST.count++; }
#define REGISTER_FUNCTION(LIST,NAME)          { strncpy(LIST.opcode[LIST.count].name, #NAME, MAXCODENAMESIZE); LIST.opcode[LIST.count].type = (Uint8)'F'; LIST.opcode[LIST.count].value = (Uint16)F_##NAME; LIST.count++; }
#define REGISTER_FUNCTION_ALIAS(LIST,NAME,ALIAS)          { strncpy(LIST.opcode[LIST.count].name, ALIAS, MAXCODENAMESIZE); LIST.opcode[LIST.count].type = (Uint8)'F'; LIST.opcode[LIST.count].value = (Uint16)F_##NAME; LIST.count++; }

//------------------------------------------------------------------------------
void load_ai_codes( char* loadname )
{
  // ZZ> This function loads all of the function and variable names
  FILE* fileread;

  log_info( "load_ai_codes() - registering internal script constants\n" );

  opcode_lst.count = 0;

  // register all the operator opcodes
  REGISTER_OPCODE( opcode_lst, 'O', OP_ADD, "+" );
  REGISTER_OPCODE( opcode_lst, 'O', OP_SUB, "-" );
  REGISTER_OPCODE( opcode_lst, 'O', OP_AND, "&" );
  REGISTER_OPCODE( opcode_lst, 'O', OP_SHR, ">" );
  REGISTER_OPCODE( opcode_lst, 'O', OP_SHL, "<" );
  REGISTER_OPCODE( opcode_lst, 'O', OP_MUL, "*" );
  REGISTER_OPCODE( opcode_lst, 'O', OP_DIV, "/" );
  REGISTER_OPCODE( opcode_lst, 'O', OP_MOD, "%" );

  // register all of the internal variables

  REGISTER_OPCODE( opcode_lst, 'V', VAR_TMP_X,           "tmpx" );   // 000
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TMP_Y,           "tmpy" );   // 001
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TMP_DISTANCE,    "tmpdist" );   // 002
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TMP_DISTANCE,    "tmpdistance" );   // 002
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TMP_TURN,        "tmpturn" );   // 003
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TMP_ARGUMENT,    "tmpargument" );   // 004
  REGISTER_OPCODE( opcode_lst, 'V', VAR_RAND,            "rand" );   // 005
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_X,          "selfx" );   // 006
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_Y,          "selfy" );   // 007
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_TURN,       "selfturn" );   // 008
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_COUNTER,    "selfcounter" );   // 009
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_ORDER,      "selforder" );   // 010
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_MORALE,     "selfmorale" );   // 011
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_LIFE,       "selflife" );   // 012
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_X,        "targetx" );   // 013
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_Y,        "targety" );   // 014
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_DISTANCE, "targetdistance" );   // 015
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_TURN,     "targetturn" );   // 016
  REGISTER_OPCODE( opcode_lst, 'V', VAR_LEADER_X,        "leaderx" );   // 017
  REGISTER_OPCODE( opcode_lst, 'V', VAR_LEADER_Y,        "leadery" );   // 018
  REGISTER_OPCODE( opcode_lst, 'V', VAR_LEADER_DISTANCE, "leaderdistance" );   // 019
  REGISTER_OPCODE( opcode_lst, 'V', VAR_LEADER_TURN,     "leaderturn" );   // 020
  REGISTER_OPCODE( opcode_lst, 'V', VAR_GOTO_X,          "gotox" );   // 021
  REGISTER_OPCODE( opcode_lst, 'V', VAR_GOTO_Y,          "gotoy" );   // 022
  REGISTER_OPCODE( opcode_lst, 'V', VAR_GOTO_DISTANCE,   "gotodistance" );   // 023
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_TURNTO,   "targetturnto" );   // 024
  REGISTER_OPCODE( opcode_lst, 'V', VAR_PASSAGE,         "passage" );   // 025
  REGISTER_OPCODE( opcode_lst, 'V', VAR_WEIGHT,          "weight" );   // 026
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_ALTITUDE,   "selfaltitude" );   // 027
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_ID,         "selfid" );   // 028
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_HATEID,     "selfhateid" );   // 029
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_MANA,       "selfmana" );   // 030
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_STR,      "targetstr" );   // 031
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_WIS,      "targetwis" );   // 032
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_INT,      "targetint" );   // 033
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_DEX,      "targetdex" );   // 034
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_LIFE,     "targetlife" );   // 035
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_MANA,     "targetmana" );   // 036
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_LEVEL,    "targetlevel" );   // 037
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_SPEEDX,   "targetspeedx" );   // 038
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_SPEEDY,   "targetspeedy" );   // 039
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_SPEEDZ,   "targetspeedz" );   // 040
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_SPAWNX,     "selfspawnx" );   // 041
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_SPAWNY,     "selfspawny" );   // 042
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_STATE,      "selfstate" );   // 043
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_STR,        "selfstr" );   // 044
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_WIS,        "selfwis" );   // 045
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_INT,        "selfint" );   // 046
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_DEX,        "selfdex" );   // 047
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_MANAFLOW,   "selfmanaflow" );   // 048
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_MANAFLOW, "targetmanaflow" );   // 049
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_ATTACHED,   "selfattached" );   // 050
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SWINGTURN,       "swingturn" );   // 051
  REGISTER_OPCODE( opcode_lst, 'V', VAR_XYDISTANCE,      "xydistance" );   // 052
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_Z,          "selfz" );   // 053
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_ALTITUDE, "targetaltitude" );   // 054
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_Z,        "targetz" );   // 055
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_INDEX,      "selfindex" );   // 056
  REGISTER_OPCODE( opcode_lst, 'V', VAR_OWNER_X,         "ownerx" );   // 057
  REGISTER_OPCODE( opcode_lst, 'V', VAR_OWNER_Y,         "ownery" );   // 058
  REGISTER_OPCODE( opcode_lst, 'V', VAR_OWNER_TURN,      "ownerturn" );   // 059
  REGISTER_OPCODE( opcode_lst, 'V', VAR_OWNER_DISTANCE,  "ownerdistance" );   // 060
  REGISTER_OPCODE( opcode_lst, 'V', VAR_OWNER_TURNTO,    "ownerturnto" );   // 061
  REGISTER_OPCODE( opcode_lst, 'V', VAR_XYTURNTO,        "xyturnto" );   // 062
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_MONEY,      "selfmoney" );   // 063
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_ACCEL,      "selfaccel" );   // 064
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_EXP,      "targetexp" );   // 065
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_AMMO,       "selfammo" );   // 066
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_AMMO,     "targetammo" );   // 067
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_MONEY,    "targetmoney" );   // 068
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_TURNAWAY, "targetturnfrom" );   // 069
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_LEVEL,      "selflevel" );   // 070
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SPAWN_DISTANCE,  "selfspawndistance" );   // 070

  // register internal constants
  REGISTER_OPCODE( opcode_lst, 'C', LATCHBUTTON_LEFT, "LATCHLEFT" );
  REGISTER_OPCODE( opcode_lst, 'C', LATCHBUTTON_RIGHT, "LATCHRIGHT" );
  REGISTER_OPCODE( opcode_lst, 'C', LATCHBUTTON_JUMP, "LATCHJUMP" );
  REGISTER_OPCODE( opcode_lst, 'C', LATCHBUTTON_ALTLEFT, "LATCHALTLEFT" );
  REGISTER_OPCODE( opcode_lst, 'C', LATCHBUTTON_ALTRIGHT, "LATCHALTRIGHT" );
  REGISTER_OPCODE( opcode_lst, 'C', LATCHBUTTON_PACKLEFT, "LATCHPACKLEFT" );
  REGISTER_OPCODE( opcode_lst, 'C', LATCHBUTTON_PACKRIGHT, "LATCHPACKRIGHT" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_SLASH, "DAMAGESLASH" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_CRUSH, "DAMAGECRUSH" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_POKE, "DAMAGEPOKE" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_HOLY, "DAMAGEHOLY" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_EVIL, "DAMAGEEVIL" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_FIRE, "DAMAGEFIRE" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_ICE, "DAMAGEICE" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_ZAP, "DAMAGEZAP" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_DA, "ACTIONDA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_DB, "ACTIONDB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_DC, "ACTIONDC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_DD, "ACTIONDD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_UA, "ACTIONUA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_UB, "ACTIONUB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_UC, "ACTIONUC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_UD, "ACTIONUD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_TA, "ACTIONTA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_TB, "ACTIONTB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_TC, "ACTIONTC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_TD, "ACTIONTD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_CA, "ACTIONCA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_CB, "ACTIONCB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_CC, "ACTIONCC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_CD, "ACTIONCD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_SA, "ACTIONSA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_SB, "ACTIONSB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_SC, "ACTIONSC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_SD, "ACTIONSD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_BA, "ACTIONBA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_BB, "ACTIONBB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_BC, "ACTIONBC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_BD, "ACTIONBD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_LA, "ACTIONLA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_LB, "ACTIONLB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_LC, "ACTIONLC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_LD, "ACTIONLD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_XA, "ACTIONXA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_XB, "ACTIONXB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_XC, "ACTIONXC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_XD, "ACTIONXD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_FA, "ACTIONFA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_FB, "ACTIONFB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_FC, "ACTIONFC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_FD, "ACTIONFD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_PA, "ACTIONPA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_PB, "ACTIONPB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_PC, "ACTIONPC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_PD, "ACTIONPD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_EA, "ACTIONEA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_EB, "ACTIONEB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_RA, "ACTIONRA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_ZA, "ACTIONZA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_ZB, "ACTIONZB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_ZC, "ACTIONZC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_ZD, "ACTIONZD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_WA, "ACTIONWA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_WB, "ACTIONWB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_WC, "ACTIONWC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_WD, "ACTIONWD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_JA, "ACTIONJA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_JB, "ACTIONJB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_JC, "ACTIONJC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_HA, "ACTIONHA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_HB, "ACTIONHB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_HC, "ACTIONHC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_HD, "ACTIONHD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_KA, "ACTIONKA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_KB, "ACTIONKB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_KC, "ACTIONKC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_KD, "ACTIONKD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MA, "ACTIONMA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MB, "ACTIONMB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MC, "ACTIONMC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MD, "ACTIONMD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_ME, "ACTIONME" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MF, "ACTIONMF" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MG, "ACTIONMG" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MH, "ACTIONMH" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MI, "ACTIONMI" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MJ, "ACTIONMJ" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MK, "ACTIONMK" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_ML, "ACTIONML" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MM, "ACTIONMM" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MN, "ACTIONMN" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_FINDSECRET, "EXPSECRET" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_WINQUEST, "EXPQUEST" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_USEDUNKOWN, "EXPDARE" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_KILLENEMY, "EXPKILL" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_KILLSLEEPY, "EXPMURDER" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_KILLHATED, "EXPREVENGE" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_TEAMKILL, "EXPTEAMWORK" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_TALKGOOD, "EXPROLEPLAY" );
  REGISTER_OPCODE( opcode_lst, 'C', MESHFX_NOREFLECT, "FXNOREFLECT" );
  REGISTER_OPCODE( opcode_lst, 'C', MESHFX_SHINY,     "FXDRAWREFLECT" );
  REGISTER_OPCODE( opcode_lst, 'C', MESHFX_ANIM,      "FXANIM" );
  REGISTER_OPCODE( opcode_lst, 'C', MESHFX_WATER,     "FXWATER" );
  REGISTER_OPCODE( opcode_lst, 'C', MESHFX_WALL,      "FXBARRIER" );
  REGISTER_OPCODE( opcode_lst, 'C', MESHFX_IMPASS,    "FXIMPASS" );
  REGISTER_OPCODE( opcode_lst, 'C', MESHFX_DAMAGE,    "FXDAMAGE" );
  REGISTER_OPCODE( opcode_lst, 'C', MESHFX_SLIPPY,    "FXSLIPPY" );
  REGISTER_OPCODE( opcode_lst, 'C', GRIP_SADDLE, "GRIPONLY" );
  REGISTER_OPCODE( opcode_lst, 'C', GRIP_LEFT,   "GRIPLEFT" );
  REGISTER_OPCODE( opcode_lst, 'C', GRIP_RIGHT,  "GRIPRIGHT" );
  REGISTER_OPCODE( opcode_lst, 'C', GRIP_ORIGIN, "SPAWNORIGIN" );
  REGISTER_OPCODE( opcode_lst, 'C', GRIP_LAST,   "SPAWNLAST" );

  REGISTER_OPCODE( opcode_lst, 'C', COLOR_WHITE,  "WHITE" );
  REGISTER_OPCODE( opcode_lst, 'C', COLOR_RED,    "RED" );
  REGISTER_OPCODE( opcode_lst, 'C', COLOR_YELLOW, "YELLOW" );
  REGISTER_OPCODE( opcode_lst, 'C', COLOR_GREEN,  "GREEN" );
  REGISTER_OPCODE( opcode_lst, 'C', COLOR_BLUE,   "BLUE" );
  REGISTER_OPCODE( opcode_lst, 'C', COLOR_PURPLE, "PURPLE" );

  REGISTER_OPCODE( opcode_lst, 'C', MOVE_MELEE,    "MOVEMELEE" );
  REGISTER_OPCODE( opcode_lst, 'C', MOVE_RANGED,   "MOVERANGED" );
  REGISTER_OPCODE( opcode_lst, 'C', MOVE_DISTANCE, "MOVEDISTANCE" );
  REGISTER_OPCODE( opcode_lst, 'C', MOVE_RETREAT,  "MOVERETREAT" );
  REGISTER_OPCODE( opcode_lst, 'C', MOVE_CHARGE,   "MOVECHARGE" );
  REGISTER_OPCODE( opcode_lst, 'C', MOVE_FOLLOW,   "MOVEFOLLOW" );

  REGISTER_OPCODE( opcode_lst, 'C', SEARCH_DEAD,    "BLAHDEAD" );
  REGISTER_OPCODE( opcode_lst, 'C', SEARCH_ENEMIES, "BLAHENEMIES" );
  REGISTER_OPCODE( opcode_lst, 'C', SEARCH_FRIENDS, "BLAHFRIENDS" );
  REGISTER_OPCODE( opcode_lst, 'C', SEARCH_ITEMS,   "BLAHITEMS" );
  REGISTER_OPCODE( opcode_lst, 'C', SEARCH_INVERT,  "BLAHINVERTID" );

  // register all the function opcodes
  REGISTER_FUNCTION( opcode_lst, IfSpawned);
  REGISTER_FUNCTION( opcode_lst, IfTimeOut);
  REGISTER_FUNCTION( opcode_lst, IfAtWaypoint);
  REGISTER_FUNCTION( opcode_lst, IfAtLastWaypoint);
  REGISTER_FUNCTION( opcode_lst, IfAttacked);
  REGISTER_FUNCTION( opcode_lst, IfBumped);
  REGISTER_FUNCTION( opcode_lst, IfSignaled);
  REGISTER_FUNCTION( opcode_lst, IfCalledForHelp);
  REGISTER_FUNCTION( opcode_lst, SetContent);
  REGISTER_FUNCTION( opcode_lst, IfKilled);
  REGISTER_FUNCTION( opcode_lst, IfTargetKilled);
  REGISTER_FUNCTION( opcode_lst, ClearWaypoints);
  REGISTER_FUNCTION( opcode_lst, AddWaypoint);
  REGISTER_FUNCTION( opcode_lst, FindPath);
  REGISTER_FUNCTION( opcode_lst, Compass);
  REGISTER_FUNCTION( opcode_lst, GetTargetArmorPrice);
  REGISTER_FUNCTION( opcode_lst, SetTime);
  REGISTER_FUNCTION( opcode_lst, GetContent);
  REGISTER_FUNCTION( opcode_lst, JoinTargetTeam);
  REGISTER_FUNCTION( opcode_lst, SetTargetToNearbyEnemy);
  REGISTER_FUNCTION( opcode_lst, SetTargetToTargetLeftHand);
  REGISTER_FUNCTION( opcode_lst, SetTargetToTargetRightHand);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWhoeverAttacked);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWhoeverAttacked);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWhoeverBumped);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWhoeverCalledForHelp);
  REGISTER_FUNCTION( opcode_lst, SetTargetToOldTarget);
  REGISTER_FUNCTION( opcode_lst, SetTurnModeToVelocity);
  REGISTER_FUNCTION( opcode_lst, SetTurnModeToWatch);
  REGISTER_FUNCTION( opcode_lst, SetTurnModeToSpin);
  REGISTER_FUNCTION( opcode_lst, SetBumpHeight);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasID);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasItemID);
  REGISTER_FUNCTION( opcode_lst, IfTargetHoldingItemID);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasSkillID);
  REGISTER_FUNCTION( opcode_lst, Else);
  REGISTER_FUNCTION( opcode_lst, Run);
  REGISTER_FUNCTION( opcode_lst, Walk);
  REGISTER_FUNCTION( opcode_lst, Sneak);
  REGISTER_FUNCTION( opcode_lst, DoAction);
  REGISTER_FUNCTION( opcode_lst, KeepAction);
  REGISTER_FUNCTION( opcode_lst, SignalTeam);
  REGISTER_FUNCTION( opcode_lst, DropWeapons);
  REGISTER_FUNCTION( opcode_lst, TargetDoAction);
  REGISTER_FUNCTION( opcode_lst, OpenPassage);
  REGISTER_FUNCTION( opcode_lst, ClosePassage);
  REGISTER_FUNCTION( opcode_lst, IfPassageOpen);
  REGISTER_FUNCTION( opcode_lst, GoPoof);
  REGISTER_FUNCTION( opcode_lst, CostTargetItemID);
  REGISTER_FUNCTION( opcode_lst, DoActionOverride);
  REGISTER_FUNCTION( opcode_lst, IfHealed);
  REGISTER_FUNCTION( opcode_lst, DisplayMessage);
  REGISTER_FUNCTION( opcode_lst, CallForHelp);
  REGISTER_FUNCTION( opcode_lst, AddIDSZ);
  REGISTER_FUNCTION( opcode_lst, End);
  REGISTER_FUNCTION( opcode_lst, SetState);
  REGISTER_FUNCTION( opcode_lst, GetState);
  REGISTER_FUNCTION( opcode_lst, IfStateIs);
  REGISTER_FUNCTION( opcode_lst, IfTargetCanOpenStuff);
  REGISTER_FUNCTION( opcode_lst, IfGrabbed);
  REGISTER_FUNCTION( opcode_lst, IfDropped);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWhoeverIsHolding);
  REGISTER_FUNCTION( opcode_lst, DamageTarget);
  REGISTER_FUNCTION( opcode_lst, IfXIsLessThanY);
  REGISTER_FUNCTION( opcode_lst, SetWeatherTime);
  REGISTER_FUNCTION( opcode_lst, GetBumpHeight);
  REGISTER_FUNCTION( opcode_lst, IfReaffirmed);
  REGISTER_FUNCTION( opcode_lst, UnkeepAction);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsOnOtherTeam);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsOnHatedTeam);
  REGISTER_FUNCTION( opcode_lst, PressLatchButton);
  REGISTER_FUNCTION( opcode_lst, SetTargetToTargetOfLeader);
  REGISTER_FUNCTION( opcode_lst, IfLeaderKilled);
  REGISTER_FUNCTION( opcode_lst, BecomeLeader);
  REGISTER_FUNCTION( opcode_lst, ChangeTargetArmor);
  REGISTER_FUNCTION( opcode_lst, GiveMoneyToTarget);
  REGISTER_FUNCTION( opcode_lst, DropKeys);
  REGISTER_FUNCTION( opcode_lst, IfLeaderIsAlive);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsOldTarget);
  REGISTER_FUNCTION( opcode_lst, SetTargetToLeader);
  REGISTER_FUNCTION( opcode_lst, SpawnCharacter);
  REGISTER_FUNCTION( opcode_lst, RespawnCharacter);
  REGISTER_FUNCTION( opcode_lst, ChangeTile);
  REGISTER_FUNCTION( opcode_lst, IfUsed);
  REGISTER_FUNCTION( opcode_lst, DropMoney);
  REGISTER_FUNCTION( opcode_lst, SetOldTarget);
  REGISTER_FUNCTION( opcode_lst, DetachFromHolder);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasVulnerabilityID);
  REGISTER_FUNCTION( opcode_lst, CleanUp);
  REGISTER_FUNCTION( opcode_lst, IfCleanedUp);
  REGISTER_FUNCTION( opcode_lst, IfSitting);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsHurt);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsAPlayer);
  REGISTER_FUNCTION( opcode_lst, PlaySound);
  REGISTER_FUNCTION( opcode_lst, SpawnParticle);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsAlive);
  REGISTER_FUNCTION( opcode_lst, Stop);
  REGISTER_FUNCTION( opcode_lst, DisaffirmCharacter);
  REGISTER_FUNCTION( opcode_lst, ReaffirmCharacter);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsSelf);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsMale);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsFemale);
  REGISTER_FUNCTION( opcode_lst, SetTargetToSelf);
  REGISTER_FUNCTION( opcode_lst, SetTargetToRider);
  REGISTER_FUNCTION( opcode_lst, GetAttackTurn);
  REGISTER_FUNCTION( opcode_lst, GetDamageType);
  REGISTER_FUNCTION( opcode_lst, BecomeSpell);
  REGISTER_FUNCTION( opcode_lst, BecomeSpellbook);
  REGISTER_FUNCTION( opcode_lst, IfScoredAHit);
  REGISTER_FUNCTION( opcode_lst, IfDisaffirmed);
  REGISTER_FUNCTION( opcode_lst, DecodeOrder);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWhoeverWasHit);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWideEnemy);
  REGISTER_FUNCTION( opcode_lst, IfChanged);
  REGISTER_FUNCTION( opcode_lst, IfInWater);
  REGISTER_FUNCTION( opcode_lst, IfBored);
  REGISTER_FUNCTION( opcode_lst, IfTooMuchBaggage);
  REGISTER_FUNCTION( opcode_lst, IfGrogged);
  REGISTER_FUNCTION( opcode_lst, IfDazed);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasSpecialID);
  REGISTER_FUNCTION( opcode_lst, PressTargetLatchButton);
  REGISTER_FUNCTION( opcode_lst, IfInvisible);
  REGISTER_FUNCTION( opcode_lst, IfArmorIs);
  REGISTER_FUNCTION( opcode_lst, GetTargetGrogTime);
  REGISTER_FUNCTION( opcode_lst, GetTargetDazeTime);
  REGISTER_FUNCTION( opcode_lst, SetDamageType);
  REGISTER_FUNCTION( opcode_lst, SetWaterLevel);
  REGISTER_FUNCTION( opcode_lst, EnchantTarget);
  REGISTER_FUNCTION( opcode_lst, EnchantChild);
  REGISTER_FUNCTION( opcode_lst, TeleportTarget);
  REGISTER_FUNCTION( opcode_lst, GiveExperienceToTarget);
  REGISTER_FUNCTION( opcode_lst, IncreaseAmmo);
  REGISTER_FUNCTION( opcode_lst, UnkurseTarget);
  REGISTER_FUNCTION( opcode_lst, GiveExperienceToTargetTeam);
  REGISTER_FUNCTION( opcode_lst, IfUnarmed);
  REGISTER_FUNCTION( opcode_lst, RestockTargetAmmoIDAll);
  REGISTER_FUNCTION( opcode_lst, RestockTargetAmmoIDFirst);
  REGISTER_FUNCTION( opcode_lst, FlashTarget);
  REGISTER_FUNCTION( opcode_lst, SetRedShift);
  REGISTER_FUNCTION( opcode_lst, SetGreenShift);
  REGISTER_FUNCTION( opcode_lst, SetBlueShift);
  REGISTER_FUNCTION( opcode_lst, SetLight);
  REGISTER_FUNCTION( opcode_lst, SetAlpha);
  REGISTER_FUNCTION( opcode_lst, IfHitFromBehind);
  REGISTER_FUNCTION( opcode_lst, IfHitFromFront);
  REGISTER_FUNCTION( opcode_lst, IfHitFromLeft);
  REGISTER_FUNCTION( opcode_lst, IfHitFromRight);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsOnSameTeam);
  REGISTER_FUNCTION( opcode_lst, KillTarget);
  REGISTER_FUNCTION( opcode_lst, UndoEnchant);
  REGISTER_FUNCTION( opcode_lst, GetWaterLevel);
  REGISTER_FUNCTION( opcode_lst, CostTargetMana);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasAnyID);
  REGISTER_FUNCTION( opcode_lst, SetBumpSize);
  REGISTER_FUNCTION( opcode_lst, IfNotDropped);
  REGISTER_FUNCTION( opcode_lst, IfYIsLessThanX);
  REGISTER_FUNCTION( opcode_lst, IfYIsLessThanX);
  REGISTER_FUNCTION( opcode_lst, SetFlyHeight);
  REGISTER_FUNCTION( opcode_lst, IfBlocked);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsDefending);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsAttacking);
  REGISTER_FUNCTION( opcode_lst, IfStateIs0);
  REGISTER_FUNCTION( opcode_lst, IfStateIs0);
  REGISTER_FUNCTION( opcode_lst, IfStateIs1);
  REGISTER_FUNCTION( opcode_lst, IfStateIs1);
  REGISTER_FUNCTION( opcode_lst, IfStateIs2);
  REGISTER_FUNCTION( opcode_lst, IfStateIs2);
  REGISTER_FUNCTION( opcode_lst, IfStateIs3);
  REGISTER_FUNCTION( opcode_lst, IfStateIs3);
  REGISTER_FUNCTION( opcode_lst, IfStateIs4);
  REGISTER_FUNCTION( opcode_lst, IfStateIs4);
  REGISTER_FUNCTION( opcode_lst, IfStateIs5);
  REGISTER_FUNCTION( opcode_lst, IfStateIs5);
  REGISTER_FUNCTION( opcode_lst, IfStateIs6);
  REGISTER_FUNCTION( opcode_lst, IfStateIs6);
  REGISTER_FUNCTION( opcode_lst, IfStateIs7);
  REGISTER_FUNCTION( opcode_lst, IfStateIs7);
  REGISTER_FUNCTION( opcode_lst, IfContentIs);
  REGISTER_FUNCTION( opcode_lst, SetTurnModeToWatchTarget);
  REGISTER_FUNCTION( opcode_lst, IfStateIsNot);
  REGISTER_FUNCTION( opcode_lst, IfXIsEqualToY);
  REGISTER_FUNCTION( opcode_lst, DisplayDebugMessage);
  REGISTER_FUNCTION( opcode_lst, BlackTarget);
  REGISTER_FUNCTION( opcode_lst, DisplayMessageNear);
  REGISTER_FUNCTION( opcode_lst, IfHitGround);
  REGISTER_FUNCTION( opcode_lst, IfNameIsKnown);
  REGISTER_FUNCTION( opcode_lst, IfUsageIsKnown);
  REGISTER_FUNCTION( opcode_lst, IfHoldingItemID);
  REGISTER_FUNCTION( opcode_lst, IfHoldingRangedWeapon);
  REGISTER_FUNCTION( opcode_lst, IfHoldingMeleeWeapon);
  REGISTER_FUNCTION( opcode_lst, IfHoldingShield);
  REGISTER_FUNCTION( opcode_lst, IfKursed);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsKursed);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsDressedUp);
  REGISTER_FUNCTION( opcode_lst, IfOverWater);
  REGISTER_FUNCTION( opcode_lst, IfThrown);
  REGISTER_FUNCTION( opcode_lst, MakeNameKnown);
  REGISTER_FUNCTION( opcode_lst, MakeUsageKnown);
  REGISTER_FUNCTION( opcode_lst, StopTargetMovement);
  REGISTER_FUNCTION( opcode_lst, SetXY);
  REGISTER_FUNCTION( opcode_lst, GetXY);
  REGISTER_FUNCTION( opcode_lst, AddXY);
  REGISTER_FUNCTION( opcode_lst, MakeAmmoKnown);
  REGISTER_FUNCTION( opcode_lst, SpawnAttachedParticle);
  REGISTER_FUNCTION( opcode_lst, SpawnExactParticle);
  REGISTER_FUNCTION( opcode_lst, AccelerateTarget);
  REGISTER_FUNCTION( opcode_lst, IfDistanceIsMoreThanTurn);
  REGISTER_FUNCTION( opcode_lst, IfCrushed);
  REGISTER_FUNCTION( opcode_lst, MakeCrushValid);
  REGISTER_FUNCTION( opcode_lst, SetTargetToLowestTarget);
  REGISTER_FUNCTION( opcode_lst, IfNotPutAway);
  REGISTER_FUNCTION( opcode_lst, IfNotPutAway);
  REGISTER_FUNCTION( opcode_lst, IfTakenOut);
  REGISTER_FUNCTION( opcode_lst, IfAmmoOut);
  REGISTER_FUNCTION( opcode_lst, PlaySoundLooped);
  REGISTER_FUNCTION( opcode_lst, StopSoundLoop);
  REGISTER_FUNCTION( opcode_lst, HealSelf);
  REGISTER_FUNCTION( opcode_lst, Equip);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasItemIDEquipped);
  REGISTER_FUNCTION( opcode_lst, SetOwnerToTarget);
  REGISTER_FUNCTION( opcode_lst, SetTargetToOwner);
  REGISTER_FUNCTION( opcode_lst, SetFrame);
  REGISTER_FUNCTION( opcode_lst, BreakPassage);
  REGISTER_FUNCTION( opcode_lst, SetReloadTime);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWideBlahID);
  REGISTER_FUNCTION( opcode_lst, PoofTarget);
  REGISTER_FUNCTION( opcode_lst, ChildDoActionOverride);
  REGISTER_FUNCTION( opcode_lst, SpawnPoof);
  REGISTER_FUNCTION( opcode_lst, SetSpeedPercent);
  REGISTER_FUNCTION( opcode_lst, SetChildState);
  REGISTER_FUNCTION( opcode_lst, SpawnAttachedSizedParticle);
  REGISTER_FUNCTION( opcode_lst, ChangeArmor);
  REGISTER_FUNCTION( opcode_lst, ShowTimer);
  REGISTER_FUNCTION( opcode_lst, IfFacingTarget);
  REGISTER_FUNCTION( opcode_lst, PlaySoundVolume);
  REGISTER_FUNCTION( opcode_lst, SpawnAttachedFacedParticle);
  REGISTER_FUNCTION( opcode_lst, IfStateIsOdd);
  REGISTER_FUNCTION( opcode_lst, SetTargetToDistantEnemy);
  REGISTER_FUNCTION( opcode_lst, Teleport);
  REGISTER_FUNCTION( opcode_lst, GiveStrengthToTarget);
  REGISTER_FUNCTION( opcode_lst, GiveWisdomToTarget);
  REGISTER_FUNCTION( opcode_lst, GiveIntelligenceToTarget);
  REGISTER_FUNCTION( opcode_lst, GiveDexterityToTarget);
  REGISTER_FUNCTION( opcode_lst, GiveLifeToTarget);
  REGISTER_FUNCTION( opcode_lst, GiveManaToTarget);
  REGISTER_FUNCTION( opcode_lst, ShowMap);
  REGISTER_FUNCTION( opcode_lst, ShowYouAreHere);
  REGISTER_FUNCTION( opcode_lst, ShowBlipXY);
  REGISTER_FUNCTION( opcode_lst, HealTarget);
  REGISTER_FUNCTION( opcode_lst, PumpTarget);
  REGISTER_FUNCTION( opcode_lst, CostAmmo);
  REGISTER_FUNCTION( opcode_lst, MakeSimilarNamesKnown);
  REGISTER_FUNCTION( opcode_lst, SpawnAttachedHolderParticle);
  REGISTER_FUNCTION( opcode_lst, SetTargetReloadTime);
  REGISTER_FUNCTION( opcode_lst, SetFogLevel);
  REGISTER_FUNCTION( opcode_lst, GetFogLevel);
  REGISTER_FUNCTION( opcode_lst, SetFogTAD);
  REGISTER_FUNCTION( opcode_lst, SetFogBottomLevel);
  REGISTER_FUNCTION( opcode_lst, GetFogBottomLevel);
  REGISTER_FUNCTION( opcode_lst, CorrectActionForHand);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsMounted);
  REGISTER_FUNCTION( opcode_lst, SparkleIcon);
  REGISTER_FUNCTION( opcode_lst, UnsparkleIcon);
  REGISTER_FUNCTION( opcode_lst, GetTileXY);
  REGISTER_FUNCTION( opcode_lst, SetTileXY);
  REGISTER_FUNCTION( opcode_lst, SetShadowSize);
  REGISTER_FUNCTION( opcode_lst, SignalTarget);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWhoeverIsInPassage);
  REGISTER_FUNCTION( opcode_lst, IfCharacterWasABook);
  REGISTER_FUNCTION( opcode_lst, SetEnchantBoostValues);
  REGISTER_FUNCTION( opcode_lst, SpawnCharacterXYZ);
  REGISTER_FUNCTION( opcode_lst, SpawnExactCharacterXYZ);
  REGISTER_FUNCTION( opcode_lst, ChangeTargetClass);
  REGISTER_FUNCTION( opcode_lst, PlayFullSound);
  REGISTER_FUNCTION( opcode_lst, SpawnExactChaseParticle);
  REGISTER_FUNCTION( opcode_lst, EncodeOrder);
  REGISTER_FUNCTION( opcode_lst, SignalSpecialID);
  REGISTER_FUNCTION( opcode_lst, UnkurseTargetInventory);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsSneaking);
  REGISTER_FUNCTION( opcode_lst, DropItems);
  REGISTER_FUNCTION( opcode_lst, RespawnTarget);
  REGISTER_FUNCTION( opcode_lst, TargetDoActionSetFrame);
  REGISTER_FUNCTION( opcode_lst, IfTargetCanSeeInvisible);
  REGISTER_FUNCTION( opcode_lst, SetTargetToNearestBlahID);
  REGISTER_FUNCTION( opcode_lst, SetTargetToNearestEnemy);
  REGISTER_FUNCTION( opcode_lst, SetTargetToNearestFriend);
  REGISTER_FUNCTION( opcode_lst, SetTargetToNearestLifeform);
  REGISTER_FUNCTION( opcode_lst, FlashPassage);
  REGISTER_FUNCTION( opcode_lst, FindTileInPassage);
  REGISTER_FUNCTION( opcode_lst, IfHeldInLeftSaddle);
  REGISTER_FUNCTION( opcode_lst, NotAnItem);
  REGISTER_FUNCTION( opcode_lst, SetChildAmmo);
  REGISTER_FUNCTION( opcode_lst, IfHitVulnerable);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsFlying);
  REGISTER_FUNCTION( opcode_lst, IdentifyTarget);
  REGISTER_FUNCTION( opcode_lst, BeatModule);
  REGISTER_FUNCTION( opcode_lst, EndModule);
  REGISTER_FUNCTION( opcode_lst, DisableExport);
  REGISTER_FUNCTION( opcode_lst, EnableExport);
  REGISTER_FUNCTION( opcode_lst, GetTargetState);
  REGISTER_FUNCTION( opcode_lst, SetSpeech);
  REGISTER_FUNCTION( opcode_lst, SetMoveSpeech);
  REGISTER_FUNCTION( opcode_lst, SetSecondMoveSpeech);
  REGISTER_FUNCTION( opcode_lst, SetAttackSpeech);
  REGISTER_FUNCTION( opcode_lst, SetAssistSpeech);
  REGISTER_FUNCTION( opcode_lst, SetTerrainSpeech);
  REGISTER_FUNCTION( opcode_lst, SetSelectSpeech);
  REGISTER_FUNCTION( opcode_lst, ClearEndText);
  REGISTER_FUNCTION( opcode_lst, AddEndText);
  REGISTER_FUNCTION( opcode_lst, PlayMusic);
  REGISTER_FUNCTION( opcode_lst, SetMusicPassage);
  REGISTER_FUNCTION( opcode_lst, MakeCrushInvalid);
  REGISTER_FUNCTION( opcode_lst, StopMusic);
  REGISTER_FUNCTION( opcode_lst, FlashVariable);
  REGISTER_FUNCTION( opcode_lst, AccelerateUp);
  REGISTER_FUNCTION( opcode_lst, FlashVariableHeight);
  REGISTER_FUNCTION( opcode_lst, SetDamageTime);
  REGISTER_FUNCTION( opcode_lst, IfStateIs8);
  REGISTER_FUNCTION( opcode_lst, IfStateIs9);
  REGISTER_FUNCTION( opcode_lst, IfStateIs10);
  REGISTER_FUNCTION( opcode_lst, IfStateIs11);
  REGISTER_FUNCTION( opcode_lst, IfStateIs12);
  REGISTER_FUNCTION( opcode_lst, IfStateIs13);
  REGISTER_FUNCTION( opcode_lst, IfStateIs14);
  REGISTER_FUNCTION( opcode_lst, IfStateIs15);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsAMount);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsAPlatform);
  REGISTER_FUNCTION( opcode_lst, AddStat);
  REGISTER_FUNCTION( opcode_lst, DisenchantTarget);
  REGISTER_FUNCTION( opcode_lst, DisenchantAll);
  REGISTER_FUNCTION( opcode_lst, SetVolumeNearestTeammate);
  REGISTER_FUNCTION( opcode_lst, AddShopPassage);
  REGISTER_FUNCTION( opcode_lst, TargetPayForArmor);
  REGISTER_FUNCTION( opcode_lst, JoinEvilTeam);
  REGISTER_FUNCTION( opcode_lst, JoinNullTeam);
  REGISTER_FUNCTION( opcode_lst, JoinGoodTeam);
  REGISTER_FUNCTION( opcode_lst, PitsKill);
  REGISTER_FUNCTION( opcode_lst, SetTargetToPassageID);
  REGISTER_FUNCTION( opcode_lst, MakeNameUnknown);
  REGISTER_FUNCTION( opcode_lst, SpawnExactParticleEndSpawn);
  REGISTER_FUNCTION( opcode_lst, SpawnPoofSpeedSpacingDamage);
  REGISTER_FUNCTION( opcode_lst, GiveExperienceToGoodTeam);
  REGISTER_FUNCTION( opcode_lst, DoNothing);
  REGISTER_FUNCTION( opcode_lst, DazeTarget);
  REGISTER_FUNCTION( opcode_lst, GrogTarget);
  REGISTER_FUNCTION( opcode_lst, AddQuest);
  REGISTER_FUNCTION( opcode_lst, BeatQuest);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasQuest);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasNotFullMana);
  REGISTER_FUNCTION( opcode_lst, IfJumping);
  REGISTER_FUNCTION( opcode_lst, DropTargetKeys);
  REGISTER_FUNCTION( opcode_lst, TargetJoinTeam);
  REGISTER_FUNCTION( opcode_lst, GetTargetContent);
  REGISTER_FUNCTION( opcode_lst, JoinTeam);
  REGISTER_FUNCTION( opcode_lst, TargetJoinTeam);
  REGISTER_FUNCTION( opcode_lst, ClearMusicPassage);
  REGISTER_FUNCTION( opcode_lst, IfOperatorIsLinux);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsOwner);
//  REGISTER_FUNCTION( opcode_lst, SetCameraSwing);

  // register all the function !!!ALIASES!!!
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfAtLastWaypoint, "IfPutAway" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfSignaled, "IfOrdered" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, SetTargetToWhoeverAttacked, "SetTargetToWhoeverHealed" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, SignalTeam, "IssueOrder" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, DisplayMessage, "SendMessage" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfGrabbed, "IfMounted" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfDropped, "IfDismounted" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfXIsLessThanY, "IfYIsMoreThanX" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfSitting, "IfHeld" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, DecodeOrder, "TranslateOrder" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfYIsLessThanX, "IfXIsMoreThanY" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs0, "IfStateIsParry" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs1, "IfStateIsWander" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs2, "IfStateIsGuard" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs3, "IfStateIsFollow" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs4, "IfStateIsSurround" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs5, "IfStateIsRetreat" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs6, "IfStateIsCharge" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs7, "IfStateIsCombat" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfXIsEqualToY, "IfYIsEqualToX" ); 
  REGISTER_FUNCTION_ALIAS( opcode_lst, DisplayDebugMessage, "DebugMessage" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, DisplayMessageNear, "SendMessageNear" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfNotPutAway, "IfNotTakenOut" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, SignalTarget, "OrderTarget" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, EncodeOrder, "CreateOrder" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, SignalSpecialID, "OrderSpecialID" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfHeldInLeftSaddle, "IfHeldInLeftHand" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, ClearEndText, "ClearEndMessage" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, AddEndText, "AddEndMessage" );
//  REGISTER_FUNCTION_ALIAS( opcode_lst, IfJumping, "IfTargetJumping" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, StopSoundLoop, "StopSound" );

  log_info( "load_ai_codes() - loading external script constants\n" );

  // read all of the remaining opcodes from the file
  fileread = fs_fileOpen( PRI_NONE, NULL, loadname, "rb" );
  if ( NULL == fileread )
  {
    log_warning( "Could not load script AI functions or variables (%s)\n", loadname );
    return;
  }

  while ( fgoto_colon_yesno( fileread ) && !feof( fileread ) )
  {
    fget_code( fileread );
  }

  fs_fileClose( fileread );
}


Uint32 load_ai_script( char *loadname )
{
  // ZZ> This function loads a script to memory and
  //     returns bfalse if it fails to do so
  FILE* fileread;
  Uint32 retval = MAXAI;

  scr_intern.line_num = 0;
  globalparsename = loadname;  // For error logging in log.TXT
  fileread = fs_fileOpen( PRI_NONE, NULL, loadname, "rb" );
  if ( NULL == fileread || scr_intern.script_count >= MAXAI )
    return retval;

  // Make room for the code
  retval = scr_intern.script_count;
  scr_intern.script_address[scr_intern.script_count] = scr_intern.script_index;
  scr_intern.script_count++;

  // Load into md2 load buffer
  scr_intern.load_size = ( int ) fread( &cLoadBuffer[0], 1, MD2MAXLOADSIZE, fileread );
  fs_fileClose( fileread );

  parse_null_terminate_comments();
  parse_line_by_line();
  parse_jumps( retval );

  return retval;
}

//------------------------------------------------------------------------------
void reset_ai_script()
{
  // ZZ> This function starts ai loading in the right spot
  int cnt;

  scr_intern.script_index = 0;
  for ( cnt = 0; cnt < MAXMODEL; cnt++ )
    madai[cnt] = 0;

  scr_intern.script_count = 0;
}

//--------------------------------------------------------------------------------------------
bool_t run_function( Uint32 value, CHR_REF ichr )
{
  // ZZ> This function runs a script function for the AI.
  //     It returns bfalse if the script should jump over the
  //     indented code that follows

  // Mask out the indentation
  OPCODE valuecode = GET_OPCODE_BITS(value);

  // Assume that the function will pass, as most do
  bool_t returncode = btrue;

  Uint16 sTmp;
  float fTmp;
  int iTmp, tTmp;
  int volume;
  Uint32 test;
  STRING cTmp;

  CHR_REF loc_aitarget = chr_get_aitarget( ichr );
  CHR_REF loc_aiowner  = chr_get_aiowner( ichr );
  CHR_REF loc_aichild  = chr_get_aichild( ichr );
  CHR_REF loc_leader   = team_get_leader( chrteam[ichr] );

  Uint16  loc_model    = chrmodel[ichr];

  // Figure out which function to run
  switch ( valuecode )
  {
    case F_IfSpawned:
      // Proceed only if it's a new character
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_SPAWNED );
      break;

    case F_IfTimeOut:
      // Proceed only if time alert is set
      returncode = ( chraitime[ichr] == 0.0f );
      break;

    case F_IfAtWaypoint:
      // Proceed only if the character reached a waypoint
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_ATWAYPOINT );
      break;

    case F_IfAtLastWaypoint:
      // Proceed only if the character reached its last waypoint
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_ATLASTWAYPOINT );
      break;

    case F_IfAttacked:
      // Proceed only if the character was damaged
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_ATTACKED );
      break;

    case F_IfBumped:
      // Proceed only if the character was bumped
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_BUMPED );
      break;

    case F_IfSignaled:
      // Proceed only if the character was ordered
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_SIGNALED );
      break;

    case F_IfCalledForHelp:
      // Proceed only if the character was called for help
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_CALLEDFORHELP );
      break;

    case F_SetContent:
      // Set the content
      chraicontent[ichr] = scr_globals.tmpargument;
      break;

    case F_IfKilled:
      // Proceed only if the character's been killed
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_KILLED );
      break;

    case F_IfTargetKilled:
      // Proceed only if the character's target has just died
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_TARGETKILLED );
      break;

    case F_ClearWaypoints:
      // Clear out all waypoints
      chraigoto[ichr] = 0;
      chraigotoadd[ichr] = 0;
      chraigotox[ichr][0] = chrpos[ichr].x;
      chraigotoy[ichr][0] = chrpos[ichr].y;
      break;

    case F_AddWaypoint:
      // Add a waypoint to the waypoint list
      chraigotox[ichr][chraigotoadd[ichr]] = scr_globals.tmpx;
      chraigotoy[ichr][chraigotoadd[ichr]] = scr_globals.tmpy;
      chraigotoadd[ichr]++;
      if ( chraigotoadd[ichr] > MAXWAY )  chraigotoadd[ichr] = MAXWAY - 1;
      break;

    case F_FindPath:
      // This function adds enough waypoints to get from one point to another
	  // And only proceeds if the target is not the character himself
      // !!!BAD!!! Todo: Only adds one straight waypoint...

	  //First setup the variables needed for the target waypoint
	  if(loc_aitarget != ichr)
	  {
	    if(scr_globals.tmpdistance != MOVE_FOLLOW)
		{
			scr_globals.tmpx = chrpos[loc_aitarget].x;
			scr_globals.tmpy = chrpos[loc_aitarget].y;
		}
		else
		{
			scr_globals.tmpx = (rand() & 1023) - 512 + chrpos[loc_aitarget].x;
			scr_globals.tmpy = (rand() & 1023) - 512 + chrpos[loc_aitarget].y;
		}
		if(scr_globals.tmpdistance == MOVE_RETREAT) scr_globals.tmpturn = (rand() & 32767) + scr_globals.tmpturn + 16384;
		else scr_globals.tmpturn = vec_to_turn( scr_globals.tmpx - chrpos[ichr].x, scr_globals.tmpy - chrpos[ichr].y );
		if(scr_globals.tmpdistance == MOVE_CHARGE || MOVE_RETREAT) reset_character_accel( ichr ); //Force 100% speed

		//Secondly we run the Compass function (If we are not in follow mode)
		if(scr_globals.tmpdistance != MOVE_FOLLOW)
		{
			sTmp = ( scr_globals.tmpturn + 16384 );
			scr_globals.tmpx -= turntosin[( sTmp>>2 ) & TRIGTABLE_MASK] * scr_globals.tmpdistance;
			scr_globals.tmpy -= turntosin[( scr_globals.tmpturn>>2 ) & TRIGTABLE_MASK] * scr_globals.tmpdistance;
		}

		//Then we add the waypoint(s), without clearing existing ones...
		chraigotox[ichr][chraigotoadd[ichr]] = scr_globals.tmpx;
		chraigotoy[ichr][chraigotoadd[ichr]] = scr_globals.tmpy;
		chraigotoadd[ichr]++;
		if ( chraigotoadd[ichr] > MAXWAY )  chraigotoadd[ichr] = MAXWAY - 1;
	  }
	break;

    case F_Compass:
      // This function changes tmpx and tmpy in a circlular manner according
      // to tmpturn and tmpdistance
      sTmp = ( scr_globals.tmpturn + 16384 );
      scr_globals.tmpx -= turntosin[( sTmp>>2 ) & TRIGTABLE_MASK] * scr_globals.tmpdistance;
      scr_globals.tmpy -= turntosin[( scr_globals.tmpturn>>2 ) & TRIGTABLE_MASK] * scr_globals.tmpdistance;
      break;

    case F_GetTargetArmorPrice:
      // This function gets the armor cost for the given skin
      sTmp = scr_globals.tmpargument  % MAXSKIN;
      scr_globals.tmpx = capskincost[chrmodel[loc_aitarget]][sTmp];
      break;

    case F_SetTime:
      // This function resets the time
      chraitime[ichr] = scr_globals.tmpargument;
      break;

    case F_GetContent:
      // Get the content
      scr_globals.tmpargument = chraicontent[ichr];
      break;

    case F_JoinTargetTeam:
      // This function allows the character to leave its own team and join another
      returncode = bfalse;
      if ( chron[loc_aitarget] )
      {
        switch_team( ichr, chrteam[loc_aitarget] );
        returncode = btrue;
      }
      break;

    case F_SetTargetToNearbyEnemy:
      // This function finds a nearby enemy, and proceeds only if there is one
      sTmp = chr_search_nearby_target( ichr, bfalse, bfalse, btrue, bfalse, IDSZ_NONE );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        chraitarget[ichr] = sTmp;
        returncode = btrue;
      }
      break;

    case F_SetTargetToTargetLeftHand:
      // This function sets the target to the target's left item
      sTmp = chr_get_holdingwhich( loc_aitarget, SLOT_LEFT );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        chraitarget[ichr] = sTmp;
        returncode = btrue;
      }
      break;

    case F_SetTargetToTargetRightHand:
      // This function sets the target to the target's right item
      sTmp = chr_get_holdingwhich( loc_aitarget, SLOT_RIGHT );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        chraitarget[ichr] = sTmp;
        returncode = btrue;
      }
      break;

    case F_SetTargetToWhoeverAttacked:
      {
        // This function sets the target to whoever attacked the character last,
        // failing for damage tiles
        CHR_REF attacklast = chr_get_aiattacklast( ichr );
        if ( VALID_CHR( attacklast ) )
        {
          chraitarget[ichr] = attacklast;
        }
        else
        {
          returncode = bfalse;
        }
      }
      break;

    case F_SetTargetToWhoeverBumped:
      // This function sets the target to whoever bumped into the
      // character last.  It never fails
      chraitarget[ichr] = chr_get_aibumplast( ichr );
      break;

    case F_SetTargetToWhoeverCalledForHelp:
      // This function sets the target to whoever needs help
      chraitarget[ichr] = team_get_sissy( chrteam[ichr] );
      break;

    case F_SetTargetToOldTarget:
      // This function reverts to the target with whom the script started
      chraitarget[ichr] = scr_globals.oldtarget;
      break;

    case F_SetTurnModeToVelocity:
      // This function sets the turn mode
      chrturnmode[ichr] = TURNMODE_VELOCITY;
      break;

    case F_SetTurnModeToWatch:
      // This function sets the turn mode
      chrturnmode[ichr] = TURNMODE_WATCH;
      break;

    case F_SetTurnModeToSpin:
      // This function sets the turn mode
      chrturnmode[ichr] = TURNMODE_SPIN;
      break;

    case F_SetBumpHeight:
      // This function changes a character's bump height
      //chrbmpdata[ichr].height = scr_globals.tmpargument * chrfat[ichr];
      //chrbmpdata_save[ichr].height = scr_globals.tmpargument;
      break;

    case F_IfTargetHasID:
      // This function proceeds if ID matches tmpargument
      returncode = bfalse;
      if ( VALID_CHR( loc_aitarget ) )
      {
        returncode = CAP_INHERIT_IDSZ( chrmodel[loc_aitarget], scr_globals.tmpargument );
      };
      break;

    case F_IfTargetHasItemID:
      // This function proceeds if the target has a matching item in his/her pack
      returncode = bfalse;
      // Check the pack
      sTmp  = chr_get_nextinpack( loc_aitarget );
      while ( VALID_CHR( sTmp ) )
      {
        if ( CAP_INHERIT_IDSZ( chrmodel[sTmp], scr_globals.tmpargument ) )
        {
          returncode = btrue;
          break;
        }
        sTmp  = chr_get_nextinpack( sTmp );
      }

      for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
      {
        sTmp = chr_get_holdingwhich( loc_aitarget, _slot );
        if ( VALID_CHR( sTmp ) )
        {
          if ( CAP_INHERIT_IDSZ( chrmodel[sTmp], scr_globals.tmpargument ) )
            returncode = btrue;
        }
      }
      break;

    case F_IfTargetHoldingItemID:
      // This function proceeds if ID matches tmpargument and returns the latch for the
      // hand in tmpargument
      returncode = bfalse;

      if ( VALID_CHR( loc_aitarget ) )
      {
        for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
        {
          sTmp = chr_get_holdingwhich( loc_aitarget, _slot );
          if ( VALID_CHR( sTmp ) && CAP_INHERIT_IDSZ( chrmodel[sTmp], scr_globals.tmpargument ) )
          {
            scr_globals.tmpargument = slot_to_latch( loc_aitarget, _slot );
            returncode = btrue;
          }
        }
      }
      break;

    case F_IfTargetHasSkillID:
      // This function proceeds if ID matches tmpargument
      returncode = check_skills( loc_aitarget, scr_globals.tmpargument );
      break;

    case F_Else:
      // This function fails if the last one was more indented
      if (( scr_globals.lastindent&0x78000000 ) > ( value&0x78000000 ) )
        returncode = bfalse;
      break;

    case F_Run:
      reset_character_accel( ichr );
      break;

    case F_Walk:
      reset_character_accel( ichr );
      chrmaxaccel[ichr] *= .66;
      break;

    case F_Sneak:
      reset_character_accel( ichr );
      chrmaxaccel[ichr] *= .33;
      break;

    case F_DoAction:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid or if the character is doing
      // something else already
      returncode = bfalse;
      if ( scr_globals.tmpargument < MAXACTION && chractionready[ichr] )
      {
        if ( madactionvalid[loc_model][scr_globals.tmpargument] )
        {
          chraction[ichr] = scr_globals.tmpargument;
          chrlip_fp8[ichr] = 0;
          chrflip[ichr] = 0.0f;
          chrframelast[ichr] = chrframe[ichr];
          chrframe[ichr] = madactionstart[loc_model][scr_globals.tmpargument];
          chractionready[ichr] = bfalse;
          returncode = btrue;
        }
      }
      break;

    case F_KeepAction:
      // This function makes the current animation halt on the last frame
      chrkeepaction[ichr] = btrue;
      break;

    case F_SignalTeam:
      // This function issues an order to all teammates
      signal_team( ichr, scr_globals.tmpargument );
      break;

    case F_DropWeapons:
      // This funtion drops the character's in hand items/riders

      for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
      {
        sTmp = chr_get_holdingwhich( ichr, _slot );
        if ( detach_character_from_mount( sTmp, btrue, btrue ) )
        {
          if ( _slot == SLOT_SADDLE )
          {
            chrvel[sTmp].z     = DISMOUNTZVEL;
            chrpos[sTmp].z    += DISMOUNTZVEL;
            chrjumptime[sTmp] = DELAY_JUMP;
          }
        }
      };
      break;

    case F_TargetDoAction:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid or if the target is doing
      // something else already
      returncode = bfalse;
      if ( chralive[loc_aitarget] )
      {
        if ( scr_globals.tmpargument < MAXACTION && chractionready[loc_aitarget] )
        {
          if ( madactionvalid[chrmodel[loc_aitarget]][scr_globals.tmpargument] )
          {
            chraction[loc_aitarget] = scr_globals.tmpargument;
            chrlip_fp8[loc_aitarget] = 0;
            chrflip[loc_aitarget] = 0.0f;
            chrframelast[loc_aitarget] = chrframe[loc_aitarget];
            chrframe[loc_aitarget] = madactionstart[chrmodel[loc_aitarget]][scr_globals.tmpargument];
            chractionready[loc_aitarget] = bfalse;
            returncode = btrue;
          }
        }
      }
      break;

    case F_OpenPassage:
      // This function opens the passage specified by tmpargument, failing if the
      // passage was already open
      returncode = open_passage( scr_globals.tmpargument );
      break;

    case F_ClosePassage:
      // This function closes the passage specified by tmpargument, and proceeds
      // only if the passage is clear of obstructions
      returncode = close_passage( scr_globals.tmpargument );
      break;

    case F_IfPassageOpen:
      // This function proceeds only if the passage specified by tmpargument
      // is both valid and open
      returncode = bfalse;
      if ( scr_globals.tmpargument < numpassage && scr_globals.tmpargument >= 0 )
      {
        returncode = passopen[scr_globals.tmpargument];
      }
      break;

    case F_GoPoof:
      // This function flags the character to be removed from the game
      returncode = bfalse;
      if ( !chrisplayer[ichr] )
      {
        returncode = btrue;
        chrgopoof[ichr] = btrue;
      }
      break;

    case F_CostTargetItemID:
      // This function checks if the target has a matching item, and poofs it
      returncode = bfalse;
      // Check the pack
      iTmp = MAXCHR;
      tTmp = chr_get_aitarget( ichr );
      sTmp  = chr_get_nextinpack( tTmp );
      while ( VALID_CHR( sTmp ) )
      {
        if ( CAP_INHERIT_IDSZ( chrmodel[sTmp], scr_globals.tmpargument ) )
        {
          returncode = btrue;
          iTmp = sTmp;
          break;
        }
        else
        {
          tTmp = sTmp;
          sTmp  = chr_get_nextinpack( sTmp );
        }
      }

      for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
      {
        sTmp = chr_get_holdingwhich( loc_aitarget, _slot );
        if ( VALID_CHR( sTmp ) )
        {
          if ( CAP_INHERIT_IDSZ( chrmodel[sTmp], scr_globals.tmpargument ) )
          {
            returncode = btrue;
            tTmp = MAXCHR;
            iTmp = chr_get_holdingwhich( loc_aitarget, _slot );
            break;
          }
        }
      };


      if ( returncode )
      {
        if ( chrammo[iTmp] <= 1 )
        {
          // Poof the item
          if ( chr_in_pack( iTmp ) )
          {
            // Remove from the pack
            chrnextinpack[tTmp]  = chr_get_nextinpack( iTmp );
            chrnuminpack[loc_aitarget]--;
            chrfreeme[iTmp] = btrue;
          }
          else
          {
            // Drop from hand
            detach_character_from_mount( iTmp, btrue, bfalse );
            chrfreeme[iTmp] = btrue;
          }
        }
        else
        {
          // Cost one ammo
          chrammo[iTmp]--;
        }
      }
      break;

    case F_DoActionOverride:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid
      returncode = bfalse;
      if ( scr_globals.tmpargument < MAXACTION )
      {
        if ( madactionvalid[loc_model][scr_globals.tmpargument] )
        {
          chraction[ichr] = scr_globals.tmpargument;
          chrlip_fp8[ichr] = 0;
          chrflip[ichr] = 0.0f;
          chrframelast[ichr] = chrframe[ichr];
          chrframe[ichr] = madactionstart[loc_model][scr_globals.tmpargument];
          chractionready[ichr] = bfalse;
          returncode = btrue;
        }
      }
      break;

    case F_IfHealed:
      // Proceed only if the character was healed
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_HEALED );
      break;

    case F_DisplayMessage:
      // This function sends a message to the players
      display_message( madmsgstart[loc_model] + scr_globals.tmpargument, ichr );
      break;

    case F_CallForHelp:
      // This function issues a call for help
      call_for_help( ichr );
      break;

    case F_AddIDSZ:
      // This function adds an idsz to the module's menu.txt file
      add_module_idsz( pickedmodule, scr_globals.tmpargument );
      break;

    case F_SetState:
      // This function sets the character's state variable
      chraistate[ichr] = scr_globals.tmpargument;
      break;

    case F_GetState:
      // This function reads the character's state variable
      scr_globals.tmpargument = chraistate[ichr];
      break;

    case F_IfStateIs:
      // This function fails if the character's state is inequal to tmpargument
      returncode = ( scr_globals.tmpargument == chraistate[ichr] );
      break;

    case F_IfTargetCanOpenStuff:
      // This function fails if the target can't open stuff
      returncode = chropenstuff[loc_aitarget];
      break;

    case F_IfGrabbed:
      // Proceed only if the character was picked up
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_GRABBED );
      break;

    case F_IfDropped:
      // Proceed only if the character was dropped
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_DROPPED );
      break;

    case F_SetTargetToWhoeverIsHolding:
      // This function sets the target to the character's mount or holder,
      // failing if the character has no mount or holder
      returncode = bfalse;
      if ( chr_attached( ichr ) )
      {
        chraitarget[ichr] = chr_get_attachedto( ichr );
        returncode = btrue;
      }
      break;

    case F_DamageTarget:
      {
        // This function applies little bit of love to the character's target.
        // The amount is set in tmpargument
        PAIR ptemp = {scr_globals.tmpargument, 1};
        damage_character( loc_aitarget, 0, &ptemp, chrdamagetargettype[ichr], chrteam[ichr], ichr, DAMFX_BLOC );
      }
      break;

    case F_IfXIsLessThanY:
      // Proceed only if tmpx is less than tmpy
      returncode = ( scr_globals.tmpx < scr_globals.tmpy );
      break;

    case F_SetWeatherTime:
      // Set the weather timer
      weathertimereset = scr_globals.tmpargument;
      weathertime = scr_globals.tmpargument;
      break;

    case F_GetBumpHeight:
      // Get the characters bump height
      scr_globals.tmpargument = chrbmpdata[ichr].calc_height;
      break;

    case F_IfReaffirmed:
      // Proceed only if the character was reaffirmed
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_REAFFIRMED );
      break;

    case F_UnkeepAction:
      // This function makes the current animation start again
      chrkeepaction[ichr] = bfalse;
      break;

    case F_IfTargetIsOnOtherTeam:
      // This function proceeds only if the target is on another team
      returncode = ( chralive[loc_aitarget] && chrteam[loc_aitarget] != chrteam[ichr] );
      break;

    case F_IfTargetIsOnHatedTeam:
      // This function proceeds only if the target is on an enemy team
      returncode = ( chralive[loc_aitarget] && teamhatesteam[chrteam[ichr]][chrteam[loc_aitarget]] && !chrinvictus[loc_aitarget] );
      break;

    case F_PressLatchButton:
      // This function sets the latch buttons
      chrlatchbutton[ichr] |= scr_globals.tmpargument;
      break;

    case F_SetTargetToTargetOfLeader:
      // This function sets the character's target to the target of its leader,
      // or it fails with no change if the leader is dead
      returncode = bfalse;
      if ( VALID_CHR( loc_leader ) )
      {
        chraitarget[ichr] = chr_get_aitarget( loc_leader );
        returncode = btrue;
      }
      break;

    case F_IfLeaderKilled:
      // This function proceeds only if the character's leader has just died
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_LEADERKILLED );
      break;

    case F_BecomeLeader:
      // This function makes the character the team leader
      teamleader[chrteam[ichr]] = ichr;
      break;

    case F_ChangeTargetArmor:
      // This function sets the target's armor type and returns the old type
      // as tmpargument and the new type as tmpx
      iTmp = ( chrtexture[loc_aitarget] - madskinstart[chrmodel[loc_aitarget]] ) % MAXSKIN;
      scr_globals.tmpx = change_armor( loc_aitarget, scr_globals.tmpargument );
      scr_globals.tmpargument = iTmp;  // The character's old armor
      break;

    case F_GiveMoneyToTarget:
      // This function transfers money from the character to the target, and sets
      // tmpargument to the amount transferred
      iTmp = chrmoney[ichr];
      tTmp = chrmoney[loc_aitarget];
      iTmp -= scr_globals.tmpargument;
      tTmp += scr_globals.tmpargument;
      if ( iTmp < 0 ) { tTmp += iTmp;  scr_globals.tmpargument += iTmp;  iTmp = 0; }
      if ( tTmp < 0 ) { iTmp += tTmp;  scr_globals.tmpargument += tTmp;  tTmp = 0; }
      if ( iTmp > MAXMONEY ) { iTmp = MAXMONEY; }
      if ( tTmp > MAXMONEY ) { tTmp = MAXMONEY; }
      chrmoney[ichr] = iTmp;
      chrmoney[loc_aitarget] = tTmp;
      break;

    case F_DropKeys:
      drop_keys( ichr );
      break;

    case F_IfLeaderIsAlive:
      // This function fails if there is no team leader
      returncode = VALID_CHR( loc_leader );
      break;

    case F_IfTargetIsOldTarget:
      // This function returns bfalse if the target has changed
      returncode = ( loc_aitarget == scr_globals.oldtarget );
      break;

    case F_SetTargetToLeader:
      // This function fails if there is no team leader
      if ( !VALID_CHR( loc_leader ) )
      {
        returncode = bfalse;
      }
      else
      {
        chraitarget[ichr] = loc_leader;
      }
      break;

    case F_SpawnCharacter:
      {
        vect3 chr_pos = {scr_globals.tmpx, scr_globals.tmpy, 0};

        // This function spawns a ichr, failing if x,y is invalid
        sTmp = spawn_one_character( chr_pos, loc_model, chrteam[ichr], 0, scr_globals.tmpturn, NULL, MAXCHR );
        returncode = bfalse;
        if ( VALID_CHR( sTmp ) )
        {
          if ( 0 != __chrhitawall( sTmp, NULL ) )
          {
            chrfreeme[sTmp] = btrue;
          }
          else
          {
            tTmp = chrturn_lr[ichr] >> 2;
            chrvel[sTmp].x += turntosin[( tTmp+8192+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK] * scr_globals.tmpdistance;
            chrvel[sTmp].y += turntosin[( tTmp+8192 ) & TRIGTABLE_MASK] * scr_globals.tmpdistance;
            chrpassage[sTmp] = chrpassage[ichr];
            chriskursed[sTmp] = bfalse;
            chraichild[ichr] = sTmp;
            chraiowner[sTmp] = loc_aiowner;
            returncode = btrue;
          }
        }
      }
      break;

    case F_RespawnCharacter:
      // This function respawns the character at its starting location
      respawn_character( ichr );
      break;

    case F_ChangeTile:
      // This function changes the floor image under the character
      mesh_set_tile( MESH_FLOAT_TO_FAN( chrpos[ichr].x ), MESH_FLOAT_TO_FAN( chrpos[ichr].y ), scr_globals.tmpargument & 0xFF );
      break;

    case F_IfUsed:
      // This function proceeds only if the character has been used
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_USED );
      break;

    case F_DropMoney:
      // This function drops some of a character's money
      drop_money( ichr, scr_globals.tmpargument );
      break;

    case F_SetOldTarget:
      // This function sets the old target to the current target
      scr_globals.oldtarget = chr_get_aitarget( ichr );
      break;

    case F_DetachFromHolder:
      // This function drops the ichr, failing only if it was not held
      returncode = detach_character_from_mount( ichr, btrue, btrue );
      break;

    case F_IfTargetHasVulnerabilityID:
      // This function proceeds if ID matches tmpargument
      returncode = ( capidsz[chrmodel[loc_aitarget]][IDSZ_VULNERABILITY] == ( IDSZ ) scr_globals.tmpargument );
      break;

    case F_CleanUp:
      // This function issues the clean up order to all teammates
      issue_clean( ichr );
      break;

    case F_IfCleanedUp:
      // This function proceeds only if the character was told to clean up
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_CLEANEDUP );
      break;

    case F_IfSitting:
      // This function proceeds if the character is riding another
      returncode = bfalse;
      sTmp = chr_get_attachedto( ichr );
      if( VALID_CHR( sTmp ) )
      {
        returncode = (ichr == chr_get_holdingwhich(sTmp, SLOT_SADDLE));
      };
      break;

    case F_IfTargetIsHurt:
      // This function passes only if the target is hurt and alive
      returncode = bfalse;
      if ( VALID_CHR( loc_aitarget ) )
      {
        returncode = chralive[loc_aitarget] &&  chrlifemax_fp8[loc_aitarget] > 0 && chrlife_fp8[loc_aitarget] < chrlifemax_fp8[loc_aitarget] - DAMAGE_HURT;
      }
      break;

    case F_IfTargetIsAPlayer:
      // This function proceeds only if the target is a player ( may not be local )
      returncode = chrisplayer[loc_aitarget];
      break;

    case F_PlaySound:
      // This function plays a sound
      returncode = bfalse;
      if ( INVALID_SOUND != scr_globals.tmpargument && chrpos_old[ichr].z > PITNOSOUND )
      {
        returncode = ( INVALID_SOUND != play_sound( 1.0f, chrpos_old[ichr], capwavelist[loc_model][scr_globals.tmpargument], 0  ) );
      }
      break;

    case F_SpawnParticle:
      // This function spawns a particle
      returncode = bfalse;
      if ( VALID_CHR( ichr ) && !chr_in_pack( ichr ) )
      {
        tTmp = ichr;
        if ( chr_attached( ichr ) )  tTmp = chr_get_attachedto( ichr );
        tTmp = spawn_one_particle( 1.0f, chrpos[ichr], chrturn_lr[ichr], loc_model, scr_globals.tmpargument, ichr, scr_globals.tmpdistance, chrteam[ichr], tTmp, 0, MAXCHR );

        if ( VALID_PRT( tTmp ) )
        {
          // Detach the particle
          attach_particle_to_character( tTmp, ichr, scr_globals.tmpdistance );
          prtattachedtochr[tTmp] = MAXCHR;

          // Correct X, Y, Z spacing
          prtpos[tTmp].x += scr_globals.tmpx;
          prtpos[tTmp].y += scr_globals.tmpy;
          prtpos[tTmp].z += pipzspacing[prtpip[tTmp]].ibase;

          // Don't spawn in walls
          if ( 0 != __prthitawall( tTmp, NULL ) )
          {
            prtpos[tTmp].x = chrpos[ichr].x;
            if ( 0 != __prthitawall( tTmp, NULL ) )
            {
              prtpos[tTmp].y = chrpos[ichr].y;
            }
          }
          returncode = btrue;
        }
      }
      break;

    case F_IfTargetIsAlive:
      // This function proceeds only if the target is alive
      returncode = chralive[loc_aitarget];
      break;

    case F_Stop:
      chrmaxaccel[ichr] = 0;
      break;

    case F_DisaffirmCharacter:
      disaffirm_attached_particles( ichr );
      break;

    case F_ReaffirmCharacter:
      reaffirm_attached_particles( ichr );
      break;

    case F_IfTargetIsSelf:
      // This function proceeds only if the target is the character too
      returncode = ( loc_aitarget == ichr );
      break;

    case F_IfTargetIsMale:
      // This function proceeds only if the target is male
      returncode = ( chrgender[ichr] == GEN_MALE );
      break;

    case F_IfTargetIsFemale:
      // This function proceeds only if the target is female
      returncode = ( chrgender[ichr] == GEN_FEMALE );
      break;

    case F_SetTargetToSelf:
      // This function sets the target to the character
      chraitarget[ichr] = ichr;
      break;

    case F_SetTargetToRider:
      // This function sets the target to the character's left/only grip weapon,
      // failing if there is none
      returncode = bfalse;
      if ( chr_using_slot( ichr, SLOT_SADDLE ) )
      {
        chraitarget[ichr] = chr_get_holdingwhich( ichr, SLOT_SADDLE );
        returncode = btrue;
      }
      break;

    case F_GetAttackTurn:
      // This function sets tmpturn to the direction of the last attack
      scr_globals.tmpturn = chrdirectionlast[ichr];
      break;

    case F_GetDamageType:
      // This function gets the last type of damage
      scr_globals.tmpargument = chrdamagetypelast[ichr];
      break;

    case F_BecomeSpell:
      // This function turns the spellbook character into a spell based on its
      // content
      chrmoney[ichr] = ( chrtexture[ichr] - madskinstart[loc_model] ) % MAXSKIN;
      change_character( ichr, chraicontent[ichr], 0, LEAVE_NONE );
      chraicontent[ichr] = 0;  // Reset so it doesn't mess up
      chraistate[ichr]   = 0;  // Reset so it doesn't mess up
      chraimorphed[ichr] = btrue;
      break;

    case F_BecomeSpellbook:
      // This function turns the spell into a spellbook, and sets the content
      // accordingly
      chraicontent[ichr] = loc_model;
      change_character( ichr, SPELLBOOK, chrmoney[ichr] % MAXSKIN, LEAVE_NONE );
      chraistate[ichr] = 0;  // Reset so it doesn't burn up
      chraimorphed[ichr] = btrue;
      break;

    case F_IfScoredAHit:
      // Proceed only if the character scored a hit
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_SCOREDAHIT );
      break;

    case F_IfDisaffirmed:
      // Proceed only if the character was disaffirmed
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_DISAFFIRMED );
      break;

    case F_DecodeOrder:
      // This function gets the order and sets tmpx, tmpy, tmpargument and the
      // target ( if valid )

      returncode = bfalse;
      sTmp = chrmessage[ichr] >> 20;
      if ( VALID_CHR( sTmp ) )
      {
        chraitarget[ichr]  = sTmp;
        scr_globals.tmpx        = (( chrmessage[ichr] >> 12 ) & 0x00FF ) << 8;
        scr_globals.tmpy        = (( chrmessage[ichr] >>  4 ) & 0x00FF ) << 8;
        scr_globals.tmpargument = chrmessage[ichr] & 0x000F;
        returncode = btrue;
      }
      break;

    case F_SetTargetToWhoeverWasHit:
      // This function sets the target to whoever the character hit last,
      chraitarget[ichr] = chr_get_aihitlast( ichr );
      break;

    case F_SetTargetToWideEnemy:
      // This function finds an enemy, and proceeds only if there is one
      sTmp = chr_search_wide_target( ichr, bfalse, bfalse, btrue, bfalse, IDSZ_NONE, bfalse );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        chraitarget[ichr] = sTmp;
        returncode = btrue;
      }
      break;

    case F_IfChanged:
      // Proceed only if the character was polymorphed
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_CHANGED );
      break;

    case F_IfInWater:
      // Proceed only if the character got wet
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_INWATER );
      break;

    case F_IfBored:
      // Proceed only if the character is bored
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_BORED );
      break;

    case F_IfTooMuchBaggage:
      // Proceed only if the character tried to grab too much
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_TOOMUCHBAGGAGE );
      break;

    case F_IfGrogged:
      // Proceed only if the character was grogged
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_GROGGED );
      break;

    case F_IfDazed:
      // Proceed only if the character was dazed
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_DAZED );
      break;

    case F_IfTargetHasSpecialID:
      // This function proceeds if ID matches tmpargument
      returncode = ( capidsz[chrmodel[loc_aitarget]][IDSZ_SPECIAL] == ( IDSZ ) scr_globals.tmpargument );
      break;

    case F_PressTargetLatchButton:
      // This function sets the target's latch buttons
      chrlatchbutton[loc_aitarget] |= scr_globals.tmpargument;
      break;

    case F_IfInvisible:
      // This function passes if the character is invisible
      returncode = chr_is_invisible( ichr );
      break;

    case F_IfArmorIs:
      // This function passes if the character's skin is tmpargument
      tTmp = ( chrtexture[ichr] - madskinstart[loc_model] ) % MAXSKIN;
      returncode = ( tTmp == scr_globals.tmpargument );
      break;

    case F_GetTargetGrogTime:
      // This function returns tmpargument as the grog time, and passes if it is not 0
      scr_globals.tmpargument = chrgrogtime[ichr];
      returncode = ( scr_globals.tmpargument != 0 );
      break;

    case F_GetTargetDazeTime:
      // This function returns tmpargument as the daze time, and passes if it is not 0
      scr_globals.tmpargument = chrdazetime[ichr];
      returncode = ( scr_globals.tmpargument != 0 );
      break;

    case F_SetDamageType:
      // This function sets the bump damage type
      chrdamagetargettype[ichr] = scr_globals.tmpargument & ( MAXDAMAGETYPE - 1 );
      break;

    case F_SetWaterLevel:
      // This function raises and lowers the module's water
      fTmp = ( scr_globals.tmpargument / 10.0 ) - waterdouselevel;
      watersurfacelevel += fTmp;
      waterdouselevel += fTmp;
      for ( iTmp = 0; iTmp < MAXWATERLAYER; iTmp++ )
        waterlayerz[iTmp] += fTmp;
      break;

    case F_EnchantTarget:
      // This function enchants the target
      sTmp = spawn_enchant( loc_aiowner, loc_aitarget, ichr, MAXENCHANT, MAXMODEL );
      returncode = ( sTmp != MAXENCHANT );
      break;

    case F_EnchantChild:
      // This function can be used with SpawnCharacter to enchant the
      // newly spawned character
      sTmp = spawn_enchant( loc_aiowner, loc_aichild, ichr, MAXENCHANT, MAXMODEL );
      returncode = ( sTmp != MAXENCHANT );
      break;

    case F_TeleportTarget:
      // This function teleports the target to the X, Y location, failing if the
      // location is off the map or blocked. Z position is defined in tmpdistance
      returncode = bfalse;
      if ( mesh_check( scr_globals.tmpx, scr_globals.tmpy ) )
      {
        // Yeah!  It worked!
        sTmp = chr_get_aitarget( ichr );
        detach_character_from_mount( sTmp, btrue, bfalse );
        chrpos_old[sTmp] = chrpos[sTmp];

        chrpos[sTmp].x = scr_globals.tmpx;
        chrpos[sTmp].y = scr_globals.tmpy;
        chrpos[sTmp].z = scr_globals.tmpdistance;
        if ( 0 != __chrhitawall( sTmp, NULL ) )
        {
          // No it didn't...
          chrpos[sTmp] = chrpos_old[sTmp];
          returncode = bfalse;
        }
        else
        {
          chrpos_old[sTmp] = chrpos[sTmp];
          returncode = btrue;
        }
      }
      break;

    case F_GiveExperienceToTarget:
      // This function gives the target some experience, xptype from distance,
      // amount from argument...
      give_experience( loc_aitarget, scr_globals.tmpargument, scr_globals.tmpdistance );
      break;

    case F_IncreaseAmmo:
      // This function increases the ammo by one
      if ( chrammo[ichr] < chrammomax[ichr] )
      {
        chrammo[ichr]++;
      }
      break;

    case F_UnkurseTarget:
      // This function unkurses the target
      chriskursed[loc_aitarget] = bfalse;
      break;

    case F_GiveExperienceToTargetTeam:
      // This function gives experience to everyone on the target's team
      give_team_experience( chrteam[loc_aitarget], scr_globals.tmpargument, scr_globals.tmpdistance );
      break;

    case F_IfUnarmed:
      // This function proceeds if the character has no item in hand
      returncode = !chr_using_slot( ichr, SLOT_LEFT ) && !chr_using_slot( ichr, SLOT_RIGHT );
      break;

    case F_RestockTargetAmmoIDAll:
      // This function restocks the ammo of every item the character is holding,
      // if the item matches the ID given ( parent or child type )
      iTmp = 0;  // Amount of ammo given

      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        sTmp = chr_get_holdingwhich( loc_aitarget, _slot );
        iTmp += restock_ammo( sTmp, scr_globals.tmpargument );
      }

      sTmp  = chr_get_nextinpack( loc_aitarget );
      while ( VALID_CHR( sTmp ) )
      {
        iTmp += restock_ammo( sTmp, scr_globals.tmpargument );
        sTmp  = chr_get_nextinpack( sTmp );
      }

      scr_globals.tmpargument = iTmp;
      returncode = ( iTmp != 0 );
      break;

    case F_RestockTargetAmmoIDFirst:
      // This function restocks the ammo of the first item the character is holding,
      // if the item matches the ID given ( parent or child type )
      iTmp = 0;  // Amount of ammo given

      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        sTmp = chr_get_holdingwhich( loc_aitarget, _slot );
        iTmp += restock_ammo( sTmp, scr_globals.tmpargument );
        if ( iTmp != 0 ) break;
      }

      if ( iTmp == 0 )
      {
        sTmp  = chr_get_nextinpack( loc_aitarget );
        while ( VALID_CHR( sTmp ) && iTmp == 0 )
        {
          iTmp += restock_ammo( sTmp, scr_globals.tmpargument );
          sTmp  = chr_get_nextinpack( sTmp );
        }
      }

      scr_globals.tmpargument = iTmp;
      returncode = ( iTmp != 0 );
      break;

    case F_FlashTarget:
      // This function flashes the character
      flash_character( loc_aitarget, 255 );
      break;

    case F_SetRedShift:
      // This function alters a character's coloration
      chrredshift[ichr] = scr_globals.tmpargument;
      break;

    case F_SetGreenShift:
      // This function alters a character's coloration
      chrgrnshift[ichr] = scr_globals.tmpargument;
      break;

    case F_SetBlueShift:
      // This function alters a character's coloration
      chrblushift[ichr] = scr_globals.tmpargument;
      break;

    case F_SetLight:
      // This function alters a character's transparency
      chrlight_fp8[ichr] = scr_globals.tmpargument;
      break;

    case F_SetAlpha:
      // This function alters a character's transparency
      chralpha_fp8[ichr]    = scr_globals.tmpargument;
      chrbumpstrength[ichr] = capbumpstrength[loc_model] * FP8_TO_FLOAT( scr_globals.tmpargument );
      break;

    case F_IfHitFromBehind:
      // This function proceeds if the character was attacked from behind
      returncode = bfalse;
      if ( chrdirectionlast[ichr] >= BEHIND - 8192 && chrdirectionlast[ichr] < BEHIND + 8192 )
        returncode = btrue;
      break;

    case F_IfHitFromFront:
      // This function proceeds if the character was attacked from the front
      returncode = bfalse;
      if ( chrdirectionlast[ichr] >= 49152 + 8192 || chrdirectionlast[ichr] < FRONT + 8192 )
        returncode = btrue;
      break;

    case F_IfHitFromLeft:
      // This function proceeds if the character was attacked from the left
      returncode = bfalse;
      if ( chrdirectionlast[ichr] >= LEFT - 8192 && chrdirectionlast[ichr] < LEFT + 8192 )
        returncode = btrue;
      break;

    case F_IfHitFromRight:
      // This function proceeds if the character was attacked from the right
      returncode = bfalse;
      if ( chrdirectionlast[ichr] >= RIGHT - 8192 && chrdirectionlast[ichr] < RIGHT + 8192 )
        returncode = btrue;
      break;

    case F_IfTargetIsOnSameTeam:
      // This function proceeds only if the target is on another team
      returncode = bfalse;
      if ( chrteam[loc_aitarget] == chrteam[ichr] )
        returncode = btrue;
      break;

    case F_KillTarget:
      // This function kills the target
      kill_character( loc_aitarget, ichr );
      break;

    case F_UndoEnchant:
      // This function undoes the last enchant
      returncode = ( chrundoenchant[ichr] != MAXENCHANT );
      remove_enchant( chrundoenchant[ichr] );
      break;

    case F_GetWaterLevel:
      // This function gets the douse level for the water, returning it in tmpargument
      scr_globals.tmpargument = waterdouselevel * 10;
      break;

    case F_CostTargetMana:
      // This function costs the target some mana
      returncode = cost_mana( loc_aitarget, scr_globals.tmpargument, ichr );
      break;

    case F_IfTargetHasAnyID:
      // This function proceeds only if one of the target's IDSZ's matches tmpargument
      returncode = bfalse;
      for ( tTmp = 0; tTmp < IDSZ_COUNT; tTmp++ )
      {
        if ( capidsz[chrmodel[loc_aitarget]][tTmp] == ( IDSZ ) scr_globals.tmpargument )
        {
          returncode = btrue;
          break;
        }
      }
      break;

    case F_SetBumpSize:
      // This function sets the character's bump size
      //fTmp = chrbmpdata[ichr].calc_size_big;
      //fTmp /= chrbmpdata[ichr].calc_size;  // 1.5 or 2.0
      //chrbmpdata[ichr].size = scr_globals.tmpargument * chrfat[ichr];
      //chrbmpdata[ichr].sizebig = fTmp * chrbmpdata[ichr].calc_size;
      //chrbmpdata_save[ichr].size = scr_globals.tmpargument;
      //chrbmpdata_save[ichr].sizebig = fTmp * chrbmpdata_save[ichr].size;
      break;

    case F_IfNotDropped:
      // This function passes if a kursed item could not be dropped
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_NOTDROPPED );
      break;

    case F_IfYIsLessThanX:
      // This function passes only if tmpy is less than tmpx
      returncode = ( scr_globals.tmpy < scr_globals.tmpx );
      break;

    case F_SetFlyHeight:
      // This function sets a character's fly height
      chrflyheight[ichr] = scr_globals.tmpargument;
      break;

    case F_IfBlocked:
      // This function passes if the character blocked an attack
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_BLOCKED );
      break;

    case F_IfTargetIsDefending:
      returncode = ( chraction[loc_aitarget] >= ACTION_PA && chraction[loc_aitarget] <= ACTION_PD );
      break;

    case F_IfTargetIsAttacking:
      returncode = ( chraction[loc_aitarget] >= ACTION_UA && chraction[loc_aitarget] <= ACTION_FD );
      break;

    case F_IfStateIs0:
      returncode = ( 0 == chraistate[ichr] );
      break;

    case F_IfStateIs1:
      returncode = ( 1 == chraistate[ichr] );
      break;

    case F_IfStateIs2:
      returncode = ( 2 == chraistate[ichr] );
      break;

    case F_IfStateIs3:
      returncode = ( 3 == chraistate[ichr] );
      break;

    case F_IfStateIs4:
      returncode = ( 4 == chraistate[ichr] );
      break;

    case F_IfStateIs5:
      returncode = ( 5 == chraistate[ichr] );
      break;

    case F_IfStateIs6:
      returncode = ( 6 == chraistate[ichr] );
      break;

    case F_IfStateIs7:
      returncode = ( 7 == chraistate[ichr] );
      break;

    case F_IfContentIs:
      returncode = ( scr_globals.tmpargument == chraicontent[ichr] );
      break;

    case F_SetTurnModeToWatchTarget:
      // This function sets the turn mode
      chrturnmode[ichr] = TURNMODE_WATCHTARGET;
      break;

    case F_IfStateIsNot:
      returncode = ( scr_globals.tmpargument != chraistate[ichr] );
      break;

    case F_IfXIsEqualToY:
      returncode = ( scr_globals.tmpx == scr_globals.tmpy );
      break;

    case F_DisplayDebugMessage:
      // This function spits out a debug message
      debug_message( 1, "aistate %d, aicontent %d, target %d", chraistate[ichr], chraicontent[ichr], chraitarget[ichr] );
      debug_message( 1, "tmpx %d, tmpy %d", scr_globals.tmpx, scr_globals.tmpy );
      debug_message( 1, "tmpdistance %d, tmpturn %d", scr_globals.tmpdistance, scr_globals.tmpturn );
      debug_message( 1, "tmpargument %d, selfturn %d", scr_globals.tmpargument, chrturn_lr[ichr] );
      break;

    case F_BlackTarget:
      // This function makes the target flash black
      flash_character( loc_aitarget, 0 );
      break;

    case F_DisplayMessageNear:
      // This function sends a message if the camera is in the nearby area.
      iTmp = ABS( chrpos_old[ichr].x - camtrackpos.x ) + ABS( chrpos_old[ichr].y - camtrackpos.y );
      if ( iTmp < MSGDISTANCE )
      {
        display_message( madmsgstart[loc_model] + scr_globals.tmpargument, ichr );
      }
      break;

    case F_IfHitGround:
      // This function passes if the character just hit the ground
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_HITGROUND );
      break;

    case F_IfNameIsKnown:
      // This function passes if the character's name is known
      returncode = chrnameknown[ichr];
      break;

    case F_IfUsageIsKnown:
      // This function passes if the character's usage is known
      returncode = capusageknown[loc_model];
      break;

    case F_IfHoldingItemID:
      // This function passes if the character is holding an item with the IDSZ given
      // in tmpargument, returning the latch to press to use it
      returncode = bfalse;

      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        sTmp = chr_get_holdingwhich( ichr, _slot );
        if ( VALID_CHR( sTmp ) )
        {
          if ( CAP_INHERIT_IDSZ( chrmodel[sTmp], scr_globals.tmpargument ) )
          {
            scr_globals.tmpargument = slot_to_latch( ichr, _slot );
            returncode = btrue;
          }
        }
      }
      break;

    case F_IfHoldingRangedWeapon:
      // This function passes if the character is holding a ranged weapon, returning
      // the latch to press to use it.  This also checks ammo/ammoknown.
      returncode = bfalse;
      scr_globals.tmpargument = 0;

      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        tTmp = chr_get_holdingwhich( ichr, _slot );
        if ( VALID_CHR( tTmp ) )
        {
          sTmp = chrmodel[tTmp];
          if ( capisranged[sTmp] && ( chrammomax[tTmp] == 0 || ( chrammo[tTmp] > 0 && chrammoknown[tTmp] ) ) )
          {
            scr_globals.tmpargument = slot_to_latch( ichr, _slot );
            returncode = btrue;
          }
        }
      };
      break;

    case F_IfHoldingMeleeWeapon:
      // This function passes if the character is holding a melee weapon, returning
      // the latch to press to use it
      returncode = bfalse;
      scr_globals.tmpargument = 0;

      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        sTmp = chr_get_holdingwhich( ichr, _slot );
        if ( VALID_CHR( sTmp ) )
        {
          if ( !capisranged[chrmodel[sTmp]] && capweaponaction[chrmodel[sTmp]] != ACTION_PA )
          {
            scr_globals.tmpargument = slot_to_latch( ichr, _slot );
            returncode = btrue;
          }
        }
      };
      break;

    case F_IfHoldingShield:
      // This function passes if the character is holding a shield, returning the
      // latch to press to use it
      returncode = bfalse;
      scr_globals.tmpargument = 0;

      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        sTmp = chr_get_holdingwhich( ichr, _slot );
        if ( VALID_CHR( sTmp ) && capweaponaction[chrmodel[sTmp]] == ACTION_PA )
        {
          scr_globals.tmpargument = slot_to_latch( ichr, _slot );
          returncode = btrue;
        }
      };
      break;

    case F_IfKursed:
      // This function passes if the character is kursed
      returncode = chriskursed[ichr];
      break;

    case F_IfTargetIsKursed:
      // This function passes if the target is kursed
      returncode = chriskursed[loc_aitarget];
      break;

    case F_IfTargetIsDressedUp:
      // This function passes if the character's skin is dressy
      iTmp = ( chrtexture[ichr] - madskinstart[loc_model] ) % MAXSKIN;
      iTmp = 1 << iTmp;
      returncode = (( capskindressy[loc_model] & iTmp ) != 0 );
      break;

    case F_IfOverWater:
      // This function passes if the character is on a water tile
      returncode = mesh_has_some_bits( chronwhichfan[ichr], MESHFX_WATER ) && wateriswater;
      break;

    case F_IfThrown:
      // This function passes if the character was thrown
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_THROWN );
      break;

    case F_MakeNameKnown:
      // This function makes the name of an item/character known.
      chrnameknown[ichr] = btrue;
      chricon[ichr] = btrue;
      break;

    case F_MakeUsageKnown:
      // This function makes the usage of an item known...  For XP gains from
      // using an unknown potion or such
      capusageknown[loc_model] = btrue;
      break;

    case F_StopTargetMovement:
      // This function makes the target stop moving temporarily
      chrvel[loc_aitarget].x = 0;
      chrvel[loc_aitarget].y = 0;
      if ( chrvel[loc_aitarget].z > 0 ) chrvel[loc_aitarget].z = gravity;
      break;

    case F_SetXY:
      // This function stores tmpx and tmpy in the storage array
      chraix[ichr][scr_globals.tmpargument&STORAND] = scr_globals.tmpx;
      chraiy[ichr][scr_globals.tmpargument&STORAND] = scr_globals.tmpy;
      break;

    case F_GetXY:
      // This function gets previously stored data, setting tmpx and tmpy
      scr_globals.tmpx = chraix[ichr][scr_globals.tmpargument&STORAND];
      scr_globals.tmpy = chraiy[ichr][scr_globals.tmpargument&STORAND];
      break;

    case F_AddXY:
      // This function adds tmpx and tmpy to the storage array
      chraix[ichr][scr_globals.tmpargument&STORAND] += scr_globals.tmpx;
      chraiy[ichr][scr_globals.tmpargument&STORAND] += scr_globals.tmpy;
      break;

    case F_MakeAmmoKnown:
      // This function makes the ammo of an item/character known.
      chrammoknown[ichr] = btrue;
      break;

    case F_SpawnAttachedParticle:
      // This function spawns an attached particle
      returncode = bfalse;
      if ( VALID_CHR( ichr ) && !chr_in_pack( ichr ) )
      {
        tTmp = ichr;
        if ( chr_attached( ichr ) )  tTmp = chr_get_attachedto( ichr );
        tTmp = spawn_one_particle( 1.0f, chrpos[ichr], chrturn_lr[ichr], loc_model, scr_globals.tmpargument, ichr, scr_globals.tmpdistance, chrteam[ichr], tTmp, 0, MAXCHR );
        returncode = VALID_PRT( tTmp );
      }
      break;

    case F_SpawnExactParticle:
      // This function spawns an exactly placed particle
      returncode = bfalse;
      if ( VALID_CHR( ichr ) && !chr_in_pack( ichr ) )
      {
        vect3 prt_pos = {scr_globals.tmpx, scr_globals.tmpy, scr_globals.tmpdistance};
        tTmp = ichr;
        if ( chr_attached( tTmp ) )  tTmp = chr_get_attachedto( tTmp );
        tTmp = spawn_one_particle( 1.0f, prt_pos, chrturn_lr[ichr], loc_model, scr_globals.tmpargument, MAXCHR, 0, chrteam[ichr], tTmp, 0, MAXCHR );
        returncode = VALID_PRT( tTmp );
      };
      break;

    case F_AccelerateTarget:
      // This function changes the target's speeds
      chrvel[loc_aitarget].x += scr_globals.tmpx;
      chrvel[loc_aitarget].y += scr_globals.tmpy;
      break;

    case F_IfDistanceIsMoreThanTurn:
      // This function proceeds tmpdistance is greater than tmpturn
      returncode = ( scr_globals.tmpdistance > ( int ) scr_globals.tmpturn );
      break;

    case F_IfCrushed:
      // This function proceeds only if the character was crushed
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_CRUSHED );
      break;

    case F_MakeCrushValid:
      // This function makes doors able to close on this object
      chrcanbecrushed[ichr] = btrue;
      break;

    case F_SetTargetToLowestTarget:
      // This sets the target to whatever the target is being held by,
      // The lowest in the set.  This function never fails
      if ( VALID_CHR( loc_aitarget ) )
      {
        CHR_REF holder   = chr_get_attachedto(loc_aitarget);
        CHR_REF attached = chr_get_attachedto(holder);
        while ( VALID_CHR(attached) )
        {
          holder = attached;
          attached = chr_get_attachedto(holder);
        }
        chraitarget[ichr] = holder;
      };
      break;

    case F_IfNotPutAway:
      // This function proceeds only if the character couln't be put in the pack
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_NOTPUTAWAY );
      break;

    case F_IfTakenOut:
      // This function proceeds only if the character was taken out of the pack
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_TAKENOUT );
      break;

    case F_IfAmmoOut:
      // This function proceeds only if the character has no ammo
      returncode = ( chrammo[ichr] == 0 );
      break;

    case F_PlaySoundLooped:
      // This function plays a looped sound
	  returncode = bfalse;
      if ( moduleActive && (0 <= scr_globals.tmpargument) && (chrpos_old[ichr].z > PITNOSOUND) && (chrloopingchannel[ichr] == INVALID_SOUND))
      {
        //You could use this, but right now there's no way to stop the sound later, so it's better not to start it
        chrloopingchannel[ichr] = play_sound( 1.0f, chrpos[ichr], capwavelist[loc_model][scr_globals.tmpargument], -1 );
        chrloopingvolume[ichr] = 1.0f;
      }
      break;

    case F_StopSoundLoop:
      // This function stops playing a sound
      if( INVALID_CHANNEL != chrloopingchannel[ichr] )
      {
        stop_sound( chrloopingchannel[ichr] );
        chrloopingchannel[ichr] = INVALID_CHANNEL;
      };
      break;

    case F_HealSelf:
      // This function heals the ichr, without setting the alert or modifying
      // the amount
      if ( chralive[ichr] )
      {
        iTmp = chrlife_fp8[ichr] + scr_globals.tmpargument;
        if ( iTmp > chrlifemax_fp8[ichr] ) iTmp = chrlifemax_fp8[ichr];
        if ( iTmp < 1 ) iTmp = 1;
        chrlife_fp8[ichr] = iTmp;
      }
      break;

    case F_Equip:
      // This function flags the character as being equipped
      chrisequipped[ichr] = btrue;
      break;

    case F_IfTargetHasItemIDEquipped:
      // This function proceeds if the target has a matching item equipped
      returncode = bfalse;
      sTmp  = chr_get_nextinpack( loc_aitarget );
      while ( VALID_CHR( sTmp ) )
      {
        if ( sTmp != ichr && chrisequipped[sTmp] && CAP_INHERIT_IDSZ( chrmodel[sTmp], scr_globals.tmpargument ) )
        {
          returncode = btrue;
          break;
        }
        else
        {
          sTmp  = chr_get_nextinpack( sTmp );
        }
      }
      break;

    case F_SetOwnerToTarget:
      // This function sets the owner
      chraiowner[ichr] = chr_get_aitarget( ichr );
      break;

    case F_SetTargetToOwner:
      // This function sets the target to the owner
      chraitarget[ichr] = loc_aiowner;
      break;

    case F_SetFrame:
      // This function sets the character's current frame
      sTmp = scr_globals.tmpargument & 3;
      iTmp = scr_globals.tmpargument >> 2;
      set_frame( ichr, iTmp, sTmp );
      break;

    case F_BreakPassage:
      // This function makes the tiles fall away ( turns into damage terrain )
      returncode = break_passage( scr_globals.tmpargument, scr_globals.tmpturn, scr_globals.tmpdistance, scr_globals.tmpx, scr_globals.tmpy );
      break;

    case F_SetReloadTime:
      // This function makes weapons fire slower
      chrreloadtime[ichr] = scr_globals.tmpargument;
      break;

    case F_SetTargetToWideBlahID:
      // This function sets the target based on the settings of
      // tmpargument and tmpdistance
      sTmp = chr_search_wide_target( ichr, 
                                     HAS_ALL_BITS( scr_globals.tmpdistance, SEARCH_ITEMS   ),
                                     HAS_ALL_BITS( scr_globals.tmpdistance, SEARCH_FRIENDS ),
                                     HAS_ALL_BITS( scr_globals.tmpdistance, SEARCH_ENEMIES ),
                                     HAS_ALL_BITS( scr_globals.tmpdistance, SEARCH_DEAD    ),
                                     scr_globals.tmpargument, 
                                     HAS_ALL_BITS( scr_globals.tmpdistance, SEARCH_INVERT  ) );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        chraitarget[ichr] = sTmp;
        returncode = btrue;
      }
      break;

    case F_PoofTarget:
      // This function makes the target go away
      returncode = bfalse;
      if ( VALID_CHR( loc_aitarget ) && !chrisplayer[loc_aitarget] )
      {
        // tell target to go away
        chrgopoof[loc_aitarget] = btrue;
        chraitarget[ichr] = ichr;
        returncode = btrue;
      }
      break;

    case F_ChildDoActionOverride:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid
      returncode = bfalse;
      if ( scr_globals.tmpargument < MAXACTION )
      {
        if ( madactionvalid[chrmodel[loc_aichild]][scr_globals.tmpargument] )
        {
          chraction[loc_aichild] = scr_globals.tmpargument;
          chrlip_fp8[loc_aichild] = 0;
          chrflip[loc_aichild] = 0.0f;
          chrframe[loc_aichild] = madactionstart[chrmodel[loc_aichild]][scr_globals.tmpargument];
          chrframelast[loc_aichild] = chrframe[loc_aichild];
          chractionready[loc_aichild] = bfalse;
          returncode = btrue;
        }
      }
      break;

    case F_SpawnPoof:
      // This function makes a lovely little poof at the character's location
      spawn_poof( ichr, loc_model );
      break;

    case F_SetSpeedPercent:
      reset_character_accel( ichr );
      chrmaxaccel[ichr] *= scr_globals.tmpargument / 100.0;
      break;

    case F_SetChildState:
      // This function sets the child's state
      chraistate[loc_aichild] = scr_globals.tmpargument;
      break;

    case F_SpawnAttachedSizedParticle:
      // This function spawns an attached particle, then sets its size
      returncode = bfalse;
      if ( VALID_CHR( ichr ) && !chr_in_pack( ichr ) )
      {
        tTmp = ichr;
        if ( chr_attached( ichr ) )  tTmp = chr_get_attachedto( ichr );

        tTmp = spawn_one_particle( 1.0f, chrpos[ichr], chrturn_lr[ichr], loc_model, scr_globals.tmpargument, ichr, scr_globals.tmpdistance, chrteam[ichr], tTmp, 0, MAXCHR );
        if ( VALID_PRT( tTmp ) )
        {
          prtsize_fp8[tTmp] = scr_globals.tmpturn;
          returncode = btrue;
        }
      }
      break;

    case F_ChangeArmor:
      // This function sets the character's armor type and returns the old type
      // as tmpargument and the new type as tmpx
      scr_globals.tmpx = scr_globals.tmpargument;
      iTmp = ( chrtexture[ichr] - madskinstart[loc_model] ) % MAXSKIN;
      scr_globals.tmpx = change_armor( ichr, scr_globals.tmpargument );
      scr_globals.tmpargument = iTmp;  // The character's old armor
      break;

    case F_ShowTimer:
      // This function turns the timer on, using the value for tmpargument
      timeron = btrue;
      timervalue = scr_globals.tmpargument;
      break;

    case F_IfFacingTarget:
      {
        // This function proceeds only if the character is facing the target
        // try to avoid using inverse trig functions
        float dx, dy, d2, ftmpx, ftmpy, ftmp;

        returncode = bfalse;
        dx = chrpos[loc_aitarget].x - chrpos[ichr].x;
        dy = chrpos[loc_aitarget].y - chrpos[ichr].y;
        d2 = dx * dx + dy * dy;

        if ( d2 > 0.0f )
        {
          // use non-inverse function to get direction vec from chrturn[]
          turn_to_vec( ichr, &ftmpx, &ftmpy );

          // calc the dotproduct
          ftmp = ( dx * ftmpx + dy * ftmpy );

          // negative dotprod means facing the wrong direction
          if ( ftmp > 0.0f )
          {
            // normalized dot product, squared
            ftmp *= ftmp / d2;

            // +/- 45 degrees means ftmp > 0.5f
            returncode = ftmp > 0.5f;
          }
        }

      }
      break;

    case F_PlaySoundVolume:
      // This function sets the volume of a sound and plays it
      returncode = bfalse;
      if ( INVALID_SOUND != scr_globals.tmpargument && scr_globals.tmpdistance >= 0 )
      {
        volume = scr_globals.tmpdistance;
        returncode = ( INVALID_SOUND != play_sound( MIN( 1.0f, volume / 255.0f ), chrpos_old[ichr], capwavelist[loc_model][scr_globals.tmpargument], 0 ) );
      }
      break;

    case F_SpawnAttachedFacedParticle:
      // This function spawns an attached particle with facing
      returncode = bfalse;
      if ( VALID_CHR( ichr ) && !chr_in_pack( ichr ) )
      {
        tTmp = ichr;
        if ( chr_attached( ichr ) )  tTmp = chr_get_attachedto( ichr );

        tTmp = spawn_one_particle( 1.0f, chrpos[ichr], scr_globals.tmpturn, loc_model, scr_globals.tmpargument, ichr, scr_globals.tmpdistance, chrteam[ichr], tTmp, 0, MAXCHR );
        returncode = VALID_PRT( bfalse );
      }
      break;

    case F_IfStateIsOdd:
      returncode = HAS_SOME_BITS( chraistate[ichr], 1 );
      break;

    case F_SetTargetToDistantEnemy:
      // This function finds an enemy, within a certain distance to the ichr, and
      // proceeds only if there is one
      sTmp = chr_search_distant_target( ichr, scr_globals.tmpdistance, btrue, bfalse );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        chraitarget[ichr] = sTmp;
        returncode = btrue;
      }
      break;

    case F_Teleport:
      // This function teleports the character to the X, Y location, failing if the
      // location is off the map or blocked
      returncode = bfalse;
      if ( mesh_check( scr_globals.tmpx, scr_globals.tmpy ) )
      {
        // Yeah!  It worked!
        detach_character_from_mount( ichr, btrue, bfalse );
        chrpos_old[ichr] = chrpos[ichr];

        chrpos[ichr].x = scr_globals.tmpx;
        chrpos[ichr].y = scr_globals.tmpy;
        if ( 0 != __chrhitawall( ichr, NULL ) )
        {
          // No it didn't...
          chrpos[ichr] = chrpos_old[ichr];
          returncode = bfalse;
        }
        else
        {
          chrpos_old[ichr] = chrpos[ichr];
          returncode = btrue;
        }
      }
      break;

    case F_GiveStrengthToTarget:
      // Permanently boost the target's strength
      if ( chralive[loc_aitarget] )
      {
        iTmp = scr_globals.tmpargument;
        getadd( 0, chrstrength_fp8[loc_aitarget], PERFECTSTAT, &iTmp );
        chrstrength_fp8[loc_aitarget] += iTmp;
      }
      break;

    case F_GiveWisdomToTarget:
      // Permanently boost the target's wisdom
      if ( chralive[loc_aitarget] )
      {
        iTmp = scr_globals.tmpargument;
        getadd( 0, chrwisdom_fp8[loc_aitarget], PERFECTSTAT, &iTmp );
        chrwisdom_fp8[loc_aitarget] += iTmp;
      }
      break;

    case F_GiveIntelligenceToTarget:
      // Permanently boost the target's intelligence
      if ( chralive[loc_aitarget] )
      {
        iTmp = scr_globals.tmpargument;
        getadd( 0, chrintelligence_fp8[loc_aitarget], PERFECTSTAT, &iTmp );
        chrintelligence_fp8[loc_aitarget] += iTmp;
      }
      break;

    case F_GiveDexterityToTarget:
      // Permanently boost the target's dexterity
      if ( chralive[loc_aitarget] )
      {
        iTmp = scr_globals.tmpargument;
        getadd( 0, chrdexterity_fp8[loc_aitarget], PERFECTSTAT, &iTmp );
        chrdexterity_fp8[loc_aitarget] += iTmp;
      }
      break;

    case F_GiveLifeToTarget:
      // Permanently boost the target's life
      if ( chralive[loc_aitarget] )
      {
        iTmp = scr_globals.tmpargument;
        getadd( LOWSTAT, chrlifemax_fp8[loc_aitarget], PERFECTBIG, &iTmp );
        chrlifemax_fp8[loc_aitarget] += iTmp;
        if ( iTmp < 0 )
        {
          getadd( 1, chrlife_fp8[loc_aitarget], PERFECTBIG, &iTmp );
        }
        chrlife_fp8[loc_aitarget] += iTmp;
      }
      break;

    case F_GiveManaToTarget:
      // Permanently boost the target's mana
      if ( chralive[loc_aitarget] )
      {
        iTmp = scr_globals.tmpargument;
        getadd( 0, chrmanamax_fp8[loc_aitarget], PERFECTBIG, &iTmp );
        chrmanamax_fp8[loc_aitarget] += iTmp;
        if ( iTmp < 0 )
        {
          getadd( 0, chrmana_fp8[loc_aitarget], PERFECTBIG, &iTmp );
        }
        chrmana_fp8[loc_aitarget] += iTmp;
      }
      break;

    case F_ShowMap:
      // Show the map...  Fails if map already visible
      if ( mapon )  returncode = bfalse;
      mapon = btrue;
      break;

    case F_ShowYouAreHere:
      // Show the camera target location
      youarehereon = btrue;
      break;

    case F_ShowBlipXY:
      // Add a blip
      if ( numblip < MAXBLIP )
      {
        if ( mesh_check( scr_globals.tmpx, scr_globals.tmpy ) )
        {
          if ( scr_globals.tmpargument < NUMBAR && scr_globals.tmpargument >= 0 )
          {
            blipx[numblip] = mesh_fraction_x( scr_globals.tmpx ) * MAPSIZE * mapscale;
            blipy[numblip] = mesh_fraction_y( scr_globals.tmpy ) * MAPSIZE * mapscale ;
            blipc[numblip] = scr_globals.tmpargument;
            numblip++;
          }
        }
      }
      break;

    case F_HealTarget:
      // Give some life to the target
      if ( chralive[loc_aitarget] )
      {
        iTmp = scr_globals.tmpargument;
        getadd( 1, chrlife_fp8[loc_aitarget], chrlifemax_fp8[loc_aitarget], &iTmp );
        chrlife_fp8[loc_aitarget] += iTmp;
        // Check all enchants to see if they are removed
        iTmp = chrfirstenchant[loc_aitarget];
        while ( iTmp != MAXENCHANT )
        {
          sTmp = encnextenchant[iTmp];
          if ( MAKE_IDSZ( "HEAL" ) == everemovedbyidsz[enceve[iTmp]] )
          {
            remove_enchant( iTmp );
          }
          iTmp = sTmp;
        }
      }
      break;

    case F_PumpTarget:
      // Give some mana to the target
      if ( chralive[loc_aitarget] )
      {
        iTmp = scr_globals.tmpargument;
        getadd( 0, chrmana_fp8[loc_aitarget], chrmanamax_fp8[loc_aitarget], &iTmp );
        chrmana_fp8[loc_aitarget] += iTmp;
      }
      break;

    case F_CostAmmo:
      // Take away one ammo
      if ( chrammo[ichr] > 0 )
      {
        chrammo[ichr]--;
      }
      break;

    case F_MakeSimilarNamesKnown:
      // Make names of matching objects known
      for ( iTmp = 0; iTmp < MAXCHR; iTmp++ )
      {
        sTmp = btrue;
        for ( tTmp = 0; tTmp < IDSZ_COUNT; tTmp++ )
        {
          if ( capidsz[loc_model][tTmp] != capidsz[chrmodel[iTmp]][tTmp] )
          {
            sTmp = bfalse;
            break;
          }
        }

        if ( sTmp )
        {
          chrnameknown[iTmp] = btrue;
        }
      }
      break;

    case F_SpawnAttachedHolderParticle:
      // This function spawns an attached particle, attached to the holder
      returncode = bfalse;
      if ( VALID_CHR( ichr ) && !chr_in_pack( ichr ) )
      {
        tTmp = ichr;
        if ( chr_attached( ichr ) )  tTmp = chr_get_attachedto( ichr );

        tTmp = spawn_one_particle( 1.0f, chrpos[ichr], chrturn_lr[ichr], loc_model, scr_globals.tmpargument, tTmp, scr_globals.tmpdistance, chrteam[ichr], tTmp, 0, MAXCHR );
        returncode = VALID_PRT( tTmp );
      };
      break;

    case F_SetTargetReloadTime:
      // This function sets the target's reload time
      chrreloadtime[loc_aitarget] = scr_globals.tmpargument;
      break;

    case F_SetFogLevel:
      // This function raises and lowers the module's fog
      fTmp = ( scr_globals.tmpargument / 10.0 ) - fogtop;
      fogtop += fTmp;
      fogdistance += fTmp;
      fogon = CData.fogallowed;
      if ( fogdistance < 1.0 )  fogon = bfalse;
      break;

    case F_GetFogLevel:
      // This function gets the fog level
      scr_globals.tmpargument = fogtop * 10;
      break;

    case F_SetFogTAD:
      // This function changes the fog color
      fogred = scr_globals.tmpturn;
      foggrn = scr_globals.tmpargument;
      fogblu = scr_globals.tmpdistance;
      break;

    case F_SetFogBottomLevel:
      // This function sets the module's bottom fog level...
      fTmp = ( scr_globals.tmpargument / 10.0 ) - fogbottom;
      fogbottom += fTmp;
      fogdistance -= fTmp;
      fogon = CData.fogallowed;
      if ( fogdistance < 1.0 )  fogon = bfalse;
      break;

    case F_GetFogBottomLevel:
      // This function gets the fog level
      scr_globals.tmpargument = fogbottom * 10;
      break;

    case F_CorrectActionForHand:
      // This function turns ZA into ZA, ZB, ZC, or ZD...
      // tmpargument must be set to one of the A actions beforehand...
      if ( chr_attached( ichr ) )
      {
        if ( chrinwhichslot[ichr] == SLOT_RIGHT )
        {
          // C or D
          scr_globals.tmpargument += 2;
        };
        scr_globals.tmpargument += ( rand() & 1 );
      }
      break;

    case F_IfTargetIsMounted:
      // This function proceeds if the target is riding a mount
      returncode = bfalse;
      sTmp = chr_get_aitarget( ichr );
      if ( VALID_CHR( sTmp ) )
      {
        sTmp = chr_get_attachedto(sTmp);
        if ( VALID_CHR( sTmp ) )
        {
          returncode = chrbmpdata[sTmp].calc_is_mount;
        }
      }
      break;

    case F_SparkleIcon:
      // This function makes a blippie thing go around the icon
      if ( scr_globals.tmpargument < NUMBAR && scr_globals.tmpargument > -1 )
      {
        chrsparkle[ichr] = scr_globals.tmpargument;
      }
      break;

    case F_UnsparkleIcon:
      // This function stops the blippie thing
      chrsparkle[ichr] = NOSPARKLE;
      break;

    case F_GetTileXY:
      // This function gets the tile at x,y
      scr_globals.tmpargument = mesh_get_tile( MESH_FLOAT_TO_FAN( scr_globals.tmpx ), MESH_FLOAT_TO_FAN( scr_globals.tmpy ) ) & 0xFF;
      break;

    case F_SetTileXY:
      // This function changes the tile at x,y
      mesh_set_tile( MESH_FLOAT_TO_FAN( scr_globals.tmpx ), MESH_FLOAT_TO_FAN( scr_globals.tmpy ), scr_globals.tmpargument & 255 );
      break;

    case F_SetShadowSize:
      // This function changes a character's shadow size
      //chrbmpdata[ichr].shadow = scr_globals.tmpargument * chrfat[ichr];
      //chrbmpdata_save[ichr].shadow = scr_globals.tmpargument;
      break;

    case F_SignalTarget:
      // This function orders one specific character...  The target
      // Be careful in using this, always checking IDSZ first
      chrmessage[loc_aitarget] = scr_globals.tmpargument;
      chrmessagedata[loc_aitarget] = 0;
      chralert[loc_aitarget] |= ALERT_SIGNALED;
      break;

    case F_SetTargetToWhoeverIsInPassage:
      // This function lets passage rectangles be used as event triggers
      sTmp = who_is_blocking_passage( scr_globals.tmpargument );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        chraitarget[ichr] = sTmp;
        returncode = btrue;
      }
      break;

    case F_IfCharacterWasABook:
      // This function proceeds if the base model is the same as the current
      // model or if the base model is SPELLBOOK
      returncode = ( chrbasemodel[ichr] == SPELLBOOK ||
                     chrbasemodel[ichr] == loc_model );
      break;

    case F_SetEnchantBoostValues:
      // This function sets the boost values for the last enchantment
      iTmp = chrundoenchant[ichr];
      if ( iTmp != MAXENCHANT )
      {
        encownermana[iTmp] = scr_globals.tmpargument;
        encownerlife[iTmp] = scr_globals.tmpdistance;
        enctargetmana[iTmp] = scr_globals.tmpx;
        enctargetlife[iTmp] = scr_globals.tmpy;
      }
      break;

    case F_SpawnCharacterXYZ:
      {
        vect3 chr_pos = {scr_globals.tmpx, scr_globals.tmpy, scr_globals.tmpdistance};

        // This function spawns a ichr, failing if x,y,z is invalid
        sTmp = spawn_one_character( chr_pos, loc_model, chrteam[ichr], 0, scr_globals.tmpturn, NULL, MAXCHR );
        returncode = bfalse;
        if ( VALID_CHR( sTmp ) )
        {
          if ( 0 != __chrhitawall( sTmp, NULL ) )
          {
            chrfreeme[sTmp] = btrue;
          }
          else
          {
            chriskursed[sTmp] = bfalse;
            chraichild[ichr] = sTmp;
            chrpassage[sTmp] = chrpassage[ichr];
            chraiowner[sTmp] = loc_aiowner;
            returncode = btrue;
          }
        }
      }
      break;

    case F_SpawnExactCharacterXYZ:
      // This function spawns a character ( specific model slot ),
      // failing if x,y,z is invalid
      {
        vect3 chr_pos = {scr_globals.tmpx, scr_globals.tmpy, scr_globals.tmpdistance};

        sTmp = spawn_one_character( chr_pos, scr_globals.tmpargument, chrteam[ichr], 0, scr_globals.tmpturn, NULL, MAXCHR );
        returncode = bfalse;
        if ( VALID_CHR( sTmp ) )
        {
          if ( 0 != __chrhitawall( sTmp, NULL ) )
          {
            chrfreeme[sTmp] = btrue;
          }
          else
          {
            chriskursed[sTmp] = bfalse;
            chraichild[ichr] = sTmp;
            chrpassage[sTmp] = chrpassage[ichr];
            chraiowner[sTmp] = loc_aiowner;
            returncode = btrue;
          }
        }
      }
      break;

    case F_ChangeTargetClass:
      // This function changes a character's model ( specific model slot )
      returncode = bfalse;
      if ( VALID_CHR( loc_aitarget ) )
      {
        change_character( loc_aitarget, scr_globals.tmpargument, 0, LEAVE_ALL );
        chraimorphed[loc_aitarget] = btrue;
      };
      break;

    case F_PlayFullSound:
      // This function plays a sound loud for everyone...  Victory music
      returncode = bfalse;
      if ( INVALID_SOUND != scr_globals.tmpargument )
      {
        returncode = ( INVALID_SOUND != play_sound( 1.0f, camtrackpos, capwavelist[loc_model][scr_globals.tmpargument], 0  ) );
      }
      break;

    case F_SpawnExactChaseParticle:
      // This function spawns an exactly placed particle that chases the target
      returncode = bfalse;
      if ( VALID_CHR( ichr ) && !chr_in_pack( ichr ) )
      {
        vect3 prt_pos = {scr_globals.tmpx, scr_globals.tmpy, scr_globals.tmpdistance};
        tTmp = ichr;
        if ( chr_attached( ichr ) )  tTmp = chr_get_attachedto( ichr );

        tTmp = spawn_one_particle( 1.0f, prt_pos, chrturn_lr[ichr], loc_model, scr_globals.tmpargument, MAXCHR, 0, chrteam[ichr], tTmp, 0, MAXCHR );
        if ( VALID_PRT( tTmp ) )
        {
          prttarget[tTmp] = chr_get_aitarget( ichr );
          returncode = btrue;
        }
      }
      break;

    case F_EncodeOrder:
      // This function packs up an order, using tmpx, tmpy, tmpargument and the
      // target ( if valid ) to create a new tmpargument
      sTmp = ( loc_aitarget           & 0x0FFF ) << 20;
      sTmp |= (( scr_globals.tmpx >> 8 ) & 0x00FF ) << 12;
      sTmp |= (( scr_globals.tmpy >> 8 ) & 0x00FF ) << 4;
      sTmp |= ( scr_globals.tmpargument & 0x000F );
      scr_globals.tmpargument = sTmp;
      break;

    case F_SignalSpecialID:
      // This function issues an order to all with the given special IDSZ
      signal_idsz_index( scr_globals.tmpargument, scr_globals.tmpdistance, IDSZ_SPECIAL );
      break;

    case F_UnkurseTargetInventory:
      // This function unkurses every item a character is holding

      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        sTmp = chr_get_holdingwhich( loc_aitarget, _slot );
        chriskursed[sTmp] = bfalse;
      };

      sTmp  = chr_get_nextinpack( loc_aitarget );
      while ( VALID_CHR( sTmp ) )
      {
        chriskursed[sTmp] = bfalse;
        sTmp  = chr_get_nextinpack( sTmp );
      }
      break;

    case F_IfTargetIsSneaking:
      // This function proceeds if the target is doing ACTION_DA or ACTION_WA
      returncode = ( chraction[loc_aitarget] == ACTION_DA || chraction[loc_aitarget] == ACTION_WA );
      break;

    case F_DropItems:
      // This function drops all of the character's items
      drop_all_items( ichr );
      break;

    case F_RespawnTarget:
      // This function respawns the target at its current location
      sTmp = chr_get_aitarget( ichr );
      chrpos_old[sTmp] = chrpos[sTmp];
      respawn_character( sTmp );
      chrpos[sTmp] = chrpos_old[sTmp];
      break;

    case F_TargetDoActionSetFrame:
      // This function starts a new action, if it is valid for the model and
      // sets the starting frame.  It will fail if the action is invalid
      returncode = bfalse;
      if ( scr_globals.tmpargument < MAXACTION )
      {
        if ( madactionvalid[chrmodel[loc_aitarget]][scr_globals.tmpargument] )
        {
          chraction[loc_aitarget] = scr_globals.tmpargument;
          chrlip_fp8[loc_aitarget] = 0;
          chrflip[loc_aitarget] = 0.0f;
          chrframe[loc_aitarget] = madactionstart[chrmodel[loc_aitarget]][scr_globals.tmpargument];
          chrframelast[loc_aitarget] = chrframe[loc_aitarget];
          chractionready[loc_aitarget] = bfalse;
          returncode = btrue;
        }
      }
      break;

    case F_IfTargetCanSeeInvisible:
      // This function proceeds if the target can see invisible
      returncode = chrcanseeinvisible[loc_aitarget];
      break;

    case F_SetTargetToNearestBlahID:
      // This function finds the nearest target that meets the
      // requirements
      sTmp = chr_search_nearest_target( ichr,
                                        HAS_ALL_BITS( scr_globals.tmpdistance, SEARCH_ITEMS   ),
                                        HAS_ALL_BITS( scr_globals.tmpdistance, SEARCH_FRIENDS ),
                                        HAS_ALL_BITS( scr_globals.tmpdistance, SEARCH_ENEMIES ),
                                        HAS_ALL_BITS( scr_globals.tmpdistance, SEARCH_DEAD    ),
                                        scr_globals.tmpargument );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        chraitarget[ichr] = sTmp;
        returncode = btrue;
      }
      break;

    case F_SetTargetToNearestEnemy:
      // This function finds the nearest target that meets the
      // requirements
      sTmp = chr_search_nearest_target( ichr, bfalse, bfalse, btrue, bfalse, IDSZ_NONE );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        chraitarget[ichr] = sTmp;
        returncode = btrue;
      }
      break;

    case F_SetTargetToNearestFriend:
      // This function finds the nearest target that meets the
      // requirements
      sTmp = chr_search_nearest_target( ichr, bfalse, btrue, bfalse, bfalse, IDSZ_NONE );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        chraitarget[ichr] = sTmp;
        returncode = btrue;
      }
      break;

    case F_SetTargetToNearestLifeform:
      // This function finds the nearest target that meets the
      // requirements
      sTmp = chr_search_nearest_target( ichr, bfalse, btrue, btrue, bfalse, IDSZ_NONE );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        chraitarget[ichr] = sTmp;
        returncode = btrue;
      }
      break;

    case F_FlashPassage:
      // This function makes the passage light or dark...  For debug...
      flash_passage( scr_globals.tmpargument, scr_globals.tmpdistance );
      break;

    case F_FindTileInPassage:
      // This function finds the next tile in the passage, tmpx and tmpy are
      // required and set on return
      returncode = search_tile_in_passage( scr_globals.tmpargument, scr_globals.tmpdistance );
      break;

    case F_IfHeldInLeftSaddle:
      // This function proceeds if the character is in the left hand of another
      // character
      returncode = bfalse;
      sTmp = chr_get_attachedto( ichr );
      if ( VALID_CHR( sTmp ) )
      {
        returncode = ( chrholdingwhich[sTmp][SLOT_SADDLE] == ichr );
      }
      break;

    case F_NotAnItem:
      // This function makes the character a non-item character
      chrisitem[ichr] = bfalse;
      break;

    case F_SetChildAmmo:
      // This function sets the child's ammo
      chrammo[loc_aichild] = scr_globals.tmpargument;
      break;

    case F_IfHitVulnerable:
      // This function proceeds if the character was hit by a weapon with the
      // correct vulnerability IDSZ...  [SILV] for Werewolves...
      returncode = HAS_SOME_BITS( chralert[ichr], ALERT_HITVULNERABLE );
      break;

    case F_IfTargetIsFlying:
      // This function proceeds if the character target is flying
      returncode = ( chrflyheight[loc_aitarget] > 0 );
      break;

    case F_IdentifyTarget:
      // This function reveals the target's name, ammo, and usage
      // Proceeds if the target was unknown
      returncode = bfalse;
      sTmp = chr_get_aitarget( ichr );
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

    case F_BeatModule:
      // This function displays the Module Ended message
      beatmodule = btrue;
      break;

    case F_EndModule:
      // This function presses the Escape key
      sdlkeybuffer[SDLK_ESCAPE] = 1;
      break;

    case F_DisableExport:
      // This function turns export off
      exportvalid = bfalse;
      break;

    case F_EnableExport:
      // This function turns export on
      exportvalid = btrue;
      break;

    case F_GetTargetState:
      // This function sets tmpargument to the state of the target
      scr_globals.tmpargument = chraistate[loc_aitarget];
      break;

    case F_ClearEndText:
      // This function empties the end-module text buffer
      endtext[0] = 0;
      endtextwrite = 0;
      break;

    case F_AddEndText:
      // This function appends a message to the end-module text buffer
      append_end_text( madmsgstart[loc_model] + scr_globals.tmpargument, ichr );
      break;

    case F_PlayMusic:
      // This function begins playing a new track of music
      if ( CData.musicvalid && ( songplaying != scr_globals.tmpargument ) )
      {
        play_music( scr_globals.tmpargument, scr_globals.tmpdistance, -1 );
      }
      break;

    case F_SetMusicPassage:
      // This function makes the given passage play music if a player enters it
      // tmpargument is the passage to set and tmpdistance is the music track to play...
      passagemusic[scr_globals.tmpargument] = scr_globals.tmpdistance;
      break;

    case F_MakeCrushInvalid:
      // This function makes doors unable to close on this object
      chrcanbecrushed[ichr] = bfalse;
      break;

    case F_StopMusic:
      // This function stops the interactive music
      stop_music(1000);
      break;

    case F_FlashVariable:
      // This function makes the character flash according to tmpargument
      flash_character( ichr, scr_globals.tmpargument );
      break;

    case F_AccelerateUp:
      // This function changes the character's up down velocity
      chrvel[ichr].z += scr_globals.tmpargument / 100.0;
      break;

    case F_FlashVariableHeight:
      // This function makes the character flash, feet one color, head another...
      flash_character_height( ichr, scr_globals.tmpturn, scr_globals.tmpx,
                              scr_globals.tmpdistance, scr_globals.tmpy );
      break;

    case F_SetDamageTime:
      // This function makes the character invincible for a little while
      chrdamagetime[ichr] = scr_globals.tmpargument;
      break;

    case F_IfStateIs8:
      returncode = ( 8 == chraistate[ichr] );
      break;

    case F_IfStateIs9:
      returncode = ( 9 == chraistate[ichr] );
      break;

    case F_IfStateIs10:
      returncode = ( 10 == chraistate[ichr] );
      break;

    case F_IfStateIs11:
      returncode = ( 11 == chraistate[ichr] );
      break;

    case F_IfStateIs12:
      returncode = ( 12 == chraistate[ichr] );
      break;

    case F_IfStateIs13:
      returncode = ( 13 == chraistate[ichr] );
      break;

    case F_IfStateIs14:
      returncode = ( 14 == chraistate[ichr] );
      break;

    case F_IfStateIs15:
      returncode = ( 15 == chraistate[ichr] );
      break;

    case F_IfTargetIsAMount:
      returncode = chrbmpdata[loc_aitarget].calc_is_mount;
      break;

    case F_IfTargetIsAPlatform:
      returncode = chrbmpdata[loc_aitarget].calc_is_platform;
      break;

    case F_AddStat:
      if ( !chrstaton[ichr] ) add_stat( ichr );
      break;

    case F_DisenchantTarget:
      returncode = ( chrfirstenchant[loc_aitarget] != MAXENCHANT );
      disenchant_character( loc_aitarget );
      break;

    case F_DisenchantAll:
      iTmp = 0;
      while ( iTmp < MAXENCHANT )
      {
        remove_enchant( iTmp );
        iTmp++;
      }
      break;

    case F_SetVolumeNearestTeammate:
	  //This sets the volume for the looping sounds of all the character's teammates
		if(moduleActive && scr_globals.tmpdistance >= 0 && CData.soundvalid)
      {
          //Go through all teammates
          sTmp = 0;
          while(sTmp < MAXCHR)
          {
              if(chron[sTmp] && chralive[sTmp] && chrteam[sTmp] == chrteam[ichr])
              {
			    //And set their volume to tmpdistance
		  	    if(scr_globals.tmpdistance >= 0 && chrloopingchannel[sTmp] != INVALID_SOUND)
			    {
					Mix_Volume(chrloopingchannel[sTmp], scr_globals.tmpdistance);
			    }
			  }
              sTmp++;
          }
      }
      break;

    case F_AddShopPassage:
      // This function defines a shop area
      add_shop_passage( ichr, scr_globals.tmpargument );
      break;

    case F_TargetPayForArmor:
      // This function costs the target some money, or fails if 'e doesn't have
      // enough...
      // tmpx is amount needed
      // tmpy is cost of new skin
      sTmp = chr_get_aitarget( ichr );    // The target
      tTmp = chrmodel[sTmp];           // The target's model
      iTmp =  capskincost[tTmp][scr_globals.tmpargument % MAXSKIN];
      scr_globals.tmpy = iTmp;                // Cost of new skin
      iTmp -= capskincost[tTmp][( chrtexture[sTmp] - madskinstart[tTmp] ) % MAXSKIN];   // Refund
      if ( iTmp > chrmoney[sTmp] )
      {
        // Not enough...
        scr_globals.tmpx = iTmp - chrmoney[sTmp];  // Amount needed
        returncode = bfalse;
      }
      else
      {
        // Pay for it...  Cost may be negative after refund...
        chrmoney[sTmp] -= iTmp;
        if ( chrmoney[sTmp] > MAXMONEY )  chrmoney[sTmp] = MAXMONEY;
        scr_globals.tmpx = 0;
        returncode = btrue;
      }
      break;

    case F_JoinEvilTeam:
      // This function adds the character to the evil team...
      switch_team( ichr, TEAM_EVIL );
      break;

    case F_JoinNullTeam:
      // This function adds the character to the null team...
      switch_team( ichr, TEAM_NULL );
      break;

    case F_JoinGoodTeam:
      // This function adds the character to the good team...
      switch_team( ichr, TEAM_GOOD );
      break;

    case F_PitsKill:
      // This function activates pit deaths...
      pitskill = btrue;
      break;

    case F_SetTargetToPassageID:
      // This function finds a character who is both in the passage and who has
      // an item with the given IDSZ
      sTmp = who_is_blocking_passage_ID( scr_globals.tmpargument, scr_globals.tmpdistance );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        chraitarget[ichr] = sTmp;
        returncode = btrue;
      }
      break;

    case F_MakeNameUnknown:
      // This function makes the name of an item/character unknown.
      chrnameknown[ichr] = bfalse;
      break;

    case F_SpawnExactParticleEndSpawn:
      {
        vect3 prt_pos = {scr_globals.tmpx, scr_globals.tmpy, scr_globals.tmpdistance};

        // This function spawns a particle that spawns a character...
        returncode = bfalse;
        tTmp = ichr;
        if ( chr_attached( ichr ) )  tTmp = chr_get_attachedto( ichr );

        tTmp = spawn_one_particle( 1.0f, prt_pos, chrturn_lr[ichr], loc_model, scr_globals.tmpargument, MAXCHR, 0, chrteam[ichr], tTmp, 0, MAXCHR );
        if ( VALID_PRT( tTmp ) )
        {
          prtspawncharacterstate[tTmp] = scr_globals.tmpturn;
          returncode = btrue;
        }
      }
      break;

    case F_SpawnPoofSpeedSpacingDamage:
      // This function makes a lovely little poof at the character's location,
      // adjusting the xy speed and spacing and the base damage first
      // Temporarily adjust the values for the particle type
      sTmp = loc_model;
      if ( MAXMODEL != sTmp )
      {
        if ( capgopoofprttype[sTmp] <= PRTPIP_PEROBJECT_COUNT )
        {
          sTmp = madprtpip[sTmp][capgopoofprttype[sTmp]];
        }
        else
        {
          sTmp = capgopoofprttype[sTmp];
        }

        if ( MAXPRTPIP != sTmp )
        {
          // store the base values
          iTmp = pipxyvel[sTmp].ibase;
          tTmp = pipxyspacing[sTmp].ibase;
          test = pipdamage_fp8[sTmp].ibase;

          // set some temporary values
          pipxyvel[sTmp].ibase = scr_globals.tmpx;
          pipxyspacing[sTmp].ibase = scr_globals.tmpy;
          pipdamage_fp8[sTmp].ibase = scr_globals.tmpargument;

          // do the poof
          spawn_poof( ichr, loc_model );

          // Restore the saved values
          pipxyvel[sTmp].ibase = iTmp;
          pipxyspacing[sTmp].ibase = tTmp;
          pipdamage_fp8[sTmp].ibase = test;
        };
      }
      break;

    case F_GiveExperienceToGoodTeam:
      // This function gives experience to everyone on the G Team
      give_team_experience( TEAM_GOOD, scr_globals.tmpargument, scr_globals.tmpdistance );
      break;

    case F_DoNothing:
      //This function does nothing (For use with combination with Else function or debugging)
      break;

    case F_DazeTarget:
      //This function dazes the target for a duration equal to tmpargument
      chrdazetime[loc_aitarget] += scr_globals.tmpargument;
      break;

    case F_GrogTarget:
      //This function grogs the target for a duration equal to tmpargument
      chrgrogtime[loc_aitarget] += scr_globals.tmpargument;
      break;

    case F_IfEquipped:
      //This proceeds if the character is equipped
      returncode = chrisequipped[ichr];
      break;

    case F_DropTargetMoney:
      // This function drops some of the target's money
      drop_money( loc_aitarget, scr_globals.tmpargument );
      break;

    case F_GetTargetContent:
      //This sets tmpargument to the current target's content value
      scr_globals.tmpargument = chraicontent[loc_aitarget];
      break;

    case F_DropTargetKeys:
      //This function makes the target drops keys in inventory (Not inhand)
      drop_keys( loc_aitarget );
      break;

    case F_JoinTeam:
      //This makes the character itself join a specified team (A = 0, B = 1, 23 = Z, etc.)
      switch_team( ichr, scr_globals.tmpargument );
      break;

    case F_TargetJoinTeam:
      //This makes the target join a team specified in tmpargument (A = 0, 23 = Z, etc.)
      switch_team( loc_aitarget, scr_globals.tmpargument );
      break;

    case F_ClearMusicPassage:
      //This clears the music for an specified passage
      passagemusic[scr_globals.tmpargument] = INVALID_SOUND;
      break;


    case F_AddQuest:
      //This function adds a quest idsz set in tmpargument into the targets quest.txt
      if ( chrisplayer[loc_aitarget] )
      {
        snprintf( cTmp, sizeof( cTmp ), "%s.obj", chrname[loc_aitarget] );
        returncode = add_quest_idsz( cTmp, scr_globals.tmpargument );
        //Todo: Should we add the quest idsz into all the players quest.txt?
      }
      break;

    case F_BeatQuest:
      //This function marks a IDSZ in the targets quest.txt as beaten
      if ( chrisplayer[loc_aitarget] )
      {
        snprintf( cTmp, sizeof( cTmp ), "%s.obj", chrname[loc_aitarget] );
        returncode = beat_quest_idsz( cTmp, scr_globals.tmpargument );
        //Todo: Should we beat the quest idsz into all the players quest.txt?
      }
      break;

    case F_IfTargetHasQuest:
      //This function proceeds if the target has the unfinished quest specified in tmpargument
      if ( chrisplayer[loc_aitarget] )
      {
        snprintf( cTmp, sizeof( cTmp ), "%s.obj", chrname[loc_aitarget] );
        iTmp = check_player_quest( cTmp, scr_globals.tmpargument );
        if ( iTmp > -1 ) returncode = btrue;
        else returncode = bfalse;
      }
      break;

    case F_IfTargetHasNotFullMana:
      //This function proceeds if the target has more than one point of mana and is alive
      returncode = bfalse;
      if ( VALID_CHR( loc_aitarget ) )
      {
        returncode = chralive[loc_aitarget] && chrmanamax_fp8[loc_aitarget] > 0 && chrmana_fp8[loc_aitarget] < chrmanamax_fp8[loc_aitarget] - DAMAGE_HURT;
      };
      break;

    case F_IfJumping:
      //This function proceeds if the character is preforming a jump animation
      returncode = chraction[ichr] >= ACTION_JA && chraction[ichr] <= ACTION_JC;
      break;

    case F_IfOperatorIsLinux:
      //This function proceeds if the computer is running a UNIX OS
	  
	  #ifdef __unix__  
	  returncode = btrue;		//Player running Linux
      #else	
	  returncode = bfalse;		//Player running something else.
	  #endif
      break;

    case F_IfTargetIsOwner:
	  //This function proceeds if the target is the characters owner
      returncode = loc_aitarget == loc_aiowner;
      break;

  /*  case F_SetCameraSwing:
	  //This function sets the camera swing rate
	    camswing = 0;
      camswingrate = scr_globals.tmpargument;
      camswingamp = scr_globals.tmpdistance;
      returncode = loc_aitarget == loc_aiowner;
      break;*/



    case F_End:
      break;

    default:
      assert( bfalse );

  }

  return returncode;
}

//--------------------------------------------------------------------------------------------
void set_operand( Uint8 variable )
{
  // ZZ> This function sets one of the tmp* values for scripted AI
  switch ( variable )
  {
    case VAR_TMP_X:
      scr_globals.tmpx = scr_globals.operationsum;
      break;

    case VAR_TMP_Y:
      scr_globals.tmpy = scr_globals.operationsum;
      break;

    case VAR_TMP_DISTANCE:
      scr_globals.tmpdistance = scr_globals.operationsum;
      break;

    case VAR_TMP_TURN:
      scr_globals.tmpturn = scr_globals.operationsum;
      break;

    case VAR_TMP_ARGUMENT:
      scr_globals.tmpargument = scr_globals.operationsum;
      break;

  }
}

//--------------------------------------------------------------------------------------------
void run_operand( Uint32 value, CHR_REF character )
{
  // ZZ> This function does the scripted arithmetic in operator,operand pairs
  int iTmp;

  CHR_REF loc_aitarget = chr_get_aitarget( character );
  CHR_REF loc_aiowner  = chr_get_aiowner( character );
  CHR_REF loc_aichild  = chr_get_aichild( character );
  CHR_REF loc_leader   = team_get_leader( chrteam[character] );

  // Get the operation code
  if ( IS_CONSTANT(value) )
  {
    // Get the working value from a constant, constants are all but high 5 bits
    iTmp = GET_CONSTANT(value);
  }
  else
  {
    // Get the working value from a register
    iTmp = 1;
    switch ( GET_VAR_BITS(value) )
    {
      case VAR_TMP_X:
        iTmp = scr_globals.tmpx;
        break;

      case VAR_TMP_Y:
        iTmp = scr_globals.tmpy;
        break;

      case VAR_TMP_DISTANCE:
        iTmp = scr_globals.tmpdistance;
        break;

      case VAR_TMP_TURN:
        iTmp = scr_globals.tmpturn;
        break;

      case VAR_TMP_ARGUMENT:
        iTmp = scr_globals.tmpargument;
        break;

      case VAR_RAND:
        iTmp = RANDIE;
        break;

      case VAR_SELF_X:
        iTmp = chrpos[character].x;
        break;

      case VAR_SELF_Y:
        iTmp = chrpos[character].y;
        break;

      case VAR_SELF_TURN:
        iTmp = chrturn_lr[character];
        break;

      case VAR_SELF_COUNTER:
        iTmp = chrmessagedata[character];
        break;

      case VAR_SELF_ORDER:
        iTmp = chrmessage[character];
        break;

      case VAR_SELF_MORALE:
        iTmp = teammorale[chrbaseteam[character]];
        break;

      case VAR_SELF_LIFE:
        iTmp = chrlife_fp8[character];
        break;

      case VAR_TARGET_X:
        iTmp = chrpos[loc_aitarget].x;
        break;

      case VAR_TARGET_Y:
        iTmp = chrpos[loc_aitarget].y;
        break;

      case VAR_TARGET_DISTANCE:
        iTmp = sqrt(( chrpos[loc_aitarget].x - chrpos[character].x ) * ( chrpos[loc_aitarget].x - chrpos[character].x ) +
                    ( chrpos[loc_aitarget].y - chrpos[character].y ) * ( chrpos[loc_aitarget].y - chrpos[character].y ) +
                    ( chrpos[loc_aitarget].z - chrpos[character].z ) * ( chrpos[loc_aitarget].z - chrpos[character].z ) );
        break;

      case VAR_TARGET_TURN:
        iTmp = chrturn_lr[loc_aitarget];
        break;

      case VAR_LEADER_X:
        iTmp = chrpos[character].x;
        if ( VALID_CHR( loc_leader ) )
          iTmp = chrpos[loc_leader].x;
        break;

      case VAR_LEADER_Y:
        iTmp = chrpos[character].y;
        if ( VALID_CHR( loc_leader ) )
          iTmp = chrpos[loc_leader].y;
        break;

      case VAR_LEADER_DISTANCE:
        iTmp = 10000;
        if ( VALID_CHR( loc_leader ) )
          iTmp = ABS(( int )( chrpos[loc_leader].x - chrpos[character].x ) ) +
                 ABS(( int )( chrpos[loc_leader].y - chrpos[character].y ) );
        break;

      case VAR_LEADER_TURN:
        iTmp = chrturn_lr[character];
        if ( VALID_CHR( loc_leader ) )
          iTmp = chrturn_lr[loc_leader];
        break;

      case VAR_GOTO_X:
        iTmp = chraigotox[character][chraigoto[character]];
        break;

      case VAR_GOTO_Y:
        iTmp = chraigotoy[character][chraigoto[character]];
        break;

      case VAR_GOTO_DISTANCE:
        iTmp = ABS(( int )( chraigotox[character][chraigoto[character]] - chrpos[character].x ) ) +
               ABS(( int )( chraigotoy[character][chraigoto[character]] - chrpos[character].y ) );
        break;

      case VAR_TARGET_TURNTO:
        iTmp = vec_to_turn( chrpos[loc_aitarget].x - chrpos[character].x, chrpos[loc_aitarget].y - chrpos[character].y );
        break;

      case VAR_PASSAGE:
        iTmp = chrpassage[character];
        break;

      case VAR_WEIGHT:
        iTmp = chrholdingweight[character];
        break;

      case VAR_SELF_ALTITUDE:
        iTmp = chrpos[character].z - chrlevel[character];
        break;

      case VAR_SELF_ID:
        iTmp = capidsz[chrmodel[character]][IDSZ_TYPE];
        break;

      case VAR_SELF_HATEID:
        iTmp = capidsz[chrmodel[character]][IDSZ_HATE];
        break;

      case VAR_SELF_MANA:
        iTmp = chrmana_fp8[character];
        if ( chrcanchannel[character] )  iTmp += chrlife_fp8[character];
        break;

      case VAR_TARGET_STR:
        iTmp = chrstrength_fp8[loc_aitarget];
        break;

      case VAR_TARGET_WIS:
        iTmp = chrwisdom_fp8[loc_aitarget];
        break;

      case VAR_TARGET_INT:
        iTmp = chrintelligence_fp8[loc_aitarget];
        break;

      case VAR_TARGET_DEX:
        iTmp = chrdexterity_fp8[loc_aitarget];
        break;

      case VAR_TARGET_LIFE:
        iTmp = chrlife_fp8[loc_aitarget];
        break;

      case VAR_TARGET_MANA:
        iTmp = chrmana_fp8[loc_aitarget];
        if ( chrcanchannel[loc_aitarget] )  iTmp += chrlife_fp8[loc_aitarget];
        break;

      case VAR_TARGET_LEVEL:
        iTmp = calc_chr_level( loc_aitarget );
        break;

      case VAR_TARGET_SPEEDX:
        iTmp = chrvel[loc_aitarget].x;
        break;

      case VAR_TARGET_SPEEDY:
        iTmp = chrvel[loc_aitarget].y;
        break;

      case VAR_TARGET_SPEEDZ:
        iTmp = chrvel[loc_aitarget].z;
        break;

      case VAR_SELF_SPAWNX:
        iTmp = chrstt[character].x;
        break;

      case VAR_SELF_SPAWNY:
        iTmp = chrstt[character].y;
        break;

      case VAR_SELF_STATE:
        iTmp = chraistate[character];
        break;

      case VAR_SELF_STR:
        iTmp = chrstrength_fp8[character];
        break;

      case VAR_SELF_WIS:
        iTmp = chrwisdom_fp8[character];
        break;

      case VAR_SELF_INT:
        iTmp = chrintelligence_fp8[character];
        break;

      case VAR_SELF_DEX:
        iTmp = chrdexterity_fp8[character];
        break;

      case VAR_SELF_MANAFLOW:
        iTmp = chrmanaflow_fp8[character];
        break;

      case VAR_TARGET_MANAFLOW:
        iTmp = chrmanaflow_fp8[loc_aitarget];
        break;

      case VAR_SELF_ATTACHED:
        iTmp = number_of_attached_particles( character );
        break;

      case VAR_SWINGTURN:
        iTmp = camswing << 2;
        break;

      case VAR_XYDISTANCE:
        iTmp = sqrt( scr_globals.tmpx * scr_globals.tmpx + scr_globals.tmpy * scr_globals.tmpy );
        break;

      case VAR_SELF_Z:
        iTmp = chrpos[character].z;
        break;

      case VAR_TARGET_ALTITUDE:
        iTmp = chrpos[loc_aitarget].z - chrlevel[loc_aitarget];
        break;

      case VAR_TARGET_Z:
        iTmp = chrpos[loc_aitarget].z;
        break;

      case VAR_SELF_INDEX:
        iTmp = character;
        break;

      case VAR_OWNER_X:
        iTmp = chrpos[loc_aiowner].x;
        break;

      case VAR_OWNER_Y:
        iTmp = chrpos[loc_aiowner].y;
        break;

      case VAR_OWNER_TURN:
        iTmp = chrturn_lr[loc_aiowner];
        break;

      case VAR_OWNER_DISTANCE:
        iTmp = sqrt(( chrpos[loc_aiowner].x - chrpos[character].x ) * ( chrpos[loc_aiowner].x - chrpos[character].x ) +
                    ( chrpos[loc_aiowner].y - chrpos[character].y ) * ( chrpos[loc_aiowner].y - chrpos[character].y ) +
                    ( chrpos[loc_aiowner].z - chrpos[character].z ) * ( chrpos[loc_aiowner].z - chrpos[character].z ) );
        break;

      case VAR_OWNER_TURNTO:
        iTmp = vec_to_turn( chrpos[loc_aiowner].x - chrpos[character].x, chrpos[loc_aiowner].y - chrpos[character].y );
        break;

      case VAR_XYTURNTO:
        iTmp = vec_to_turn( scr_globals.tmpx - chrpos[character].x, scr_globals.tmpy - chrpos[character].y );
        break;

      case VAR_SELF_MONEY:
        iTmp = chrmoney[character];
        break;

      case VAR_SELF_ACCEL:
        iTmp = ( chrmaxaccel[character] * 100.0 );
        break;

      case VAR_TARGET_EXP:
        iTmp = chrexperience[loc_aitarget];
        break;

      case VAR_SELF_AMMO:
        iTmp = chrammo[character];
        break;

      case VAR_TARGET_AMMO:
        iTmp = chrammo[loc_aitarget];
        break;

      case VAR_TARGET_MONEY:
        iTmp = chrmoney[loc_aitarget];
        break;

      case VAR_TARGET_TURNAWAY:
        iTmp = vec_to_turn( chrpos[character].x - chrpos[loc_aitarget].x, chrpos[character].y - chrpos[loc_aitarget].y );
        break;

      case VAR_SELF_LEVEL:
        iTmp = calc_chr_level( character );
        break;

      case VAR_SPAWN_DISTANCE:
        iTmp = sqrt(( chrstt[character].x - chrpos[character].x ) * ( chrstt[character].x - chrpos[character].x ) +
                    ( chrstt[character].y - chrpos[character].y ) * ( chrstt[character].y - chrpos[character].y ) +
                    ( chrstt[character].z - chrpos[character].z ) * ( chrstt[character].z - chrpos[character].z ) );
        break;

    }
  }


  // Now do the math
  switch ( GET_OP_BITS(value) )
  {
    case OP_ADD:
      scr_globals.operationsum += iTmp;
      break;

    case OP_SUB:
      scr_globals.operationsum -= iTmp;
      break;

    case OP_AND:
      scr_globals.operationsum &= iTmp;
      break;

    case OP_SHR:
      scr_globals.operationsum >>= iTmp;
      break;

    case OP_SHL:
      scr_globals.operationsum <<= iTmp;
      break;

    case OP_MUL:
      scr_globals.operationsum *= iTmp;
      break;

    case OP_DIV:
      if ( iTmp != 0 )
      {
        scr_globals.operationsum /= iTmp;
      }
      break;

    case OP_MOD:
      if ( iTmp != 0 )
      {
        scr_globals.operationsum %= iTmp;
      }
      break;

  }
}

//--------------------------------------------------------------------------------------------
void let_character_think( CHR_REF character, float dUpdate )
{
  // ZZ> This function lets one character do AI stuff
  Uint16 aicode;
  Uint32 index;
  Uint32 value;
  Uint32 iTmp;
  bool_t functionreturn;
  int operands;


  // Make life easier
  scr_globals.oldtarget = chr_get_aitarget( character );
  aicode = chraitype[character];


  // Figure out alerts that weren't already set
  set_alerts( character, dUpdate );

  // Clear the button latches
  if ( !chrisplayer[character] )
  {
    chrlatchbutton[character] = 0;
  }


  // Reset the target if it can't be seen
  if ( !chrcanseeinvisible[character] && chralive[character] )
  {
    if ( chr_is_invisible( character ) )
    {
      chraitarget[character] = character;
    }
  }


  // Run the AI Script
  index = scr_intern.script_address[aicode];


  value = scr_intern.compiled[index];
  while ( GET_FUNCTION_BITS( value ) != END_FUNCTION )  // End Function
  {
    value = scr_intern.compiled[index];
    // Was it a function
    if ( IS_FUNCTION( value ) )
    {
      // Run the function
      functionreturn = run_function( value, character );
      // Get the jump code
      index++;
      iTmp = scr_intern.compiled[index];
      if ( functionreturn )
      {
        // Proceed to the next function
        index++;
      }
      else
      {
        // Jump to where the jump code says to go
        index = iTmp;
      }
    }
    else
    {
      // Get the number of operands
      index++;
      operands = scr_intern.compiled[index];
      // Now run the operation
      scr_globals.operationsum = 0;
      index++;
      while ( operands > 0 )
      {
        iTmp = scr_intern.compiled[index];
        run_operand( iTmp, character );  // This sets scr_globals.operationsum
        operands--;
        index++;
      }
      // Save the results in the register that called the arithmetic
      set_operand( value );
    }
    // This is used by the Else function
    scr_globals.lastindent = value;
  }


  // Set latches
  if ( !chrisplayer[character] && aicode != 0 )
  {
    CHR_REF rider = chr_get_holdingwhich( character, SLOT_SADDLE );

    if ( chrismount[character] && VALID_CHR( rider ) && !chrisitem[rider] )
    {
      // Mount
      chrlatchx[character] = chrlatchx[rider];
      chrlatchy[character] = chrlatchy[rider];
    }
    else
    {
      float fnum, fden;
      // Normal AI
      chrlatchx[character] = chraigotox[character][chraigoto[character]] - chrpos[character].x;
      chrlatchy[character] = chraigotoy[character][chraigoto[character]] - chrpos[character].y;

      fnum = chrlatchx[character] * chrlatchx[character] + chrlatchy[character] * chrlatchy[character];
      fden = fnum + 25 * chrbmpdata[character].calc_size * chrbmpdata[character].calc_size;
      if ( fnum > 0.0f )
      {
        float ftmp = 1.0f / sqrt( fnum ) * fnum / fden;
        chrlatchx[character] *= ftmp;
        chrlatchy[character] *= ftmp;
      }
    }
  }


  // Clear alerts for next time around
  chralert[character] = 0;
  if ( chraimorphed[character] )
  {
    chralert[character] |= ALERT_CHANGED;
    chraimorphed[character] = bfalse;
  };
}

//--------------------------------------------------------------------------------------------
void let_ai_think( float dUpdate )
{
  // ZZ> This function lets every computer controlled character do AI stuff
  CHR_REF character;
  bool_t allow_thinking;


  numblip = 0;
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( character ) ) continue;

    allow_thinking = bfalse;

    // Cleaned up characters shouldn't be alert to anything else
    if ( HAS_SOME_BITS( chralert[character], ALERT_CRUSHED ) )
    {
      chralert[character] = ALERT_CRUSHED;
      allow_thinking = btrue;
    }

    if ( HAS_SOME_BITS( chralert[character], ALERT_CLEANEDUP ) )
    {
      chralert[character] = ALERT_CLEANEDUP;
      allow_thinking = btrue;
    };

    // Do not exclude items in packs. In NetHack, eggs can hatch while in your pack...
    if ( !chr_in_pack( character ) || chralive[character] )
    {
      allow_thinking = btrue;
    }

    if ( allow_thinking )
    {
      let_character_think( character, dUpdate );
    }
  }


}



