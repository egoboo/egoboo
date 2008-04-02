/* Egoboo - graphic.c
* All sorts of stuff related to drawing the game, and all sorts of other stuff
* (such as data loading) that really should not be in here.
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

#include "graphic.h"
#include "egoboo.h"
#include "egoboo_math.h"
#include "Log.h"
#include "Ui.h"
#include "mesh.h"
#include "Menu.h"

#include <assert.h>
#include <stdarg.h>

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

#ifdef __unix__
#include <unistd.h>
#endif

static int draw_wrap_string( GLtexture * pfnt, float x, float y, GLfloat tint[], float maxx, char * szFormat, ... );
static int draw_status( GLtexture * pfnt, CHR_REF character, int x, int y );
static void draw_text( GLtexture * pfnt );


// Defined in egoboo.h
SDL_Surface *displaySurface = NULL;
bool_t gTextureOn = bfalse;

void render_particles();

void EnableTexturing()
{
  //if ( !gTextureOn )
  //{
  //  glEnable( GL_TEXTURE_2D );
  //  gTextureOn = btrue;
  //}
}

void DisableTexturing()
{
  //if ( gTextureOn )
  //{
  //  glDisable( GL_TEXTURE_2D );
  //  gTextureOn = bfalse;
  //}
}



// This needs work
static GLint threeDmode_begin_level = 0;
void Begin3DMode()
{
  assert( 0 == threeDmode_begin_level );

  ATTRIB_GUARD_OPEN( threeDmode_begin_level );
  ATTRIB_PUSH( "Begin3DMode", GL_TRANSFORM_BIT | GL_CURRENT_BIT );

  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadMatrixf( mProjection.v );

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadMatrixf( mView.v );

  glColor4f( 1, 1, 1, 1 );
}

void End3DMode()
{
  GLint threeDmode_end_level;

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  ATTRIB_POP( "End3DMode" );

  ATTRIB_GUARD_CLOSE( threeDmode_begin_level, threeDmode_end_level );
  threeDmode_begin_level = 0;
}

/********************> Begin2DMode() <*****/
static GLint twoD_begin_level = 0;

void Begin2DMode( void )
{
  assert( 0 == twoD_begin_level );

  ATTRIB_GUARD_OPEN( twoD_begin_level );
  ATTRIB_PUSH( "Begin2DMode", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_TRANSFORM_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT );

  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();         // Reset The Projection Matrix
  glOrtho( 0, CData.scrx, 0, CData.scry, 1, -1 );   // Set up an orthogonal projection

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();

  glDisable( GL_DEPTH_TEST );

  glColor4f( 1, 1, 1, 1 );
};

/********************> End2DMode() <*****/
void End2DMode( void )
{
  GLint twoD_end_level;

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  ATTRIB_POP( "End2DMode" );

  ATTRIB_GUARD_CLOSE( twoD_begin_level, twoD_end_level );
  twoD_begin_level = 0;
}

//--------------------------------------------------------------------------------------------
static GLint text_begin_level = 0;
void BeginText( GLtexture * pfnt )
{
  assert( 0 == text_begin_level );

  ATTRIB_GUARD_OPEN( text_begin_level );
  ATTRIB_PUSH( "BeginText", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT );

  GLTexture_Bind( pfnt, CData.texturefilter );

  glEnable( GL_ALPHA_TEST );
  glAlphaFunc( GL_GREATER, 0 );

  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

  glDisable( GL_DEPTH_TEST );
  glDisable( GL_CULL_FACE );

  glColor4f( 1, 1, 1, 1 );
}

//--------------------------------------------------------------------------------------------
void EndText()
{
  GLint text_end_level;

  ATTRIB_POP( "EndText" );

  ATTRIB_GUARD_CLOSE( text_begin_level, text_end_level );
  text_begin_level = 0;
}



//---------------------------------------------------------------------------------------------
void release_all_textures()
{
  // ZZ> This function clears out all of the textures
  int cnt;

  for ( cnt = 0; cnt < MAXTEXTURE; cnt++ )
    GLTexture_Release( &TxTexture[cnt] );
}

//--------------------------------------------------------------------------------------------
void load_one_icon( char *szLoadName )
{
  // ZZ> This function is used to load an icon.  Most icons are loaded
  //     without this function though...

  if ( INVALID_TEXTURE != GLTexture_Load( GL_TEXTURE_2D,  &TxIcon[globalnumicon], szLoadName, INVALID_KEY ) )
  {
    /* PORT
    DDSetColorKey(lpDDSIcon[globalnumicon], 0); */
  }
  globalnumicon++;

}

//---------------------------------------------------------------------------------------------
void prime_titleimage()
{
  // ZZ> This function sets the title image pointers to NULL
  int cnt;

  for ( cnt = 0; cnt < MAXMODULE; cnt++ )
    TxTitleImage[cnt].textureID = INVALID_TEXTURE;

  //titlerect.x = 0;
  //titlerect.w = TITLESIZE;
  //titlerect.y = 0;
  //titlerect.h = TITLESIZE;
}

//---------------------------------------------------------------------------------------------
void prime_icons()
{
  // ZZ> This function sets the icon pointers to NULL
  int cnt;

  for ( cnt = 0; cnt < MAXTEXTURE + 1; cnt++ )
  {
    //lpDDSIcon[cnt]=NULL;
    TxIcon[cnt].textureID = INVALID_TEXTURE;
    madskintoicon[cnt] = 0;
  }
  iconrect.left = 0;
  iconrect.right = 32;
  iconrect.top = 0;
  iconrect.bottom = 32;
  globalnumicon = 0;


  nullicon = 0;
  keybicon = 0;
  mousicon = 0;
  joyaicon = 0;
  joybicon = 0;
  bookicon = 0;


}

//---------------------------------------------------------------------------------------------
void release_all_icons()
{
  // ZZ> This function clears out all of the icons
  int cnt;

  for ( cnt = 0; cnt < MAXTEXTURE + 1; cnt++ )
    GLTexture_Release( &TxIcon[cnt] );

  prime_icons(); /* Do we need this? */
}

//---------------------------------------------------------------------------------------------
void release_all_titleimages()
{
  // ZZ> This function clears out all of the title images
  int cnt;

  for ( cnt = 0; cnt < MAXMODULE; cnt++ )
    GLTexture_Release( &TxTitleImage[cnt] );
}

//---------------------------------------------------------------------------------------------
void init_all_models()
{
  // ZZ> This function initializes the models

  int cnt;
  for ( cnt = 0; cnt < MAXMODEL; cnt++ )
  {
    capclassname[cnt][0] = '\0';
    madused[cnt] = bfalse;
    strcpy(madname[cnt], "*NONE*");
    mad_md2[cnt] = NULL;
    madframelip[cnt] = NULL;
    madframefx[cnt]  = NULL;
  }

  madloadframe = 0;
}

//---------------------------------------------------------------------------------------------
void release_all_models()
{
  // ZZ> This function clears out all of the models
  int cnt;
  for ( cnt = 0; cnt < MAXMODEL; cnt++ )
  {
    capclassname[cnt][0] = 0;
    madused[cnt] = bfalse;
    strcpy(madname[cnt], "*NONE*");

    free_one_md2( cnt );
  }
  madloadframe = 0;
}

//--------------------------------------------------------------------------------------------
void release_map()
{
  // ZZ> This function releases all the map images

  GLTexture_Release( &TxMap );
}

//--------------------------------------------------------------------------------------------
static void write_debug_message( int time, const char *format, va_list args )
{
  // ZZ> This function sticks a message in the display queue and sets its timer

  STRING buffer;
  int slot = get_free_message();

  // print the formatted messafe into the buffer
  vsnprintf( buffer, sizeof( buffer ) - 1, format, args );

  // Copy the message
  strncpy( msgtextdisplay[slot], buffer, sizeof( msgtextdisplay[slot] ) );
  msgtime[slot] = time * DELAY_MESSAGE;
}

//--------------------------------------------------------------------------------------------
void debug_message( int time, const char *format, ... )
{
  va_list args;

  va_start( args, format );
  write_debug_message( time, format, args );
  va_end( args );
};


//--------------------------------------------------------------------------------------------
void reset_end_text()
{
  // ZZ> This function resets the end-module text
  if ( numpla > 1 )
  {
    snprintf( endtext, sizeof( endtext ), "Sadly, they were never heard from again..." );
    endtextwrite = 42;  // Where to append further text
  }
  else
  {
    if ( numpla == 0 )
    {
      // No players???  RTS module???
      snprintf( endtext, sizeof( endtext ), "The game has ended..." );
      endtextwrite = 21;
    }
    else
    {
      // One player
      snprintf( endtext, sizeof( endtext ), "Sadly, no trace was ever found..." );
      endtextwrite = 33;  // Where to append further text
    }
  }
}

//--------------------------------------------------------------------------------------------
void append_end_text( int message, CHR_REF character )
{
  // ZZ> This function appends a message to the end-module text
  int read, cnt;
  char *eread;
  STRING szTmp;
  char cTmp, lTmp;
  Uint16 target, owner;

  target = chr_get_aitarget( character );
  owner = chr_get_aiowner( character );
  if ( message < msgtotal )
  {
    // Copy the message
    read = msgindex[message];
    cnt = 0;
    cTmp = msgtext[read];  read++;
    while ( cTmp != 0 )
    {
      if ( cTmp == '%' )
      {
        // Escape sequence
        eread = szTmp;
        szTmp[0] = 0;
        cTmp = msgtext[read];  read++;
        if ( cTmp == 'n' ) // Name
        {
          if ( chrnameknown[character] )
            strncpy( szTmp, chrname[character], sizeof( STRING ) );
          else
          {
            lTmp = capclassname[chrmodel[character]][0];
            if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
              snprintf( szTmp, sizeof( szTmp ), "an %s", capclassname[chrmodel[character]] );
            else
              snprintf( szTmp, sizeof( szTmp ), "a %s", capclassname[chrmodel[character]] );
          }
          if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
        }
        if ( cTmp == 'c' ) // Class name
        {
          eread = capclassname[chrmodel[character]];
        }
        if ( cTmp == 't' ) // Target name
        {
          if ( chrnameknown[target] )
            strncpy( szTmp, chrname[target], sizeof( STRING ) );
          else
          {
            lTmp = capclassname[chrmodel[target]][0];
            if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
              snprintf( szTmp, sizeof( szTmp ), "an %s", capclassname[chrmodel[target]] );
            else
              snprintf( szTmp, sizeof( szTmp ), "a %s", capclassname[chrmodel[target]] );
          }
          if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
        }
        if ( cTmp == 'o' ) // Owner name
        {
          if ( chrnameknown[owner] )
            strncpy( szTmp, chrname[owner], sizeof( STRING ) );
          else
          {
            lTmp = capclassname[chrmodel[owner]][0];
            if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
              snprintf( szTmp, sizeof( szTmp ), "an %s", capclassname[chrmodel[owner]] );
            else
              snprintf( szTmp, sizeof( szTmp ), "a %s", capclassname[chrmodel[owner]] );
          }
          if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
        }
        if ( cTmp == 's' ) // Target class name
        {
          eread = capclassname[chrmodel[target]];
        }
        if ( cTmp >= '0' && cTmp <= '0' + ( MAXSKIN - 1 ) )  // Target's skin name
        {
          eread = capskinname[chrmodel[target]][cTmp-'0'];
        }
        if ( cTmp == 'd' ) // tmpdistance value
        {
          snprintf( szTmp, sizeof( szTmp ), "%d", scr_globals.tmpdistance );
        }
        if ( cTmp == 'x' ) // tmpx value
        {
          snprintf( szTmp, sizeof( szTmp ), "%d", scr_globals.tmpx );
        }
        if ( cTmp == 'y' ) // tmpy value
        {
          snprintf( szTmp, sizeof( szTmp ), "%d", scr_globals.tmpy );
        }
        if ( cTmp == 'D' ) // tmpdistance value
        {
          snprintf( szTmp, sizeof( szTmp ), "%2d", scr_globals.tmpdistance );
        }
        if ( cTmp == 'X' ) // tmpx value
        {
          snprintf( szTmp, sizeof( szTmp ), "%2d", scr_globals.tmpx );
        }
        if ( cTmp == 'Y' ) // tmpy value
        {
          snprintf( szTmp, sizeof( szTmp ), "%2d", scr_globals.tmpy );
        }
        if ( cTmp == 'a' ) // Character's ammo
        {
          if ( chrammoknown[character] )
            snprintf( szTmp, sizeof( szTmp ), "%d", chrammo[character] );
          else
            snprintf( szTmp, sizeof( szTmp ), "?" );
        }
        if ( cTmp == 'k' ) // Kurse state
        {
          if ( chriskursed[character] )
            snprintf( szTmp, sizeof( szTmp ), "kursed" );
          else
            snprintf( szTmp, sizeof( szTmp ), "unkursed" );
        }
        if ( cTmp == 'p' ) // Character's possessive
        {
          if ( chrgender[character] == GEN_FEMALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "her" );
          }
          else
          {
            if ( chrgender[character] == GEN_MALE )
            {
              snprintf( szTmp, sizeof( szTmp ), "his" );
            }
            else
            {
              snprintf( szTmp, sizeof( szTmp ), "its" );
            }
          }
        }
        if ( cTmp == 'm' ) // Character's gender
        {
          if ( chrgender[character] == GEN_FEMALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "female " );
          }
          else
          {
            if ( chrgender[character] == GEN_MALE )
            {
              snprintf( szTmp, sizeof( szTmp ), "male " );
            }
            else
            {
              snprintf( szTmp, sizeof( szTmp ), " " );
            }
          }
        }
        if ( cTmp == 'g' ) // Target's possessive
        {
          if ( chrgender[target] == GEN_FEMALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "her" );
          }
          else
          {
            if ( chrgender[target] == GEN_MALE )
            {
              snprintf( szTmp, sizeof( szTmp ), "his" );
            }
            else
            {
              snprintf( szTmp, sizeof( szTmp ), "its" );
            }
          }
        }
        // Copy the generated text
        cTmp = *eread;  eread++;
        while ( cTmp != 0 && endtextwrite < MAXENDTEXT - 1 )
        {
          endtext[endtextwrite] = cTmp;
          cTmp = *eread;  eread++;
          endtextwrite++;
        }
      }
      else
      {
        // Copy the letter
        if ( endtextwrite < MAXENDTEXT - 1 )
        {
          endtext[endtextwrite] = cTmp;
          endtextwrite++;
        }
      }
      cTmp = msgtext[read];  read++;
      cnt++;
    }
  }
  endtext[endtextwrite] = 0;
}

//--------------------------------------------------------------------------------------------
void make_textureoffset( void )
{
  // ZZ> This function sets up for moving textures
  int cnt;
  for ( cnt = 0; cnt < 256; cnt++ )
    textureoffset[cnt] = FP8_TO_FLOAT( cnt );
}

//--------------------------------------------------------------------------------------------
void figure_out_what_to_draw()
{
  // ZZ> This function determines the things that need to be drawn

  // Find the render area corners
  //project_view();

  // Make the render list for the mesh
  make_renderlist();

  camturn_lr_one   = camturn_lr / (float)(1<<16);

  // Request matrices needed for local machine
  make_dolist();
  order_dolist();
}

//--------------------------------------------------------------------------------------------
void animate_tiles( float dUpdate )
{
  // This function changes the animated tile frame

  animtileframefloat += dUpdate / ( float ) animtileupdateand;
  while ( animtileframefloat >= 1.0f )
  {
    animtileframefloat -= 1.0f;
    animtileframeadd = ( animtileframeadd + 1 ) & animtileframeand;
  };
}

//--------------------------------------------------------------------------------------------
void load_basic_textures( char *modname )
{
  // ZZ> This function loads the standard textures for a module
  // BB> In each case, try to load one stored with the module first.

  // Particle sprites
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, "particle.bmp" );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[0], CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.particle_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[0], CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "!!!!Particle bitmap could not be found!!!! Missing File = \"%s\"\n", CStringTmp1 );
    }
  };

  // Module background tiles
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.tile0_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[1], CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.tile0_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[1], CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 0 could not be found. Missing File = \"%s\"\n", CData.tile0_bitmap );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.tile1_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,   &TxTexture[2], CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.tile1_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[2], CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 1 could not be found. Missing File = \"%s\"\n", CData.tile1_bitmap );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.tile2_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[3], CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.tile2_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[3], CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 2 could not be found. Missing File = \"%s\"\n", CData.tile2_bitmap );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.tile3_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[4], CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.tile3_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[4], CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 3 could not be found. Missing File = \"%s\"\n", CData.tile3_bitmap );
    }
  };


  // Water textures
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.watertop_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[5], CStringTmp1, INVALID_KEY ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.watertop_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[5], CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Water Layer 1 could not be found. Missing File = \"%s\"\n", CData.watertop_bitmap );
    }
  };

  // This is also used as far background
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.waterlow_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[6], CStringTmp1, INVALID_KEY ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.waterlow_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[6], CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Water Layer 0 could not be found. Missing File = \"%s\"\n", CData.waterlow_bitmap );
    }
  };


  // BB > this is handled differently now and is not needed
  // Texture 7 is the phong map
  //snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.phong_bitmap);
  //if(INVALID_TEXTURE==GLTexture_Load(GL_TEXTURE_2D,  &TxTexture[7], CStringTmp1, INVALID_KEY))
  //{
  //  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.phong_bitmap);
  //  GLTexture_Load(GL_TEXTURE_2D,  &TxTexture[7], CStringTmp1, TRANSCOLOR );
  //  {
  //    log_warning("Phong Bitmap Layer 1 could not be found. Missing File = \"%s\"", CData.phong_bitmap);
  //  }
  //};


}

//--------------------------------------------------------------------------------------------
ACTION action_number(char * szName)
{
  // ZZ> This function returns the number of the action in cFrameName, or
  //     it returns ACTION_INVALID if it could not find a match
  ACTION cnt;

  if(NULL==szName || '\0' == szName[0] || '\0' == szName[1]) return ACTION_INVALID;

  for ( cnt = 0; cnt < MAXACTION; cnt++ )
  {
    if ( 0 == strncmp(szName, cActionName[cnt], 2) ) return cnt;
  }

  return ACTION_INVALID;
}

//--------------------------------------------------------------------------------------------
Uint16 action_frame()
{
  // ZZ> This function returns the frame number in the third and fourth characters
  //     of cFrameName

  int number;
  sscanf( &cFrameName[2], "%d", &number );
  return number;
}

//--------------------------------------------------------------------------------------------
bool_t test_frame_name( char letter )
{
  // ZZ> This function returns btrue if the 4th, 5th, 6th, or 7th letters
  //     of the frame name matches the input argument
  if ( cFrameName[4] == letter ) return btrue;
  if ( cFrameName[4] == 0 ) return bfalse;
  if ( cFrameName[5] == letter ) return btrue;
  if ( cFrameName[5] == 0 ) return bfalse;
  if ( cFrameName[6] == letter ) return btrue;
  if ( cFrameName[6] == 0 ) return bfalse;
  if ( cFrameName[7] == letter ) return btrue;
  return bfalse;
}

//--------------------------------------------------------------------------------------------
void action_copy_correct( Uint16 object, ACTION actiona, ACTION actionb )
{
  // ZZ> This function makes sure both actions are valid if either of them
  //     are valid.  It will copy start and ends to mirror the valid action.
  if ( madactionvalid[object][actiona] == madactionvalid[object][actionb] )
  {
    // They are either both valid or both invalid, in either case we can't help
  }
  else
  {
    // Fix the invalid one
    if ( !madactionvalid[object][actiona] )
    {
      // Fix actiona
      madactionvalid[object][actiona] = btrue;
      madactionstart[object][actiona] = madactionstart[object][actionb];
      madactionend[object][actiona] = madactionend[object][actionb];
    }
    else
    {
      // Fix actionb
      madactionvalid[object][actionb] = btrue;
      madactionstart[object][actionb] = madactionstart[object][actiona];
      madactionend[object][actionb] = madactionend[object][actiona];
    }
  }
}

//--------------------------------------------------------------------------------------------
void get_walk_frame( Uint16 object, LIPT lip_trans, ACTION action )
{
  // ZZ> This helps make walking look right
  int frame = 0;
  int framesinaction = madactionend[object][action] - madactionstart[object][action];

  while ( frame < 16 )
  {
    int framealong = 0;
    if ( framesinaction > 0 )
    {
      framealong = (( float )( frame * framesinaction ) / ( float ) MAXFRAMESPERANIM ) + 2;
      framealong %= framesinaction;
    }
    madframeliptowalkframe[object][lip_trans][frame] = madactionstart[object][action] + framealong;
    frame++;
  }
}

//--------------------------------------------------------------------------------------------
Uint16 get_framefx( char * szName )
{
  // ZZ> This function figures out the IFrame invulnerability, and Attack, Grab, and
  //     Drop timings
  Uint16 fx = 0;

  if(NULL == szName || '\0' == szName[0] || '\0' == szName[1] ) return 0;

  if ( test_frame_name( 'I' ) )
    fx |= MADFX_INVICTUS;
  if ( test_frame_name( 'L' ) )
  {
    if ( test_frame_name( 'A' ) )
      fx |= MADFX_ACTLEFT;
    if ( test_frame_name( 'G' ) )
      fx |= MADFX_GRABLEFT;
    if ( test_frame_name( 'D' ) )
      fx |= MADFX_DROPLEFT;
    if ( test_frame_name( 'C' ) )
      fx |= MADFX_CHARLEFT;
  }
  if ( test_frame_name( 'R' ) )
  {
    if ( test_frame_name( 'A' ) )
      fx |= MADFX_ACTRIGHT;
    if ( test_frame_name( 'G' ) )
      fx |= MADFX_GRABRIGHT;
    if ( test_frame_name( 'D' ) )
      fx |= MADFX_DROPRIGHT;
    if ( test_frame_name( 'C' ) )
      fx |= MADFX_CHARRIGHT;
  }
  if ( test_frame_name( 'S' ) )
    fx |= MADFX_STOP;
  if ( test_frame_name( 'F' ) )
    fx |= MADFX_FOOTFALL;
  if ( test_frame_name( 'P' ) )
    fx |= MADFX_POOF;

  return fx;
}

//--------------------------------------------------------------------------------------------
void make_framelip( Uint16 imdl, ACTION action )
{
  // ZZ> This helps make walking look right
  int frame, framesinaction;

  if ( madactionvalid[imdl][action] )
  {
    framesinaction = madactionend[imdl][action] - madactionstart[imdl][action];
    frame = madactionstart[imdl][action];
    while ( frame < madactionend[imdl][action] )
    {
      madframelip[imdl][frame] = ( frame - madactionstart[imdl][action] ) * 15 / framesinaction;
      madframelip[imdl][frame] = ( madframelip[imdl][frame] ) % 16;
      frame++;
    }
  }
}

//--------------------------------------------------------------------------------------------
void get_actions( Uint16 mdl )
{
  // ZZ> This function creates the frame lists for each action based on the
  //     name of each md2 frame in the model
  int frame, framesinaction;
  ACTION action, lastaction;
  int         iFrameCount;
  MD2_Model * m;
  char      * szName;

  if( mdl>=MAXMODEL || !madused[mdl] ) return;

  m = mad_md2[mdl];
  if(NULL == m) return;

  // Clear out all actions and reset to invalid
  action = 0;
  while ( action < MAXACTION )
  {
    madactionvalid[mdl][action] = bfalse;
    action++;
  }

  iFrameCount = md2_get_numFrames(m);
  if(0 == iFrameCount) return;

  // Set the primary dance action to be the first frame, just as a default
  madactionvalid[mdl][ACTION_DA] = btrue;
  madactionstart[mdl][ACTION_DA] = 0;
  madactionend[mdl][ACTION_DA]   = 1;

  
  // Now go huntin' to see what each frame is, look for runs of same action
  szName = md2_get_Frame(m, 0)->name;
  lastaction = action_number(szName);  
  framesinaction = 0;
  for ( frame = 0; frame < iFrameCount; frame++ )
  {
    MD2_Frame * pFrame = md2_get_Frame(m, frame);
    szName = pFrame->name;
    action = action_number(szName);
    if ( lastaction == action )
    {
      framesinaction++;
    }
    else
    {
      // Write the old action
      if ( lastaction < MAXACTION )
      {
        madactionvalid[mdl][lastaction] = btrue;
        madactionstart[mdl][lastaction] = frame - framesinaction;
        madactionend[mdl][lastaction]   = frame;
      }
      framesinaction = 1;
      lastaction = action;
    }
    madframefx[mdl][frame] = get_framefx( szName );
  }

  // Write the old action
  if ( lastaction < MAXACTION )
  {
    madactionvalid[mdl][lastaction] = btrue;
    madactionstart[mdl][lastaction] = frame - framesinaction;
    madactionend[mdl][lastaction]   = frame;
  }

  // Make sure actions are made valid if a similar one exists
  action_copy_correct( mdl, ACTION_DA, ACTION_DB );   // All dances should be safe
  action_copy_correct( mdl, ACTION_DB, ACTION_DC );
  action_copy_correct( mdl, ACTION_DC, ACTION_DD );
  action_copy_correct( mdl, ACTION_DB, ACTION_DC );
  action_copy_correct( mdl, ACTION_DA, ACTION_DB );
  action_copy_correct( mdl, ACTION_UA, ACTION_UB );
  action_copy_correct( mdl, ACTION_UB, ACTION_UC );
  action_copy_correct( mdl, ACTION_UC, ACTION_UD );
  action_copy_correct( mdl, ACTION_TA, ACTION_TB );
  action_copy_correct( mdl, ACTION_TC, ACTION_TD );
  action_copy_correct( mdl, ACTION_CA, ACTION_CB );
  action_copy_correct( mdl, ACTION_CC, ACTION_CD );
  action_copy_correct( mdl, ACTION_SA, ACTION_SB );
  action_copy_correct( mdl, ACTION_SC, ACTION_SD );
  action_copy_correct( mdl, ACTION_BA, ACTION_BB );
  action_copy_correct( mdl, ACTION_BC, ACTION_BD );
  action_copy_correct( mdl, ACTION_LA, ACTION_LB );
  action_copy_correct( mdl, ACTION_LC, ACTION_LD );
  action_copy_correct( mdl, ACTION_XA, ACTION_XB );
  action_copy_correct( mdl, ACTION_XC, ACTION_XD );
  action_copy_correct( mdl, ACTION_FA, ACTION_FB );
  action_copy_correct( mdl, ACTION_FC, ACTION_FD );
  action_copy_correct( mdl, ACTION_PA, ACTION_PB );
  action_copy_correct( mdl, ACTION_PC, ACTION_PD );
  action_copy_correct( mdl, ACTION_ZA, ACTION_ZB );
  action_copy_correct( mdl, ACTION_ZC, ACTION_ZD );
  action_copy_correct( mdl, ACTION_WA, ACTION_WB );
  action_copy_correct( mdl, ACTION_WB, ACTION_WC );
  action_copy_correct( mdl, ACTION_WC, ACTION_WD );
  action_copy_correct( mdl, ACTION_DA, ACTION_WD );   // All walks should be safe
  action_copy_correct( mdl, ACTION_WC, ACTION_WD );
  action_copy_correct( mdl, ACTION_WB, ACTION_WC );
  action_copy_correct( mdl, ACTION_WA, ACTION_WB );
  action_copy_correct( mdl, ACTION_JA, ACTION_JB );
  action_copy_correct( mdl, ACTION_JB, ACTION_JC );
  action_copy_correct( mdl, ACTION_DA, ACTION_JC );  // All jumps should be safe
  action_copy_correct( mdl, ACTION_JB, ACTION_JC );
  action_copy_correct( mdl, ACTION_JA, ACTION_JB );
  action_copy_correct( mdl, ACTION_HA, ACTION_HB );
  action_copy_correct( mdl, ACTION_HB, ACTION_HC );
  action_copy_correct( mdl, ACTION_HC, ACTION_HD );
  action_copy_correct( mdl, ACTION_HB, ACTION_HC );
  action_copy_correct( mdl, ACTION_HA, ACTION_HB );
  action_copy_correct( mdl, ACTION_KA, ACTION_KB );
  action_copy_correct( mdl, ACTION_KB, ACTION_KC );
  action_copy_correct( mdl, ACTION_KC, ACTION_KD );
  action_copy_correct( mdl, ACTION_KB, ACTION_KC );
  action_copy_correct( mdl, ACTION_KA, ACTION_KB );
  action_copy_correct( mdl, ACTION_MH, ACTION_MI );
  action_copy_correct( mdl, ACTION_DA, ACTION_MM );
  action_copy_correct( mdl, ACTION_MM, ACTION_MN );


  // Create table for doing transition from one type of walk to another...
  // Clear 'em all to start
  for ( frame = 0; frame < iFrameCount; frame++ )
    madframelip[mdl][frame] = 0;

  // Need to figure out how far into action each frame is
  make_framelip( mdl, ACTION_WA );
  make_framelip( mdl, ACTION_WB );
  make_framelip( mdl, ACTION_WC );

  // Now do the same, in reverse, for walking animations
  get_walk_frame( mdl, LIPT_DA, ACTION_DA );
  get_walk_frame( mdl, LIPT_WA, ACTION_WA );
  get_walk_frame( mdl, LIPT_WB, ACTION_WB );
  get_walk_frame( mdl, LIPT_WC, ACTION_WC );
}

//--------------------------------------------------------------------------------------------
void make_mad_equally_lit( Uint16 model )
{
  // ZZ> This function makes ultra low poly models look better
  int frame, vert;
  int iFrames, iVerts;
  MD2_Model * m;

  if(model > MAXMODEL || !madused[model]) return;

  m = mad_md2[model];
  if(NULL == m) return;

  iFrames = md2_get_numFrames(m);
  iVerts  = md2_get_numVertices(m);
  
  for ( frame = 0; frame < iFrames; frame++ )
  {
    MD2_Frame * f = md2_get_Frame(m, frame);
    for ( vert = 0; vert < iVerts; vert++ )
    {
      f->vertices[vert].normal = EQUALLIGHTINDEX;
    }
  }

}

//--------------------------------------------------------------------------------------------
void check_copy( char* loadname, Uint16 object )
{
  // ZZ> This function copies a model's actions
  FILE *fileread;
  ACTION actiona, actionb;
  char szOne[16], szTwo[16];


  madmsgstart[object] = 0;
  fileread = fs_fileOpen( PRI_NONE, NULL, loadname, "r" );
  if ( NULL != fileread )
  {
    while ( fget_next_string( fileread, szOne, sizeof( szOne ) ) )
    {
      actiona = what_action( szOne[0] );

      fget_string( fileread, szTwo, sizeof( szTwo ) );
      actionb = what_action( szTwo[0] );

      action_copy_correct( object, actiona, actionb );
      action_copy_correct( object, actiona + 1, actionb + 1 );
      action_copy_correct( object, actiona + 2, actionb + 2 );
      action_copy_correct( object, actiona + 3, actionb + 3 );
    }
    fs_fileClose( fileread );
  }
}

//--------------------------------------------------------------------------------------------
int load_one_object( int skin, char* tmploadname )
{
  // ZZ> This function loads one object and returns the number of skins
  Uint16 object;
  int numskins, numicon, skin_index;
  STRING newloadname, loc_loadpath, wavename;
  int cnt;

  // Load the object data file and get the object number
  snprintf( newloadname, sizeof( newloadname ), "%s/%s", tmploadname, CData.data_file );
  object = load_one_character_profile( newloadname );


  // Make up a name for the model...  IMPORT\TEMP0000.OBJ
  strncpy( madname[object], tmploadname, sizeof( madname[object] ) );

  // Append a slash to the tmploadname
  strncpy( loc_loadpath, tmploadname, sizeof( loc_loadpath ) );
  strncat( loc_loadpath, "/", sizeof( STRING ) );

  // Load the AI script for this object
  snprintf( newloadname, sizeof( newloadname ), "%s%s", loc_loadpath, CData.script_file );

  // Create a reference to the one we just loaded
  madai[object] = load_ai_script( newloadname );
  if ( MAXAI == madai[object] )
  {
    // use the default script
    madai[object] = 0;
  }


  // Load the object model
  make_newloadname( loc_loadpath, "tris.md2", newloadname );

#ifdef __unix__
  // unix is case sensitive, but sometimes this file is called tris.MD2
  if ( access( newloadname, R_OK ) )
  {
    make_newloadname( loc_loadpath, "tris.MD2", newloadname );
    // still no luck !
    if ( access( newloadname, R_OK ) )
    {
      fprintf( stderr, "ERROR: cannot open: %s\n", newloadname );
      SDL_Quit();
      exit( 1 );
    }
  }
#endif

  load_one_md2( newloadname, object );


  // Fix lighting if need be
  if ( capuniformlit[object] )
  {
    make_mad_equally_lit( object );
  }


  // Create the actions table for this object
  get_actions( object );


  // Copy entire actions to save frame space COPY.TXT
  snprintf( newloadname, sizeof( newloadname ), "%s%s", loc_loadpath, CData.copy_file );
  check_copy( newloadname, object );


  // Load the messages for this object
  make_newloadname( loc_loadpath, CData.message_file, newloadname );
  load_all_messages( newloadname, object );


  // Load the random naming table for this object
  make_newloadname( loc_loadpath, CData.naming_file, newloadname );
  read_naming( object, newloadname );


  // Load the particles for this object
  for ( cnt = 0; cnt < PRTPIP_PEROBJECT_COUNT; cnt++ )
  {
    madprtpip[MAXMODEL][cnt] = MAXPRTPIP;
    snprintf( newloadname, sizeof( newloadname ), "%spart%d.txt", loc_loadpath, cnt );
    load_one_particle_profile( newloadname, object, cnt );
  }


  // Load the waves for this object
  for ( cnt = 0; cnt < MAXWAVE; cnt++ )
  {
    snprintf( wavename, sizeof( wavename ), "%ssound%d.wav", loc_loadpath, cnt );
    capwavelist[object][cnt] = Mix_LoadWAV( wavename );
  }


  // Load the enchantment for this object
  make_newloadname( loc_loadpath, CData.enchant_file, newloadname );
  load_one_enchant_profile( newloadname, object );


  // Load the skins and icons
  madskinstart[object] = skin;
  numskins = 0;
  numicon = 0;


  for ( skin_index = 0; skin_index < MAXSKIN; skin_index++ )
  {
    snprintf( newloadname, sizeof( newloadname ), "%stris%d.bmp", loc_loadpath, skin_index );
    if ( INVALID_TEXTURE != GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[skin+numskins], newloadname, TRANSCOLOR ) )
    {
      numskins++;
      snprintf( newloadname, sizeof( newloadname ), "%sicon%d.bmp", loc_loadpath, skin_index );
      if ( INVALID_TEXTURE != GLTexture_Load( GL_TEXTURE_2D,  &TxIcon[globalnumicon], newloadname, INVALID_KEY ) )
      {
        if ( object == SPELLBOOK && bookicon == 0 )
          bookicon = globalnumicon;

        while ( numicon < numskins )
        {
          madskintoicon[skin+numicon] = globalnumicon;
          numicon++;
        }
        globalnumicon++;
      }
    }
  }

  madskins[object] = numskins;
  if ( numskins == 0 )
  {
    // If we didn't get a skin, set it to the water texture
    madskinstart[object] = 5;
    madskins[object] = 1;
  }


  return numskins;
}

//--------------------------------------------------------------------------------------------
void load_all_objects( char *modname )
{
  // ZZ> This function loads a module's objects
  const char *filehandle;
  FILE* fileread;
  STRING newloadname, filename;
  int cnt;
  int skin;
  int importplayer;

  // Clear the import slots...
  for ( cnt = 0; cnt < MAXMODEL; cnt++ )
    capimportslot[cnt] = 10000;

  // Load the import directory
  importplayer = -1;
  skin = 8;  // Character skins start at 8...  Trust me
  if ( importvalid )
  {
    for ( cnt = 0; cnt < MAXIMPORT; cnt++ )
    {
      snprintf( filename, sizeof( filename ), "%s/temp%04d.obj", CData.import_dir, cnt );
      // Make sure the object exists...
      snprintf( newloadname, sizeof( newloadname ), "%s/%s", filename, CData.data_file );
      fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "r" );
      if ( NULL == fileread ) continue;

      fs_fileClose( fileread );

      // Load it...
      if (( cnt % 9 ) == 0 )
      {
        importplayer++;
      }

      importobject = cnt;
      capimportslot[importobject] = cnt;
      skin += load_one_object( skin, filename );
    }
  }

  //empty_import_directory();  // Free up that disk space...

  // Search for .obj directories and load them
  importobject = -100;
  snprintf( newloadname, sizeof( newloadname ), "%s%s/", modname, CData.objects_dir );
  filehandle = fs_findFirstFile( newloadname, "obj" );

  while ( NULL != filehandle )
  {
    snprintf( filename, sizeof( filename ), "%s%s", newloadname, filehandle );
    skin += load_one_object( skin, filename );
    filehandle = fs_findNextFile();
  }

  fs_findClose();
}

//--------------------------------------------------------------------------------------------
bool_t load_bars( char* szBitmap )
{
  // ZZ> This function loads the status bar bitmap
  int cnt;

  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D, &TxBars, szBitmap, 0 ) )
  {
    return bfalse;
  }


  // Make the blit rectangles
  for ( cnt = 0; cnt < NUMBAR; cnt++ )
  {
    tabrect[cnt].left = 0;
    tabrect[cnt].right = TABX;
    tabrect[cnt].top = cnt * BARY;
    tabrect[cnt].bottom = ( cnt + 1 ) * BARY;
    barrect[cnt].left = TABX;
    barrect[cnt].right = BARX;  // This is reset whenever a bar is drawn
    barrect[cnt].top = tabrect[cnt].top;
    barrect[cnt].bottom = tabrect[cnt].bottom;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
void load_map( char* szModule )
{
  // ZZ> This function loads the map bitmap and the blip bitmap

  // Turn it all off
  mapon = bfalse;
  youarehereon = bfalse;
  numblip = 0;

  // Load the images
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", szModule, CData.gamedat_dir, CData.plan_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D, &TxMap, CStringTmp1, INVALID_KEY ) )
    log_warning( "Cannot load map: %s\n", CStringTmp1 );

  // Set up the rectangles
  mapscale = MIN(( float ) CData.scrx / 640.0f, ( float ) CData.scry / 480.0f );
  maprect.left   = 0;
  maprect.right  = MAPSIZE * mapscale;
  maprect.top    = CData.scry - MAPSIZE * mapscale;
  maprect.bottom = CData.scry;

}

//--------------------------------------------------------------------------------------------
bool_t load_font( char* szBitmap, char* szSpacing )
{
  // ZZ> This function loads the font bitmap and sets up the coordinates
  //     of each font on that bitmap...  Bitmap must have 16x6 fonts
  int cnt, y, xsize, ysize, xdiv, ydiv;
  int xstt, ystt;
  int xspacing, yspacing;
  Uint8 cTmp;
  FILE *fileread;


  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D, &TxFont, szBitmap, 0 ) ) return bfalse;


  // Clear out the conversion table
  for ( cnt = 0; cnt < 256; cnt++ )
    asciitofont[cnt] = 0;


  // Get the size of the bitmap
  xsize = GLTexture_GetImageWidth( &TxFont );
  ysize = GLTexture_GetImageHeight( &TxFont );
  if ( xsize == 0 || ysize == 0 )
    log_warning( "Bad font size! (basicdat/%s) - X size: %i , Y size: %i\n", szBitmap, xsize, ysize );


  // Figure out the general size of each font
  ydiv = ysize / NUMFONTY;
  xdiv = xsize / NUMFONTX;


  // Figure out where each font is and its spacing
  fileread = fs_fileOpen( PRI_NONE, NULL, szSpacing, "r" );
  if ( fileread == NULL ) return bfalse;

  globalname = szSpacing;

  // Uniform font height is at the top
  yspacing = fget_next_int( fileread );
  fontoffset = CData.scry - yspacing;

  // Mark all as unused
  for ( cnt = 0; cnt < 255; cnt++ )
    asciitofont[cnt] = 255;


  cnt = 0;
  y = 0;
  xstt = 0;
  ystt = 0;
  while ( cnt < 255 && fgoto_colon_yesno( fileread ) )
  {
    cTmp = fgetc( fileread );
    xspacing = fget_int( fileread );
    if ( asciitofont[cTmp] == 255 )
    {
      asciitofont[cTmp] = cnt;
    }

    if ( xstt + xspacing + 1 >= xsize )
    {
      xstt = 0;
      ystt += yspacing;
    }

    fontrect[cnt].x = xstt;
    fontrect[cnt].w = xspacing;
    fontrect[cnt].y = ystt;
    fontrect[cnt].h = yspacing - 1;
    fontxspacing[cnt] = xspacing;

    xstt += xspacing + 1;

    cnt++;
  }
  fs_fileClose( fileread );


  // Space between lines
  fontyspacing = ( yspacing >> 1 ) + FONTADD;

  return btrue;
}

//--------------------------------------------------------------------------------------------
void make_water()
{
  // ZZ> This function sets up water movements
  int layer, frame, point, mode, cnt;
  float tmp_sin, tmp_cos, tmp;
  Uint8 spek;


  layer = 0;
  while ( layer < numwaterlayer )
  {
    waterlayeru[layer] = 0;
    waterlayerv[layer] = 0;
    frame = 0;
    while ( frame < MAXWATERFRAME )
    {
      // Do first mode
      mode = 0;
      for ( point = 0; point < WATERPOINTS; point++ )
      {
        tmp_sin = sin(( frame * TWO_PI / MAXWATERFRAME ) + ( PI * point / WATERPOINTS ) + ( PI_OVER_TWO * layer / MAXWATERLAYER ) );
        tmp_cos = cos(( frame * TWO_PI / MAXWATERFRAME ) + ( PI * point / WATERPOINTS ) + ( PI_OVER_TWO * layer / MAXWATERLAYER ) );
        waterlayerzadd[layer][frame][mode][point]  = tmp_sin * waterlayeramp[layer];
      }

      // Now mirror and copy data to other three modes
      mode++;
      waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][1];
      //waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][1];
      waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][0];
      //waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][0];
      waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][3];
      //waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][3];
      waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][2];
      //waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][2];
      mode++;

      waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][3];
      //waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][3];
      waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][2];
      //waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][2];
      waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][1];
      //waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][1];
      waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][0];
      //waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][0];
      mode++;

      waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][2];
      //waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][2];
      waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][3];
      //waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][3];
      waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][0];
      //waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][0];
      waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][1];
      //waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][1];
      frame++;
    }
    layer++;
  }


  // Calculate specular highlights
  spek = 0;
  for ( cnt = 0; cnt < 256; cnt++ )
  {
    tmp = FP8_TO_FLOAT( cnt );
    spek = 255 * tmp * tmp;

    waterspek[cnt] = spek;

    // [claforte] Probably need to replace this with a
    //            glColor4f( FP8_TO_FLOAT(spek), FP8_TO_FLOAT(spek), FP8_TO_FLOAT(spek), 1.0f) call:
  }
}

//--------------------------------------------------------------------------------------------
void read_wawalite( char *modname )
{
  // ZZ> This function sets up water and lighting for the module
  FILE* fileread;

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.wawalite_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, CStringTmp1, "r" );
  if ( NULL != fileread )
  {
    fgoto_colon( fileread );
    //  !!!BAD!!!
    //  Random map...
    //  If someone else wants to handle this, here are some thoughts for approaching
    //  it.  The .MPD file for the level should give the basic size of the map.  Use
    //  a standard tile set like the Palace modules.  Only use objects that are in
    //  the module's object directory, and only use some of them.  Imagine several Rock
    //  Moles eating through a stone filled level to make a path from the entrance to
    //  the exit.  Door placement will be difficult.
    //  !!!BAD!!!


    // Read water data first
    numwaterlayer = fget_next_int( fileread );
    waterspekstart = fget_next_int( fileread );
    waterspeklevel_fp8 = fget_next_int( fileread );
    waterdouselevel = fget_next_int( fileread );
    watersurfacelevel = fget_next_int( fileread );
    waterlight = fget_next_bool( fileread );
    wateriswater = fget_next_bool( fileread );
    CData.render_overlay = fget_next_bool( fileread ) && CData.overlayvalid;
    CData.render_background = fget_next_bool( fileread ) && CData.backgroundvalid;
    waterlayerdistx[0] = fget_next_float( fileread );
    waterlayerdisty[0] = fget_next_float( fileread );
    waterlayerdistx[1] = fget_next_float( fileread );
    waterlayerdisty[1] = fget_next_float( fileread );
    foregroundrepeat = fget_next_int( fileread );
    backgroundrepeat = fget_next_int( fileread );


    waterlayerz[0] = fget_next_int( fileread );
    waterlayeralpha_fp8[0] = fget_next_int( fileread );
    waterlayerframeadd[0] = fget_next_int( fileread );
    waterlightlevel_fp8[0] = fget_next_int( fileread );
    waterlightadd_fp8[0] = fget_next_int( fileread );
    waterlayeramp[0] = fget_next_float( fileread );
    waterlayeruadd[0] = fget_next_float( fileread );
    waterlayervadd[0] = fget_next_float( fileread );

    waterlayerz[1] = fget_next_int( fileread );
    waterlayeralpha_fp8[1] = fget_next_int( fileread );
    waterlayerframeadd[1] = fget_next_int( fileread );
    waterlightlevel_fp8[1] = fget_next_int( fileread );
    waterlightadd_fp8[1] = fget_next_int( fileread );
    waterlayeramp[1] = fget_next_float( fileread );
    waterlayeruadd[1] = fget_next_float( fileread );
    waterlayervadd[1] = fget_next_float( fileread );

    waterlayeru[0] = 0;
    waterlayerv[0] = 0;
    waterlayeru[1] = 0;
    waterlayerv[1] = 0;
    waterlayerframe[0] = rand() & WATERFRAMEAND;
    waterlayerframe[1] = rand() & WATERFRAMEAND;

    // Read light data second
    lightspekdir.x = fget_next_float( fileread );
    lightspekdir.y = fget_next_float( fileread );
    lightspekdir.z = fget_next_float( fileread );
    lightambi  = fget_next_float( fileread );

    lightspek = DotProduct( lightspekdir, lightspekdir );
    if ( 0 != lightspek )
    {
      lightspek = sqrt( lightspek );
      lightspekdir.x /= lightspek;
      lightspekdir.y /= lightspek;
      lightspekdir.z /= lightspek;

      lightspek *= lightambi;
    }

    lightspekcol.r = 
    lightspekcol.g = 
    lightspekcol.b = lightspek;

    lightambicol.r =
    lightambicol.g =
    lightambicol.b = lightambi;

    // Read tile data third
    hillslide = fget_next_float( fileread );
    slippyfriction = fget_next_float( fileread );
    airfriction = fget_next_float( fileread );
    waterfriction = fget_next_float( fileread );
    noslipfriction = fget_next_float( fileread );
    gravity = fget_next_float( fileread );
    slippyfriction = MAX( slippyfriction, sqrt( noslipfriction ) );
    airfriction    = MAX( airfriction,    sqrt( slippyfriction ) );
    waterfriction  = MIN( waterfriction,  pow( airfriction, 4.0f ) );

    animtileupdateand = fget_next_int( fileread );
    animtileframeand = fget_next_int( fileread );
    animtilebigframeand = ( animtileframeand << 1 ) + 1;
    damagetileamount = fget_next_int( fileread );
    damagetiletype = fget_next_damage( fileread );

    // Read weather data fourth
    weatheroverwater = fget_next_bool( fileread );
    weathertimereset = fget_next_int( fileread );
    weathertime = weathertimereset;
    weatherplayer = 0;

    // Read extra data
    meshexploremode = fget_next_bool( fileread );
    usefaredge = fget_next_bool( fileread );
    camswing = 0;
    camswingrate = fget_next_float( fileread );
    camswingamp = fget_next_float( fileread );


    // Read unnecessary data...  Only read if it exists...
    fogon = bfalse;
    fogaffectswater = btrue;
    fogtop = 100;
    fogbottom = 0;
    fogdistance = 100;
    fogred = 255;
    foggrn = 255;
    fogblu = 255;
    damagetileparttype = MAXPRTPIP;
    damagetilepartand = 255;
    damagetilesound = INVALID_SOUND;

    if ( fgoto_colon_yesno( fileread ) )
    {
      fogon           = CData.fogallowed;
      fogtop          = fget_next_float( fileread );
      fogbottom       = fget_next_float( fileread );
      fogred          = fget_next_fixed( fileread );
      foggrn          = fget_next_fixed( fileread );
      fogblu          = fget_next_fixed( fileread );
      fogaffectswater = fget_next_bool( fileread );

      fogdistance = ( fogtop - fogbottom );
      if ( fogdistance < 1.0 )  fogon = bfalse;

      // Read extra stuff for damage tile particles...
      if ( fgoto_colon_yesno( fileread ) )
      {
        damagetileparttype = fget_int( fileread );
        damagetilepartand  = fget_next_int( fileread );
        damagetilesound    = fget_next_int( fileread );
      }
    }

    // Allow slow machines to ignore the fancy stuff
    if ( !CData.twolayerwateron && numwaterlayer > 1 )
    {
      int iTmp;
      numwaterlayer = 1;
      iTmp = waterlayeralpha_fp8[0];
      iTmp = FP8_MUL( waterlayeralpha_fp8[1], iTmp ) + iTmp;
      if ( iTmp > 255 ) iTmp = 255;
      waterlayeralpha_fp8[0] = iTmp;
    }


    fs_fileClose( fileread );

    // Do it
    make_speklut();
    make_lighttospek();
    make_spektable( lightspekdir );
    make_water();
  }

}

//--------------------------------------------------------------------------------------------
void render_background( Uint16 texture )
{
  // ZZ> This function draws the large background
  GLVertex vtlist[4];
  float size;
  float sinsize, cossize;
  float x, y, z, u, v;
  int i;

  float loc_backgroundrepeat;


  // Figure out the screen coordinates of its corners
  x = CData.scrx << 6;
  y = CData.scry << 6;
  z = -100;
  u = waterlayeru[1];
  v = waterlayerv[1];
  size = x + y + 1;
  sinsize = turntosin[( 3*2047 ) & TRIGTABLE_MASK] * size;   // why 3/8 of a turn???
  cossize = turntosin[( 3*2047 + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK] * size;   // why 3/8 of a turn???
  loc_backgroundrepeat = backgroundrepeat * MIN( x / CData.scrx, y / CData.scrx );


  vtlist[0].pos.x = x + cossize;
  vtlist[0].pos.y = y - sinsize;
  vtlist[0].pos.z = z;
  vtlist[0].s = 0 + u;
  vtlist[0].t = 0 + v;

  vtlist[1].pos.x = x + sinsize;
  vtlist[1].pos.y = y + cossize;
  vtlist[1].pos.z = z;
  vtlist[1].s = loc_backgroundrepeat + u;
  vtlist[1].t = 0 + v;

  vtlist[2].pos.x = x - cossize;
  vtlist[2].pos.y = y + sinsize;
  vtlist[2].pos.z = z;
  vtlist[2].s = loc_backgroundrepeat + u;
  vtlist[2].t = loc_backgroundrepeat + v;

  vtlist[3].pos.x = x - sinsize;
  vtlist[3].pos.y = y - cossize;
  vtlist[3].pos.z = z;
  vtlist[3].s = 0 + u;
  vtlist[3].t = loc_backgroundrepeat + v;

  //-------------------------------------------------
  ATTRIB_PUSH( "render_background", GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT );
  {
    glShadeModel( GL_FLAT );  // Flat shade this
    glDepthMask( GL_ALWAYS );

    glDisable( GL_CULL_FACE );

    GLTexture_Bind( &TxTexture[texture], CData.texturefilter );

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    glBegin( GL_TRIANGLE_FAN );
    for ( i = 0; i < 4; i++ )
    {
      glTexCoord2f( vtlist[i].s, vtlist[i].t );
      glVertex3fv( vtlist[i].pos.v );
    }
    glEnd();
  };
  ATTRIB_POP( "render_background" );
  //-------------------------------------------------
}



//--------------------------------------------------------------------------------------------
void render_foreground_overlay( Uint16 texture )
{
  GLVertex vtlist[4];
  float size;
  float sinsize, cossize;
  float x, y, z, u, v;
  int i;
  Uint16 rotate;
  float loc_foregroundrepeat;

  // Figure out the screen coordinates of its corners
  x = CData.scrx << 6;
  y = CData.scry << 6;
  z = 0;
  u = waterlayeru[1];
  v = waterlayerv[1];
  size = x + y + 1;
  rotate = 16384 + 8192;
  rotate >>= 2;
  sinsize = turntosin[rotate & TRIGTABLE_MASK] * size;
  cossize = turntosin[( rotate+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK] * size;

  loc_foregroundrepeat = foregroundrepeat * MIN( x / CData.scrx, y / CData.scrx ) / 4.0;


  vtlist[0].pos.x = x + cossize;
  vtlist[0].pos.y = y - sinsize;
  vtlist[0].pos.z = z;
  vtlist[0].s = 0 + u;
  vtlist[0].t = 0 + v;

  vtlist[1].pos.x = x + sinsize;
  vtlist[1].pos.y = y + cossize;
  vtlist[1].pos.z = z;
  vtlist[1].s = loc_foregroundrepeat + u;
  vtlist[1].t = v;

  vtlist[2].pos.x = x - cossize;
  vtlist[2].pos.y = y + sinsize;
  vtlist[2].pos.z = z;
  vtlist[2].s = loc_foregroundrepeat + u;
  vtlist[2].t = loc_foregroundrepeat + v;

  vtlist[3].pos.x = x - sinsize;
  vtlist[3].pos.y = y - cossize;
  vtlist[3].pos.z = z;
  vtlist[3].s = u;
  vtlist[3].t = loc_foregroundrepeat + v;

  //-------------------------------------------------
  ATTRIB_PUSH( "render_forground_overlay", GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_HINT_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT );
  {
    glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );  // make sure that the texture is as smooth as possible

    glShadeModel( GL_FLAT );  // Flat shade this

    glDepthMask( GL_FALSE );   // do not write into the depth buffer
    glDepthFunc( GL_ALWAYS );  // make it appear over the top of everything

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );  // make the texture a filter

    glDisable( GL_CULL_FACE );

    GLTexture_Bind( &TxTexture[texture], CData.texturefilter );

    glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
    glBegin( GL_TRIANGLE_FAN );
    for ( i = 0; i < 4; i++ )
    {
      glTexCoord2f( vtlist[i].s, vtlist[i].t );
      glVertex3fv( vtlist[i].pos.v );
    }
    glEnd();
  }
  ATTRIB_POP( "render_forground_overlay" );
  //-------------------------------------------------
}

//--------------------------------------------------------------------------------------------
void render_shadow( CHR_REF character )
{
  // ZZ> This function draws a NIFTY shadow
  GLVertex v[4];

  float x, y;
  float level, height;
  float size_umbra_x,size_umbra_y, size_penumbra_x,size_penumbra_y;
  float height_factor, ambient_factor, tile_factor;
  float alpha_umbra, alpha_penumbra, alpha_character, light_character;
  Sint8 hide;
  int i;
  Uint16 chrlightambi = chrlightambir_fp8[character] + chrlightambig_fp8[character] + chrlightambib_fp8[character];
  Uint16 chrlightspek = chrlightspekr_fp8[character] + chrlightspekg_fp8[character] + chrlightspekb_fp8[character];
  float  globlightambi = lightambicol.r + lightambicol.g + lightambicol.b;
  float  globlightspek = lightspekcol.r + lightspekcol.g + lightspekcol.b;

  hide = caphidestate[chrmodel[character]];
  if ( hide != NOHIDE && hide == chraistate[character] ) return;

  // Original points
  level = chrlevel[character];
  level += SHADOWRAISE;
  height = chrmatrix[character]_CNV( 3, 2 ) - level;
  if ( height < 0 ) height = 0;

  tile_factor = mesh_has_some_bits( chronwhichfan[character], MESHFX_WATER ) ? 0.5 : 1.0;

  height_factor   = MAX( MIN(( 5 * chrbmpdata[character].calc_size / height ), 1 ), 0 );
  ambient_factor  = ( float )( chrlightspek ) / ( float )( chrlightambi + chrlightspek );
  ambient_factor  = 0.5f * ( ambient_factor + globlightspek / ( globlightambi + globlightspek ) );
  alpha_character = FP8_TO_FLOAT( chralpha_fp8[character] );
  if ( chrlight_fp8[character] == 255 )
  {
    light_character = 1.0f;
  }
  else
  {
    light_character = ( float ) chrlightspek / 3.0f / ( float ) chrlight_fp8[character];
    light_character =  MIN( 1, MAX( 0, light_character ) );
  };


  size_umbra_x    = ( chrbmpdata[character].cv.x_max - chrbmpdata[character].cv.x_min - height / 30.0 );
  size_umbra_y    = ( chrbmpdata[character].cv.y_max - chrbmpdata[character].cv.y_min - height / 30.0 );
  size_penumbra_x = ( chrbmpdata[character].cv.x_max - chrbmpdata[character].cv.x_min + height / 30.0 );
  size_penumbra_y = ( chrbmpdata[character].cv.y_max - chrbmpdata[character].cv.y_min + height / 30.0 );

  alpha_umbra    = alpha_character * height_factor * ambient_factor * light_character * tile_factor;
  alpha_penumbra = alpha_character * height_factor * ambient_factor * light_character * tile_factor;

  if ( FLOAT_TO_FP8( alpha_umbra ) == 0 && FLOAT_TO_FP8( alpha_penumbra ) == 0 ) return;

  x = chrmatrix[character]_CNV( 3, 0 );
  y = chrmatrix[character]_CNV( 3, 1 );

  //GOOD SHADOW
  v[0].s = CALCULATE_PRT_U0( 238 );
  v[0].t = CALCULATE_PRT_V0( 238 );

  v[1].s = CALCULATE_PRT_U1( 255 );
  v[1].t = CALCULATE_PRT_V0( 238 );

  v[2].s = CALCULATE_PRT_U1( 255 );
  v[2].t = CALCULATE_PRT_V1( 255 );

  v[3].s = CALCULATE_PRT_U0( 238 );
  v[3].t = CALCULATE_PRT_V1( 255 );

  if ( size_penumbra_x > 0 && size_penumbra_y > 0 )
  {
    v[0].pos.x = x + size_penumbra_x;
    v[0].pos.y = y - size_penumbra_y;
    v[0].pos.z = level;

    v[1].pos.x = x + size_penumbra_x;
    v[1].pos.y = y + size_penumbra_y;
    v[1].pos.z = level;

    v[2].pos.x = x - size_penumbra_x;
    v[2].pos.y = y + size_penumbra_y;
    v[2].pos.z = level;

    v[3].pos.x = x - size_penumbra_x;
    v[3].pos.y = y - size_penumbra_y;
    v[3].pos.z = level;

    ATTRIB_PUSH( "render_shadow", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT );
    {
      glEnable( GL_BLEND );
      glBlendFunc( GL_ZERO, GL_ONE_MINUS_SRC_COLOR );

      glDepthMask( GL_FALSE );
      glDepthFunc( GL_LEQUAL );

      // Choose texture.
      GLTexture_Bind( &TxTexture[particletexture], CData.texturefilter );

      glBegin( GL_TRIANGLE_FAN );
      glColor4f( alpha_penumbra, alpha_penumbra, alpha_penumbra, 1.0 );
      for ( i = 0; i < 4; i++ )
      {
        glTexCoord2f( v[i].s, v[i].t );
        glVertex3fv( v[i].pos.v );
      }
      glEnd();
    }
    ATTRIB_POP( "render_shadow" );
  };

  if ( size_umbra_x > 0 && size_umbra_y > 0 )
  {
    v[0].pos.x = x + size_umbra_x;
    v[0].pos.y = y - size_umbra_y;
    v[0].pos.z = level + 0.1;

    v[1].pos.x = x + size_umbra_x;
    v[1].pos.y = y + size_umbra_y;
    v[1].pos.z = level + 0.1;

    v[2].pos.x = x - size_umbra_x;
    v[2].pos.y = y + size_umbra_y;
    v[2].pos.z = level + 0.1;

    v[3].pos.x = x - size_umbra_x;
    v[3].pos.y = y - size_umbra_y;
    v[3].pos.z = level + 0.1;

    ATTRIB_PUSH( "render_shadow", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT );
    {
      glDisable( GL_CULL_FACE );

      glEnable( GL_BLEND );
      glBlendFunc( GL_ZERO, GL_ONE_MINUS_SRC_COLOR );

      glDepthMask( GL_FALSE );
      glDepthFunc( GL_LEQUAL );

      // Choose texture.
      GLTexture_Bind( &TxTexture[particletexture], CData.texturefilter );

      glBegin( GL_TRIANGLE_FAN );
      glColor4f( alpha_penumbra, alpha_penumbra, alpha_penumbra, 1.0 );
      for ( i = 0; i < 4; i++ )
      {
        glTexCoord2f( v[i].s, v[i].t );
        glVertex3fv( v[i].pos.v );
      }
      glEnd();
    }
    ATTRIB_POP( "render_shadow" );
  };
};

//--------------------------------------------------------------------------------------------
//void render_bad_shadow(CHR_REF character)
//{
//  // ZZ> This function draws a sprite shadow
//  GLVertex v[4];
//  float size, x, y;
//  Uint8 ambi;
//  //DWORD light;
//  float level; //, z;
//  int height;
//  Sint8 hide;
//  Uint8 trans;
//  int i;
//
//
//  hide = caphidestate[chrmodel[character]];
//  if (hide == NOHIDE || hide != chraistate[character])
//  {
//    // Original points
//    level = chrlevel[character];
//    level += SHADOWRAISE;
//    height = chrmatrix[character]_CNV(3, 2) - level;
//    if (height > 255)  return;
//    if (height < 0) height = 0;
//    size = chrbmpdata[character].calc_shadowsize - FP8_MUL(height, chrbmpdata[character].calc_shadowsize);
//    if (size < 1) return;
//    ambi = chrlightspek_fp8[character] >> 4;  // LUL >>3;
//    trans = ((255 - height) >> 1) + 64;
//
//    x = chrmatrix[character]_CNV(3, 0);
//    y = chrmatrix[character]_CNV(3, 1);
//    v[0].pos.x = (float) x + size;
//    v[0].pos.y = (float) y - size;
//    v[0].pos.z = (float) level;
//
//    v[1].pos.x = (float) x + size;
//    v[1].pos.y = (float) y + size;
//    v[1].pos.z = (float) level;
//
//    v[2].pos.x = (float) x - size;
//    v[2].pos.y = (float) y + size;
//    v[2].pos.z = (float) level;
//
//    v[3].pos.x = (float) x - size;
//    v[3].pos.y = (float) y - size;
//    v[3].pos.z = (float) level;
//
//
//    v[0].s = CALCULATE_PRT_U0(236);
//    v[0].t = CALCULATE_PRT_V0(236);
//
//    v[1].s = CALCULATE_PRT_U1(253);
//    v[1].t = CALCULATE_PRT_V0(236);
//
//    v[2].s = CALCULATE_PRT_U1(253);
//    v[2].t = CALCULATE_PRT_V1(253);
//
//    v[3].s = CALCULATE_PRT_U0(236);
//    v[3].t = CALCULATE_PRT_V1(253);
//
//    ATTRIB_PUSH("render_bad_shadow", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);
//    {
//
//      glDisable(GL_CULL_FACE);
//
//      //glEnable(GL_ALPHA_TEST);
//      //glAlphaFunc(GL_GREATER, 0);
//
//      glEnable(GL_BLEND);
//      glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
//
//      glDepthMask(GL_FALSE);
//      glDepthFunc(GL_LEQUAL);
//
//      // Choose texture.
//      GLTexture_Bind(&TxTexture[particletexture], CData.texturefilter);
//
//      glColor4f(FP8_TO_FLOAT(ambi), FP8_TO_FLOAT(ambi), FP8_TO_FLOAT(ambi), FP8_TO_FLOAT(trans));
//      glBegin(GL_TRIANGLE_FAN);
//      for (i = 0; i < 4; i++)
//      {
//        glTexCoord2f(v[i].s, v[i].t);
//        glVertex3fv(v[i].pos.v);
//      }
//      glEnd();
//    }
//    ATTRIB_POP("render_bad_shadow");
//  }
//}
//

//--------------------------------------------------------------------------------------------
void calc_chr_lighting( int x, int y, Uint16 tl, Uint16 tr, Uint16 bl, Uint16 br, Uint16 * spek, Uint16 * ambi )
{
  ( *spek ) = 0;
  ( *ambi ) = MIN( MIN( tl, tr ), MIN( bl, br ) );

  // Interpolate lighting level using tile corners
  switch ( x )
  {
    case 0:
      ( *spek ) += ( tl - ( *ambi ) ) << 1;
      ( *spek ) += ( bl - ( *ambi ) ) << 1;
      break;

    case 1:
    case 2:
      ( *spek ) += ( tl - ( *ambi ) );
      ( *spek ) += ( tr - ( *ambi ) );
      ( *spek ) += ( bl - ( *ambi ) );
      ( *spek ) += ( br - ( *ambi ) );
      break;

    case 3:
      ( *spek ) += ( tr - ( *ambi ) ) << 1;
      ( *spek ) += ( br - ( *ambi ) ) << 1;
      break;
  }


  switch ( y )
  {
    case 0:
      ( *spek ) += ( tl - ( *ambi ) ) << 1;
      ( *spek ) += ( tr - ( *ambi ) ) << 1;
      break;

    case 1:
    case 2:
      ( *spek ) += ( tl - ( *ambi ) );
      ( *spek ) += ( tr - ( *ambi ) );
      ( *spek ) += ( bl - ( *ambi ) );
      ( *spek ) += ( br - ( *ambi ) );
      break;

    case 3:
      ( *spek ) += ( bl - ( *ambi ) ) << 1;
      ( *spek ) += ( br - ( *ambi ) ) << 1;
      break;
  }

  ( *spek ) >>= 3;
};

//--------------------------------------------------------------------------------------------
void light_characters()
{
  // ZZ> This function figures out character lighting
  int cnt, tnc, x, y;
  Uint16 i0, i1, i2, i3;
  Uint16 spek, ambi;
  Uint32 vrtstart;


  for ( cnt = 0; cnt < numdolist; cnt++ )
  {
    tnc = dolist[cnt];
    vrtstart = meshvrtstart[chronwhichfan[tnc]];

    x = chrpos[tnc].x;
    y = chrpos[tnc].y;
    x = ( x & 127 ) >> 5;  // From 0 to 3
    y = ( y & 127 ) >> 5;  // From 0 to 3

    i0 = meshvrtlr_fp8[vrtstart + 0];
    i1 = meshvrtlr_fp8[vrtstart + 1];
    i2 = meshvrtlr_fp8[vrtstart + 2];
    i3 = meshvrtlr_fp8[vrtstart + 3];
    calc_chr_lighting( x, y, i0, i1, i2, i3, &spek, &ambi );
    chrlightambir_fp8[tnc] = ambi;
    chrlightspekr_fp8[tnc] = spek;

    if ( !meshexploremode )
    {
      // Look up spek direction using corners again
      i0 = (( i0 & 0xf0 ) << 8 ) & 0xf000;
      i1 = (( i1 & 0xf0 ) << 4 ) & 0x0f00;
      i3 = (( i3 & 0xf0 ) << 0 ) & 0x00f0;
      i2 = (( i2 & 0xf0 ) >> 4 ) & 0x000f;
      i0 = i0 | i1 | i3 | i2;
      chrlightturn_lrr[tnc] = ( lightdirectionlookup[i0] << 8 );
    }
    else
    {
      chrlightturn_lrr[tnc] = 0;
    }

    i0 = meshvrtlg_fp8[vrtstart + 0];
    i1 = meshvrtlg_fp8[vrtstart + 1];
    i3 = meshvrtlg_fp8[vrtstart + 2];
    i2 = meshvrtlg_fp8[vrtstart + 3];
    calc_chr_lighting( x, y, i0, i1, i2, i3, &spek, &ambi );
    chrlightambig_fp8[tnc] = ambi;
    chrlightspekg_fp8[tnc] = spek;

    if ( !meshexploremode )
    {
      // Look up spek direction using corners again
      i0 = (( i0 & 0xf0 ) << 8 ) & 0xf000;
      i1 = (( i1 & 0xf0 ) << 4 ) & 0x0f00;
      i3 = (( i3 & 0xf0 ) << 0 ) & 0x00f0;
      i2 = (( i2 & 0xf0 ) >> 4 ) & 0x000f;
      i0 = i0 | i1 | i3 | i2;
      chrlightturn_lrg[tnc] = ( lightdirectionlookup[i0] << 8 );
    }
    else
    {
      chrlightturn_lrg[tnc] = 0;
    }

    calc_chr_lighting( x, y, i0, i1, i2, i3, &spek, &ambi );
    chrlightambib_fp8[tnc] = ambi;
    chrlightspekb_fp8[tnc] = spek;

    if ( !meshexploremode )
    {
      // Look up spek direction using corners again
      i0 = (( i0 & 0xf0 ) << 8 ) & 0xf000;
      i1 = (( i1 & 0xf0 ) << 4 ) & 0x0f00;
      i3 = (( i3 & 0xf0 ) << 0 ) & 0x00f0;
      i2 = (( i2 & 0xf0 ) >> 4 ) & 0x000f;
      i0 = i0 | i1 | i3 | i2;
      chrlightturn_lrb[tnc] = ( lightdirectionlookup[i0] << 8 );
    }
    else
    {
      chrlightturn_lrb[tnc] = 0;
    }
  }
}

//--------------------------------------------------------------------------------------------
void light_particles()
{
  // ZZ> This function figures out particle lighting
  int cnt;
  CHR_REF character;

  for ( cnt = 0; cnt < MAXPRT; cnt++ )
  {
    if ( !VALID_PRT( cnt ) ) continue;

    switch ( prttype[cnt] )
    {
      case PRTTYPE_LIGHT:
        {
          float ftmp = prtdynalightlevel[cnt] * ( 127 * prtdynalightfalloff[cnt] ) / FP8_TO_FLOAT( FP8_MUL( prtsize_fp8[cnt], prtsize_fp8[cnt] ) );
          if ( ftmp > 255 ) ftmp = 255;

          prtlightr_fp8[cnt] =
          prtlightg_fp8[cnt] =
          prtlightb_fp8[cnt] = ftmp;
        }
        break;

      case PRTTYPE_ALPHA:
      case PRTTYPE_SOLID:
        {
          character = prt_get_attachedtochr( cnt );
          if ( VALID_CHR( character ) )
          {
            prtlightr_fp8[cnt] = chrlightspekr_fp8[character] + chrlightambir_fp8[character];
            prtlightg_fp8[cnt] = chrlightspekg_fp8[character] + chrlightambig_fp8[character];
            prtlightb_fp8[cnt] = chrlightspekb_fp8[character] + chrlightambib_fp8[character];
          }
          else if ( INVALID_FAN != prtonwhichfan[cnt] )
          {
            prtlightr_fp8[cnt] = meshvrtlr_fp8[meshvrtstart[prtonwhichfan[cnt]]];
            prtlightg_fp8[cnt] = meshvrtlg_fp8[meshvrtstart[prtonwhichfan[cnt]]];
            prtlightb_fp8[cnt] = meshvrtlb_fp8[meshvrtstart[prtonwhichfan[cnt]]];
          }
          else
          {
            prtlightr_fp8[cnt] =
              prtlightg_fp8[cnt] =
                prtlightb_fp8[cnt] = 0;
          }
        }
        break;

      default:
        prtlightr_fp8[cnt] =
          prtlightg_fp8[cnt] =
            prtlightb_fp8[cnt] = 0;
    };
  }

}

//--------------------------------------------------------------------------------------------
void render_water()
{
  // ZZ> This function draws all of the water fans

  int cnt;

  // Bottom layer first
  if ( !CData.render_background && numwaterlayer > 1 )
  {
    cnt = 0;
    while ( cnt < numrenderlist_watr )
    {
      render_water_fan( renderlist_watr[cnt], 1, (( renderlist_watr[cnt] >> watershift ) &2 ) + ( renderlist_watr[cnt]&1 ) );
      cnt++;
    }
  }

  // Top layer second
  if ( !CData.render_overlay && numwaterlayer > 0 )
  {
    cnt = 0;
    while ( cnt < numrenderlist_watr )
    {
      render_water_fan( renderlist_watr[cnt], 0, (( renderlist_watr[cnt] >> watershift ) &2 ) + ( renderlist_watr[cnt]&1 ) );
      cnt++;
    }
  }
}

void render_water_lit()
{
  // BB> This function draws the hilites for water tiles using global lighting

  int cnt;

  // Bottom layer first
  if ( !CData.render_background && numwaterlayer > 1 )
  {
    float ambi_level = FP8_TO_FLOAT( waterlightadd_fp8[1] + waterlightlevel_fp8[1] );
    float spek_level =  FP8_TO_FLOAT( waterspeklevel_fp8 );
    float spekularity = MIN( 40, spek_level / ambi_level ) + 2;
    GLfloat mat_none[]      = {0, 0, 0, 0};
    GLfloat mat_ambient[]   = { ambi_level, ambi_level, ambi_level, 1.0 };
    GLfloat mat_diffuse[]   = { spek_level, spek_level, spek_level, 1.0 };
    GLfloat mat_shininess[] = {spekularity};

    if ( waterlight )
    {
      // self-lit water provides its own light
      glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, mat_ambient );
    }
    else
    {
      // non-self-lit water needs an external lightsource
      glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, mat_none );
    }

    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,  mat_diffuse );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, mat_diffuse );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess );

    cnt = 0;
    while ( cnt < numrenderlist_watr )
    {
      render_water_fan_lit( renderlist_watr[cnt], 1, (( renderlist_watr[cnt] >> watershift ) &2 ) + ( renderlist_watr[cnt]&1 ) );
      cnt++;
    }
  }

  // Top layer second
  if ( !CData.render_overlay && numwaterlayer > 0 )
  {
    float ambi_level = ( waterlightadd_fp8[1] + waterlightlevel_fp8[1] ) / 255.0;
    float spek_level =  FP8_TO_FLOAT( waterspeklevel_fp8 );
    float spekularity = MIN( 40, spek_level / ambi_level ) + 2;
    GLfloat mat_none[]      = {0, 0, 0, 0};
    GLfloat mat_ambient[]   = { ambi_level, ambi_level, ambi_level, 1.0 };
    GLfloat mat_diffuse[]   = { spek_level, spek_level, spek_level, 1.0 };
    GLfloat mat_shininess[] = {spekularity};

    if ( waterlight )
    {
      // self-lit water provides its own light
      glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, mat_ambient );
    }
    else
    {
      // non-self-lit water needs an external lightsource
      glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, mat_none );
    }

    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,  mat_diffuse );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, mat_diffuse );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess );

    cnt = 0;
    while ( cnt < numrenderlist_watr )
    {
      render_water_fan_lit( renderlist_watr[cnt], 0, (( renderlist_watr[cnt] >> watershift ) &2 ) + ( renderlist_watr[cnt]&1 ) );
      cnt++;
    }
  }

}

//--------------------------------------------------------------------------------------------
void render_good_shadows()
{
  int cnt, tnc;

  // Good shadows for me
  ATTRIB_PUSH( "render_good_shadows", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT );
  {
    glDisable( GL_ALPHA_TEST );
    //glAlphaFunc(GL_GREATER, 0);

    glDisable( GL_CULL_FACE );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ZERO, GL_ONE_MINUS_SRC_COLOR );

    glDepthMask( GL_FALSE );
    glDepthFunc( GL_LEQUAL );

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
      tnc = dolist[cnt];
      if ( chrbmpdata[tnc].shadow != 0 || capforceshadow[chrmodel[tnc]] && mesh_has_no_bits( chronwhichfan[tnc], MESHFX_SHINY ) )
        render_shadow( tnc );
    }
  }
  ATTRIB_POP( "render_good_shadows" );
}

//--------------------------------------------------------------------------------------------
//void render_bad_shadows()
//{
//  int cnt, tnc;
//
//  // Bad shadows
//  ATTRIB_PUSH("render_bad_shadows", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
//  {
//    glDepthMask(GL_FALSE);
//
//    glEnable(GL_ALPHA_TEST);
//    glAlphaFunc(GL_GREATER, 0);
//
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      tnc = dolist[cnt];
//      //if(chrattachedto[tnc] == MAXCHR)
//      //{
//      if (chrbmpdata[tnc].calc_shadowsize != 0 || capforceshadow[chrmodel[tnc]] && HAS_NO_BITS(meshfx[chronwhichfan[tnc]], MESHFX_SHINY))
//        render_bad_shadow(tnc);
//      //}
//    }
//  }
//  ATTRIB_POP("render_bad_shadows");
//}

//--------------------------------------------------------------------------------------------
void render_character_reflections()
{
  int cnt, tnc;

  // Render reflections of characters
  ATTRIB_PUSH( "render_character_reflections", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT );
  {
    glDepthMask( GL_TRUE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CCW );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
      tnc = dolist[cnt];
      if ( mesh_has_some_bits( chronwhichfan[tnc], MESHFX_SHINY ) )
        render_refmad( tnc, chralpha_fp8[tnc] / 2 );
    }
  }
  ATTRIB_POP( "render_character_reflections" );

};

//--------------------------------------------------------------------------------------------
void render_normal_fans()
{
  int cnt, tnc, fan, texture;

  ATTRIB_PUSH( "render_normal_fans", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
  {
    // depth buffer stuff
    glDepthMask( GL_TRUE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    // CData.shading stuff
    glShadeModel( CData.shading );

    // alpha stuff
    glDisable( GL_BLEND );
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    // backface culling
    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW );
    glCullFace( GL_BACK );

    for ( cnt = 0; cnt < 4; cnt++ )
    {
      texture = cnt + 1;
      meshlasttexture = texture;
      GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
      for ( tnc = 0; tnc < numrenderlist_norm; tnc++ )
      {
        fan = renderlist_norm[tnc];
        render_fan( fan, texture );
      };
    }
  }
  ATTRIB_POP( "render_normal_fans" );
};

//--------------------------------------------------------------------------------------------
void render_shiny_fans()
{
  int cnt, tnc, fan, texture;

  ATTRIB_PUSH( "render_shiny_fans", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
  {
    // depth buffer stuff
    glDepthMask( GL_TRUE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    // CData.shading stuff
    glShadeModel( CData.shading );

    // alpha stuff
    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    // backface culling
    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW );
    glCullFace( GL_BACK );

    for ( cnt = 0; cnt < 4; cnt++ )
    {
      texture = cnt + 1;
      meshlasttexture = texture;
      GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
      for ( tnc = 0; tnc < numrenderlist_shine; tnc++ )
      {
        fan = renderlist_shine[tnc];
        render_fan( fan, texture );
      };
    }
  }
  ATTRIB_POP( "render_shiny_fans" );
};


//--------------------------------------------------------------------------------------------
//void render_reflected_fans()
//{
//  int cnt, tnc, fan, texture;
//
//  // Render the shadow floors
//  ATTRIB_PUSH("render_reflected_fans", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);
//  {
//    // depth buffer stuff
//    glDepthMask(GL_TRUE);
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LEQUAL);
//
//    // CData.shading stuff
//    glShadeModel(CData.shading);
//
//    // alpha stuff
//    glDisable(GL_BLEND);
//    glEnable(GL_ALPHA_TEST);
//    glAlphaFunc(GL_GREATER, 0);
//
//    // backface culling
//    glEnable(GL_CULL_FACE);
//    glFrontFace(GL_CW);
//    glCullFace(GL_BACK);
//
//    for (cnt = 0; cnt < 4; cnt++)
//    {
//      texture = cnt + 1;
//      meshlasttexture = texture;
//      GLTexture_Bind(&TxTexture[texture], CData.texturefilter);
//      for (tnc = 0; tnc < numrenderlist_reflc; tnc++)
//      {
//        fan = renderlist_reflc[tnc];
//        render_fan(fan, texture);
//      };
//    }
//  }
//  ATTRIB_POP("render_reflected_fans");
//};
//
//--------------------------------------------------------------------------------------------
void render_reflected_fans_ref()
{
  int cnt, tnc, fan, texture;

  ATTRIB_PUSH( "render_reflected_fans_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
  {
    // depth buffer stuff
    glDepthMask( GL_TRUE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    // CData.shading stuff
    glShadeModel( CData.shading );

    // alpha stuff
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    // backface culling
    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CCW );
    glCullFace( GL_BACK );

    for ( cnt = 0; cnt < 4; cnt++ )
    {
      texture = cnt + 1;
      meshlasttexture = texture;
      GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
      for ( tnc = 0; tnc < numrenderlist_shine; tnc++ )
      {
        fan = renderlist_reflc[tnc];
        render_fan_ref( fan, texture, camtracklevel );
      };
    }
  }
  ATTRIB_POP( "render_reflected_fans_ref" );
};

//--------------------------------------------------------------------------------------------
//void render_sha_fans_ref()
//{
//  int cnt, tnc, fan, texture;
//
//  // Render the shadow floors
//  ATTRIB_PUSH("render_sha_fans_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);
//  {
//    // depth buffer stuff
//    glDepthMask(GL_TRUE);
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LEQUAL);
//
//    // CData.shading stuff
//    glShadeModel(CData.shading);
//
//    // alpha stuff
//    glEnable(GL_ALPHA_TEST);
//    glAlphaFunc(GL_GREATER, 0);
//
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_ONE, GL_ONE);
//    glEnable(GL_ALPHA_TEST);
//    glAlphaFunc(GL_GREATER, 0);
//
//    // backface culling
//    glEnable(GL_CULL_FACE);
//    glFrontFace(GL_CCW);
//    glCullFace(GL_BACK);
//
//    for (cnt = 0; cnt < 4; cnt++)
//    {
//      texture = cnt + 1;
//      meshlasttexture = texture;
//      GLTexture_Bind(&TxTexture[texture], CData.texturefilter);
//      for (tnc = 0; tnc < numrenderlist_reflc; tnc++)
//      {
//        fan = renderlist_reflc[tnc];
//        render_fan_ref(fan, texture, camtracklevel);
//      };
//    }
//
//  }
//  ATTRIB_POP("render_sha_fans_ref");
//};
//
//--------------------------------------------------------------------------------------------
void render_solid_characters()
{
  int cnt, tnc;

  // Render the normal characters
  ATTRIB_PUSH( "render_solid_characters", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT );
  {
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );
    glFrontFace( GL_CW );

    // must be here even for solid characters because of stuff like the spikemace
    // and a lot of other items that just use a simple texturemapped plane with
    // transparency for their shape.  Maybe it should be converted to blend?
    glDisable( GL_BLEND );
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glDepthMask( GL_TRUE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
      tnc = dolist[cnt];
      if ( chralpha_fp8[tnc] == 255 && chrlight_fp8[tnc] == 255 )
        render_mad( tnc, 255 );
    }
  }
  ATTRIB_POP( "render_solid_characters" );

};

//--------------------------------------------------------------------------------------------
void render_alpha_characters()
{
  int cnt, tnc;
  Uint8 trans;

  // Now render the transparent characters
  ATTRIB_PUSH( "render_alpha_characters", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT );
  {
    glDepthMask( GL_FALSE );

    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW );
    glCullFace( GL_BACK );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_COLOR );

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
      tnc = dolist[cnt];
      if ( chralpha_fp8[tnc] != 255 )
      {
        trans = chralpha_fp8[tnc];

        if (( chralpha_fp8[tnc] + chrlight_fp8[tnc] ) < SEEINVISIBLE &&  localseeinvisible && chrislocalplayer[tnc] )
          trans = SEEINVISIBLE - chrlight_fp8[tnc];

        if ( trans > 0 )
        {
          render_mad( tnc, trans );
        };
      }
    }

  }
  ATTRIB_POP( "render_alpha_characters" );

};

//--------------------------------------------------------------------------------------------
// render the water hilights, etc. using global lighting
void render_water_highlights()
{
  ATTRIB_PUSH( "render_water_highlights", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT );
  {
    GLfloat light_position[] = { 10000*lightspekdir.x, 10000*lightspekdir.y, 10000*lightspekdir.z, 1.0 };
    GLfloat lmodel_ambient[] = { lightambicol.r, lightambicol.g, lightambicol.b, 1.0 };
    GLfloat light_diffuse[]  = { lightspekcol.r, lightspekcol.g, lightspekcol.b, 1.0 };

    glDepthMask( GL_FALSE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );

    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );

    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lmodel_ambient );

    //glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv( GL_LIGHT0, GL_DIFFUSE, light_diffuse );
    glLightfv( GL_LIGHT0, GL_SPECULAR, light_diffuse );
    glLightfv( GL_LIGHT0, GL_POSITION, light_position );

    render_water_lit();

    glDisable( GL_LIGHT0 );
    glDisable( GL_LIGHTING );
  }
  ATTRIB_POP( "render_water_highlights" );
};

//--------------------------------------------------------------------------------------------
void render_alpha_water()
{

  ATTRIB_PUSH( "render_alpha_water", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT | GL_POLYGON_BIT );
  {
    glDisable( GL_CULL_FACE );
    glCullFace( GL_BACK );
    glFrontFace( GL_CW );

    glDepthMask( GL_FALSE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    render_water();
  }
  ATTRIB_POP( "render_alpha_water" );
};

//--------------------------------------------------------------------------------------------
void render_light_water()
{
  ATTRIB_PUSH( "render_light_water", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT );
  {
    glDisable( GL_CULL_FACE );
    glCullFace( GL_BACK );
    glFrontFace( GL_CW );

    glDepthMask( GL_FALSE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );
    render_water();
  }
  ATTRIB_POP( "render_light_water" );
};

//--------------------------------------------------------------------------------------------
void render_character_highlights()
{
  int cnt, tnc;

  ATTRIB_PUSH( "render_character_highlights", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT );
  {
    GLfloat light_none[]     = {0, 0, 0, 0};
    GLfloat light_position[] = { 10000*lightspekdir.x, 10000*lightspekdir.y, 10000*lightspekdir.z, 1.0 };
    GLfloat lmodel_ambient[] = { lightambicol.r, lightambicol.g, lightambicol.b, 1.0 };
    GLfloat light_specular[] = { lightspekcol.r, lightspekcol.g, lightspekcol.b, 1.0 };

    glDisable( GL_CULL_FACE );

    glDepthMask( GL_FALSE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_LESS, 1 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );

    glLightfv( GL_LIGHT1, GL_AMBIENT,  light_none );
    glLightfv( GL_LIGHT1, GL_DIFFUSE,  light_none );
    glLightfv( GL_LIGHT1, GL_SPECULAR, light_specular );
    glLightfv( GL_LIGHT1, GL_POSITION, light_position );

    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, light_none );
    glLightModeli( GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE );
    glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE );

    glEnable( GL_LIGHTING );
    glDisable( GL_LIGHT0 );
    glEnable( GL_LIGHT1 );

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
      tnc = dolist[cnt];

      if ( chrsheen_fp8[tnc] == 0 && chrlight_fp8[tnc] == 255 && chralpha_fp8[tnc] == 255 ) continue;

      render_mad_lit( tnc );
    }

    glDisable( GL_LIGHT0 );
    glDisable( GL_LIGHTING );
  }
  ATTRIB_POP( "render_character_highlights" );
};

GLint inp_attrib_stack, out_attrib_stack;
void draw_scene_zreflection()
{
  // ZZ> This function draws 3D objects
  // do all the rendering of reflections
  if ( CData.refon )
  {
    ATTRIB_GUARD_OPEN( inp_attrib_stack );
    render_reflected_fans_ref();
    ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

    ATTRIB_GUARD_OPEN( inp_attrib_stack );
    //render_sha_fans_ref();
    ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

    ATTRIB_GUARD_OPEN( inp_attrib_stack );
    render_character_reflections();
    ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

    ATTRIB_GUARD_OPEN( inp_attrib_stack );
    render_particle_reflections();
    ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );
  }

  //---- render the non-reflective fans ----
  {
    ATTRIB_GUARD_OPEN( inp_attrib_stack );
    render_normal_fans();
    ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

    ATTRIB_GUARD_OPEN( inp_attrib_stack );
    render_shiny_fans();
    ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

    ATTRIB_GUARD_OPEN( inp_attrib_stack );
    //render_reflected_fans();
    ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );
  }

  // Render the shadows
  if ( CData.shaon )
  {
    if ( CData.shasprite )
    {
      ATTRIB_GUARD_OPEN( inp_attrib_stack );
      //render_bad_shadows();
      ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );
    }
    else
    {
      ATTRIB_GUARD_OPEN( inp_attrib_stack );
      render_good_shadows();
      ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );
    }
  }


  ATTRIB_GUARD_OPEN( inp_attrib_stack );
  render_solid_characters();
  ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

  //---- Render transparent objects ----
  {
    render_alpha_characters();

    // And alpha water
    if ( !waterlight )
    {
      ATTRIB_GUARD_OPEN( inp_attrib_stack );
      render_alpha_water();
      ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );
    };

    // Do self-lit water
    if ( waterlight )
    {
      ATTRIB_GUARD_OPEN( inp_attrib_stack );
      render_light_water();
      ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );
    };

  };

  // do highlights
  ATTRIB_GUARD_OPEN( inp_attrib_stack );
  render_character_highlights();
  ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

  ATTRIB_GUARD_OPEN( inp_attrib_stack );
  render_water_highlights();
  ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

  // render the sprites
  ATTRIB_GUARD_OPEN( inp_attrib_stack );
  render_particles();
  ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

  // render the collision volumes
#if defined(DEBUG_CVOLUME) && defined(_DEBUG)
  if(CData.DevMode)
  {
    cv_list_draw();
  };
#endif
};


//--------------------------------------------------------------------------------------------
//void draw_scene_zreflection()
//{
//  // ZZ> This function draws 3D objects
//  Uint16 cnt, tnc;
//  Uint8 trans;
//
//  // Clear the image if need be
//  // PORT: I don't think this is needed if(render_background) { clear_surface(lpDDSBack); }
//  // Zbuffer is cleared later
//
//  // Render the reflective floors
//  glDisable(GL_DEPTH_TEST);
//  glDepthMask(GL_FALSE);
//  glDisable(GL_BLEND);
//
//  // Renfer ref
//  glEnable(GL_ALPHA_TEST);
//  glAlphaFunc(GL_GREATER, 0);
//  meshlasttexture = 0;
//  for (cnt = 0; cnt < numrenderlist_shine; cnt++)
//    render_fan(renderlist_shine[cnt]);
//
//  // Renfer sha
//  // BAD: DRAW SHADOW STUFF TOO
//  glEnable(GL_ALPHA_TEST);
//  glAlphaFunc(GL_GREATER, 0);
//  for (cnt = 0; cnt < numrenderlist_reflc; cnt++)
//    render_fan(renderlist_reflc[cnt]);
//
//  glEnable(GL_DEPTH_TEST);
//  glDepthMask(GL_TRUE);
//  if(CData.refon)
//  {
//    // Render reflections of characters
//    glFrontFace(GL_CCW);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//    glDepthFunc(GL_LEQUAL);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      tnc = dolist[cnt];
//      if((meshfx[chronwhichfan[tnc]]&MESHFX_SHINY))
//        render_refmad(tnc, chralpha_fp8[tnc]&chrlight_fp8[tnc]);
//    }
//
//    // [claforte] I think this is wrong... I think we should choose some other depth func.
//    glDepthFunc(GL_ALWAYS);
//
//    // Render the reflected sprites
//    render_particle_reflections();
//  }
//
//  // Render the shadow floors
//  meshlasttexture = 0;
//
//  glEnable(GL_ALPHA_TEST);
//  glAlphaFunc(GL_GREATER, 0);
//  for (cnt = 0; cnt < numrenderlist_reflc; cnt++)
//    render_fan(renderlist_reflc[cnt]);
//
//  // Render the shadows
//  if (CData.shaon)
//  {
//    if (CData.shasprite)
//    {
//      // Bad shadows
//      glDepthMask(GL_FALSE);
//      glEnable(GL_BLEND);
//      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//      for (cnt = 0; cnt < numdolist; cnt++)
//      {
//        tnc = dolist[cnt];
//        if(chrattachedto[tnc] == MAXCHR)
//        {
//          if(((chrlight_fp8[tnc]==255 && chralpha_fp8[tnc]==255) || capforceshadow[chrmodel[tnc]]) && chrbmpdata[tnc].calc_shadowsize!=0)
//            render_bad_shadow(tnc);
//        }
//      }
//      glDisable(GL_BLEND);
//      glDepthMask(GL_TRUE);
//    }
//    else
//    {
//      // Good shadows for me
//      glDepthMask(GL_FALSE);
//      glDepthFunc(GL_LEQUAL);
//      glEnable(GL_BLEND);
//      glBlendFunc(GL_SRC_COLOR, GL_ZERO);
//
//      for (cnt = 0; cnt < numdolist; cnt++)
//      {
//        tnc = dolist[cnt];
//        if(chrattachedto[tnc] == MAXCHR)
//        {
//          if(((chrlight_fp8[tnc]==255 && chralpha_fp8[tnc]==255) || capforceshadow[chrmodel[tnc]]) && chrbmpdata[tnc].calc_shadowsize!=0)
//            render_shadow(tnc);
//        }
//      }
//
//      glDisable(GL_BLEND);
//      glDepthMask ( GL_TRUE );
//    }
//  }
//
//  // Render the normal characters
//  ATTRIB_PUSH("zref",GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
//  {
//    glDepthMask ( GL_TRUE );
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LESS);
//
//    glDisable(GL_BLEND);
//
//    glEnable(GL_ALPHA_TEST);
//    glAlphaFunc(GL_GREATER, 0);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      tnc = dolist[cnt];
//      if(chralpha_fp8[tnc]==255 && chrlight_fp8[tnc]==255)
//        render_mad(tnc, 255);
//    }
//  }
//  ATTRIB_POP("zref");
//
//  //// Render the sprites
//  glDepthMask ( GL_FALSE );
//  glEnable(GL_BLEND);
//
//  // Now render the transparent characters
//  glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
//  {
//    glDepthMask ( GL_FALSE );
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LEQUAL);
//
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      tnc = dolist[cnt];
//      if(chralpha_fp8[tnc]!=255 && chrlight_fp8[tnc]==255)
//      {
//        trans = chralpha_fp8[tnc];
//        if(trans < SEEINVISIBLE && (localseeinvisible || chrislocalplayer[tnc]))  trans = SEEINVISIBLE;
//        render_mad(tnc, trans);
//      }
//    }
//
//  }
//
//  // And alpha water floors
//  if(!waterlight)
//    render_water();
//
//  // Then do the light characters
//  glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
//  {
//    glDepthMask ( GL_FALSE );
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LEQUAL);
//
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      tnc = dolist[cnt];
//      if(chrlight_fp8[tnc]!=255)
//      {
//        trans = FP8_TO_FLOAT(FP8_MUL(chrlight_fp8[tnc], chralpha_fp8[tnc])) * 0.5f;
//        if(trans < SEEINVISIBLE && (localseeinvisible || chrislocalplayer[tnc]))  trans = SEEINVISIBLE;
//        render_mad(tnc, trans);
//      }
//    }
//  }
//
//  // Do phong highlights
//  if(CData.phongon && chrsheen_fp8[tnc] > 0)
//  {
//    Uint16 texturesave, envirosave;
//
//    ATTRIB_PUSH("zref", GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
//    {
//      glDepthMask ( GL_FALSE );
//      glEnable(GL_DEPTH_TEST);
//      glDepthFunc(GL_LEQUAL);
//
//      glEnable(GL_BLEND);
//      glBlendFunc(GL_ONE, GL_ONE);
//
//      envirosave = chrenviro[tnc];
//      texturesave = chrtexture[tnc];
//      chrenviro[tnc] = btrue;
//      chrtexture[tnc] = 7;  // The phong map texture...
//      render_enviromad(tnc, (chralpha_fp8[tnc] * spek_global[chrsheen_fp8[tnc]][chrlight_fp8[tnc]]) / 2, GL_TEXTURE_2D);
//      chrtexture[tnc] = texturesave;
//      chrenviro[tnc] = envirosave;
//    };
//    ATTRIB_POP("zref");
//  }
//
//
//  // Do light water
//  if(waterlight)
//    render_water();
//
//  // Turn Z buffer back on, alphablend off
//  render_particles();
//
//  // Done rendering
//};

bool_t draw_texture_box( GLtexture * ptx, FRect * tx_rect, FRect * sc_rect )
{
  FRect rtmp;

  if( NULL == sc_rect ) return bfalse;

  if(NULL != ptx)
  {
    GLTexture_Bind( ptx, CData.texturefilter );
  }

  if(NULL == tx_rect)
  {
    rtmp.left = 0; rtmp.right = 1;
    rtmp.top  = 0; rtmp.bottom = 1;
    tx_rect = &rtmp;
  };

  glBegin( GL_QUADS );
    glTexCoord2f( tx_rect->left,  tx_rect->bottom );   glVertex2f( sc_rect->left,  sc_rect->bottom );
    glTexCoord2f( tx_rect->right, tx_rect->bottom );   glVertex2f( sc_rect->right, sc_rect->bottom );
    glTexCoord2f( tx_rect->right, tx_rect->top    );   glVertex2f( sc_rect->right, sc_rect->top    );
    glTexCoord2f( tx_rect->left,  tx_rect->top    );   glVertex2f( sc_rect->left,  sc_rect->top    );
  glEnd();

  return btrue;
}

//--------------------------------------------------------------------------------------------
void draw_blip( COLR color, float x, float y)
{
  // ZZ> This function draws a blip

  FRect tx_rect, sc_rect;
  float width, height;

  width  = 3.0*mapscale*0.5f; 
  height = 3.0*mapscale*0.5f;

  if ( x < -width || x > CData.scrx + width || y < -height || y > CData.scry + height ) return;

  ATTRIB_PUSH( "draw_blip", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    // depth buffer stuff
    glDepthMask( GL_FALSE );

    // CData.shading stuff
    glShadeModel( GL_FLAT );

    // alpha stuff
    glDisable( GL_ALPHA_TEST );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // backface culling
    glDisable( GL_CULL_FACE );

    tx_rect.left   = (( float ) bliprect[color].left   ) / ( float ) GLTexture_GetTextureWidth(&TxBlip)  + 0.01;
    tx_rect.right  = (( float ) bliprect[color].right  ) / ( float ) GLTexture_GetTextureWidth(&TxBlip)  - 0.01;
    tx_rect.top    = (( float ) bliprect[color].top    ) / ( float ) GLTexture_GetTextureHeight(&TxBlip) + 0.01;
    tx_rect.bottom = (( float ) bliprect[color].bottom ) / ( float ) GLTexture_GetTextureHeight(&TxBlip) - 0.01;

    sc_rect.left   = x - width;
    sc_rect.right  = x + width;
    sc_rect.top    = CData.scry - ( y - height );
    sc_rect.bottom = CData.scry - ( y + height );

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    draw_texture_box(&TxBlip, &tx_rect, &sc_rect);
  }
  ATTRIB_POP( "draw_blip" );
}

//--------------------------------------------------------------------------------------------
void draw_one_icon( int icontype, int x, int y, Uint8 sparkle )
{
  // ZZ> This function draws an icon
  int position, blipx, blipy;
  int width, height;
  FRect tx_rect, sc_rect;

  if ( INVALID_TEXTURE == TxIcon[icontype].textureID ) return;

  ATTRIB_PUSH( "draw_one_icon", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    // depth buffer stuff
    glDepthMask( GL_FALSE );

    // CData.shading stuff
    glShadeModel( GL_FLAT );

    // alpha stuff
    glDisable( GL_ALPHA_TEST );
    glDisable( GL_BLEND );

    // backface culling
    glDisable( GL_CULL_FACE );

    tx_rect.left   = (( float ) iconrect.left   ) / GLTexture_GetTextureWidth(&TxIcon[icontype]);
    tx_rect.right  = (( float ) iconrect.right  ) / GLTexture_GetTextureWidth(&TxIcon[icontype]);
    tx_rect.top    = (( float ) iconrect.top    ) / GLTexture_GetTextureHeight(&TxIcon[icontype]);
    tx_rect.bottom = (( float ) iconrect.bottom ) / GLTexture_GetTextureHeight(&TxIcon[icontype]);

    width  = iconrect.right  - iconrect.left;
    height = iconrect.bottom - iconrect.top;

    sc_rect.left   = x;
    sc_rect.right  = x + width;
    sc_rect.top    = CData.scry - y;
    sc_rect.bottom = CData.scry - (y + height);

    draw_texture_box( &TxIcon[icontype], &tx_rect, &sc_rect);
  }
  ATTRIB_POP( "draw_one_icon" );

  if ( sparkle != NOSPARKLE )
  {
    position = wldframe & 31;
    position = ( SPARKLESIZE * position >> 5 );

    blipx = x + SPARKLEADD + position;
    blipy = y + SPARKLEADD;
    draw_blip( sparkle, blipx, blipy );

    blipx = x + SPARKLEADD + SPARKLESIZE;
    blipy = y + SPARKLEADD + position;
    draw_blip( sparkle, blipx, blipy );

    blipx -= position;
    blipy = y + SPARKLEADD + SPARKLESIZE;
    draw_blip( sparkle, blipx, blipy );

    blipx = x + SPARKLEADD;
    blipy -= position;
    draw_blip( sparkle, blipx, blipy );
  }
}

//--------------------------------------------------------------------------------------------
void draw_one_font( int fonttype, float x, float y )
{
  // ZZ> This function draws a letter or number
  // GAC> Very nasty version for starters.  Lots of room for improvement.
  GLfloat dx, dy;
  FRect tx_rect, sc_rect;

  dx = 2.0 / 512;
  dy = 1.0 / 256;

  tx_rect.left   = fontrect[fonttype].x * dx + 0.001f;
  tx_rect.right  = ( fontrect[fonttype].x + fontrect[fonttype].w ) * dx - 0.001f;
  tx_rect.top    = fontrect[fonttype].y * dy + 0.001f;
  tx_rect.bottom = ( fontrect[fonttype].y + fontrect[fonttype].h ) * dy;

  sc_rect.left   = x;
  sc_rect.right  = x + fontrect[fonttype].w;
  sc_rect.top    = CData.scry - y;
  sc_rect.bottom = CData.scry - (y + fontrect[fonttype].h);

  draw_texture_box(NULL, &tx_rect, &sc_rect);
}

//--------------------------------------------------------------------------------------------
void draw_map( float x, float y )
{
  // ZZ> This function draws the map

  FRect tx_rect, sc_rect;

  ATTRIB_PUSH( "draw_map", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
  {
    // depth buffer stuff
    glDepthMask( GL_FALSE );

    // CData.shading stuff
    glShadeModel( GL_FLAT );

    // alpha stuff
    glDisable( GL_ALPHA_TEST );
    glDisable( GL_BLEND );

    // backface culling
    glDisable( GL_CULL_FACE );

    tx_rect.left   = 0.0f;
    tx_rect.right  = (float)GLTexture_GetImageWidth(&TxMap) / (float)GLTexture_GetTextureWidth(&TxMap);
    tx_rect.top    = 0.0f;
    tx_rect.bottom = (float)GLTexture_GetImageHeight(&TxMap) / (float)GLTexture_GetTextureHeight(&TxMap);

    sc_rect.left   = x + maprect.left;
    sc_rect.right  = x + maprect.right;
    sc_rect.top    = maprect.bottom - y;
    sc_rect.bottom = maprect.top    - y;

    draw_texture_box( &TxMap, &tx_rect, &sc_rect );
  }
  ATTRIB_POP( "draw_map" );
}

//--------------------------------------------------------------------------------------------
int draw_one_bar( int bartype, int x, int y, int ticks, int maxticks )
{
  // ZZ> This function draws a bar and returns the y position for the next one
  int noticks;
  FRect tx_rect, sc_rect;
  int width, height;
  int ystt = y;

  ATTRIB_PUSH( "draw_one_bar", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    // depth buffer stuff
    glDepthMask( GL_FALSE );

    // CData.shading stuff
    glShadeModel( GL_FLAT );

    // alpha stuff
    glDisable( GL_ALPHA_TEST );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // backface culling
    glDisable( GL_CULL_FACE );

    if ( maxticks > 0 && ticks >= 0 )
    {
      // Draw the tab
      GLTexture_Bind( &TxBars, CData.texturefilter );
      tx_rect.left   = ( float ) tabrect[bartype].left   / ( float ) GLTexture_GetTextureWidth( &TxBars );
      tx_rect.right  = ( float ) tabrect[bartype].right  / ( float ) GLTexture_GetTextureWidth( &TxBars );
      tx_rect.top    = ( float ) tabrect[bartype].top    / ( float ) GLTexture_GetTextureHeight( &TxBars );
      tx_rect.bottom = ( float ) tabrect[bartype].bottom / ( float ) GLTexture_GetTextureHeight( &TxBars );

      width  = tabrect[bartype].right  - tabrect[bartype].left; 
      height = tabrect[bartype].bottom - tabrect[bartype].top;

      sc_rect.left   = x;
      sc_rect.right  = x + width;
      sc_rect.top    = CData.scry - y;
      sc_rect.bottom = CData.scry - (y + height);

      draw_texture_box(&TxBars, &tx_rect, &sc_rect);

      // Error check
      if ( maxticks > MAXTICK ) maxticks = MAXTICK;
      if ( ticks > maxticks ) ticks = maxticks;

      // Draw the full rows of ticks
      x += TABX;
      while ( ticks >= NUMTICK )
      {
        barrect[bartype].right = BARX;

        tx_rect.left   = ( float ) barrect[bartype].left   / ( float ) GLTexture_GetTextureWidth( &TxBars );
        tx_rect.right  = ( float ) barrect[bartype].right  / ( float ) GLTexture_GetTextureWidth( &TxBars );
        tx_rect.top    = ( float ) barrect[bartype].top    / ( float ) GLTexture_GetTextureHeight( &TxBars );
        tx_rect.bottom = ( float ) barrect[bartype].bottom / ( float ) GLTexture_GetTextureHeight( &TxBars );

        width  = barrect[bartype].right  - barrect[bartype].left; 
        height = barrect[bartype].bottom - barrect[bartype].top;

        sc_rect.left   = x;
        sc_rect.right  = x + width;
        sc_rect.top    = CData.scry - y;
        sc_rect.bottom = CData.scry - (y + height);

        draw_texture_box(&TxBars, &tx_rect, &sc_rect);

        y += BARY;
        ticks -= NUMTICK;
        maxticks -= NUMTICK;
      }


      // Draw any partial rows of ticks
      if ( maxticks > 0 )
      {
        // Draw the filled ones
        barrect[bartype].right = ( ticks << 3 ) + TABX;

        tx_rect.left   = ( float ) barrect[bartype].left   / ( float ) GLTexture_GetTextureWidth( &TxBars );
        tx_rect.right  = ( float ) barrect[bartype].right  / ( float ) GLTexture_GetTextureWidth( &TxBars );
        tx_rect.top    = ( float ) barrect[bartype].top    / ( float ) GLTexture_GetTextureHeight( &TxBars );
        tx_rect.bottom = ( float ) barrect[bartype].bottom / ( float ) GLTexture_GetTextureHeight( &TxBars );

        width  = barrect[bartype].right  - barrect[bartype].left; 
        height = barrect[bartype].bottom - barrect[bartype].top;

        sc_rect.left   = x;
        sc_rect.right  = x + width;
        sc_rect.top    = CData.scry - y;
        sc_rect.bottom = CData.scry - (y + height);

        draw_texture_box(&TxBars, &tx_rect, &sc_rect);

        // Draw the empty ones
        noticks = maxticks - ticks;
        if ( noticks > ( NUMTICK - ticks ) ) noticks = ( NUMTICK - ticks );
        barrect[0].right = ( noticks << 3 ) + TABX;

        tx_rect.left   = ( float ) barrect[0].left   / ( float ) GLTexture_GetTextureWidth( &TxBars );
        tx_rect.right  = ( float ) barrect[0].right  / ( float ) GLTexture_GetTextureWidth( &TxBars );
        tx_rect.top    = ( float ) barrect[0].top    / ( float ) GLTexture_GetTextureHeight( &TxBars );
        tx_rect.bottom = ( float ) barrect[0].bottom / ( float ) GLTexture_GetTextureHeight( &TxBars );

        width  = barrect[0].right  - barrect[0].left; 
        height = barrect[0].bottom - barrect[0].top;

        sc_rect.left   = x;
        sc_rect.right  = x + width;
        sc_rect.top    = CData.scry - y;
        sc_rect.bottom = CData.scry - (y + height);

        draw_texture_box(&TxBars, &tx_rect, &sc_rect);

        maxticks -= NUMTICK;
        y += BARY;
      }


      // Draw full rows of empty ticks
      while ( maxticks >= NUMTICK )
      {
        barrect[0].right = BARX;

        tx_rect.left   = ( float ) barrect[0].left   / ( float ) GLTexture_GetTextureWidth( &TxBars );
        tx_rect.right  = ( float ) barrect[0].right  / ( float ) GLTexture_GetTextureWidth( &TxBars );
        tx_rect.top    = ( float ) barrect[0].top    / ( float ) GLTexture_GetTextureHeight( &TxBars );
        tx_rect.bottom = ( float ) barrect[0].bottom / ( float ) GLTexture_GetTextureHeight( &TxBars );

        width  = barrect[0].right  - barrect[0].left; 
        height = barrect[0].bottom - barrect[0].top;

        sc_rect.left   = x;
        sc_rect.right  = x + width;
        sc_rect.top    = CData.scry - y;
        sc_rect.bottom = CData.scry - (y + height);

        draw_texture_box(&TxBars, &tx_rect, &sc_rect);

        y += BARY;
        maxticks -= NUMTICK;
      }


      // Draw the last of the empty ones
      if ( maxticks > 0 )
      {
        barrect[0].right = ( maxticks << 3 ) + TABX;

        tx_rect.left   = ( float ) barrect[0].left   / ( float ) GLTexture_GetTextureWidth( &TxBars );
        tx_rect.right  = ( float ) barrect[0].right  / ( float ) GLTexture_GetTextureWidth( &TxBars );
        tx_rect.top    = ( float ) barrect[0].top    / ( float ) GLTexture_GetTextureHeight( &TxBars );
        tx_rect.bottom = ( float ) barrect[0].bottom / ( float ) GLTexture_GetTextureHeight( &TxBars );

        width  = barrect[0].right  - barrect[0].left; 
        height = barrect[0].bottom - barrect[0].top;

        sc_rect.left   = x;
        sc_rect.right  = x + width;
        sc_rect.top    = CData.scry - y;
        sc_rect.bottom = CData.scry - (y + height);

        draw_texture_box(&TxBars, &tx_rect, &sc_rect);

        y += BARY;
      }
    }
  }
  ATTRIB_POP( "draw_one_bar" );

  return y - ystt;;
}

//--------------------------------------------------------------------------------------------
int draw_string( GLtexture * pfnt, float x, float y, GLfloat tint[], char * szFormat, ... )
{
  // ZZ> This function spits a line of null terminated text onto the backbuffer
  char cTmp;
  GLfloat current_tint[4], temp_tint[4];
  STRING szText;
  va_list args;
  int cnt = 1;

  if(NULL==pfnt || NULL == szFormat) return 0;

  // write the string to the buffer
  va_start( args, szFormat );
  vsnprintf( szText, sizeof(STRING), szFormat, args );
  va_end( args );

  // set the tint of the text
  glGetFloatv(GL_CURRENT_COLOR, current_tint); 

  BeginText( pfnt );
  {
    if(NULL != tint)
    {
      temp_tint[0] = current_tint[0] * tint[0];
      temp_tint[1] = current_tint[1] * tint[1];
      temp_tint[2] = current_tint[2] * tint[2];
      temp_tint[3] = current_tint[3];
      glColor4fv(temp_tint);
    }

    cTmp = szText[0];
    while ( cTmp != 0 )
    {
      // Convert ASCII to our own little font
      if ( cTmp == '~' )
      {
        // Use squiggle for tab
        x = ((int)(x)) | TABAND;
      }
      else if ( cTmp == '\n' )
      {
        break;

      }
      else
      {
        // Normal letter
        cTmp = asciitofont[cTmp];
        draw_one_font( cTmp, x, y );
        x += fontxspacing[cTmp];
      }
      cTmp = szText[cnt];
      cnt++;
    }
  }
  EndText();

  // restore the old tint
  glColor4fv(current_tint); 

  return fontyspacing;
}

//--------------------------------------------------------------------------------------------
int length_of_word( char *szText )
{
  // ZZ> This function returns the number of pixels the
  //     next word will take on screen in the x direction

  // Count all preceeding spaces
  int x = 0;
  int cnt = 0;
  Uint8 cTmp = szText[cnt];

  while ( cTmp == ' ' || cTmp == '~' || cTmp == '\n' )
  {
    if ( cTmp == ' ' )
    {
      x += fontxspacing[asciitofont[cTmp]];
    }
    else if ( cTmp == '~' )
    {
      x += TABAND + 1;
    }
    cnt++;
    cTmp = szText[cnt];
  }


  while ( cTmp != ' ' && cTmp != '~' && cTmp != '\n' && cTmp != 0 )
  {
    x += fontxspacing[asciitofont[cTmp]];
    cnt++;
    cTmp = szText[cnt];
  }
  return x;
}

//--------------------------------------------------------------------------------------------
int draw_wrap_string( GLtexture * pfnt, float x, float y, GLfloat tint[], float maxx, char * szFormat, ... )
{
  // ZZ> This function spits a line of null terminated text onto the backbuffer,
  //     wrapping over the right side and returning the new y value

  va_list args;
  STRING szText;
  int xstt, ystt, newy, cnt;
  char cTmp;
  bool_t newword;
  GLfloat current_tint[4], temp_tint[4];
  
  
  // write the string to the buffer
  va_start( args, szFormat );
  vsnprintf( szText, sizeof(STRING), szFormat, args );
  va_end( args );

  // set the tint of the text
  glGetFloatv(GL_CURRENT_COLOR, current_tint); 
  if(NULL != tint)
  {
    temp_tint[0] = 1 - (1.0f - current_tint[0])*(1.0f - tint[0]);
    temp_tint[1] = 1 - (1.0f - current_tint[1])*(1.0f - tint[1]);
    temp_tint[2] = 1 - (1.0f - current_tint[2])*(1.0f - tint[2]);
    temp_tint[3] = current_tint[3];
    glColor4fv(tint);
  }

  BeginText( pfnt );
  {
    newword = btrue;
    xstt = x;
    ystt = y;
    maxx += xstt;
    newy = y + fontyspacing;

    cnt = 1;
    cTmp = szText[0];
    while ( cTmp != 0 )
    {
      // Check each new word for wrapping
      if ( newword )
      {
        int endx = x + length_of_word( szText + cnt - 1 );

        newword = bfalse;
        if ( endx > maxx )
        {
          // Wrap the end and cut off spaces and tabs
          x = xstt + fontyspacing;
          y += fontyspacing;
          newy += fontyspacing;
          while ( cTmp == ' ' || cTmp == '~' )
          {
            cTmp = szText[cnt];
            cnt++;
          }
        }
      }
      else
      {
        if ( cTmp == '~' )
        {
          // Use squiggle for tab
          x = ((int)(x)) | TABAND;
        }
        else if ( cTmp == '\n' )
        {
          x = xstt;
          y += fontyspacing;
          newy += fontyspacing;
        }
        else
        {
          // Normal letter
          cTmp = asciitofont[cTmp];
          draw_one_font( cTmp, x, y );
          x += fontxspacing[cTmp];
        }
        cTmp = szText[cnt];
        if ( cTmp == '~' || cTmp == ' ' )
        {
          newword = btrue;
        }
        cnt++;
      }
    }
  }
  EndText();

  // restore the old tint
  glColor4fv(current_tint); 

  return newy-ystt;
}

//--------------------------------------------------------------------------------------------
int draw_status( GLtexture * pfnt, CHR_REF character, int x, int y )
{
  // ZZ> This function shows a character's icon, status and inventory
  //     The x,y coordinates are the top left point of the image to draw
  Uint16 item;
  char cTmp;
  char *readtext;
  int ystt = y;

  float life    = FP8_TO_FLOAT( chrlife_fp8[character] );
  float lifemax = FP8_TO_FLOAT( chrlifemax_fp8[character] );
  float mana    = FP8_TO_FLOAT( chrmana_fp8[character] );
  float manamax = FP8_TO_FLOAT( chrmanamax_fp8[character] );
  int cnt;

  // Write the character's first name
  if ( chrnameknown[character] )
    readtext = chrname[character];
  else
    readtext = capclassname[chrmodel[character]];

  for ( cnt = 0; cnt < 6; cnt++ )
  {
    cTmp = readtext[cnt];
    if ( cTmp == ' ' || cTmp == 0 )
    {
      generictext[cnt] = 0;
      break;

    }
    else
      generictext[cnt] = cTmp;
  }
  generictext[6] = 0;
  y += draw_string( pfnt, x + 8, y, NULL, generictext );


  // Write the character's money
  y += 8 + draw_string( pfnt, x + 8, y, NULL, "$%4d", chrmoney[character] );


  // Draw the icons
  draw_one_icon( madskintoicon[chrtexture[character]], x + 40, y, chrsparkle[character] );
  item = chr_get_holdingwhich( character, SLOT_LEFT );
  if ( VALID_CHR( item ) )
  {
    if ( chricon[item] )
    {
      draw_one_icon( madskintoicon[chrtexture[item]], x + 8, y, chrsparkle[item] );
      if ( chrammomax[item] != 0 && chrammoknown[item] )
      {
        if ( !capisstackable[chrmodel[item]] || chrammo[item] > 1 )
        {
          // Show amount of ammo left
          draw_string( pfnt, x + 8, y - 8, NULL, "%2d", chrammo[item] );
        }
      }
    }
    else
      draw_one_icon( bookicon + ( chrmoney[item] % MAXSKIN ), x + 8, y, chrsparkle[item] );
  }
  else
    draw_one_icon( nullicon, x + 8, y, NOSPARKLE );

  item = chr_get_holdingwhich( character, SLOT_RIGHT );
  if ( VALID_CHR( item ) )
  {
    if ( chricon[item] )
    {
      draw_one_icon( madskintoicon[chrtexture[item]], x + 72, y, chrsparkle[item] );
      if ( chrammomax[item] != 0 && chrammoknown[item] )
      {
        if ( !capisstackable[chrmodel[item]] || chrammo[item] > 1 )
        {
          // Show amount of ammo left
          draw_string( pfnt, x + 72, y - 8, NULL, "%2d", chrammo[item] );
        }
      }
    }
    else
      draw_one_icon( bookicon + ( chrmoney[item] % MAXSKIN ), x + 72, y, chrsparkle[item] );
  }
  else
    draw_one_icon( nullicon, x + 72, y, NOSPARKLE );

  y += 32;

  // Draw the bars
  if ( chralive[character] )
  {
    y += draw_one_bar( chrlifecolor[character], x, y, life, lifemax );
    y += draw_one_bar( chrmanacolor[character], x, y, mana, manamax );
  }
  else
  {
    y += draw_one_bar( 0, x, y, 0, lifemax ); // Draw a black bar
    y += draw_one_bar( 0, x, y, 0, manamax ); // Draw a black bar
  };

   
  return y - ystt;
}

//--------------------------------------------------------------------------------------------
bool_t do_map()
{
  PLA_REF ipla;
  CHR_REF ichr;
  int cnt;

  if(!mapon) return bfalse;

  draw_map( maprect.left, maprect.top );

  for ( cnt = 0; cnt < numblip; cnt++ )
  {
    draw_blip( blipc[cnt], maprect.left + blipx[cnt], maprect.top + blipy[cnt] );
  };

  if ( youarehereon && ( wldframe&8 ) )
  {
    for ( ipla = 0; ipla < MAXPLAYER; ipla++ )
    {
      if ( !VALID_PLA( ipla ) || INBITS_NONE == pladevice[ipla] ) continue;

      ichr = pla_get_character( ipla );
      if ( !VALID_CHR( ichr ) || !chralive[ichr] ) continue;

      draw_blip( 0, maprect.left + MAPSIZE * mesh_fraction_x( chrpos[ichr].x ) * mapscale, maprect.top + MAPSIZE * mesh_fraction_y( chrpos[ichr].y ) * mapscale );
    }
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
int do_messages( GLtexture * pfnt, int x, int y )
{
  int cnt, tnc;
  int ystt = y;

  if ( NULL==pfnt || !CData.messageon ) return 0;

  // Display the messages
  tnc = msgstart;
  for ( cnt = 0; cnt < CData.maxmessage; cnt++ )
  {
    // mesages with negative times never time out!
    if ( msgtime[tnc] != 0 )
    {
      y += draw_wrap_string( pfnt, x, y, NULL, CData.scrx - CData.wraptolerance - x, msgtextdisplay[tnc] );

      if ( msgtime[tnc] > 0 )
      {
        msgtime[tnc] -= msgtimechange;
        if ( msgtime[tnc] < 0 ) msgtime[tnc] = 0;
      };
    }
    tnc++;
    tnc %= CData.maxmessage;
  }

  return y - ystt;
}

//--------------------------------------------------------------------------------------------
int do_status( GLtexture * pfnt, int x, int y)
{
  int cnt;
  int ystt = y;
    
  if ( !CData.staton ) return 0;


  for ( cnt = 0; cnt < numstat && y < CData.scry; cnt++ )
  {
    y += draw_status( pfnt, statlist[cnt], x, y );
  };

  return y - ystt;
};

//--------------------------------------------------------------------------------------------
void draw_text( GLtexture * pfnt )
{
  // ZZ> This function spits out some words
  char text[512];
  int y, fifties, seconds, minutes;


  
  Begin2DMode();
  {
    // Status bars
    y = 0;
    y += do_status( pfnt, CData.scrx - BARX, y);

    // Map display
    do_map();


    y = 0;
    if ( outofsync )
    {
      y += draw_string( pfnt, 0, y, NULL, "OUT OF SYNC!" );;
    }

    if ( parseerror && CData.DevMode )
    {
      y += draw_string( pfnt, 0, y, NULL, "SCRIPT ERROR ( SEE LOG.TXT )" );
    }

    if ( CData.fpson )
    {
      CHR_REF pla_chr = pla_get_character( 0 );

      y += draw_string( pfnt, 0, y, NULL, "%2.3f FPS, %2.3f UPS", stabilized_fps, stabilized_ups );

      if( CData.DevMode )
      {
        y += draw_string( pfnt, 0, y, NULL, "wldframe %d, wldclock %d, allclock %d", wldframe, wldclock, allclock );
        y += draw_string( pfnt, 0, y, NULL, "<%3.2f,%3.2f,%3.2f>", chrpos[pla_chr].x, chrpos[pla_chr].y, chrpos[pla_chr].z );
        y += draw_string( pfnt, 0, y, NULL, "<%3.2f,%3.2f,%3.2f>", chrvel[pla_chr].x, chrvel[pla_chr].y, chrvel[pla_chr].z );
      }
    }

    {
      CHR_REF ichr, iref;
      GLvector tint = {0.5, 1.0, 1.0, 1.0};
      
      ichr = pla_get_character( 0 );
      if( VALID_CHR( ichr) )
      {
        iref = chr_get_attachedto(ichr);
        if( VALID_CHR(iref) )
        {
          y += draw_string( pfnt, 0, y, tint.v, "PLA0 holder == %s(%s)", chrname[iref], capclassname[chrmodel[iref]] );
        };

        iref = chr_get_inwhichpack(ichr);
        if( VALID_CHR(iref) )
        {
          y += draw_string( pfnt, 0, y, tint.v, "PLA0 packer == %s(%s)", chrname[iref], capclassname[chrmodel[iref]] );
        };

        iref = chr_get_onwhichplatform(ichr);
        if( VALID_CHR(iref) )
        {
          y += draw_string( pfnt, 0, y, tint.v, "PLA0 platform == %s(%s)", chrname[iref], capclassname[chrmodel[iref]] );
        };

      };
    }

    if ( SDLKEYDOWN( SDLK_F1 ) )
    {
      // In-Game help
      y += draw_string( pfnt, 0, y, NULL, "!!!MOUSE HELP!!!" );;
      y += draw_string( pfnt, 0, y, NULL, "~Edit CONTROLS.TXT to change" );;
      y += draw_string( pfnt, 0, y, NULL, "~Left Click to use an item" );;
      y += draw_string( pfnt, 0, y, NULL, "~Left and Right Click to grab" );;
      y += draw_string( pfnt, 0, y, NULL, "~Middle Click to jump" );;
      y += draw_string( pfnt, 0, y, NULL, "~A and S keys do stuff" );;
      y += draw_string( pfnt, 0, y, NULL, "~Right Drag to move camera" );;
    }

    if ( SDLKEYDOWN( SDLK_F2 ) )
    {
      // In-Game help
      y += draw_string( pfnt, 0, y, NULL, "!!!JOYSTICK HELP!!!" );;
      y += draw_string( pfnt, 0, y, NULL, "~Edit CONTROLS.TXT to change" );;
      y += draw_string( pfnt, 0, y, NULL, "~Hit the buttons" );;
      y += draw_string( pfnt, 0, y, NULL, "~You'll figure it out" );;
    }

    if ( SDLKEYDOWN( SDLK_F3 ) )
    {
      // In-Game help
      y += draw_string( pfnt, 0, y, NULL, "!!!KEYBOARD HELP!!!" );;
      y += draw_string( pfnt, 0, y, NULL, "~Edit CONTROLS.TXT to change" );;
      y += draw_string( pfnt, 0, y, NULL, "~TGB controls left hand" );;
      y += draw_string( pfnt, 0, y, NULL, "~YHN controls right hand" );;
      y += draw_string( pfnt, 0, y, NULL, "~Keypad to move and jump" );;
      y += draw_string( pfnt, 0, y, NULL, "~Number keys for stats" );;
    }


    // PLAYER DEBUG MODE
    if ( SDLKEYDOWN( SDLK_F5 ) && CData.DevMode )
    {
      CHR_REF pla_chr = pla_get_character( 0 );

      y += draw_string( pfnt, 0, y, NULL, "!!!DEBUG MODE-5!!!" ); 
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f", campos.x, campos.y );

      y += draw_string( pfnt, 0, y, NULL, "  PLA0DEF %d %d %d %d %d %d %d %d",
                chrdamagemodifier_fp8[pla_chr][0]&DAMAGE_SHIFT,
                chrdamagemodifier_fp8[pla_chr][1]&DAMAGE_SHIFT,
                chrdamagemodifier_fp8[pla_chr][2]&DAMAGE_SHIFT,
                chrdamagemodifier_fp8[pla_chr][3]&DAMAGE_SHIFT,
                chrdamagemodifier_fp8[pla_chr][4]&DAMAGE_SHIFT,
                chrdamagemodifier_fp8[pla_chr][5]&DAMAGE_SHIFT,
                chrdamagemodifier_fp8[pla_chr][6]&DAMAGE_SHIFT,
                chrdamagemodifier_fp8[pla_chr][7]&DAMAGE_SHIFT );

      y += draw_string( pfnt, 0, y, NULL, "~PLA0 %5.1f %5.1f", chrpos[pla_chr].x / 128.0, chrpos[pla_chr].y / 128.0  );

      pla_chr = pla_get_character( 1 );
      y += draw_string( pfnt, 0, y, NULL, "~PLA1 %5.1f %5.1f", chrpos[pla_chr].x / 128.0, chrpos[pla_chr].y / 128.0 );
    }


    // GLOBAL DEBUG MODE
    if ( SDLKEYDOWN( SDLK_F6 ) && CData.DevMode )
    {
      // More debug information
      y += draw_string( pfnt, 0, y, NULL, "!!!DEBUG MODE-6!!!" );
      y += draw_string( pfnt, 0, y, NULL, "~FREEPRT %d", numfreeprt );
      y += draw_string( pfnt, 0, y, NULL, "~FREECHR %d",  numfreechr );
      y += draw_string( pfnt, 0, y, NULL, "~MACHINE %d", localmachine );
      y += draw_string( pfnt, 0, y, NULL, "~EXPORT %d", exportvalid );
      y += draw_string( pfnt, 0, y, NULL, "~FOGAFF %d", fogaffectswater );
      y += draw_string( pfnt, 0, y, NULL, "~PASS %d/%d", numshoppassage, numpassage );
      y += draw_string( pfnt, 0, y, NULL, "~NETPLAYERS %d", numplayer );
      y += draw_string( pfnt, 0, y, NULL, "~DAMAGEPART %d", damagetileparttype );
    }

    // CAMERA DEBUG MODE
    if ( SDLKEYDOWN( SDLK_F7 ) && CData.DevMode )
    {
      // White debug mode
      y += draw_string( pfnt, 0, y, NULL, "!!!DEBUG MODE-7!!!" );
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f %f %f", ( mView ) _CNV( 0, 0 ), ( mView ) _CNV( 1, 0 ), ( mView ) _CNV( 2, 0 ), ( mView ) _CNV( 3, 0 ) );
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f %f %f", ( mView ) _CNV( 0, 1 ), ( mView ) _CNV( 1, 1 ), ( mView ) _CNV( 2, 1 ), ( mView ) _CNV( 3, 1 ) );
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f %f %f", ( mView ) _CNV( 0, 2 ), ( mView ) _CNV( 1, 2 ), ( mView ) _CNV( 2, 2 ), ( mView ) _CNV( 3, 2 ) );
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f %f %f", ( mView ) _CNV( 0, 3 ), ( mView ) _CNV( 1, 3 ), ( mView ) _CNV( 2, 3 ), ( mView ) _CNV( 3, 3 ) );
      y += draw_string( pfnt, 0, y, NULL, "~x %f, %f", camcenterpos.x, camtrackpos.x );
      y += draw_string( pfnt, 0, y, NULL, "~y %f %f", camcenterpos.y, camtrackpos.y );
      y += draw_string( pfnt, 0, y, NULL, "~turn %d %d", CData.autoturncamera, doturntime );
    }


    if( !ingameMenuActive && SDLKEYDOWN( SDLK_F9 ) && CData.DevMode )
    {
      ingameMenuActive = btrue;
      mnu_enterMenuMode();
    }



    //Draw paused text
    if ( gamepaused && !SDLKEYDOWN( SDLK_F11 ) )
    {
      snprintf( text, sizeof( text ), "GAME PAUSED" );
      draw_string( pfnt, -90 + CData.scrx / 2, 0 + CData.scry / 2, NULL, text  );
    }

    //Pressed panic button
    if ( SDLKEYDOWN( SDLK_q ) && SDLKEYDOWN( SDLK_LCTRL ) )
    {
      log_info( "User pressed escape button (LCTRL+Q)... Quitting game.\n" );
      gameActive = bfalse;
      moduleActive = bfalse;
    }

    // TIMER
    if ( timeron )
    {
      fifties = ( timervalue % 50 ) << 1;
      seconds = (( timervalue / 50 ) % 60 );
      minutes = ( timervalue / 3000 );
      y += draw_string( pfnt, 0, y, NULL, "=%d:%02d:%02d=", minutes, seconds, fifties );
    }

    // WAITING TEXT
    if ( waitingforplayers )
    {
      y += draw_string( pfnt, 0, y, NULL, "Waiting for players..." );
    }

    // MODULE EXIT TEXT
    if ( beatmodule )
    {
      y += draw_string( pfnt, 0, y, NULL, "VICTORY!  PRESS ESCAPE" );
    }
    else if ( alllocalpladead && !respawnvalid )
    {
      y += draw_string( pfnt, 0, y, NULL, "PRESS ESCAPE TO QUIT" );
    }
    else if (( alllocalpladead && respawnvalid ) || ( somelocalpladead && respawnanytime ) )
    {
      y += draw_string( pfnt, 0, y, NULL, "JUMP TO RESPAWN" );
    }



    // Network message input
    if ( netmessagemode )
    {
      y += draw_wrap_string( pfnt, 0, y, NULL, CData.scrx - CData.wraptolerance, netmessage );
    }


    // Messages
    y += do_messages( pfnt, 0, y );
  }
  End2DMode();
}

//--------------------------------------------------------------------------------------------
static bool_t pageflip_requested = bfalse;
static bool_t clear_requested    = btrue;
bool_t request_pageflip()
{
  bool_t retval = !pageflip_requested;
  pageflip_requested = btrue;

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t do_pageflip()
{
  bool_t retval = pageflip_requested;
  if ( pageflip_requested )
  {
    SDL_GL_SwapBuffers();
    allframe++;
    fps_loops++;
    pageflip_requested = bfalse;
    clear_requested    = btrue;
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t do_clear()
{
  bool_t retval = clear_requested;

  if ( clear_requested )
  {
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    clear_requested = bfalse;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
void draw_scene()
{
  Begin3DMode();

  make_prtlist();
  do_dynalight();
  light_characters();
  light_particles();

  // Render the background
  if ( CData.render_background )
  {
    render_background( 6 );   // 6 is the texture for waterlow.bmp
  }

  draw_scene_zreflection();

  //Foreground overlay
  if ( CData.render_overlay )
  {
    render_foreground_overlay( 5 );   // Texture 5 is watertop.bmp
  }

  End3DMode();
}

//--------------------------------------------------------------------------------------------
void draw_main( float frameDuration )
{
  // ZZ> This function does all the drawing stuff

  draw_scene();

  draw_text( &TxFont );

  request_pageflip();
}

//--------------------------------------------------------------------------------------------
bool_t load_one_title_image( int titleimage, char *szLoadName )
{
  // ZZ> This function loads a title in the specified image slot, forcing it into
  //     system memory.  Returns btrue if it worked

  return INVALID_TEXTURE != GLTexture_Load( GL_TEXTURE_2D,  &TxTitleImage[titleimage], szLoadName, INVALID_KEY );
}

//--------------------------------------------------------------------------------------------
void load_all_menu_images()
{
  // ZZ> This function loads the title image for each module.  Modules without a
  //     title are marked as invalid

  char searchname[15];
  STRING loadname;
  const char *FileName;
  FILE* filesave;

  // Convert searchname
  strcpy( searchname, "modules/*.mod" );

  // Log a directory list
  filesave = fs_fileOpen( PRI_NONE, NULL, CData.modules_file, "w" );
  if ( filesave != NULL )
  {
    fprintf( filesave, "This file logs all of the modules found\n" );
    fprintf( filesave, "** Denotes an invalid module (Or locked)\n\n" );
  }
  else log_warning( "Could not write to modules.txt\n" );

  // Search for .mod directories
  FileName = fs_findFirstFile( CData.modules_dir, "mod" );
  globalnummodule = 0;
  while ( FileName && globalnummodule < MAXMODULE )
  {
    strncpy( modloadname[globalnummodule], FileName, sizeof( modloadname[globalnummodule] ) );
    snprintf( loadname, sizeof( loadname ), "%s/%s/%s/%s", CData.modules_dir, FileName, CData.gamedat_dir, CData.mnu_file );
    if ( get_module_data( globalnummodule, loadname ) )
    {
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s/%s", CData.modules_dir, FileName, CData.gamedat_dir, CData.title_bitmap );
      if ( load_one_title_image( globalnummodule, CStringTmp1 ) )
      {
        fprintf( filesave, "%02d.  %s\n", globalnummodule, modlongname[globalnummodule] );
        globalnummodule++;
      }
      else
      {
        fprintf( filesave, "**.  %s\n", FileName );
      }
    }
    else
    {
      fprintf( filesave, "**.  %s\n", FileName );
    }
    FileName = fs_findNextFile();
  }
  fs_findClose();
  if ( filesave != NULL ) fs_fileClose( filesave );
}

//--------------------------------------------------------------------------------------------
void load_blip_bitmap( char * modname )
{
  //This function loads the blip bitmaps
  int cnt;

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.blip_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxBlip, CStringTmp1, INVALID_KEY ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.blip_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxBlip, CStringTmp1, INVALID_KEY ) )
    {
      log_warning( "Blip bitmap not loaded. Missing file = \"%s\"\n", CStringTmp1 );
    }
  };



  // Set up the rectangles
  blipwidth  = TxBlip.imgW / NUMBAR;
  blipheight = TxBlip.imgH;
  for ( cnt = 0; cnt < NUMBAR; cnt++ )
  {
    bliprect[cnt].left   = ( cnt + 0 ) * blipwidth;
    bliprect[cnt].right  = ( cnt + 1 ) * blipwidth;
    bliprect[cnt].top    = 0;
    bliprect[cnt].bottom = blipheight;
  }
}

//--------------------------------------------------------------------------------------------
void load_menu()
{
  // ZZ> This function loads all of the menu data...  Images are loaded into system
  // memory

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.font_bitmap );
  snprintf( CStringTmp2, sizeof( CStringTmp2 ), "%s/%s", CData.basicdat_dir, CData.fontdef_file );
  load_font( CStringTmp1, CStringTmp2 );
  //load_all_menu_images();
}

//--------------------------------------------------------------------------------------------

/********************> Reshape3D() <*****/
void Reshape3D( int w, int h )
{
  glViewport( 0, 0, w, h );
}

int glinit( int argc, char **argv )
{
  // get the maximum anisotropy fupported by the video vard
  glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy );
  log2Anisotropy = ( maxAnisotropy == 0 ) ? 0 : floor( log( maxAnisotropy + 1e-6 ) / log( 2.0f ) );

  if ( maxAnisotropy == 0.0f && CData.texturefilter >= TX_ANISOTROPIC )
  {
    CData.texturefilter = TX_TRILINEAR_2;
  }
  userAnisotropy = MIN( maxAnisotropy, userAnisotropy );

  /* Depth testing stuff */
  glClearDepth( 1.0 );
  glDepthFunc( GL_LESS );
  glEnable( GL_DEPTH_TEST );

  //Load the current graphical settings
  load_graphics();

  //fill mode
  glPolygonMode( GL_FRONT, GL_FILL );
  glPolygonMode( GL_BACK,  GL_FILL );

  /* Disable OpenGL lighting */
  glDisable( GL_LIGHTING );

  /* Backface culling */
  glEnable( GL_CULL_FACE );  // This seems implied - DDOI
  glCullFace( GL_BACK );

  // set up environment mapping
  glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );  // Set The Texture Generation Mode For S To Sphere Mapping (NEW)
  glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );  // Set The Texture Generation Mode For T To Sphere Mapping (NEW)

  EnableTexturing();  // Enable texture mapping

  return 1;
}

void sdlinit( int argc, char **argv )
{
  int cnt, colordepth;
  SDL_Surface *theSurface;
  STRING strbuffer = {0};
  
  log_info("Initializing main SDL services version %i.%i.%i... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
  if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK ) < 0 )
  {
    log_message("Failed!\n");

    log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
    exit( 1 );
  }
  else
  {
    log_message("Success!\n");
  }

  atexit( SDL_Quit );

  // log the video driver info
  SDL_VideoDriverName( strbuffer, 256 );
  log_info( "Using Video Driver - %s\n", strbuffer );

  colordepth = CData.scrd / 3;

  /* Setup the cute windows manager icon */
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.icon_bitmap );
  theSurface = SDL_LoadBMP( CStringTmp1 );
  if ( theSurface == NULL )
  {
    log_warning( "Unable to load icon (basicdat/icon.bmp)\n" );
  }
  SDL_WM_SetIcon( theSurface, NULL );

  /* Set the OpenGL Attributes */
#ifndef __unix__
  // Under Unix we cannot specify these, we just get whatever format
  // the framebuffer has, specifying depths > the framebuffer one
  // will cause SDL_SetVideoMode to fail with: "Unable to set video mode: Couldn't find matching GLX visual"
  SDL_GL_SetAttribute( SDL_GL_RED_SIZE, colordepth );
  SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, colordepth );
  SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  colordepth );
  SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, CData.scrd );
#endif
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

  displaySurface = SDL_SetVideoMode( CData.scrx, CData.scry, CData.scrd, SDL_DOUBLEBUF | SDL_OPENGL | ( CData.fullscreen ? SDL_FULLSCREEN : 0 ) );
  if ( displaySurface == NULL )
  {
    log_error( "Unable to set video mode: %s\n", SDL_GetError() );
    exit( 1 );
  }
  video_mode_chaged = bfalse;

  //Debug
  //Enable antialiasing X16
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
  glEnable(GL_MULTISAMPLE_ARB);
  //glEnable(GL_MULTISAMPLE);
  //Debug end

  //Force OpenGL hardware acceleration
  if(CData.gfxacceleration)
  {
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  }

  //Grab all the available video modes
  video_mode_list = SDL_ListModes( displaySurface->format, SDL_DOUBLEBUF | SDL_HWSURFACE | SDL_FULLSCREEN | SDL_OPENGL | SDL_HWACCEL | SDL_SRCALPHA );
  log_info( "Detecting avalible video modes...\n" );
  for ( cnt = 0; NULL != video_mode_list[cnt]; ++cnt )
  {
    log_info( "Video Mode - %d x %d\n", video_mode_list[cnt]->w, video_mode_list[cnt]->h );
  };

  // Set the window name
  SDL_WM_SetCaption( "Egoboo", "Egoboo" );

  // set the mouse cursor
  SDL_WM_GrabInput( CData.GrabMouse );
  //if (CData.HideMouse) SDL_ShowCursor(SDL_DISABLE);

  if ( SDL_NumJoysticks() > 0 )
  {
    sdljoya = SDL_JoystickOpen( 0 );
    joyaon = btrue;
  }
}

void load_graphics()
{
  //This function loads all the graphics based on the game settings

  //Check if the computer graphic driver supports anisotropic filtering
  if ( CData.texturefilter >= TX_ANISOTROPIC )
  {
    if ( 0 == strstr(( char* ) glGetString( GL_EXTENSIONS ), "GL_EXT_texture_filter_anisotropic" ) )
    {
      log_warning( "Your graphics driver does not support anisotropic filtering.\n" );
      CData.texturefilter = TX_TRILINEAR_2; //Set filtering to trillienar instead
    }
  }

  //Enable prespective correction?
  glHint( GL_PERSPECTIVE_CORRECTION_HINT, CData.perspective );

  //Enable dithering?
  if ( CData.dither ) glEnable( GL_DITHER );
  else glDisable( GL_DITHER );

  //Enable gourad CData.shading? (Important!)
  glShadeModel( CData.shading );

  //Enable CData.antialiasing?
  if ( CData.antialiasing )
  {
    glEnable( GL_LINE_SMOOTH );
    glHint( GL_LINE_SMOOTH_HINT,    GL_NICEST );

    glEnable( GL_POINT_SMOOTH );
    glHint( GL_POINT_SMOOTH_HINT,   GL_NICEST );

    glDisable( GL_POLYGON_SMOOTH );
    glHint( GL_POLYGON_SMOOTH_HINT,    GL_FASTEST );

    // PLEASE do not turn this on unless you use
    //  glEnable (GL_BLEND);
    //  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // before every single draw command
    //
    //glEnable(GL_POLYGON_SMOOTH);
    //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
  }
  else
  {
    glDisable( GL_POINT_SMOOTH );
    glDisable( GL_LINE_SMOOTH );
    glDisable( GL_POLYGON_SMOOTH );
  }

  //Check which particle image to load
  if ( CData.particletype      == PART_NORMAL ) strncpy( CData.particle_bitmap, "particle_normal.png" , sizeof( STRING ) );
  else if ( CData.particletype == PART_SMOOTH ) strncpy( CData.particle_bitmap, "particle_smooth.png" , sizeof( STRING ) );
  else if ( CData.particletype == PART_FAST )   strncpy( CData.particle_bitmap, "particle_fast.png" , sizeof( STRING ) );

  //Wait for vertical synchronization?
  if( CData.vsync )
  {
	  // Fedora 7 doesn't suuport SDL_GL_SWAP_CONTROL, but we use this  nvidia extension instead.
#if defined(__unix__)
	    SDL_putenv("__GL_SYNC_TO_VBLANK=1");
#else
	  /* Turn on vsync, this works on Windows. */
	    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
#endif
  }
}

/* obsolete graphics functions */
#if 0
////--------------------------------------------------------------------------------------------
//void draw_titleimage(int image, int x, int y)
//{
//  // ZZ> This function draws a title image on the backbuffer
//  GLfloat txWidth, txHeight;
//
//  if ( INVALID_TEXTURE != GLTexture_GetTextureID(&TxTitleImage[image]) )
//  {
//    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
//    Begin2DMode();
//    glNormal3f( 0, 0, 1 ); // glNormal3f( 0, 1, 0 );
//
//    /* Calculate the texture width & height */
//    txWidth = ( GLfloat )( GLTexture_GetImageWidth( &TxTitleImage[image] )/GLTexture_GetTextureWidth( &TxTitleImage[image] ) );
//    txHeight = ( GLfloat )( GLTexture_GetImageHeight( &TxTitleImage[image] )/GLTexture_GetTextureHeight( &TxTitleImage[image] ) );
//
//    /* Bind the texture */
//    GLTexture_Bind( &TxTitleImage[image], CData.texturefilter );
//
//    /* Draw the quad */
//    glBegin( GL_QUADS );
//    glTexCoord2f( 0, 1 ); glVertex2f( x, CData.scry - y - GLTexture_GetImageHeight( &TxTitleImage[image] ) );
//    glTexCoord2f( txWidth, 1 ); glVertex2f( x + GLTexture_GetImageWidth( &TxTitleImage[image] ), CData.scry - y - GLTexture_GetImageHeight( &TxTitleImage[image] ) );
//    glTexCoord2f( txWidth, 1-txHeight ); glVertex2f( x + GLTexture_GetImageWidth( &TxTitleImage[image] ), CData.scry - y );
//    glTexCoord2f( 0, 1-txHeight ); glVertex2f( x, CData.scry - y );
//    glEnd();
//
//    End2DMode();
//  }
//}
//-------------------------------------------------------------------------------------------------
//void do_cursor()
//{
//  // This function implements a mouse cursor
//  read_input( NULL );
//  cursorx = mousex;  if ( cursorx < 6 )  cursorx = 6;  if ( cursorx > CData.scrx - 16 )  cursorx = CData.scrx - 16;
//  cursory = mousey;  if ( cursory < 8 )  cursory = 8;  if ( cursory > CData.scry - 24 )  cursory = CData.scry - 24;
//  clicked = bfalse;
//  if ( mousebutton[0] && !pressed )
//  {
//    clicked = btrue;
//  }
//  pressed = mousebutton[0];
//
//  BeginText( &TxFont );
//  {
//    draw_one_font( 95, cursorx - 5, cursory - 7 );
//  }
//  EndText();
//}

#endif
