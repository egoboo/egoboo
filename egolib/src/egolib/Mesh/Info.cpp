#include "egolib/Mesh/Info.hpp"
#include "egolib/FileFormats/map_fx.hpp"

namespace Ego {

MeshInfo::MeshInfo()
	: MeshInfo(0, 0) {
}

MeshInfo::MeshInfo(const MeshInfo& other)
	: _tileCountX(other._tileCountX), _tileCountY(other._tileCountY),
	  _tileCount(other._tileCount),
	  _vertexCount(other._vertexCount) {

}

MeshInfo::MeshInfo(size_t tileCountX, size_t tileCountY)
	: MeshInfo(MAP_FAN_VERTICES_MAX * tileCountX * tileCountY, tileCountX, tileCountY) {
}

MeshInfo::MeshInfo(size_t vertexCount, size_t tileCountX, size_t tileCountY)
	: _vertexCount(vertexCount), _tileCountX(tileCountX), _tileCountY(tileCountY),
	  _tileCount(_tileCountX * _tileCountY) {
}

MeshInfo::~MeshInfo() {
}

MeshInfo& MeshInfo::operator=(const MeshInfo& other) {
	_tileCountX = other._tileCountX;
	_tileCountY = other._tileCountY;
	_tileCount = other._tileCount;
	_vertexCount = other._vertexCount;
	return *this;
}

void MeshInfo::reset() {
	_tileCountX = 0;
	_tileCountY = 0;
	_tileCount = 0;
	_vertexCount = 0;
}

void MeshInfo::reset(size_t tileCountX, size_t tileCountY) {
	_tileCountX = tileCountX;
	_tileCountY = tileCountY;
	_tileCount = _tileCountX * _tileCountY;
	_vertexCount = MAP_FAN_VERTICES_MAX * _tileCount;
}

} // namespace Ego
