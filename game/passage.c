//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - passage.c
 * Passages and doors and whatnot.  Things that impede your progress!
 */

#include "passage.h"

#include "char.h"
#include "script.h"
#include "sound.h"
#include "mesh.h"
#include "game.h"
#include "network.h"

#include "egoboo_fileutil.h"
#include "egoboo.h"

#include "SDL_extensions.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
DECLARE_STACK( ACCESS_TYPE_NONE, passage_t, PassageStack );
DECLARE_STACK( ACCESS_TYPE_NONE, shop_t,    ShopStack    );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t open_passage( Uint16 passage )
{
    // ZZ> This function makes a passage passable
    int x, y;
    Uint32 fan;
    bool_t useful = bfalse;
    if ( INVALID_PASSAGE(passage) ) return useful;

    useful = ( !PassageStack.lst[passage].open );
    PassageStack.lst[passage].open = btrue;
    y = PassageStack.lst[passage].toplefty;
    while ( y <= PassageStack.lst[passage].bottomrighty )
    {
        useful = ( !PassageStack.lst[passage].open );
        PassageStack.lst[passage].open = btrue;

        for (y = PassageStack.lst[passage].toplefty; y <= PassageStack.lst[passage].bottomrighty; y++ )
        {

            for (x = PassageStack.lst[passage].topleftx; x <= PassageStack.lst[passage].bottomrightx; x++ )
            {
                fan = mesh_get_tile_int( PMesh, x, y );
                if ( VALID_TILE(PMesh, fan) ) PMesh->mmem.tile_list[fan].fx &= ~( MPDFX_WALL | MPDFX_IMPASS );
            }
        }
    }

    return useful;
}

//--------------------------------------------------------------------------------------------
bool_t break_passage( script_state_t * pstate, Uint16 passage, Uint16 starttile, Uint16 frames,
                      Uint16 become, Uint8 meshfxor )
{
    // ZZ> This function breaks the tiles of a passage if there is a character standing
    //    on 'em...  Turns the tiles into damage terrain if it reaches last frame.
    Uint16 tile, endtile;
    Uint32 fan;
    int useful, character;
    if ( INVALID_PASSAGE( passage ) ) return bfalse;

    endtile = starttile + frames - 1;
    useful = bfalse;
    for ( character = 0; character < MAX_CHR; character++ )
    {
        if ( !ChrList.lst[character].on || ChrList.lst[character].pack_ispacked || MAX_CHR != ChrList.lst[character].attachedto ) continue;

        if ( ChrList.lst[character].weight > 20 && (0 == ChrList.lst[character].flyheight) && ( ChrList.lst[character].pos.z < ChrList.lst[character].floor_level + 20 ) )
        {
            fan = mesh_get_tile( PMesh, ChrList.lst[character].pos.x, ChrList.lst[character].pos.y );
            if ( VALID_TILE(PMesh, fan) )
            {
                tile = PMesh->mmem.tile_list[fan].img;
                if ( tile >= starttile && tile < endtile )
                {
                    if ( is_in_passage( passage, ChrList.lst[character].pos.x, ChrList.lst[character].pos.y, ChrList.lst[character].bumpsize ) )
                    {
                        // Remember where the hit occured...
                        pstate->x = ChrList.lst[character].pos.x;
                        pstate->y = ChrList.lst[character].pos.y;

                        useful = btrue;

                        // Change the tile
                        tile++;
                        if ( tile == endtile )
                        {
                            PMesh->mmem.tile_list[fan].fx |= meshfxor;
                            if ( become != 0 )
                            {
                                tile = become;
                            }
                        }

                        mesh_set_texture(PMesh, fan, tile);
                    }
                }
            }
        }
    }

    return useful;
}

//--------------------------------------------------------------------------------------------
void flash_passage( Uint16 passage, Uint8 color )
{
    // ZZ> This function makes a passage flash white
    int x, y, cnt, numvert;
    Uint32 fan, vert;

    if ( INVALID_PASSAGE( passage ) ) return;

    for ( y = PassageStack.lst[passage].toplefty; y <= PassageStack.lst[passage].bottomrighty; y++ )
    {
        for ( x = PassageStack.lst[passage].topleftx; x <= PassageStack.lst[passage].bottomrightx; x++ )
        {
            fan = mesh_get_tile_int( PMesh, x, y );

            if ( VALID_TILE(PMesh, fan) )
            {
                Uint16 ttype = PMesh->mmem.tile_list[fan].type;

                ttype &= 0x3F;

                numvert = tile_dict[ttype].numvertices;
                vert    = PMesh->mmem.tile_list[fan].vrtstart;
                for ( cnt = 0; cnt < numvert; cnt++ )
                {
                    PMesh->gmem.light[vert].a = color;
                    vert++;
                }
            }
        }
    }

}

//--------------------------------------------------------------------------------------------
Uint8 find_tile_in_passage( script_state_t * pstate, Uint16 passage, int tiletype )
{
    // ZZ> This function finds the next tile in the passage, pstate->x and pstate->y
    //    must be set first, and are set on a find...  Returns btrue or bfalse
    //    depending on if it finds one or not
    int x, y;
    Uint32 fan;

    if ( INVALID_PASSAGE( passage ) ) return bfalse;

    // Do the first row
    x = pstate->x >> TILE_BITS;
    y = pstate->y >> TILE_BITS;

    if ( x < PassageStack.lst[passage].topleftx )  x = PassageStack.lst[passage].topleftx;
    if ( y < PassageStack.lst[passage].toplefty )  y = PassageStack.lst[passage].toplefty;

    if ( y < PassageStack.lst[passage].bottomrighty )
    {
        while ( x <= PassageStack.lst[passage].bottomrightx )
        {
            fan = mesh_get_tile_int( PMesh, x, y );

            if ( VALID_TILE(PMesh, fan) )
            {
                if ( (PMesh->mmem.tile_list[fan].img & 0xFF) == tiletype )
                {
                    pstate->x = ( x << TILE_BITS ) + 64;
                    pstate->y = ( y << TILE_BITS ) + 64;
                    return btrue;
                }

            }
            x++;
        }

        y++;
    }

    // Do all remaining rows
    while ( y <= PassageStack.lst[passage].bottomrighty )
    {
        x = PassageStack.lst[passage].topleftx;

        while ( x <= PassageStack.lst[passage].bottomrightx )
        {
            fan = mesh_get_tile_int( PMesh, x, y );

            if ( VALID_TILE(PMesh, fan) )
            {

                if ( (PMesh->mmem.tile_list[fan].img & 0xFF) == tiletype )
                {
                    pstate->x = ( x << TILE_BITS ) + 64;
                    pstate->y = ( y << TILE_BITS ) + 64;
                    return btrue;
                }
            }

            x++;
        }

        y++;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t is_in_passage( Uint16 passage, float xpos, float ypos, float tolerance )
{
    // ZF> This return btrue if the specified X and Y coordinates are within the passage
    // tolerance is how much offset we allow outside the passage
    float tlx, tly, brx, bry;
    if ( INVALID_PASSAGE(passage) ) return bfalse;

    // Passage area
    tolerance += CLOSETOLERANCE;
    tlx = ( PassageStack.lst[passage].topleftx << TILE_BITS ) - tolerance;
    tly = ( PassageStack.lst[passage].toplefty << TILE_BITS ) - tolerance;
    brx = ( ( PassageStack.lst[passage].bottomrightx + 1 ) << TILE_BITS ) + tolerance;
    bry = ( ( PassageStack.lst[passage].bottomrighty + 1 ) << TILE_BITS ) + tolerance;

    if ( xpos > tlx && xpos < brx )
    {
        if ( ypos > tly && ypos < bry )
        {
            // The coordinate is within the passage
            return btrue;
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint16 who_is_blocking_passage( Uint16 passage, bool_t targetitems, bool_t targetdead, bool_t targetquest,
							   bool_t requireitem, IDSZ findidsz )
{
    // ZZ> This function returns MAX_CHR if there is no character in the passage,
    //    otherwise the index of the first character found is returned...
	//    Can also look for characters with a specific quest or item in his or her inventory
    //    Finds living ones, then items and corpses
    Uint16 character, foundother;

	if ( INVALID_PASSAGE(passage) ) return MAX_CHR;

    // Look at each character
    foundother = MAX_CHR;
    for ( character = 0; character < MAX_CHR; character++ )
    {
        if ( !ChrList.lst[character].on || ChrList.lst[character].pack_ispacked || ChrList.lst[character].attachedto != MAX_CHR ) continue;
		if ( ChrList.lst[character].invictus || ChrList.lst[character].weight == 0xFFFFFFFF ) continue;

		//Do items?
		if( !targetitems && ChrList.lst[character].isitem ) continue;

		//Do dead stuff?
		if( !targetdead && !ChrList.lst[character].alive ) continue;

		//Require target to have specific quest?
		if( targetquest && (!ChrList.lst[character].isplayer || QUEST_NONE  >= check_player_quest( ChrList.lst[character].name, findidsz )) ) continue;

		//Now check if it actually is inside the passage area
        if ( is_in_passage( passage, ChrList.lst[character].pos.x, ChrList.lst[character].pos.y, ChrList.lst[character].bumpsize ) )
        {
            if ( ChrList.lst[character].alive && !ChrList.lst[character].isitem )
            {
				Uint16 item;

				// Found a live one, do we need to check for required items as well?
                if( !requireitem ) return character;

				// It needs to have a specific item as well
				else
				{
					// I: Check left hand
					item = ChrList.lst[character].holdingwhich[SLOT_LEFT];
					if ( item != MAX_CHR )
					{
						item = ChrList.lst[item].model;
						if ( CapList[item].idsz[IDSZ_PARENT] == findidsz || CapList[item].idsz[IDSZ_TYPE] == findidsz )
						{
							// It has the item...
							return character;
						}
					}

					// II: Check right hand
					item = ChrList.lst[character].holdingwhich[SLOT_RIGHT];
					if ( item != MAX_CHR )
					{
						item = ChrList.lst[item].model;
						if ( CapList[item].idsz[IDSZ_PARENT] == findidsz || CapList[item].idsz[IDSZ_TYPE] == findidsz )
						{
							// It has the item...
							return character;
						}
					}

					// III: Check the pack
					item = ChrList.lst[character].pack_next;
					while ( item != MAX_CHR )
					{
						if ( CapList[ChrList.lst[item].model].idsz[IDSZ_PARENT] == findidsz || CapList[ChrList.lst[item].model].idsz[IDSZ_TYPE] == findidsz )
						{
							// It has the item in inventory...
							return character;
						}
						item = ChrList.lst[item].pack_next;
					}
				}

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
    // ZF> This function checks all passages if there is a player in it, if it is, it plays a specified
    // song set in by the AI script functions
    Uint16 character = 0, passage, cnt;

    // Check every music passage
    for ( passage = 0; passage < PassageStack.count; passage++ )
    {
        if ( PassageStack.lst[passage].music == NO_MUSIC ) continue;

        // Look at each player
        for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
        {
            character = PlaList[cnt].index;
            if ( !ChrList.lst[character].on || ChrList.lst[character].pack_ispacked || !ChrList.lst[character].alive || !ChrList.lst[character].isplayer ) continue;

            // Is it in the passage?
            if (  is_in_passage( passage, ChrList.lst[character].pos.x, ChrList.lst[character].pos.y, ChrList.lst[character].bumpsize  ) )
            {
                // Found a player, start music track
                sound_play_song( PassageStack.lst[passage].music, 0, -1 );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t close_passage( Uint16 passage )
{
    // ZZ> This function makes a passage impassable, and returns btrue if it isn't blocked
    int x, y, cnt;
    Uint32 fan;
    Uint16 character;
    float bumpsize;
    Uint16 numcrushed = 0;
    Uint16 crushedcharacters[MAX_CHR];
    if ( INVALID_PASSAGE( passage ) ) return bfalse;

    if ( HAS_SOME_BITS( PassageStack.lst[passage].mask, MPDFX_IMPASS | MPDFX_WALL ) )
    {
        // Make sure it isn't blocked
        for ( character = 0; character < MAX_CHR; character++ )
        {
            if ( !ChrList.lst[character].on ) continue;

            bumpsize = ChrList.lst[character].bumpsize;
            if ( (!ChrList.lst[character].pack_ispacked ) && ChrList.lst[character].attachedto == MAX_CHR && ChrList.lst[character].bumpsize != 0 )
            {
                if ( is_in_passage( passage, ChrList.lst[character].pos.x, ChrList.lst[character].pos.y, ChrList.lst[character].bumpsize ))
                {
                    if ( !ChrList.lst[character].canbecrushed )
                    {
                        // Someone is blocking, stop here
                        return bfalse;
                    }
                    else
                    {
                        crushedcharacters[numcrushed] = character;
                        numcrushed++;
                    }
                }
            }
        }

        // Crush any unfortunate characters
        cnt = 0;
        while ( cnt < numcrushed )
        {
            character = crushedcharacters[cnt];
            ChrList.lst[character].ai.alert |= ALERTIF_CRUSHED;
            cnt++;
        }
    }

    // Close it off
    PassageStack.lst[passage].open = bfalse;
    for ( y = PassageStack.lst[passage].toplefty; y <= PassageStack.lst[passage].bottomrighty; y++ )
    {
        for (x = PassageStack.lst[passage].topleftx; x <= PassageStack.lst[passage].bottomrightx; x++ )
        {
            fan = mesh_get_tile_int( PMesh, x, y );
            if ( VALID_TILE(PMesh, fan) )
            {
                PMesh->mmem.tile_list[fan].fx = PMesh->mmem.tile_list[fan].fx | PassageStack.lst[passage].mask;
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void clear_all_passages()
{
    Uint16 cnt = 0;

    // ZZ> This function clears the passage list ( for doors )
    PassageStack.count = 0;
    ShopStack.count    = 0;

    for ( cnt = 0; cnt < MAX_PASS; cnt++ )
    {
        ShopStack.lst[cnt].owner = NOOWNER;
        ShopStack.lst[cnt].passage = 0;
    }
}

//--------------------------------------------------------------------------------------------
void add_shop_passage( Uint16 owner, Uint16 passage )
{
    // ZZ> This function creates a shop passage
    if ( VALID_PASSAGE(passage) && VALID_SHOP(ShopStack.count) )
    {
        // The passage exists...
        ShopStack.lst[ShopStack.count].passage = passage;
        ShopStack.lst[ShopStack.count].owner   = owner;  // Assume the owner is alive
        ShopStack.count++;
    }
}

//--------------------------------------------------------------------------------------------
void add_passage( int tlx, int tly, int brx, int bry, bool_t open, Uint8 mask )
{
    // ZZ> This function creates a passage area

    if ( PassageStack.count < MAX_PASS )
    {
        tlx = CLIP(tlx, 0, PMesh->info.tiles_x - 1);
        tly = CLIP(tly, 0, PMesh->info.tiles_y - 1);

        brx = CLIP(brx, 0, PMesh->info.tiles_x - 1);
        bry = CLIP(bry, 0, PMesh->info.tiles_y - 1);

        PassageStack.lst[PassageStack.count].topleftx        = tlx;
        PassageStack.lst[PassageStack.count].toplefty        = tly;
        PassageStack.lst[PassageStack.count].bottomrightx    = brx;
        PassageStack.lst[PassageStack.count].bottomrighty    = bry;
        PassageStack.lst[PassageStack.count].mask            = mask;
        PassageStack.lst[PassageStack.count].music           = NO_MUSIC;     // Set no song as default

        // Is it open or closed?
        if (!open) close_passage( PassageStack.count );
        else PassageStack.lst[PassageStack.count].open = btrue;

        PassageStack.count++;
    }
}

//--------------------------------------------------------------------------------------------
void setup_passage( const char *modname )
{
    // ZZ> This function reads the passage file
    STRING newloadname;
    int tlx, tly, brx, bry;
    bool_t open;
    Uint8 mask;
    vfs_FILE *fileread;

    // Reset all of the old passages
    clear_all_passages();

    // Load the file
    make_newloadname( modname, "gamedat" SLASH_STR "passage.txt", newloadname );
    fileread = vfs_openRead( newloadname );
    if ( NULL == fileread ) return;

    while ( goto_colon( NULL, fileread, btrue ) )
    {
        tlx  = fget_int( fileread );
        tly  = fget_int( fileread );
        brx  = fget_int( fileread );
        bry  = fget_int( fileread );

        open = fget_bool( fileread );

        mask = MPDFX_IMPASS | MPDFX_WALL;
        if ( fget_bool( fileread ) ) mask = MPDFX_IMPASS;
        if ( fget_bool( fileread ) ) mask = MPDFX_SLIPPY;

        add_passage( tlx, tly, brx, bry, open, mask );
    }

    vfs_close( fileread );
}

//--------------------------------------------------------------------------------------------
Uint16 shop_get_owner( int ix, int iy )
{
    // This function returns the owner of a item in a shop
    int cnt;
    Uint16 owner = NOOWNER;

    for ( cnt = 0; cnt < ShopStack.count; cnt++ )
    {
        Uint16 passage;

        passage = ShopStack.lst[cnt].passage;
        if ( INVALID_PASSAGE(passage) ) continue;

        if ( ix >= PassageStack.lst[passage].topleftx && ix <= PassageStack.lst[passage].bottomrightx )
        {
            if ( iy >= PassageStack.lst[passage].toplefty && iy <= PassageStack.lst[passage].bottomrighty )
            {
                // if there is NOOWNER, someone has been murdered!
                owner  = ShopStack.lst[cnt].owner;
                break;
            }
        }
    }

    return owner;
}
