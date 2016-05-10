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

#include "cartman/Tile.hpp"

cartman_mpd_tile_t::cartman_mpd_tile_t() :
	type(0),
	tx_bits(MAP_FANOFF),
	twist(TWIST_FLAT),
	fx(MAPFX_WALL | MAPFX_IMPASS),
	vrtstart(MAP_FAN_ENTRIES_MAX) {
}

void cartman_mpd_tile_t::reset() {
	type = 0;
	tx_bits = MAP_FANOFF;
	twist = TWIST_FLAT;
	fx = MAPFX_WALL | MAPFX_IMPASS;
	vrtstart = MAP_FAN_ENTRIES_MAX;
}

bool cartman_mpd_tile_t::isPassableFloor() const
{
	static const uint8_t bits = MAPFX_WALL | MAPFX_IMPASS;
	return 0 == (this->fx & bits);
}

bool cartman_mpd_tile_t::isImpassableWall() const
{
	static const uint8_t bits = MAPFX_WALL | MAPFX_IMPASS;
	return bits == (fx & bits);
}

void cartman_mpd_tile_t::setImpassable() {
	fx |= MAPFX_IMPASS;
}

bool cartman_mpd_tile_t::isImpassable() const {
	return MAPFX_IMPASS == (fx & MAPFX_IMPASS);
}


void cartman_mpd_tile_t::setFX(uint8_t fx) {
	this->fx = fx;
}
uint8_t cartman_mpd_tile_t::getFX() const {
	return this->fx;
}