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