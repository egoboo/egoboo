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

#include "egoboo_typedef.h"
#include "egoboo_math.h"

#define LINK_COUNT 16

// The data needed to describe a single link between modules
struct s_link
{
    bool_t   valid;
    STRING   modname;
    PASS_REF passage;
};
typedef struct s_link Link_t;

extern Link_t LinkList[LINK_COUNT];

// Function prototypes
bool_t link_follow_modname( const char * modname, bool_t push_current_module );
bool_t link_build_vfs( const char * fname, Link_t list[] );

bool_t link_pop_module();
bool_t link_load_parent( const char * modname, fvec3_t   pos );

