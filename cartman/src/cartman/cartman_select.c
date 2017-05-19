//********************************************************************************************
//*
//*    This file is part of Cartman.
//*
//*    Cartman is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Cartman is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Cartman.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "cartman/cartman_select.h"

#include "cartman/cartman_map.h"

//--------------------------------------------------------------------------------------------

/// The default selection list.
static select_lst_t g_selection;

/// Get the default selection list.
select_lst_t& select_lst_default()
{
    return g_selection;
}

//--------------------------------------------------------------------------------------------

void select_lst_t::init(cartman_mpd_t *pmesh)
{
    // get proper mesh
    if (!pmesh) pmesh = &mesh;

    // clear the list
    clear();

    // attach the correct mesh
    _pmesh = pmesh;
}

void select_lst_t::clear()
{
    _count = 0;
    _which[0] = CHAINEND;
}

bool select_lst_t::add(int vertex)
{
	if (!CART_VALID_VERTEX_RANGE(vertex)) {
		throw id::runtime_error(__FILE__, __LINE__, "vertex index out of bounds");
	}

    // Is the vertex index in the list?
    int index = find(vertex);
	if (-1 != index)
	{
		// The vertex is already in the list. => Do nothing and return false.
		return false;
	}
	else
	{
		// The vertex index is not in the list. => Append it and return true.
        _which[_count] = vertex;
        _count++;

        if (_count < MAP_VERTICES_MAX)
        {
			_which[_count] = CHAINEND;
        }
		return true;
    }
}

bool select_lst_t::remove(int vertex)
{
    int index = find(vertex);
	if (-1 == index)
	{
		// The vertex is not in the list. => Do nothing and return false.
		return false;
	}
	else
	{
        // The vertex is in the list. => Remove it and return true.
        if (_count > 1 )
        {
            for (int i = index; i < _count - 1; ++i)
            {
				_which[i] = _which[i-1];
            }
        }

        // blank out the last vertex
		_which[_count] = CHAINEND;

        // shorten the chain
        _count--;
		return true;
    }
}

int select_lst_t::find(int vertex) const
{
	if (!CART_VALID_VERTEX_RANGE(vertex)) {
		throw id::runtime_error(__FILE__, __LINE__, "vertex index out of bounds");
	}

    for (int i = 0; i < _count; ++i)
    {
        if (vertex == _which[i])
        {
            return i;
        }
    }

    return -1;
}

int select_lst_t::count() const
{
    return _count;
}

void select_lst_t::synch_mesh(cartman_mpd_t *pmesh)
{
    if ( NULL == _pmesh ) _pmesh = pmesh;
    if ( NULL == _pmesh ) _pmesh = &mesh;
}

void select_lst_t::set_mesh( cartman_mpd_t *pmesh )
{
    if (!pmesh) pmesh = &mesh;

    if (_pmesh != pmesh)
    {
        init(pmesh);
    }
}

cartman_mpd_t *select_lst_t::get_mesh() {
	return _pmesh;
}
