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

#include "egoboo_fileutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Passages
int   numpassage = 0;              // Number of passages in the module
int   passtlx[MAXPASS];          // Passage positions
int   passtly[MAXPASS];
int   passbrx[MAXPASS];
int   passbry[MAXPASS];
int   passagemusic[MAXPASS];        // Music track appointed to the specific passage
Uint8 passmask[MAXPASS];
Uint8 passopen[MAXPASS];      // Is the passage open?

// For shops
int     numshoppassage = 0;
Uint16  shoppassage[MAXPASS];  // The passage number
Uint16  shopowner[MAXPASS];    // Who gets the gold?

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int open_passage( Uint16 passage )
{
    // ZZ> This function makes a passage passable
    int x, y;
    Uint32 fan;
    int useful;

    useful = bfalse;
    if ( passage < numpassage )
    {
        useful = ( !passopen[passage] );
        passopen[passage] = btrue;
        y = passtly[passage];

        while ( y <= passbry[passage] )
        {
            x = passtlx[passage];

            while ( x <= passbrx[passage] )
            {
                fan = mesh_get_tile_int( &mesh, x, y );

                if ( INVALID_TILE != fan )
                {
                    mesh.mem.tile_list[fan].fx &= ~( MESHFX_WALL | MESHFX_IMPASS );
                }
                x++;
            }

            y++;
        }
    }

    return useful;
}

//--------------------------------------------------------------------------------------------
int break_passage( script_state_t * pstate, Uint16 passage, Uint16 starttile, Uint16 frames,
                   Uint16 become, Uint8 meshfxor )
{
    // ZZ> This function breaks the tiles of a passage if there is a character standing
    //     on 'em...  Turns the tiles into damage terrain if it reaches last frame.
    int x, y;
    Uint16 tile, endtile;
    Uint32 fan;
    int useful, character;

    if ( passage > numpassage ) return bfalse;

    endtile = starttile + frames - 1;
    useful = bfalse;
    for ( character = 0; character < MAXCHR; character++ )
    {
        if ( !ChrList[character].on || ChrList[character].inpack ) continue;

        if ( ChrList[character].weight > 20 && (0 == ChrList[character].flyheight) && ( ChrList[character].zpos < ChrList[character].level + 20 ) && (MAXCHR == ChrList[character].attachedto) )
        {
            fan = mesh_get_tile( ChrList[character].xpos, ChrList[character].ypos );
            if ( INVALID_TILE != fan )
            {
                tile = mesh.mem.tile_list[fan].img;
                if ( tile >= starttile && tile < endtile )
                {
                    x = ChrList[character].xpos;
                    x >>= 7;
                    if ( x >= passtlx[passage] && x <= passbrx[passage] )
                    {
                        y = ChrList[character].ypos;
                        y >>= 7;
                        if ( y >= passtly[passage] && y <= passbry[passage] )
                        {
                            // Remember where the hit occured...
                            pstate->x = ChrList[character].xpos;
                            pstate->y = ChrList[character].ypos;

                            useful = btrue;

                            // Change the tile
                            tile++;
                            if ( tile == endtile )
                            {
                                mesh.mem.tile_list[fan].fx |= meshfxor;
                                if ( become != 0 )
                                {
                                    tile = become;
                                }
                            }

                            mesh.mem.tile_list[fan].img = tile;
                        }
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

    if ( passage >= numpassage ) return;

    for ( y = passtly[passage]; y <= passbry[passage]; y++ )
    {
        for ( x = passtlx[passage]; x <= passbrx[passage]; x++ )
        {
            fan = mesh_get_tile_int( &mesh, x, y );

            if ( INVALID_TILE != fan )
            {
                Uint16 ttype = mesh.mem.tile_list[fan].type;

                ttype &= 0x3F;

                numvert = tile_dict[ttype].numvertices;
                vert    = mesh.mem.tile_list[fan].vrtstart;
                for ( cnt = 0; cnt < numvert; cnt++ )
                {
                    mesh.mem.vrt_a[vert] = color;
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
    //     must be set first, and are set on a find...  Returns btrue or bfalse
    //     depending on if it finds one or not
    int x, y;
    Uint32 fan;

    if ( passage >= numpassage ) return bfalse;

    // Do the first row
    x = pstate->x >> 7;
    y = pstate->y >> 7;

    if ( x < passtlx[passage] )  x = passtlx[passage];
    if ( y < passtly[passage] )  y = passtly[passage];

    if ( y < passbry[passage] )
    {
        while ( x <= passbrx[passage] )
        {
            fan = mesh_get_tile_int( &mesh, x, y );

            if ( INVALID_TILE != fan )
            {
                if ( (mesh.mem.tile_list[fan].img & 0xFF) == tiletype )
                {
                    pstate->x = ( x << 7 ) + 64;
                    pstate->y = ( y << 7 ) + 64;
                    return btrue;
                }

            }
            x++;
        }

        y++;
    }

    // Do all remaining rows
    while ( y <= passbry[passage] )
    {
        x = passtlx[passage];

        while ( x <= passbrx[passage] )
        {
            fan = mesh_get_tile_int( &mesh, x, y );

            if ( INVALID_TILE != fan )
            {

                if ( (mesh.mem.tile_list[fan].img & 0xFF) == tiletype )
                {
                    pstate->x = ( x << 7 ) + 64;
                    pstate->y = ( y << 7 ) + 64;
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
Uint16 who_is_blocking_passage( Uint16 passage )
{
    // ZZ> This function returns MAXCHR if there is no character in the passage,
    //     otherwise the index of the first character found is returned...
    //     Finds living ones, then items and corpses
    float tlx, tly, brx, bry;
    Uint16 character, foundother;
    float bumpsize;

    // Passage area
    tlx = ( passtlx[passage] << 7 ) - CLOSETOLERANCE;
    tly = ( passtly[passage] << 7 ) - CLOSETOLERANCE;
    brx = ( ( passbrx[passage] + 1 ) << 7 ) + CLOSETOLERANCE;
    bry = ( ( passbry[passage] + 1 ) << 7 ) + CLOSETOLERANCE;

    // Look at each character
    foundother = MAXCHR;
    character = 0;

    while ( character < MAXCHR )
    {
        if ( ChrList[character].on )
        {
            bumpsize = ChrList[character].bumpsize;
            if ( ( !ChrList[character].inpack ) && ChrList[character].attachedto == MAXCHR && bumpsize != 0 )
            {
                if ( ChrList[character].xpos > tlx - bumpsize && ChrList[character].xpos < brx + bumpsize )
                {
                    if ( ChrList[character].ypos > tly - bumpsize && ChrList[character].ypos < bry + bumpsize )
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
        }

        character++;
    }

    // No characters found
    return foundother;
}

//--------------------------------------------------------------------------------------------
void check_passage_music()
{
    // This function checks all passages if there is a player in it, if it is, it plays a specified
    // song set in by the AI script functions
    float tlx, tly, brx, bry;
    Uint16 character, passage;
    float bumpsize;

    passage = 0;

    while ( passage < numpassage )
    {
        if ( passagemusic[passage] != -1 )       // Only check passages that have music assigned to them
        {
            // Passage area
            tlx = ( passtlx[passage] << 7 ) - CLOSETOLERANCE;
            tly = ( passtly[passage] << 7 ) - CLOSETOLERANCE;
            brx = ( ( passbrx[passage] + 1 ) << 7 ) + CLOSETOLERANCE;
            bry = ( ( passbry[passage] + 1 ) << 7 ) + CLOSETOLERANCE;

            // Look at each character
            character = 0;

            while ( character < MAXCHR )
            {
                if ( ChrList[character].on )
                {
                    bumpsize = ChrList[character].bumpsize;
                    if ( ( !ChrList[character].inpack ) && ChrList[character].attachedto == MAXCHR && bumpsize != 0 )
                    {
                        if ( ChrList[character].xpos > tlx - bumpsize && ChrList[character].xpos < brx + bumpsize )
                        {
                            if ( ChrList[character].ypos > tly - bumpsize && ChrList[character].ypos < bry + bumpsize )
                            {
                                if ( ChrList[character].alive && !ChrList[character].isitem && ChrList[character].isplayer )
                                {
                                    // Found a player, start music track
                                    sound_play_song( passagemusic[passage], 0, -1 );
                                }
                            }
                        }
                    }
                }

                character++;
            }
        }

        passage++;
    }
}

//--------------------------------------------------------------------------------------------
Uint16 who_is_blocking_passage_ID( Uint16 passage, Uint32 idsz )
{
    // ZZ> This function returns MAXCHR if there is no character in the passage who
    //     have an item with the given ID.  Otherwise, the index of the first character
    //     found is returned...  Only finds living characters...
    float tlx, tly, brx, bry;
    Uint16 character, sTmp;
    float bumpsize;

    // Passage area
    tlx = ( passtlx[passage] << 7 ) - CLOSETOLERANCE;
    tly = ( passtly[passage] << 7 ) - CLOSETOLERANCE;
    brx = ( ( passbrx[passage] + 1 ) << 7 ) + CLOSETOLERANCE;
    bry = ( ( passbry[passage] + 1 ) << 7 ) + CLOSETOLERANCE;

    // Look at each character
    character = 0;

    while ( character < MAXCHR )
    {
        if ( ChrList[character].on )
        {
            bumpsize = ChrList[character].bumpsize;
            if ( ( !ChrList[character].isitem ) && bumpsize != 0 && ChrList[character].inpack == 0 )
            {
                if ( ChrList[character].xpos > tlx - bumpsize && ChrList[character].xpos < brx + bumpsize )
                {
                    if ( ChrList[character].ypos > tly - bumpsize && ChrList[character].ypos < bry + bumpsize )
                    {
                        if ( ChrList[character].alive )
                        {
                            // Found a live one...  Does it have a matching item?

                            // Check the pack
                            sTmp = ChrList[character].nextinpack;

                            while ( sTmp != MAXCHR )
                            {
                                if ( CapList[ChrList[sTmp].model].idsz[IDSZ_PARENT] == idsz || CapList[ChrList[sTmp].model].idsz[IDSZ_TYPE] == idsz )
                                {
                                    // It has the item...
                                    return character;
                                }

                                sTmp = ChrList[sTmp].nextinpack;
                            }

                            // Check left hand
                            sTmp = ChrList[character].holdingwhich[0];
                            if ( sTmp != MAXCHR )
                            {
                                sTmp = ChrList[sTmp].model;
                                if ( CapList[sTmp].idsz[IDSZ_PARENT] == idsz || CapList[sTmp].idsz[IDSZ_TYPE] == idsz )
                                {
                                    // It has the item...
                                    return character;
                                }
                            }

                            // Check right hand
                            sTmp = ChrList[character].holdingwhich[1];
                            if ( sTmp != MAXCHR )
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
            }
        }

        character++;
    }

    // No characters found
    return MAXCHR;
}

//--------------------------------------------------------------------------------------------
int close_passage( Uint16 passage )
{
    // ZZ> This function makes a passage impassable, and returns btrue if it isn't blocked
    int x, y, cnt;
    float tlx, tly, brx, bry;
    Uint32 fan;
    Uint16 character;
    float bumpsize;
    Uint16 numcrushed;
    Uint16 crushedcharacters[MAXCHR];
    if ( ( passmask[passage]&( MESHFX_IMPASS | MESHFX_WALL ) ) )
    {
        // Make sure it isn't blocked
        tlx = ( passtlx[passage] << 7 ) - CLOSETOLERANCE;
        tly = ( passtly[passage] << 7 ) - CLOSETOLERANCE;
        brx = ( ( passbrx[passage] + 1 ) << 7 ) + CLOSETOLERANCE;
        bry = ( ( passbry[passage] + 1 ) << 7 ) + CLOSETOLERANCE;
        numcrushed = 0;
        character = 0;

        while ( character < MAXCHR )
        {
            bumpsize = ChrList[character].bumpsize;
            if ( ChrList[character].on && ( !ChrList[character].inpack ) && ChrList[character].attachedto == MAXCHR && ChrList[character].bumpsize != 0 )
            {
                if ( ChrList[character].xpos > tlx - bumpsize && ChrList[character].xpos < brx + bumpsize )
                {
                    if ( ChrList[character].ypos > tly - bumpsize && ChrList[character].ypos < bry + bumpsize )
                    {
                        if ( !ChrList[character].canbecrushed )
                        {
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

            character++;
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
    if ( passage < numpassage )
    {
        passopen[passage] = bfalse;
        y = passtly[passage];

        while ( y <= passbry[passage] )
        {
            x = passtlx[passage];

            while ( x <= passbrx[passage] )
            {
                fan = mesh_get_tile_int( &mesh, x, y );

                if ( INVALID_TILE != fan )
                {
                    mesh.mem.tile_list[fan].fx = mesh.mem.tile_list[fan].fx | passmask[passage];
                }
                x++;
            }

            y++;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void clear_passages()
{
    int cnt = 0;

    // ZZ> This function clears the passage list ( for doors )
    numpassage = 0;
    numshoppassage = 0;

    while ( cnt < MAXPASS )
    {
        shopowner[cnt] = NOOWNER;
        shoppassage[cnt] = 0;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void add_shop_passage( Uint16 owner, Uint16 passage )
{
    // ZZ> This function creates a shop passage
    if ( passage < numpassage && numshoppassage < MAXPASS )
    {
        // The passage exists...
        shoppassage[numshoppassage] = passage;
        shopowner[numshoppassage] = owner;  // Assume the owner is alive
        numshoppassage++;
    }
}

//--------------------------------------------------------------------------------------------
void add_passage( int tlx, int tly, int brx, int bry, Uint8 open, Uint8 mask )
{
    // ZZ> This function creates a passage area

    if ( numpassage < MAXPASS )
    {
        tlx = CLIP(tlx, 0, mesh.info.tiles_x - 1);
        tly = CLIP(tly, 0, mesh.info.tiles_y - 1);

        brx = CLIP(brx, 0, mesh.info.tiles_x - 1);
        bry = CLIP(bry, 0, mesh.info.tiles_y - 1);

        passtlx[numpassage]      = tlx;
        passtly[numpassage]      = tly;
        passbrx[numpassage]      = brx;
        passbry[numpassage]      = bry;
        passmask[numpassage]     = mask;
        passagemusic[numpassage] = -1;          // Set no song as default
        passopen[numpassage]     = (open == btrue);

        numpassage++;
    }
}

//--------------------------------------------------------------------------------------------
void setup_passage(  const char *modname )
{
    // ZZ> This function reads the passage file
    char newloadname[256];
    char cTmp;
    int tlx, tly, brx, bry;
    bool_t open;
    Uint8 mask;
    FILE *fileread;

    // Reset all of the old passages
    clear_passages();

    // Load the file
    make_newloadname( modname, "/gamedat/passage.txt", newloadname );
    fileread = fopen( newloadname, "r" );
    if ( fileread )
    {
        while ( goto_colon_yesno( fileread ) )
        {
            fscanf( fileread, "%d%d%d%d", &tlx, &tly, &brx, &bry );
            cTmp = fget_first_letter( fileread );
            open = bfalse;
            if ( cTmp == 'T' || cTmp == 't' ) open = btrue;

            cTmp = fget_first_letter( fileread );
            mask = MESHFX_IMPASS | MESHFX_WALL;
            if ( cTmp == 'T' || cTmp == 't' ) mask = MESHFX_IMPASS;

            cTmp = fget_first_letter( fileread );
            if ( cTmp == 'T' || cTmp == 't' ) mask = MESHFX_SLIPPY;

            add_passage( tlx, tly, brx, bry, open, mask );
        }

        fclose( fileread );
    }
}
