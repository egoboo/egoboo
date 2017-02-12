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

/// @file egolib/Script/Buffer.hpp
/// @brief Dynamically resizing buffer for bytes.
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Script {

/// @brief A dynamically resizing buffer for bytes.
class Buffer : public Id::NonCopyable
{

private:
    std::deque<char> elements;

public:

    /// @brief Construct this buffer.
    /// @throw std::bad_alloc if not enough memory is available
    Buffer();

    /// @brief Destruct this buffer.
    virtual ~Buffer();

    /// @brief Get the size of this buffer.
    /// @return the size
    size_t getSize() const;

    /// @brief Clear this buffer.
    void clear();

    /// @brief Get the contents of this buffer as a string.
    /// @return the contents of this buffer as a string
    std::string toString() const;

    /// @brief Prepend a byte to this buffer growing this buffer if necessary.
    /// @param byte the byte
    /// @throw std::bad_array_new_length not enough memory is available
    void prepend(char byte);

    /// @brief Prepend bytes to this buffer growing the buffer if necessary.
    /// @param bytes a pointer to an array of @a numberOfBytes Bytes
    /// @param numberOfBytes the length, in Bytes, of the array pointed to by @a bytes
    /// @throw std::bad_array_new_length not enough memory is available
    /// @post All bytes have been prepended or the buffer was not modified.
    ///       Otherwise no bytes have been prepended.
    /// @throw std::bad_array_new_length if not enough memory is available
    /// @post
    /// All bytes have been prepended or the buffer was not modified.
    void prepend(const char *bytes, size_t numberOfBytes);

    /// @brief Append a byte to this buffer growing this buffer if necessary.
    /// @param byte the byte
    /// @throw std::bad_array_new_length not enough memory is available
    void append(char byte);

    /// @brief Append bytes to this buffer growing the buffer if necessary.
    /// @param bytes a pointer to an array of @a numberOfBytes Bytes
    /// @param numberOfBytes the length, in Bytes, of the array pointed to by @a bytes
    /// @throw std::bad_array_new_length not enough memory is available
    /// @post All bytes have been appended or the buffer was not modified.
    ///       Otherwise no bytes have been appended.
    /// @throw std::bad_array_new_length if not enough memory is available
    /// @post
    /// All bytes have been appended or the buffer was not modified.
    void append(const char *bytes, size_t numberOfBytes);

    /// @brief Get the Byte at the specified in index in this buffer.
    /// @param index the index
    /// @return the Byte
    /// @throw std::runtime_error the index is greater than or equal to the size of this buffer
    char get(size_t index) const;

    /// @brief Insert a byte into the buffer at the specified index.
    /// @param byte the byte
    /// @param index the index
    /// @throw std::bad_alloc not enough memory is available
    /// @throw std::out_of_range the index greater than the size of the buffer
    /// @remark For a buffer @a o and a byte @a x, the expressions
    /// @code
    /// o.insert(x, 0)
    /// @endcode
    /// and
    /// @code
    /// o.prepend(x)
    /// @endcode
    /// are equivalent.
    /// @remark For a buffer @a o and a byte @a x, the expressions
    /// @code
    /// o.insert(x, o.getSize())
    /// @endcode
    /// and
    /// @code
    /// o.append(x)
    /// @endcode
    /// are equivalent.
    void insert(char byte, size_t index);

    /// @brief Get if the buffer is empty.
    /// @return @a true if the buffer is empty, @a false otherwise
    bool isEmpty() const;

    /// @brief Exchanges the content of this buffer by the content of another buffer.
    /// @noexcept
    void swap(Buffer& other) noexcept;

    /// @brief Random access iterator.
    using iterator = typename std::deque<char>::iterator;
    
    /// @brief Constant random access iterator.
    using const_iterator = typename std::deque<char>::const_iterator;
    
    /// @brief Reverse random access iterator.
    using reverse_iterator = typename std::deque<char>::reverse_iterator;
    
    /// @brief Constant reverse random access iterator.
    using const_reverse_iterator = typename	std::deque<char>::const_reverse_iterator;

    /// @brief Get a random access iterator to the start of an iteration.
    /// @return the iterator
    iterator begin() { return elements.begin(); }

    /// @brief Get a random access iterator pointing to the end of an iteration.
    /// @return the iterator
    iterator end() { return elements.end(); }
    
    /// @brief Get a constant random access iterator pointing to the start of an iteration.
    /// @return the iterator
    const_iterator cbegin() const { return elements.cbegin(); }

    /// @brief Get a constant random access iterator to the end.
    /// @return the iterator
    const_iterator cend() const { return elements.cend(); }

    /// @brief Get a reverse random access iterator pointing to the start of a reverse iteration.
    /// @return the iterator
    reverse_iterator rbegin() { return elements.rbegin(); }

    /// @brief Get a reverse random access iterator pointing to the end of a reverse iteration.
    /// @return the iterator
    reverse_iterator rend() { return elements.rend(); }

    /// @brief Get a constant reverse random access iterator pointing to the start of a reverse iteration.
    /// @return the iterator
    const_reverse_iterator crbegin() const { return elements.crbegin(); }

    /// @brief Get a constant reverse randmo access iterator pointing to the end of a reverse iteration.
    /// @return the iterator
    const_reverse_iterator crend() const { return elements.crend(); }
};

} // namespace Script
} // namespace Ego
