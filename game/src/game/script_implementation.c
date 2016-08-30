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

/// @file game/script_implementation.c
/// @brief implementation of the internal egoscript functions
/// @details These are the c-functions that are called to implement the egoscript commsnds
/// At some pont, the non-trivial commands in this file will be broken out to discrete functions
/// and wrapped by some external language (like Lua) using something like SWIG, and a cross compiler written.
/// The current code is about 3/4 of the way toward this goal.
/// The functions below will then be replaced with stub calls to the "real" functions.

#include "game/script_implementation.h"

#include "egolib/egolib.h"

#include "game/game.h"
#include "game/Entities/_Include.hpp"
#include "game/mesh.h"
#include "game/Module/Module.hpp"
#include "game/Module/Passage.hpp"

//--------------------------------------------------------------------------------------------
// wrap generic bitwise conversion macros
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint8  BIT_FIELD_clip_to_08_bits( BIT_FIELD val )  { return val & 0xFF;      }

//--------------------------------------------------------------------------------------------
Uint16 BIT_FIELD_clip_to_16_bits( BIT_FIELD val )  { return val & 0xFFFF;     }

//--------------------------------------------------------------------------------------------
Uint32 BIT_FIELD_clip_to_24_bits( BIT_FIELD val )  { return val & 0xFFFFFF;   }

//--------------------------------------------------------------------------------------------
Uint32 BIT_FIELD_clip_to_32_bits( BIT_FIELD val )  { return val & 0xFFFFFFFF; }

//--------------------------------------------------------------------------------------------
// wrap the BIT_FIELD macros, since lua doesn't recognize bitwise functions
//--------------------------------------------------------------------------------------------

BIT_FIELD BIT_FIELD_fill( BIT_FIELD val )
{
    return FULL_BIT_FIELD;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_empty( BIT_FIELD val )
{
    return EMPTY_BIT_FIELD;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_set_bits( BIT_FIELD val, BIT_FIELD bits )
{
    BIT_FIELD new_val = val;

    SET_BIT( new_val, bits );

    return new_val;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_unset_bits( BIT_FIELD val, BIT_FIELD bits )
{
    BIT_FIELD new_val = val;

    UNSET_BIT( new_val, bits );

    return new_val;
}

//--------------------------------------------------------------------------------------------
bool BIT_FIELD_has_some_bits( BIT_FIELD val, BIT_FIELD test )
{
    return HAS_SOME_BITS( val, test );
}

//--------------------------------------------------------------------------------------------
bool BIT_FIELD_has_all_bits( BIT_FIELD val, BIT_FIELD test )
{
    return HAS_ALL_BITS( val, test );
}

//--------------------------------------------------------------------------------------------
bool BIT_FIELD_has_no_bits( BIT_FIELD val, BIT_FIELD test )
{
    return HAS_NO_BITS( val, test );
}

//--------------------------------------------------------------------------------------------
bool BIT_FIELD_missing_bits( BIT_FIELD val, BIT_FIELD test )
{
    return HAS_SOME_BITS( val, test ) && !HAS_ALL_BITS( val, test );
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_set_all_bits( BIT_FIELD val, BIT_FIELD bits )
{
    BIT_FIELD new_val = val;

    SET_BIT( new_val, bits );

    return new_val;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_clear_all_bits( BIT_FIELD val, BIT_FIELD bits )
{
    BIT_FIELD new_val = val;

    UNSET_BIT( new_val, bits );

    return new_val;
}

//--------------------------------------------------------------------------------------------
bool BIT_FIELD_test_all_bits( BIT_FIELD val, BIT_FIELD bits )
{
    return HAS_ALL_BITS( val, bits );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool AddWaypoint( waypoint_list_t& wplst, ObjectRef ichr, float pos_x, float pos_y )
{
    // AddWaypoint( tmpx = "x position", tmpy = "y position" )
    /// @author ZZ
    /// @details This function tells the character where to move next

    bool returncode;

#if defined(_DEBUG) && defined(DEBUG_WAYPOINTS)
	Vector2f loc_pos;
    Vector3f nrm;
    float   pressure;

    // init the vector with the desired position
    loc_pos.x = pos_x;
    loc_pos.y = pos_y;

    // is this a safe position?
    returncode = false;

    ObjectProfile * profile = chr_get_ppro( ichr );
    if ( nullptr != profile )
    {
        if ( CAP_INFINITE_WEIGHT == profile->getWeight() || !ego_mesh_hit_wall( _currentModule->getMeshPointer(), loc_pos.v, pchr->bump.size, pchr->stoppedby, nrm.v, &pressure, NULL ) )
        {
			// yes it is safe. add it.
			returncode = true;
            waypoint_list_t::push( wplst, pos_x, pos_y );
        }
        else
        {
            // no it is not safe. what to do? nothing, or add the current position?
			//returncode = true;
			//waypoint_list_t::push( wplst, pchr->loc_pos.x, pchr->loc_pos.y );

            log_warning( "%s - failed to add a waypoint because object was \"inside\" a wall.\n"
                         "\tcharacter %d (\"%s\", \"%s\")\n"
                         "\tWaypoint index %d\n"
                         "\tWaypoint location (in tiles) <%f,%f>\n"
                         "\tWall normal <%1.4f,%1.4f>\n"
                         "\tPressure %f\n",
                         __FUNCTION__,
                         ichr, pchr->Name, profile->getClassName().c_str(),
                         plst->head,
                         loc_pos.x / GRID_FSIZE, loc_pos.y / GRID_FSIZE,
                         nrm.x, nrm.y,
                         SQRT( pressure ) / GRID_FSIZE );
        }
    }
#else
	returncode = true;
    waypoint_list_t::push( wplst, pos_x, pos_y );
#endif

    return returncode;
}

//--------------------------------------------------------------------------------------------
bool FindPath( waypoint_list_t& wplst, Object * pchr, float dst_x, float dst_y, bool * used_astar_ptr )
{
    // FindPath
    /// @author ZF
    /// @details Ported the A* path finding algorithm by birdsey and heavily modified it
    /// This function adds enough waypoints to get from one point to another

    int src_ix, src_iy;
    int dst_ix, dst_iy;
    line_of_sight_info_t los_info;
    bool straight_line;
    bool returncode = false;

    if ( NULL != used_astar_ptr )
    {
        *used_astar_ptr = false;
    }

    //Our current position
    src_ix = ( int )pchr->getPosX() / Info<int>::Grid::Size();
    src_iy = ( int )pchr->getPosY() / Info<int>::Grid::Size();

    //Destination position
    dst_ix = dst_x / Info<int>::Grid::Size();
    dst_iy = dst_y / Info<int>::Grid::Size();

    //Always clear any old waypoints
    waypoint_list_t::clear( wplst );

    //Don't do need to do anything if there is no need to move
    if ( src_ix == dst_ix && src_iy == dst_iy ) return false;

    returncode = false;

    //setup line of sight data for source
    los_info.stopped_by = pchr->stoppedby;
    los_info.x0 = pchr->getPosX();
    los_info.y0 = pchr->getPosY();
    los_info.z0 = 0;

    //setup line of sight to target
    los_info.x1 = dst_x;
    los_info.y1 = dst_y;
    los_info.z1 = 0;

    // test for the simple case... a straight line
    straight_line = !line_of_sight_info_t::blocked(los_info, _currentModule->getMeshPointer());

    if ( !straight_line )
    {
#ifdef DEBUG_ASTAR
        printf( "Finding a path from %d,%d to %d,%d: \n", src_ix, src_iy, dst_ix, dst_iy );
#endif
        //Try to find a path with the AStar algorithm
        if ( g_astar.find_path( _currentModule->getMeshPointer(), pchr->stoppedby, src_ix, src_iy, dst_ix, dst_iy ) )
        {
            returncode = g_astar.get_path( dst_x, dst_y, wplst);
        }

        if ( NULL != used_astar_ptr )
        {
            *used_astar_ptr = true;
        }
    }

    //failed to find a path
    if ( !returncode )
    {
        // just use a straight line path
        waypoint_list_t::push( wplst, dst_x, dst_y );
    }

    return returncode || straight_line;
}

//--------------------------------------------------------------------------------------------
bool Compass( Vector2f& pos, int facing, float distance )
{
    // Compass( tmpturn = "rotation", tmpdistance = "radius" )

    /// @author ZZ
    /// @details This function modifies tmpx and tmpy, depending on the setting of
    /// tmpdistance and tmpturn.  It acts like one of those Compass thing
    /// with the two little needle legs

    Facing turn = Facing(facing);

    pos[kX] -= std::cos(turn) * distance;
    pos[kY] -= std::sin(turn) * distance;

    return true;
}

//--------------------------------------------------------------------------------------------
Uint32 UpdateTime( Uint32 time_val, int delay )
{
    // UpdateTime( tmpargument = "time" )
    /// @author ZZ
    /// @details This function sets the character's ai timer.  50 clicks per second.
    /// Used in conjunction with IfTimeOut

    Uint32 new_time_val;

    if ( delay <= 0 )
    {
        new_time_val = time_val;
    }
    else
    {
        new_time_val = update_wld + delay;
    }

    return new_time_val;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint8 BreakPassage( int mesh_fx_or, const Uint16 become, const int frames, const int starttile, const int passageID, int *ptilex, int *ptiley )
{
    /// @author ZZ
    /// @details This function breaks the tiles of a passage if there is a character standing
    ///               on 'em.  Turns the tiles into damage terrain if it reaches last frame.

	auto mesh = _currentModule->getMeshPointer();
	if (!mesh) {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "nullptr == mesh");
	}

    const std::shared_ptr<Passage> &passage = _currentModule->getPassageByID(passageID);

    if ( !passage ) return false;

    // limit the start tile the the 256 tile images that we have
	int loc_starttile = Ego::Math::clipBits<8>( starttile );

    // same with the end tile
	Uint32 endtile = Ego::Math::constrain(loc_starttile + frames - 1, 0, 255);

	bool useful = false;
    for(const std::shared_ptr<Object> &pchr : _currentModule->getObjectHandler().iterator())
    {
        // nothing in packs
        if (pchr->isBeingHeld()) continue;

        // nothing flying
        if (pchr->isFlying()) continue;

		float lerp_z;
        lerp_z = ( pchr->getPosZ() - pchr->getObjectPhysics().getGroundElevation() ) / DAMAGERAISE;
        lerp_z = 1.0f - Ego::Math::constrain( lerp_z, 0.0f, 1.0f );

        if ( pchr->phys.weight * lerp_z <= 20 ) continue;

        Index1D fan = mesh->getTileIndex(Vector2f(pchr->getPosX(), pchr->getPosY()));

		ego_tile_info_t& ptile = mesh->getTileInfo(fan);
        {
            Uint16 img      = ptile._img & TILE_LOWER_MASK;
            Uint16 highbits = ptile._img & TILE_UPPER_MASK;

            if ( img >= loc_starttile && img < endtile )
            {
                if (passage->objectIsInPassage(pchr))
                {
                    // Remember where the hit occured.
                    *ptilex = pchr->getPosX();
                    *ptiley = pchr->getPosY();

                    useful = true;

                    // Change the tile image
                    img++;
                }
            }

            if ( img == endtile )
            {
                useful = mesh->add_fx( fan, mesh_fx_or );

                if ( become != 0 )
                {
                    img = become;
                }
            }

            if ( ptile._img != ( img | highbits ) )
            {
                mesh->set_texture( fan, img | highbits );
            }
        }
    }

    return useful;
}

//--------------------------------------------------------------------------------------------
Uint8 AddEndMessage( Object * pchr, const int message_index, script_state_t * pstate )
{
    /// @author ZZ
    /// @details This function appends a message to the end-module text

    if ( nullptr == pchr) return false;

    if ( !pchr->getProfile()->isValidMessageID( message_index ) ) return false;

    std::string escapedText = endtext + expandEscapeCodes(pchr->toSharedPointer(), *pstate, pchr->getProfile()->getMessage(message_index));
    strncpy(endtext, escapedText.c_str(), SDL_arraysize(endtext));

    endtext_carat = strlen(endtext);
    str_add_linebreaks(endtext, strlen(endtext), 30);

    return true;
}

//--------------------------------------------------------------------------------------------
Uint8 FindTileInPassage( const int x0, const int y0, const int tiletype, const int passageID, int *px1, int *py1 )
{
    /// @author ZZ
    /// @details This function finds the next tile in the passage, x0 and y0
    ///    must be set first, and are set on a find.  Returns true or false
    ///    depending on if it finds one or not

    const std::shared_ptr<Passage> &passage = _currentModule->getPassageByID(passageID);
    if ( !passage ) return false;

    int x = std::max<int>(x0, passage->getAxisAlignedBox2f().getMin().x()) / Info<int>::Grid::Size();
    int y = std::max<int>(y0, passage->getAxisAlignedBox2f().getMin().y()) / Info<int>::Grid::Size();
    int right = passage->getAxisAlignedBox2f().getMax().x() / Info<int>::Grid::Size();
    int bottom = passage->getAxisAlignedBox2f().getMax().y() / Info<int>::Grid::Size();

    // Do the first row
    if (y < bottom)
    {
        for ( /*nothing*/; x <= right; x++ )
        {
            Index1D fan = _currentModule->getMeshPointer()->getTileIndex(Index2D(x, y));

			ego_tile_info_t& ptile = _currentModule->getMeshPointer()->getTileInfo(fan);
            if (tiletype == ( ptile._img & TILE_LOWER_MASK ) )
            {
                *px1 = x * Info<float>::Grid::Size() + 64;
                *py1 = y * Info<float>::Grid::Size() + 64;
                return true;
            }
        }
        y++;
    }

    // Do all remaining rows
    for ( /* nothing */; y <= bottom; y++ )
    {
        for ( x = passage->getAxisAlignedBox2f().getMin().x() / Info<int>::Grid::Size(); x <= right; x++ )
        {
            Index1D fan = _currentModule->getMeshPointer()->getTileIndex(Index2D(x, y));

			ego_tile_info_t& ptile = _currentModule->getMeshPointer()->getTileInfo(fan);
            if (tiletype == ( ptile._img & TILE_LOWER_MASK ) )
            {
                *px1 = x * Info<float>::Grid::Size() + 64;
                *py1 = y * Info<float>::Grid::Size() + 64;
                return true;
            }
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------
Uint8 _display_message( const ObjectRef ichr, const PRO_REF iprofile, const int message, script_state_t * pstate )
{
    /// @author ZZ
    /// @details This function sticks a message_offset in the display queue and sets its timer

    const std::shared_ptr<ObjectProfile> &ppro = ProfileSystem::get().getProfile(iprofile);
    if ( !ppro->isValidMessageID(message) ) return false;

    std::string text = expandEscapeCodes(_currentModule->getObjectHandler()[ichr], *pstate, ppro->getMessage(message));
    DisplayMsg_print(text);

    return true;
}

//--------------------------------------------------------------------------------------------
ObjectRef FindWeapon( Object * pchr, float max_distance, const IDSZ2& weap_idsz, bool find_ranged, bool use_line_of_sight )
{
    /// @author ZF
    /// @details This function searches the nearby vincinity for a melee weapon the character can use

    ObjectRef retval = ObjectRef::Invalid;

    ObjectRef best_target = ObjectRef::Invalid;
    float   best_dist   = WIDE * WIDE;

    line_of_sight_info_t los;

    if (nullptr == pchr) {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "nullptr == pchr");
    }

    // set up the target
    best_target = ObjectRef::Invalid;
    best_dist   = SQR( max_distance );

    //setup line of sight data
    los.x0 = pchr->getPosX();
    los.y0 = pchr->getPosY();
    los.z0 = pchr->getPosZ();
    los.stopped_by = pchr->stoppedby;

    for(const std::shared_ptr<Object> &pweapon : _currentModule->getObjectHandler().iterator())
    {
        //only do items on the ground
        if ( _currentModule->getObjectHandler().exists( pweapon->attachedto ) || !pweapon->isitem ) continue;
        const std::shared_ptr<ObjectProfile> &weaponProfile = pweapon->getProfile();

        // only target those with a the given IDSZ
        if ( !pweapon->getProfile()->hasIDSZ(weap_idsz) ) continue;

        // ignore ranged weapons
        if ( !find_ranged && weaponProfile->isRangedWeapon() ) continue;

        // see if the character can use this weapon (we assume everyone has a left grip here)
        if ( ACTION_COUNT == pchr->getProfile()->getModel()->randomizeAction(weaponProfile->getWeaponAction(), SLOT_LEFT)) continue;

        // then check if a skill is needed
        if ( weaponProfile->requiresSkillIDToUse() )
        {
            if (!pchr->hasSkillIDSZ(pweapon->getProfile()->getIDSZ(IDSZ_SKILL))) continue;
        }

        //check distance
		Vector3f diff = pchr->getPosition() - pweapon->getPosition();
        float dist = diff.length_2();
        if ( dist < best_dist )
        {
            //finally, check line of sight. we only care for weapons we can see
            los.x1 = pweapon->getPosX();
            los.y1 = pweapon->getPosY();
            los.z1 = pweapon->getPosZ();

            if ( !use_line_of_sight || !line_of_sight_info_t::blocked(los, _currentModule->getMeshPointer()) )
            {
                //found a valid weapon!
                best_target = pweapon->getObjRef();
                best_dist = dist;
            }
        }
    }

    //Did we find anything?
    retval = ObjectRef::Invalid;
    if ( _currentModule->getObjectHandler().exists( best_target ) )
    {
        retval = best_target;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool FlashObject( Object * pchr, Uint8 value )
{
    /// @author ZZ
    /// @details This function sets a character's lighting
	if (!pchr || pchr->isTerminated()) {
		return false;
	}
	pchr->inst.flash(value);
	return true;
}

//--------------------------------------------------------------------------------------------
int RestockAmmo( const ObjectRef character, const IDSZ2& idsz )
{
    /// @author ZZ
    /// @details This function restocks the characters ammo, if it needs ammo and if
    ///    either its parent or type idsz match the given idsz.  This
    ///    function returns the amount of ammo given.

    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[character];
    if(!pchr) {
        return 0;
    }

    int amount = 0;
    if (pchr->getProfile()->hasTypeIDSZ(idsz))
    {
        if (pchr->ammo < pchr->ammomax)
        {
            amount = pchr->ammomax - pchr->ammo;
            pchr->ammo = pchr->ammomax;
        }
    }

    return amount;
}
