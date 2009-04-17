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

#include "link.h"
#include "menu.h"
#include "log.h"
#include "graphic.h"
#include "char.h"
#include "camera.h"

#include "egoboo_fileutil.h"
#include "egoboo.h"

#include "proto.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Link_t LinkList[LINK_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t link_follow_modname( const char * modname )
{
    // BB> This causes the game to follow a link, given the module name

    bool_t retval;

    if ( NULL == modname || '\0' == modname[0] ) return bfalse;

    // export all the local and remote characters
    game_update_imports();

    // quit the old module
    game_quit_module();

    // try to load the new module
    retval = game_init_module( modname, seed );

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t link_build( const char * fname, Link_t list[] )
{
    FILE * pfile;
    int i;
    if ( NULL == fname || '\0' == fname ) return bfalse;

    pfile = fopen( fname, "r" );
    if ( NULL == pfile ) return bfalse;

    i = 0;

    while ( goto_colon_yesno( pfile ) && i < LINK_COUNT )
    {
        fscanf( pfile, "%s", list[i].modname );
        list[i].valid = btrue;
        i++;
    }

    return i > 0;
}
