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

/// @file passage.c
/// @brief Passages and doors and whatnot.  Things that impede your progress!

#include "passage.h"

#include "../egolib/fileutil.h"
#include "../egolib/_math.h"
#include "../egolib/file_formats/quest_file.h"

#include "script.h"
#include "sound.h"
#include "game.h"
#include "network.h"
#include "player.h"
#include "egoboo.h"

#include "char.inl"
#include "mesh.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INSTANTIATE_STACK( ACCESS_TYPE_NONE, passage_t, PassageStack, MAX_PASS );
INSTANTIATE_STACK( ACCESS_TYPE_NONE, shop_t,    ShopStack, MAX_SHOP );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int ShopStack_get_free( void );
static void ShopStack_free_all( void );
static int PasageStack_get_free( void );
static void PassageStack_free_all( void );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

IMPLEMENT_STACK( passage_t, PassageStack, MAX_PASS );
IMPLEMENT_STACK( shop_t,    ShopStack, MAX_SHOP );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void PassageStack_free_all( void )
{
    PassageStack.count = 0;
}

//--------------------------------------------------------------------------------------------
int PasageStack_get_free( void )
{
    int ipass = MAX_PASS;

    if ( PassageStack.count < MAX_PASS )
    {
        ipass = PassageStack.count;
        PassageStack.count++;
    };

    return ipass;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ShopStack_free_all( void )
{
    SHOP_REF cnt;

    for ( cnt = 0; cnt < MAX_PASS; cnt++ )
    {
        ShopStack.lst[cnt].owner   = SHOP_NOOWNER;
        ShopStack.lst[cnt].passage = 0;
    }
    ShopStack.count = 0;
}

//--------------------------------------------------------------------------------------------
int ShopStack_get_free( void )
{
    int ishop = INVALID_PASS_REF;

    if ( ShopStack.count < MAX_PASS )
    {
        ishop = ShopStack.count;
        ShopStack.count++;
    };

    return ishop;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t open_passage( const PASS_REF passage )
{
    /// @author ZZ
    /// @details This function makes a passage passable

    int x, y;
    Uint32 fan;
    passage_t * ppass;

    if ( INVALID_PASSAGE( passage ) ) return bfalse;
    ppass = PassageStack_get_ptr( passage );

    //no need to do this if it already is open
    if ( ppass->open ) return btrue;

    if ( ppass->area.top <= ppass->area.bottom )
    {
        ppass->open = btrue;
        for ( y = ppass->area.top; y <= ppass->area.bottom; y++ )
        {
            for ( x = ppass->area.left; x <= ppass->area.right; x++ )
            {
                fan = ego_mesh_get_tile_int( PMesh, x, y );
                ego_mesh_clear_fx( PMesh, fan, MAPFX_WALL | MAPFX_IMPASS );
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void flash_passage( const PASS_REF passage, Uint8 color )
{
    /// @author ZZ
    /// @details This function makes a passage flash white

    int x, y, cnt;
    Uint32 fan;
    passage_t * ppass = NULL;
    ego_tile_info_t * ptile = NULL;

    if ( INVALID_PASSAGE( passage ) ) return;
    ppass = PassageStack_get_ptr( passage );

    for ( y = ppass->area.top; y <= ppass->area.bottom; y++ )
    {
        for ( x = ppass->area.left; x <= ppass->area.right; x++ )
        {
            fan = ego_mesh_get_tile_int( PMesh, x, y );

            ptile = ego_mesh_get_ptile( PMesh, fan );
            if ( NULL == ptile ) continue;

            for ( cnt = 0; cnt < 4; cnt++ )
            {
                // set the color
                ptile->lcache[cnt]       = color;

                // force the lighting code to update
                ptile->request_clst_update = btrue;
                ptile->clst_frame        = -1;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t point_is_in_passage( const PASS_REF passage, float xpos, float ypos )
{
    /// @author ZF
    /// @details This return btrue if the specified X and Y coordinates are within the passage

    passage_t * ppass;
    frect_t tmp_rect;

    if ( INVALID_PASSAGE( passage ) ) return bfalse;
    ppass = PassageStack_get_ptr( passage );

    // Passage area
    tmp_rect.left   = ppass->area.left * GRID_FSIZE;
    tmp_rect.top    = ppass->area.top * GRID_FSIZE;
    tmp_rect.right  = ( ppass->area.right + 1 ) * GRID_FSIZE;
    tmp_rect.bottom = ( ppass->area.bottom + 1 ) * GRID_FSIZE;

    return frect_point_inside( &tmp_rect, xpos, ypos );
}

//--------------------------------------------------------------------------------------------
bool_t object_is_in_passage( const PASS_REF passage, float xpos, float ypos, float radius )
{
    /// @author ZF
    /// @details This return btrue if the specified X and Y coordinates are within the passage
    ///     radius is how much offset we allow outside the passage

    passage_t * ppass;
    frect_t tmp_rect;

    if ( INVALID_PASSAGE( passage ) ) return bfalse;
    ppass = PassageStack_get_ptr( passage );

    // Passage area
    radius += CLOSETOLERANCE;
    tmp_rect.left   = ( ppass->area.left          * GRID_FSIZE ) - radius;
    tmp_rect.top    = ( ppass->area.top           * GRID_FSIZE ) - radius;
    tmp_rect.right  = (( ppass->area.right + 1 )  * GRID_FSIZE ) + radius;
    tmp_rect.bottom = (( ppass->area.bottom + 1 ) * GRID_FSIZE ) + radius;

    return frect_point_inside( &tmp_rect, xpos, ypos );
}

//--------------------------------------------------------------------------------------------
CHR_REF who_is_blocking_passage( const PASS_REF passage, const CHR_REF isrc, IDSZ idsz, const BIT_FIELD targeting_bits, IDSZ require_item )
{
    /// @author ZZ
    /// @details This function returns MAX_CHR if there is no character in the passage,
    ///    otherwise the index of the first character found is returned...
    ///    Can also look for characters with a specific quest or item in his or her inventory
    ///    Finds living ones, then items and corpses

    CHR_REF character, foundother;
    passage_t * ppass;
    chr_t *psrc;

    // Skip if the one who is looking doesn't exist
    if ( !INGAME_CHR( isrc ) ) return INVALID_CHR_REF;
    psrc = ChrList_get_ptr( isrc );

    // Skip invalid passages
    if ( INVALID_PASSAGE( passage ) ) return INVALID_CHR_REF;
    ppass = PassageStack_get_ptr( passage );

    // Look at each character
    foundother = INVALID_CHR_REF;
    for ( character = 0; character < MAX_CHR; character++ )
    {
        chr_t * pchr;

        if ( !INGAME_CHR( character ) ) continue;
        pchr = ChrList_get_ptr( character );

        // dont do scenery objects unless we allow items
        if ( !HAS_SOME_BITS( targeting_bits, TARGET_ITEMS ) && ( CHR_INFINITE_WEIGHT == pchr->phys.weight ) ) continue;

        //Check if the object has the requirements
        if ( !chr_check_target( psrc, character, idsz, targeting_bits ) ) continue;

        //Now check if it actually is inside the passage area
        if ( object_is_in_passage( passage, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
        {
            // Found a live one, do we need to check for required items as well?
            if ( IDSZ_NONE == require_item )
            {
                return character;
            }

            // It needs to have a specific item as well
            else
            {
                // I: Check left hand
                if ( chr_is_type_idsz( pchr->holdingwhich[SLOT_LEFT], require_item ) )
                {
                    // It has the item...
                    return character;
                }

                // II: Check right hand
                if ( chr_is_type_idsz( pchr->holdingwhich[SLOT_RIGHT], require_item ) )
                {
                    // It has the item...
                    return character;
                }

                // III: Check the pack
                PACK_BEGIN_LOOP( pchr->inventory, pitem, item )
                {
                    if ( chr_is_type_idsz( item, require_item ) )
                    {
                        // It has the ipacked in inventory...
                        return character;
                    }
                }
                PACK_END_LOOP();
            }
        }
    }

    // No characters found
    return foundother;
}

//--------------------------------------------------------------------------------------------
void check_passage_music( void )
{
    /// @author ZF
    /// @details This function checks all passages if there is a player in it, if it is, it plays a specified
    /// song set in by the AI script functions

    CHR_REF character = INVALID_CHR_REF;
    PASS_REF passage;

    // Check every music passage
    for ( passage = 0; passage < PassageStack.count; passage++ )
    {
        PLA_REF ipla;
        passage_t * ppass = PassageStack_get_ptr( passage );

        if ( ppass->music == NO_MUSIC || ppass->music == get_current_song_playing() ) continue;

        // Look at each player
        for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
        {
            chr_t * pchr;

            character = PlaStack.lst[ipla].index;

            if ( !INGAME_CHR( character ) ) continue;
            pchr = ChrList_get_ptr( character );

            //dont do items in hands or inventory
            if ( IS_ATTACHED_CHR( character ) ) continue;

            if ( !pchr->alive || !VALID_PLA( pchr->is_which_player ) ) continue;

            // Is it in the passage?
            if ( object_is_in_passage( passage, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
            {
                // Found a player, start music track
                sound_play_song( ppass->music, 0, -1 );
                return;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t close_passage( const PASS_REF passage )
{
    /// @author ZZ
    /// @details This function makes a passage impassable, and returns btrue if it isn't blocked
    int x, y;
    Uint32 fan, cnt;
    passage_t * ppass;
    CHR_REF character;

    if ( INVALID_PASSAGE( passage ) ) return bfalse;
    ppass = PassageStack_get_ptr( passage );

    //is it already closed?
    if ( !ppass->open ) return btrue;

    // don't compute all of this for nothing
    if ( EMPTY_BIT_FIELD == ppass->mask ) return btrue;

    // check to see if a wall can close
    if ( 0 != HAS_SOME_BITS( ppass->mask, MAPFX_IMPASS | MAPFX_WALL ) )
    {
        size_t  numcrushed = 0;
        CHR_REF crushedcharacters[MAX_CHR];

        // Make sure it isn't blocked
        for ( character = 0; character < MAX_CHR; character++ )
        {
            chr_t *pchr;

            if ( !INGAME_CHR( character ) ) continue;
            pchr = ChrList_get_ptr( character );

            //Don't do held items
            if ( IS_ATTACHED_CHR( character ) ) continue;

            if ( 0.0f != pchr->bump_stt.size )
            {
                if ( object_is_in_passage( passage, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
                {
                    if ( !pchr->canbecrushed || ( pchr->alive && pchr->openstuff ) )
                    {
                        // Someone is blocking who can open stuff, stop here
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
            SET_BIT( chr_get_pai( character )->alert, ALERTIF_CRUSHED );
        }
    }

    // Close it off
    ppass->open = bfalse;
    for ( y = ppass->area.top; y <= ppass->area.bottom; y++ )
    {
        for ( x = ppass->area.left; x <= ppass->area.right; x++ )
        {
            fan = ego_mesh_get_tile_int( PMesh, x, y );
            ego_mesh_add_fx( PMesh, fan, ppass->mask );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void clear_all_passages( void )
{
    /// @author ZZ
    /// @details This function clears the passage list ( for doors )

    PassageStack_free_all();
    ShopStack_free_all();
}

//--------------------------------------------------------------------------------------------
void add_shop_passage( const CHR_REF owner, const PASS_REF passage )
{
    /// @author ZZ
    /// @details This function creates a shop passage

    SHOP_REF ishop;
    CHR_REF  ichr;

    if ( !VALID_PASSAGE( passage ) ) return;

    if ( !INGAME_CHR( owner ) || !ChrList.lst[owner].alive ) return;

    ishop = (SHOP_REF)ShopStack_get_free();
    if ( !VALID_SHOP( ishop ) ) return;

    // The passage exists...
    ShopStack.lst[ishop].passage = passage;
    ShopStack.lst[ishop].owner   = owner;

    // flag every item in the shop as a shop item
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        chr_t * pchr;

        if ( !INGAME_CHR( ichr ) ) continue;
        pchr = ChrList_get_ptr( ichr );

        if ( pchr->isitem )
        {
            if ( object_is_in_passage( ShopStack.lst[ishop].passage, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
            {
                pchr->isshopitem = btrue;               // Full value
                pchr->iskursed   = bfalse;              // Shop items are never kursed
                pchr->nameknown  = btrue;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void add_passage( passage_t * pdata )
{
    /// @author ZZ
    /// @details This function creates a passage area

    PASS_REF    ipass;
    passage_t * ppass;

    if ( NULL == pdata ) return;

    ipass = (PASS_REF)PasageStack_get_free();

    if ( ipass >= MAX_PASS ) return;
    ppass = PassageStack_get_ptr( ipass );

    ppass->area.left      = CLIP( pdata->area.left, 0, PMesh->info.tiles_x - 1 );
    ppass->area.top      = CLIP( pdata->area.top, 0, PMesh->info.tiles_y - 1 );

    ppass->area.right  = CLIP( pdata->area.right, 0, PMesh->info.tiles_x - 1 );
    ppass->area.bottom  = CLIP( pdata->area.bottom, 0, PMesh->info.tiles_y - 1 );

    ppass->mask          = pdata->mask;
    ppass->music         = pdata->music;
    ppass->open          = btrue;

    // Is it closed? (default is open)
    if ( !pdata->open )
    {
        close_passage( ipass );
    }
}

//--------------------------------------------------------------------------------------------
void activate_passages_file_vfs( void )
{
    /// @author ZZ
    /// @details This function reads the passage file
    passage_t  tmp_passage;
    vfs_FILE  *fileread;

    // Reset all of the old passages
    clear_all_passages();

    // Load the file
    fileread = vfs_openRead( "mp_data/passage.txt" );
    if ( NULL == fileread ) return;

    while ( scan_passage_file( fileread, &tmp_passage ) )
    {
        add_passage( &tmp_passage );
    }

    vfs_close( fileread );
}

//--------------------------------------------------------------------------------------------
CHR_REF shop_get_owner( int ix, int iy )
{
    /// @author ZZ
    /// @details This function returns the owner of a item in a shop

    SHOP_REF cnt;
    CHR_REF  owner = ( CHR_REF )SHOP_NOOWNER;

    for ( cnt = 0; cnt < ShopStack.count; cnt++ )
    {
        PASS_REF    passage;
        passage_t * ppass;
        shop_t    * pshop;

        pshop = ShopStack_get_ptr( cnt );

        passage = pshop->passage;

        if ( INVALID_PASSAGE( passage ) ) continue;
        ppass = PassageStack_get_ptr( passage );

        if ( irect_point_inside( &( ppass->area ), ix, iy ) )
        {
            // if there is SHOP_NOOWNER, someone has been murdered!
            owner = pshop->owner;
            break;
        }
    }

    return owner;
}
