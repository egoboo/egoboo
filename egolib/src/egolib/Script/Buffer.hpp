//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file   egolib/Script/Buffer.hpp
/// @brief  A text buffer.
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace Ego
{
namespace Script
{

/**
 * @brief
 *  A dynamically resizing buffer for bytes.
 * @author
 *  Michael Heilmann
 */
class Buffer : public Id::NonCopyable
{

private:

    /**
     * @brief
     *  The size of the buffer.
     */
    size_t _size;

    /**
     * @brief
     *  The capacity of the buffer.
     */
    size_t _capacity;

    /**
     * @brief
     *  The elements of this buffer.
     */
    char *_elements;

public:

    /**
     * @brief
     *  Construct this buffer with the specified initial capacity.
     * @param initialCapacity
     *  the initial capacity of the buffer
     * @throw std::bad_alloc
     *  if not enough memory is available
     */
    Buffer(size_t initialCapacity);

    /**
     * @brief
     *  Destruct this buffer.
     */
    ~Buffer();

    /**
     * @brief
     *  Get the size of this buffer.
     * @return
     *  the size
     */
    size_t getSize() const;

    /**
     * @brief
     *  Get the capacity of this buffer.
     * @return
     *  the capacity
     */
    size_t getCapacity() const;

    /**
     * @brief
     *  Get the maximum capacity of this buffer.
     * @return
     *  the maximum capacity
     */
    size_t getMaxCapacity() const;

    /**
     * @brief
     *  Increase the capacity by at least the specified additional required capacity.
     * @param req
     *  the additional capacity
     * @throw std::bad_array_new_length
     *  if the new capacity exceeds
     * @throw std::bad_alloc
     *  if not enough memory is available
     */
    void increaseCapacity(size_t req);

    /**
     * @brief
     *  Clear this buffer.
     */
    void clear();

    /**
     * @brief
     *  Get the contents of this buffer as a string.
     * @return
     *  the contents of this buffer as a string
     */
    std::string toString() const;

    /**
     * @brief
     *  Append a byte to this buffer growing this buffer if necessary.
     * @param byte
     *  the byte
     * @throw std::bad_alloc
     *  if not enough memory is available
     */
    void append(char byte);

	/**
	 * @brief
	 *	Insert a byte into the buffer at the specified index.
	 * @param byte
	 *	the byte
	 * @param index
	 *	the index
	 * @throw std::bad_alloc
	 *	if not enough memory is available
	 * @throw std::out_of_range
	 *	if @a index greater than the size of the buffer
	 */
	void insert(char byte, size_t index);

	/**
	 * @brief
	 *	Get if the buffer is empty.
	 * @return
	 *	@a true if the buffer is empty
	 */
	bool isEmpty() const;
};

} // namespace Script
} // namespace Ego
