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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
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
#include "quest.h"
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
void PassageStack_free_all()
{
    PassageStack.count = 0;
}

//--------------------------------------------------------------------------------------------
int PasageStack_get_free()
{
    int ipass = MAX_PASS;

    if( PassageStack.count < MAX_PASS )
    {
        ipass = PassageStack.count;
        PassageStack.count++;
    };

    return ipass;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ShopStack_free_all()
{
    int cnt;

    for ( cnt = 0; cnt < MAX_PASS; cnt++ )
    {
        ShopStack.lst[cnt].owner   = NOOWNER;
        ShopStack.lst[cnt].passage = 0;
    }
    ShopStack.count = 0;
}

//--------------------------------------------------------------------------------------------
int ShopStack_get_free()
{
    int ishop = MAX_PASS;

    if( ShopStack.count < MAX_PASS )
    {
        ishop = ShopStack.count;
        ShopStack.count++;
    };

    return ishop;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t open_passage( Uint16 passage )
{
    // ZZ> This function makes a passage passable
    int x, y;
    Uint32 fan;
    bool_t useful = bfalse;
    passage_t * ppass;

    if ( INVALID_PASSAGE(passage) ) return useful;
    ppass = PassageStack.lst + passage;

    useful = !ppass->open;
    ppass->open = btrue;

    if ( ppass->area.top <= ppass->area.bottom )
    {
        useful = ( !ppass->open );
        ppass->open = btrue;

        for (y = ppass->area.top; y <= ppass->area.bottom; y++ )
        {
            for (x = ppass->area.left; x <= ppass->area.right; x++ )
            {
                fan = mesh_get_tile_int( PMesh, x, y );
                if ( VALID_TILE(PMesh, fan) )
                {
                    // clear the wall and impass flags
                    PMesh->mmem.tile_list[fan].fx &= ~( MPDFX_WALL | MPDFX_IMPASS );
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
    passage_t * ppass;

    if ( INVALID_PASSAGE( passage ) ) return;
    ppass = PassageStack.lst + passage;

    for ( y = ppass->area.top; y <= ppass->area.bottom; y++ )
    {
        for ( x = ppass->area.left; x <= ppass->area.right; x++ )
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
bool_t point_is_in_passage( Uint16 passage, float xpos, float ypos )
{
    // ZF> This return btrue if the specified X and Y coordinates are within the passage
    //     radius is how much offset we allow outside the passage

    passage_t * ppass;
    frect_t tmp_rect;

    if ( INVALID_PASSAGE(passage) ) return bfalse;
    ppass = PassageStack.lst + passage;

    // Passage area
    tmp_rect.left   = ppass->area.left * TILE_SIZE;
    tmp_rect.top    = ppass->area.top * TILE_SIZE;
    tmp_rect.right  = ( ppass->area.right + 1 ) * TILE_SIZE;
    tmp_rect.bottom = ( ppass->area.bottom + 1 ) * TILE_SIZE;

    return frect_point_inside( &tmp_rect, xpos, ypos );
}

//--------------------------------------------------------------------------------------------
bool_t object_is_in_passage( Uint16 passage, float xpos, float ypos, float radius )
{
    // ZF> This return btrue if the specified X and Y coordinates are within the passage
    //     radius is how much offset we allow outside the passage

    passage_t * ppass;
    frect_t tmp_rect;

    if ( INVALID_PASSAGE(passage) ) return bfalse;
    ppass = PassageStack.lst + passage;

    // Passage area
    radius += CLOSETOLERANCE;
    tmp_rect.left   = (   ppass->area.left         * TILE_SIZE ) - radius;
    tmp_rect.top    = (   ppass->area.top          * TILE_SIZE ) - radius;
    tmp_rect.right  = ( ( ppass->area.right + 1  ) * TILE_SIZE ) + radius;
    tmp_rect.bottom = ( ( ppass->area.bottom + 1 ) * TILE_SIZE ) + radius;

    return frect_point_inside( &tmp_rect, xpos, ypos );
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
    passage_t * ppass;

    if ( INVALID_PASSAGE(passage) ) return MAX_CHR;
    ppass = PassageStack.lst + passage;

    // Look at each character
    foundother = MAX_CHR;
    for ( character = 0; character < MAX_CHR; character++ )
    {
        chr_t * pchr;

        if ( INACTIVE_CHR(character) ) continue;
        pchr = ChrList.lst + character;

        // no carried items
        if( pchr->pack_ispacked || pchr->attachedto != MAX_CHR ) continue;

        // do not do invulnerable or scenery items
        if ( pchr->invictus || pchr->weight == 0xFFFFFFFF ) continue;

        //Do items?
        if( !targetitems && pchr->isitem ) continue;

        //Do dead stuff?
        if( !targetdead && !pchr->alive ) continue;

        //Require target to have specific quest?
        if( targetquest && (!pchr->isplayer || QUEST_NONE  >= quest_check( pchr->name, findidsz )) ) continue;

        //Now check if it actually is inside the passage area
        if ( object_is_in_passage( passage, pchr->pos.x, pchr->pos.y, pchr->bump.size ) )
        {
            if ( pchr->alive && !pchr->isitem )
            {
                Uint16 item;

                // Found a live one, do we need to check for required items as well?
                if( !requireitem ) return character;

                // It needs to have a specific item as well
                else
                {
                    // I: Check left hand
                    if ( chr_is_type_idsz(pchr->holdingwhich[SLOT_LEFT],findidsz) )
                    {
                        // It has the item...
                        return character;
                    }

                    // II: Check right hand
                    if ( chr_is_type_idsz(pchr->holdingwhich[SLOT_RIGHT],findidsz) )
                    {
                        // It has the item...
                        return character;
                    }

                    // III: Check the pack
                    item = pchr->pack_next;
                    while ( item != MAX_CHR )
                    {
                        if ( chr_is_type_idsz(item,findidsz) )
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
        passage_t * ppass = PassageStack.lst + passage;

        if ( ppass->music == NO_MUSIC ) continue;

        // Look at each player
        for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
        {
            chr_t * pchr;

            character = PlaList[cnt].index;

            if ( INACTIVE_CHR(character) ) continue;
            pchr = ChrList.lst + character;

            if( pchr->pack_ispacked || !pchr->alive || !pchr->isplayer ) continue;

            // Is it in the passage?
            if (  object_is_in_passage( passage, pchr->pos.x, pchr->pos.y, pchr->bump.size  ) )
            {
                // Found a player, start music track
                sound_play_song( ppass->music, 0, -1 );
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
    passage_t * ppass;

    if ( INVALID_PASSAGE( passage ) ) return bfalse;
    ppass = PassageStack.lst + passage;

    // don't compute all of this for nothing
    if( 0 == ppass->mask ) return btrue;

    // check to see if a wall can close
    if ( HAS_SOME_BITS( ppass->mask, MPDFX_IMPASS | MPDFX_WALL ) )
    {
        Uint16 numcrushed = 0;
        Uint16 crushedcharacters[MAX_CHR];

        // Make sure it isn't blocked
        for ( character = 0; character < MAX_CHR; character++ )
        {
            chr_t * pchr;

            if ( INACTIVE_CHR(character) ) continue;
            pchr = ChrList.lst + character;

            bumpsize = pchr->bump.size;
            if ( (!pchr->pack_ispacked ) && pchr->attachedto == MAX_CHR && pchr->bump.size != 0 )
            {
                if ( object_is_in_passage( passage, pchr->pos.x, pchr->pos.y, pchr->bump.size ))
                {
                    if ( !pchr->canbecrushed )
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
        for ( cnt = 0; cnt < numcrushed; cnt++ )
        {
            character = crushedcharacters[cnt];

            chr_get_pai(character)->alert |= ALERTIF_CRUSHED;
        }
    }

    // Close it off
    ppass->open = bfalse;
    for ( y = ppass->area.top; y <= ppass->area.bottom; y++ )
    {
        for (x = ppass->area.left; x <= ppass->area.right; x++ )
        {
            fan = mesh_get_tile_int( PMesh, x, y );
            if ( VALID_TILE(PMesh, fan) )
            {
                PMesh->mmem.tile_list[fan].fx |= ppass->mask;
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void clear_all_passages()
{
    // ZZ> This function clears the passage list ( for doors )

    PassageStack_free_all();
    ShopStack_free_all();
}

//--------------------------------------------------------------------------------------------
void add_shop_passage( Uint16 owner, Uint16 passage )
{
    // ZZ> This function creates a shop passage

    int ishop;

    if( !VALID_PASSAGE(passage) ) return;

    if( INACTIVE_CHR(owner) || !ChrList.lst[owner].alive ) return;

    ishop = ShopStack_get_free();
    if( !VALID_SHOP(ishop) ) return;

    // The passage exists...
    ShopStack.lst[ishop].passage = passage;
    ShopStack.lst[ishop].owner   = owner;
}

//--------------------------------------------------------------------------------------------
void add_passage( passage_t * pdata )
{
    // ZZ> This function creates a passage area

    int         ipass;
    passage_t * ppass;

    if( NULL == pdata ) return;

    ipass = PasageStack_get_free();

    if ( ipass >= MAX_PASS ) return;
    ppass = PassageStack.lst + ipass;

    ppass->area.left      = CLIP(pdata->area.left, 0, PMesh->info.tiles_x - 1);
    ppass->area.top      = CLIP(pdata->area.top, 0, PMesh->info.tiles_y - 1);

    ppass->area.right  = CLIP(pdata->area.right, 0, PMesh->info.tiles_x - 1);
    ppass->area.bottom  = CLIP(pdata->area.bottom, 0, PMesh->info.tiles_y - 1);

    ppass->mask          = pdata->mask;
    ppass->music         = pdata->music;

    // Is it open or closed?
    if ( pdata->open )
    {
        ppass->open = btrue;
    }
    else
    {
        close_passage( ipass );
    }
}

//--------------------------------------------------------------------------------------------
void setup_all_passages( const char *modname )
{
    // ZZ> This function reads the passage file

    STRING     newloadname;
    passage_t  tmp_passage;
    vfs_FILE  *fileread;

    // Reset all of the old passages
    clear_all_passages();

    // Load the file
    make_newloadname( modname, "gamedat" SLASH_STR "passage.txt", newloadname );
    fileread = vfs_openRead( newloadname );
    if ( NULL == fileread ) return;

    while ( scan_passage_file( fileread, &tmp_passage ) )
    {
        add_passage( &tmp_passage );
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
        Uint16      passage;
        passage_t * ppass;
        shop_t    * pshop;

        pshop = ShopStack.lst + cnt;

        passage = pshop->passage;

        if ( INVALID_PASSAGE(passage) ) continue;
        ppass = PassageStack.lst + passage;

        if ( irect_point_inside( &(ppass->area), ix, iy ) )
        {
            // if there is NOOWNER, someone has been murdered!
            owner = pshop->owner;
            break;
        }
    }

    return owner;
}
