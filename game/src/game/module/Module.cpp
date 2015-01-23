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

/// @file game/module/Module.cpp
/// @details Code handling a game module
/// @author Johan Jansen

#include "game/module/Module.hpp"
#include "game/network.h"
#include "egolib/math/Random.hpp"

GameModule::GameModule() :
		_name("*NONE*"),
        _importAmount(0),
        _exportValid(false),
        _exportReset(false),
        _playerAmount(1),
        _canRespawnAnyTime(false),
        _isRespawnValid(false),
        _isBeaten(false),
        _seed(std::numeric_limits<uint32_t>::max())
{
	//ctor
}

//--------------------------------------------------------------------------------------------
bool GameModule::setup( const mod_file_t * pdata, const std::string& name, const uint32_t seed )
{
    if ( nullptr == pdata ) return false;

    _importAmount   = pdata->importamount;
    _exportValid    = pdata->allowexport;
    _exportReset    = pdata->allowexport;
    _playerAmount   = pdata->maxplayers;
    _isRespawnValid   = ( false != pdata->respawnvalid );
    _canRespawnAnyTime = (RESPAWN_ANYTIME == pdata->respawnvalid);
    _name = name;
    _isBeaten = false;
    _seed = seed;

    return true;
}


//--------------------------------------------------------------------------------------------
bool GameModule::reset( const uint32_t seed )
{
    _isBeaten    = false;
    _exportValid = _exportReset;
    _seed        = seed;

    return true;
}

//--------------------------------------------------------------------------------------------
bool GameModule::start()
{
    active = true;

    srand( _seed );
    Random::setSeed(_seed);
    randindex = rand() % RANDIE_COUNT;

    egonet_set_hostactive( true ); // very important or the input will not work

    return true;
}

//--------------------------------------------------------------------------------------------
bool GameModule::stop()
{
    // network stuff
    egonet_set_hostactive( false );

    return true;
}
