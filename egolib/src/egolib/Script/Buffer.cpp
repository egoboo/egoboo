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

/// @file egolib/Script/Buffer.cpp
/// @brief Dynamically resizing buffer for bytes.
/// @author Michael Heilmann

#include "egolib/Script/Buffer.hpp"

namespace Ego {
namespace Script {

Buffer::Buffer(size_t initialCapacity) :
    _size(0),
    _capacity(initialCapacity),
    _elements(new char[initialCapacity]) {
    //ctor
}

Buffer::~Buffer() {
    delete[] _elements;
}

size_t Buffer::getSize() const {
    return _size;
}

size_t Buffer::getCapacity() const {
    return _capacity;
}

size_t Buffer::getMaximalCapacity() const {
    return std::numeric_limits<size_t>::max();
}

void Buffer::increaseCapacity(size_t requiredAdditionalCapacity) {
    if (0 == requiredAdditionalCapacity) {
        return;
    }
    // Compute the maximum additional capacity.
    size_t maximumAdditionalCapacity = getMaximalCapacity() - getCapacity();
    // If the required additional capacity is greater than the maximum additional capacity ...
    if (maximumAdditionalCapacity < requiredAdditionalCapacity) {
        // ...raise an exception.
        Id::RuntimeErrorException(__FILE__, __LINE__, "required additional capacity exceeds maximum additional capacity");
    }
    // Compute the desired additional capacity.
    // Note that if getCapacity() * 2 overflows, then we have a wrap around. This does not cause any harm.
    size_t desiredAdditionalCapacity = getCapacity() > 0 ? getCapacity() * 2 : 8;
    // The desired additional capacity might be smaller, equal, or greater than the required additional capacity.
    // Take the maximum of both.
    size_t additionalCapacity = std::max(desiredAdditionalCapacity, requiredAdditionalCapacity);
    // The additional capacity may not exceed the maximum additional capacity.
    additionalCapacity = std::min(maximumAdditionalCapacity, additionalCapacity);

    // Compute the new capacity.
    size_t newCapacity = getCapacity() + additionalCapacity;
    
    char *newElements = new char[newCapacity];
    memcpy(newElements, _elements, _size);
    delete[] _elements;
    _elements = newElements;
    _capacity = newCapacity;
}

void Buffer::ensureFreeCapacity(size_t requiredFreeCapacity) {
    // (1) Compute the available free capacity.
    size_t availableFreeCapacity = getCapacity() - getSize();
    // (2) If the required free capacity is smaller than or equal to the available free capacity ...
    if (requiredFreeCapacity <= availableFreeCapacity) {
        // ... do nothing.
        return;
    }
    // (3) Otherwise: Compute the additional required free capacity which is guaranteed to be greater than 0 at this point.
    size_t additionalCapacity = requiredFreeCapacity - availableFreeCapacity;
    // (4) Increase the capacity by the additional capacity.
    increaseCapacity(additionalCapacity);
}

char Buffer::get(size_t index) const {
    if (index >= _size) {
        throw std::runtime_error("index ouf of bounds");
    }
    return _elements[index];
}

void Buffer::clear() {
    _size = 0;
}

std::string Buffer::toString() const {
    return std::string(_elements, _size);
}

void Buffer::prepend(char byte) {
    ensureFreeCapacity(1);
    memmove(_elements + 1, _elements, _size);
    _elements[0] = byte;
    _size++;
}

void Buffer::prepend(const char *bytes, size_t numberOfBytes) {
    if (0 == numberOfBytes) return;
    ensureFreeCapacity(numberOfBytes);
    memmove(_elements + numberOfBytes, _elements, _size);
    memcpy(_elements, bytes, numberOfBytes);
    _size += numberOfBytes;
}

void Buffer::append(char byte) {
    ensureFreeCapacity(1);
    _elements[_size++] = byte;
}

void Buffer::append(const char *bytes, size_t numberOfBytes) {
    if (0 == numberOfBytes) return;
    ensureFreeCapacity(numberOfBytes);
    memcpy(_elements + _size, bytes, numberOfBytes);
    _size += numberOfBytes;
}

void Buffer::insert(char byte, size_t index) {
    if (index > _size) {
        throw std::out_of_range("index out of range");
    }
    ensureFreeCapacity(1);
    size_t toCopy = _size - index;
    if (toCopy) {
        memmove(_elements + index + 1, _elements + index, toCopy);
    }
    _elements[index] = byte;
}

bool Buffer::isEmpty() const {
    return 0 == getSize();
}

void Buffer::swap(Buffer& other) noexcept {
    std::swap(_elements, other._elements);
    std::swap(_size, other._size);
    std::swap(_capacity, other._capacity);
}

} // namespace Script
} // namespace Ego
