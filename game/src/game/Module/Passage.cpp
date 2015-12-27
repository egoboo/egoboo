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

/// @file game/Module/Passage.cpp
/// @brief A scanner for the passage file for a given module ( /modules/*.mod/basicdat/passages.txt )
/// @author Johan Jansen

#include "game/Module/Passage.hpp"
#include "game/game.h"
#include "game/char.h"
#include "game/mesh.h"
#include "game/Entities/_Include.hpp"

const ObjectRef Passage::SHOP_NOOWNER = ObjectRef::Invalid;

Passage::Passage(GameModule &module, const irect_t& area, const uint8_t mask) :
    _module(module),
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

    if ( _area._top <= _area._bottom )
    {
        _open = true;
        for ( int y = _area._top; y <= _area._bottom; y++ )
        {
            for ( int x = _area._left; x <= _area._right; x++ )
            {
                //clear impassable and wall bits
				auto mesh = _module.getMeshPointer();
                Index1D fan = mesh->getTileIndex(Index2D(x, y));
				mesh->clear_fx( fan, MAPFX_WALL | MAPFX_IMPASS );
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
        std::vector<std::shared_ptr<Object>> crushedCharacters;

        // Make sure it isn't blocked
        for(const std::shared_ptr<Object> &object : _module.getObjectHandler().iterator())
        {
            if(object->isTerminated()) {
                continue;
            }

            //Don't do held items
            if (object->isBeingHeld()) continue;

            if ( 0.0f != object->bump_stt.size )
            {
                if ( objectIsInPassage( object->getPosX(), object->getPosY(), object->bump_1.size ) )
                {
                    if ( !object->canbecrushed || ( object->isAlive() && object->getProfile()->canOpenStuff() ) )
                    {
                        // Someone is blocking who can open stuff, stop here
                        return false;
                    }
                    else
                    {
                        crushedCharacters.push_back(object);
                    }
                }
            }
        }

        // Crush any unfortunate characters
        for(const std::shared_ptr<Object> &character : crushedCharacters) {
            SET_BIT( character->ai.alert, ALERTIF_CRUSHED );
        }
    }

    // Close it off
    _open = false;
    for ( int y = _area._top; y <= _area._bottom; y++ )
    {
        for ( int x = _area._left; x <= _area._right; x++ )
        {
            Index1D fan = _module.getMeshPointer()->getTileIndex(Index2D(x, y));
			_module.getMeshPointer()->add_fx( fan, _mask );
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
    tmp_rect._left   = ( _area._left          * Info<float>::Grid::Size()) - radius;
    tmp_rect._top    = ( _area._top           * Info<float>::Grid::Size()) - radius;
    tmp_rect._right  = (( _area._right + 1 )  * Info<float>::Grid::Size()) + radius;
    tmp_rect._bottom = (( _area._bottom + 1 ) * Info<float>::Grid::Size()) + radius;

    return tmp_rect.point_inside(xpos, ypos);
}

ObjectRef Passage::whoIsBlockingPassage( ObjectRef objRef, const IDSZ2& idsz, const BIT_FIELD targeting_bits, const IDSZ2& require_item ) const
{
    // Skip if the one who is looking doesn't exist
    if ( !_module.getObjectHandler().exists(objRef) ) return ObjectRef::Invalid;
    Object *psrc = _module.getObjectHandler().get(objRef);

    // Look at each character
    for(const std::shared_ptr<Object> &pchr : _module.getObjectHandler().iterator())
    {
        if(pchr->isTerminated()) {
            continue;
        }

        // dont do scenery objects unless we allow items
        if ( !HAS_SOME_BITS( targeting_bits, TARGET_ITEMS ) && ( CHR_INFINITE_WEIGHT == pchr->phys.weight ) ) continue;

        //Check if the object has the requirements
        if ( !chr_check_target( psrc, pchr, idsz, targeting_bits ) ) continue;

        //Now check if it actually is inside the passage area
        if ( objectIsInPassage( pchr->getPosX(), pchr->getPosY(), pchr->bump_1.size ) )
        {
            // Found a live one, do we need to check for required items as well?
            if ( IDSZ2::None == require_item )
            {
                return pchr->getObjRef();
            }

            // It needs to have a specific item as well
            else
            {
                // I: Check hands
                if(pchr->isWieldingItemIDSZ(require_item)) {
                    return pchr->getObjRef();
                }
                
                // II: Check the pack
                for(const std::shared_ptr<Object> pitem : pchr->getInventory().iterate())
                {
                    if ( pitem->getProfile()->hasTypeIDSZ(require_item) )
                    {
                        // It has the ipacked in inventory...
                        return pchr->getObjRef();
                    }
                }
            }
        }
    }

    // No characters found
    return ObjectRef::Invalid;
}

void Passage::flashColor(uint8_t color)
{
    for (int y = _area._top; y <= _area._bottom; y++ )
    {
        for (int x = _area._left; x <= _area._right; x++ )
        {
            Index1D fan = _module.getMeshPointer()->getTileIndex(Index2D(x, y));

            ego_tile_info_t& ptile = _module.getMeshPointer()->getTileInfo(fan);

            for (size_t cnt = 0; cnt < 4; ++cnt)
            {
                // set the color
                ptile._lightingCache._contents[cnt] = color;

                // force the lighting code to update
                ptile._vertexLightingCache.setNeedUpdate(true);
                ptile._vertexLightingCache._lastFrame = -1;
            }
        }
    }
}

bool Passage::isPointInside( float xpos, float ypos ) const
{
    frect_t tmp_rect;

    // Passage area
    tmp_rect._left   = _area._left * Info<float>::Grid::Size();
    tmp_rect._top    = _area._top * Info<float>::Grid::Size();
    tmp_rect._right  = ( _area._right + 1 ) * Info<float>::Grid::Size();
    tmp_rect._bottom = ( _area._bottom + 1 ) * Info<float>::Grid::Size();

    return tmp_rect.point_inside(xpos, ypos );
}

bool Passage::checkPassageMusic(const Object * pchr) const
{
    if (_music == INVALID_SOUND_ID)
    {
       return false; 
    } 

    if(!objectIsInPassage(pchr->getPosX(), pchr->getPosY(), pchr->bump_1.size))
    {
        return false;
    }

    // character is inside, start music track
    AudioSystem::get().playMusic(_music);
    return true;
}

void Passage::setMusic(const int32_t musicID)
{
    _music = musicID;
}

bool Passage::isShop() const
{
    return ObjectRef::Invalid != _shopOwner;
}

ObjectRef Passage::getShopOwner() const {
    return _shopOwner;
}

void Passage::makeShop(ObjectRef owner)
{
    //Make sure owner is valid
    const std::shared_ptr<Object> &powner = _module.getObjectHandler()[owner];
    if ( !powner || powner->isTerminated() || !powner->isAlive() ) return;

    //Mark as shop
    _isShop = true;
    _shopOwner = owner;

    // flag every item in the shop as a shop item
    for(const std::shared_ptr<Object> &object : _module.getObjectHandler().iterator())
    {
        if (object->isTerminated()) continue;

        if ( object->isitem )
        {
            if ( objectIsInPassage( object->getPosX(), object->getPosY(), object->bump_1.size ) )
            {
                object->isshopitem = true;               // Full value
                object->iskursed   = false;              // Shop items are never kursed
                object->nameknown  = true;
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