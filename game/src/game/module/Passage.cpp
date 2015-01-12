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

/// @file egolib/file_formats/Passage.cpp
/// @brief A scanner for the passage file for a given module ( /modules/*.mod/basicdat/passages.txt )
/// @author Johan Jansen

#include <forward_list>
#include "game/module/Passage.hpp"
#include "game/game.h"
#include "game/char.h"
#include "game/mesh.h"
#include "game/ChrList.h"

Passage::Passage() :
    _area{0, 0, 0, 0},
    _music(NO_MUSIC),
    _mask(MAPFX_IMPASS | MAPFX_WALL),
    _open(true),
    _isShop(false),
    _shopOwner(SHOP_NOOWNER)
{
    //ctor
}

Passage::Passage(const irect_t& area, const uint8_t mask) :
    _area(area),
    _music(NO_MUSIC),
    _mask(mask),
    _open(true),
    _isShop(false),
    _shopOwner(SHOP_NOOWNER)
{
    //ctor
}

bool Passage::isOpen() const
{
    return _open;
}

void Passage::open()
{
    //no need to do this if it already is open
    if ( isOpen() ) {
       return; 
    } 

    if ( _area.top <= _area.bottom )
    {
        _open = true;
        for ( int y = _area.top; y <= _area.bottom; y++ )
        {
            for ( int x = _area.left; x <= _area.right; x++ )
            {
                //clear impassable and wall bits
                uint32_t fan = ego_mesh_get_tile_int( PMesh, x, y );
                ego_mesh_clear_fx( PMesh, fan, MAPFX_WALL | MAPFX_IMPASS );
            }
        }
    }

}

bool Passage::close()
{
    //is it already closed?
    if(!isOpen()) {
        return true;
    }

    // don't compute all of this for nothing
    if ( EMPTY_BIT_FIELD == _mask ) {
        return true;
    }

    // check to see if a wall can close
    if ( 0 != HAS_SOME_BITS( _mask, MAPFX_IMPASS | MAPFX_WALL ) )
    {
        std::forward_list<CHR_REF> crushedCharacters;

        // Make sure it isn't blocked
        for ( CHR_REF character = 0; character < MAX_CHR; character++ )
        {
            chr_t *pchr;

            if ( !INGAME_CHR( character ) ) continue;
            pchr = ChrList_get_ptr( character );

            //Don't do held items
            if ( IS_ATTACHED_CHR( character ) ) continue;

            if ( 0.0f != pchr->bump_stt.size )
            {
                if ( objectIsInPassage( pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
                {
                    if ( !pchr->canbecrushed || ( pchr->alive && pchr->openstuff ) )
                    {
                        // Someone is blocking who can open stuff, stop here
                        return false;
                    }
                    else
                    {
                        crushedCharacters.push_front(character);
                    }
                }
            }
        }

        // Crush any unfortunate characters
        for(CHR_REF character : crushedCharacters) {
            SET_BIT( chr_get_pai( character )->alert, ALERTIF_CRUSHED );
        }
    }

    // Close it off
    _open = false;
    for ( int y = _area.top; y <= _area.bottom; y++ )
    {
        for ( int x = _area.left; x <= _area.right; x++ )
        {
            Uint32 fan = ego_mesh_get_tile_int( PMesh, x, y );
            ego_mesh_add_fx( PMesh, fan, _mask );
        }
    }

    return true;    
}

//--------------------------------------------------------------------------------------------
bool Passage::objectIsInPassage( float xpos, float ypos, float radius ) const
{
    frect_t tmp_rect;

    // Passage area
    radius += CLOSE_TOLERANCE;
    tmp_rect.left   = ( _area.left          * GRID_FSIZE ) - radius;
    tmp_rect.top    = ( _area.top           * GRID_FSIZE ) - radius;
    tmp_rect.right  = (( _area.right + 1 )  * GRID_FSIZE ) + radius;
    tmp_rect.bottom = (( _area.bottom + 1 ) * GRID_FSIZE ) + radius;

    return frect_point_inside( &tmp_rect, xpos, ypos );
}

CHR_REF Passage::whoIsBlockingPassage( const CHR_REF isrc, IDSZ idsz, const BIT_FIELD targeting_bits, IDSZ require_item ) const
{
    // Skip if the one who is looking doesn't exist
    if ( !INGAME_CHR( isrc ) ) return INVALID_CHR_REF;
    chr_t *psrc = ChrList_get_ptr( isrc );

    // Look at each character
    for ( CHR_REF character = 0; character < MAX_CHR; character++ )
    {
        if ( !INGAME_CHR( character ) ) continue;
        chr_t * pchr = ChrList_get_ptr( character );

        // dont do scenery objects unless we allow items
        if ( !HAS_SOME_BITS( targeting_bits, TARGET_ITEMS ) && ( CHR_INFINITE_WEIGHT == pchr->phys.weight ) ) continue;

        //Check if the object has the requirements
        if ( !chr_check_target( psrc, character, idsz, targeting_bits ) ) continue;

        //Now check if it actually is inside the passage area
        if ( objectIsInPassage( pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
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
    return INVALID_CHR_REF;
}

void Passage::flashColor(uint8_t color)
{
    for (int y = _area.top; y <= _area.bottom; y++ )
    {
        for (int x = _area.left; x <= _area.right; x++ )
        {
            Uint32 fan = ego_mesh_get_tile_int( PMesh, x, y );

            ego_tile_info_t *ptile = ego_mesh_get_ptile( PMesh, fan );
            if ( NULL == ptile ) continue;

            for (int cnt = 0; cnt < 4; cnt++ )
            {
                // set the color
                ptile->lcache[cnt]       = color;

                // force the lighting code to update
                ptile->request_clst_update = true;
                ptile->clst_frame          = -1;
            }
        }
    }
}

bool Passage::isPointInside( float xpos, float ypos ) const
{
    frect_t tmp_rect;

    // Passage area
    tmp_rect.left   = _area.left * GRID_FSIZE;
    tmp_rect.top    = _area.top * GRID_FSIZE;
    tmp_rect.right  = ( _area.right + 1 ) * GRID_FSIZE;
    tmp_rect.bottom = ( _area.bottom + 1 ) * GRID_FSIZE;

    return frect_point_inside( &tmp_rect, xpos, ypos );
}

bool Passage::checkPassageMusic(const chr_t * pchr) const
{
    if ( _music == NO_MUSIC || _music == get_current_song_playing() ) {
       return false; 
    } 

    if(!objectIsInPassage(pchr->pos.x, pchr->pos.y, pchr->bump_1.size)) {
        return false;
    }

    // character is inside, start music track
    sound_play_song( _music, 0, -1 );
    return true;
}

void Passage::setMusic(const int32_t musicID)
{
    _music = musicID;
}

bool Passage::isShop() const
{
    return _shopOwner;
}

CHR_REF Passage::getShopOwner() const
{
    if(!isShop()) {
        return INVALID_CHR_REF;
    }

    return _shopOwner;
}

void Passage::makeShop(CHR_REF owner)
{
    //Make sure owner is valid
    if ( !INGAME_CHR( owner ) || !ChrList.lst[owner].alive ) return;

    //Mark as shop
    _isShop = true;
    _shopOwner = owner;

    // flag every item in the shop as a shop item
    for ( CHR_REF ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        if ( !INGAME_CHR( ichr ) ) continue;
        chr_t * pchr = ChrList_get_ptr( ichr );

        if ( pchr->isitem )
        {
            if ( objectIsInPassage( pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
            {
                pchr->isshopitem = true;               // Full value
                pchr->iskursed   = false;              // Shop items are never kursed
                pchr->nameknown  = true;
            }
        }
    }    
}

void Passage::removeShop()
{
    if(!isShop()) {
        return;
    }

    _isShop = false;
    _shopOwner = SHOP_NOOWNER;
}