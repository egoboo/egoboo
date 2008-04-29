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
#include "egoboo_math.h"
#include "Log.h"
#include "Ui.h"
#include "mesh.h"
#include "Menu.h"
#include "input.h"
#include "camera.h"
#include "script.h"
#include "particle.h"
#include "passage.h"
#include "input.h"
#include "Network.h"

#include "egoboo_utility.h"
#include "egoboo.h"

#include <assert.h>
#include <stdarg.h>

#ifdef __unix__
#include <unistd.h>
#endif

CURSOR cursor =
{
  0,           //  x
  0,           //  y
  bfalse,      //  pressed
  bfalse,      //  clicked
  bfalse,      //  pending
};

static int draw_wrap_string( BMFont * pfnt, float x, float y, GLfloat tint[], float maxx, char * szFormat, ... );
static int draw_status(  BMFont *  pfnt , CHR_REF character, int x, int y );
static void draw_text(  BMFont *  pfnt  );


// Defined in egoboo.h
SDL_Surface *displaySurface;
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
  glLoadMatrixf( GCamera.mProjection.v );

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadMatrixf( GCamera.mView.v );

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
    skintoicon[cnt] = 0;
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
    CapList[cnt].classname[0] = '\0';
    MadList[cnt].used = bfalse;
    strcpy(MadList[cnt].name, "*NONE*");
    MadList[cnt].md2_ptr = NULL;
    MadList[cnt].framelip = NULL;
    MadList[cnt].framefx  = NULL;
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
    CapList[cnt].classname[0] = 0;
    MadList[cnt].used = bfalse;
    strcpy(MadList[cnt].name, "*NONE*");

    free_one_mad( cnt );
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
  strncpy( GMsg.textdisplay[slot], buffer, sizeof( GMsg.textdisplay[slot] ) );
  GMsg.time[slot] = time * DELAY_MESSAGE;
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
      // No players???
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
  if ( message < GMsg.total )
  {
    // Copy the message
    read = GMsg.index[message];
    cnt = 0;
    cTmp = GMsg.text[read];  read++;
    while ( cTmp != 0 )
    {
      if ( cTmp == '%' )
      {
        // Escape sequence
        eread = szTmp;
        szTmp[0] = 0;
        cTmp = GMsg.text[read];  read++;
        if ( cTmp == 'n' ) // Name
        {
          if ( ChrList[character].nameknown )
            strncpy( szTmp, ChrList[character].name, sizeof( STRING ) );
          else
          {
            lTmp = CapList[ChrList[character].model].classname[0];
            if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
              snprintf( szTmp, sizeof( szTmp ), "an %s", CapList[ChrList[character].model].classname );
            else
              snprintf( szTmp, sizeof( szTmp ), "a %s", CapList[ChrList[character].model].classname );
          }
          if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
        }
        if ( cTmp == 'c' ) // Class name
        {
          eread = CapList[ChrList[character].model].classname;
        }
        if ( cTmp == 't' ) // Target name
        {
          if ( ChrList[target].nameknown )
            strncpy( szTmp, ChrList[target].name, sizeof( STRING ) );
          else
          {
            lTmp = CapList[ChrList[target].model].classname[0];
            if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
              snprintf( szTmp, sizeof( szTmp ), "an %s", CapList[ChrList[target].model].classname );
            else
              snprintf( szTmp, sizeof( szTmp ), "a %s", CapList[ChrList[target].model].classname );
          }
          if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
        }
        if ( cTmp == 'o' ) // Owner name
        {
          if ( ChrList[owner].nameknown )
            strncpy( szTmp, ChrList[owner].name, sizeof( STRING ) );
          else
          {
            lTmp = CapList[ChrList[owner].model].classname[0];
            if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
              snprintf( szTmp, sizeof( szTmp ), "an %s", CapList[ChrList[owner].model].classname );
            else
              snprintf( szTmp, sizeof( szTmp ), "a %s", CapList[ChrList[owner].model].classname );
          }
          if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
        }
        if ( cTmp == 's' ) // Target class name
        {
          eread = CapList[ChrList[target].model].classname;
        }
        if ( cTmp >= '0' && cTmp <= '0' + ( MAXSKIN - 1 ) )  // Target's skin name
        {
          eread = CapList[ChrList[target].model].skinname[cTmp-'0'];
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
          if ( ChrList[character].ammoknown )
            snprintf( szTmp, sizeof( szTmp ), "%d", ChrList[character].ammo );
          else
            snprintf( szTmp, sizeof( szTmp ), "?" );
        }
        if ( cTmp == 'k' ) // Kurse state
        {
          if ( ChrList[character].iskursed )
            snprintf( szTmp, sizeof( szTmp ), "kursed" );
          else
            snprintf( szTmp, sizeof( szTmp ), "unkursed" );
        }
        if ( cTmp == 'p' ) // Character's possessive
        {
          if ( ChrList[character].gender == GEN_FEMALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "her" );
          }
          else
          {
            if ( ChrList[character].gender == GEN_MALE )
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
          if ( ChrList[character].gender == GEN_FEMALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "female " );
          }
          else
          {
            if ( ChrList[character].gender == GEN_MALE )
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
          if ( ChrList[target].gender == GEN_FEMALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "her" );
          }
          else
          {
            if ( ChrList[target].gender == GEN_MALE )
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
      cTmp = GMsg.text[read];  read++;
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

  GCamera.turn_lr_one   = GCamera.turn_lr / (float)(1<<16);

  // Request matrices needed for local machine
  make_dolist();
  order_dolist();
}

//--------------------------------------------------------------------------------------------
void animate_tiles( float dUpdate )
{
  // This function changes the animated tile frame

  GTile_Anim.framefloat += dUpdate / ( float ) GTile_Anim.updateand;
  while ( GTile_Anim.framefloat >= 1.0f )
  {
    GTile_Anim.framefloat -= 1.0f;
    GTile_Anim.frameadd = ( GTile_Anim.frameadd + 1 ) & GTile_Anim.frameand;
  };
}

//--------------------------------------------------------------------------------------------
void load_basic_textures( char *modname )
{
  // ZZ> This function loads the standard textures for a module
  // BB> In each case, try to load one stored with the module first.



  // Particle sprites
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, "particle.bmp" );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[TX_PARTICLE], CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.particle_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[TX_PARTICLE], CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "!!!!Particle bitmap could not be found!!!! Missing File = \"%s\"\n", CStringTmp1 );
    }
  };

  // Module background tiles
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.tile0_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[TX_TILE_0], CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.tile0_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[TX_TILE_0], CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 0 could not be found. Missing File = \"%s\"\n", CData.tile0_bitmap );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.tile1_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,   &TxTexture[TX_TILE_1], CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.tile1_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[TX_TILE_1], CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 1 could not be found. Missing File = \"%s\"\n", CData.tile1_bitmap );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.tile2_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[TX_TILE_2], CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.tile2_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[TX_TILE_2], CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 2 could not be found. Missing File = \"%s\"\n", CData.tile2_bitmap );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.tile3_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[TX_TILE_3], CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.tile3_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[TX_TILE_3], CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 3 could not be found. Missing File = \"%s\"\n", CData.tile3_bitmap );
    }
  };


  // Water textures
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.watertop_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[TX_WATER_TOP], CStringTmp1, INVALID_KEY ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.watertop_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[TX_WATER_TOP], CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Water Layer 1 could not be found. Missing File = \"%s\"\n", CData.watertop_bitmap );
    }
  };

  // This is also used as far background
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.waterlow_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[TX_WATER_LOW], CStringTmp1, INVALID_KEY ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.waterlow_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[TX_WATER_LOW], CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Water Layer 0 could not be found. Missing File = \"%s\"\n", CData.waterlow_bitmap );
    }
  };


  // BB > this is handled differently now and is not needed
  // Texture 7 is the phong map
  //snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.phong_bitmap);
  //if(INVALID_TEXTURE==GLTexture_Load(GL_TEXTURE_2D,  &TxTexture[TX_PHONG], CStringTmp1, INVALID_KEY))
  //{
  //  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.phong_bitmap);
  //  GLTexture_Load(GL_TEXTURE_2D,  &TxTexture[TX_PHONG], CStringTmp1, TRANSCOLOR );
  //  {
  //    log_warning("Phong Bitmap Layer 1 could not be found. Missing File = \"%s\"", CData.phong_bitmap);
  //  }
  //};


}



//--------------------------------------------------------------------------------------------
Uint16 load_one_object( int skin, char* szObjectname )
{
  // ZZ> This function loads one iobj and returns the number of skins
  Uint16 iobj;
  int numskins, numicon, skin_index;
  STRING newloadname, loc_loadpath, wavename;
  int cnt;

  // generate an index for this object
  snprintf( newloadname, sizeof( newloadname ), "%s/%s", szObjectname, CData.data_file );
  iobj = object_generate_index(newloadname);

  // Append a slash to the szObjectname
  strncpy( loc_loadpath, szObjectname, sizeof( loc_loadpath ) );
  strncat( loc_loadpath, "/", sizeof( loc_loadpath ) );

  // Load the iobj data file
  iobj = load_one_cap(loc_loadpath, iobj);

  // load the model data
  load_one_mad( loc_loadpath, iobj );

  // Fix lighting if need be
  if ( CapList[iobj].uniformlit )
  {
    make_mad_equally_lit( iobj );
  }

  // Load the messages for this object
  make_newloadname( loc_loadpath, CData.message_file, newloadname );
  load_all_messages( newloadname, iobj );


  // Load the random naming table for this object
  make_newloadname( loc_loadpath, CData.naming_file, newloadname );
  read_naming( iobj, newloadname );


  // Load the particles for this object
  for ( cnt = 0; cnt < PRTPIP_PEROBJECT_COUNT; cnt++ )
  {
    MadList[MAXMODEL].prtpip[cnt] = MAXPRTPIP;
    snprintf( newloadname, sizeof( newloadname ), "%spart%d.txt", loc_loadpath, cnt );
    load_one_pip( newloadname, iobj, cnt );
  }


  // Load the waves for this object
  for ( cnt = 0; cnt < MAXWAVE; cnt++ )
  {
    snprintf( wavename, sizeof( wavename ), "%ssound%d.wav", loc_loadpath, cnt );
    CapList[iobj].wavelist[cnt] = Mix_LoadWAV( wavename );
  }


  // Load the enchantment for this object
  make_newloadname( loc_loadpath, CData.enchant_file, newloadname );
  load_one_enchant_profile( newloadname, iobj );


  // Load the skins and icons
  MadList[iobj].skinstart = skin;
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
        if ( iobj == SPELLBOOK && bookicon == 0 )
          bookicon = globalnumicon;

        while ( numicon < numskins )
        {
          skintoicon[skin+numicon] = globalnumicon;
          numicon++;
        }
        globalnumicon++;
      }
    }
  }

  MadList[iobj].skins = numskins;
  if ( numskins == 0 )
  {
    // If we didn't get a skin, set it to the water texture
    MadList[iobj].skinstart = TX_WATER_TOP;
    MadList[iobj].skins = 1;
  }


  return numskins;
}

//--------------------------------------------------------------------------------------------
void load_all_objects( char *modname )
{
  // ZZ> This function loads a module's objects
  const char *filehandle;
  FILE* fileread, *filewrite;
  STRING newloadname, filename;
  int cnt;
  int skin;
  int importplayer;

  // Clear the import slots...
  for ( cnt = 0; cnt < MAXMODEL; cnt++ )
    CapList[cnt].importslot = 10000;

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
      CapList[importobject].importslot = cnt;
      skin += load_one_object( skin, filename );
    }
  }

  //empty_import_directory();  // Free up that disk space...

  // If in Developer mode, create a new debug.txt file for debug info logging
  if( CData.DevMode )
  {
    filewrite = fs_fileOpen( PRI_NONE, NULL, CData.debug_file, "w" );
    fprintf( filewrite, "DEBUG INFORMATION FOR MODULE: \"%s\" \n", modname );
    fprintf( filewrite, "This document logs extra debugging information for the last module loaded.\n");
    fprintf( filewrite, "\nSpawning log after module has started...\n");
    fprintf( filewrite, "-----------------------------------------------\n" );
    fs_fileClose( filewrite );
  }

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


  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D, &(bmfont.tex), szBitmap, 0 ) ) return bfalse;


  // Clear out the conversion table
  for ( cnt = 0; cnt < 256; cnt++ )
    bmfont.ascii_table[cnt] = 0;


  // Get the size of the bitmap
  xsize = GLTexture_GetImageWidth( &(bmfont.tex) );
  ysize = GLTexture_GetImageHeight( &(bmfont.tex) );
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
  bmfont.offset = CData.scry - yspacing;

  // Mark all as unused
  for ( cnt = 0; cnt < 255; cnt++ )
    bmfont.ascii_table[cnt] = 255;


  cnt = 0;
  y = 0;
  xstt = 0;
  ystt = 0;
  while ( cnt < 255 && fgoto_colon_yesno( fileread ) )
  {
    cTmp = fgetc( fileread );
    xspacing = fget_int( fileread );
    if ( bmfont.ascii_table[cTmp] == 255 )
    {
      bmfont.ascii_table[cTmp] = cnt;
    }

    if ( xstt + xspacing + 1 >= xsize )
    {
      xstt = 0;
      ystt += yspacing;
    }

    bmfont.rect[cnt].x = xstt;
    bmfont.rect[cnt].w = xspacing;
    bmfont.rect[cnt].y = ystt;
    bmfont.rect[cnt].h = yspacing - 1;
    bmfont.spacing_x[cnt] = xspacing;

    xstt += xspacing + 1;

    cnt++;
  }
  fs_fileClose( fileread );


  // Space between lines
  bmfont.spacing_y = ( yspacing >> 1 ) + FONTADD;

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
  while ( layer < GWater.num_layer )
  {
    GWater.layeru[layer] = 0;
    GWater.layerv[layer] = 0;
    frame = 0;
    while ( frame < MAXWATERFRAME )
    {
      // Do first mode
      mode = 0;
      for ( point = 0; point < WATERPOINTS; point++ )
      {
        tmp_sin = sin(( frame * TWO_PI / MAXWATERFRAME ) + ( PI * point / WATERPOINTS ) + ( PI_OVER_TWO * layer / MAXWATERLAYER ) );
        tmp_cos = cos(( frame * TWO_PI / MAXWATERFRAME ) + ( PI * point / WATERPOINTS ) + ( PI_OVER_TWO * layer / MAXWATERLAYER ) );
        GWater.layerzadd[layer][frame][mode][point]  = tmp_sin * GWater.layeramp[layer];
      }

      // Now mirror and copy data to other three modes
      mode++;
      GWater.layerzadd[layer][frame][mode][0] = GWater.layerzadd[layer][frame][0][1];
      //GWater.layercolor[layer][frame][mode][0] = GWater.layercolor[layer][frame][0][1];
      GWater.layerzadd[layer][frame][mode][1] = GWater.layerzadd[layer][frame][0][0];
      //GWater.layercolor[layer][frame][mode][1] = GWater.layercolor[layer][frame][0][0];
      GWater.layerzadd[layer][frame][mode][2] = GWater.layerzadd[layer][frame][0][3];
      //GWater.layercolor[layer][frame][mode][2] = GWater.layercolor[layer][frame][0][3];
      GWater.layerzadd[layer][frame][mode][3] = GWater.layerzadd[layer][frame][0][2];
      //GWater.layercolor[layer][frame][mode][3] = GWater.layercolor[layer][frame][0][2];
      mode++;

      GWater.layerzadd[layer][frame][mode][0] = GWater.layerzadd[layer][frame][0][3];
      //GWater.layercolor[layer][frame][mode][0] = GWater.layercolor[layer][frame][0][3];
      GWater.layerzadd[layer][frame][mode][1] = GWater.layerzadd[layer][frame][0][2];
      //GWater.layercolor[layer][frame][mode][1] = GWater.layercolor[layer][frame][0][2];
      GWater.layerzadd[layer][frame][mode][2] = GWater.layerzadd[layer][frame][0][1];
      //GWater.layercolor[layer][frame][mode][2] = GWater.layercolor[layer][frame][0][1];
      GWater.layerzadd[layer][frame][mode][3] = GWater.layerzadd[layer][frame][0][0];
      //GWater.layercolor[layer][frame][mode][3] = GWater.layercolor[layer][frame][0][0];
      mode++;

      GWater.layerzadd[layer][frame][mode][0] = GWater.layerzadd[layer][frame][0][2];
      //GWater.layercolor[layer][frame][mode][0] = GWater.layercolor[layer][frame][0][2];
      GWater.layerzadd[layer][frame][mode][1] = GWater.layerzadd[layer][frame][0][3];
      //GWater.layercolor[layer][frame][mode][1] = GWater.layercolor[layer][frame][0][3];
      GWater.layerzadd[layer][frame][mode][2] = GWater.layerzadd[layer][frame][0][0];
      //GWater.layercolor[layer][frame][mode][2] = GWater.layercolor[layer][frame][0][0];
      GWater.layerzadd[layer][frame][mode][3] = GWater.layerzadd[layer][frame][0][1];
      //GWater.layercolor[layer][frame][mode][3] = GWater.layercolor[layer][frame][0][1];
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

    GWater.spek[cnt] = spek;

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
    GWater.num_layer = fget_next_int( fileread );
    GWater.spekstart = fget_next_int( fileread );
    GWater.speklevel_fp8 = fget_next_int( fileread );
    GWater.douselevel = fget_next_int( fileread );
    GWater.surfacelevel = fget_next_int( fileread );
    GWater.light = fget_next_bool( fileread );
    GWater.iswater = fget_next_bool( fileread );
    CData.render_overlay = fget_next_bool( fileread ) && CData.overlayvalid;
    CData.render_background = fget_next_bool( fileread ) && CData.backgroundvalid;
    GWater.layerdistx[0] = fget_next_float( fileread );
    GWater.layerdisty[0] = fget_next_float( fileread );
    GWater.layerdistx[1] = fget_next_float( fileread );
    GWater.layerdisty[1] = fget_next_float( fileread );
    foregroundrepeat = fget_next_int( fileread );
    backgroundrepeat = fget_next_int( fileread );


    GWater.layerz[0] = fget_next_int( fileread );
    GWater.layeralpha_fp8[0] = fget_next_int( fileread );
    GWater.layerframeadd[0] = fget_next_int( fileread );
    GWater.lightlevel_fp8[0] = fget_next_int( fileread );
    GWater.lightadd_fp8[0] = fget_next_int( fileread );
    GWater.layeramp[0] = fget_next_float( fileread );
    GWater.layeruadd[0] = fget_next_float( fileread );
    GWater.layervadd[0] = fget_next_float( fileread );

    GWater.layerz[1] = fget_next_int( fileread );
    GWater.layeralpha_fp8[1] = fget_next_int( fileread );
    GWater.layerframeadd[1] = fget_next_int( fileread );
    GWater.lightlevel_fp8[1] = fget_next_int( fileread );
    GWater.lightadd_fp8[1] = fget_next_int( fileread );
    GWater.layeramp[1] = fget_next_float( fileread );
    GWater.layeruadd[1] = fget_next_float( fileread );
    GWater.layervadd[1] = fget_next_float( fileread );

    GWater.layeru[0] = 0;
    GWater.layerv[0] = 0;
    GWater.layeru[1] = 0;
    GWater.layerv[1] = 0;
    GWater.layerframe[0] = rand() & WATERFRAMEAND;
    GWater.layerframe[1] = rand() & WATERFRAMEAND;

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

    GTile_Anim.updateand = fget_next_int( fileread );
    GTile_Anim.frameand = fget_next_int( fileread );
    GTile_Anim.bigframeand = ( GTile_Anim.frameand << 1 ) + 1;
    GTile_Dam.amount = fget_next_int( fileread );
    GTile_Dam.type = fget_next_damage( fileread );

    // Read weather data fourth
    GWeather.overwater = fget_next_bool( fileread );
    GWeather.timereset = fget_next_int( fileread );
    GWeather.time = GWeather.timereset;
    GWeather.player = 0;

    // Read extra data
    mesh.exploremode = fget_next_bool( fileread );
    usefaredge = fget_next_bool( fileread );
    GCamera.swing = 0;
    GCamera.swingrate = fget_next_float( fileread );
    GCamera.swingamp = fget_next_float( fileread );


    // Read unnecessary data...  Only read if it exists...
    GFog.on = bfalse;
    GFog.affectswater = btrue;
    GFog.top = 100;
    GFog.bottom = 0;
    GFog.distance = 100;
    GFog.red = 255;
    GFog.grn = 255;
    GFog.blu = 255;
    GTile_Dam.parttype = MAXPRTPIP;
    GTile_Dam.partand = 255;
    GTile_Dam.sound = INVALID_SOUND;

    if ( fgoto_colon_yesno( fileread ) )
    {
      GFog.on           = CData.fogallowed;
      GFog.top          = fget_next_float( fileread );
      GFog.bottom       = fget_next_float( fileread );
      GFog.red          = fget_next_fixed( fileread );
      GFog.grn          = fget_next_fixed( fileread );
      GFog.blu          = fget_next_fixed( fileread );
      GFog.affectswater = fget_next_bool( fileread );

      GFog.distance = ( GFog.top - GFog.bottom );
      if ( GFog.distance < 1.0 )  GFog.on = bfalse;

      // Read extra stuff for damage tile particles...
      if ( fgoto_colon_yesno( fileread ) )
      {
        GTile_Dam.parttype = fget_int( fileread );
        GTile_Dam.partand  = fget_next_int( fileread );
        GTile_Dam.sound    = fget_next_int( fileread );
      }
    }

    // Allow slow machines to ignore the fancy stuff
    if ( !CData.twolayerwateron && GWater.num_layer > 1 )
    {
      int iTmp;
      GWater.num_layer = 1;
      iTmp = GWater.layeralpha_fp8[0];
      iTmp = FP8_MUL( GWater.layeralpha_fp8[1], iTmp ) + iTmp;
      if ( iTmp > 255 ) iTmp = 255;
      GWater.layeralpha_fp8[0] = iTmp;
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
  u = GWater.layeru[1];
  v = GWater.layerv[1];
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
  u = GWater.layeru[1];
  v = GWater.layerv[1];
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
  Uint16 chrlightambi = ChrList[character].lightambir_fp8 + ChrList[character].lightambig_fp8 + ChrList[character].lightambib_fp8;
  Uint16 chrlightspek = ChrList[character].lightspekr_fp8 + ChrList[character].lightspekg_fp8 + ChrList[character].lightspekb_fp8;
  float  globlightambi = lightambicol.r + lightambicol.g + lightambicol.b;
  float  globlightspek = lightspekcol.r + lightspekcol.g + lightspekcol.b;

  hide = CapList[ChrList[character].model].hidestate;
  if ( hide != NOHIDE && hide == ChrList[character].aistate ) return;

  // Original points
  level = ChrList[character].level;
  level += SHADOWRAISE;
  height = ChrList[character].matrix _CNV( 3, 2 ) - level;
  if ( height < 0 ) height = 0;

  tile_factor = mesh_has_some_bits( ChrList[character].onwhichfan, MESHFX_WATER ) ? 0.5 : 1.0;

  height_factor   = MAX( MIN(( 5 * ChrList[character].bmpdata.calc_size / height ), 1 ), 0 );
  ambient_factor  = ( float )( chrlightspek ) / ( float )( chrlightambi + chrlightspek );
  ambient_factor  = 0.5f * ( ambient_factor + globlightspek / ( globlightambi + globlightspek ) );
  alpha_character = FP8_TO_FLOAT( ChrList[character].alpha_fp8 );
  if ( ChrList[character].light_fp8 == 255 )
  {
    light_character = 1.0f;
  }
  else
  {
    light_character = ( float ) chrlightspek / 3.0f / ( float ) ChrList[character].light_fp8;
    light_character =  MIN( 1, MAX( 0, light_character ) );
  };


  size_umbra_x    = ( ChrList[character].bmpdata.cv.x_max - ChrList[character].bmpdata.cv.x_min - height / 30.0 );
  size_umbra_y    = ( ChrList[character].bmpdata.cv.y_max - ChrList[character].bmpdata.cv.y_min - height / 30.0 );
  size_penumbra_x = ( ChrList[character].bmpdata.cv.x_max - ChrList[character].bmpdata.cv.x_min + height / 30.0 );
  size_penumbra_y = ( ChrList[character].bmpdata.cv.y_max - ChrList[character].bmpdata.cv.y_min + height / 30.0 );

  alpha_umbra    = alpha_character * height_factor * ambient_factor * light_character * tile_factor;
  alpha_penumbra = alpha_character * height_factor * ambient_factor * light_character * tile_factor;

  if ( FLOAT_TO_FP8( alpha_umbra ) == 0 && FLOAT_TO_FP8( alpha_penumbra ) == 0 ) return;

  x = ChrList[character].matrix _CNV( 3, 0 );
  y = ChrList[character].matrix _CNV( 3, 1 );

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
//  hide = CapList[ChrList[character].model].hidestate;
//  if (hide == NOHIDE || hide != ChrList[character].aistate)
//  {
//    // Original points
//    level = ChrList[character].level;
//    level += SHADOWRAISE;
//    height = ChrList[character].matrix _CNV(3, 2) - level;
//    if (height > 255)  return;
//    if (height < 0) height = 0;
//    size = ChrList[character].bmpdata.calc_shadowsize - FP8_MUL(height, ChrList[character].bmpdata.calc_shadowsize);
//    if (size < 1) return;
//    ambi = ChrList[character].lightspek_fp8 >> 4;  // LUL >>3;
//    trans = ((255 - height) >> 1) + 64;
//
//    x = ChrList[character].matrix _CNV(3, 0);
//    y = ChrList[character].matrix _CNV(3, 1);
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
  Uint32 fan;


  for ( cnt = 0; cnt < numdolist; cnt++ )
  {
    tnc = dolist[cnt];
    fan = ChrList[tnc].onwhichfan;

    if(INVALID_FAN == fan) continue;

    vrtstart = Mesh_Fan[ChrList[tnc].onwhichfan].vrt_start;

    x = ChrList[tnc].pos.x;
    y = ChrList[tnc].pos.y;
    x = ( x & 127 ) >> 5;  // From 0 to 3
    y = ( y & 127 ) >> 5;  // From 0 to 3

    i0 = Mesh_Mem.vrt_lr_fp8[vrtstart + 0];
    i1 = Mesh_Mem.vrt_lr_fp8[vrtstart + 1];
    i2 = Mesh_Mem.vrt_lr_fp8[vrtstart + 2];
    i3 = Mesh_Mem.vrt_lr_fp8[vrtstart + 3];
    calc_chr_lighting( x, y, i0, i1, i2, i3, &spek, &ambi );
    ChrList[tnc].lightambir_fp8 = ambi;
    ChrList[tnc].lightspekr_fp8 = spek;

    if ( !mesh.exploremode )
    {
      // Look up spek direction using corners again
      i0 = (( i0 & 0xf0 ) << 8 ) & 0xf000;
      i1 = (( i1 & 0xf0 ) << 4 ) & 0x0f00;
      i3 = (( i3 & 0xf0 ) << 0 ) & 0x00f0;
      i2 = (( i2 & 0xf0 ) >> 4 ) & 0x000f;
      i0 = i0 | i1 | i3 | i2;
      ChrList[tnc].lightturn_lrr = ( lightdirectionlookup[i0] << 8 );
    }
    else
    {
      ChrList[tnc].lightturn_lrr = 0;
    }

    i0 = Mesh_Mem.vrt_lg_fp8[vrtstart + 0];
    i1 = Mesh_Mem.vrt_lg_fp8[vrtstart + 1];
    i3 = Mesh_Mem.vrt_lg_fp8[vrtstart + 2];
    i2 = Mesh_Mem.vrt_lg_fp8[vrtstart + 3];
    calc_chr_lighting( x, y, i0, i1, i2, i3, &spek, &ambi );
    ChrList[tnc].lightambig_fp8 = ambi;
    ChrList[tnc].lightspekg_fp8 = spek;

    if ( !mesh.exploremode )
    {
      // Look up spek direction using corners again
      i0 = (( i0 & 0xf0 ) << 8 ) & 0xf000;
      i1 = (( i1 & 0xf0 ) << 4 ) & 0x0f00;
      i3 = (( i3 & 0xf0 ) << 0 ) & 0x00f0;
      i2 = (( i2 & 0xf0 ) >> 4 ) & 0x000f;
      i0 = i0 | i1 | i3 | i2;
      ChrList[tnc].lightturn_lrg = ( lightdirectionlookup[i0] << 8 );
    }
    else
    {
      ChrList[tnc].lightturn_lrg = 0;
    }

    i0 = Mesh_Mem.vrt_lb_fp8[vrtstart + 0];
    i1 = Mesh_Mem.vrt_lb_fp8[vrtstart + 1];
    i3 = Mesh_Mem.vrt_lb_fp8[vrtstart + 2];
    i2 = Mesh_Mem.vrt_lb_fp8[vrtstart + 3];
    calc_chr_lighting( x, y, i0, i1, i2, i3, &spek, &ambi );
    ChrList[tnc].lightambib_fp8 = ambi;
    ChrList[tnc].lightspekb_fp8 = spek;

    if ( !mesh.exploremode )
    {
      // Look up spek direction using corners again
      i0 = (( i0 & 0xf0 ) << 8 ) & 0xf000;
      i1 = (( i1 & 0xf0 ) << 4 ) & 0x0f00;
      i3 = (( i3 & 0xf0 ) << 0 ) & 0x00f0;
      i2 = (( i2 & 0xf0 ) >> 4 ) & 0x000f;
      i0 = i0 | i1 | i3 | i2;
      ChrList[tnc].lightturn_lrb = ( lightdirectionlookup[i0] << 8 );
    }
    else
    {
      ChrList[tnc].lightturn_lrb = 0;
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

    switch ( PrtList[cnt].type )
    {
      case PRTTYPE_LIGHT:
        {
          float ftmp = PrtList[cnt].dyna.level * ( 127 * PrtList[cnt].dyna.falloff ) / FP8_TO_FLOAT( FP8_MUL( PrtList[cnt].size_fp8, PrtList[cnt].size_fp8 ) );
          if ( ftmp > 255 ) ftmp = 255;

          PrtList[cnt].lightr_fp8 =
          PrtList[cnt].lightg_fp8 =
          PrtList[cnt].lightb_fp8 = ftmp;
        }
        break;

      case PRTTYPE_ALPHA:
      case PRTTYPE_SOLID:
        {
          character = prt_get_attachedtochr( cnt );
          if ( VALID_CHR( character ) )
          {
            PrtList[cnt].lightr_fp8 = ChrList[character].lightspekr_fp8 + ChrList[character].lightambir_fp8;
            PrtList[cnt].lightg_fp8 = ChrList[character].lightspekg_fp8 + ChrList[character].lightambig_fp8;
            PrtList[cnt].lightb_fp8 = ChrList[character].lightspekb_fp8 + ChrList[character].lightambib_fp8;
          }
          else if ( INVALID_FAN != PrtList[cnt].onwhichfan )
          {
            PrtList[cnt].lightr_fp8 = Mesh_Mem.vrt_lr_fp8[Mesh_Fan[PrtList[cnt].onwhichfan].vrt_start];
            PrtList[cnt].lightg_fp8 = Mesh_Mem.vrt_lg_fp8[Mesh_Fan[PrtList[cnt].onwhichfan].vrt_start];
            PrtList[cnt].lightb_fp8 = Mesh_Mem.vrt_lb_fp8[Mesh_Fan[PrtList[cnt].onwhichfan].vrt_start];
          }
          else
          {
            PrtList[cnt].lightr_fp8 =
              PrtList[cnt].lightg_fp8 =
                PrtList[cnt].lightb_fp8 = 0;
          }
        }
        break;

      default:
        PrtList[cnt].lightr_fp8 =
          PrtList[cnt].lightg_fp8 =
            PrtList[cnt].lightb_fp8 = 0;
    };
  }

}

//--------------------------------------------------------------------------------------------
void render_water()
{
  // ZZ> This function draws all of the water fans

  int cnt;

  // Bottom layer first
  if ( !CData.render_background && GWater.num_layer > 1 )
  {
    cnt = 0;
    while ( cnt < renderlist.num_watr )
    {
      render_water_fan( renderlist.watr[cnt], 1, (( renderlist.watr[cnt] >> GWater.shift ) &2 ) + ( renderlist.watr[cnt]&1 ) );
      cnt++;
    }
  }

  // Top layer second
  if ( !CData.render_overlay && GWater.num_layer > 0 )
  {
    cnt = 0;
    while ( cnt < renderlist.num_watr )
    {
      render_water_fan( renderlist.watr[cnt], 0, (( renderlist.watr[cnt] >> GWater.shift ) &2 ) + ( renderlist.watr[cnt]&1 ) );
      cnt++;
    }
  }
}

void render_water_lit()
{
  // BB> This function draws the hilites for water tiles using global lighting

  int cnt;

  // Bottom layer first
  if ( !CData.render_background && GWater.num_layer > 1 )
  {
    float ambi_level = FP8_TO_FLOAT( GWater.lightadd_fp8[1] + GWater.lightlevel_fp8[1] );
    float spek_level =  FP8_TO_FLOAT( GWater.speklevel_fp8 );
    float spekularity = MIN( 40, spek_level / ambi_level ) + 2;
    GLfloat mat_none[]      = {0, 0, 0, 0};
    GLfloat mat_ambient[]   = { ambi_level, ambi_level, ambi_level, 1.0 };
    GLfloat mat_diffuse[]   = { spek_level, spek_level, spek_level, 1.0 };
    GLfloat mat_shininess[] = {spekularity};

    if ( GWater.light )
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
    while ( cnt < renderlist.num_watr )
    {
      render_water_fan_lit( renderlist.watr[cnt], 1, (( renderlist.watr[cnt] >> GWater.shift ) &2 ) + ( renderlist.watr[cnt]&1 ) );
      cnt++;
    }
  }

  // Top layer second
  if ( !CData.render_overlay && GWater.num_layer > 0 )
  {
    float ambi_level = ( GWater.lightadd_fp8[1] + GWater.lightlevel_fp8[1] ) / 255.0;
    float spek_level =  FP8_TO_FLOAT( GWater.speklevel_fp8 );
    float spekularity = MIN( 40, spek_level / ambi_level ) + 2;
    GLfloat mat_none[]      = {0, 0, 0, 0};
    GLfloat mat_ambient[]   = { ambi_level, ambi_level, ambi_level, 1.0 };
    GLfloat mat_diffuse[]   = { spek_level, spek_level, spek_level, 1.0 };
    GLfloat mat_shininess[] = {spekularity};

    if ( GWater.light )
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
    while ( cnt < renderlist.num_watr )
    {
      render_water_fan_lit( renderlist.watr[cnt], 0, (( renderlist.watr[cnt] >> GWater.shift ) &2 ) + ( renderlist.watr[cnt]&1 ) );
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
      if ( ChrList[tnc].bmpdata.shadow != 0 || CapList[ChrList[tnc].model].forceshadow && mesh_has_no_bits( ChrList[tnc].onwhichfan, MESHFX_SHINY ) )
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
//      //if(ChrList[tnc].attachedto == MAXCHR)
//      //{
//      if (ChrList[tnc].bmpdata.calc_shadowsize != 0 || CapList[ChrList[tnc].model].forceshadow && HAS_NO_BITS(Mesh[ChrList[tnc].onwhichfan].fx, MESHFX_SHINY))
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
      if ( mesh_has_some_bits( ChrList[tnc].onwhichfan, MESHFX_SHINY ) )
        render_refmad( tnc, ChrList[tnc].alpha_fp8 / 2 );
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
      texture = cnt + TX_TILE_0;
      mesh.last_texture = texture;
      GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
      for ( tnc = 0; tnc < renderlist.num_norm; tnc++ )
      {
        fan = renderlist.norm[tnc];
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
      texture = cnt + TX_TILE_0;
      mesh.last_texture = texture;
      GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
      for ( tnc = 0; tnc < renderlist.num_shine; tnc++ )
      {
        fan = renderlist.shine[tnc];
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
//      texture = cnt + TX_TILE_0;
//      mesh.last_texture = texture;
//      GLTexture_Bind(&TxTexture[texture], CData.texturefilter);
//      for (tnc = 0; tnc < renderlist.num_reflc; tnc++)
//      {
//        fan = renderlist.reflc[tnc];
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
      texture = cnt + TX_TILE_0;
      mesh.last_texture = texture;
      GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
      for ( tnc = 0; tnc < renderlist.num_shine; tnc++ )
      {
        fan = renderlist.reflc[tnc];
        render_fan_ref( fan, texture, GCamera.tracklevel );
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
//      texture = cnt + TX_TILE_0;
//      mesh.last_texture = texture;
//      GLTexture_Bind(&TxTexture[texture], CData.texturefilter);
//      for (tnc = 0; tnc < renderlist.num_reflc; tnc++)
//      {
//        fan = renderlist.reflc[tnc];
//        render_fan_ref(fan, texture, GCamera.tracklevel);
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
      if ( ChrList[tnc].alpha_fp8 == 255 && ChrList[tnc].light_fp8 == 255 )
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
      if ( ChrList[tnc].alpha_fp8 != 255 )
      {
        trans = ChrList[tnc].alpha_fp8;

        if (( ChrList[tnc].alpha_fp8 + ChrList[tnc].light_fp8 ) < SEEINVISIBLE &&  localseeinvisible && ChrList[tnc].islocalplayer )
          trans = SEEINVISIBLE - ChrList[tnc].light_fp8;

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

      if ( ChrList[tnc].sheen_fp8 == 0 && ChrList[tnc].light_fp8 == 255 && ChrList[tnc].alpha_fp8 == 255 ) continue;

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
    if ( !GWater.light )
    {
      ATTRIB_GUARD_OPEN( inp_attrib_stack );
      render_alpha_water();
      ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );
    };

    // Do self-lit water
    if ( GWater.light )
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

#if defined(DEBUG_BBOX) && defined(_DEBUG)
  if(CData.DevMode)
  {
    int i;

    for(i=0; i<MAXCHR; i++)
    {
      if(!ChrList[i].on) continue;

      mad_display_bbox_tree(2, ChrList[i].matrix, MadList + ChrList[i].model, ChrList[i].anim.last, ChrList[i].anim.next );
    }
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
//  mesh.last_texture = 0;
//  for (cnt = 0; cnt < renderlist.num_shine; cnt++)
//    render_fan(renderlist.shine[cnt]);
//
//  // Renfer sha
//  // BAD: DRAW SHADOW STUFF TOO
//  glEnable(GL_ALPHA_TEST);
//  glAlphaFunc(GL_GREATER, 0);
//  for (cnt = 0; cnt < renderlist.num_reflc; cnt++)
//    render_fan(renderlist.reflc[cnt]);
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
//      if((Mesh[ChrList[tnc].onwhichfan].fx&MESHFX_SHINY))
//        render_refmad(tnc, ChrList[tnc].alpha_fp8&ChrList[tnc].light_fp8);
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
//  mesh.last_texture = 0;
//
//  glEnable(GL_ALPHA_TEST);
//  glAlphaFunc(GL_GREATER, 0);
//  for (cnt = 0; cnt < renderlist.num_reflc; cnt++)
//    render_fan(renderlist.reflc[cnt]);
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
//        if(ChrList[tnc].attachedto == MAXCHR)
//        {
//          if(((ChrList[tnc].light_fp8==255 && ChrList[tnc].alpha_fp8==255) || CapList[ChrList[tnc].model].forceshadow) && ChrList[tnc].bmpdata.calc_shadowsize!=0)
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
//        if(ChrList[tnc].attachedto == MAXCHR)
//        {
//          if(((ChrList[tnc].light_fp8==255 && ChrList[tnc].alpha_fp8==255) || CapList[ChrList[tnc].model].forceshadow) && ChrList[tnc].bmpdata.calc_shadowsize!=0)
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
//      if(ChrList[tnc].alpha_fp8==255 && ChrList[tnc].light_fp8==255)
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
//      if(ChrList[tnc].alpha_fp8!=255 && ChrList[tnc].light_fp8==255)
//      {
//        trans = ChrList[tnc].alpha_fp8;
//        if(trans < SEEINVISIBLE && (localseeinvisible || ChrList[tnc].islocalplayer))  trans = SEEINVISIBLE;
//        render_mad(tnc, trans);
//      }
//    }
//
//  }
//
//  // And alpha water floors
//  if(!GWater.light)
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
//      if(ChrList[tnc].light_fp8!=255)
//      {
//        trans = FP8_TO_FLOAT(FP8_MUL(ChrList[tnc].light_fp8, ChrList[tnc].alpha_fp8)) * 0.5f;
//        if(trans < SEEINVISIBLE && (localseeinvisible || ChrList[tnc].islocalplayer))  trans = SEEINVISIBLE;
//        render_mad(tnc, trans);
//      }
//    }
//  }
//
//  // Do phong highlights
//  if(CData.phongon && ChrList[tnc].sheen_fp8 > 0)
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
//      envirosave = ChrList[tnc].enviro;
//      texturesave = ChrList[tnc].texture;
//      ChrList[tnc].enviro = btrue;
//      ChrList[tnc].texture = TX_PHONG;  // The phong map texture...
//      render_enviromad(tnc, (ChrList[tnc].alpha_fp8 * spek_global[ChrList[tnc].sheen_fp8][ChrList[tnc].light_fp8]) / 2, GL_TEXTURE_2D);
//      ChrList[tnc].texture = texturesave;
//      ChrList[tnc].enviro = envirosave;
//    };
//    ATTRIB_POP("zref");
//  }
//
//
//  // Do light water
//  if(GWater.light)
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

    tx_rect.left   = (( float ) BlipList[color].rect.left   ) / ( float ) GLTexture_GetTextureWidth(&TxBlip)  + 0.01;
    tx_rect.right  = (( float ) BlipList[color].rect.right  ) / ( float ) GLTexture_GetTextureWidth(&TxBlip)  - 0.01;
    tx_rect.top    = (( float ) BlipList[color].rect.top    ) / ( float ) GLTexture_GetTextureHeight(&TxBlip) + 0.01;
    tx_rect.bottom = (( float ) BlipList[color].rect.bottom ) / ( float ) GLTexture_GetTextureHeight(&TxBlip) - 0.01;

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

  tx_rect.left   = bmfont.rect[fonttype].x * dx + 0.001f;
  tx_rect.right  = ( bmfont.rect[fonttype].x + bmfont.rect[fonttype].w ) * dx - 0.001f;
  tx_rect.top    = bmfont.rect[fonttype].y * dy + 0.001f;
  tx_rect.bottom = ( bmfont.rect[fonttype].y + bmfont.rect[fonttype].h ) * dy;

  sc_rect.left   = x;
  sc_rect.right  = x + bmfont.rect[fonttype].w;
  sc_rect.top    = CData.scry - y;
  sc_rect.bottom = CData.scry - (y + bmfont.rect[fonttype].h);

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

  return y - ystt;
}

//--------------------------------------------------------------------------------------------
int draw_string( BMFont * pfnt, float x, float y, GLfloat tint[], char * szFormat, ... )
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

  BeginText( &(pfnt->tex) );
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
        cTmp = pfnt->ascii_table[cTmp];
        draw_one_font( cTmp, x, y );
        x += pfnt->spacing_x[cTmp];
      }
      cTmp = szText[cnt];
      cnt++;
    }
  }
  EndText();

  // restore the old tint
  glColor4fv(current_tint);

  return pfnt->spacing_y;
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
      x += bmfont.spacing_x[bmfont.ascii_table[cTmp]];
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
    x += bmfont.spacing_x[bmfont.ascii_table[cTmp]];
    cnt++;
    cTmp = szText[cnt];
  }
  return x;
}

//--------------------------------------------------------------------------------------------
int draw_wrap_string( BMFont * pfnt, float x, float y, GLfloat tint[], float maxx, char * szFormat, ... )
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

  BeginText( &(pfnt->tex) );
  {
    newword = btrue;
    xstt = x;
    ystt = y;
    maxx += xstt;
    newy = y + bmfont.spacing_y;

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
          x = xstt + bmfont.spacing_y;
          y += bmfont.spacing_y;
          newy += bmfont.spacing_y;
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
          y += bmfont.spacing_y;
          newy += bmfont.spacing_y;
        }
        else
        {
          // Normal letter
          cTmp = bmfont.ascii_table[cTmp];
          draw_one_font( cTmp, x, y );
          x += bmfont.spacing_x[cTmp];
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
int draw_status( BMFont * pfnt, CHR_REF character, int x, int y )
{
  // ZZ> This function shows a character's icon, status and inventory
  //     The x,y coordinates are the top left point of the image to draw
  Uint16 item;
  char cTmp;
  char *readtext;
  int ystt = y;

  float life    = FP8_TO_FLOAT( ChrList[character].life_fp8 );
  float lifemax = FP8_TO_FLOAT( ChrList[character].lifemax_fp8 );
  float mana    = FP8_TO_FLOAT( ChrList[character].mana_fp8 );
  float manamax = FP8_TO_FLOAT( ChrList[character].manamax_fp8 );
  int cnt;

  // Write the character's first name
  if ( ChrList[character].nameknown )
    readtext = ChrList[character].name;
  else
    readtext = CapList[ChrList[character].model].classname;

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
  y += 8 + draw_string( pfnt, x + 8, y, NULL, "$%4d", ChrList[character].money );


  // Draw the icons
  draw_one_icon( skintoicon[ChrList[character].texture], x + 40, y, ChrList[character].sparkle );
  item = chr_get_holdingwhich( character, SLOT_LEFT );
  if ( VALID_CHR( item ) )
  {
    if ( ChrList[item].icon )
    {
      draw_one_icon( skintoicon[ChrList[item].texture], x + 8, y, ChrList[item].sparkle );
      if ( ChrList[item].ammomax != 0 && ChrList[item].ammoknown )
      {
        if ( !CapList[ChrList[item].model].isstackable || ChrList[item].ammo > 1 )
        {
          // Show amount of ammo left
          draw_string( pfnt, x + 8, y - 8, NULL, "%2d", ChrList[item].ammo );
        }
      }
    }
    else
      draw_one_icon( bookicon + ( ChrList[item].money % MAXSKIN ), x + 8, y, ChrList[item].sparkle );
  }
  else
    draw_one_icon( nullicon, x + 8, y, NOSPARKLE );

  item = chr_get_holdingwhich( character, SLOT_RIGHT );
  if ( VALID_CHR( item ) )
  {
    if ( ChrList[item].icon )
    {
      draw_one_icon( skintoicon[ChrList[item].texture], x + 72, y, ChrList[item].sparkle );
      if ( ChrList[item].ammomax != 0 && ChrList[item].ammoknown )
      {
        if ( !CapList[ChrList[item].model].isstackable || ChrList[item].ammo > 1 )
        {
          // Show amount of ammo left
          draw_string( pfnt, x + 72, y - 8, NULL, "%2d", ChrList[item].ammo );
        }
      }
    }
    else
      draw_one_icon( bookicon + ( ChrList[item].money % MAXSKIN ), x + 72, y, ChrList[item].sparkle );
  }
  else
    draw_one_icon( nullicon, x + 72, y, NOSPARKLE );

  y += 32;

  // Draw the bars
  if ( ChrList[character].alive )
  {
    y += draw_one_bar( ChrList[character].lifecolor, x, y, life, lifemax );
    y += draw_one_bar( ChrList[character].manacolor, x, y, mana, manamax );
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
    draw_blip( BlipList[cnt].c, maprect.left + BlipList[cnt].x, maprect.top + BlipList[cnt].y );
  };

  if ( youarehereon && ( wldframe&8 ) )
  {
    for ( ipla = 0; ipla < MAXPLAYER; ipla++ )
    {
      if ( !VALID_PLA( ipla ) || INBITS_NONE == PlaList[ipla].device ) continue;

      ichr = pla_get_character( ipla );
      if ( !VALID_CHR( ichr ) || !ChrList[ichr].alive ) continue;

      draw_blip( 0, maprect.left + MAPSIZE * mesh_fraction_x( ChrList[ichr].pos.x ) * mapscale, maprect.top + MAPSIZE * mesh_fraction_y( ChrList[ichr].pos.y ) * mapscale );
    }
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
int do_messages( BMFont * pfnt, int x, int y )
{
  int cnt, tnc;
  int ystt = y;

  if ( NULL==pfnt || !CData.messageon ) return 0;

  // Display the messages
  tnc = GMsg.start;
  for ( cnt = 0; cnt < CData.maxmessage; cnt++ )
  {
    // mesages with negative times never time out!
    if ( GMsg.time[tnc] != 0 )
    {
      y += draw_wrap_string( pfnt, x, y, NULL, CData.scrx - CData.wraptolerance - x, GMsg.textdisplay[tnc] );

      if ( GMsg.time[tnc] > 0 )
      {
        GMsg.time[tnc] -= GMsg.timechange;
        if ( GMsg.time[tnc] < 0 ) GMsg.time[tnc] = 0;
      };
    }
    tnc++;
    tnc %= CData.maxmessage;
  }

  return y - ystt;
}

//--------------------------------------------------------------------------------------------
int do_status( BMFont * pfnt, int x, int y)
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
void draw_text( BMFont *  pfnt )
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
      y += draw_string( pfnt, 0, y, NULL, "OUT OF SYNC!" );
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
        y += draw_string( pfnt, 0, y, NULL, "<%3.2f,%3.2f,%3.2f>", ChrList[pla_chr].pos.x, ChrList[pla_chr].pos.y, ChrList[pla_chr].pos.z );
        y += draw_string( pfnt, 0, y, NULL, "<%3.2f,%3.2f,%3.2f>", ChrList[pla_chr].vel.x, ChrList[pla_chr].vel.y, ChrList[pla_chr].vel.z );
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
          y += draw_string( pfnt, 0, y, tint.v, "PLA0 holder == %s(%s)", ChrList[iref].name, CapList[ChrList[iref].model].classname );
        };

        iref = chr_get_inwhichpack(ichr);
        if( VALID_CHR(iref) )
        {
          y += draw_string( pfnt, 0, y, tint.v, "PLA0 packer == %s(%s)", ChrList[iref].name, CapList[ChrList[iref].model].classname );
        };

        iref = chr_get_onwhichplatform(ichr);
        if( VALID_CHR(iref) )
        {
          y += draw_string( pfnt, 0, y, tint.v, "PLA0 platform == %s(%s)", ChrList[iref].name, CapList[ChrList[iref].model].classname );
        };

      };
    }

    if ( SDLKEYDOWN( SDLK_F1 ) )
    {
      // In-Game help
      y += draw_string( pfnt, 0, y, NULL, "!!!MOUSE HELP!!!" );
      y += draw_string( pfnt, 0, y, NULL, "~Edit CONTROLS.TXT to change" );
      y += draw_string( pfnt, 0, y, NULL, "~Left Click to use an item" );
      y += draw_string( pfnt, 0, y, NULL, "~Left and Right Click to grab" );
      y += draw_string( pfnt, 0, y, NULL, "~Middle Click to jump" );
      y += draw_string( pfnt, 0, y, NULL, "~A and S keys do stuff" );
      y += draw_string( pfnt, 0, y, NULL, "~Right Drag to move camera" );
    }

    if ( SDLKEYDOWN( SDLK_F2 ) )
    {
      // In-Game help
      y += draw_string( pfnt, 0, y, NULL, "!!!JOYSTICK HELP!!!" );
      y += draw_string( pfnt, 0, y, NULL, "~Edit CONTROLS.TXT to change" );
      y += draw_string( pfnt, 0, y, NULL, "~Hit the buttons" );
      y += draw_string( pfnt, 0, y, NULL, "~You'll figure it out" );
    }

    if ( SDLKEYDOWN( SDLK_F3 ) )
    {
      // In-Game help
      y += draw_string( pfnt, 0, y, NULL, "!!!KEYBOARD HELP!!!" );
      y += draw_string( pfnt, 0, y, NULL, "~Edit CONTROLS.TXT to change" );
      y += draw_string( pfnt, 0, y, NULL, "~TGB controls left hand" );
      y += draw_string( pfnt, 0, y, NULL, "~YHN controls right hand" );
      y += draw_string( pfnt, 0, y, NULL, "~Keypad to move and jump" );
      y += draw_string( pfnt, 0, y, NULL, "~Number keys for stats" );
    }


    // PLAYER DEBUG MODE
    if ( SDLKEYDOWN( SDLK_F5 ) && CData.DevMode )
    {
      CHR_REF pla_chr = pla_get_character( 0 );

      y += draw_string( pfnt, 0, y, NULL, "!!!DEBUG MODE-5!!!" );
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f", GCamera.pos.x, GCamera.pos.y );

      y += draw_string( pfnt, 0, y, NULL, "  PLA0DEF %d %d %d %d %d %d %d %d",
                ChrList[pla_chr].damagemodifier_fp8[0]&DAMAGE_SHIFT,
                ChrList[pla_chr].damagemodifier_fp8[1]&DAMAGE_SHIFT,
                ChrList[pla_chr].damagemodifier_fp8[2]&DAMAGE_SHIFT,
                ChrList[pla_chr].damagemodifier_fp8[3]&DAMAGE_SHIFT,
                ChrList[pla_chr].damagemodifier_fp8[4]&DAMAGE_SHIFT,
                ChrList[pla_chr].damagemodifier_fp8[5]&DAMAGE_SHIFT,
                ChrList[pla_chr].damagemodifier_fp8[6]&DAMAGE_SHIFT,
                ChrList[pla_chr].damagemodifier_fp8[7]&DAMAGE_SHIFT );

      y += draw_string( pfnt, 0, y, NULL, "~PLA0 %5.1f %5.1f", ChrList[pla_chr].pos.x / 128.0, ChrList[pla_chr].pos.y / 128.0  );

      pla_chr = pla_get_character( 1 );
      y += draw_string( pfnt, 0, y, NULL, "~PLA1 %5.1f %5.1f", ChrList[pla_chr].pos.x / 128.0, ChrList[pla_chr].pos.y / 128.0 );
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
      y += draw_string( pfnt, 0, y, NULL, "~FOGAFF %d", GFog.affectswater );
      y += draw_string( pfnt, 0, y, NULL, "~PASS %d/%d", numshoppassage, numpassage );
      y += draw_string( pfnt, 0, y, NULL, "~NETPLAYERS %d", GNet.num_player );
      y += draw_string( pfnt, 0, y, NULL, "~DAMAGEPART %d", GTile_Dam.parttype );
    }

    // CAMERA DEBUG MODE
    if ( SDLKEYDOWN( SDLK_F7 ) && CData.DevMode )
    {
      // White debug mode
      y += draw_string( pfnt, 0, y, NULL, "!!!DEBUG MODE-7!!!" );
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f %f %f", ( GCamera.mView ) _CNV( 0, 0 ), ( GCamera.mView ) _CNV( 1, 0 ), ( GCamera.mView ) _CNV( 2, 0 ), ( GCamera.mView ) _CNV( 3, 0 ) );
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f %f %f", ( GCamera.mView ) _CNV( 0, 1 ), ( GCamera.mView ) _CNV( 1, 1 ), ( GCamera.mView ) _CNV( 2, 1 ), ( GCamera.mView ) _CNV( 3, 1 ) );
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f %f %f", ( GCamera.mView ) _CNV( 0, 2 ), ( GCamera.mView ) _CNV( 1, 2 ), ( GCamera.mView ) _CNV( 2, 2 ), ( GCamera.mView ) _CNV( 3, 2 ) );
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f %f %f", ( GCamera.mView ) _CNV( 0, 3 ), ( GCamera.mView ) _CNV( 1, 3 ), ( GCamera.mView ) _CNV( 2, 3 ), ( GCamera.mView ) _CNV( 3, 3 ) );
      y += draw_string( pfnt, 0, y, NULL, "~x %f, %f", GCamera.centerpos.x, GCamera.trackpos.x );
      y += draw_string( pfnt, 0, y, NULL, "~y %f %f", GCamera.centerpos.y, GCamera.trackpos.y );
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
    if ( GNet.messagemode )
    {
      y += draw_wrap_string( pfnt, 0, y, NULL, CData.scrx - CData.wraptolerance, GNet.message );
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
    render_background( TX_WATER_LOW );   // TX_WATER_LOW == 6 is the texture for waterlow.bmp
  }

  draw_scene_zreflection();

  //Foreground overlay
  if ( CData.render_overlay )
  {
    render_foreground_overlay( TX_WATER_TOP );   // TX_WATER_TOP ==  5 is watertop.bmp
  }

  End3DMode();
}

//--------------------------------------------------------------------------------------------
void draw_main( float frameDuration )
{
  // ZZ> This function does all the drawing stuff

  draw_scene();

  draw_text( &bmfont );

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
  else
  {
    log_warning( "Could not write to %s\n", CData.modules_file );
  }

  // Search for .mod directories
  FileName = fs_findFirstFile( CData.modules_dir, "mod" );
  globalnummodule = 0;
  while ( FileName && globalnummodule < MAXMODULE )
  {
    strncpy( ModList[globalnummodule].loadname, FileName, sizeof( ModList[globalnummodule].loadname ) );
    snprintf( loadname, sizeof( loadname ), "%s/%s/%s/%s", CData.modules_dir, FileName, CData.gamedat_dir, CData.mnu_file );
    if ( get_module_data( globalnummodule, loadname ) )
    {
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s/%s", CData.modules_dir, FileName, CData.gamedat_dir, CData.title_bitmap );
      if ( load_one_title_image( globalnummodule, CStringTmp1 ) )
      {
        fprintf( filesave, "%02d.  %s\n", globalnummodule, ModList[globalnummodule].longname );
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
    BlipList[cnt].rect.left   = ( cnt + 0 ) * blipwidth;
    BlipList[cnt].rect.right  = ( cnt + 1 ) * blipwidth;
    BlipList[cnt].rect.top    = 0;
    BlipList[cnt].rect.bottom = blipheight;
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
  }
  else
  {
    log_message("Succeeded!\n");
  }

  atexit( SDL_Quit );

 //Force OpenGL hardware acceleration (This must be done before video mode)
  if(CData.gfxacceleration)
  {
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  }

  // log the video driver info
  SDL_VideoDriverName( strbuffer, 256 );
  log_info( "Using Video Driver - %s\n", strbuffer );

  colordepth = CData.scrd / 3;

  /* Setup the cute windows manager icon */
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.icon_bitmap );
  theSurface = SDL_LoadBMP( CStringTmp1 );
  if ( theSurface == NULL )
  {
    log_warning( "Unable to load icon (%s)\n", CStringTmp1 );
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

  //Enable antialiasing X16
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
  glEnable(GL_MULTISAMPLE_ARB);
  //glEnable(GL_MULTISAMPLE);

  ////Force OpenGL hardware acceleration
  //if(CData.gfxacceleration)
  //{
  //  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  //}



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

  input_setup();
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

  // Wait for vertical synchronization?
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
//--------------------------------------------------------------------------------------------
//void do_cursor()
//{
//  // This function implements a mouse cursor
//  read_input();
//
//  cursor.x = mous.latch.x;  if ( cursor.x < 6 )  cursor.x = 6;  if ( cursor.x > CData.scrx - 16 )  cursor.x = CData.scrx - 16;
//  cursor.y = mous.latch.y;  if ( cursor.y < 8 )  cursor.y = 8;  if ( cursor.y > CData.scry - 24 )  cursor.y = CData.scry - 24;
//  cursor.clicked = bfalse;
//  if ( mous.button[0] && !cursor.pressed )
//  {
//    cursor.clicked = btrue;
//  }
//  cursor.pressed = mous.button[0];
//
//  BeginText( &(bmfont.tex) );
//  {
//    draw_one_font( 95, cursor.x - 5, cursor.y - 7 );
//  }
//  EndText();
//}
#endif
