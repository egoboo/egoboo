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

#include "passage.h"
#include "mesh.h"
#include "char.h"
#include "script.h"

#include "egoboo_utility.h"
#include "egoboo.h"

Uint32  passage_count = 0;             // Number of passages in the module
PASSAGE PassList[MAXPASS];

// For shops
Uint16 shop_count = 0;
SHOP   ShopList[MAXPASS];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t open_passage( Uint32 passage )
{
  // ZZ> This function makes a passage passable
  int fan_x, fan_y;
  bool_t useful = bfalse, btmp;

  if ( passage >= passage_count ) return bfalse;

  useful = !PassList[passage].open;

  for ( fan_y = PassList[passage].area.top; fan_y <= PassList[passage].area.bottom; fan_y++ )
  {
    for ( fan_x = PassList[passage].area.left; fan_x <= PassList[passage].area.right; fan_x++ )
    {
      btmp = mesh_clear_fan_bits( fan_x, fan_y, MPDFX_IMPASS | MPDFX_WALL | PassList[passage].mask );
      useful = useful || btmp;
    }
  }

  PassList[passage].open = btrue;

  return useful;
}

//--------------------------------------------------------------------------------------------
bool_t break_passage( SCRIPT_GLOBAL_VALUES * pg_scr, Uint32 passage, Uint16 starttile, Uint16 frames, Uint16 become, Uint32 meshfxor )
{
  // ZZ> This function breaks the tiles of a passage if there is a character standing
  //     on 'em...  Turns the tiles into damage terrain if it reaches last frame.
  int fan_x, fan_y;
  Uint16 tile, endtile;
  bool_t useful = bfalse;
  CHR_REF character;

  if ( passage >= passage_count ) return useful;

  endtile = starttile + frames - 1;
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( character ) || chr_in_pack( character ) ) continue;

    if ( ChrList[character].weight > 20 && ChrList[character].flyheight == 0 && ChrList[character].pos.z < ( ChrList[character].level + 20 ) && !chr_attached( character ) )
    {
      if ( passage_check( character, passage, NULL ) )
      {
        fan_x = MESH_FLOAT_TO_FAN( ChrList[character].pos.x );
        fan_y = MESH_FLOAT_TO_FAN( ChrList[character].pos.y );

        // The character is in the passage, so might need to break...
        tile =  mesh_get_tile( fan_x, fan_y );
        if ( tile >= starttile && tile < endtile )
        {
          // Remember where the hit occured...
          pg_scr->tmpx = ChrList[character].pos.x;
          pg_scr->tmpy = ChrList[character].pos.y;
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

  if ( passage >= passage_count ) return;

  for ( fan_y = PassList[passage].area.top; fan_y <= PassList[passage].area.bottom; fan_y++ )
  {
    for ( fan_x = PassList[passage].area.left; fan_x <= PassList[passage].area.right; fan_x++ )
    {
      mesh_set_colora( fan_x, fan_y, color );
    }
  }

}

//--------------------------------------------------------------------------------------------
bool_t search_tile_in_passage( SCRIPT_GLOBAL_VALUES * pgscr, Uint32 passage, Uint32 tiletype )
{
  // ZZ> This function finds the next tile in the passage, pgscr->tmpx and pgscr->tmpy
  //     must be set first, and are set on a find...  Returns btrue or bfalse
  //     depending on if it finds one or not
  int fan_x, fan_y;

  if ( passage >= passage_count ) return bfalse;

  // Do the first row
  fan_x = MESH_INT_TO_FAN( pgscr->tmpx );
  fan_y = MESH_INT_TO_FAN( pgscr->tmpy );

  if ( fan_x < PassList[passage].area.left )  fan_x = PassList[passage].area.left;
  if ( fan_y < PassList[passage].area.top )  fan_y = PassList[passage].area.top;

  for ( /*nothing*/; fan_y <= PassList[passage].area.bottom; fan_y++ )
  {
    for ( /*nothing*/; fan_x <= PassList[passage].area.right; fan_x++ )
    {
      if ( tiletype == mesh_get_tile( fan_x, fan_y ) )
      {
        pgscr->tmpx = MESH_FAN_TO_INT( fan_x ) + ( 1 << 6 );
        pgscr->tmpy = MESH_FAN_TO_INT( fan_y ) + ( 1 << 6 );
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

  if ( passage >= passage_count ) return MAXCHR;

  // Look at each character
  foundother = MAXCHR;
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( character ) || chr_in_pack( character ) ) continue;

    if ( passage_check_any( character, passage, NULL ) )
    {
      if ( ChrList[character].alive && !ChrList[character].isitem )
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

  for ( passage = 0; passage < passage_count; passage++ )
  {
    //Only check passages that have music assigned to them
    if ( INVALID_SOUND == PassList[passage].music ) continue;

    // Look at each character
    for ( character = 0; character < MAXCHR; character++ )
    {
      if ( !VALID_CHR( character ) || chr_in_pack( character ) || !ChrList[character].isplayer ) continue;

      if ( passage_check_any( character, passage, NULL ) )
      {
        play_music( PassList[passage].music, 0, -1 );   //start music track
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

  CHR_REF character;
  Uint16  sTmp;

  // Look at each character
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( character ) || chr_in_pack( character ) ) continue;

    if ( !ChrList[character].isitem && ChrList[character].alive )
    {
      if ( passage_check_any( character, passage, NULL ) )
      {
        // Found a live one...  Does it have a matching item?

        // Check the pack
        sTmp  = chr_get_nextinpack( character );
        while ( VALID_CHR( sTmp ) )
        {
          if ( CAP_INHERIT_IDSZ( ChrList[sTmp].model, idsz ) )
          {
            // It has the item...
            return character;
          }
          sTmp  = chr_get_nextinpack( sTmp );
        }

        for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
        {
          sTmp = chr_get_holdingwhich( character, _slot );
          if ( VALID_CHR( sTmp ) && CAP_INHERIT_IDSZ( ChrList[sTmp].model, idsz ) )
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
  PASSAGE * ppass;

  if ( passage >= passage_count )
    return bfalse;

  ppass = PassList + passage;

  if ( ppass->area.left > ppass->area.right || ppass->area.top > ppass->area.bottom )
    return bfalse;

  useful = ppass->open;

  if ( HAS_SOME_BITS( ppass->mask, MPDFX_IMPASS | MPDFX_WALL ) )
  {
    numcrushed = 0;
    for ( character = 0; character < MAXCHR; character++ )
    {
      if ( !VALID_CHR( character ) ) continue;

      if ( passage_check( character, passage, NULL ) )
      {
        if ( !ChrList[character].canbecrushed )
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
      ChrList[character].aistate.alert |= ALERT_CRUSHED;
    }

    useful = useful || ( numcrushed != 0 );
  }

  // Close it off
  for ( fan_y = ppass->area.top; fan_y <= ppass->area.bottom; fan_y++ )
  {
    for ( fan_x = ppass->area.left; fan_x <= ppass->area.right; fan_x++ )
    {
      btmp = mesh_add_fan_bits( fan_x, fan_y, ppass->mask );
      useful = useful || btmp;
    }
  }

  ppass->open = bfalse;

  return useful;
}

//--------------------------------------------------------------------------------------------
void clear_passages()
{
  // ZZ> This function clears the passage list ( for doors )
  passage_count = 0;
  shop_count    = 0;
}

//--------------------------------------------------------------------------------------------
Uint32 add_shop_passage( Uint16 owner, Uint32 passage )
{
  // ZZ> This function creates a shop passage
  Uint32 shop_passage = MAXPASS;

  if ( passage < passage_count && shop_count < MAXPASS )
  {
    shop_passage = shop_count;

    // The passage exists...
    ShopList[shop_passage].passage = passage;
    PassList[passage].owner        = owner;
    ShopList[shop_passage].owner   = owner;  // Assume the owner is alive
    shop_count++;
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

  if ( passage_count < MAXPASS )
  {
    passage = passage_count;
    passage_count++;

    //PassList[passage].area.left   = MIN( tlx, brx );
    //PassList[passage].area.top    = MIN( tly, bry );
    //PassList[passage].area.right  = MAX( tlx, brx );
    //PassList[passage].area.bottom = MAX( tly, bry );

    // allow for "inverted" passages
    PassList[passage].area.left   = tlx;
    PassList[passage].area.top    = tly;
    PassList[passage].area.right  = brx;
    PassList[passage].area.bottom = bry;

    PassList[passage].mask        = mask;
    PassList[passage].music       = INVALID_SOUND;     //Set no song as default

    PassList[passage].open = open;
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
  snprintf( newloadname, sizeof( newloadname ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.passage_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "r" );
  if ( NULL == fileread ) return;

  while ( fgoto_colon_yesno( fileread ) )
  {
    fscanf( fileread, "%d%d%d%d", &tlx, &tly, &brx, &bry );

    open = fget_bool( fileread );

    // set basic wall flags
    mask = MPDFX_IMPASS | MPDFX_WALL;

    // "Shoot Through"
    if ( fget_bool( fileread ) )
      mask = MPDFX_IMPASS;

    // "Slippy Close"
    if ( fget_bool( fileread ) )
      mask = MPDFX_SLIPPY;

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

  if ( !VALID_CHR( ichr ) || pass >= passage_count ) return retval;

  x_min = ChrList[ichr].bmpdata.cv.x_min;
  x_max = ChrList[ichr].bmpdata.cv.x_max;

  y_min = ChrList[ichr].bmpdata.cv.x_min;
  y_max = ChrList[ichr].bmpdata.cv.x_max;

  retval = ( x_min > MESH_FAN_TO_INT( PassList[pass].area.left ) && x_max < MESH_FAN_TO_INT( PassList[pass].area.right + 1 ) ) &&
           ( y_min > MESH_FAN_TO_INT( PassList[pass].area.top ) && y_max < MESH_FAN_TO_INT( PassList[pass].area.bottom + 1 ) );

  if ( retval )
  {
    signal_target( PassList[pass].owner, MESSAGE_ENTERPASSAGE, ichr );

    if ( NULL != powner )
    {
      *powner = PassList[pass].owner;
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

  if ( !VALID_CHR( ichr ) || pass >= passage_count ) return bfalse;

  x_min = ChrList[ichr].bmpdata.cv.x_min;
  x_max = ChrList[ichr].bmpdata.cv.x_max;

  y_min = ChrList[ichr].bmpdata.cv.y_min;
  y_max = ChrList[ichr].bmpdata.cv.y_max;

  if ( x_max < MESH_FAN_TO_INT( PassList[pass].area.left ) || x_min > MESH_FAN_TO_INT( PassList[pass].area.right + 1 ) ) return bfalse;
  if ( y_max < MESH_FAN_TO_INT( PassList[pass].area.top ) || y_min > MESH_FAN_TO_INT( PassList[pass].area.bottom + 1 ) ) return bfalse;

  signal_target( PassList[pass].owner, MESSAGE_ENTERPASSAGE, ichr );

  if ( NULL != powner )
  {
    *powner = PassList[pass].owner;
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t passage_check( CHR_REF ichr, Uint16 pass, Uint16 * powner )
{
  // BB > character ichr's center is inside passage pass

  bool_t retval = bfalse;

  if ( !VALID_CHR( ichr ) || pass >= passage_count ) return retval;

  retval = ( ChrList[ichr].pos.x > MESH_FAN_TO_INT( PassList[pass].area.left ) && ChrList[ichr].pos.x < MESH_FAN_TO_INT( PassList[pass].area.right + 1 ) ) &&
           ( ChrList[ichr].pos.y > MESH_FAN_TO_INT( PassList[pass].area.top ) && ChrList[ichr].pos.y < MESH_FAN_TO_INT( PassList[pass].area.bottom + 1 ) );

  if ( retval )
  {
    signal_target( PassList[pass].owner, MESSAGE_ENTERPASSAGE, ichr );

    if ( NULL != powner )
    {
      *powner = PassList[pass].owner;
    }
  };

  return retval;
};