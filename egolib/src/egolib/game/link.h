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

/// @file egolib/game/link.h

#pragma once

#include "egolib/game/egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define LINK_COUNT 16

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// The data needed to describe a single link between modules
struct Link_t
{
    bool valid;
    std::string modname;
    PASS_REF passage;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern std::array<Link_t, LINK_COUNT> LinkList;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Function prototypes
bool link_follow_modname( const std::string& modname, bool push_current_module );
bool link_build_vfs( const std::string& fname, std::array<Link_t, LINK_COUNT>& list );

bool link_pop_module();
bool link_load_parent( const char * modname, const Vector3f& pos );
