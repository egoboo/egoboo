#pragma once

#include "egolib/platform.h"

namespace Ego {

/**
 * @brief
 *  Generic run-time description of a mesh.
 */
struct MeshInfo {
protected:
	/**
	 * @brief
	 *  The number of vertices in the mesh.
	 */
	size_t _vertexCount;

	/**
	 * @brief
	 *  The size, in tiles, of this mesh along the x-axis.
	 */
	size_t _tileCountX;

	/**
	 * @brief
	 *  The size, in tiles, of the mesh along the y-axis.
	 */
	size_t _tileCountY;

	/**
	 * @brief
	 *  The number of tiles in the mesh.
	 * @invariant
	 *  <tt>tileCount = tileCountX * tileCountY</tt>
	 */
	size_t _tileCount;

public:
	/**
	 * @brief
	 *  Construct this mesh information for a mesh with 0 tiles along the x- and the y-axes.
	 */
	MeshInfo();

	/**
	 * @brief
	 *  Copy-construct this mesh information.
	 * @param other
	 *  the construction source
	 */
	MeshInfo(const MeshInfo& other);

	/**
	 * @brief
	 *  Construct this mesh information for a mesh with the specified number of tiles along the x- and y-axes.
	 * @param tileCountX
	 *  the number of tiles along the x-axis
	 * @param tileCountY
	 *  the number of tiles along the y-axis
	 * @remark
	 *  The number of vertices is set to the maximum for the given number of tiles.
	 */
	MeshInfo(size_t tileCountX, size_t tileCountY);

	/**
	 * @brief
	 *  Construct this mesh information for a mesh with the specified number of tiles along the x- and y-axes and the specified number of vertices.
	 * @param vertexCount
	 *  the number of vertices
	 * @param tileCountX
	 *  the number of tiles along the x-axis
	 * @param tileCountY
	 *  the number of tiles along the y-axis
	 */
	MeshInfo(size_t vertexCount, size_t tileCountX, size_t tileCountY);

	/**
	 * @brief
	 *  Assign this mesh information from another mesh information.
	 * @param other
	 *  the assignment source
	 */
	MeshInfo& operator=(const MeshInfo& other);

	/**
	 * @brief
	 *  Destruct this mesh information.
	 */
	virtual ~MeshInfo();

	/**
	 * @brief
	 *  Get the size, in tiles, of the mesh along the x-axis.
	 * @return
	 *  the size, in tiles, of the mesh along the x-axis
	 */
	size_t getTileCountX() const {
		return _tileCountX;
	}

	/**
	 * @brief
	 *  Get the size, in tiles, of the mesh along the y-axis.
	 * @return
	 *  the size, in tiles, of the mesh along the y-axis
	 */
	size_t getTileCountY() const {
		return _tileCountY;
	}

	/**
	 * @brief
	 *  Get the number of vertices of the mesh.
	 * @return
	 *  the number of vertices of the mesh
	 */
	size_t getVertexCount() const {
		return _vertexCount;
	}

	/**
	 * @brief
	 *  Get the number of tiles of the mesh.
	 * @return
	 *  the number of tiles of the mesh, always <tt>getTileCountX()*getTileCountY()</tt>
     */
	size_t getTileCount() const {
		return _tileCount;
	}

	virtual void reset();
	virtual void reset(size_t tileCountX, size_t tileCountY);

};

} // namespace Ego
