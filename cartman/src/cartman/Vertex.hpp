#pragma once

#include "egolib/platform.h"

#define CHAINEND 0xFFFFFFFF     // End of vertex chain
#define VERTEXUNUSED 0          // Check mesh.vrta to see if used

namespace Cartman {

struct mpd_vertex_t {

	/**
	 * @brief
	 *  The next vertex in the fan of this vertex.
	 * @default
	 *  <tt>CHAINEND</tt>
	 */
	uint32_t next;

	/**
	 * @{
	 * @brief
	 *  The vertex position.
	 * @remark
	 *  @a z is sometimes referred to as the "elevation" of the vertex.
	 * @default
	 *  <tt>(0,0,0)</tt>
	 * @todo
	 *  Use a 3D vector type to represent the position.
	 */
	float x, y, z;
	/** @} */

	/**
	 * @brief
	 *  The basic light of the vertex.
	 * @remark
	 *  If this is @a VERTEXUNUSED then the basic light is ignored.
	 * @default
	 *  @a VERTEXUNUSED
	 */
	uint8_t a;

	/**
	 * @brief
	 *  Construct this vertex with its default values.
	 */
	mpd_vertex_t();

	/**
	 * @brief
	 *  Destruct this vertex
	 */
	virtual ~mpd_vertex_t();

	/**
	 * @brief
	 *  Reset this vertex to its default values.
	 */
	void reset();

};
} // namespace Cartman