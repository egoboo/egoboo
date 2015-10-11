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