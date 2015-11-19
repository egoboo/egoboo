#pragma once

#include "egolib/platform.h"

#include "egolib/Grid/Index.hpp"
#include "egolib/Grid/Rect.hpp"

typedef Grid::Index<int, Grid::CoordinateSystem::Grid> Index2D;
typedef Grid::Index<int, Grid::CoordinateSystem::List> Index1D;
typedef Grid::Rect<int, Grid::CoordinateSystem::Grid> IndexRect;

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

    /**
     * @brief Get if an index is valid and within bounds.
     * @param index the index
     * @return @a true if the index is valid and within bounds
     */
    bool isValid(const Index1D& index) const {
        return Index1D::Invalid == index
            && index < getTileCount()
            && index >= 0;
    }

    /**
     * @brief Get if an index is valid and within bounds
     * @param index the index
     * @return @a true if the index is valid and within bounds
     */
    bool isValid(const Index2D& index) const {
        return index.getX() < getTileCountX()
            && index.getY() < getTileCountY()
            && index.getX() >= 0
            && index.getY() >= 0;
    }

    /**
     * @brief Assert that an index is valid and within bounds.
     * @param index the index
     * @throw Id::RuntimeErrorException if the index is not valid or not within bounds
     */
    void assertValid(const Index1D& index) const {
        if (Index1D::Invalid == index) {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "invalid index");
        }
        if (index >= getTileCount() || index < 0) {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
        }
    }
    /**
     * @brief Assert that an index is valid and within bounds.
     * @param index the index
     * @throw Id::RuntimeErrorException if the index is not valid or not within bounds
     */
    void assertValid(const Index2D& index) const {
        if (index.getX() >= getTileCountX() || index.getX() < 0) {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
        }
        if (index.getY() >= getTileCountY() || index.getY() < 0) {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
        }
    }

    Index2D map(const Index1D& i) const {
        return Grid::map(i, (int)getTileCountX());
    }

    Index1D map(const Index2D& i) const {
        return Grid::map(i, (int)getTileCountX());
    }

};

} // namespace Ego
