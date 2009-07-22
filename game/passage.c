// ********************************************************************************************
// *
// *    This file is part of Egoboo.
// *
// *    Egoboo is free software: you can redistribute it and/or modify it
// *    under the terms of the GNU General Public License as published by
// *    the Free Software Foundation, either version 3 of the License, or
// *    (at your option) any later version.
// *
// *    Egoboo is distributed in the hope that it will be useful, but
// *    WITHOUT ANY WARRANTY; without even the implied warranty of
// *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// *    General Public License for more details.
// *
// *    You should have received a copy of the GNU General Public License
// *    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
// *
// ********************************************************************************************

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

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------

// Passages
int   numpassage = 0;              // Number of passages in the module
passage_t       PassageList[MAX_PASS];

// For shops
int     numshoppassage = 0;
Uint16  shoppassage[MAX_PASS];  // The passage number
Uint16  shopowner[MAX_PASS];    // Who gets the gold?

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
bool_t open_passage( Uint16 passage )
{
    // ZZ> This function makes a passage passable
    int x, y;
    Uint32 fan;
    bool_t useful = bfalse;
	if( INVALID_PASSAGE(passage) ) return useful;

	useful = ( !PassageList[passage].open );
	PassageList[passage].open = btrue;
	y = PassageList[passage].toplefty;
	while ( y <= PassageList[passage].bottomrighty )
    {
        useful = ( !PassageList[passage].open );
        PassageList[passage].open = btrue;
        y = PassageList[passage].toplefty;

        while ( y <= PassageList[passage].bottomrighty )
        {
            x = PassageList[passage].topleftx;

            while ( x <= PassageList[passage].bottomrightx )
            {
				fan = mesh_get_tile( PMesh, x, y );
				if ( VALID_TILE(PMesh, fan) ) PMesh->mmem.tile_list[fan].fx &= ~( MPDFX_WALL | MPDFX_IMPASS );
            }
            x++;
        }

        y++;
    }

    return useful;
}

// --------------------------------------------------------------------------------------------
bool_t break_passage( script_state_t * pstate, Uint16 passage, Uint16 starttile, Uint16 frames,
                   Uint16 become, Uint8 meshfxor )
{
    // ZZ> This function breaks the tiles of a passage if there is a character standing
    //     on 'em...  Turns the tiles into damage terrain if it reaches last frame.
    Uint16 tile, endtile;
    Uint32 fan;
    int useful, character;
    if ( INVALID_PASSAGE( passage ) ) return bfalse;

    endtile = starttile + frames - 1;
    useful = bfalse;
    for ( character = 0; character < MAX_CHR; character++ )
    {
        if ( !ChrList[character].on || ChrList[character].pack_ispacked || MAX_CHR != ChrList[character].attachedto ) continue;

        if ( ChrList[character].weight > 20 && (0 == ChrList[character].flyheight) && ( ChrList[character].pos.z < ChrList[character].floor_level + 20 ) )
        {
            fan = mesh_get_tile( PMesh, ChrList[character].pos.x, ChrList[character].pos.y );
            if ( VALID_TILE(PMesh, fan) )
            {
                tile = PMesh->mmem.tile_list[fan].img;
                if ( tile >= starttile && tile < endtile )
                {
                    if ( is_in_passage( passage, ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].bumpsize ) )
                    {
                        // Remember where the hit occured...
                        pstate->x = ChrList[character].pos.x;
                        pstate->y = ChrList[character].pos.y;

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

// --------------------------------------------------------------------------------------------
void flash_passage( Uint16 passage, Uint8 color )
{
    // ZZ> This function makes a passage flash white
    int x, y, cnt, numvert;
    Uint32 fan, vert;

    if ( INVALID_PASSAGE( passage ) ) return;

    for ( y = PassageList[passage].toplefty; y <= PassageList[passage].bottomrighty; y++ )
    {
        for ( x = PassageList[passage].topleftx; x <= PassageList[passage].bottomrightx; x++ )
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

// --------------------------------------------------------------------------------------------
Uint8 find_tile_in_passage( script_state_t * pstate, Uint16 passage, int tiletype )
{
    // ZZ> This function finds the next tile in the passage, pstate->x and pstate->y
    //     must be set first, and are set on a find...  Returns btrue or bfalse
    //     depending on if it finds one or not
    int x, y;
    Uint32 fan;

    if ( INVALID_PASSAGE( passage ) ) return bfalse;

    // Do the first row
    x = pstate->x >> TILE_BITS;
    y = pstate->y >> TILE_BITS;

    if ( x < PassageList[passage].topleftx )  x = PassageList[passage].topleftx;
    if ( y < PassageList[passage].toplefty )  y = PassageList[passage].toplefty;

    if ( y < PassageList[passage].bottomrighty )
    {
        while ( x <= PassageList[passage].bottomrightx )
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
    while ( y <= PassageList[passage].bottomrighty )
    {
        x = PassageList[passage].topleftx;

        while ( x <= PassageList[passage].bottomrightx )
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

// --------------------------------------------------------------------------------------------
bool_t is_in_passage( Uint16 passage, float xpos, float ypos, float tolerance )
{
    // ZF> This return btrue if the specified X and Y coordinates are within the passage
    // tolerance is how much offset we allow outside the passage
    float tlx, tly, brx, bry;
	if( INVALID_PASSAGE(passage) ) return bfalse;
    
    // Passage area
	tolerance += CLOSETOLERANCE;	
	tlx = ( PassageList[passage].topleftx << TILE_BITS ) - tolerance;
	tly = ( PassageList[passage].toplefty << TILE_BITS ) - tolerance;
	brx = ( ( PassageList[passage].bottomrightx + 1 ) << TILE_BITS ) + tolerance;
	bry = ( ( PassageList[passage].bottomrighty + 1 ) << TILE_BITS ) + tolerance;
    
    if ( xpos > tlx && xpos < brx )
    {
        if ( ypos > tly && ypos < bry )
        {
            //The coordinate is within the passage
            return btrue;
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint16 who_is_blocking_passage( Uint16 passage )
{
    // ZZ> This function returns MAX_CHR if there is no character in the passage,
    //     otherwise the index of the first character found is returned...
    //     Finds living ones, then items and corpses
    Uint16 character, foundother;

    // Look at each character
    foundother = MAX_CHR;
    for ( character = 0; character < MAX_CHR; character++ )
    {
        if ( !ChrList[character].on ) continue;

        if ( ChrList[character].bumpsize != 0 && !ChrList[character].pack_ispacked && ChrList[character].attachedto == MAX_CHR )
        {
            if ( is_in_passage( passage, ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].bumpsize ) )
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
    }

    // No characters found
    return foundother;
}

// --------------------------------------------------------------------------------------------
void check_passage_music()
{
    // ZF> This function checks all passages if there is a player in it, if it is, it plays a specified
    // song set in by the AI script functions
    Uint16 character = 0, passage, cnt;

    //Check every music passage
    for ( passage = 0; passage < numpassage; passage++ )
    {
        if ( PassageList[passage].music == NO_MUSIC ) continue;

        // Look at each player
        for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
        {
            character = PlaList[cnt].index;
            if ( INVALID_CHR( character ) || ChrList[character].pack_ispacked || !ChrList[character].alive || !ChrList[character].isplayer ) continue;

            // Is it in the passage?
            if (  is_in_passage( passage, ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].bumpsize  ) )
            {
                // Found a player, start music track
                sound_play_song( PassageList[passage].music, 0, -1 );
            }
        }
    }
}

// --------------------------------------------------------------------------------------------
Uint16 who_is_blocking_passage_ID( Uint16 passage, IDSZ idsz )
{
    // ZZ> This function returns MAX_CHR if there is no character in the passage who
    //     have an item with the given ID.  Otherwise, the index of the first character
    //     found is returned...  Only finds living characters...
    Uint16 character, sTmp;

    // Look at each character
    for ( character = 0; character < MAX_CHR; character++ )
    {
        if ( !ChrList[character].on || !ChrList[character].alive ) continue;

        if ( ( !ChrList[character].isitem ) && ChrList[character].attachedto == MAX_CHR && ChrList[character].bumpsize != 0 && !ChrList[character].pack_ispacked )
        {
            if ( is_in_passage( passage, ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].bumpsize ) )
            {
                // Found a live one...  Does it have a matching item?
                // Check the pack
                sTmp = ChrList[character].pack_next;
                while ( sTmp != MAX_CHR )
                {
                    if ( CapList[ChrList[sTmp].model].idsz[IDSZ_PARENT] == idsz || CapList[ChrList[sTmp].model].idsz[IDSZ_TYPE] == idsz )
                    {
                        // It has the item...
                        return character;
                    }

                    sTmp = ChrList[sTmp].pack_next;
                }

                // Check left hand
                sTmp = ChrList[character].holdingwhich[SLOT_LEFT];
                if ( sTmp != MAX_CHR )
                {
                    sTmp = ChrList[sTmp].model;
                    if ( CapList[sTmp].idsz[IDSZ_PARENT] == idsz || CapList[sTmp].idsz[IDSZ_TYPE] == idsz )
                    {
                        // It has the item...
                        return character;
                    }
                }

                // Check right hand
                sTmp = ChrList[character].holdingwhich[SLOT_RIGHT];
                if ( sTmp != MAX_CHR )
                {
                    sTmp = ChrList[sTmp].model;
                    if ( CapList[sTmp].idsz[IDSZ_PARENT] == idsz || CapList[sTmp].idsz[IDSZ_TYPE] == idsz )
                    {
                        // It has the item...
                        return character;
                    }
                }
            }
        }
    }

    // No characters found
    return MAX_CHR;
}

// --------------------------------------------------------------------------------------------
bool_t close_passage( Uint16 passage )
{
    // ZZ> This function makes a passage impassable, and returns btrue if it isn't blocked
    int x, y, cnt;
    Uint32 fan;
    Uint16 character;
    float bumpsize;
    Uint16 numcrushed = 0;
    Uint16 crushedcharacters[MAX_CHR];
	if( INVALID_PASSAGE( passage ) ) return bfalse;

    if ( HAS_SOME_BITS( PassageList[passage].mask, MPDFX_IMPASS | MPDFX_WALL ) )
    {
        // Make sure it isn't blocked
        for ( character = 0; character < MAX_CHR; character++ )
        {
            if ( !ChrList[character].on ) continue;

            bumpsize = ChrList[character].bumpsize;
            if ( (!ChrList[character].pack_ispacked ) && ChrList[character].attachedto == MAX_CHR && ChrList[character].bumpsize != 0 )
            {
                if ( is_in_passage( passage, ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].bumpsize ))
                {
                    if ( !ChrList[character].canbecrushed )
                    {
                        //Someone is blocking, stop here
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
            ChrList[character].ai.alert |= ALERTIF_CRUSHED;
            cnt++;
        }
    }

    // Close it off
	PassageList[passage].open = bfalse;
	y = PassageList[passage].toplefty;

	while ( y <= PassageList[passage].bottomrighty )
    {
        PassageList[passage].open = bfalse;
        y = PassageList[passage].toplefty;

        while ( y <= PassageList[passage].bottomrighty )
        {
            x = PassageList[passage].topleftx;

            while ( x <= PassageList[passage].bottomrightx )
            {
                fan = mesh_get_tile_int( PMesh, x, y );

                if ( VALID_TILE(PMesh, fan) )
                {
                    PMesh->mmem.tile_list[fan].fx = PMesh->mmem.tile_list[fan].fx | PassageList[passage].mask;
                }
                x++;
            }
            x++;
        }

        y++;
    }

    return btrue;
}

// --------------------------------------------------------------------------------------------
void clear_shop_passages()
{
    Uint16 cnt = 0;

    // ZZ> This function clears the passage list ( for doors )
    numpassage = 0;
    numshoppassage = 0;

    while ( cnt < MAX_PASS )
    {
        shopowner[cnt] = NOOWNER;
        shoppassage[cnt] = 0;
        cnt++;
    }
}

// --------------------------------------------------------------------------------------------
void add_shop_passage( Uint16 owner, Uint16 passage )
{
    // ZZ> This function creates a shop passage
    if ( VALID_PASSAGE(passage) && numshoppassage < MAX_PASS )
    {
        // The passage exists...
        shoppassage[numshoppassage] = passage;
        shopowner[numshoppassage] = owner;  // Assume the owner is alive
        numshoppassage++;
    }
}

// --------------------------------------------------------------------------------------------
void add_passage( int tlx, int tly, int brx, int bry, bool_t open, Uint8 mask )
{
    // ZZ> This function creates a passage area
    if ( numpassage < MAX_PASS )
    {
        tlx = CLIP(tlx, 0, PMesh->info.tiles_x - 1);
        tly = CLIP(tly, 0, PMesh->info.tiles_y - 1);

        brx = CLIP(brx, 0, PMesh->info.tiles_x - 1);
        bry = CLIP(bry, 0, PMesh->info.tiles_y - 1);

        PassageList[numpassage].topleftx        = tlx;
        PassageList[numpassage].toplefty        = tly;
        PassageList[numpassage].bottomrightx    = brx;
        PassageList[numpassage].bottomrighty    = bry;
        PassageList[numpassage].mask            = mask;
        PassageList[numpassage].music           = NO_MUSIC;     // Set no song as default

        // Is it open or closed?
        if (!open) close_passage( numpassage );
        else PassageList[numpassage].open = btrue;

        numpassage++;
    }
}

// --------------------------------------------------------------------------------------------
void setup_passage( const char *modname )
{
    // ZZ> This function reads the passage file
    char newloadname[256];
    char cTmp;
    int tlx, tly, brx, bry;
    bool_t open;
    Uint8 mask;
    FILE *fileread;

    // Reset all of the old passages
    clear_shop_passages();

    // Load the file
    make_newloadname( modname, "gamedat" SLASH_STR "passage.txt", newloadname );
    fileread = fopen( newloadname, "r" );
    if ( NULL == fileread ) return;

    while ( goto_colon( NULL, fileread, btrue ) )
    {
        fscanf( fileread, "%d%d%d%d", &tlx, &tly, &brx, &bry );
        cTmp = fget_first_letter( fileread );
        open = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) open = btrue;

        cTmp = fget_first_letter( fileread );
        mask = MPDFX_IMPASS | MPDFX_WALL;
        if ( cTmp == 'T' || cTmp == 't' ) mask = MPDFX_IMPASS;

        cTmp = fget_first_letter( fileread );
        if ( cTmp == 'T' || cTmp == 't' ) mask = MPDFX_SLIPPY;

        add_passage( tlx, tly, brx, bry, open, mask );
    }

    fclose( fileread );

}

//--------------------------------------------------------------------------------------------
Uint16 shop_get_owner( int ix, int iy )
{
    //This function returns the owner of a item in a shop
    int cnt;
    Uint16 owner = NOOWNER;

    for ( cnt = 0; cnt < numshoppassage; cnt++ )
    {
        Uint16 passage;

        passage = shoppassage[cnt];
        if ( INVALID_PASSAGE(passage) ) continue;

        if ( ix >= PassageList[passage].topleftx && ix <= PassageList[passage].bottomrightx )
        {
            if ( iy >= PassageList[passage].toplefty && iy <= PassageList[passage].bottomrighty )
            {
                // if there is NOOWNER, someone has been murdered!
                owner  = shopowner[cnt];
                break;
            }
        }
    }

    return owner;
}