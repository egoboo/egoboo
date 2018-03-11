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

size_t MeshInfo::getTileCountX() const
{
    return _tileCountX;
}

size_t MeshInfo::getTileCountY() const
{
    return _tileCountY;
}

size_t MeshInfo::getTileCount() const
{
    return _tileCount;
}

size_t MeshInfo::getVertexCount() const
{
    return _vertexCount;
}

void MeshInfo::setVertexCount(size_t vertexCount)
{
    _vertexCount = vertexCount;
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

void MeshInfo::Iterator::increment()
{
    int endx = target->getTileCountX(),
        endy = target->getTileCountY();
    if (endx == 0 || endy == 0)
    {
        x = endx;
        y = endy;
    }
    // If x is not at the end, increment x.
    else if (x < endx)
    {
        x++;
        // If x is NOW at the end, increment y.
        if (x == endx)
        {
            // If y is not at the end, increment y.
            if (y < endy)
            {
                y++;
                // If y is not at the end, set x to begin.
                if (y < endy) x = 0;
            }
        }
    }
}

MeshInfo::Iterator::Iterator(size_t x, size_t y, MeshInfo *target)
    : x(x), y(y), target(target)
{
    int endx = target->getTileCountX(),
        endy = target->getTileCountY();
    if (endx == 0 || endy == 0)
    {
        this->x = endx;
        this->y = endy;
    }
}

MeshInfo::Iterator::Iterator(const Iterator& other)
    : x(other.x), y(other.y), target(other.target) {}

const MeshInfo::Iterator& MeshInfo::Iterator::operator=(const Iterator& rhs)
{
    x = rhs.x;
    y = rhs.y;
    target = rhs.target;
    return *this;
}

bool MeshInfo::Iterator::equal_to(const Iterator& rhs) const noexcept
{
    return x == rhs.x
        && y == rhs.y;
}

const Index2D MeshInfo::Iterator::operator*()
{ 
    return Index2D(x, y); 
}

const Index2D MeshInfo::Iterator::operator->()
{ 
    return Index2D(x, y); 
}

} // namespace Ego
