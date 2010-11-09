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

/// @file egoboo_object.c
/// @brief Implementation of Egoboo "object" control routines
/// @details

#include "egoboo_object.h"

#include "egoboo_strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
obj_data_t * ego_object_ctor( obj_data_t * pbase )
{
    if ( NULL == pbase ) return pbase;

    memset( pbase, 0, sizeof( *pbase ) );

    pbase->_name[0] = CSTR_END;
    pbase->state    = ego_object_invalid;

    return pbase;
}

//--------------------------------------------------------------------------------------------
obj_data_t * ego_object_dtor( obj_data_t * pbase )
{
    if ( NULL == pbase ) return pbase;

    memset( pbase, 0, sizeof( *pbase ) );

    pbase->_name[0] = CSTR_END;
    pbase->state    = ego_object_invalid;

    return pbase;
}