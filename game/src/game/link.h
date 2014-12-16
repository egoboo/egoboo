#pragma once

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

/// @file game/link.h

#include "game/egoboo_typedef.h"

#include "egolib/_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_link;
typedef struct s_link Link_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define LINK_COUNT 16

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// The data needed to describe a single link between modules
struct s_link
{
    ego_bool   valid;
    STRING   modname;
    PASS_REF passage;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern Link_t LinkList[LINK_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Function prototypes
ego_bool link_follow_modname( const char * modname, ego_bool push_current_module );
ego_bool link_build_vfs( const char * fname, Link_t list[] );

ego_bool link_pop_module( void );
ego_bool link_load_parent( const char * modname, fvec3_t pos );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _link_h
