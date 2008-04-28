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

#include "egoboo_math.h"
#include "script.h"
#include "char.h"
#include "particle.h"
#include "Log.h"
#include "mesh.h"
#include "input.h"
#include "mad.h"
#include "passage.h"
#include "camera.h"
#include "enchant.h"
#include "egoboo_utility.h"
#include "egoboo.h"

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
  Uint32 operands;


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
  REGISTER_FUNCTION( opcode_lst, SetQuestLevel);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasNotFullMana);
  REGISTER_FUNCTION( opcode_lst, IfDoingAction);
  REGISTER_FUNCTION( opcode_lst, DropTargetKeys);
  REGISTER_FUNCTION( opcode_lst, TargetJoinTeam);
  REGISTER_FUNCTION( opcode_lst, GetTargetContent);
  REGISTER_FUNCTION( opcode_lst, JoinTeam);
  REGISTER_FUNCTION( opcode_lst, TargetJoinTeam);
  REGISTER_FUNCTION( opcode_lst, ClearMusicPassage);
  REGISTER_FUNCTION( opcode_lst, IfOperatorIsLinux);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsOwner);
  REGISTER_FUNCTION( opcode_lst, SetCameraSwing);
  REGISTER_FUNCTION( opcode_lst, EnableRespawn);
  REGISTER_FUNCTION( opcode_lst, DisableRespawn);
  REGISTER_FUNCTION( opcode_lst, IfButtonPressed);


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


//------------------------------------------------------------------------------
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
    MadList[cnt].ai = 0;

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
  CHR_REF loc_leader   = team_get_leader( ChrList[ichr].team );

  Uint16  loc_model    = ChrList[ichr].model;

  // Figure out which function to run
  switch ( valuecode )
  {
    case F_IfSpawned:
      // Proceed only if it's a new character
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_SPAWNED );
      break;

    case F_IfTimeOut:
      // Proceed only if time alert is set
      returncode = ( ChrList[ichr].aitime == 0.0f );
      break;

    case F_IfAtWaypoint:
      // Proceed only if the character reached a waypoint
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_ATWAYPOINT );
      break;

    case F_IfAtLastWaypoint:
      // Proceed only if the character reached its last waypoint
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_ATLASTWAYPOINT );
      break;

    case F_IfAttacked:
      // Proceed only if the character was damaged
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_ATTACKED );
      break;

    case F_IfBumped:
      // Proceed only if the character was bumped
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_BUMPED );
      break;

    case F_IfSignaled:
      // Proceed only if the character was GOrder.ed
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_SIGNALED );
      break;

    case F_IfCalledForHelp:
      // Proceed only if the character was called for help
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_CALLEDFORHELP );
      break;

    case F_SetContent:
      // Set the content
      ChrList[ichr].aicontent = scr_globals.tmpargument;
      break;

    case F_IfKilled:
      // Proceed only if the character's been killed
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_KILLED );
      break;

    case F_IfTargetKilled:
      // Proceed only if the character's target has just died
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_TARGETKILLED );
      break;

    case F_ClearWaypoints:
      // Clear out all waypoints
      ChrList[ichr].aigoto = 0;
      ChrList[ichr].aigotoadd = 0;
      ChrList[ichr].aigotox[0] = ChrList[ichr].pos.x;
      ChrList[ichr].aigotoy[0] = ChrList[ichr].pos.y;
      break;

    case F_AddWaypoint:
      // Add a waypoint to the waypoint list
      ChrList[ichr].aigotox[ChrList[ichr].aigotoadd] = scr_globals.tmpx;
      ChrList[ichr].aigotoy[ChrList[ichr].aigotoadd] = scr_globals.tmpy;
      ChrList[ichr].aigotoadd++;
      if ( ChrList[ichr].aigotoadd > MAXWAY )  ChrList[ichr].aigotoadd = MAXWAY - 1;
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
      scr_globals.tmpx = ChrList[loc_aitarget].pos.x;
      scr_globals.tmpy = ChrList[loc_aitarget].pos.y;
    }
    else
    {
      scr_globals.tmpx = (rand() & 1023) - 512 + ChrList[loc_aitarget].pos.x;
      scr_globals.tmpy = (rand() & 1023) - 512 + ChrList[loc_aitarget].pos.y;
    }
    if(scr_globals.tmpdistance == MOVE_RETREAT) scr_globals.tmpturn = (rand() & 32767) + scr_globals.tmpturn + 16384;
    else scr_globals.tmpturn = vec_to_turn( scr_globals.tmpx - ChrList[ichr].pos.x, scr_globals.tmpy - ChrList[ichr].pos.y );
    if(scr_globals.tmpdistance == MOVE_CHARGE || MOVE_RETREAT) reset_character_accel( ichr ); //Force 100% speed

    //Secondly we run the Compass function (If we are not in follow mode)
    if(scr_globals.tmpdistance != MOVE_FOLLOW)
    {
      sTmp = ( scr_globals.tmpturn + 16384 );
      scr_globals.tmpx -= turntosin[( sTmp>>2 ) & TRIGTABLE_MASK] * scr_globals.tmpdistance;
      scr_globals.tmpy -= turntosin[( scr_globals.tmpturn>>2 ) & TRIGTABLE_MASK] * scr_globals.tmpdistance;
    }

    //Then we add the waypoint(s), without clearing existing ones...
    ChrList[ichr].aigotox[ChrList[ichr].aigotoadd] = scr_globals.tmpx;
    ChrList[ichr].aigotoy[ChrList[ichr].aigotoadd] = scr_globals.tmpy;
    ChrList[ichr].aigotoadd++;
    if ( ChrList[ichr].aigotoadd > MAXWAY )  ChrList[ichr].aigotoadd = MAXWAY - 1;
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
      scr_globals.tmpx = CapList[ChrList[loc_aitarget].model].skincost[sTmp];
      break;

    case F_SetTime:
      // This function resets the time
      ChrList[ichr].aitime = scr_globals.tmpargument;
      break;

    case F_GetContent:
      // Get the content
      scr_globals.tmpargument = ChrList[ichr].aicontent;
      break;

    case F_JoinTargetTeam:
      // This function allows the character to leave its own team and join another
      returncode = bfalse;
      if ( ChrList[loc_aitarget].on )
      {
        switch_team( ichr, ChrList[loc_aitarget].team );
        returncode = btrue;
      }
      break;

    case F_SetTargetToNearbyEnemy:
      // This function finds a nearby enemy, and proceeds only if there is one
      sTmp = chr_search_nearby_target( ichr, bfalse, bfalse, btrue, bfalse, IDSZ_NONE );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        ChrList[ichr].aitarget = sTmp;
        returncode = btrue;
      }
      break;

    case F_SetTargetToTargetLeftHand:
      // This function sets the target to the target's left item
      sTmp = chr_get_holdingwhich( loc_aitarget, SLOT_LEFT );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        ChrList[ichr].aitarget = sTmp;
        returncode = btrue;
      }
      break;

    case F_SetTargetToTargetRightHand:
      // This function sets the target to the target's right item
      sTmp = chr_get_holdingwhich( loc_aitarget, SLOT_RIGHT );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        ChrList[ichr].aitarget = sTmp;
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
          ChrList[ichr].aitarget = attacklast;
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
      ChrList[ichr].aitarget = chr_get_aibumplast( ichr );
      break;

    case F_SetTargetToWhoeverCalledForHelp:
      // This function sets the target to whoever needs help
      ChrList[ichr].aitarget = team_get_sissy( ChrList[ichr].team );
      break;

    case F_SetTargetToOldTarget:
      // This function reverts to the target with whom the script started
      ChrList[ichr].aitarget = scr_globals.oldtarget;
      break;

    case F_SetTurnModeToVelocity:
      // This function sets the turn mode
      ChrList[ichr].turnmode = TURNMODE_VELOCITY;
      break;

    case F_SetTurnModeToWatch:
      // This function sets the turn mode
      ChrList[ichr].turnmode = TURNMODE_WATCH;
      break;

    case F_SetTurnModeToSpin:
      // This function sets the turn mode
      ChrList[ichr].turnmode = TURNMODE_SPIN;
      break;

    case F_SetBumpHeight:
      // This function changes a character's bump height
      //ChrList[ichr].bmpdata.height = scr_globals.tmpargument * ChrList[ichr].fat;
      //ChrList[ichr].bmpdata_save.height = scr_globals.tmpargument;
      break;

    case F_IfTargetHasID:
      // This function proceeds if ID matches tmpargument
      returncode = bfalse;
      if ( VALID_CHR( loc_aitarget ) )
      {
        returncode = CAP_INHERIT_IDSZ( ChrList[loc_aitarget].model, scr_globals.tmpargument );
      };
      break;

    case F_IfTargetHasItemID:
      // This function proceeds if the target has a matching item in his/her pack
      returncode = bfalse;
      // Check the pack
      sTmp  = chr_get_nextinpack( loc_aitarget );
      while ( VALID_CHR( sTmp ) )
      {
        if ( CAP_INHERIT_IDSZ( ChrList[sTmp].model, scr_globals.tmpargument ) )
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
          if ( CAP_INHERIT_IDSZ( ChrList[sTmp].model, scr_globals.tmpargument ) )
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
          if ( VALID_CHR( sTmp ) && CAP_INHERIT_IDSZ( ChrList[sTmp].model, scr_globals.tmpargument ) )
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
      ChrList[ichr].maxaccel *= .66;
      break;

    case F_Sneak:
      reset_character_accel( ichr );
      ChrList[ichr].maxaccel *= .33;
      break;

    case F_DoAction:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid or if the character is doing
      // something else already
      returncode = bfalse;
      if ( scr_globals.tmpargument < MAXACTION && ChrList[ichr].action.ready )
      {
        if ( MadList[loc_model].actionvalid[scr_globals.tmpargument] )
        {
          ChrList[ichr].action.now = scr_globals.tmpargument;
          ChrList[ichr].action.ready = bfalse;

          ChrList[ichr].anim.lip_fp8 = 0;
          ChrList[ichr].anim.flip = 0.0f;
          ChrList[ichr].anim.last = ChrList[ichr].anim.next;
          ChrList[ichr].anim.next = MadList[loc_model].actionstart[scr_globals.tmpargument];

          returncode = btrue;
        }
      }
      break;

    case F_KeepAction:
      // This function makes the current animation halt on the last frame
      ChrList[ichr].action.keep = btrue;
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
            ChrList[sTmp].vel.z     = DISMOUNTZVEL;
            ChrList[sTmp].pos.z    += DISMOUNTZVEL;
            ChrList[sTmp].jumptime = DELAY_JUMP;
          }
        }
      };
      break;

    case F_TargetDoAction:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid or if the target is doing
      // something else already
      returncode = bfalse;
      if ( ChrList[loc_aitarget].alive )
      {
        if ( scr_globals.tmpargument < MAXACTION && ChrList[loc_aitarget].action.ready )
        {
          if ( MadList[ChrList[loc_aitarget].model].actionvalid[scr_globals.tmpargument] )
          {
            ChrList[loc_aitarget].action.now = scr_globals.tmpargument;
            ChrList[loc_aitarget].action.ready = bfalse;

            ChrList[loc_aitarget].anim.lip_fp8 = 0;
            ChrList[loc_aitarget].anim.flip    = 0.0f;
            ChrList[loc_aitarget].anim.last    = ChrList[loc_aitarget].anim.next;
            ChrList[loc_aitarget].anim.next    = MadList[ChrList[loc_aitarget].model].actionstart[scr_globals.tmpargument];

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
      if ( !ChrList[ichr].isplayer )
      {
        returncode = btrue;
        ChrList[ichr].gopoof = btrue;
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
        if ( CAP_INHERIT_IDSZ( ChrList[sTmp].model, scr_globals.tmpargument ) )
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
          if ( CAP_INHERIT_IDSZ( ChrList[sTmp].model, scr_globals.tmpargument ) )
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
        if ( ChrList[iTmp].ammo <= 1 )
        {
          // Poof the item
          if ( chr_in_pack( iTmp ) )
          {
            // Remove from the pack
            ChrList[tTmp].nextinpack  = chr_get_nextinpack( iTmp );
            ChrList[loc_aitarget].numinpack--;
            ChrList[iTmp].freeme = btrue;
          }
          else
          {
            // Drop from hand
            detach_character_from_mount( iTmp, btrue, bfalse );
            ChrList[iTmp].freeme = btrue;
          }
        }
        else
        {
          // Cost one ammo
          ChrList[iTmp].ammo--;
        }
      }
      break;

    case F_DoActionOverride:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid
      returncode = bfalse;
      if ( scr_globals.tmpargument < MAXACTION )
      {
        if ( MadList[loc_model].actionvalid[scr_globals.tmpargument] )
        {
          ChrList[ichr].action.now   = scr_globals.tmpargument;
          ChrList[ichr].action.ready = bfalse;

          ChrList[ichr].anim.lip_fp8 = 0;
          ChrList[ichr].anim.flip    = 0.0f;
          ChrList[ichr].anim.last    = ChrList[ichr].anim.next;
          ChrList[ichr].anim.next    = MadList[loc_model].actionstart[scr_globals.tmpargument];

          returncode = btrue;
        }
      }
      break;

    case F_IfHealed:
      // Proceed only if the character was healed
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_HEALED );
      break;

    case F_DisplayMessage:
      // This function sends a message to the players
      display_message( MadList[loc_model].msg_start + scr_globals.tmpargument, ichr );
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
      ChrList[ichr].aistate = scr_globals.tmpargument;
      break;

    case F_GetState:
      // This function reads the character's state variable
      scr_globals.tmpargument = ChrList[ichr].aistate;
      break;

    case F_IfStateIs:
      // This function fails if the character's state is inequal to tmpargument
      returncode = ( scr_globals.tmpargument == ChrList[ichr].aistate );
      break;

    case F_IfTargetCanOpenStuff:
      // This function fails if the target can't open stuff
      returncode = ChrList[loc_aitarget].openstuff;
      break;

    case F_IfGrabbed:
      // Proceed only if the character was picked up
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_GRABBED );
      break;

    case F_IfDropped:
      // Proceed only if the character was dropped
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_DROPPED );
      break;

    case F_SetTargetToWhoeverIsHolding:
      // This function sets the target to the character's mount or holder,
      // failing if the character has no mount or holder
      returncode = bfalse;
      if ( chr_attached( ichr ) )
      {
        ChrList[ichr].aitarget = chr_get_attachedto( ichr );
        returncode = btrue;
      }
      break;

    case F_DamageTarget:
      {
        // This function applies little bit of love to the character's target.
        // The amount is set in tmpargument
        PAIR ptemp = {scr_globals.tmpargument, 1};
        damage_character( loc_aitarget, 0, &ptemp, ChrList[ichr].damagetargettype, ChrList[ichr].team, ichr, DAMFX_BLOC );
      }
      break;

    case F_IfXIsLessThanY:
      // Proceed only if tmpx is less than tmpy
      returncode = ( scr_globals.tmpx < scr_globals.tmpy );
      break;

    case F_SetWeatherTime:
      // Set the weather timer
      GWeather.timereset = scr_globals.tmpargument;
      GWeather.time = scr_globals.tmpargument;
      break;

    case F_GetBumpHeight:
      // Get the characters bump height
      scr_globals.tmpargument = ChrList[ichr].bmpdata.calc_height;
      break;

    case F_IfReaffirmed:
      // Proceed only if the character was reaffirmed
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_REAFFIRMED );
      break;

    case F_UnkeepAction:
      // This function makes the current animation start again
      ChrList[ichr].action.keep = bfalse;
      break;

    case F_IfTargetIsOnOtherTeam:
      // This function proceeds only if the target is on another team
      returncode = ( ChrList[loc_aitarget].alive && ChrList[loc_aitarget].team != ChrList[ichr].team );
      break;

    case F_IfTargetIsOnHatedTeam:
      // This function proceeds only if the target is on an enemy team
      returncode = ( ChrList[loc_aitarget].alive && TeamList[ChrList[ichr].team].hatesteam[ChrList[loc_aitarget].team] && !ChrList[loc_aitarget].invictus );
      break;

    case F_PressLatchButton:
      // This function sets the latch buttons
      ChrList[ichr].latch.b |= scr_globals.tmpargument;
      break;

    case F_SetTargetToTargetOfLeader:
      // This function sets the character's target to the target of its leader,
      // or it fails with no change if the leader is dead
      returncode = bfalse;
      if ( VALID_CHR( loc_leader ) )
      {
        ChrList[ichr].aitarget = chr_get_aitarget( loc_leader );
        returncode = btrue;
      }
      break;

    case F_IfLeaderKilled:
      // This function proceeds only if the character's leader has just died
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_LEADERKILLED );
      break;

    case F_BecomeLeader:
      // This function makes the character the team leader
      TeamList[ChrList[ichr].team].leader = ichr;
      break;

    case F_ChangeTargetArmor:
      // This function sets the target's armor type and returns the old type
      // as tmpargument and the new type as tmpx
      iTmp = ( ChrList[loc_aitarget].texture - MadList[ChrList[loc_aitarget].model].skinstart ) % MAXSKIN;
      scr_globals.tmpx = change_armor( loc_aitarget, scr_globals.tmpargument );
      scr_globals.tmpargument = iTmp;  // The character's old armor
      break;

    case F_GiveMoneyToTarget:
      // This function transfers money from the character to the target, and sets
      // tmpargument to the amount transferred
      iTmp = ChrList[ichr].money;
      tTmp = ChrList[loc_aitarget].money;
      iTmp -= scr_globals.tmpargument;
      tTmp += scr_globals.tmpargument;
      if ( iTmp < 0 ) { tTmp += iTmp;  scr_globals.tmpargument += iTmp;  iTmp = 0; }
      if ( tTmp < 0 ) { iTmp += tTmp;  scr_globals.tmpargument += tTmp;  tTmp = 0; }
      if ( iTmp > MAXMONEY ) { iTmp = MAXMONEY; }
      if ( tTmp > MAXMONEY ) { tTmp = MAXMONEY; }
      ChrList[ichr].money = iTmp;
      ChrList[loc_aitarget].money = tTmp;
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
        ChrList[ichr].aitarget = loc_leader;
      }
      break;

    case F_SpawnCharacter:
      {
        vect3 chr_pos = {scr_globals.tmpx, scr_globals.tmpy, 0};

        // This function spawns a ichr, failing if x,y is invalid
        sTmp = spawn_one_character( chr_pos, loc_model, ChrList[ichr].team, 0, scr_globals.tmpturn, NULL, MAXCHR );
        returncode = bfalse;
        if ( VALID_CHR( sTmp ) )
        {
          if ( 0 != __chrhitawall( sTmp, NULL ) )
          {
            ChrList[sTmp].freeme = btrue;
          }
          else
          {
            tTmp = ChrList[ichr].turn_lr >> 2;
            ChrList[sTmp].vel.x += turntosin[( tTmp+8192+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK] * scr_globals.tmpdistance;
            ChrList[sTmp].vel.y += turntosin[( tTmp+8192 ) & TRIGTABLE_MASK] * scr_globals.tmpdistance;
            ChrList[sTmp].passage = ChrList[ichr].passage;
            ChrList[sTmp].iskursed = bfalse;
            ChrList[ichr].aichild = sTmp;
            ChrList[sTmp].aiowner = loc_aiowner;
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
      mesh_set_tile( MESH_FLOAT_TO_FAN( ChrList[ichr].pos.x ), MESH_FLOAT_TO_FAN( ChrList[ichr].pos.y ), scr_globals.tmpargument & 0xFF );
      break;

    case F_IfUsed:
      // This function proceeds only if the character has been used
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_USED );
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
      returncode = ( CapList[ChrList[loc_aitarget].model].idsz[IDSZ_VULNERABILITY] == ( IDSZ ) scr_globals.tmpargument );
      break;

    case F_CleanUp:
      // This function issues the clean up order to all teammates
      issue_clean( ichr );
      break;

    case F_IfCleanedUp:
      // This function proceeds only if the character was told to clean up
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_CLEANEDUP );
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
        returncode = ChrList[loc_aitarget].alive &&  ChrList[loc_aitarget].lifemax_fp8 > 0 && ChrList[loc_aitarget].life_fp8 < ChrList[loc_aitarget].lifemax_fp8 - DAMAGE_HURT;
      }
      break;

    case F_IfTargetIsAPlayer:
      // This function proceeds only if the target is a player ( may not be local )
      returncode = ChrList[loc_aitarget].isplayer;
      break;

    case F_PlaySound:
      // This function plays a sound
      returncode = bfalse;
      if ( INVALID_SOUND != scr_globals.tmpargument && ChrList[ichr].pos_old.z > PITNOSOUND )
      {
        returncode = ( INVALID_SOUND != play_sound( 1.0f, ChrList[ichr].pos_old, CapList[loc_model].wavelist[scr_globals.tmpargument], 0, loc_model, scr_globals.tmpargument) );
      }
      break;

    case F_SpawnParticle:
      // This function spawns a particle
      returncode = bfalse;
      if ( VALID_CHR( ichr ) && !chr_in_pack( ichr ) )
      {
        tTmp = ichr;
        if ( chr_attached( ichr ) )  tTmp = chr_get_attachedto( ichr );
        tTmp = spawn_one_particle( 1.0f, ChrList[ichr].pos, ChrList[ichr].turn_lr, loc_model, scr_globals.tmpargument, ichr, scr_globals.tmpdistance, ChrList[ichr].team, tTmp, 0, MAXCHR );

        if ( VALID_PRT( tTmp ) )
        {
          // Detach the particle
          attach_particle_to_character( tTmp, ichr, scr_globals.tmpdistance );
          PrtList[tTmp].attachedtochr = MAXCHR;

          // Correct X, Y, Z spacing
          PrtList[tTmp].pos.x += scr_globals.tmpx;
          PrtList[tTmp].pos.y += scr_globals.tmpy;
          PrtList[tTmp].pos.z += PipList[PrtList[tTmp].pip].zspacing.ibase;

          // Don't spawn in walls
          if ( 0 != __prthitawall( tTmp, NULL ) )
          {
            PrtList[tTmp].pos.x = ChrList[ichr].pos.x;
            if ( 0 != __prthitawall( tTmp, NULL ) )
            {
              PrtList[tTmp].pos.y = ChrList[ichr].pos.y;
            }
          }
          returncode = btrue;
        }
      }
      break;

    case F_IfTargetIsAlive:
      // This function proceeds only if the target is alive
      returncode = ChrList[loc_aitarget].alive;
      break;

    case F_Stop:
      ChrList[ichr].maxaccel = 0;
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
      returncode = ( ChrList[ichr].gender == GEN_MALE );
      break;

    case F_IfTargetIsFemale:
      // This function proceeds only if the target is female
      returncode = ( ChrList[ichr].gender == GEN_FEMALE );
      break;

    case F_SetTargetToSelf:
      // This function sets the target to the character
      ChrList[ichr].aitarget = ichr;
      break;

    case F_SetTargetToRider:
      // This function sets the target to the character's left/only grip weapon,
      // failing if there is none
      returncode = bfalse;
      if ( chr_using_slot( ichr, SLOT_SADDLE ) )
      {
        ChrList[ichr].aitarget = chr_get_holdingwhich( ichr, SLOT_SADDLE );
        returncode = btrue;
      }
      break;

    case F_GetAttackTurn:
      // This function sets tmpturn to the direction of the last attack
      scr_globals.tmpturn = ChrList[ichr].directionlast;
      break;

    case F_GetDamageType:
      // This function gets the last type of damage
      scr_globals.tmpargument = ChrList[ichr].damagetypelast;
      break;

    case F_BecomeSpell:
      // This function turns the spellbook character into a spell based on its
      // content
      ChrList[ichr].money = ( ChrList[ichr].texture - MadList[loc_model].skinstart ) % MAXSKIN;
      change_character( ichr, ChrList[ichr].aicontent, 0, LEAVE_NONE );
      ChrList[ichr].aicontent = 0;  // Reset so it doesn't mess up
      ChrList[ichr].aistate   = 0;  // Reset so it doesn't mess up
      ChrList[ichr].aimorphed = btrue;
      break;

    case F_BecomeSpellbook:
      // This function turns the spell into a spellbook, and sets the content
      // accordingly
      ChrList[ichr].aicontent = loc_model;
      change_character( ichr, SPELLBOOK, ChrList[ichr].money % MAXSKIN, LEAVE_NONE );
      ChrList[ichr].aistate = 0;  // Reset so it doesn't burn up
      ChrList[ichr].aimorphed = btrue;
      break;

    case F_IfScoredAHit:
      // Proceed only if the character scored a hit
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_SCOREDAHIT );
      break;

    case F_IfDisaffirmed:
      // Proceed only if the character was disaffirmed
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_DISAFFIRMED );
      break;

    case F_DecodeOrder:
      // This function gets the order and sets tmpx, tmpy, tmpargument and the
      // target ( if valid )

      returncode = bfalse;
      sTmp = ChrList[ichr].message >> 20;
      if ( VALID_CHR( sTmp ) )
      {
        ChrList[ichr].aitarget  = sTmp;
        scr_globals.tmpx        = (( ChrList[ichr].message >> 12 ) & 0x00FF ) << 8;
        scr_globals.tmpy        = (( ChrList[ichr].message >>  4 ) & 0x00FF ) << 8;
        scr_globals.tmpargument = ChrList[ichr].message & 0x000F;
        returncode = btrue;
      }
      break;

    case F_SetTargetToWhoeverWasHit:
      // This function sets the target to whoever the character hit last,
      ChrList[ichr].aitarget = chr_get_aihitlast( ichr );
      break;

    case F_SetTargetToWideEnemy:
      // This function finds an enemy, and proceeds only if there is one
      sTmp = chr_search_wide_target( ichr, bfalse, bfalse, btrue, bfalse, IDSZ_NONE, bfalse );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        ChrList[ichr].aitarget = sTmp;
        returncode = btrue;
      }
      break;

    case F_IfChanged:
      // Proceed only if the character was polymorphed
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_CHANGED );
      break;

    case F_IfInWater:
      // Proceed only if the character got wet
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_INWATER );
      break;

    case F_IfBored:
      // Proceed only if the character is bored
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_BORED );
      break;

    case F_IfTooMuchBaggage:
      // Proceed only if the character tried to grab too much
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_TOOMUCHBAGGAGE );
      break;

    case F_IfGrogged:
      // Proceed only if the character was grogged
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_GROGGED );
      break;

    case F_IfDazed:
      // Proceed only if the character was dazed
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_DAZED );
      break;

    case F_IfTargetHasSpecialID:
      // This function proceeds if ID matches tmpargument
      returncode = ( CapList[ChrList[loc_aitarget].model].idsz[IDSZ_SPECIAL] == ( IDSZ ) scr_globals.tmpargument );
      break;

    case F_PressTargetLatchButton:
      // This function sets the target's latch buttons
      ChrList[loc_aitarget].latch.b |= scr_globals.tmpargument;
      break;

    case F_IfInvisible:
      // This function passes if the character is invisible
      returncode = chr_is_invisible( ichr );
      break;

    case F_IfArmorIs:
      // This function passes if the character's skin is tmpargument
      tTmp = ( ChrList[ichr].texture - MadList[loc_model].skinstart ) % MAXSKIN;
      returncode = ( tTmp == scr_globals.tmpargument );
      break;

    case F_GetTargetGrogTime:
      // This function returns tmpargument as the grog time, and passes if it is not 0
      scr_globals.tmpargument = ChrList[ichr].grogtime;
      returncode = ( scr_globals.tmpargument != 0 );
      break;

    case F_GetTargetDazeTime:
      // This function returns tmpargument as the daze time, and passes if it is not 0
      scr_globals.tmpargument = ChrList[ichr].dazetime;
      returncode = ( scr_globals.tmpargument != 0 );
      break;

    case F_SetDamageType:
      // This function sets the bump damage type
      ChrList[ichr].damagetargettype = scr_globals.tmpargument & ( MAXDAMAGETYPE - 1 );
      break;

    case F_SetWaterLevel:
      // This function raises and lowers the module's water
      fTmp = ( scr_globals.tmpargument / 10.0 ) - GWater.douselevel;
      GWater.surfacelevel += fTmp;
      GWater.douselevel += fTmp;
      for ( iTmp = 0; iTmp < MAXWATERLAYER; iTmp++ )
        GWater.layerz[iTmp] += fTmp;
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
        ChrList[sTmp].pos_old = ChrList[sTmp].pos;

        ChrList[sTmp].pos.x = scr_globals.tmpx;
        ChrList[sTmp].pos.y = scr_globals.tmpy;
        ChrList[sTmp].pos.z = scr_globals.tmpdistance;
        if ( 0 != __chrhitawall( sTmp, NULL ) )
        {
          // No it didn't...
          ChrList[sTmp].pos = ChrList[sTmp].pos_old;
          returncode = bfalse;
        }
        else
        {
          ChrList[sTmp].pos_old = ChrList[sTmp].pos;
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
      if ( ChrList[ichr].ammo < ChrList[ichr].ammomax )
      {
        ChrList[ichr].ammo++;
      }
      break;

    case F_UnkurseTarget:
      // This function unkurses the target
      ChrList[loc_aitarget].iskursed = bfalse;
      break;

    case F_GiveExperienceToTargetTeam:
      // This function gives experience to everyone on the target's team
      give_team_experience( ChrList[loc_aitarget].team, scr_globals.tmpargument, scr_globals.tmpdistance );
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
      ChrList[ichr].redshift = scr_globals.tmpargument;
      break;

    case F_SetGreenShift:
      // This function alters a character's coloration
      ChrList[ichr].grnshift = scr_globals.tmpargument;
      break;

    case F_SetBlueShift:
      // This function alters a character's coloration
      ChrList[ichr].blushift = scr_globals.tmpargument;
      break;

    case F_SetLight:
      // This function alters a character's transparency
      ChrList[ichr].light_fp8 = scr_globals.tmpargument;
      break;

    case F_SetAlpha:
      // This function alters a character's transparency
      ChrList[ichr].alpha_fp8    = scr_globals.tmpargument;
      ChrList[ichr].bumpstrength = CapList[loc_model].bumpstrength * FP8_TO_FLOAT( scr_globals.tmpargument );
      break;

    case F_IfHitFromBehind:
      // This function proceeds if the character was attacked from behind
      returncode = bfalse;
      if ( ChrList[ichr].directionlast >= BEHIND - 8192 && ChrList[ichr].directionlast < BEHIND + 8192 )
        returncode = btrue;
      break;

    case F_IfHitFromFront:
      // This function proceeds if the character was attacked from the front
      returncode = bfalse;
      if ( ChrList[ichr].directionlast >= 49152 + 8192 || ChrList[ichr].directionlast < FRONT + 8192 )
        returncode = btrue;
      break;

    case F_IfHitFromLeft:
      // This function proceeds if the character was attacked from the left
      returncode = bfalse;
      if ( ChrList[ichr].directionlast >= LEFT - 8192 && ChrList[ichr].directionlast < LEFT + 8192 )
        returncode = btrue;
      break;

    case F_IfHitFromRight:
      // This function proceeds if the character was attacked from the right
      returncode = bfalse;
      if ( ChrList[ichr].directionlast >= RIGHT - 8192 && ChrList[ichr].directionlast < RIGHT + 8192 )
        returncode = btrue;
      break;

    case F_IfTargetIsOnSameTeam:
      // This function proceeds only if the target is on another team
      returncode = bfalse;
      if ( ChrList[loc_aitarget].team == ChrList[ichr].team )
        returncode = btrue;
      break;

    case F_KillTarget:
      // This function kills the target
      kill_character( loc_aitarget, ichr );
      break;

    case F_UndoEnchant:
      // This function undoes the last enchant
      returncode = ( ChrList[ichr].undoenchant != MAXENCHANT );
      remove_enchant( ChrList[ichr].undoenchant );
      break;

    case F_GetWaterLevel:
      // This function gets the douse level for the water, returning it in tmpargument
      scr_globals.tmpargument = GWater.douselevel * 10;
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
        if ( CapList[ChrList[loc_aitarget].model].idsz[tTmp] == ( IDSZ ) scr_globals.tmpargument )
        {
          returncode = btrue;
          break;
        }
      }
      break;

    case F_SetBumpSize:
      // This function sets the character's bump size
      //fTmp = ChrList[ichr].bmpdata.calc_size_big;
      //fTmp /= ChrList[ichr].bmpdata.calc_size;  // 1.5 or 2.0
      //ChrList[ichr].bmpdata.size = scr_globals.tmpargument * ChrList[ichr].fat;
      //ChrList[ichr].bmpdata.sizebig = fTmp * ChrList[ichr].bmpdata.calc_size;
      //ChrList[ichr].bmpdata_save.size = scr_globals.tmpargument;
      //ChrList[ichr].bmpdata_save.sizebig = fTmp * ChrList[ichr].bmpdata_save.size;
      break;

    case F_IfNotDropped:
      // This function passes if a kursed item could not be dropped
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_NOTDROPPED );
      break;

    case F_IfYIsLessThanX:
      // This function passes only if tmpy is less than tmpx
      returncode = ( scr_globals.tmpy < scr_globals.tmpx );
      break;

    case F_SetFlyHeight:
      // This function sets a character's fly height
      ChrList[ichr].flyheight = scr_globals.tmpargument;
      break;

    case F_IfBlocked:
      // This function passes if the character blocked an attack
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_BLOCKED );
      break;

    case F_IfTargetIsDefending:
      returncode = ( ChrList[loc_aitarget].action.now >= ACTION_PA && ChrList[loc_aitarget].action.now <= ACTION_PD );
      break;

    case F_IfTargetIsAttacking:
      returncode = ( ChrList[loc_aitarget].action.now >= ACTION_UA && ChrList[loc_aitarget].action.now <= ACTION_FD );
      break;

    case F_IfStateIs0:
      returncode = ( 0 == ChrList[ichr].aistate );
      break;

    case F_IfStateIs1:
      returncode = ( 1 == ChrList[ichr].aistate );
      break;

    case F_IfStateIs2:
      returncode = ( 2 == ChrList[ichr].aistate );
      break;

    case F_IfStateIs3:
      returncode = ( 3 == ChrList[ichr].aistate );
      break;

    case F_IfStateIs4:
      returncode = ( 4 == ChrList[ichr].aistate );
      break;

    case F_IfStateIs5:
      returncode = ( 5 == ChrList[ichr].aistate );
      break;

    case F_IfStateIs6:
      returncode = ( 6 == ChrList[ichr].aistate );
      break;

    case F_IfStateIs7:
      returncode = ( 7 == ChrList[ichr].aistate );
      break;

    case F_IfContentIs:
      returncode = ( scr_globals.tmpargument == ChrList[ichr].aicontent );
      break;

    case F_SetTurnModeToWatchTarget:
      // This function sets the turn mode
      ChrList[ichr].turnmode = TURNMODE_WATCHTARGET;
      break;

    case F_IfStateIsNot:
      returncode = ( scr_globals.tmpargument != ChrList[ichr].aistate );
      break;

    case F_IfXIsEqualToY:
      returncode = ( scr_globals.tmpx == scr_globals.tmpy );
      break;

    case F_DisplayDebugMessage:
      // This function spits out a debug message
      if( CData.DevMode )
      {
        debug_message( 1, "aistate %d, aicontent %d, target %d", ChrList[ichr].aistate, ChrList[ichr].aicontent, ChrList[ichr].aitarget );
        debug_message( 1, "tmpx %d, tmpy %d", scr_globals.tmpx, scr_globals.tmpy );
        debug_message( 1, "tmpdistance %d, tmpturn %d", scr_globals.tmpdistance, scr_globals.tmpturn );
        debug_message( 1, "tmpargument %d, selfturn %d", scr_globals.tmpargument, ChrList[ichr].turn_lr );
      }
      break;

    case F_BlackTarget:
      // This function makes the target flash black
      flash_character( loc_aitarget, 0 );
      break;

    case F_DisplayMessageNear:
      // This function sends a message if the camera is in the nearby area.
      iTmp = ABS( ChrList[ichr].pos_old.x - GCamera.trackpos.x ) + ABS( ChrList[ichr].pos_old.y - GCamera.trackpos.y );
      if ( iTmp < MSGDISTANCE )
      {
        display_message( MadList[loc_model].msg_start + scr_globals.tmpargument, ichr );
      }
      break;

    case F_IfHitGround:
      // This function passes if the character just hit the ground
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_HITGROUND );
      break;

    case F_IfNameIsKnown:
      // This function passes if the character's name is known
      returncode = ChrList[ichr].nameknown;
      break;

    case F_IfUsageIsKnown:
      // This function passes if the character's usage is known
      returncode = CapList[loc_model].usageknown;
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
          if ( CAP_INHERIT_IDSZ( ChrList[sTmp].model, scr_globals.tmpargument ) )
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
          sTmp = ChrList[tTmp].model;
          if ( CapList[sTmp].isranged && ( ChrList[tTmp].ammomax == 0 || ( ChrList[tTmp].ammo > 0 && ChrList[tTmp].ammoknown ) ) )
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
          if ( !CapList[ChrList[sTmp].model].isranged && CapList[ChrList[sTmp].model].weaponaction != ACTION_PA )
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
        if ( VALID_CHR( sTmp ) && CapList[ChrList[sTmp].model].weaponaction == ACTION_PA )
        {
          scr_globals.tmpargument = slot_to_latch( ichr, _slot );
          returncode = btrue;
        }
      };
      break;

    case F_IfKursed:
      // This function passes if the character is kursed
      returncode = ChrList[ichr].iskursed;
      break;

    case F_IfTargetIsKursed:
      // This function passes if the target is kursed
      returncode = ChrList[loc_aitarget].iskursed;
      break;

    case F_IfTargetIsDressedUp:
      // This function passes if the character's skin is dressy
      iTmp = ( ChrList[ichr].texture - MadList[loc_model].skinstart ) % MAXSKIN;
      iTmp = 1 << iTmp;
      returncode = (( CapList[loc_model].skindressy & iTmp ) != 0 );
      break;

    case F_IfOverWater:
      // This function passes if the character is on a water tile
      returncode = mesh_has_some_bits( ChrList[ichr].onwhichfan, MESHFX_WATER ) && GWater.iswater;
      break;

    case F_IfThrown:
      // This function passes if the character was thrown
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_THROWN );
      break;

    case F_MakeNameKnown:
      // This function makes the name of an item/character known.
      ChrList[ichr].nameknown = btrue;
      ChrList[ichr].icon = btrue;
      break;

    case F_MakeUsageKnown:
      // This function makes the usage of an item known...  For XP gains from
      // using an unknown potion or such
      CapList[loc_model].usageknown = btrue;
      break;

    case F_StopTargetMovement:
      // This function makes the target stop moving temporarily
      ChrList[loc_aitarget].vel.x = 0;
      ChrList[loc_aitarget].vel.y = 0;
      if ( ChrList[loc_aitarget].vel.z > 0 ) ChrList[loc_aitarget].vel.z = gravity;
      break;

    case F_SetXY:
      // This function stores tmpx and tmpy in the storage array
      ChrList[ichr].aix[scr_globals.tmpargument&STORAND] = scr_globals.tmpx;
      ChrList[ichr].aiy[scr_globals.tmpargument&STORAND] = scr_globals.tmpy;
      break;

    case F_GetXY:
      // This function gets previously stored data, setting tmpx and tmpy
      scr_globals.tmpx = ChrList[ichr].aix[scr_globals.tmpargument&STORAND];
      scr_globals.tmpy = ChrList[ichr].aiy[scr_globals.tmpargument&STORAND];
      break;

    case F_AddXY:
      // This function adds tmpx and tmpy to the storage array
      ChrList[ichr].aix[scr_globals.tmpargument&STORAND] += scr_globals.tmpx;
      ChrList[ichr].aiy[scr_globals.tmpargument&STORAND] += scr_globals.tmpy;
      break;

    case F_MakeAmmoKnown:
      // This function makes the ammo of an item/character known.
      ChrList[ichr].ammoknown = btrue;
      break;

    case F_SpawnAttachedParticle:
      // This function spawns an attached particle
      returncode = bfalse;
      if ( VALID_CHR( ichr ) && !chr_in_pack( ichr ) )
      {
        tTmp = ichr;
        if ( chr_attached( ichr ) )  tTmp = chr_get_attachedto( ichr );
        tTmp = spawn_one_particle( 1.0f, ChrList[ichr].pos, ChrList[ichr].turn_lr, loc_model, scr_globals.tmpargument, ichr, scr_globals.tmpdistance, ChrList[ichr].team, tTmp, 0, MAXCHR );
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
        tTmp = spawn_one_particle( 1.0f, prt_pos, ChrList[ichr].turn_lr, loc_model, scr_globals.tmpargument, MAXCHR, 0, ChrList[ichr].team, tTmp, 0, MAXCHR );
        returncode = VALID_PRT( tTmp );
      };
      break;

    case F_AccelerateTarget:
      // This function changes the target's speeds
      ChrList[loc_aitarget].vel.x += scr_globals.tmpx;
      ChrList[loc_aitarget].vel.y += scr_globals.tmpy;
      break;

    case F_IfDistanceIsMoreThanTurn:
      // This function proceeds tmpdistance is greater than tmpturn
      returncode = ( scr_globals.tmpdistance > ( int ) scr_globals.tmpturn );
      break;

    case F_IfCrushed:
      // This function proceeds only if the character was crushed
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_CRUSHED );
      break;

    case F_MakeCrushValid:
      // This function makes doors able to close on this object
      ChrList[ichr].canbecrushed = btrue;
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
        ChrList[ichr].aitarget = holder;
      };
      break;

    case F_IfNotPutAway:
      // This function proceeds only if the character couln't be put in the pack
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_NOTPUTAWAY );
      break;

    case F_IfTakenOut:
      // This function proceeds only if the character was taken out of the pack
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_TAKENOUT );
      break;

    case F_IfAmmoOut:
      // This function proceeds only if the character has no ammo
      returncode = ( ChrList[ichr].ammo == 0 );
      break;

    case F_PlaySoundLooped:
      // This function plays a looped sound
    returncode = bfalse;
      if ( moduleActive && (0 <= scr_globals.tmpargument) && (ChrList[ichr].pos_old.z > PITNOSOUND) && (ChrList[ichr].loopingchannel == INVALID_SOUND))
      {
        //You could use this, but right now there's no way to stop the sound later, so it's better not to start it
        ChrList[ichr].loopingchannel = play_sound( 1.0f, ChrList[ichr].pos, CapList[loc_model].wavelist[scr_globals.tmpargument], -1, loc_model, scr_globals.tmpargument);
        ChrList[ichr].loopingvolume = 1.0f;
      }
      break;

    case F_StopSoundLoop:
      // This function stops playing a sound
      if( INVALID_CHANNEL != ChrList[ichr].loopingchannel )
      {
        stop_sound( ChrList[ichr].loopingchannel );
        ChrList[ichr].loopingchannel = INVALID_CHANNEL;
      };
      break;

    case F_HealSelf:
      // This function heals the ichr, without setting the alert or modifying
      // the amount
      if ( ChrList[ichr].alive )
      {
        iTmp = ChrList[ichr].life_fp8 + scr_globals.tmpargument;
        if ( iTmp > ChrList[ichr].lifemax_fp8 ) iTmp = ChrList[ichr].lifemax_fp8;
        if ( iTmp < 1 ) iTmp = 1;
        ChrList[ichr].life_fp8 = iTmp;
      }
      break;

    case F_Equip:
      // This function flags the character as being equipped
      ChrList[ichr].isequipped = btrue;
      break;

    case F_IfTargetHasItemIDEquipped:
      // This function proceeds if the target has a matching item equipped
      returncode = bfalse;
      sTmp  = chr_get_nextinpack( loc_aitarget );
      while ( VALID_CHR( sTmp ) )
      {
        if ( sTmp != ichr && ChrList[sTmp].isequipped && CAP_INHERIT_IDSZ( ChrList[sTmp].model, scr_globals.tmpargument ) )
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
      ChrList[ichr].aiowner = chr_get_aitarget( ichr );
      break;

    case F_SetTargetToOwner:
      // This function sets the target to the owner
      ChrList[ichr].aitarget = loc_aiowner;
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
      ChrList[ichr].reloadtime = scr_globals.tmpargument;
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
        ChrList[ichr].aitarget = sTmp;
        returncode = btrue;
      }
      break;

    case F_PoofTarget:
      // This function makes the target go away
      returncode = bfalse;
      if ( VALID_CHR( loc_aitarget ) && !ChrList[loc_aitarget].isplayer )
      {
        // tell target to go away
        ChrList[loc_aitarget].gopoof = btrue;
        ChrList[ichr].aitarget = ichr;
        returncode = btrue;
      }
      break;

    case F_ChildDoActionOverride:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid
      returncode = bfalse;
      if ( scr_globals.tmpargument < MAXACTION )
      {
        if ( MadList[ChrList[loc_aichild].model].actionvalid[scr_globals.tmpargument] )
        {
          ChrList[loc_aichild].action.now = scr_globals.tmpargument;
          ChrList[loc_aichild].action.ready = bfalse;

          ChrList[loc_aichild].anim.lip_fp8 = 0;
          ChrList[loc_aichild].anim.flip    = 0.0f;
          ChrList[loc_aichild].anim.next    = MadList[ChrList[loc_aichild].model].actionstart[scr_globals.tmpargument];
          ChrList[loc_aichild].anim.last    = ChrList[loc_aichild].anim.next;
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
      ChrList[ichr].maxaccel *= scr_globals.tmpargument / 100.0;
      break;

    case F_SetChildState:
      // This function sets the child's state
      ChrList[loc_aichild].aistate = scr_globals.tmpargument;
      break;

    case F_SpawnAttachedSizedParticle:
      // This function spawns an attached particle, then sets its size
      returncode = bfalse;
      if ( VALID_CHR( ichr ) && !chr_in_pack( ichr ) )
      {
        tTmp = ichr;
        if ( chr_attached( ichr ) )  tTmp = chr_get_attachedto( ichr );

        tTmp = spawn_one_particle( 1.0f, ChrList[ichr].pos, ChrList[ichr].turn_lr, loc_model, scr_globals.tmpargument, ichr, scr_globals.tmpdistance, ChrList[ichr].team, tTmp, 0, MAXCHR );
        if ( VALID_PRT( tTmp ) )
        {
          PrtList[tTmp].size_fp8 = scr_globals.tmpturn;
          returncode = btrue;
        }
      }
      break;

    case F_ChangeArmor:
      // This function sets the character's armor type and returns the old type
      // as tmpargument and the new type as tmpx
      scr_globals.tmpx = scr_globals.tmpargument;
      iTmp = ( ChrList[ichr].texture - MadList[loc_model].skinstart ) % MAXSKIN;
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
        dx = ChrList[loc_aitarget].pos.x - ChrList[ichr].pos.x;
        dy = ChrList[loc_aitarget].pos.y - ChrList[ichr].pos.y;
        d2 = dx * dx + dy * dy;

        if ( d2 > 0.0f )
        {
          // use non-inverse function to get direction vec from ChrList[].turn
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
        returncode = ( INVALID_SOUND != play_sound( MIN( 1.0f, volume / 255.0f ), ChrList[ichr].pos_old, CapList[loc_model].wavelist[scr_globals.tmpargument], 0, loc_model, scr_globals.tmpargument ) );
      }
      break;

    case F_SpawnAttachedFacedParticle:
      // This function spawns an attached particle with facing
      returncode = bfalse;
      if ( VALID_CHR( ichr ) && !chr_in_pack( ichr ) )
      {
        tTmp = ichr;
        if ( chr_attached( ichr ) )  tTmp = chr_get_attachedto( ichr );

        tTmp = spawn_one_particle( 1.0f, ChrList[ichr].pos, scr_globals.tmpturn, loc_model, scr_globals.tmpargument, ichr, scr_globals.tmpdistance, ChrList[ichr].team, tTmp, 0, MAXCHR );
        returncode = VALID_PRT( bfalse );
      }
      break;

    case F_IfStateIsOdd:
      returncode = HAS_SOME_BITS( ChrList[ichr].aistate, 1 );
      break;

    case F_SetTargetToDistantEnemy:
      // This function finds an enemy, within a certain distance to the ichr, and
      // proceeds only if there is one
      sTmp = chr_search_distant_target( ichr, scr_globals.tmpdistance, btrue, bfalse );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        ChrList[ichr].aitarget = sTmp;
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
        ChrList[ichr].pos_old = ChrList[ichr].pos;

        ChrList[ichr].pos.x = scr_globals.tmpx;
        ChrList[ichr].pos.y = scr_globals.tmpy;
        if ( 0 != __chrhitawall( ichr, NULL ) )
        {
          // No it didn't...
          ChrList[ichr].pos = ChrList[ichr].pos_old;
          returncode = bfalse;
        }
        else
        {
          ChrList[ichr].pos_old = ChrList[ichr].pos;
          returncode = btrue;
        }
      }
      break;

    case F_GiveStrengthToTarget:
      // Permanently boost the target's strength
      if ( ChrList[loc_aitarget].alive )
      {
        iTmp = scr_globals.tmpargument;
        getadd( 0, ChrList[loc_aitarget].strength_fp8, PERFECTSTAT, &iTmp );
        ChrList[loc_aitarget].strength_fp8 += iTmp;
      }
      break;

    case F_GiveWisdomToTarget:
      // Permanently boost the target's wisdom
      if ( ChrList[loc_aitarget].alive )
      {
        iTmp = scr_globals.tmpargument;
        getadd( 0, ChrList[loc_aitarget].wisdom_fp8, PERFECTSTAT, &iTmp );
        ChrList[loc_aitarget].wisdom_fp8 += iTmp;
      }
      break;

    case F_GiveIntelligenceToTarget:
      // Permanently boost the target's intelligence
      if ( ChrList[loc_aitarget].alive )
      {
        iTmp = scr_globals.tmpargument;
        getadd( 0, ChrList[loc_aitarget].intelligence_fp8, PERFECTSTAT, &iTmp );
        ChrList[loc_aitarget].intelligence_fp8 += iTmp;
      }
      break;

    case F_GiveDexterityToTarget:
      // Permanently boost the target's dexterity
      if ( ChrList[loc_aitarget].alive )
      {
        iTmp = scr_globals.tmpargument;
        getadd( 0, ChrList[loc_aitarget].dexterity_fp8, PERFECTSTAT, &iTmp );
        ChrList[loc_aitarget].dexterity_fp8 += iTmp;
      }
      break;

    case F_GiveLifeToTarget:
      // Permanently boost the target's life
      if ( ChrList[loc_aitarget].alive )
      {
        iTmp = scr_globals.tmpargument;
        getadd( LOWSTAT, ChrList[loc_aitarget].lifemax_fp8, PERFECTBIG, &iTmp );
        ChrList[loc_aitarget].lifemax_fp8 += iTmp;
        if ( iTmp < 0 )
        {
          getadd( 1, ChrList[loc_aitarget].life_fp8, PERFECTBIG, &iTmp );
        }
        ChrList[loc_aitarget].life_fp8 += iTmp;
      }
      break;

    case F_GiveManaToTarget:
      // Permanently boost the target's mana
      if ( ChrList[loc_aitarget].alive )
      {
        iTmp = scr_globals.tmpargument;
        getadd( 0, ChrList[loc_aitarget].manamax_fp8, PERFECTBIG, &iTmp );
        ChrList[loc_aitarget].manamax_fp8 += iTmp;
        if ( iTmp < 0 )
        {
          getadd( 0, ChrList[loc_aitarget].mana_fp8, PERFECTBIG, &iTmp );
        }
        ChrList[loc_aitarget].mana_fp8 += iTmp;
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
            BlipList[numblip].x = mesh_fraction_x( scr_globals.tmpx ) * MAPSIZE * mapscale;
            BlipList[numblip].y = mesh_fraction_y( scr_globals.tmpy ) * MAPSIZE * mapscale ;
            BlipList[numblip].c = scr_globals.tmpargument;
            numblip++;
          }
        }
      }
      break;

    case F_HealTarget:
      // Give some life to the target
      if ( ChrList[loc_aitarget].alive )
      {
        iTmp = scr_globals.tmpargument;
        getadd( 1, ChrList[loc_aitarget].life_fp8, ChrList[loc_aitarget].lifemax_fp8, &iTmp );
        ChrList[loc_aitarget].life_fp8 += iTmp;
        // Check all enchants to see if they are removed
        iTmp = ChrList[loc_aitarget].firstenchant;
        while ( iTmp != MAXENCHANT )
        {
          sTmp = EncList[iTmp].nextenchant;
          if ( MAKE_IDSZ( "HEAL" ) == EveList[EncList[iTmp].eve].removedbyidsz )
          {
            remove_enchant( iTmp );
          }
          iTmp = sTmp;
        }
      }
      break;

    case F_PumpTarget:
      // Give some mana to the target
      if ( ChrList[loc_aitarget].alive )
      {
        iTmp = scr_globals.tmpargument;
        getadd( 0, ChrList[loc_aitarget].mana_fp8, ChrList[loc_aitarget].manamax_fp8, &iTmp );
        ChrList[loc_aitarget].mana_fp8 += iTmp;
      }
      break;

    case F_CostAmmo:
      // Take away one ammo
      if ( ChrList[ichr].ammo > 0 )
      {
        ChrList[ichr].ammo--;
      }
      break;

    case F_MakeSimilarNamesKnown:
      // Make names of matching objects known
      for ( iTmp = 0; iTmp < MAXCHR; iTmp++ )
      {
        sTmp = btrue;
        for ( tTmp = 0; tTmp < IDSZ_COUNT; tTmp++ )
        {
          if ( CapList[loc_model].idsz[tTmp] != CapList[ChrList[iTmp].model].idsz[tTmp] )
          {
            sTmp = bfalse;
            break;
          }
        }

        if ( sTmp )
        {
          ChrList[iTmp].nameknown = btrue;
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

        tTmp = spawn_one_particle( 1.0f, ChrList[ichr].pos, ChrList[ichr].turn_lr, loc_model, scr_globals.tmpargument, tTmp, scr_globals.tmpdistance, ChrList[ichr].team, tTmp, 0, MAXCHR );
        returncode = VALID_PRT( tTmp );
      };
      break;

    case F_SetTargetReloadTime:
      // This function sets the target's reload time
      ChrList[loc_aitarget].reloadtime = scr_globals.tmpargument;
      break;

    case F_SetFogLevel:
      // This function raises and lowers the module's fog
      fTmp = ( scr_globals.tmpargument / 10.0 ) - GFog.top;
      GFog.top += fTmp;
      GFog.distance += fTmp;
      GFog.on = CData.fogallowed;
      if ( GFog.distance < 1.0 )  GFog.on = bfalse;
      break;

    case F_GetFogLevel:
      // This function gets the fog level
      scr_globals.tmpargument = GFog.top * 10;
      break;

    case F_SetFogTAD:
      // This function changes the fog color
      GFog.red = scr_globals.tmpturn;
      GFog.grn = scr_globals.tmpargument;
      GFog.blu = scr_globals.tmpdistance;
      break;

    case F_SetFogBottomLevel:
      // This function sets the module's bottom fog level...
      fTmp = ( scr_globals.tmpargument / 10.0 ) - GFog.bottom;
      GFog.bottom += fTmp;
      GFog.distance -= fTmp;
      GFog.on = CData.fogallowed;
      if ( GFog.distance < 1.0 )  GFog.on = bfalse;
      break;

    case F_GetFogBottomLevel:
      // This function gets the fog level
      scr_globals.tmpargument = GFog.bottom * 10;
      break;

    case F_CorrectActionForHand:
      // This function turns ZA into ZA, ZB, ZC, or ZD...
      // tmpargument must be set to one of the A actions beforehand...
      if ( chr_attached( ichr ) )
      {
        if ( ChrList[ichr].inwhichslot == SLOT_RIGHT )
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
          returncode = ChrList[sTmp].bmpdata.calc_is_mount;
        }
      }
      break;

    case F_SparkleIcon:
      // This function makes a blippie thing go around the icon
      if ( scr_globals.tmpargument < NUMBAR && scr_globals.tmpargument > -1 )
      {
        ChrList[ichr].sparkle = scr_globals.tmpargument;
      }
      break;

    case F_UnsparkleIcon:
      // This function stops the blippie thing
      ChrList[ichr].sparkle = NOSPARKLE;
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
      //ChrList[ichr].bmpdata.shadow = scr_globals.tmpargument * ChrList[ichr].fat;
      //ChrList[ichr].bmpdata_save.shadow = scr_globals.tmpargument;
      break;

    case F_SignalTarget:
      // This function GOrder.s one specific character...  The target
      // Be careful in using this, always checking IDSZ first
      ChrList[loc_aitarget].message = scr_globals.tmpargument;
      ChrList[loc_aitarget].messagedata = 0;
      ChrList[loc_aitarget].alert |= ALERT_SIGNALED;
      break;

    case F_SetTargetToWhoeverIsInPassage:
      // This function lets passage rectangles be used as event triggers
      sTmp = who_is_blocking_passage( scr_globals.tmpargument );
      returncode = bfalse;
      if ( VALID_CHR( sTmp ) )
      {
        ChrList[ichr].aitarget = sTmp;
        returncode = btrue;
      }
      break;

    case F_IfCharacterWasABook:
      // This function proceeds if the base model is the same as the current
      // model or if the base model is SPELLBOOK
      returncode = ( ChrList[ichr].basemodel == SPELLBOOK ||
                     ChrList[ichr].basemodel == loc_model );
      break;

    case F_SetEnchantBoostValues:
      // This function sets the boost values for the last enchantment
      iTmp = ChrList[ichr].undoenchant;
      if ( iTmp != MAXENCHANT )
      {
        EncList[iTmp].ownermana = scr_globals.tmpargument;
        EncList[iTmp].ownerlife = scr_globals.tmpdistance;
        EncList[iTmp].targetmana = scr_globals.tmpx;
        EncList[iTmp].targetlife = scr_globals.tmpy;
      }
      break;

    case F_SpawnCharacterXYZ:
      {
        vect3 chr_pos = {scr_globals.tmpx, scr_globals.tmpy, scr_globals.tmpdistance};

        // This function spawns a ichr, failing if x,y,z is invalid
        sTmp = spawn_one_character( chr_pos, loc_model, ChrList[ichr].team, 0, scr_globals.tmpturn, NULL, MAXCHR );
        returncode = bfalse;
        if ( VALID_CHR( sTmp ) )
        {
          if ( 0 != __chrhitawall( sTmp, NULL ) )
          {
            ChrList[sTmp].freeme = btrue;
          }
          else
          {
            ChrList[sTmp].iskursed = bfalse;
            ChrList[ichr].aichild = sTmp;
            ChrList[sTmp].passage = ChrList[ichr].passage;
            ChrList[sTmp].aiowner = loc_aiowner;
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

        sTmp = spawn_one_character( chr_pos, scr_globals.tmpargument, ChrList[ichr].team, 0, scr_globals.tmpturn, NULL, MAXCHR );
        returncode = bfalse;
        if ( VALID_CHR( sTmp ) )
        {
          if ( 0 != __chrhitawall( sTmp, NULL ) )
          {
            ChrList[sTmp].freeme = btrue;
          }
          else
          {
            ChrList[sTmp].iskursed = bfalse;
            ChrList[ichr].aichild = sTmp;
            ChrList[sTmp].passage = ChrList[ichr].passage;
            ChrList[sTmp].aiowner = loc_aiowner;
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
        ChrList[loc_aitarget].aimorphed = btrue;
      };
      break;

    case F_PlayFullSound:
      // This function plays a sound loud for everyone...  Victory music
      returncode = bfalse;
      if ( INVALID_SOUND != scr_globals.tmpargument )
      {
        returncode = ( INVALID_SOUND != play_sound( 1.0f, GCamera.trackpos, CapList[loc_model].wavelist[scr_globals.tmpargument], 0, loc_model, scr_globals.tmpargument) );
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

        tTmp = spawn_one_particle( 1.0f, prt_pos, ChrList[ichr].turn_lr, loc_model, scr_globals.tmpargument, MAXCHR, 0, ChrList[ichr].team, tTmp, 0, MAXCHR );
        if ( VALID_PRT( tTmp ) )
        {
          PrtList[tTmp].target = chr_get_aitarget( ichr );
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
        ChrList[sTmp].iskursed = bfalse;
      };

      sTmp  = chr_get_nextinpack( loc_aitarget );
      while ( VALID_CHR( sTmp ) )
      {
        ChrList[sTmp].iskursed = bfalse;
        sTmp  = chr_get_nextinpack( sTmp );
      }
      break;

    case F_IfTargetIsSneaking:
      // This function proceeds if the target is doing ACTION_DA or ACTION_WA
      returncode = ( ChrList[loc_aitarget].action.now == ACTION_DA || ChrList[loc_aitarget].action.now == ACTION_WA );
      break;

    case F_DropItems:
      // This function drops all of the character's items
      drop_all_items( ichr );
      break;

    case F_RespawnTarget:
      // This function respawns the target at its current location
      sTmp = chr_get_aitarget( ichr );
      ChrList[sTmp].pos_old = ChrList[sTmp].pos;
      respawn_character( sTmp );
      ChrList[sTmp].pos = ChrList[sTmp].pos_old;
      break;

    case F_TargetDoActionSetFrame:
      // This function starts a new action, if it is valid for the model and
      // sets the starting frame.  It will fail if the action is invalid
      returncode = bfalse;
      if ( scr_globals.tmpargument < MAXACTION )
      {
        if ( MadList[ChrList[loc_aitarget].model].actionvalid[scr_globals.tmpargument] )
        {
          ChrList[loc_aitarget].action.now = scr_globals.tmpargument;
          ChrList[loc_aitarget].action.ready = bfalse;

          ChrList[loc_aitarget].anim.lip_fp8 = 0;
          ChrList[loc_aitarget].anim.flip    = 0.0f;
          ChrList[loc_aitarget].anim.next    = MadList[ChrList[loc_aitarget].model].actionstart[scr_globals.tmpargument];
          ChrList[loc_aitarget].anim.last    = ChrList[loc_aitarget].anim.next;

          returncode = btrue;
        }
      }
      break;

    case F_IfTargetCanSeeInvisible:
      // This function proceeds if the target can see invisible
      returncode = ChrList[loc_aitarget].canseeinvisible;
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
        ChrList[ichr].aitarget = sTmp;
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
        ChrList[ichr].aitarget = sTmp;
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
        ChrList[ichr].aitarget = sTmp;
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
        ChrList[ichr].aitarget = sTmp;
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
        returncode = ( ChrList[sTmp].holdingwhich[SLOT_SADDLE] == ichr );
      }
      break;

    case F_NotAnItem:
      // This function makes the character a non-item character
      ChrList[ichr].isitem = bfalse;
      break;

    case F_SetChildAmmo:
      // This function sets the child's ammo
      ChrList[loc_aichild].ammo = scr_globals.tmpargument;
      break;

    case F_IfHitVulnerable:
      // This function proceeds if the character was hit by a weapon with the
      // correct vulnerability IDSZ...  [SILV] for Werewolves...
      returncode = HAS_SOME_BITS( ChrList[ichr].alert, ALERT_HITVULNERABLE );
      break;

    case F_IfTargetIsFlying:
      // This function proceeds if the character target is flying
      returncode = ( ChrList[loc_aitarget].flyheight > 0 );
      break;

    case F_IdentifyTarget:
      // This function reveals the target's name, ammo, and usage
      // Proceeds if the target was unknown
      returncode = bfalse;
      sTmp = chr_get_aitarget( ichr );
      if ( ChrList[sTmp].ammomax != 0 )  ChrList[sTmp].ammoknown = btrue;
      if ( ChrList[sTmp].name[0] != 'B' ||
           ChrList[sTmp].name[1] != 'l' ||
           ChrList[sTmp].name[2] != 'a' ||
           ChrList[sTmp].name[3] != 'h' ||
           ChrList[sTmp].name[4] != 0 )
      {
        returncode = !ChrList[sTmp].nameknown;
        ChrList[sTmp].nameknown = btrue;
      }
      CapList[ChrList[sTmp].model].usageknown = btrue;
      break;

    case F_BeatModule:
      // This function displays the Module Ended message
      beatmodule = btrue;
      break;

    case F_EndModule:
      // This function presses the Escape key
      keyb.state[SDLK_ESCAPE] = 1;
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
      scr_globals.tmpargument = ChrList[loc_aitarget].aistate;
      break;

    case F_ClearEndText:
      // This function empties the end-module text buffer
      endtext[0] = 0;
      endtextwrite = 0;
      break;

    case F_AddEndText:
      // This function appends a message to the end-module text buffer
      append_end_text( MadList[loc_model].msg_start + scr_globals.tmpargument, ichr );
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
      ChrList[ichr].canbecrushed = bfalse;
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
      ChrList[ichr].vel.z += scr_globals.tmpargument / 100.0;
      break;

    case F_FlashVariableHeight:
      // This function makes the character flash, feet one color, head another...
      flash_character_height( ichr, scr_globals.tmpturn, scr_globals.tmpx,
                              scr_globals.tmpdistance, scr_globals.tmpy );
      break;

    case F_SetDamageTime:
      // This function makes the character invincible for a little while
      ChrList[ichr].damagetime = scr_globals.tmpargument;
      break;

    case F_IfStateIs8:
      returncode = ( 8 == ChrList[ichr].aistate );
      break;

    case F_IfStateIs9:
      returncode = ( 9 == ChrList[ichr].aistate );
      break;

    case F_IfStateIs10:
      returncode = ( 10 == ChrList[ichr].aistate );
      break;

    case F_IfStateIs11:
      returncode = ( 11 == ChrList[ichr].aistate );
      break;

    case F_IfStateIs12:
      returncode = ( 12 == ChrList[ichr].aistate );
      break;

    case F_IfStateIs13:
      returncode = ( 13 == ChrList[ichr].aistate );
      break;

    case F_IfStateIs14:
      returncode = ( 14 == ChrList[ichr].aistate );
      break;

    case F_IfStateIs15:
      returncode = ( 15 == ChrList[ichr].aistate );
      break;

    case F_IfTargetIsAMount:
      returncode = ChrList[loc_aitarget].bmpdata.calc_is_mount;
      break;

    case F_IfTargetIsAPlatform:
      returncode = ChrList[loc_aitarget].bmpdata.calc_is_platform;
      break;

    case F_AddStat:
      if ( !ChrList[ichr].staton ) add_stat( ichr );
      break;

    case F_DisenchantTarget:
      returncode = ( ChrList[loc_aitarget].firstenchant != MAXENCHANT );
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
              if(ChrList[sTmp].on && ChrList[sTmp].alive && ChrList[sTmp].team == ChrList[ichr].team)
              {
          //And set their volume to tmpdistance
            if(scr_globals.tmpdistance >= 0 && ChrList[sTmp].loopingchannel != INVALID_SOUND)
          {
          Mix_Volume(ChrList[sTmp].loopingchannel, scr_globals.tmpdistance);
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
      tTmp = ChrList[sTmp].model;           // The target's model
      iTmp =  CapList[tTmp].skincost[scr_globals.tmpargument % MAXSKIN];
      scr_globals.tmpy = iTmp;                // Cost of new skin
      iTmp -= CapList[tTmp].skincost[( ChrList[sTmp].texture - MadList[tTmp].skinstart ) % MAXSKIN];   // Refund
      if ( iTmp > ChrList[sTmp].money )
      {
        // Not enough...
        scr_globals.tmpx = iTmp - ChrList[sTmp].money;  // Amount needed
        returncode = bfalse;
      }
      else
      {
        // Pay for it...  Cost may be negative after refund...
        ChrList[sTmp].money -= iTmp;
        if ( ChrList[sTmp].money > MAXMONEY )  ChrList[sTmp].money = MAXMONEY;
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
        ChrList[ichr].aitarget = sTmp;
        returncode = btrue;
      }
      break;

    case F_MakeNameUnknown:
      // This function makes the name of an item/character unknown.
      ChrList[ichr].nameknown = bfalse;
      break;

    case F_SpawnExactParticleEndSpawn:
      {
        vect3 prt_pos = {scr_globals.tmpx, scr_globals.tmpy, scr_globals.tmpdistance};

        // This function spawns a particle that spawns a character...
        returncode = bfalse;
        tTmp = ichr;
        if ( chr_attached( ichr ) )  tTmp = chr_get_attachedto( ichr );

        tTmp = spawn_one_particle( 1.0f, prt_pos, ChrList[ichr].turn_lr, loc_model, scr_globals.tmpargument, MAXCHR, 0, ChrList[ichr].team, tTmp, 0, MAXCHR );
        if ( VALID_PRT( tTmp ) )
        {
          PrtList[tTmp].spawncharacterstate = scr_globals.tmpturn;
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
        if ( CapList[sTmp].gopoofprttype <= PRTPIP_PEROBJECT_COUNT )
        {
          sTmp = MadList[sTmp].prtpip[CapList[sTmp].gopoofprttype];
        }
        else
        {
          sTmp = CapList[sTmp].gopoofprttype;
        }

        if ( MAXPRTPIP != sTmp )
        {
          // store the base values
          iTmp = PipList[sTmp].xyvel.ibase;
          tTmp = PipList[sTmp].xyspacing.ibase;
          test = PipList[sTmp].damage_fp8.ibase;

          // set some temporary values
          PipList[sTmp].xyvel.ibase = scr_globals.tmpx;
          PipList[sTmp].xyspacing.ibase = scr_globals.tmpy;
          PipList[sTmp].damage_fp8.ibase = scr_globals.tmpargument;

          // do the poof
          spawn_poof( ichr, loc_model );

          // Restore the saved values
          PipList[sTmp].xyvel.ibase = iTmp;
          PipList[sTmp].xyspacing.ibase = tTmp;
          PipList[sTmp].damage_fp8.ibase = test;
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
      ChrList[loc_aitarget].dazetime += scr_globals.tmpargument;
      break;

    case F_GrogTarget:
      //This function grogs the target for a duration equal to tmpargument
      ChrList[loc_aitarget].grogtime += scr_globals.tmpargument;
      break;

    case F_IfEquipped:
      //This proceeds if the character is equipped
      returncode = ChrList[ichr].isequipped;
      break;

    case F_DropTargetMoney:
      // This function drops some of the target's money
      drop_money( loc_aitarget, scr_globals.tmpargument );
      break;

    case F_GetTargetContent:
      //This sets tmpargument to the current target's content value
      scr_globals.tmpargument = ChrList[loc_aitarget].aicontent;
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
      if ( ChrList[loc_aitarget].isplayer )
      {
        snprintf( cTmp, sizeof( cTmp ), "%s.obj", ChrList[loc_aitarget].name );
        returncode = add_quest_idsz( cTmp, scr_globals.tmpargument );
      }
      break;

    case F_BeatQuest:
      //This function marks a IDSZ in the targets quest.txt as beaten
      returncode = bfalse;
      if ( ChrList[loc_aitarget].isplayer )
      {
        snprintf( cTmp, sizeof( cTmp ), "%s.obj", ChrList[loc_aitarget].name );
        if(modify_quest_idsz( cTmp, scr_globals.tmpargument, 0 ) == -2) returncode = btrue;
      }
      break;

    case F_IfTargetHasQuest:
      //This function proceeds if the target has the unfinished quest specified in tmpargument
      //and sets tmpx to the Quest Level of the specified quest.
      if ( ChrList[loc_aitarget].isplayer )
      {
        snprintf( cTmp, sizeof( cTmp ), "%s.obj", ChrList[loc_aitarget].name );
        iTmp = check_player_quest( cTmp, scr_globals.tmpargument );
        if ( iTmp > -1 )
        {
          returncode = btrue;
          scr_globals.tmpx = iTmp;
        }
        else returncode = bfalse;
      }
      break;

    case F_SetQuestLevel:
      //This function modifies the quest level for a specific quest IDSZ
      //tmpargument specifies quest idsz and tmpdistance the adjustment (which may be negative)	  
      returncode = bfalse;
      if ( ChrList[loc_aitarget].isplayer && scr_globals.tmpdistance != 0 )
      {
        snprintf( cTmp, sizeof( cTmp ), "%s.obj", ChrList[loc_aitarget].name );
        if(modify_quest_idsz( cTmp, scr_globals.tmpargument, scr_globals.tmpdistance ) != -1) returncode = btrue;
      }
      break;

    case F_IfTargetHasNotFullMana:
      //This function proceeds if the target has more than one point of mana and is alive
      returncode = bfalse;
      if ( VALID_CHR( loc_aitarget ) )
      {
        returncode = ChrList[loc_aitarget].alive && ChrList[loc_aitarget].manamax_fp8 > 0 && ChrList[loc_aitarget].mana_fp8 < ChrList[loc_aitarget].manamax_fp8 - DAMAGE_MIN;
      };
      break;

    case F_IfDoingAction:
      //This function proceeds if the character is preforming the animation specified in tmpargument
      returncode = ChrList[ichr].action.now >= scr_globals.tmpargument && ChrList[ichr].action.now <= scr_globals.tmpargument;
      break;

    case F_IfOperatorIsLinux:
      //This function proceeds if the computer is running a UNIX OS

    #ifdef __unix__
    returncode = btrue;    //Player running Linux
      #else
    returncode = bfalse;    //Player running something else.
    #endif
      break;

    case F_IfTargetIsOwner:
    //This function proceeds if the target is the characters owner
      returncode = loc_aitarget == loc_aiowner;
      break;

    case F_SetCameraSwing:
      //This function sets the camera swing rate
      GCamera.swing = 0;
      GCamera.swingrate = scr_globals.tmpargument;
      GCamera.swingamp = scr_globals.tmpdistance;
      break;

    case F_EnableRespawn:
      // This function turns respawn with JUMP button on
      respawnvalid = btrue;
      break;

    case F_DisableRespawn:
      // This function turns respawning with JUMP button off
      respawnvalid = bfalse;
      break;

    case F_IfButtonPressed:
      // This proceeds if the character is a player and pressing the latch specified in tmpargument
      returncode = HAS_SOME_BITS( ChrList[ichr].latch.b, scr_globals.tmpargument ) && ChrList[ichr].isplayer;
      break;



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
  CHR_REF loc_leader   = team_get_leader( ChrList[character].team );

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
        iTmp = ChrList[character].pos.x;
        break;

      case VAR_SELF_Y:
        iTmp = ChrList[character].pos.y;
        break;

      case VAR_SELF_TURN:
        iTmp = ChrList[character].turn_lr;
        break;

      case VAR_SELF_COUNTER:
        iTmp = ChrList[character].messagedata;
        break;

      case VAR_SELF_ORDER:
        iTmp = ChrList[character].message;
        break;

      case VAR_SELF_MORALE:
        iTmp = TeamList[ChrList[character].baseteam].morale;
        break;

      case VAR_SELF_LIFE:
        iTmp = ChrList[character].life_fp8;
        break;

      case VAR_TARGET_X:
        iTmp = ChrList[loc_aitarget].pos.x;
        break;

      case VAR_TARGET_Y:
        iTmp = ChrList[loc_aitarget].pos.y;
        break;

      case VAR_TARGET_DISTANCE:
        iTmp = sqrt(( ChrList[loc_aitarget].pos.x - ChrList[character].pos.x ) * ( ChrList[loc_aitarget].pos.x - ChrList[character].pos.x ) +
                    ( ChrList[loc_aitarget].pos.y - ChrList[character].pos.y ) * ( ChrList[loc_aitarget].pos.y - ChrList[character].pos.y ) +
                    ( ChrList[loc_aitarget].pos.z - ChrList[character].pos.z ) * ( ChrList[loc_aitarget].pos.z - ChrList[character].pos.z ) );
        break;

      case VAR_TARGET_TURN:
        iTmp = ChrList[loc_aitarget].turn_lr;
        break;

      case VAR_LEADER_X:
        iTmp = ChrList[character].pos.x;
        if ( VALID_CHR( loc_leader ) )
          iTmp = ChrList[loc_leader].pos.x;
        break;

      case VAR_LEADER_Y:
        iTmp = ChrList[character].pos.y;
        if ( VALID_CHR( loc_leader ) )
          iTmp = ChrList[loc_leader].pos.y;
        break;

      case VAR_LEADER_DISTANCE:
        iTmp = 10000;
        if ( VALID_CHR( loc_leader ) )
          iTmp = ABS(( int )( ChrList[loc_leader].pos.x - ChrList[character].pos.x ) ) +
                 ABS(( int )( ChrList[loc_leader].pos.y - ChrList[character].pos.y ) );
        break;

      case VAR_LEADER_TURN:
        iTmp = ChrList[character].turn_lr;
        if ( VALID_CHR( loc_leader ) )
          iTmp = ChrList[loc_leader].turn_lr;
        break;

      case VAR_GOTO_X:
        iTmp = ChrList[character].aigotox[ChrList[character].aigoto];
        break;

      case VAR_GOTO_Y:
        iTmp = ChrList[character].aigotoy[ChrList[character].aigoto];
        break;

      case VAR_GOTO_DISTANCE:
        iTmp = ABS(( int )( ChrList[character].aigotox[ChrList[character].aigoto] - ChrList[character].pos.x ) ) +
               ABS(( int )( ChrList[character].aigotoy[ChrList[character].aigoto] - ChrList[character].pos.y ) );
        break;

      case VAR_TARGET_TURNTO:
        iTmp = vec_to_turn( ChrList[loc_aitarget].pos.x - ChrList[character].pos.x, ChrList[loc_aitarget].pos.y - ChrList[character].pos.y );
        break;

      case VAR_PASSAGE:
        iTmp = ChrList[character].passage;
        break;

      case VAR_WEIGHT:
        iTmp = ChrList[character].holdingweight;
        break;

      case VAR_SELF_ALTITUDE:
        iTmp = ChrList[character].pos.z - ChrList[character].level;
        break;

      case VAR_SELF_ID:
        iTmp = CapList[ChrList[character].model].idsz[IDSZ_TYPE];
        break;

      case VAR_SELF_HATEID:
        iTmp = CapList[ChrList[character].model].idsz[IDSZ_HATE];
        break;

      case VAR_SELF_MANA:
        iTmp = ChrList[character].mana_fp8;
        if ( ChrList[character].canchannel )  iTmp += ChrList[character].life_fp8;
        break;

      case VAR_TARGET_STR:
        iTmp = ChrList[loc_aitarget].strength_fp8;
        break;

      case VAR_TARGET_WIS:
        iTmp = ChrList[loc_aitarget].wisdom_fp8;
        break;

      case VAR_TARGET_INT:
        iTmp = ChrList[loc_aitarget].intelligence_fp8;
        break;

      case VAR_TARGET_DEX:
        iTmp = ChrList[loc_aitarget].dexterity_fp8;
        break;

      case VAR_TARGET_LIFE:
        iTmp = ChrList[loc_aitarget].life_fp8;
        break;

      case VAR_TARGET_MANA:
        iTmp = ChrList[loc_aitarget].mana_fp8;
        if ( ChrList[loc_aitarget].canchannel )  iTmp += ChrList[loc_aitarget].life_fp8;
        break;

      case VAR_TARGET_LEVEL:
        iTmp = calc_chr_level( loc_aitarget );
        break;

      case VAR_TARGET_SPEEDX:
        iTmp = ChrList[loc_aitarget].vel.x;
        break;

      case VAR_TARGET_SPEEDY:
        iTmp = ChrList[loc_aitarget].vel.y;
        break;

      case VAR_TARGET_SPEEDZ:
        iTmp = ChrList[loc_aitarget].vel.z;
        break;

      case VAR_SELF_SPAWNX:
        iTmp = ChrList[character].stt.x;
        break;

      case VAR_SELF_SPAWNY:
        iTmp = ChrList[character].stt.y;
        break;

      case VAR_SELF_STATE:
        iTmp = ChrList[character].aistate;
        break;

      case VAR_SELF_STR:
        iTmp = ChrList[character].strength_fp8;
        break;

      case VAR_SELF_WIS:
        iTmp = ChrList[character].wisdom_fp8;
        break;

      case VAR_SELF_INT:
        iTmp = ChrList[character].intelligence_fp8;
        break;

      case VAR_SELF_DEX:
        iTmp = ChrList[character].dexterity_fp8;
        break;

      case VAR_SELF_MANAFLOW:
        iTmp = ChrList[character].manaflow_fp8;
        break;

      case VAR_TARGET_MANAFLOW:
        iTmp = ChrList[loc_aitarget].manaflow_fp8;
        break;

      case VAR_SELF_ATTACHED:
        iTmp = number_of_attached_particles( character );
        break;

      case VAR_SWINGTURN:
        iTmp = GCamera.swing << 2;
        break;

      case VAR_XYDISTANCE:
        iTmp = sqrt( scr_globals.tmpx * scr_globals.tmpx + scr_globals.tmpy * scr_globals.tmpy );
        break;

      case VAR_SELF_Z:
        iTmp = ChrList[character].pos.z;
        break;

      case VAR_TARGET_ALTITUDE:
        iTmp = ChrList[loc_aitarget].pos.z - ChrList[loc_aitarget].level;
        break;

      case VAR_TARGET_Z:
        iTmp = ChrList[loc_aitarget].pos.z;
        break;

      case VAR_SELF_INDEX:
        iTmp = character;
        break;

      case VAR_OWNER_X:
        iTmp = ChrList[loc_aiowner].pos.x;
        break;

      case VAR_OWNER_Y:
        iTmp = ChrList[loc_aiowner].pos.y;
        break;

      case VAR_OWNER_TURN:
        iTmp = ChrList[loc_aiowner].turn_lr;
        break;

      case VAR_OWNER_DISTANCE:
        iTmp = sqrt(( ChrList[loc_aiowner].pos.x - ChrList[character].pos.x ) * ( ChrList[loc_aiowner].pos.x - ChrList[character].pos.x ) +
                    ( ChrList[loc_aiowner].pos.y - ChrList[character].pos.y ) * ( ChrList[loc_aiowner].pos.y - ChrList[character].pos.y ) +
                    ( ChrList[loc_aiowner].pos.z - ChrList[character].pos.z ) * ( ChrList[loc_aiowner].pos.z - ChrList[character].pos.z ) );
        break;

      case VAR_OWNER_TURNTO:
        iTmp = vec_to_turn( ChrList[loc_aiowner].pos.x - ChrList[character].pos.x, ChrList[loc_aiowner].pos.y - ChrList[character].pos.y );
        break;

      case VAR_XYTURNTO:
        iTmp = vec_to_turn( scr_globals.tmpx - ChrList[character].pos.x, scr_globals.tmpy - ChrList[character].pos.y );
        break;

      case VAR_SELF_MONEY:
        iTmp = ChrList[character].money;
        break;

      case VAR_SELF_ACCEL:
        iTmp = ( ChrList[character].maxaccel * 100.0 );
        break;

      case VAR_TARGET_EXP:
        iTmp = ChrList[loc_aitarget].experience;
        break;

      case VAR_SELF_AMMO:
        iTmp = ChrList[character].ammo;
        break;

      case VAR_TARGET_AMMO:
        iTmp = ChrList[loc_aitarget].ammo;
        break;

      case VAR_TARGET_MONEY:
        iTmp = ChrList[loc_aitarget].money;
        break;

      case VAR_TARGET_TURNAWAY:
        iTmp = vec_to_turn( ChrList[character].pos.x - ChrList[loc_aitarget].pos.x, ChrList[character].pos.y - ChrList[loc_aitarget].pos.y );
        break;

      case VAR_SELF_LEVEL:
        iTmp = calc_chr_level( character );
        break;

      case VAR_SPAWN_DISTANCE:
        iTmp = sqrt(( ChrList[character].stt.x - ChrList[character].pos.x ) * ( ChrList[character].stt.x - ChrList[character].pos.x ) +
                    ( ChrList[character].stt.y - ChrList[character].pos.y ) * ( ChrList[character].stt.y - ChrList[character].pos.y ) +
                    ( ChrList[character].stt.z - ChrList[character].pos.z ) * ( ChrList[character].stt.z - ChrList[character].pos.z ) );
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
  aicode = ChrList[character].aitype;


  // Figure out alerts that weren't already set
  set_alerts( character, dUpdate );

  // Clear the button latches
  if ( !ChrList[character].isplayer )
  {
    ChrList[character].latch.b = 0;
  }


  // Reset the target if it can't be seen
  if ( !ChrList[character].canseeinvisible && ChrList[character].alive )
  {
    if ( chr_is_invisible( character ) )
    {
      ChrList[character].aitarget = character;
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
  if ( !ChrList[character].isplayer && aicode != 0 )
  {
    CHR_REF rider = chr_get_holdingwhich( character, SLOT_SADDLE );

    if ( ChrList[character].ismount && VALID_CHR( rider ) && !ChrList[rider].isitem )
    {
      // Mount
      ChrList[character].latch.x = ChrList[rider].latch.x;
      ChrList[character].latch.y = ChrList[rider].latch.y;
    }
    else
    {
      float fnum, fden;
      // Normal AI
      ChrList[character].latch.x = ChrList[character].aigotox[ChrList[character].aigoto] - ChrList[character].pos.x;
      ChrList[character].latch.y = ChrList[character].aigotoy[ChrList[character].aigoto] - ChrList[character].pos.y;

      fnum = ChrList[character].latch.x * ChrList[character].latch.x + ChrList[character].latch.y * ChrList[character].latch.y;
      fden = fnum + 25 * ChrList[character].bmpdata.calc_size * ChrList[character].bmpdata.calc_size;
      if ( fnum > 0.0f )
      {
        float ftmp = 1.0f / sqrt( fnum ) * fnum / fden;
        ChrList[character].latch.x *= ftmp;
        ChrList[character].latch.y *= ftmp;
      }
    }
  }


  // Clear alerts for next time around
  ChrList[character].alert = 0;
  if ( ChrList[character].aimorphed )
  {
    ChrList[character].alert |= ALERT_CHANGED;
    ChrList[character].aimorphed = bfalse;
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
    if ( HAS_SOME_BITS( ChrList[character].alert, ALERT_CRUSHED ) )
    {
      ChrList[character].alert = ALERT_CRUSHED;
      allow_thinking = btrue;
    }

    if ( HAS_SOME_BITS( ChrList[character].alert, ALERT_CLEANEDUP ) )
    {
      ChrList[character].alert = ALERT_CLEANEDUP;
      allow_thinking = btrue;
    };

    // Do not exclude items in packs. In NetHack, eggs can hatch while in your pack...
    // this may need to be handled differently. Currently dead things are thinking too much...
    if ( !chr_in_pack( character ) || ChrList[character].alive )
    {
      allow_thinking = btrue;
    }

    if ( allow_thinking )
    {
      let_character_think( character, dUpdate );
    }
  }


}



//------------------------------------------------------------------------------
// Don't use the aicodes file, just register them in the program.
// There will never be a reason to change them, and it will prevent transcription mistakes between the
// source code and the aicodes.txt file

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

