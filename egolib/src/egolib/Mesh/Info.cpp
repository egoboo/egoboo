#include "egolib/Mesh/Info.hpp"
#include "egolib/FileFormats/map_fx.hpp"

ego_mesh_info_t::ego_mesh_info_t()
	: ego_mesh_info_t(0, 0) {
}

ego_mesh_info_t::ego_mesh_info_t(const ego_mesh_info_t& other)
	: _tileCountX(other._tileCountX), _tileCountY(other._tileCountY),
	  _tileCount(other._tileCount),
	  _vertexCount(other._vertexCount) {

}

ego_mesh_info_t::ego_mesh_info_t(size_t tileCountX, size_t tileCountY)
	: ego_mesh_info_t(MAP_FAN_VERTICES_MAX * tileCountX * tileCountY, tileCountX, tileCountY) {
}

ego_mesh_info_t::ego_mesh_info_t(size_t vertexCount, size_t tileCountX, size_t tileCountY)
	: _vertexCount(vertexCount), _tileCountX(tileCountX), _tileCountY(tileCountY),
	  _tileCount(_tileCountX * _tileCountY) {
}

ego_mesh_info_t::~ego_mesh_info_t() {
}

ego_mesh_info_t& ego_mesh_info_t::operator=(const ego_mesh_info_t& other) {
	_tileCountX = other._tileCountX;
	_tileCountY = other._tileCountY;
	_tileCount = other._tileCount;
	_vertexCount = other._vertexCount;
	return *this;
}

void ego_mesh_info_t::reset() {
	_tileCountX = 0;
	_tileCountY = 0;
	_tileCount = 0;
	_vertexCount = 0;
}

void ego_mesh_info_t::reset(size_t tileCountX, size_t tileCountY) {
	_tileCountX = tileCountX;
	_tileCountY = tileCountY;
	_tileCount = _tileCountX * _tileCountY;
	_vertexCount = MAP_FAN_VERTICES_MAX * _tileCount;
}
