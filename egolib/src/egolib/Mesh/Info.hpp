#pragma once

#include "egolib/Grid/Rect.hpp"

using IndexRect = Grid::IndexRectangle<int, Grid::CoordinateSystem::Grid>;
using Index2D = typename IndexRect::Index2Type;
using Index1D = typename IndexRect::Index1Type;

namespace Ego {

/// @brief Generic run-time description of a mesh.
struct MeshInfo {
protected:
	/// @brief The number of vertices in the mesh.
	size_t _vertexCount;

	/// @brief The size, in tiles, of this mesh along the x-axis.
	size_t _tileCountX;

	/// @brief The size, in tiles, of the mesh along the y-axis.
	size_t _tileCountY;

	/// @brief The number of tiles in the mesh.
	/// @invariant <tt>tileCount = tileCountX * tileCountY</tt>
	size_t _tileCount;

public:
    struct Iterator : public Id::Equatable<Iterator>,
                      public Id::Incrementable<Iterator>
    {
    private:
        size_t x, y;
        MeshInfo *target;

    protected:
        friend struct MeshInfo;
        Iterator(size_t x, size_t y, MeshInfo *target);
    public:
        Iterator(const Iterator& other);

    public:
        const Iterator& operator=(const Iterator& rhs);
    
		// CRTP
        void increment();
		// CRTP
        bool equalTo(const Iterator& other) const noexcept;

        const Index2D operator*();
        const Index2D operator->();
    };

public:
	/// @brief Construct this mesh information for a mesh with 0 tiles along the x- and the y-axes.
	MeshInfo();

	/// @brief Copy-construct this mesh information.
	/// @param other the construction source
	MeshInfo(const MeshInfo& other);

	/// @brief Construct this mesh information for a mesh with the specified number of tiles along the x- and y-axes.
	/// @param tileCountX the number of tiles along the x-axis
	/// @param tileCountY the number of tiles along the y-axis
	/// @remark The number of vertices is set to the maximum for the given number of tiles.
	MeshInfo(size_t tileCountX, size_t tileCountY);

	/// @brief Construct this mesh information for a mesh with the specified number of tiles along the x- and y-axes and the specified number of vertices.
	/// @param vertexCount the number of vertices
	/// @param tileCountX the number of tiles along the x-axis
	/// @param tileCountY the number of tiles along the y-axis
	MeshInfo(size_t vertexCount, size_t tileCountX, size_t tileCountY);

	//// @brief Assign this mesh information from another mesh information.
	//// @param other the assignment source
	MeshInfo& operator=(const MeshInfo& other);

	/// @brief Destruct this mesh information.
	virtual ~MeshInfo();

	/// @brief Get the size, in tiles, of the mesh along the x-axis.
	/// @return the size, in tiles, of the mesh along the x-axis
    size_t getTileCountX() const;

	/// @brief Get the size, in tiles, of the mesh along the y-axis.
	/// @return the size, in tiles, of the mesh along the y-axis
    size_t getTileCountY() const;

    /// @brief Get the number of tiles of the mesh.
    /// @return the number of tiles of the mesh, always <tt>getTileCountX()*getTileCountY()</tt>
    size_t getTileCount() const;

	/// @brief Get the number of vertices in the mesh.
    /// @return the number of vertices in the mesh
    size_t getVertexCount() const;

    /// @brief Set the number of vertices in the mesh.
    /// @param vertexCount the number of vertices in the mesh
    void setVertexCount(size_t vertexCount);

	virtual void reset();
	virtual void reset(size_t tileCountX, size_t tileCountY);

    /// @brief Get if an index is valid and within bounds.
    /// @param index the index
    /// @return @a true if the index is valid and within bounds
    bool isValid(const Index1D& index) const {
        return Index1D::Invalid != index
            && index < getTileCount()
            && index >= 0;
    }

    /// @brief Get if an index is valid and within bounds
    /// @param index the index
    /// @return @a true if the index is valid and within bounds
    bool isValid(const Index2D& index) const {
        return index.x() < getTileCountX()
            && index.y() < getTileCountY()
            && index.x() >= 0
            && index.y() >= 0;
    }

    /// @brief Assert that an index is valid and within bounds.
    /// @param index the index
    /// @throw Id::RuntimeErrorException if the index is not valid or not within bounds
    void assertValid(const Index1D& index) const {
        if (Index1D::Invalid == index) {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "invalid index");
        }
        if (index >= getTileCount() || index < 0) {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
        }
    }

    /// @brief Assert that an index is valid and within bounds.
    /// @param index the index
    /// @throw Id::RuntimeErrorException if the index is not valid or not within bounds
    void assertValid(const Index2D& index) const {
        if (index.x() >= getTileCountX() || index.x() < 0) {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
        }
        if (index.y() >= getTileCountY() || index.y() < 0) {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
        }
    }

    Index2D map(const Index1D& i) const {
        return Grid::map(i, (int)getTileCountX());
    }

    Index1D map(const Index2D& i) const {
        return Grid::map(i, (int)getTileCountX());
    }

public:
    /// @brief Get an iterator to the start of the tile indices.
    /// @return the iterator
    /// @remark the iterator iterates from left to right and top to bottom over the tile indices.
    Iterator begin() { return Iterator(0, 0, this); }

    /// @brief Get an iterator to the end of the tile indices.
    /// @return the iterator
    Iterator end() { return Iterator(getTileCountX(), getTileCountY(), this); }

};

} // namespace Ego
