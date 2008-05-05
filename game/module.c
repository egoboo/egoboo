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
#include "graphic.h"
#include "enchant.h"

#include "egoboo_strutil.h"
#include "egoboo_utility.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------

static Uint16 load_one_object( int skin, char * szObjectpath, char* szObjectname );
static void   load_all_objects( char * szModname );

//--------------------------------------------------------------------------------------------
void release_bumplist(void)
{
  FREE(bumplist.chr_list);
  FREE(bumplist.prt_list);

  bumplist_renew( &bumplist );
};

//--------------------------------------------------------------------------------------------
void module_release( void )
{
  // ZZ> This function frees up memory used by the module

  if(!moduleActive) return;

  release_all_textures();
  release_all_icons();
  release_map();
  release_bumplist();

  // Close and then reopen SDL_mixer; it's easier than manually unloading each sound
  if ( mixeron )
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

  if ( 0 == strcmp(szLoadName, "NONE") ) return btrue;
  if ( idsz == IDSZ_NONE ) return btrue;

  foundidsz = bfalse;
  snprintf( newloadname, sizeof( newloadname ), "%s" SLASH_STRING "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.modules_dir, szLoadName, CData.gamedat_dir, CData.mnu_file );
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
    snprintf( newloadname, sizeof( newloadname ), "%s" SLASH_STRING "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.modules_dir, szLoadName, CData.gamedat_dir, CData.mnu_file );
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
void module_load( char *smallname )
{
  // ZZ> This function loads a module
  STRING szModpath;

  beatmodule = bfalse;
  timeron = bfalse;
  snprintf( szModpath, sizeof( szModpath ), "%s" SLASH_STRING "%s" SLASH_STRING, CData.modules_dir, smallname );

  make_randie();

  reset_teams();

  load_one_icon( CData.basicdat_dir, NULL, CData.nullicon_bitmap );

  load_global_waves( szModpath );

  reset_particles( szModpath );

  mad_clear_pips();

  read_wawalite( szModpath );

  make_twist();

  reset_messages();

  prime_names();

  load_basic_textures( szModpath );

  reset_ai_script();


  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s", szModpath, CData.gamedat_dir );
  if ( MAXAI == load_ai_script( CStringTmp1, NULL ) );
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s", CData.basicdat_dir, CData.script_file );
    load_ai_script( CStringTmp1, NULL );
  };

  release_all_models();

  free_all_enchants();

  load_all_objects( szModpath );

  if ( !load_mesh( szModpath ) )
  {
    log_error( "Load problems with the mesh.\n" );
  }

  setup_particles();

  setup_passage( szModpath );

  reset_players();

  setup_characters( szModpath );

  reset_end_text();

  setup_alliances( szModpath );

  // Load fonts and bars after other images, as not to hog videomem
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", szModpath, CData.gamedat_dir, CData.font_bitmap );
  snprintf( CStringTmp2, sizeof( CStringTmp2 ), "%s%s" SLASH_STRING "%s", szModpath, CData.gamedat_dir, CData.fontdef_file );
  if ( !load_font( CStringTmp1, CStringTmp2 ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.font_bitmap );
    snprintf( CStringTmp2, sizeof( CStringTmp2 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.fontdef_file );
    if ( !load_font( CStringTmp1, CStringTmp2 ) )
    {
      log_warning( "Fonts not loaded.  Files missing from %s directory\n", CData.basicdat_dir );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", szModpath, CData.gamedat_dir, CData.bars_bitmap );
  if ( !load_bars( CStringTmp1 ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.bars_bitmap );
    if ( !load_bars( CStringTmp1 ) )
    {
      log_warning( "Could not load status bars. File missing = \"%s\"\n", CStringTmp1 );
    }
  };

  load_map( szModpath );
  load_blip_bitmap( szModpath );

  if ( CData.DevMode )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.debug_file );
    log_madused( CData.debug_file );
  };

}

//--------------------------------------------------------------------------------------------
bool_t module_read_data( int modnumber, char *szLoadName )
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
    while ( !playerhasquest && iTmp < loadplayer_count)
    {
      snprintf( playername, sizeof( playername ), "%s", loadplayer[iTmp].dir );
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
bool_t module_read_summary( char *szLoadName )
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
      str_convert_underscores( modsummary[cnt], sizeof( modsummary[cnt] ), szLine );
      cnt++;
    }
    result = btrue;
  }

  fs_fileClose( fileread );
  return result;
}



//--------------------------------------------------------------------------------------------
void load_all_objects( char * szModpath )
{
  // ZZ> This function loads a module's objects
  const char *filehandle;
  FILE* fileread;
  STRING szObjectpath, szTempdir, tmpstr;
  int cnt;
  int skin;
  FS_FIND_INFO fs_finfo;

  fs_find_info_new( &fs_finfo );

  // Clear the import slots...
  import_info_clear(&import);

  // Load the import directory
  skin = TX_LAST;  // Character skins start just after the last special texture
  if ( import.valid )
  {
    for ( cnt = 0; cnt < MAXIMPORT; cnt++ )
    {
      STRING objectname;

      snprintf( objectname, sizeof( objectname ), "temp%04d.obj", cnt );

      // Make sure the object exists...
      snprintf( szTempdir, sizeof( szTempdir ), "%s" SLASH_STRING "%s", CData.import_dir, objectname );
      if ( !fs_fileIsDirectory(szTempdir) ) continue;

      // Load it...
      import.player = cnt / 9;

      import_info_add(&import, cnt);

      skin += load_one_object( skin, CData.import_dir, objectname );
    }
  }

  empty_import_directory();  // Free up that disk space...

  // If in Developer mode, create a new debug.txt file for debug info logging
  log_debug( "DEBUG INFORMATION FOR MODULE: \"%s\"\n", szModpath );
  log_debug( "This document logs extra debugging information for the last module loaded.\n");
  log_debug( "\nSpawning log after module has started...\n");
  log_debug( "-----------------------------------------------\n" );


  // Search for .obj directories and load them
  import.object = -100;
  snprintf( szObjectpath, sizeof( szObjectpath ), "%s%s" SLASH_STRING, szModpath, CData.objects_dir );

  filehandle = fs_findFirstFile( &fs_finfo, szObjectpath, NULL, "*.obj" );
  while ( NULL != filehandle )
  {
    strcpy(tmpstr, filehandle);
    skin += load_one_object( skin, szObjectpath, tmpstr );
    filehandle = fs_findNextFile(&fs_finfo);
  }

  fs_findClose(&fs_finfo);
}

//--------------------------------------------------------------------------------------------
Uint16 load_one_object( int skin_count, char * szObjectpath, char* szObjectname )
{
  // ZZ> This function loads one iobj and returns the number of skins
  Uint16 iobj;
  int numskins, numicon, skin_index;
  STRING newloadname, loc_loadpath, wavename;
  int cnt;
  FILE * ftemp;

  // generate an index for this object
  snprintf( newloadname, sizeof( newloadname ), "%s%s" SLASH_STRING "%s", szObjectpath, szObjectname, CData.data_file );
  iobj = object_generate_index(newloadname);

  // Append a slash to the szObjectname
  strncpy( loc_loadpath, szObjectpath, sizeof( loc_loadpath ) );
  str_append_slash( loc_loadpath, sizeof( loc_loadpath ) );
  strncat( loc_loadpath, szObjectname, sizeof( loc_loadpath ) );
  str_append_slash( loc_loadpath, sizeof( loc_loadpath ) );

  // Load the iobj data file
  load_one_cap( szObjectpath, szObjectname, iobj );

  // load the model data
  load_one_mad( szObjectpath, szObjectname, iobj );

  // Fix lighting if need be
  if ( CapList[iobj].uniformlit )
  {
    make_mad_equally_lit( iobj );
  }

  // Load the messages for this object
  load_all_messages( szObjectpath, szObjectname, iobj );


  // Load the random naming table for this object
  read_naming( szObjectpath, szObjectname, iobj );

  // Load the particles for this object
  for ( cnt = 0; cnt < PRTPIP_PEROBJECT_COUNT; cnt++ )
  {
    snprintf( newloadname, sizeof( newloadname ), "part%d.txt", cnt );
    MadList[iobj].prtpip[cnt] = load_one_pip( szObjectpath, szObjectname, newloadname, -1 );
  }


  // Load the waves for this object
  for ( cnt = 0; cnt < MAXWAVE; cnt++ )
  {
    snprintf( wavename, sizeof( wavename ), "sound%d.wav", cnt );
    CapList[iobj].wavelist[cnt] = Mix_LoadWAV( inherit_fname(szObjectpath, szObjectname, wavename) );
  }


  // Load the enchantment for this object
  load_one_eve( loc_loadpath, szObjectname, iobj );


  // Load the skins and icons
  MadList[iobj].skinstart = skin_count;
  numskins = 0;
  numicon = 0;
  for ( skin_index = 0; skin_index < MAXSKIN; skin_index++ )
  {
    STRING fname;
    Uint32 tx_index;

    // try various file types
    snprintf( fname, sizeof( newloadname ), "tris%d.png", skin_index );
    tx_index = GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[skin_count+numskins], inherit_fname(szObjectpath, szObjectname, fname), TRANSCOLOR );

    if(INVALID_TEXTURE == tx_index)
    {
      snprintf( fname, sizeof( newloadname ), "tris%d.bmp", skin_index );
      tx_index = GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[skin_count+numskins], inherit_fname(szObjectpath, szObjectname, fname), TRANSCOLOR );
    }

    if(INVALID_TEXTURE == tx_index)
    {
      snprintf( fname, sizeof( newloadname ), "tris%d.pcx", skin_index );
      tx_index = GLTexture_Load( GL_TEXTURE_2D,  &TxTexture[skin_count+numskins], inherit_fname(szObjectpath, szObjectname, fname), TRANSCOLOR );
    }

    if ( INVALID_TEXTURE != tx_index )
    {
      numskins++;

      snprintf( fname, sizeof( newloadname ), "icon%d.png", skin_index );
      tx_index = GLTexture_Load( GL_TEXTURE_2D,  &TxIcon[globalnumicon], inherit_fname(szObjectpath, szObjectname, fname), INVALID_KEY );

      if ( INVALID_TEXTURE == tx_index )
      {
        snprintf( fname, sizeof( newloadname ), "icon%d.bmp", skin_index );
        tx_index = GLTexture_Load( GL_TEXTURE_2D,  &TxIcon[globalnumicon], inherit_fname(szObjectpath, szObjectname, fname), INVALID_KEY );
      }

      if ( INVALID_TEXTURE == tx_index )
      {
        snprintf( fname, sizeof( newloadname ), "icon%d.pcx", skin_index );
        tx_index = GLTexture_Load( GL_TEXTURE_2D,  &TxIcon[globalnumicon], inherit_fname(szObjectpath, szObjectname, fname), INVALID_KEY );
      }

      if( INVALID_TEXTURE != tx_index )
      {
        if ( iobj == SPELLBOOK && bookicon == 0 )
        {
          bookicon = globalnumicon;
        }

        while ( numicon < numskins )
        {
          skintoicon[skin_count+numicon] = globalnumicon;
          numicon++;
        }

        globalnumicon++;
      }
    }
  }

  MadList[iobj].skins = numskins;
  if ( numskins == 0 )
  {
    // If we didn't get a skin_count, set it to the water texture
    MadList[iobj].skinstart = TX_WATER_TOP;
    MadList[iobj].skins = 1;
  }


  return numskins;
}

