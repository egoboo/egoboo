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
//*
//********************************************************************************************

#include "cartman/Vertex.hpp"

namespace Cartman {

mpd_vertex_t::mpd_vertex_t() :
	next(CHAINEND),
	x(0.0f), y(0.0f), z(0.0f),
	a(VERTEXUNUSED)
{}

mpd_vertex_t::~mpd_vertex_t() {
	x = 0.0f; y = 0.0f; z = 0.0f;
	a = VERTEXUNUSED;
	next = CHAINEND;
}

void Cartman::mpd_vertex_t::reset() {
	x = 0.0f; y = 0.0f; z = 0.0f;
	a = VERTEXUNUSED;
	next = CHAINEND;
}

} // namespace Cartman