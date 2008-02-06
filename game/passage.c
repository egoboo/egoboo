/* Egoboo - passage.c
* Passages and doors and whatnot.  Things that impede your progress!
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

#include "egoboo.h"
#include "mesh.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t open_passage( Uint32 passage )
{
  // ZZ> This function makes a passage passable
  int fan_x, fan_y;
  bool_t useful = bfalse, btmp;

  if ( passage >= numpassage ) return bfalse;

  useful = !passopen[passage];

  for ( fan_y = passtly[passage]; fan_y <= passbry[passage]; fan_y++ )
  {
    for ( fan_x = passtlx[passage]; fan_x <= passbrx[passage]; fan_x++ )
    {
      btmp = mesh_clear_fan_bits( fan_x, fan_y, MESHFX_IMPASS | MESHFX_WALL | passmask[passage] );
      useful = useful || btmp;
    }
  }

  passopen[passage] = btrue;

  return useful;
}

//--------------------------------------------------------------------------------------------
bool_t break_passage( Uint32 passage, Uint16 starttile, Uint16 frames, Uint16 become, Uint32 meshfxor )
{
  // ZZ> This function breaks the tiles of a passage if there is a character standing
  //     on 'em...  Turns the tiles into damage terrain if it reaches last frame.
  int fan_x, fan_y;
  Uint16 tile, endtile;
  bool_t useful = bfalse;
  CHR_REF character;

  if ( passage >= numpassage ) return useful;

  endtile = starttile + frames - 1;
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( character ) || chr_in_pack( character ) ) continue;

    if ( chrweight[character] > 20 && chrflyheight[character] == 0 && chrpos[character].z < ( chrlevel[character] + 20 ) && !chr_attached( character ) )
    {
      if ( passage_check( character, passage, NULL ) )
      {
        fan_x = MESH_FLOAT_TO_FAN( chrpos[character].x );
        fan_y = MESH_FLOAT_TO_FAN( chrpos[character].y );

        // The character is in the passage, so might need to break...
        tile =  mesh_get_tile( fan_x, fan_y );
        if ( tile >= starttile && tile < endtile )
        {
          // Remember where the hit occured...
          scr_globals.tmpx = chrpos[character].x;
          scr_globals.tmpy = chrpos[character].y;
          useful = btrue;

          // Change the tile
          tile = mesh_bump_tile( fan_x, fan_y );
          if ( tile == endtile )
          {
            mesh_add_fan_bits( fan_x, fan_y, meshfxor );
            mesh_set_tile( fan_x, fan_y, become );
          }
        }
      }
    }
  }

  return useful;
}

//--------------------------------------------------------------------------------------------
void flash_passage( Uint32 passage, Uint8 color )
{
  // ZZ> This function makes a passage flash white

  int fan_x, fan_y;

  if ( passage >= numpassage ) return;

  for ( fan_y = passtly[passage]; fan_y <= passbry[passage]; fan_y++ )
  {
    for ( fan_x = passtlx[passage]; fan_x <= passbrx[passage]; fan_x++ )
    {
      mesh_set_colora( fan_x, fan_y, color );
    }
  }

}

//--------------------------------------------------------------------------------------------
bool_t search_tile_in_passage( Uint32 passage, Uint32 tiletype )
{
  // ZZ> This function finds the next tile in the passage, scr_globals.tmpx and scr_globals.tmpy
  //     must be set first, and are set on a find...  Returns btrue or bfalse
  //     depending on if it finds one or not
  int fan_x, fan_y;

  if ( passage >= numpassage ) return bfalse;

  // Do the first row
  fan_x = MESH_INT_TO_FAN( scr_globals.tmpx );
  fan_y = MESH_INT_TO_FAN( scr_globals.tmpy );

  if ( fan_x < passtlx[passage] )  fan_x = passtlx[passage];
  if ( fan_y < passtly[passage] )  fan_y = passtly[passage];

  for ( /*nothing*/; fan_y <= passbry[passage]; fan_y++ )
  {
    for ( /*nothing*/; fan_x <= passbrx[passage]; fan_x++ )
    {
      if ( tiletype == mesh_get_tile( fan_x, fan_y ) )
      {
        scr_globals.tmpx = MESH_FAN_TO_INT( fan_x ) + ( 1 << 6 );
        scr_globals.tmpy = MESH_FAN_TO_INT( fan_y ) + ( 1 << 6 );
        return btrue;
      }
    }
  }

  return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint16 who_is_blocking_passage( Uint32 passage )
{
  // ZZ> This function returns MAXCHR if there is no character in the passage,
  //     otherwise the index of the first character found is returned...
  //     Finds living ones, then items and corpses

  CHR_REF character, foundother;

  if ( passage >= numpassage ) return MAXCHR;

  // Look at each character
  foundother = MAXCHR;
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( character ) || chr_in_pack( character ) ) continue;

    if ( passage_check_any( character, passage, NULL ) )
    {
      if ( chralive[character] && !chrisitem[character] )
      {
        // Found a live one
        return character;
      }
      else
      {
        // Found something else
        foundother = character;
      }
    }
  }

  // No characters found
  return foundother;
}

//--------------------------------------------------------------------------------------------
void check_passage_music()
{
  //This function checks all passages if there is a player in it, if it is, it plays a specified
  //song set in by the AI script functions

  CHR_REF character;
  Uint16 passage;

  for ( passage = 0; passage < numpassage; passage++ )
  {
    //Only check passages that have music assigned to them
    if ( INVALID_SOUND == passagemusic[passage] ) continue;

    // Look at each character
    for ( character = 0; character < MAXCHR; character++ )
    {
      if ( !VALID_CHR( character ) || chr_in_pack( character ) || !chrisplayer[character] ) continue;

      if ( passage_check_any( character, passage, NULL ) )
      {
        play_music( passagemusic[passage], 0, -1 );   //start music track
      }
    }
  }

}

//--------------------------------------------------------------------------------------------
Uint16 who_is_blocking_passage_ID( Uint32 passage, IDSZ idsz )
{
  // ZZ> This function returns MAXCHR if there is no character in the passage who
  //     have an item with the given ID.  Otherwise, the index of the first character
  //     found is returned...  Only finds living characters...
  //float tlx, tly, brx, bry;
  CHR_REF character;
  Uint16 sTmp;
  float bumpsize;

  // Look at each character
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( character ) || chr_in_pack( character ) ) continue;

    bumpsize = chrbumpsize[character];
    if ( !chrisitem[character] && chralive[character] )
    {
      if ( passage_check_any( character, passage, NULL ) )
      {
        // Found a live one...  Does it have a matching item?

        // Check the pack
        sTmp  = chr_get_nextinpack( character );
        while ( VALID_CHR( sTmp ) )
        {
          if ( CAP_INHERIT_IDSZ( chrmodel[sTmp], idsz ) )
          {
            // It has the item...
            return character;
          }
          sTmp  = chr_get_nextinpack( sTmp );
        }

        for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
        {
          sTmp = chr_get_holdingwhich( character, _slot );
          if ( VALID_CHR( sTmp ) && CAP_INHERIT_IDSZ( chrmodel[sTmp], idsz ) )
          {
            // It has the item...
            return character;
          }
        };
      }
    }
  }

  // No characters found
  return MAXCHR;
}

//--------------------------------------------------------------------------------------------
bool_t close_passage( Uint32 passage )
{
  // ZZ> This function makes a passage impassable, and returns btrue if it isn't blocked
  int fan_x, fan_y, cnt;
  CHR_REF character;
  Uint16 numcrushed;
  Uint16 crushedcharacters[MAXCHR];
  bool_t useful = bfalse, btmp;

  if ( passage >= numpassage )
    return bfalse;

  if ( passtlx[passage] > passbrx[passage] || passtly[passage] > passbry[passage] )
    return bfalse;

  useful = passopen[passage];

  if ( HAS_SOME_BITS( passmask[passage], MESHFX_IMPASS | MESHFX_WALL ) )
  {
    numcrushed = 0;
    for ( character = 0; character < MAXCHR; character++ )
    {
      if ( !VALID_CHR( character ) ) continue;

      if ( passage_check( character, passage, NULL ) )
      {
        if ( !chrcanbecrushed[character] )
        {
          // door cannot close
          return bfalse;
        }
        else
        {
          crushedcharacters[numcrushed] = character;
          numcrushed++;
        }
      }
    }


    // Crush any unfortunate characters
    for ( cnt = 0; cnt < numcrushed; cnt++ )
    {
      character = crushedcharacters[cnt];
      chralert[character] |= ALERT_CRUSHED;
    }

    useful = useful || ( numcrushed != 0 );
  }

  // Close it off
  for ( fan_y = passtly[passage]; fan_y <= passbry[passage]; fan_y++ )
  {
    for ( fan_x = passtlx[passage]; fan_x <= passbrx[passage]; fan_x++ )
    {
      btmp = mesh_add_fan_bits( fan_x, fan_y, passmask[passage] );
      useful = useful || btmp;
    }
  }

  passopen[passage] = bfalse;

  return useful;
}

//--------------------------------------------------------------------------------------------
void clear_passages()
{
  // ZZ> This function clears the passage list ( for doors )
  numpassage = 0;
  numshoppassage = 0;
}

//--------------------------------------------------------------------------------------------
Uint32 add_shop_passage( Uint16 owner, Uint32 passage )
{
  // ZZ> This function creates a shop passage
  Uint32 shop_passage = MAXPASS;

  if ( passage < numpassage && numshoppassage < MAXPASS )
  {
    shop_passage = numshoppassage;

    // The passage exists...
    shoppassage[shop_passage] = passage;
    passowner[passage]        = owner;
    shopowner[shop_passage]   = owner;  // Assume the owner is alive
    numshoppassage++;
  }

  return shop_passage;
}

//--------------------------------------------------------------------------------------------
Uint32 add_passage( int tlx, int tly, int brx, int bry, bool_t open, Uint32 mask )
{
  // ZZ> This function creates a passage area
  Uint32 passage = MAXPASS;

  // clip the passage borders
  tlx = mesh_clip_fan_x( tlx );
  tly = mesh_clip_fan_x( tly );

  brx = mesh_clip_fan_x( brx );
  bry = mesh_clip_fan_x( bry );

  if ( numpassage < MAXPASS )
  {
    passage = numpassage;
    numpassage++;

    passtlx[passage] = MIN( tlx, brx );
    passtly[passage] = MIN( tly, bry );
    passbrx[passage] = MAX( tlx, brx );
    passbry[passage] = MAX( tly, bry );
    passmask[passage] = mask;
    passagemusic[passage] = INVALID_SOUND;     //Set no song as default

    passopen[passage] = open;

    //open_passage(passage);

    //if ( !open )
    //{
    //  close_passage( passage );
    //}
  }

  return passage;
}

//--------------------------------------------------------------------------------------------
void setup_passage( char *modname )
{
  // ZZ> This function reads the passage file
  STRING newloadname;
  int tlx, tly, brx, bry;
  bool_t open;
  Uint32 mask;
  FILE *fileread;


  // Reset all of the old passages
  clear_passages();


  // Load the file
  snprintf( newloadname, sizeof( newloadname ), "%s%s/%s", modname, CData.gamedat_dir, CData.passage_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "r" );
  if ( NULL == fileread ) return;

  while ( fgoto_colon_yesno( fileread ) )
  {
    fscanf( fileread, "%d%d%d%d", &tlx, &tly, &brx, &bry );

    open = fget_bool( fileread );

    // set basic wall flags
    mask = MESHFX_IMPASS | MESHFX_WALL;

    // "Shoot Through"
    if ( fget_bool( fileread ) )
      mask = MESHFX_IMPASS;

    // "Slippy Close"
    if ( fget_bool( fileread ) )
      mask = MESHFX_SLIPPY;

    add_passage( tlx, tly, brx, bry, open, mask );
  }
  fs_fileClose( fileread );


};

//--------------------------------------------------------------------------------------------
bool_t passage_check_all( CHR_REF ichr, Uint16 pass, Uint16 * powner )
{
  // BB > character ichr is completely inside passage pass

  float x_min, x_max;
  float y_min, y_max;
  bool_t retval = bfalse;

  if ( !VALID_CHR( ichr ) || pass >= numpassage ) return retval;

  x_min = chrpos[ichr].x - chrbumpsize[ichr];
  x_max = chrpos[ichr].x + chrbumpsize[ichr];

  y_min = chrpos[ichr].y - chrbumpsize[ichr];
  y_max = chrpos[ichr].y + chrbumpsize[ichr];

  retval = ( x_min > MESH_FAN_TO_INT( passtlx[pass] ) && x_max < MESH_FAN_TO_INT( passbrx[pass] + 1 ) ) &&
           ( y_min > MESH_FAN_TO_INT( passtly[pass] ) && y_max < MESH_FAN_TO_INT( passbry[pass] + 1 ) );

  if ( retval )
  {
    signal_target( passowner[pass], MESSAGE_ENTERPASSAGE, ichr );

    if ( NULL != powner )
    {
      *powner = passowner[pass];
    }
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t passage_check_any( CHR_REF ichr, Uint16 pass, Uint16 * powner )
{
  // BB > character ichr is partially inside passage pass

  float x_min, x_max;
  float y_min, y_max;

  if ( !VALID_CHR( ichr ) || pass >= numpassage ) return bfalse;

  x_min = chrpos[ichr].x - chrbumpsize[ichr];
  x_max = chrpos[ichr].x + chrbumpsize[ichr];

  y_min = chrpos[ichr].y - chrbumpsize[ichr];
  y_max = chrpos[ichr].y + chrbumpsize[ichr];

  if ( x_max < MESH_FAN_TO_INT( passtlx[pass] ) || x_min > MESH_FAN_TO_INT( passbrx[pass] + 1 ) ) return bfalse;
  if ( y_max < MESH_FAN_TO_INT( passtly[pass] ) || y_min > MESH_FAN_TO_INT( passbry[pass] + 1 ) ) return bfalse;

  signal_target( passowner[pass], MESSAGE_ENTERPASSAGE, ichr );

  if ( NULL != powner )
  {
    *powner = passowner[pass];
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t passage_check( CHR_REF ichr, Uint16 pass, Uint16 * powner )
{
  // BB > character ichr's center is inside passage pass

  bool_t retval = bfalse;

  if ( !VALID_CHR( ichr ) || pass >= numpassage ) return retval;

  retval = ( chrpos[ichr].x > MESH_FAN_TO_INT( passtlx[pass] ) && chrpos[ichr].x < MESH_FAN_TO_INT( passbrx[pass] + 1 ) ) &&
           ( chrpos[ichr].y > MESH_FAN_TO_INT( passtly[pass] ) && chrpos[ichr].y < MESH_FAN_TO_INT( passbry[pass] + 1 ) );

  if ( retval )
  {
    signal_target( passowner[pass], MESSAGE_ENTERPASSAGE, ichr );

    if ( NULL != powner )
    {
      *powner = passowner[pass];
    }
  };

  return retval;
};