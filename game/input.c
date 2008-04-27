/* Egoboo - input.c
 * Keyboard, mouse, and joystick handling code.
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

#include "input.h"
#include "Ui.h"
#include "Log.h"
#include "Network.h"

#include "egoboo_utility.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

int    numpla = 0;                                 // Number of players

PLAYER PlaList[MAXPLAYER];

MOUSE mous =
{
  btrue,            // on
  2.0f,             // sense
  0.9f,             // sustain
  0.1f,             // cover
  {0.0f, 0.0f, 0},  // latch
  {0.0f, 0.0f, 0},  // latch_old
  {0.0f, 0.0f, 0},  // dlatch
  0,                // z
};

JOYSTICK joy[2] =
{
  // "joya"
  {
    NULL,            // sdl_device
    bfalse,          // on
    {0.0f, 0.0f, 0}  // latch
  },

  // "joyb"
  {
    NULL,            // sdl_device
    bfalse,          // on
    {0.0f, 0.0f, 0}  // latch
  }
};

KEYBOARD keyb =
{
  btrue,            //  on
  NULL,             //  state
  {0.0f, 0.0f, 0}   // latch
};

//--------------------------------------------------------------------------------------------

//Key/Control input defenitions
#define MAXTAG              128                     // Number of tags in scancode.txt
#define TAGSIZE             32                      // Size of each tag

typedef struct scancode_data_t
{
  char   name[TAGSIZE];             // Scancode names
  Uint32 value;                     // Scancode values
} SCANCODE_DATA;

typedef struct scantag_list_t
{
  int           count;
  SCANCODE_DATA data[MAXTAG];
} SCANTAG_LIST;

SCANTAG_LIST tags;

//--------------------------------------------------------------------------------------------
typedef struct control_data_t
{
  int    value;             // The scancode or mask
  bool_t is_key;            // Is it a key?
} CONTROL_DATA;

CONTROL_DATA control_list[INPUT_COUNT][CONTROL_COUNT];


//--------------------------------------------------------------------------------------------
// Tags
static void reset_tags();
static int read_tag( FILE *fileread );
static int tag_value( char *string );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void input_setup()
{
  if ( SDL_NumJoysticks() > 0 )
  {
    joy[0].sdl_device = SDL_JoystickOpen( 0 );
    joy[0].on = (NULL != joy[0].sdl_device);
  }

  if ( SDL_NumJoysticks() > 1 )
  {
    joy[1].sdl_device = SDL_JoystickOpen( 1 );
    joy[1].on = (NULL != joy[1].sdl_device);
  }

}

//--------------------------------------------------------------------------------------------
bool_t read_mouse(MOUSE * pm)
{
  int x,y,b;

  if(NULL == pm) return bfalse;

  if ( pm->on )
  {
    b = SDL_GetRelativeMouseState( &x, &y );

    pm->dlatch.x = x;
    pm->dlatch.y = y;

    pm->latch.x += pm->dlatch.x;
    pm->latch.x = CLIP(pm->latch.x, 0, CData.scrx);

    pm->latch.y += pm->dlatch.y;
    pm->latch.y = CLIP(pm->latch.y, 0, CData.scry);

    pm->button[0] = ( b & SDL_BUTTON( 1 ) ) ? 1 : 0;
    pm->button[1] = ( b & SDL_BUTTON( 3 ) ) ? 1 : 0;
    pm->button[2] = ( b & SDL_BUTTON( 2 ) ) ? 1 : 0;  // Middle is 2 on SDL
    pm->button[3] = ( b & SDL_BUTTON( 4 ) ) ? 1 : 0;
  }
  else
  {
    SDL_GetMouseState( &x, &y );

    pm->latch.x = x;
    pm->latch.y = y;

    pm->dlatch.x = pm->dlatch.y = 0;

    pm->button[0] = 0;
    pm->button[1] = 0;
    pm->button[2] = 0;
    pm->button[3] = 0;
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t read_key(KEYBOARD * pk)
{
  if(NULL == pk) return bfalse;

  pk->state = SDL_GetKeyState( NULL );

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t read_joystick(JOYSTICK * pj)
{
  int button;
  float jx, jy;
  const float jthresh = .5;

  if(NULL == pj) return bfalse;

  if (!pj->on || NULL == pj->sdl_device )
  {
    pj->latch.x = pj->latch.y = 0.0f;
    pj->latch.b = 0;

    joy[1].latch.x = joy[1].latch.y = 0.0f;
    joy[1].latch.b = 0;

    return (NULL != pj->sdl_device);
  };

  jx = ((Sint16)(SDL_JoystickGetAxis( pj->sdl_device, 0 ) >> 8)) / ( float )( 0xFF );
  jy = ((Sint16)(SDL_JoystickGetAxis( pj->sdl_device, 1 ) >> 8)) / ( float )( 0xFF );

  pj->latch.x = 0.0f;
  pj->latch.y = 0.0f;
  if ( ABS( jx ) + ABS( jy ) > 0.0f )
  {
    pj->latch.x = jx;
    pj->latch.y = jy;

    jx = sqrt(( jx * jx + jy * jy ) * 0.5f );
    jy = ( jx - jthresh * jx / ( jthresh + fabs( jx ) ) ) * ( jthresh + 1.0f );

    pj->latch.x *= jy / jx / sqrt( 2.0f );
    pj->latch.y *= jy / jx / sqrt( 2.0f );
  }

  button = SDL_JoystickNumButtons( pj->sdl_device );
  while ( button >= 0 )
  {
    pj->button[button] = SDL_JoystickGetButton( pj->sdl_device, button );
    button--;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t reset_press(KEYBOARD * pk)
{
  // ZZ> This function resets key press information

  return (NULL != pk);

  /*PORT
      int cnt;
      cnt = 0;
      while(cnt < 256)
      {
          keypress[cnt] = bfalse;
          cnt++;
      }
  */
}


//--------------------------------------------------------------------------------------------
//Tag Reading---------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void reset_tags()
{
  //This function resets the tags
  tags.count = 0;
}

//--------------------------------------------------------------------------------------------
int read_tag( FILE *fileread )
{
  // ZZ> This function finds the next tag, returning btrue if it found one
  if ( tags.count < MAXTAG )
  {
    if ( fget_next_string( fileread, tags.data[tags.count].name, sizeof( tags.data[tags.count].name ) ) )
    {
      tags.data[tags.count].value = fget_int( fileread );
      tags.count++;
      return btrue;
    }
  }
  return bfalse;
}

//--------------------------------------------------------------------------------------------
void read_all_tags( char *szFilename )
{
  // ZZ> This function reads the scancode.txt file
  FILE* fileread;

  reset_tags();
  fileread = fs_fileOpen( PRI_WARN, NULL, szFilename, "r" );
  if ( NULL == fileread )
  {
    log_warning("Could not read input codes (%s)\n", szFilename);
    return;
  };

  while ( read_tag( fileread ) );
  fs_fileClose( fileread );
}

//--------------------------------------------------------------------------------------------
int tag_value( char *string )
{
  // ZZ> This function matches the string with its tag, and returns the value...
  //     It will return 255 if there are no matches.
  int cnt;

  for (cnt = 0; cnt < tags.count; cnt++ )
  {
    if (0 == strcmp( string, tags.data[cnt].name ))
    {
      // They match
      return tags.data[cnt].value;
    }
  }

  // No matches
  return -1;
}

//--------------------------------------------------------------------------------------------
void read_controls( char *szFilename )
{
  // ZZ> This function reads the controls.txt file
  FILE* fileread;
  char currenttag[TAGSIZE];
  int tnc;
  INPUT_TYPE   input;
  CONTROL_LIST control;

  log_info( "read_controls() - reading control settings from the %s file.\n", szFilename );

  fileread = fs_fileOpen( PRI_WARN, NULL, szFilename, "r" );
  if ( NULL != fileread )
  {
    input = INPUT_KEYB;
    for ( control = KEY_FIRST, tnc = 0; control < KEY_LAST; control = ( CONTROL_LIST )( control + 1 ), tnc++ )
    {
      fget_next_string( fileread, currenttag, sizeof( currenttag ) );
      control_list[input][tnc].value = tag_value( currenttag );
      control_list[input][tnc].is_key = ( currenttag[0] == 'K' );
    }

    input = INPUT_MOUS;
    for ( control = MOS_FIRST, tnc = 0; control < MOS_LAST; control = ( CONTROL_LIST )( control + 1 ), tnc++ )
    {
      fget_next_string( fileread, currenttag, sizeof( currenttag ) );
      control_list[input][tnc].value = tag_value( currenttag );
      control_list[input][tnc].is_key = ( currenttag[0] == 'K' );
    }

    input = INPUT_JOYA;
    for ( control = JOA_FIRST, tnc = 0; control < JOA_LAST; control = ( CONTROL_LIST )( control + 1 ), tnc++ )
    {
      fget_next_string( fileread, currenttag, sizeof( currenttag ) );
      control_list[input][tnc].value = tag_value( currenttag );
      control_list[input][tnc].is_key = ( currenttag[0] == 'K' );
    }

    input = INPUT_JOYB;
    for ( control = JOB_FIRST, tnc = 0; control < JOB_LAST; control = ( CONTROL_LIST )( control + 1 ), tnc++ )
    {
      fget_next_string( fileread, currenttag, sizeof( currenttag ) );
      control_list[input][tnc].value = tag_value( currenttag );
      control_list[input][tnc].is_key = ( currenttag[0] == 'K' );
    }

    fs_fileClose( fileread );
  }
  else log_warning( "Could not load input settings (%s)\n", szFilename );
}

//--------------------------------------------------------------------------------------------
bool_t key_is_pressed( int keycode )
{
  // ZZ> This function returns btrue if the given control is pressed...
  if ( GNet.messagemode )  return bfalse;

  if ( keyb.state )
    return SDLKEYDOWN( keycode );
  else
    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t control_key_is_pressed( CONTROL control )
{
  // ZZ> This function returns btrue if the given control is pressed...

  if ( control_list[INPUT_KEYB][control].is_key )
    return key_is_pressed( control_list[INPUT_KEYB][control].value );
  else
    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t control_mouse_is_pressed( CONTROL control )
{
  // ZZ> This function returns btrue if the given control is pressed...
  bool_t retval = bfalse;

  if ( control_list[INPUT_MOUS][control].is_key )
  {
    retval = key_is_pressed( control_list[INPUT_MOUS][control].value );
  }
  else
  {
    retval = ( mous.latch.b == control_list[INPUT_MOUS][control].value );
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t control_joy_is_pressed( int joy_num, CONTROL control )
{
  // ZZ> This function returns btrue if the given control is pressed...
  INPUT_TYPE it;
  bool_t retval = bfalse;

  it = (joy_num == 0) ? INPUT_JOYA : INPUT_JOYB;

  if ( control_list[it][control].is_key )
  {
    retval = key_is_pressed( control_list[it][control].value );
  }
  else
  {
    retval = ( joy[joy_num].latch.b == control_list[it][control].value );
  }

  return retval;
}