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

size_t Buffer::getMaxCapacity() const {
    return std::numeric_limits<size_t>::max();
}

void Buffer::increaseCapacity(size_t req) {
    if (0 == req) return;
    // This buffer is re-used and does not shrink.
    // Nevertheless we want to avoid frequent reallocations and if the requested additional capacity is too small,
    // add a bit more.
    size_t best = req;
    if (best < 5012) {
        best = 5012;
    } else {
        best = req;
    }
    // It is unlikely that the requested additional capacity ever exceeds
    // the maximum available additional capacity. However if it does ...
    if (getMaxCapacity() - best < _capacity) {
        // ... compute the available additional capacity.
        size_t avl = getMaxCapacity() - _capacity;
        // If the available additional capacity is smaller than then requested additional capacity,
        // raise an exception.
        if (avl < req) {
            throw std::bad_array_new_length();
        }
        best = avl;
    }
    char *newElements = new char[_capacity + best];
    memcpy(newElements, _elements, _size);
    delete[] _elements;
    _elements = newElements;
    _capacity += best;
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
    if (_size == _capacity) {
        increaseCapacity(1);
    }
    memmove(_elements, _elements + 1, _size);
    _elements[0] = byte;
    _size++;
}

void Buffer::append(char byte) {
    if (_size == _capacity) {
        increaseCapacity(1);
    }
    _elements[_size++] = byte;
}

void Buffer::append(const char *bytes, size_t numberOfBytes) {
    size_t maximalFreeCapacity = SIZE_MAX - _size;
    // If the maximal free capacity is smaller than the number of Bytes to append ...
    if (maximalFreeCapacity < numberOfBytes) {
        // ... then raise an exception.
        throw std::bad_array_new_length();
    }
    size_t capacity = _capacity;
    size_t requiredCapacity = _size + numberOfBytes;
    // If the capacity is smaller than the required capacity ...
    if (capacity < requiredCapacity) {
        // ... then increase the capacity by the difference of the required capacity and the capacity.
        increaseCapacity(requiredCapacity - capacity);
    }
    for (size_t i = 0, n = numberOfBytes; i < n; ++i) {
        _elements[i] = bytes[i];
    }
    _size += numberOfBytes;
}

void Buffer::insert(char byte, size_t index) {
    if (index > _size) {
        throw std::out_of_range("index out of range");
    }
    if (_size == _capacity) {
        increaseCapacity(1);
    }
    size_t toCopy = _size - index;
    if (toCopy) {
        memmove(_elements + index, _elements + index + 1, toCopy);
    }
    _elements[index] = byte;
}

bool Buffer::isEmpty() const {
    return 0 == getSize();
}

} // namespace Script
} // namespace Ego