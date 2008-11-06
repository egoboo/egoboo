/* Egoboo - module.c
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
    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#include "egoboo.h"
#include "Log.h"

//--------------------------------------------------------------------------------------------
void release_module( void )
{
  // ZZ> This function frees up memory used by the module
  release_all_textures();
  release_all_icons();
  release_map();

  // Close and then reopen SDL_mixer; it's easier than manually unloading each sound
  if ( musicvalid || soundvalid )
  {
    Mix_CloseAudio();
    songplaying = -1;
    Mix_OpenAudio( MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, buffersize );
    Mix_AllocateChannels( maxsoundchannel );
  }
}

//--------------------------------------------------------------------------------------------
int module_reference_matches( char *szLoadName, Uint32 idsz )
{
  // ZZ> This function returns btrue if the named module has the required IDSZ
  FILE *fileread;
  char newloadname[256];
  Uint32 newidsz;
  int foundidsz;
  int cnt;

  if ( szLoadName[0] == 'N' && szLoadName[1] == 'O' && szLoadName[2] == 'N' && szLoadName[3] == 'E' && szLoadName[4] == 0 )
    return btrue;
  if ( idsz == IDSZNONE )
    return btrue;

  // Developers mode - unlock everything.
  if ( gDevMode ) return btrue;

  foundidsz = bfalse;
  sprintf( newloadname, "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "menu.txt", szLoadName );
  fileread = fopen( newloadname, "r" );
  if ( fileread )
  {
    // Read basic data
    globalname = szLoadName;
    goto_colon( fileread );  // Name of module...  Doesn't matter
    goto_colon( fileread );  // Reference directory...
    goto_colon( fileread );  // Reference IDSZ...
    goto_colon( fileread );  // Import...
    goto_colon( fileread );  // Export...
    goto_colon( fileread );  // Min players...
    goto_colon( fileread );  // Max players...
    goto_colon( fileread );  // Respawn...
    goto_colon( fileread );  // BAD! NOT USED
    goto_colon( fileread );  // Rank...


    // Summary...
    cnt = 0;
    while ( cnt < SUMMARYLINES )
    {
      goto_colon( fileread );
      cnt++;
    }


    // Now check expansions
    while ( goto_colon_yesno( fileread ) && !foundidsz )
    {
      newidsz = get_idsz( fileread );
      if ( newidsz == idsz )
      {
        foundidsz = btrue;
      }
    }


    fclose( fileread );
  }
  return foundidsz;
}

//--------------------------------------------------------------------------------------------
void add_module_idsz( char *szLoadName, Uint32 idsz )
{
  // ZZ> This function appends an IDSZ to the module's menu.txt file
  FILE *filewrite;
  char newloadname[256];
  char chara, charb, charc, chard;

  // Only add if there isn't one already
  if ( !module_reference_matches( szLoadName, idsz ) )
  {
    // Try to open the file in append mode
    sprintf( newloadname, "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "menu.txt", szLoadName );
    filewrite = fopen( newloadname, "a" );
    if ( filewrite )
    {
      chara = ( ( idsz >> 15 ) & 31 ) + 'A';
      charb = ( ( idsz >> 10 ) & 31 ) + 'A';
      charc = ( ( idsz >> 5 ) & 31 ) + 'A';
      chard = ( ( idsz ) & 31 ) + 'A';
      fprintf( filewrite, "\n:[%c%c%c%c]\n", chara, charb, charc, chard );
      fclose( filewrite );
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
    if ( strcmp( smallname, modloadname[cnt] ) == 0 )
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
  char modname[128];

  // printf("in load_module\n");

  // Load all the global icons
  if ( !load_all_global_icons() ) log_warning( "Could not load all global icons!\n" );

  beatmodule = bfalse;
  timeron = bfalse;
  sprintf( modname, "modules" SLASH_STR "%s" SLASH_STR, smallname );
  make_randie();  // This should work
  // printf("randie done\n");
  reset_teams();      // This should work
  // printf("reset_teams done\n");
  load_one_icon( "basicdat" SLASH_STR "nullicon.bmp" );  // This works (without transparency)
  // printf("load_one_icon done\n");
  if ( soundvalid ) load_global_waves( modname );
  reset_particles( modname );  // This should work
  read_wawalite( modname ); // This should work
  // printf("read_wawa done\n");
  make_twist();             // This should work
  // printf("make_twist done\n");
  reset_messages();         // This should work
  // printf("reset messages done\n");
  prime_names();            // This should work
  // printf("prime_names done\n");
  load_basic_textures( modname );  // This should work (without colorkey stuff)
  // printf("load_basic_tex done\n");
  reset_ai_script();        // This should work
  // printf("reset_ai_script done\n");
  load_ai_script( "basicdat" SLASH_STR "script.txt" );  // This should work
  // printf("load_ai_script done\n");
  release_all_models();     // This should work
  // printf("release_all_models done\n");
  free_all_enchants();      // This should work
  // printf("free_all_enchants done\n");

  // printf("Got to load_all_objects\n");
  load_all_objects( modname ); // This is broken and needs to be fixed (is it really?)
  // load_one_object(0, "basicdat" SLASH_STR "objects" SLASH_STR "book.obj");

  //  printf("Got to load mesh\n");
  if ( !load_mesh( modname ) )
  {
    log_error( "Uh oh! Problems loading the mesh! (%s)\n", modname );
  }
  // printf("Got to setup_particles\n");
  setup_particles();
  // printf("Got to setup_passage\n");
  setup_passage( modname );
  // printf("Got to reset_players\n");

  reset_players();
  // printf("Got to setup_characters\n");

  setup_characters( modname );
//#endif // SDL_LIL_ENDIAN

  //  printf("Got to reset_end_text\n");
  reset_end_text();

  // printf("Got to setup_alliances\n");
  setup_alliances( modname );

  // Load fonts and bars after other images, as not to hog videomem
  // printf("Got to load_font\n");
  load_font( "basicdat" SLASH_STR "font.bmp", "basicdat" SLASH_STR "font.txt", bfalse );
  // printf("Got to load_bars\n");
  load_bars( "basicdat" SLASH_STR "bars.bmp" );
  // printf("Got to load_map\n");
  load_map( modname );
  // printf("Got to log_madused\n");
  log_madused( "slotused.txt" );


  // Start playing the damage tile sound silently...
  /*PORT
      play_sound_pvf_looped(damagetilesound, PANMID, VOLMIN, FRQDEFAULT);
  */
}

//--------------------------------------------------------------------------------------------
int get_module_data( int modnumber, char *szLoadName )
{
  // ZZ> This function loads the module data file
  FILE *fileread;
  char reference[128];
  Uint32 idsz;
  char cTmp;
  int iTmp;

  fileread = fopen( szLoadName, "r" );
  if ( fileread )
  {
    // Read basic data
    globalname = szLoadName;
    goto_colon( fileread );  get_name( fileread, modlongname[modnumber] );
    goto_colon( fileread );  fscanf( fileread, "%s", reference );
    goto_colon( fileread );  idsz = get_idsz( fileread );
    if ( module_reference_matches( reference, idsz ) )
    {
      globalname = szLoadName;
      goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
      modimportamount[modnumber] = iTmp;
      goto_colon( fileread );  cTmp = get_first_letter( fileread );
      modallowexport[modnumber] = bfalse;
      if ( cTmp == 'T' || cTmp == 't' )  modallowexport[modnumber] = btrue;
      goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  modminplayers[modnumber] = iTmp;
      goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  modmaxplayers[modnumber] = iTmp;
      goto_colon( fileread );  cTmp = get_first_letter( fileread );
      modrespawnvalid[modnumber] = bfalse;
      if ( cTmp == 'T' || cTmp == 't' )  modrespawnvalid[modnumber] = btrue;
      if ( cTmp == 'A' || cTmp == 'a' )  modrespawnvalid[modnumber] = ANYTIME;
      goto_colon( fileread );  cTmp = get_first_letter( fileread );
      modrtscontrol[modnumber] = bfalse;
      if ( cTmp == 'T' || cTmp == 't' )  modrtscontrol[modnumber] = btrue;
      goto_colon( fileread );  fscanf( fileread, "%s", generictext );
      iTmp = 0;
      while ( iTmp < RANKSIZE - 1 )
      {
        modrank[modnumber][iTmp] = generictext[iTmp];
        iTmp++;
      }
      modrank[modnumber][iTmp] = 0;



      // Read the expansions
      return btrue;
    }
  }
  return bfalse;
}

//--------------------------------------------------------------------------------------------
int get_module_summary( char *szLoadName )
{
  // ZZ> This function gets the quest description out of the module's menu file
  FILE *fileread;
  char cTmp;
  char szLine[160];
  int cnt;
  int tnc;
  bool_t result = bfalse;

  fileread = fopen( szLoadName, "r" );
  if ( fileread )
  {
    // Skip over basic data
    globalname = szLoadName;
    goto_colon( fileread );  // Name...
    goto_colon( fileread );  // Reference...
    goto_colon( fileread );  // IDSZ...
    goto_colon( fileread );  // Import...
    goto_colon( fileread );  // Export...
    goto_colon( fileread );  // Min players...
    goto_colon( fileread );  // Max players...
    goto_colon( fileread );  // Respawn...
    goto_colon( fileread );  // BAD! NOT USED
    goto_colon( fileread );  // Rank...


    // Read the summary
    cnt = 0;
    while ( cnt < SUMMARYLINES )
    {
      goto_colon( fileread );  fscanf( fileread, "%s", szLine );
      tnc = 0;
      cTmp = szLine[tnc];  if ( cTmp == '_' )  cTmp = ' ';
      while ( tnc < SUMMARYSIZE - 1 && cTmp != 0 )
      {
        modsummary[cnt][tnc] = cTmp;
        tnc++;
        cTmp = szLine[tnc];  if ( cTmp == '_' )  cTmp = ' ';
      }
      modsummary[cnt][tnc] = 0;
      cnt++;
    }
    result = btrue;
  }
  fclose( fileread );
  return result;
}

