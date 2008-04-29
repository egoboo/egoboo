/* Egoboo - module.c
 * Handles ingame maps and levels, called Modules
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

#include "Log.h"
#include "sound.h"
#include "Mesh.h"
#include "Particle.h"
#include "script.h"
#include "menu.h"

#include "egoboo_strutil.h"
#include "egoboo_utility.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
void release_module( void )
{
  // ZZ> This function frees up memory used by the module

  if(!moduleActive) return;

  release_all_textures();
  release_all_icons();
  release_map();

  // Close and then reopen SDL_mixer; it's easier than manually unloading each sound
  if ( mixeron)
  {
    Mix_CloseAudio();
    songplaying = -1;
    Mix_OpenAudio( MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, CData.buffersize );
    Mix_AllocateChannels( CData.maxsoundchannel );
  }
}

//--------------------------------------------------------------------------------------------
bool_t module_reference_matches( char *szLoadName, IDSZ idsz )
{
  // ZZ> This function returns btrue if the named module has the required IDSZ
  FILE *fileread;
  STRING newloadname;
  IDSZ newidsz;
  bool_t foundidsz;
  int cnt;


  if ( szLoadName[0] == 'N' && szLoadName[1] == 'O' && szLoadName[2] == 'N' && szLoadName[3] == 'E' && szLoadName[4] == 0 )
    return btrue;

  if ( idsz == IDSZ_NONE )
    return btrue;

  foundidsz = bfalse;
  snprintf( newloadname, sizeof( newloadname ), "%s/%s/%s/%s", CData.modules_dir, szLoadName, CData.gamedat_dir, CData.mnu_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "r" );
  if ( NULL == fileread ) return bfalse;

  // Read basic data
  globalname = szLoadName;
  fgoto_colon( fileread );   // Name of module...  Doesn't matter
  fgoto_colon( fileread );   // Reference directory...
  fgoto_colon( fileread );   // Reference IDSZ...
  fgoto_colon( fileread );   // Import...
  fgoto_colon( fileread );   // Export...
  fgoto_colon( fileread );   // Min players...
  fgoto_colon( fileread );   // Max players...
  fgoto_colon( fileread );   // Respawn...
  fgoto_colon( fileread );   // RTS... (OUTDATED)
  fgoto_colon( fileread );   // Rank...


  // Summary...
  cnt = 0;
  while ( cnt < SUMMARYLINES )
  {
    fgoto_colon( fileread );
    cnt++;
  }


  // Now check expansions
  while ( fgoto_colon_yesno( fileread ) && !foundidsz )
  {
    newidsz = fget_idsz( fileread );
    if ( newidsz == idsz )
    {
      foundidsz = btrue;
    }
  }


  fs_fileClose( fileread );

  return foundidsz;
}

//--------------------------------------------------------------------------------------------
void add_module_idsz( char *szLoadName, IDSZ idsz )
{
  // ZZ> This function appends an IDSZ to the module's menu.txt file
  FILE *filewrite;
  STRING newloadname;

  // Only add if there isn't one already
  if ( !module_reference_matches( szLoadName, idsz ) )
  {
    // Try to open the file in append mode
    snprintf( newloadname, sizeof( newloadname ), "%s/%s/%s/%s", CData.modules_dir, szLoadName, CData.gamedat_dir, CData.mnu_file );
    filewrite = fs_fileOpen( PRI_NONE, NULL, newloadname, "a" );
    if ( filewrite )
    {
      fprintf( filewrite, "\n:[%4s]\n", undo_idsz( idsz ) );
      fs_fileClose( filewrite );
    }
  }
}

//--------------------------------------------------------------------------------------------
int find_module( char *smallname )
{
  // ZZ> This function returns -1 if the module does not exist locally, the module
  //     index otherwise

  int cnt, index;
  cnt = 0;
  index = -1;
  while ( cnt < globalnummodule )
  {
    if ( strcmp( smallname, ModList[cnt].loadname ) == 0 )
    {
      index = cnt;
      cnt = globalnummodule;
    }
    cnt++;
  }
  return index;
}

//--------------------------------------------------------------------------------------------
void load_module( char *smallname )
{
  // ZZ> This function loads a module
  STRING modname;

  beatmodule = bfalse;
  timeron = bfalse;
  snprintf( modname, sizeof( modname ), "%s/%s/", CData.modules_dir, smallname );

  make_randie();

  reset_teams();

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.nullicon_bitmap );
  load_one_icon( CStringTmp1 );

  load_global_waves( modname );

  reset_particles( modname );

  read_wawalite( modname );

  make_twist();

  reset_messages();

  prime_names();

  load_basic_textures( modname );

  reset_ai_script();


  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.script_file );
  if ( MAXAI == load_ai_script( CStringTmp1 ) );
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.script_file );
    load_ai_script( CStringTmp1 );
  };

  release_all_models();

  free_all_enchants();

  load_all_objects( modname );  // This is broken and needs to be fixed (is it really?)

  if ( !load_mesh( modname ) )
  {
    log_error( "Load problems with the mesh.\n" );
  }

  setup_particles();

  setup_passage( modname );

  reset_players();

  setup_characters( modname );

  reset_end_text();

  setup_alliances( modname );

  // Load fonts and bars after other images, as not to hog videomem
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.font_bitmap );
  snprintf( CStringTmp2, sizeof( CStringTmp2 ), "%s%s/%s", modname, CData.gamedat_dir, CData.fontdef_file );
  if ( !load_font( CStringTmp1, CStringTmp2 ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.font_bitmap );
    snprintf( CStringTmp2, sizeof( CStringTmp2 ), "%s/%s", CData.basicdat_dir, CData.fontdef_file );
    if ( !load_font( CStringTmp1, CStringTmp2 ) )
    {
      log_warning( "Fonts not loaded.  Files missing from %s directory\n", CData.basicdat_dir );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.bars_bitmap );
  if ( !load_bars( CStringTmp1 ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.bars_bitmap );
    if ( !load_bars( CStringTmp1 ) )
    {
      log_warning( "Could not load status bars. File missing = \"%s\"\n", CStringTmp1 );
    }
  };

  load_map( modname );
  load_blip_bitmap( modname );

  if ( CData.DevMode )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.debug_file );
    log_madused( CData.debug_file );
  };

}

//--------------------------------------------------------------------------------------------
bool_t get_module_data( int modnumber, char *szLoadName )
{
  // ZZ> This function loads the module data file
  FILE *fileread;
  char reference[128];
  STRING playername;
  Uint32 idsz;
  int iTmp;
  bool_t playerhasquest;

  fileread = fs_fileOpen( PRI_NONE, NULL, szLoadName, "r" );
  if ( NULL != fileread )
  {
    // Read basic data
    globalname = szLoadName;
    fget_next_name( fileread, ModList[modnumber].longname, sizeof( ModList[modnumber].longname ) );
    fget_next_string( fileread, reference, sizeof( reference ) );
    idsz = fget_next_idsz( fileread );

    //Check all selected players directories
    playerhasquest = bfalse;
    iTmp = 0;
    while ( !playerhasquest && iTmp < numloadplayer)
    {
      snprintf( playername, sizeof( playername ), "%s", loadplayerdir[iTmp] );
      if( check_player_quest( playername, idsz ) >= 0 ) playerhasquest = btrue;
      iTmp++;
    }

    //Check for unlocked modules (Both in Quest IDSZ and Module IDSZ). Skip this if in DevMode.
    if( CData.DevMode || playerhasquest || module_reference_matches( reference, idsz ) )
    {
      globalname = szLoadName;
      ModList[modnumber].importamount = fget_next_int( fileread );
      ModList[modnumber].allowexport = fget_next_bool( fileread );
      ModList[modnumber].minplayers = fget_next_int( fileread );
      ModList[modnumber].maxplayers = fget_next_int( fileread );
      ModList[modnumber].respawnvalid = fget_next_respawn( fileread );
      fget_next_bool( fileread );  // ModList[modnumber].GRTS.control
      fget_next_string( fileread, generictext, sizeof( generictext ) );
      iTmp = 0;
      while ( iTmp < RANKSIZE - 1 )
      {
        ModList[modnumber].rank[iTmp] = generictext[iTmp];
        iTmp++;
      }
      ModList[modnumber].rank[iTmp] = 0;



      // Read the expansions
      return btrue;
    }
  }
  return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t get_module_summary( char *szLoadName )
{
  // ZZ> This function gets the quest description out of the module's menu file
  FILE *fileread;
  char szLine[160];
  int cnt;
  bool_t result = bfalse;

  fileread = fs_fileOpen( PRI_NONE, NULL, szLoadName, "r" );
  if ( NULL != fileread )
  {
    // Skip over basic data
    globalname = szLoadName;
    fgoto_colon( fileread );   // Name...
    fgoto_colon( fileread );   // Reference...
    fgoto_colon( fileread );   // IDSZ...
    fgoto_colon( fileread );   // Import...
    fgoto_colon( fileread );   // Export...
    fgoto_colon( fileread );   // Min players...
    fgoto_colon( fileread );   // Max players...
    fgoto_colon( fileread );   // Respawn...
    fgoto_colon( fileread );   // Not Used
    fgoto_colon( fileread );   // Rank...


    // Read the summary
    cnt = 0;
    while ( cnt < SUMMARYLINES )
    {
      fget_next_string( fileread, szLine, sizeof( szLine ) );
      convert_underscores( modsummary[cnt], sizeof( modsummary[cnt] ), szLine );
      cnt++;
    }
    result = btrue;
  }

  fs_fileClose( fileread );
  return result;
}



