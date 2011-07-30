//********************************************************************************************
//*
//*    This file is part of Cartman.
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

#include "cartman_select.h"

#include "cartman_mpd.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static select_lst_t _selection =  SELECT_LST_INIT;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
select_lst_t * select_lst_default()
{
    return &_selection;
}

//--------------------------------------------------------------------------------------------
select_lst_t * select_lst_init( select_lst_t * plst, const cartman_mpd_t * pmpd )
{
    // get proper list
    if( NULL == plst ) plst = &_selection;

    // get proper mesh
    if( NULL == pmpd ) pmpd = &mesh;

    // clear the list
    select_lst_clear( plst );

    // attach the correct mesh
    plst->pmesh = pmpd;

    return plst;
}

//--------------------------------------------------------------------------------------------
select_lst_t * select_lst_clear( select_lst_t * plst )
{
    // ZZ> This function unselects all vertices

    // get proper list
    if( NULL == plst ) plst = &_selection;

    plst->count    = 0;
    plst->which[0] = CHAINEND;

    return plst;
}

//--------------------------------------------------------------------------------------------
select_lst_t * select_lst_add( select_lst_t * plst, int vert )
{
    // ZZ> This function highlights a vertex

    int find_rv;

    // get proper list
    if( NULL == plst ) plst = &_selection;

    // is it in the list?
    find_rv = select_lst_find( plst, vert );
    if( find_rv < 0 )
    {
        // not found, so add it to the end
        plst->which[plst->count] = vert;
        plst->count++;

        if( plst->count < MPD_VERTICES_MAX )
        {
            plst->which[plst->count] = CHAINEND;
        }
    }

    return plst;
}

//--------------------------------------------------------------------------------------------
select_lst_t * select_lst_remove( select_lst_t * plst, int vert )
{
    // ZZ> This function makes sure the vertex is not highlighted
    int cnt, find_rv;

    // get proper list
    if( NULL == plst ) plst = &_selection;

    find_rv = select_lst_find( plst, vert );
    if( find_rv >= 0 )
    {
        // the vertex was found

        // shorten the gap
        if( plst->count > 1 )
        {
            for ( cnt = find_rv; cnt < plst->count-1; cnt++ )
            {
                plst->which[cnt] = plst->which[cnt-1];
            }
        }

        // blank out the last vertex
        plst->which[plst->count] = CHAINEND;

        // shorten the chain
        plst->count--;
    }

    return plst;
}

//--------------------------------------------------------------------------------------------
int select_lst_find( const select_lst_t * plst, int vert )
{
    // ZZ> This function returns btrue if the vertex has been highlighted by user

    int cnt, rv;

    // get proper list
    if( NULL == plst ) plst = &_selection;

    // a valid range?
    if( vert < 0 || vert >= MPD_VERTICES_MAX ) return -1;

    rv = -1;
    for ( cnt = 0; cnt < plst->count; cnt++ )
    {
        if ( vert == plst->which[cnt] )
        {
            rv = cnt;
        }
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
int select_lst_count( const select_lst_t * plst )
{
    // get proper list
    if( NULL == plst ) plst = &_selection;

    return plst->count;
}

//--------------------------------------------------------------------------------------------
select_lst_t * select_lst_synch_mesh( select_lst_t * plst, const cartman_mpd_t * pmesh )
{
    if( NULL == plst ) plst = &_selection;
    if( NULL == plst ) return plst;

    if( NULL == plst->pmesh ) plst->pmesh = pmesh;
    if( NULL == plst->pmesh ) plst->pmesh = &mesh;

    return plst;

}

//--------------------------------------------------------------------------------------------
select_lst_t * select_lst_set_mesh( select_lst_t * plst, const cartman_mpd_t * pmesh )
{
    if( NULL == plst  ) plst = &_selection;
    if( NULL == pmesh ) pmesh = &mesh;

    if( plst->pmesh != pmesh )
    {
        select_lst_init( plst, pmesh );
    }

    return plst;
}