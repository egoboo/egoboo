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

#pragma once

#include "cartman/cartman_typedef.h"
#include "cartman/Vertex.hpp"

//--------------------------------------------------------------------------------------------

struct select_lst_t
{
	/// The maximum number of points that can be selected.
	static constexpr size_t MAXSELECT = 2560;
private:
	/// The mesh to to which the selection applies.
    cartman_mpd_t *_pmesh;
	/// The actualnumber of points selected.
    int _count;
	/// An array of indices of selected points.
    uint32_t _which[MAXSELECT];
public:
	select_lst_t()
		: _pmesh(nullptr), _count(0), _which{ CHAINEND } {
	}
	static int at(select_lst_t& self, int index) {
		if (index < 0 || index >= self.count()) {
			throw id::runtime_error(__FILE__, __LINE__, "index out of bounds");
		}
		return self._which[index];
	}

	void init(cartman_mpd_t *pmesh);
	/**
	 * @brief
	 *  Ensure that no vertex is selected.
	 */
	void clear();
	/**
	 * @brief
	 *  Ensure that the specified vertex is selected.
	 * @param vertex
	 *  the index of the vertex
	 * @return
	 *  @a true if the vertex was not in the list,
	 *  @a false otherwise
	 */
	bool add(int vertex);
	/**
	 * @brief
	 *  Ensure that the specified vertex is not selected.
	 * @param vertex
	 *  the index of the vertex
	 * @return
	 *  @a true if the vertex was in the list,
	 *  @a false otherwise
	 */
	bool remove(int vertex);
	/**
	 * @brief
	 *  Search a vertex in this selection list.
	 * @param vertex
	 *  the vertex index
	 * @return
	 *  the index of the vertex in the selection list if it was found, -1 otherwise
	 */
	int find(int vertex) const;
	/**
	 * @brief
	 *  Get the number of vertices in this selection list.
	 * @return
	 *  the number of vertices in this selection list
	 */
	int count() const;

	void synch_mesh(cartman_mpd_t *mesh);
	/**
	 * @brief
	 *	Set the mesh of this selection list.
	 * @param mesh
	 *  a pointer to the mesh or a null pointer
	 */
	void set_mesh(cartman_mpd_t *mesh);
	/**
	 * @brief
	 *  Get the mesh of this selection list.
	 * @return
	 *  a pointer to the mesh or a  null pointer
	 */
	cartman_mpd_t *get_mesh();
};

//--------------------------------------------------------------------------------------------

select_lst_t& select_lst_default();

