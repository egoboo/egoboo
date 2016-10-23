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

/// @file egolib/Graphics/Buffer.hpp
/// @brief Abstract base class of all buffers.
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace Ego {

/// @brief The abstract base class of all vertex- and index buffers.
class Buffer : private Id::NonCopyable {
private:
    /// @brief The size, in Bytes, of this buffer.
    size_t size;

protected:
    /// @brief Construct this buffer.
    /// @param size the size, in Bytes, of this buffer
    Buffer(size_t size);

    /// @brief Destruct this buffer.
    virtual ~Buffer();

public:
    /// @brief Get the size, in Bytes, of this buffer.
    /// @return the size, in Bytes, of this buffer
    size_t getSize() const;

    /// @brief Lock this buffer.
    /// @return a pointer to the buffer data
    /// @throw Ego::Core::LockFailedException locking the buffer failed
    virtual void *lock() = 0;

    /// @brief Unlock this buffer.
    /// @remark If the buffer is not locked, a call to this method is a no-op.
    virtual void unlock() = 0;

}; // class Buffer

/// @brief Provides convenient RAII-style mechanism for locking/unlocking a buffers.
struct BufferScopedLock {
private:
    /// @brief A pointer to the backing memory of the buffer.
    void *pointer;

    /// @brief A pointer to the buffer.
    Ego::Buffer *buffer;

public:
    /// @brief Construct this buffer scoped lock, locking the buffer.
    /// @param buffer the buffer
    /// @throw Ego::Core::LockFailedException the buffer can not be locked
    BufferScopedLock(Ego::Buffer& buffer)
        : buffer(&buffer), pointer(buffer.lock()) {}

    /// @brief Destruct his buffer scoped lock, unlocking the buffer.
    ~BufferScopedLock() {
        buffer->unlock();
    }

    /// @brief Get a pointer to the backing memory of the vertex buffer.
    /// @return a pointer to the backing memory of the vertex buffer
    template <typename Type>
    Type *get() {
        return static_cast<Type *>(pointer);
    }

}; // struct BufferScopedLock

} // namespace Ego
